#include "MusicMenuLite.h"
#include "Alarm.h"
#include "Buzzer.h" // 用于歌曲定义, numSongs, PlayMode枚举
#include "RotaryEncoder.h"
#include "Menu.h"
#include "weather.h" // 用于 getLocalTime
#include "Alarm.h"   // 用于 g_alarm_is_ringing
#include <freertos/task.h>

// --- 进度条颜色定义 ---
static const uint16_t song_colors[] = {
  TFT_CYAN, TFT_MAGENTA, TFT_YELLOW, TFT_GREEN, TFT_ORANGE, TFT_PINK, TFT_VIOLET
};
static const int num_song_colors = sizeof(song_colors) / sizeof(song_colors[0]);

// --- 任务和状态管理 ---
static TaskHandle_t musicLiteTaskHandle = NULL; // 音乐播放任务的句柄
static volatile bool stopMusicLiteTask = false; // 停止任务的标志
static volatile bool isPaused = false;          // 暂停标志

// --- UI和播放任务之间的共享状态 ---
// 使用 volatile 关键字确保在多任务环境下变量被正确访问
static volatile int shared_song_index = 0;           // 当前播放歌曲的索引
static volatile int shared_note_index = 0;           // 当前歌曲中正在播放的音符索引
static volatile int shared_total_notes = 0;          // 当前歌曲的总音符数
static volatile PlayMode shared_play_mode = LIST_LOOP; // 当前播放模式
static volatile TickType_t current_note_start_tick = 0; // 当前音符开始播放的时间点
static volatile int shared_current_note_duration = 0;  // 当前音符的时长
static volatile int shared_current_note_frequency = 0; // 当前音符的频率

// --- 函数前向声明 ---
static void MusicLite_Playback_Task(void *pvParameters);
static void displaySongList_Lite(int selectedIndex, int displayOffset);
static void displayPlayingScreen_Lite(uint16_t progress_bar_color);
static void stop_lite_playback();
static uint32_t calculateSongDuration_ms(int songIndex);
static uint32_t calculateElapsedTime_ms();

// --- 辅助函数 ---

/**
 * @brief 计算指定歌曲的总时长（毫秒）
 * @param songIndex 歌曲的索引
 * @return 歌曲的总时长（毫秒）
 */
static uint32_t calculateSongDuration_ms(int songIndex) {
    if (songIndex < 0 || songIndex >= numSongs) return 0;
    Song song;
    memcpy_P(&song, &songs[songIndex], sizeof(Song)); // 从PROGMEM中复制歌曲数据
    uint32_t total_ms = 0;
    for (int i = 0; i < song.length; i++) {
        total_ms += pgm_read_word(song.durations + i); // 累加每个音符的时长
    }
    return total_ms;
}

/**
 * @brief 计算当前歌曲已播放的时长（毫秒）
 * @return 已播放的时长（毫秒）
 */
static uint32_t calculateElapsedTime_ms() {
    if (shared_song_index < 0 || shared_song_index >= numSongs) return 0;
    Song song;
    memcpy_P(&song, &songs[shared_song_index], sizeof(Song));
    uint32_t elapsed_ms = 0;
    // 累加已播放音符的时长
    for (int i = 0; i < shared_note_index; i++) {
        elapsed_ms += pgm_read_word(song.durations + i);
    }
    // 加上当前正在播放音符所经过的时间
    TickType_t current_note_elapsed_ticks = xTaskGetTickCount() - current_note_start_tick;
    elapsed_ms += (current_note_elapsed_ticks * 1000) / configTICK_RATE_HZ;
    return elapsed_ms;
}

/**
 * @brief [FreeRTOS Task] 音乐播放后台任务
 * @param pvParameters 指向要播放的歌曲索引的指针
 */
void MusicLite_Playback_Task(void *pvParameters) {
    int songIndex = *(int*)pvParameters;

    for (;;) { // 无限循环以支持不同播放模式下的歌曲切换
        shared_song_index = songIndex;
        Song song;
        memcpy_P(&song, &songs[songIndex], sizeof(Song));
        shared_total_notes = song.length;
        shared_note_index = 0;

        // 遍历并播放当前歌曲的每个音符
        for (int i = 0; i < song.length; i++) {
            if (stopMusicLiteTask) { noTone(BUZZER_PIN); vTaskDelete(NULL); } // 检查停止标志

            while (isPaused) { // 如果暂停，则在此循环等待
                if (stopMusicLiteTask) { noTone(BUZZER_PIN); vTaskDelete(NULL); }
                noTone(BUZZER_PIN);
                shared_current_note_duration = 0;
                shared_current_note_frequency = 0;
                current_note_start_tick = xTaskGetTickCount(); // 暂停时重置开始时间，避免进度条跳跃
                vTaskDelay(pdMS_TO_TICKS(50));
            }

            shared_note_index = i;
            current_note_start_tick = xTaskGetTickCount();
            int note = pgm_read_word(song.melody + i);
            int duration = pgm_read_word(song.durations + i);

            // 更新共享变量以供UI显示
            shared_current_note_duration = duration;
            shared_current_note_frequency = note;

            if (note > 0) { tone(BUZZER_PIN, note, duration); }
            
            vTaskDelay(pdMS_TO_TICKS(duration)); // 等待音符时长
        }
        
        shared_current_note_duration = 0;
        shared_current_note_frequency = 0;

        vTaskDelay(pdMS_TO_TICKS(2000)); // 歌曲结束后暂停2秒

        // 根据播放模式决定下一首歌曲
        if (shared_play_mode == SINGLE_LOOP) {
            // 单曲循环，songIndex不变
        } else if (shared_play_mode == LIST_LOOP) {
            songIndex = (songIndex + 1) % numSongs; // 列表循环
        } else if (shared_play_mode == RANDOM_PLAY) {
            if (numSongs > 1) { // 随机播放
                int currentSong = songIndex;
                do { songIndex = random(numSongs); } while (songIndex == currentSong);
            }
        }
    }
}

/**
 * @brief 停止音乐播放相关的任务和硬件
 */
void stop_lite_playback() {
    if (musicLiteTaskHandle != NULL) {
        vTaskDelete(musicLiteTaskHandle);
        musicLiteTaskHandle = NULL;
    }
    noTone(BUZZER_PIN);
    isPaused = false;
    stopMusicLiteTask = false;
}

// --- UI绘制函数 ---

/**
 * @brief 绘制精简版的歌曲列表界面
 * @param selectedIndex 当前选中的歌曲索引
 * @param displayOffset 列表的滚动偏移量
 */
void displaySongList_Lite(int selectedIndex, int displayOffset) {
  menuSprite.fillScreen(TFT_BLACK);
  menuSprite.setTextColor(TFT_WHITE, TFT_BLACK);
  menuSprite.setTextSize(2);
  menuSprite.setTextDatum(MC_DATUM);
  menuSprite.drawString("Music Lite", 120, 28);
  
  const int visibleSongs = 3;
  for (int i = 0; i < visibleSongs; i++) {
    int songIdx = displayOffset + i;
    if (songIdx >= numSongs) break;
   
    int yPos = 60 + i * 50;
    
    if (songIdx == selectedIndex) { // 高亮选中的项目
      menuSprite.fillRoundRect(10, yPos - 18, 220, 36, 5, 0x001F); 
      menuSprite.setTextSize(2);
      menuSprite.setTextColor(TFT_WHITE, 0x001F);
      menuSprite.drawString(songs[songIdx].name, 120, yPos);
    } else {
      menuSprite.fillRoundRect(10, yPos - 18, 220, 36, 5, TFT_BLACK);
      menuSprite.setTextSize(1);
      menuSprite.setTextColor(TFT_WHITE, TFT_BLACK);
      menuSprite.drawString(songs[songIdx].name, 120, yPos);
    }
  }
  
  menuSprite.setTextDatum(TL_DATUM);
  menuSprite.pushSprite(0, 0);
}

/**
 * @brief 绘制正在播放的界面
 * @param progress_bar_color 进度条的颜色
 */
void displayPlayingScreen_Lite(uint16_t progress_bar_color) {
    uint32_t elapsed_ms = calculateElapsedTime_ms();
    uint32_t total_ms = calculateSongDuration_ms(shared_song_index);

    menuSprite.fillScreen(TFT_BLACK);
    menuSprite.setTextDatum(MC_DATUM);
    menuSprite.setTextColor(TFT_WHITE, TFT_BLACK);

    // 歌曲名称
    menuSprite.setTextSize(2);
    menuSprite.drawString(songs[shared_song_index].name, 120, 20);

    // 当前音符信息
    char noteInfoStr[30];
    snprintf(noteInfoStr, sizeof(noteInfoStr), "%d Hz  %d ms", shared_current_note_frequency, shared_current_note_duration);
    menuSprite.drawString(noteInfoStr, 120, 50);

    // 播放模式
    switch (shared_play_mode) {
        case SINGLE_LOOP: menuSprite.drawString("Single Loop", 120, 80); break;
        case LIST_LOOP: menuSprite.drawString("List Loop", 120, 80); break;
        case RANDOM_PLAY: menuSprite.drawString("Random", 120, 80); break;
    }

    // 系统时间
    extern struct tm timeinfo;
    if (getLocalTime(&timeinfo, 0)) {
        char timeStr[20];
        strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &timeinfo);
        menuSprite.drawString(timeStr, 120, 110);
    }

    // 播放/暂停状态
    menuSprite.drawString(isPaused ? "Paused" : "Playing", 120, 140);

    // 进度条
    int progressWidth = (total_ms > 0) ? (elapsed_ms * 220) / total_ms : 0;
    if (progressWidth > 220) progressWidth = 220;
    menuSprite.drawRoundRect(10, 165, 220, 10, 5, progress_bar_color);
    menuSprite.fillRoundRect(10, 165, progressWidth, 10, 5, progress_bar_color);

    // 时间显示 (mm:ss / mm:ss)
    char time_buf[20];
    snprintf(time_buf, sizeof(time_buf), "%02d:%02d / %02d:%02d", 
             (int)(elapsed_ms / 60000), (int)(elapsed_ms / 1000) % 60,
             (int)(total_ms / 60000), (int)(total_ms / 1000) % 60);
    menuSprite.drawString(time_buf, 120, 190);

    // 音符计数显示
    char note_count_buf[20];
    snprintf(note_count_buf, sizeof(note_count_buf), "%d / %d", shared_note_index + 1, shared_total_notes);
    menuSprite.drawString(note_count_buf, 120, 210);

    menuSprite.pushSprite(0, 0);
}

/**
 * @brief 精简版音乐菜单的主函数
 */
void MusicMenuLite() {
    int selectedSongIndex = 0;
    int displayOffset = 0;
    const int visibleSongs = 3;

    while (true) { // 主循环，在列表和播放界面间切换
        // --- 歌曲选择循环 ---
        displaySongList_Lite(selectedSongIndex, displayOffset);
        bool inListMenu = true;
        
        while(inListMenu) {
            if (g_alarm_is_ringing) return;

            int encoderChange = readEncoder();
            if (encoderChange != 0) { // 旋转编码器选择歌曲
                selectedSongIndex = (selectedSongIndex + encoderChange + numSongs) % numSongs;
                // 调整列表滚动
                if (selectedSongIndex < displayOffset) displayOffset = selectedSongIndex;
                else if (selectedSongIndex >= displayOffset + visibleSongs) displayOffset = selectedSongIndex - visibleSongs + 1;
                displaySongList_Lite(selectedSongIndex, displayOffset);
                tone(BUZZER_PIN, 1000, 50);
            }
            
            if (readButton()) { // 短按进入播放界面
                tone(BUZZER_PIN, 1500, 50);
                inListMenu = false;
            }

            if (readButtonLongPress()) return; // 长按退出
            vTaskDelay(pdMS_TO_TICKS(20));
        }

        // --- 播放界面循环 ---
        stopMusicLiteTask = false;
        isPaused = false;
        shared_play_mode = LIST_LOOP;
        
        if (musicLiteTaskHandle == NULL) { // 创建播放任务
            xTaskCreatePinnedToCore(MusicLite_Playback_Task, "MusicLite_Playback_Task", 4096, &selectedSongIndex, 2, &musicLiteTaskHandle, 0);
        }

        unsigned long lastScreenUpdateTime = 0;
        bool inPlayMenu = true;

        while(inPlayMenu) {
            if (g_alarm_is_ringing) { stop_lite_playback(); return; }

            if (readButtonLongPress()) { // 长按停止播放并返回列表
                stop_lite_playback();
                inPlayMenu = false;
            }

            if (readButton()) { // 短按暂停/继续
                isPaused = !isPaused;
                tone(BUZZER_PIN, 1000, 50);
                lastScreenUpdateTime = 0; // 强制刷新屏幕
            }

            int encoderChange = readEncoder();
            if (encoderChange != 0) { // 旋转编码器切换播放模式
                int mode = (int)shared_play_mode;
                mode = (mode + encoderChange + 3) % 3;
                shared_play_mode = (PlayMode)mode;
                tone(BUZZER_PIN, 1200, 50);
                lastScreenUpdateTime = 0; // 强制刷新屏幕
            }

            // 定期更新屏幕
            if (millis() - lastScreenUpdateTime > 200) {
                uint16_t current_color = song_colors[shared_song_index % num_song_colors];
                displayPlayingScreen_Lite(current_color);
                lastScreenUpdateTime = millis();
            }

            vTaskDelay(pdMS_TO_TICKS(20));
        }
    }
}

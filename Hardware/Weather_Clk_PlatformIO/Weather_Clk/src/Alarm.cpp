// 包含核心库和模块头文件
#include "Alarm.h"          // 本模块的头文件
#include "Menu.h"           // 主菜单，用于访问全局TFT sprite
#include "RotaryEncoder.h"  // 旋转编码器，用于用户输入
#include "Buzzer.h"         // 蜂鸣器，用于播放闹钟铃声
#include "weather.h"        // 天气模块，用于获取和显示时间
#include "MQTT.h"           // MQTT模块，用于访问全局退出标志
#include <EEPROM.h>         // EEPROM库，用于持久化存储闹钟设置
#include <freertos/task.h>  // FreeRTOS任务管理
#include <pgmspace.h>       // PROGMEM宏，用于将数据存储在闪存中

// --- 宏定义 ---
#define MAX_ALARMS 10           // 支持的最大闹钟数量
#define EEPROM_START_ADDR 0     // EEPROM中存储闹钟数据的起始地址
#define EEPROM_MAGIC_KEY 0xAD   // 用于验证EEPROM数据有效性的“魔术数字”
#define ALARMS_PER_PAGE 5       // 闹钟列表每页显示的数量

// --- 全局状态变量 ---
// 闹钟是否正在响铃的全局标志，volatile确保在中断或多任务环境下被正确访问
volatile bool g_alarm_is_ringing = false;

// --- 内部枚举和结构体 ---
// 编辑闹钟时的不同模式
enum EditMode { EDIT_HOUR, EDIT_MINUTE, EDIT_DAYS, EDIT_SAVE, EDIT_DELETE };

// --- 模块内部静态变量 ---
static AlarmSetting alarms[MAX_ALARMS]; // 存储所有闹钟设置的数组
static int alarm_count = 0;             // 当前已设置的闹钟数量
static int last_checked_day = -1;       // 上次检查闹钟触发是星期几，用于每日重置状态

// --- FreeRTOS任务句柄 ---
static TaskHandle_t alarmMusicTaskHandle = NULL; // 闹钟音乐播放任务的句柄
static volatile bool stopAlarmMusic = false;     // 用于从外部停止音乐播放任务的标志

// --- UI状态变量 ---
static int list_selected_index = 0;     // 在闹钟列表中当前选中的项
static int list_scroll_offset = 0;      // 列表的滚动偏移量，用于翻页
static unsigned long last_click_time = 0; // 上次按钮点击的时间，用于区分单击和双击
static const unsigned long DOUBLE_CLICK_WINDOW = 350; // 双击的时间窗口（毫秒）

// =====================================================================================
//                                 内部函数前向声明
// =====================================================================================
static void saveAlarms();
static void Alarm_Delete(int index);
static void editAlarm(int index);
static void drawAlarmList();
void Alarm_Loop_Check();


// =====================================================================================
//                                     UI绘制逻辑
// =====================================================================================

/**
 * @brief 绘制闹钟列表界面
 * @details 在TFT sprite上绘制所有闹钟的列表，包括启用状态、时间、重复日期。
 *          同时显示当前选中的项目、当前时间和翻页指示器。
 */
static void drawAlarmList() {
    menuSprite.fillScreen(TFT_BLACK); // 清空sprite
    menuSprite.setTextFont(1);
    menuSprite.setTextSize(2);
    menuSprite.setTextDatum(TL_DATUM);
    menuSprite.setTextColor(TFT_WHITE);

    // 显示当前日期和时间
    extern struct tm timeinfo;
    char titleBuf[30];
    if (timeinfo.tm_year >= 100) { // Check if time is valid
        strftime(titleBuf, sizeof(titleBuf), "%Y-%m-%d %H:%M:%S %a", &timeinfo);
    } else {
        strcpy(titleBuf, "Time not synced");
    }
    menuSprite.drawString(titleBuf, 10, 10);
    
    // 如果闹钟数量超过一页，显示翻页指示器
    if (alarm_count > ALARMS_PER_PAGE) {
        char scrollIndicator[10];
        int currentPage = list_scroll_offset / ALARMS_PER_PAGE + 1;
        int totalPages = (alarm_count + ALARMS_PER_PAGE - 1) / ALARMS_PER_PAGE;
        snprintf(scrollIndicator, sizeof(scrollIndicator), "%d/%d", currentPage, totalPages);
        menuSprite.drawString(scrollIndicator, 200, 10);
    }
    
    menuSprite.drawFastHLine(0, 32, 240, TFT_DARKGREY); // 标题栏下方的分隔线

    // 计算当前页面应显示的闹钟范围
    int start_index = list_scroll_offset;
    int end_index = min(list_scroll_offset + ALARMS_PER_PAGE, alarm_count + 1);
    
    // 遍历并绘制每个闹钟条目
    for (int i = start_index; i < end_index; i++) {
        int display_index = i - list_scroll_offset;
        int y_pos = 45 + (display_index * 38);
        
        // 高亮显示当前选中的条目
        if (i == list_selected_index) {
            menuSprite.drawRoundRect(5, y_pos - 5, 230, 36, 5, TFT_YELLOW);
        }

        if (i < alarm_count) { // 如果是已存在的闹钟
            // 绘制启用/禁用状态的复选框
            int box_x = 15, box_y = y_pos - 2;
            menuSprite.drawRect(box_x, box_y, 20, 20, TFT_WHITE);
            if (alarms[i].enabled) { // 启用状态画对勾
                menuSprite.drawLine(box_x + 4, box_y + 10, box_x + 8, box_y + 14, TFT_GREEN);
                menuSprite.drawLine(box_x + 8, box_y + 14, box_x + 16, box_y + 6, TFT_GREEN);
            } else { // 禁用状态画叉
                menuSprite.drawLine(box_x + 4, box_y + 4, box_x + 16, box_y + 16, TFT_RED);
                menuSprite.drawLine(box_x + 16, box_y + 4, box_x + 4, box_y + 16, TFT_RED);
            }

            // 绘制闹钟时间
            char buf[20];
            snprintf(buf, sizeof(buf), "%02d:%02d", alarms[i].hour, alarms[i].minute);
            menuSprite.drawString(buf, 50, y_pos);

            // 绘制重复日期 (S M T W T F S)
            const char* days[] = {"S", "M", "T", "W", "T", "F", "S"};
            for(int d=0; d<7; d++){
                // 如果某天被选中，则用绿色显示，否则用灰色
                menuSprite.setTextColor((alarms[i].days_of_week & (1 << d)) ? TFT_GREEN : TFT_DARKGREY);
                menuSprite.drawString(days[d], 140 + (d * 12), y_pos);
            }
            menuSprite.setTextColor(TFT_WHITE); // 恢复默认颜色
        } else if (i == alarm_count && i < MAX_ALARMS) { // 如果是列表末尾的“添加”选项
            menuSprite.drawString("+ Add New Alarm", 15, y_pos);
        }
    }
    menuSprite.pushSprite(0, 0); // 将绘制好的sprite内容推送到屏幕
}

/**
 * @brief 绘制闹钟编辑界面
 * @param alarm 要编辑的闹钟设置
 * @param mode 当前的编辑模式（时、分、日期等）
 * @param day_cursor 在编辑日期模式下，当前光标所在的星期位置
 */
static void drawEditScreen(const AlarmSetting& alarm, EditMode mode, int day_cursor) {
    menuSprite.fillScreen(TFT_BLACK);
    menuSprite.setTextDatum(MC_DATUM);

    // 绘制大号的时间数字
    menuSprite.setTextFont(7); menuSprite.setTextSize(1);
    char time_buf[6];
    sprintf(time_buf, "%02d:%02d", alarm.hour, alarm.minute);
    menuSprite.drawString(time_buf, 120, 80);

    // 根据编辑模式，在时或分下方绘制高亮下划线
    if (mode == EDIT_HOUR) menuSprite.fillRect(38, 115, 70, 4, TFT_YELLOW);
    else if (mode == EDIT_MINUTE) menuSprite.fillRect(132, 115, 70, 4, TFT_YELLOW);

    // 绘制星期选择
    menuSprite.setTextFont(1);
    const char* days[] = {"S","M", "T", "W", "T", "F", "S"};
    for(int d=0; d<7; d++){
        int day_x = 24 + (d * 30); int day_y = 160;
        // 高亮显示当前选中的星期
        if (mode == EDIT_DAYS && d == day_cursor) menuSprite.drawRect(day_x - 10, day_y - 12, 20, 24, TFT_YELLOW);
        // 已选中的星期用绿色，未选中的用灰色
        menuSprite.setTextColor((alarm.days_of_week & (1 << d)) ? TFT_GREEN : TFT_DARKGREY);
        menuSprite.drawString(days[d], day_x, day_y);
    }

    // 绘制“保存”和“删除”按钮
    int save_box_y = 205;
    menuSprite.setTextFont(1); menuSprite.setTextColor(TFT_WHITE);
    if (mode == EDIT_SAVE) { // 高亮“保存”按钮
        menuSprite.fillRoundRect(40, save_box_y, 75, 30, 5, TFT_GREEN);
        menuSprite.setTextColor(TFT_BLACK);
    }
    menuSprite.drawRoundRect(40, save_box_y, 75, 30, 5, TFT_WHITE);
    menuSprite.drawString("SAVE", 78, save_box_y + 15);

    menuSprite.setTextColor(TFT_WHITE);
    if (mode == EDIT_DELETE) { // 高亮“删除”按钮
        menuSprite.fillRoundRect(125, save_box_y, 75, 30, 5, TFT_RED);
        menuSprite.setTextColor(TFT_BLACK);
    }
    menuSprite.drawRoundRect(125, save_box_y, 75, 30, 5, TFT_WHITE);
    menuSprite.drawString("DELETE", 163, save_box_y + 15);

    menuSprite.pushSprite(0, 0); // 推送到屏幕
}

// =====================================================================================
//                                 后台逻辑 (EEPROM, 任务)
// =====================================================================================

/**
 * @brief 将所有闹钟设置保存到EEPROM
 */
static void saveAlarms() {
  EEPROM.write(EEPROM_START_ADDR, EEPROM_MAGIC_KEY); // 写入魔术数字，表示数据有效
  EEPROM.put(EEPROM_START_ADDR + 1, alarms);         // 写入整个闹钟数组
  EEPROM.commit();                                   // 提交更改
}

/**
 * @brief 从EEPROM加载闹钟设置
 */
static void loadAlarms() {
  // 检查魔术数字是否匹配
  if (EEPROM.read(EEPROM_START_ADDR) == EEPROM_MAGIC_KEY) {
    EEPROM.get(EEPROM_START_ADDR + 1, alarms); // 读取闹钟数组
    // 重新计算有效的闹钟数量
    alarm_count = 0;
    for (int i = 0; i < MAX_ALARMS; ++i) if (alarms[i].hour != 255) alarm_count++;
  } else { // 如果魔术数字不匹配，说明是首次运行或数据损坏
    memset(alarms, 0xFF, sizeof(alarms)); // 用无效值(255)填充数组
    for(int i=0; i<MAX_ALARMS; ++i) alarms[i].enabled = false;
    alarm_count = 0;
    saveAlarms(); // 保存这个初始化的空状态
  }
}

/**
 * @brief 删除指定索引的闹钟
 * @param index 要删除的闹钟的索引
 */
static void Alarm_Delete(int index) {
    if (index < 0 || index >= alarm_count) return; // 索引检查
    // 将后面的闹钟向前移动，覆盖被删除的闹钟
    for (int i = index; i < alarm_count - 1; i++) {
        alarms[i] = alarms[i + 1];
    }
    alarm_count--; // 闹钟总数减一
    // 清理数组末尾的旧数据
    memset(&alarms[alarm_count], 0xFF, sizeof(AlarmSetting));
    alarms[alarm_count].enabled = false;
    saveAlarms(); // 保存更改
}

/**
 * @brief [FreeRTOS Task] 闹钟响铃时循环播放音乐的任务
 * @param pvParameters 未使用
 */
void Alarm_MusicLoop_Task(void *pvParameters) {
    while (true) {
        // 循环播放音乐库中的所有歌曲
        for (int i = 0; i < numSongs; i++) {
            if (stopAlarmMusic) { // 检查停止标志
                vTaskDelete(NULL); // 如果置位，则删除任务
            }

            Song currentSong;
            memcpy_P(&currentSong, &songs[i], sizeof(Song)); // 从PROGMEM中复制歌曲数据

            // 播放当前歌曲的每个音符
            for (int j = 0; j < currentSong.length; j++) {
                if (stopAlarmMusic) {
                    noTone(BUZZER_PIN); // 立即停止声音
                    vTaskDelete(NULL);
                }
                int note = pgm_read_word(&currentSong.melody[j]); // 读取音符频率
                int duration = pgm_read_word(&currentSong.durations[j]); // 读取音符时长
                
                tone(BUZZER_PIN, note, duration * 0.9); // 播放音符，留出10%的间隔
                vTaskDelay(pdMS_TO_TICKS(duration)); // 等待音符时长
            }
            vTaskDelay(pdMS_TO_TICKS(500)); // 每首歌之间暂停500ms
        }
    }
}

/**
 * @brief 触发指定索引的闹钟
 * @param index 要触发的闹钟的索引
 */
static void triggerAlarm(int index) {
  alarms[index].triggered_today = true; // 标记为今天已触发
  saveAlarms();
  Serial.printf("ALARM %d TRIGGERED! PLAYING MUSIC...\n", index);

  // 如果已有音乐任务在运行，先删除它
  if (alarmMusicTaskHandle != NULL) {
      vTaskDelete(alarmMusicTaskHandle);
      alarmMusicTaskHandle = NULL;
  }

  exitSubMenu = true;       // 设置全局退出子菜单标志，让当前活动的功能退出
  g_alarm_is_ringing = true; // 设置全局响铃标志
  stopAlarmMusic = false;   // 重置音乐停止标志
  
  // 创建新的音乐播放任务
  xTaskCreatePinnedToCore(Alarm_MusicLoop_Task, "AlarmMusicLoopTask", 8192, NULL, 1, &alarmMusicTaskHandle, 0);
}

/**
 * @brief 停止正在播放的闹钟音乐。
 * @details 此函数用于在用户操作后停止闹钟音乐的播放，并重置响铃状态标志。
 */
void Alarm_StopMusic() {
    if (alarmMusicTaskHandle != NULL) {
        stopAlarmMusic = true; // 设置停止标志
        vTaskDelay(pdMS_TO_TICKS(50)); // 等待任务自行删除
        alarmMusicTaskHandle = NULL;
    }
    noTone(BUZZER_PIN); // 确保声音立即停止
    g_alarm_is_ringing = false; // 重置全局响铃标志
    Serial.println("Alarm music stopped by user.");
    // 恢复菜单的默认字体，避免UI混乱
    menuSprite.setTextFont(1);
    menuSprite.setTextSize(1);
}

/**
 * @brief 初始化闹钟系统。
 * @details 此函数从EEPROM中加载之前保存的闹钟设置。
 *          同时，它会创建一个后台FreeRTOS任务，用于周期性地检查是否有闹钟需要触发。
 */
void Alarm_Init() {
  loadAlarms(); // 从EEPROM加载闹钟
  last_checked_day = -1; // 重置上次检查日期
  
  void Alarm_Check_Task(void *pvParameters); // 前向声明检查任务
  // 创建一个后台任务，用于周期性检查闹钟
  xTaskCreate(Alarm_Check_Task, "Alarm Check Task", 2048, NULL, 5, NULL);
}

/**
 * @brief [FreeRTOS Task] 周期性检查闹钟的任务
 * @param pvParameters 未使用
 */
void Alarm_Check_Task(void *pvParameters) {
    for(;;) {
        Alarm_Loop_Check(); // 调用核心检查逻辑
        vTaskDelay(pdMS_TO_TICKS(1000)); // 每秒检查一次
    }
}

/**
 * @brief 周期性检查闹钟触发。
 * @details 此函数应被一个定时任务反复调用。它会检查当前时间是否与任何已启用的
 *          闹钟匹配。如果匹配，并且当天尚未触发过，它将调用 `triggerAlarm` 函数。
 */
void Alarm_Loop_Check() {
  extern struct tm timeinfo;
  if (timeinfo.tm_year < 100) return; // 如果时间未同步，则不检查
  
  int current_day = timeinfo.tm_wday;
  // 如果日期变化，重置所有闹钟的“今日已触发”标志
  if (last_checked_day != current_day) {
    for (int i = 0; i < alarm_count; ++i) { alarms[i].triggered_today = false; }
    last_checked_day = current_day;
    saveAlarms();
  }
  
  // 遍历所有闹钟
  for (int i = 0; i < alarm_count; ++i) {
    if (g_alarm_is_ringing) break; // 如果已有闹钟在响，则不触发新的
    // 检查闹钟是否启用、今天尚未触发
    if (alarms[i].enabled && !alarms[i].triggered_today) {
      bool day_match = alarms[i].days_of_week & (1 << current_day); // 检查星期是否匹配
      // 检查时和分是否匹配
      if (day_match && alarms[i].hour == timeinfo.tm_hour && alarms[i].minute == timeinfo.tm_min) {
        triggerAlarm(i); // 触发闹钟
      }
    }
  }
}

// =====================================================================================
//                                     UI交互逻辑
// =====================================================================================

/**
 * @brief 显示闹钟响铃界面。
 * @details 当一个闹钟被触发时，此函数负责显示一个全屏界面，
 *          提示用户时间已到，并播放音乐。用户可以通过按下按钮来停止闹钟。
 */
void Alarm_ShowRingingScreen() {
    menuSprite.fillScreen(TFT_BLACK);
    menuSprite.setTextDatum(MC_DATUM);

    // 绘制静态文本
    menuSprite.setTextColor(TFT_YELLOW);
    menuSprite.setTextFont(4);
    menuSprite.drawString("Time's Up!", 120, 120);
    menuSprite.setTextFont(2);
    menuSprite.setTextColor(TFT_WHITE);
    menuSprite.drawString("Press to Stop", 120, 180);

    unsigned long last_time_update = 0;
    bool initial_draw = true;

    // 循环直到闹钟被停止
    while (g_alarm_is_ringing) {
        if (readButton()) { // 检测按钮点击
            tone(BUZZER_PIN, 1500, 100); // 提示音
            Alarm_StopMusic(); // 停止闹钟
        }

        // 每秒更新一次屏幕上的时间
        if (millis() - last_time_update >= 1000 || initial_draw) {
            last_time_update = millis();
            initial_draw = false;

            extern struct tm timeinfo;
            if (getLocalTime(&timeinfo, 0)) {
                char time_buf[30];
                strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %a", &timeinfo);
                menuSprite.setTextFont(4);
                menuSprite.setTextColor(TFT_WHITE, TFT_BLACK);
                menuSprite.drawString(time_buf, 120, 60);
                
                strftime(time_buf, sizeof(time_buf), "%H:%M:%S", &timeinfo);
                menuSprite.drawString(time_buf, 120, 90);
            }
             menuSprite.pushSprite(0, 0);
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

/**
 * @brief 进入并处理闹钟编辑界面
 * @param index 要编辑的闹钟索引。如果索引超出当前闹钟数量，则为创建新闹钟。
 */
static void editAlarm(int index) {
    AlarmSetting temp_alarm;
    bool is_new_alarm = (index >= alarm_count);

    // 如果是新闹钟，使用默认值；否则加载现有闹钟数据
    if (is_new_alarm) temp_alarm = {12, 0, 0, true, false};
    else temp_alarm = alarms[index];

    EditMode edit_mode = EDIT_HOUR;
    int day_cursor = 0;

    drawEditScreen(temp_alarm, edit_mode, day_cursor);

    while(true) {
        if (g_alarm_is_ringing) { return; } // 如果在编辑时有其他闹钟响起，则退出

        if (readButtonLongPress()) { // 长按退出编辑
            menuSprite.setTextFont(1); 
            menuSprite.setTextSize(2);
            return; 
        }

        int encoder_value = readEncoder(); // 读取编码器旋转
        if (encoder_value != 0) {
            tone(BUZZER_PIN, 1000, 20); // 旋转提示音
            switch(edit_mode) {
                case EDIT_HOUR: temp_alarm.hour = (temp_alarm.hour + encoder_value + 24) % 24; break;
                case EDIT_MINUTE: temp_alarm.minute = (temp_alarm.minute + encoder_value + 60) % 60; break;
                case EDIT_DAYS: day_cursor = (day_cursor + encoder_value + 7) % 7; break;
                case EDIT_SAVE:
                case EDIT_DELETE: // 在保存和删除之间切换
                    if (encoder_value > 0) edit_mode = EDIT_DELETE;
                    if (encoder_value < 0) edit_mode = EDIT_SAVE;
                    break;
                default: break;
            }
            drawEditScreen(temp_alarm, edit_mode, day_cursor); // 重新绘制界面
        }

        if (readButton()) { // 读取按钮短按
            tone(BUZZER_PIN, 2000, 50);
            if (edit_mode == EDIT_DAYS) { // 在星期编辑模式下，单击是选中/取消选中
                temp_alarm.days_of_week ^= (1 << day_cursor);
            } else if (edit_mode == EDIT_SAVE) { // 单击“保存”
                if (is_new_alarm) {
                    alarms[alarm_count] = temp_alarm;
                    alarm_count++;
                } else {
                    alarms[index] = temp_alarm;
                }
                saveAlarms();
                return; // 保存并退出
            } else if (edit_mode == EDIT_DELETE) { // 单击“删除”
                if (!is_new_alarm) Alarm_Delete(index);
                return; // 删除并退出
            }
            // 切换到下一个编辑模式
            edit_mode = (EditMode)((edit_mode + 1) % 5);
            drawEditScreen(temp_alarm, edit_mode, day_cursor);
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

/**
 * @brief 显示并管理闹钟设置菜单。
 * @details 该函数是闹钟设置功能的用户界面入口。它允许用户通过旋转编码器和按钮
 *          来浏览、启用/禁用、添加、编辑和删除闹钟。
 */
void AlarmMenu() {
    list_selected_index = 0;
    list_scroll_offset = 0;
    int click_count = 0;
    unsigned long last_update_time = 0;
    const unsigned long UPDATE_INTERVAL = 1000;
    drawAlarmList();

    while (true) {
        if (g_alarm_is_ringing) { return; } // 如果闹钟响起，退出菜单

        // 每秒更新一次列表上的时间显示
        unsigned long current_time = millis();
        if (current_time - last_update_time >= UPDATE_INTERVAL) {
            extern struct tm timeinfo;
            if (!getLocalTime(&timeinfo)) {
                Serial.println("Failed to obtain time");
            }
            drawAlarmList();
            last_update_time = current_time;
        }

        if (readButtonLongPress()) { // 长按退出
            return; 
        }

        int encoder_value = readEncoder(); // 旋转编码器来选择
        if (encoder_value != 0) {
            int max_items = (alarm_count < MAX_ALARMS) ? alarm_count + 1 : MAX_ALARMS;
            list_selected_index = (list_selected_index + encoder_value + max_items) % max_items;
            
            // 处理翻页
            if (list_selected_index < list_scroll_offset) {
                list_scroll_offset = list_selected_index;
            } else if (list_selected_index >= list_scroll_offset + ALARMS_PER_PAGE) {
                list_scroll_offset = list_selected_index - ALARMS_PER_PAGE + 1;
            }
            
            drawAlarmList();
            tone(BUZZER_PIN, 1000 + 50 * list_selected_index, 20);
        }

        if (readButton()) { click_count++; last_click_time = millis(); }

        // 处理单击和双击
        if (click_count > 0 && millis() - last_click_time > DOUBLE_CLICK_WINDOW) {
            if (click_count == 1) { // 单击
                tone(BUZZER_PIN, 2000, 50);
                if (list_selected_index < alarm_count) { // 对已存在的闹钟，单击是启用/禁用
                    alarms[list_selected_index].enabled = !alarms[list_selected_index].enabled;
                    saveAlarms();
                } else { // 对“添加”选项，单击是进入编辑界面
                    editAlarm(alarm_count); 
                }
                drawAlarmList();
            }
            click_count = 0;
        }
        
        if (click_count >= 2) { // 双击
            tone(BUZZER_PIN, 2500, 50);
            if (list_selected_index < alarm_count) { // 对已存在的闹钟，双击是进入编辑界面
                editAlarm(list_selected_index);
                drawAlarmList();
            }
            click_count = 0;
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

// =====================================================================================
//                                     公共GETTER函数
// =====================================================================================

/**
 * @brief 获取当前设置的闹钟数量。
 * @return 返回当前存储的闹钟总数。
 */
int getAlarmCount() {
    return alarm_count;
}

/**
 * @brief 获取指定闹钟的详细信息。
 * @param index 要查询的闹钟的索引。
 * @param[out] settings 用于接收闹钟设置信息的结构体引用。
 * @return 如果索引有效，则返回true并填充settings结构体；否则返回false。
 */
bool getAlarmInfo(int index, AlarmSetting& settings) {
    if (index < 0 || index >= alarm_count) {
        return false;
    }
    settings = alarms[index];
    return true;
}

/**
 * @brief 添加或更新一个闹钟。
 * @param index 要更新的闹钟的索引 (0-9)。如果超出当前闹钟数量，则会添加新闹钟（如果未满）。
 * @param hour 小时 (0-23)。
 * @param minute 分钟 (0-59)。
 * @param days_of_week 星期选择的位掩码 (e.g., 1<<0 for Sunday)。
 * @param enabled 闹钟是否启用。
 * @return 如果操作成功，返回true。
 */
bool Alarm_Update(int index, uint8_t hour, uint8_t minute, uint8_t days_of_week, bool enabled) {
    if (index < 0 || index >= MAX_ALARMS) {
        return false; // 索引超出最大范围
    }
    if (hour > 23 || minute > 59) {
        return false; // 时间无效
    }

    // 如果索引指向一个新闹钟
    if (index >= alarm_count) {
        alarm_count = index + 1; // 更新闹钟数量
    }

    // 更新闹钟设置
    alarms[index].hour = hour;
    alarms[index].minute = minute;
    alarms[index].days_of_week = days_of_week;
    alarms[index].enabled = enabled;
    alarms[index].triggered_today = false; // 重置触发状态

    // 保存到EEPROM
    saveAlarms();

    return true;
}
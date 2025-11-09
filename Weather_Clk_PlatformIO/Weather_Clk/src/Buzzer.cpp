#include "Buzzer.h"
#include "Alarm.h"
#include "RotaryEncoder.h"
#include <TFT_eSPI.h>
#include <time.h>
#include "LED.h"
#include <math.h>
#include "Menu.h"
#include "MQTT.h"
#include "weather.h"
#include "Alarm.h"
#include <freertos/task.h>

// --- Task Handles ---
TaskHandle_t buzzerTaskHandle = NULL;
TaskHandle_t ledTaskHandle = NULL;

// --- Playback State ---
PlayMode currentPlayMode = LIST_LOOP;
volatile bool stopBuzzerTask = false;
volatile bool isPaused = false;
volatile bool stopLedTask = false;

// --- Shared state for UI ---
static volatile int shared_song_index = 0;
static volatile int shared_note_index = 0;
static volatile TickType_t current_note_start_tick = 0;

// --- Song Data ---

const Song songs[] PROGMEM = {
  { "Ai Ni", melody_ai_ni, durations_ai_ni, sizeof(melody_ai_ni) / sizeof(melody_ai_ni[0]), 0 },
  { "Ai Qing Xun Xi", melody_ai_qing_xun_xi, durations_ai_qing_xun_xi, sizeof(melody_ai_qing_xun_xi) / sizeof(melody_ai_qing_xun_xi[0]), 0 },
  { "A Peng You Zai Jian", melody_a_peng_you_zai_jian, durations_a_peng_you_zai_jian, sizeof(melody_a_peng_you_zai_jian) / sizeof(melody_a_peng_you_zai_jian[0]), 0 },
  { "Ba Hui Yi Pin Hao Gei Ni", melody_ba_hui_yi_pin_hao_gei_ni, durations_ba_hui_yi_pin_hao_gei_ni, sizeof(melody_ba_hui_yi_pin_hao_gei_ni) / sizeof(melody_ba_hui_yi_pin_hao_gei_ni[0]), 0 },
  { "Bao Wei Huang He", melody_bao_wei_huang_he, durations_bao_wei_huang_he, sizeof(melody_bao_wei_huang_he) / sizeof(melody_bao_wei_huang_he[0]), 0 },
  { "Bu De Bu Ai", melody_bu_de_bu_ai, durations_bu_de_bu_ai, sizeof(melody_bu_de_bu_ai) / sizeof(melody_bu_de_bu_ai[0]), 0 },
  { "Bu Zai You Yu", melody_bu_zai_you_yu, durations_bu_zai_you_yu, sizeof(melody_bu_zai_you_yu) / sizeof(melody_bu_zai_you_yu[0]), 0 },
  { "Cai Bu Tou", melody_cai_bu_tou, durations_cai_bu_tou, sizeof(melody_cai_bu_tou) / sizeof(melody_cai_bu_tou[0]), 0 },
  { "Casablanca", melody_casablanca, durations_casablanca, sizeof(melody_casablanca) / sizeof(melody_casablanca[0]), 2 },
  { "Ce Lian", melody_ce_lian, durations_ce_lian, sizeof(melody_ce_lian) / sizeof(melody_ce_lian[0]), 0 },
  { "Cheng Du", melody_cheng_du, durations_cheng_du, sizeof(melody_cheng_du) / sizeof(melody_cheng_du[0]), 2 },
  { "Chong Er Fei", melody_chong_er_fei, durations_chong_er_fei, sizeof(melody_chong_er_fei) / sizeof(melody_chong_er_fei[0]), 2 },
  { "Counting Stars", melody_counting_stars, durations_counting_stars, sizeof(melody_counting_stars) / sizeof(melody_counting_stars[0]), 0 },
  { "Dao Shu", melody_dao_shu, durations_dao_shu, sizeof(melody_dao_shu) / sizeof(melody_dao_shu[0]), 0 },
  { "Chun Jiao Yu Zhi Ming", melody_chun_jiao_yu_zhi_ming, durations_chun_jiao_yu_zhi_ming, sizeof(melody_chun_jiao_yu_zhi_ming) / sizeof(melody_chun_jiao_yu_zhi_ming[0]), 1 },
  { "Da Hai", melody_da_hai, durations_da_hai, sizeof(melody_da_hai) / sizeof(melody_da_hai[0]), 2 },
  { "Dao Xiang", melody_dao_xiang, durations_dao_xiang, sizeof(melody_dao_xiang) / sizeof(melody_dao_xiang[0]), 2 },
  { "Dong Fang Hong", melody_dong_fang_hong, durations_dong_fang_hong, sizeof(melody_dong_fang_hong) / sizeof(melody_dong_fang_hong[0]), 2 },
  { "Dong Fang Zhi Zhu", melody_dong_fang_zhi_zhu, durations_dong_fang_zhi_zhu, sizeof(melody_dong_fang_zhi_zhu) / sizeof(melody_dong_fang_zhi_zhu[0]), 2 },
  { "Dong Xi", melody_dong_xi, durations_dong_xi, sizeof(melody_dong_xi) / sizeof(melody_dong_xi[0]), 2 },
  { "Dream Wedding", melody_dream_wedding, durations_dream_wedding, sizeof(melody_dream_wedding) / sizeof(melody_dream_wedding[0]), 2 },
  { "Duo Yuan Dou Yao Zai Yi Qi", melody_duo_yuan_dou_yao_zai_yi_qi, durations_duo_yuan_dou_yao_zai_yi_qi, sizeof(melody_duo_yuan_dou_yao_zai_yi_qi) / sizeof(melody_duo_yuan_dou_yao_zai_yi_qi[0]), 0 },
  { "Fan Fang Xiang De Zhong", melody_fan_fang_xiang_de_zhong, durations_fan_fang_xiang_de_zhong, sizeof(melody_fan_fang_xiang_de_zhong) / sizeof(melody_fan_fang_xiang_de_zhong[0]), 2 },
  { "Fate", melody_fate, durations_fate, sizeof(melody_fate) / sizeof(melody_fate[0]), 2 },
  { "Five Hundred Miles", melody_five_hundred_miles, durations_five_hundred_miles, sizeof(melody_five_hundred_miles) / sizeof(melody_five_hundred_miles[0]), 2 },
  { "For Elise", melody_for_elise, durations_for_elise, sizeof(melody_for_elise) / sizeof(melody_for_elise[0]), 2 },
  { "For Ya", melody_for_ya, durations_for_ya, sizeof(melody_for_ya) / sizeof(melody_for_ya[0]), 0 },
  { "Fu Shi Shan Xia", melody_fu_shi_shan_xia, durations_fu_shi_shan_xia, sizeof(durations_fu_shi_shan_xia) / sizeof(durations_fu_shi_shan_xia[0]), 2 },
  { "Gao Bai Qi Qiu", melody_gao_bai_qi_qiu, durations_gao_bai_qi_qiu, sizeof(melody_gao_bai_qi_qiu) / sizeof(melody_gao_bai_qi_qiu[0]), 2 },
  { "Gang Hao Yu Jian Ni", melody_gang_hao_yu_jian_ni, durations_gang_hao_yu_jian_ni, sizeof(melody_gang_hao_yu_jian_ni) / sizeof(melody_gang_hao_yu_jian_ni[0]), 2 },
  { "Ge Chang Zu Guo", melody_ge_chang_zu_guo, durations_ge_chang_zu_guo, sizeof(melody_ge_chang_zu_guo) / sizeof(melody_ge_chang_zu_guo[0]), 2 },
  { "Guang Hui Sui Yue", melody_guang_hui_sui_yue, durations_guang_hui_sui_yue, sizeof(melody_guang_hui_sui_yue) / sizeof(melody_guang_hui_sui_yue[0]), 0 },
  { "Guo Huo", melody_guo_huo, durations_guo_huo, sizeof(melody_guo_huo) / sizeof(melody_guo_huo[0]), 0 },
  { "Hai Kuo Tian Kong", melody_hai_kuo_tian_kong, durations_hai_kuo_tian_kong, sizeof(melody_hai_kuo_tian_kong) / sizeof(melody_hai_kuo_tian_kong[0]), 3 },
  { "He Tang Yue Se", melody_he_tang_yue_ye, durations_he_tang_yue_ye, sizeof(melody_he_tang_yue_ye) / sizeof(melody_he_tang_yue_ye[0]), 2 },
  { "Hong Dou", melody_hong_dou, durations_hong_dou, sizeof(melody_hong_dou) / sizeof(melody_hong_dou[0]), 4 },
  { "Hong Se Gao Gen Xie", melody_hong_se_gao_gen_xie, durations_hong_se_gao_gen_xie, sizeof(melody_hong_se_gao_gen_xie) / sizeof(melody_hong_se_gao_gen_xie[0]), 0 },
  { "Hou Lai", melody_hou_lai, durations_hou_lai, sizeof(melody_hou_lai) / sizeof(melody_hou_lai[0]), 0 },
  { "Hua Hai", melody_hua_hai, durations_hua_hai, sizeof(melody_hua_hai) / sizeof(melody_hua_hai[0]), 0 },
  { "Jiang Nan", melody_jiang_nan, durations_jiang_nan, sizeof(melody_jiang_nan) / sizeof(melody_jiang_nan[0]), 0 },
  { "Kai Shi Dong Le", melody_kai_shi_dong_le, durations_kai_shi_dong_le, sizeof(melody_kai_shi_dong_le) / sizeof(melody_kai_shi_dong_le[0]), 1 },
  { "Ke Bu Ke Yi", melody_ke_bu_ke_yi, durations_ke_bu_ke_yi, sizeof(melody_ke_bu_ke_yi) / sizeof(melody_ke_bu_ke_yi[0]), 0 },
  { "Lan Ting Xu", melody_lan_ting_xu, durations_lan_ting_xu, sizeof(melody_lan_ting_xu) / sizeof(melody_lan_ting_xu[0]), 2 },
  { "Liang Zhu", melody_liang_zhu, durations_liang_zhu, sizeof(melody_liang_zhu) / sizeof(melody_liang_zhu[0]), 2 },
  { "Lv Se", melody_lv_se, durations_lv_se, sizeof(melody_lv_se) / sizeof(melody_lv_se[0]), 2 },
  { "Ming Tian Hui Geng Hao", melody_ming_tian_hui_geng_hao, durations_ming_tian_hui_geng_hao, sizeof(melody_ming_tian_hui_geng_hao) / sizeof(melody_ming_tian_hui_geng_hao[0]), 0 },
  { "Nan Fang Gu Niang", melody_nan_fang_gu_niang, durations_nan_fang_gu_niang, sizeof(melody_nan_fang_gu_niang) / sizeof(melody_nan_fang_gu_niang[0]), 2 },
  { "Ni Ruo San Dong", melody_ni_ruo_san_dong, durations_ni_ruo_san_dong, sizeof(melody_ni_ruo_san_dong) / sizeof(melody_ni_ruo_san_dong[0]), 0 },
  { "Nu Fang De Sheng Ming", melody_nu_fang_de_sheng_ming, durations_nu_fang_de_sheng_ming, sizeof(melody_nu_fang_de_sheng_ming) / sizeof(melody_nu_fang_de_sheng_ming[0]), 0 },
  { "Ode to Joy", melody_ode_to_joy, durations_ode_to_joy, sizeof(melody_ode_to_joy) / sizeof(melody_ode_to_joy[0]), 3 },
  { "Peng You", melody_peng_you, durations_peng_you, sizeof(melody_peng_you) / sizeof(melody_peng_you[0]), 0 },
  { "Pi Pa Xing", melody_pi_pa_xing, durations_pi_pa_xing, sizeof(melody_pi_pa_xing) / sizeof(melody_pi_pa_xing[0]), 2 },
  { "Ping Fan Zhi Lu", melody_ping_fan_zhi_lu, durations_ping_fan_zhi_lu, sizeof(melody_ping_fan_zhi_lu) / sizeof(melody_ping_fan_zhi_lu[0]), 0 },
  { "Qi Feng Le", melody_qi_feng_le, durations_qi_feng_le, sizeof(melody_qi_feng_le) / sizeof(melody_qi_feng_le[0]), 3 },
  { "Qi Li Xiang", melody_qi_li_xiang, durations_qi_li_xiang, sizeof(melody_qi_li_xiang) / sizeof(melody_qi_li_xiang[0]), 3 },
  { "Qing Hua Ci", melody_qing_hua_ci, durations_qing_hua_ci, sizeof(melody_qing_hua_ci) / sizeof(melody_qing_hua_ci[0]), 3 },
  { "Qing Tian", melody_qing_tian, durations_qing_tian, sizeof(melody_qing_tian) / sizeof(melody_qing_tian[0]), 3 },
  { "Radetzky Marsch", melody_radetzky_marsch, durations_radetzky_marsch, sizeof(melody_radetzky_marsch) / sizeof(melody_radetzky_marsch[0]), 2 },
  { "Ruo Shui San Qian", melody_ruo_shui_san_qian, durations_ruo_shui_san_qian, sizeof(melody_ruo_shui_san_qian) / sizeof(melody_ruo_shui_san_qian[0]), 0 },
  { "Shan Qiu", melody_shan_qiu, durations_shan_qiu, sizeof(melody_shan_qiu) / sizeof(melody_shan_qiu[0]), 2 },
  { "Shape Of You", melody_shape_of_you, durations_shape_of_you, sizeof(melody_shape_of_you) / sizeof(melody_shape_of_you[0]), 2 },
  { "Shi Nian", melody_shi_nian, durations_shi_nian, sizeof(melody_shi_nian) / sizeof(melody_shi_nian[0]), 2 },
  { "Song Bie", melody_song_bie, durations_song_bie, sizeof(melody_song_bie) / sizeof(melody_song_bie[0]), 2 },
  { "Su Yan", melody_su_yan, durations_su_yan, sizeof(melody_su_yan) / sizeof(melody_su_yan[0]), 2 },
  { "Sugar", melody_sugar, durations_sugar, sizeof(melody_sugar) / sizeof(melody_sugar[0]), 2 },
  { "Take Me Hand", melody_take_me_hand, durations_take_me_hand, sizeof(melody_take_me_hand) / sizeof(melody_take_me_hand[0]), 2 },
  { "Tian Hei Hei", melody_tian_hei_hei, durations_tian_hei_hei, sizeof(melody_tian_hei_hei) / sizeof(melody_tian_hei_hei[0]), 2 },
  { "Tong Nian", melody_tong_nian, durations_tong_nian, sizeof(melody_tong_nian) / sizeof(melody_tong_nian[0]), 2 },
  { "Turkish March", melody_turkish_march, durations_turkish_march, sizeof(melody_turkish_march) / sizeof(melody_turkish_march[0]), 2 },
  { "Wo Huai Nian De", melody_wo_huai_nian_de, durations_wo_huai_nian_de, sizeof(melody_wo_huai_nian_de) / sizeof(melody_wo_huai_nian_de[0]), 0 },
  { "Wo Shi Yi Zhi Xiao Xiao Niao", melody_wo_shi_yi_zhi_xiao_xiao_niao, durations_wo_shi_yi_zhi_xiao_xiao_niao, sizeof(melody_wo_shi_yi_zhi_xiao_xiao_niao) / sizeof(melody_wo_shi_yi_zhi_xiao_xiao_niao[0]), 4 },
  { "Xi Huan Ni", melody_xi_huan_ni, durations_xi_huan_ni, sizeof(melody_xi_huan_ni) / sizeof(melody_xi_huan_ni[0]), 4 },
  { "Xia Ge Lu Kou", melody_xia_ge_lu_kou_jian, durations_xia_ge_lu_kou_jian, sizeof(melody_xia_ge_lu_kou_jian) / sizeof(melody_xia_ge_lu_kou_jian[0]), 4 },
  { "Xiao Ping Guo", melody_xiao_ping_guo, durations_xiao_ping_guo, sizeof(melody_xiao_ping_guo) / sizeof(melody_xiao_ping_guo[0]), 0 },
  { "Xin Qiang", melody_xin_qiang, durations_xin_qiang, sizeof(melody_xin_qiang) / sizeof(melody_xin_qiang[0]), 4 },
  { "Xiu Lian Ai Qing", melody_xiu_lian_ai_qing, durations_xiu_lian_ai_qing, sizeof(melody_xiu_lian_ai_qing) / sizeof(melody_xiu_lian_ai_qing[0]), 4 },
  { "Yan Yuan", melody_yan_yuan, durations_yan_yuan, sizeof(melody_yan_yuan) / sizeof(melody_yan_yuan[0]), 2 },
  { "Yi Sheng You Ni", melody_yi_sheng_you_ni, durations_yi_sheng_you_ni, sizeof(melody_yi_sheng_you_ni) / sizeof(melody_yi_sheng_you_ni[0]), 0 },
  { "Yin Xing De Chi Bang", melody_yin_xing_de_chi_bang, durations_yin_xing_de_chi_bang, sizeof(melody_yin_xing_de_chi_bang) / sizeof(melody_yin_xing_de_chi_bang[0]), 0 },
  { "You Dian Tian", melody_you_dian_tian, durations_you_dian_tian, sizeof(melody_you_dian_tian) / sizeof(melody_you_dian_tian[0]), 0 },
  { "Yu Jian", melody_yu_jian, durations_yu_jian, sizeof(melody_yu_jian) / sizeof(melody_yu_jian[0]), 0 },
  { "Ye Qu", melody_ye_qu, durations_ye_qu, sizeof(melody_ye_qu) / sizeof(melody_ye_qu[0]), 0 },
  { "Yuan Yang Xi", melody_yuan_yang_xi, durations_yuan_yang_xi, sizeof(melody_yuan_yang_xi) / sizeof(melody_yuan_yang_xi[0]), 0 },
  { "Zhe Shi Jie Na Mo Duo Ren", melody_zhe_shi_jie_na_mo_duo_ren, durations_zhe_shi_jie_na_mo_duo_ren, sizeof(melody_zhe_shi_jie_na_mo_duo_ren) / sizeof(melody_zhe_shi_jie_na_mo_duo_ren[0]), 2 },
  { "Zhen De Ai Ni", melody_zhen_de_ai_ni, durations_zhen_de_ai_ni, sizeof(melody_zhen_de_ai_ni) / sizeof(melody_zhen_de_ai_ni[0]), 3 },
  { "Zhui Guang Zhe", melody_zhui_guang_zhe, durations_zhui_guang_zhe, sizeof(melody_zhui_guang_zhe) / sizeof(melody_zhui_guang_zhe[0]), 0 },
  { "Zui Hou Yi Ye", melody_zui_hou_yi_ye, durations_zui_hou_yi_ye, sizeof(melody_zui_hou_yi_ye) / sizeof(melody_zui_hou_yi_ye[0]), 3 },
  { "Windows XP", melody_windows, durations_windows, sizeof(melody_windows) / sizeof(melody_windows[0]), 3 }
};
const int numSongs = sizeof(songs) / sizeof(songs[0]);

// --- UI State ---
int selectedSongIndex = 0;
int displayOffset = 0;
const int visibleSongs = 3;

// --- Forward Declarations ---
static void stop_buzzer_playback();

// --- Helper Functions ---
static uint32_t calculateSongDuration_ms(int songIndex)
{
  if (songIndex < 0 || songIndex >= numSongs) return 0;
  Song song;
  memcpy_P(&song, &songs[songIndex], sizeof(Song));
  uint32_t total_ms = 0;
  for (int i = 0; i < song.length; i++)
  {
    total_ms += pgm_read_word(song.durations + i);
  }
  return total_ms;
}

static uint32_t calculateElapsedTime_ms()
{
  if (shared_song_index < 0 || shared_song_index >= numSongs) return 0;
  Song song;
  memcpy_P(&song, &songs[shared_song_index], sizeof(Song));
  uint32_t elapsed_ms = 0;
  for (int i = 0; i < shared_note_index; i++)
  {
    elapsed_ms += pgm_read_word(song.durations + i);
  }
  TickType_t current_note_elapsed_ticks = xTaskGetTickCount() - current_note_start_tick;
  elapsed_ms += (current_note_elapsed_ticks * 1000) / configTICK_RATE_HZ;
  return elapsed_ms;
}

// --- UI Drawing ---
void displaySongList(int selectedIndex)
{
  menuSprite.fillScreen(TFT_BLACK);
  menuSprite.setTextColor(TFT_WHITE, TFT_BLACK);
  menuSprite.setTextSize(2);
  menuSprite.setTextColor(TFT_WHITE, TFT_BLACK);
  menuSprite.setTextDatum(MC_DATUM);
  menuSprite.drawString("Music Menu", 120, 28);
  char index_buf[20];
  sprintf(index_buf, "%d/%d", selectedIndex + 1, numSongs);  // +1 是为了让索引从1开始显示
  menuSprite.setTextDatum(MR_DATUM);  // 右上对齐，方便放在标题右边
  menuSprite.setTextSize(1);
  menuSprite.drawString(index_buf, 230, 28);  // x=230 靠右，与左边标题对称
  menuSprite.setTextDatum(MC_DATUM);
  for (int i = 0; i < visibleSongs; i++)
  {
    int songIdx = displayOffset + i;
    if (songIdx >= numSongs) break;
    int yPos = 60 + i * 50;
    if (songIdx == selectedIndex)
    {
      menuSprite.fillRoundRect(10, yPos - 18, 220, 36, 5, 0x001F);
      menuSprite.setTextSize(2);
      menuSprite.setTextColor(TFT_WHITE, 0x001F);
      menuSprite.drawString(songs[songIdx].name, 120, yPos);
    }
    else
    {
      menuSprite.fillRoundRect(10, yPos - 18, 220, 36, 5, TFT_BLACK);
      menuSprite.setTextSize(1);
      menuSprite.setTextColor(TFT_WHITE, TFT_BLACK);
      menuSprite.drawString(songs[songIdx].name, 120, yPos);
    }
  }
  menuSprite.setTextDatum(TL_DATUM);
  menuSprite.pushSprite(0, 0);
}

void displayPlayingSong()
{
  uint32_t elapsed_ms = calculateElapsedTime_ms();
  uint32_t total_ms = calculateSongDuration_ms(shared_song_index);

  menuSprite.fillScreen(TFT_BLACK);
  menuSprite.setTextDatum(MC_DATUM);
  menuSprite.setTextColor(TFT_WHITE, TFT_BLACK);

  menuSprite.setTextSize(2);
  menuSprite.drawString(songs[shared_song_index].name, 120, 20);

  extern struct tm timeinfo;
  if (getLocalTime(&timeinfo, 0))
  {
    char timeStr[20];
    strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &timeinfo);
    menuSprite.setTextSize(2);
    menuSprite.drawString(timeStr, 120, 50);
  }

  // Display Play/Pause status
  String status_text = isPaused ? "Paused" : "Playing";
  menuSprite.drawString(status_text, 120, 80);

  // Display Play Mode below status
  String mode_text;
  switch (currentPlayMode)
  {
  case SINGLE_LOOP: mode_text = "Single Loop"; break;
  case LIST_LOOP:   mode_text = "List Loop"; break;
  case RANDOM_PLAY: mode_text = "Random"; break;
  }
  menuSprite.setTextSize(1); // Use smaller text for the mode
  menuSprite.drawString(mode_text, 120, 100);
  menuSprite.setTextSize(2); // Reset text size

  // --- Time-domain song visualization ---
  Song current_song;
  memcpy_P(&current_song, &songs[shared_song_index], sizeof(Song));

  if (current_song.length > 0)
  {
    // Find min (non-zero) and max frequency for scaling
    int min_freq = 10000;
    int max_freq = 0;
    for (int i = 0; i < current_song.length; i++)
    {
      int freq = pgm_read_word(current_song.melody + i);
      if (freq > 0)
      {
        if (freq < min_freq) min_freq = freq;
        if (freq > max_freq) max_freq = freq;
      }
    }
    // Handle songs with no audible notes or a single note
    if (min_freq > max_freq) { min_freq = 200; max_freq = 2000; }
    if (min_freq == max_freq) { min_freq = max_freq / 2; }

    // Drawing parameters
    const int graph_x = 10;
    const int graph_y_bottom = 180;
    const int graph_area_width = 220;
    const int max_bar_height = 75;
    const int min_bar_height = 2;

    float step = (float) graph_area_width / current_song.length;
    int bar_width = (step > 1.0f) ? floor(step * 0.8f) : 1; // 80% of space, or 1px minimum
    if (bar_width == 0) bar_width = 1;

    for (int i = 0; i < current_song.length; i++)
    {
      int freq = pgm_read_word(current_song.melody + i);
      int bar_height = 0;
      if (freq > 0)
      {
        bar_height = map(freq, min_freq, max_freq, min_bar_height, max_bar_height);
      }

      if (bar_height > 0)
      {
        uint16_t color = (i <= shared_note_index) ? TFT_CYAN : TFT_DARKGREY;
        int x_pos = graph_x + floor(i * step);
        menuSprite.fillRect(x_pos, graph_y_bottom - bar_height, bar_width, bar_height, color);
      }
    }
  }

  int progressWidth = 0;
  if (total_ms > 0)
  {
    progressWidth = (elapsed_ms * 220) / total_ms;
    if (progressWidth > 220) progressWidth = 220;
  }
  menuSprite.drawRoundRect(10, 185, 220, 10, 5, TFT_WHITE);
  menuSprite.fillRoundRect(10, 185, progressWidth, 10, 5, TFT_WHITE);

  char time_buf[20];
  snprintf(time_buf, sizeof(time_buf), "%02d:%02d / %02d:%02d", elapsed_ms / 60000, (elapsed_ms / 1000) % 60, total_ms / 60000, (total_ms / 1000) % 60);
  menuSprite.drawString(time_buf, 120, 210);

  // --- Current Note Info ---
  int current_freq = 0;
  int current_dur = 0;
  if (!isPaused && shared_note_index < current_song.length)
  {
    current_freq = pgm_read_word(current_song.melody + shared_note_index);
    current_dur = pgm_read_word(current_song.durations + shared_note_index);
  }
  char note_info[30];
  snprintf(note_info, sizeof(note_info), "Note: %d Hz, %d ms", current_freq, current_dur);
  menuSprite.setTextSize(1); // Use smaller text
  menuSprite.drawString(note_info, 120, 228);

  menuSprite.pushSprite(0, 0);
}

// --- Tasks ---
uint32_t Wheel(byte WheelPos)
{
  WheelPos = 255 - WheelPos;
  if (WheelPos < 85)
  {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if (WheelPos < 170)
  {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

void Led_Rainbow_Task(void *pvParameters)
{
  for (uint16_t j = 0; ; j++)
  {
    if (stopLedTask)
    {
      strip.clear();
      strip.show();
      vTaskDelete(NULL);
    }
    for (uint16_t i = 0; i < strip.numPixels(); i++)
    {
      strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
    }
    strip.show();
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

void Buzzer_Task(void *pvParameters)
{
  int songIdx = *(int *) pvParameters;
  for (;;)
  {
    shared_song_index = songIdx;
    Song song;
    memcpy_P(&song, &songs[songIdx], sizeof(Song));
    for (int i = 0; i < song.length; i++)
    {
      shared_note_index = i;
      current_note_start_tick = xTaskGetTickCount();
      if (stopBuzzerTask)
      {
        noTone(BUZZER_PIN);
        vTaskDelete(NULL);
      }
      while (isPaused)
      {
        noTone(BUZZER_PIN);
        current_note_start_tick = xTaskGetTickCount();
        vTaskDelay(pdMS_TO_TICKS(50));
      }
      int note = pgm_read_word(song.melody + i);
      int duration = pgm_read_word(song.durations + i);
      if (note > 0)
      {
        tone(BUZZER_PIN, note, duration);
      }
      vTaskDelay(pdMS_TO_TICKS(duration));
    }
    vTaskDelay(pdMS_TO_TICKS(2000));
    if (currentPlayMode == SINGLE_LOOP) {}
    else if (currentPlayMode == LIST_LOOP)
    {
      songIdx = (songIdx + 1) % numSongs;
    }
    else if (currentPlayMode == RANDOM_PLAY)
    {
      if (numSongs > 1)
      {
        int currentSong = songIdx;
        do { songIdx = random(numSongs); } while (songIdx == currentSong);
      }
    }
  }
}

// This is the restored task for background music (e.g., boot, chime)
void Buzzer_PlayMusic_Task(void *pvParameters)
{
  int songIndex = *(int *) pvParameters;
  Song song;
  memcpy_P(&song, &songs[songIndex], sizeof(Song));

  for (int i = 0; i < song.length; i++)
  {
    if (stopBuzzerTask)
    { // Check the global stop flag
      noTone(BUZZER_PIN);
      vTaskDelete(NULL);
      return;
    }

    int note = pgm_read_word(song.melody + i);
    int duration = pgm_read_word(song.durations + i);

    if (note > 0)
    {
      tone(BUZZER_PIN, note, duration);
    }

    vTaskDelay(pdMS_TO_TICKS(duration));
  }

  vTaskDelete(NULL); // Self-delete when done
}

// --- Main Menu Function ---
void Buzzer_Init() { pinMode(BUZZER_PIN, OUTPUT); }

static void stop_buzzer_playback()
{
  if (buzzerTaskHandle != NULL) { vTaskDelete(buzzerTaskHandle); buzzerTaskHandle = NULL; }
  if (ledTaskHandle != NULL) { vTaskDelete(ledTaskHandle); ledTaskHandle = NULL; }
  noTone(BUZZER_PIN);
  strip.clear();
  strip.show();
  isPaused = false;
  stopBuzzerTask = false;
  stopLedTask = false;
}

void BuzzerMenu()
{
  selectedSongIndex = 0;
  displayOffset = 0;
  isPaused = false;

  // 主循环，允许在播放界面和列表界面之间切换
  while (1)
  {
    // 显示歌曲列表
    displaySongList(selectedSongIndex);

    // 在歌曲列表界面中的循环
    bool inListMenu = true;
    while (inListMenu)
    {
      if (exitSubMenu || g_alarm_is_ringing)
      {
        stop_buzzer_playback();
        return;
      }

      int encoderChange = readEncoder();
      if (encoderChange != 0)
      {
        selectedSongIndex = (selectedSongIndex + encoderChange + numSongs) % numSongs;
        if (selectedSongIndex < displayOffset)
        {
          displayOffset = selectedSongIndex;
        }
        else if (selectedSongIndex >= displayOffset + visibleSongs)
        {
          displayOffset = selectedSongIndex - visibleSongs + 1;
        }
        displaySongList(selectedSongIndex);
        tone(BUZZER_PIN, 1000, 50);
      }

      if (readButton())
      {
        tone(BUZZER_PIN, 1500, 50);
        inListMenu = false; // 退出列表界面，进入播放界面
      }

      if (readButtonLongPress())
      {
        return; // 长按直接返回主菜单
      }

      vTaskDelay(pdMS_TO_TICKS(20));
    }

    // 开始播放选中的歌曲
    stopBuzzerTask = false;
    stopLedTask = false;
    isPaused = false;
    currentPlayMode = LIST_LOOP;

    // 创建播放任务
    if (buzzerTaskHandle == NULL)
    {
      xTaskCreatePinnedToCore(Buzzer_Task, "Buzzer_Task", 4096, &selectedSongIndex, 2, &buzzerTaskHandle, 0);
    }
    if (ledTaskHandle == NULL)
    {
      xTaskCreatePinnedToCore(Led_Rainbow_Task, "Led_Rainbow_Task", 2048, NULL, 1, &ledTaskHandle, 0);
    }

    unsigned long lastScreenUpdateTime = 0;

    // 在播放界面中的循环
    bool inPlayMenu = true;
    while (inPlayMenu)
    {
      if (exitSubMenu || g_alarm_is_ringing)
      {
        stop_buzzer_playback();
        return;
      }

      if (readButtonLongPress())
      {
        // 长按停止播放并返回列表界面
        stop_buzzer_playback();
        inPlayMenu = false;
      }

      if (readButton())
      {
        // 短按暂停/继续播放
        isPaused = !isPaused;
        tone(BUZZER_PIN, 1000, 50);
      }

      int encoderChange = readEncoder();
      if (encoderChange != 0)
      {
        // 旋转编码器切换播放模式
        int mode = (int) currentPlayMode;
        mode = (mode + encoderChange + 3) % 3;
        currentPlayMode = (PlayMode) mode;
      }

      // 更新显示
      if (millis() - lastScreenUpdateTime > 100)
      {
        displayPlayingSong();
        lastScreenUpdateTime = millis();
      }

      vTaskDelay(pdMS_TO_TICKS(20));
    }
  }
}
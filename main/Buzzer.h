#ifndef BUZZER_H
#define BUZZER_H

#define BUZZER_PIN 5

#include <Arduino.h>

// 播放模式枚举
enum PlayMode {
  SINGLE_LOOP,   // 单曲循环
  LIST_LOOP,     // 列表播放
  RANDOM_PLAY    // 随机播放
};
#include <TFT_eSPI.h>
#include "Music_processed/cai_bu_tou.h"
#include "Music_processed/cheng_du.h"
#include "Music_processed/hai_kuo_tian_kong.h"
#include "Music_processed/hong_dou.h"
#include "Music_processed/hou_lai.h"
#include "Music_processed/kai_shi_dong_le.h"
#include "Music_processed/lv_se.h"
#include "Music_processed/qing_hua_ci.h"
#include "Music_processed/xin_qiang.h"
#include "Music_processed/you_dian_tian.h"
#include "Music_processed/chun_jiao_yu_zhi_ming.h"
#include "Music_processed/Windows.h"
#include "Music_processed/mi_ren_de_wei_xian.h"
#include "Music_processed/hong_se_gao_gen_xie.h"
#include "Music_processed/liang_zhu.h"
#include "Music_processed/qi_feng_le.h"
#include "Music_processed/da_hai.h"
#include "Music_processed/yu_jian.h"
#include "Music_processed/Dream_Wedding.h"
#include "Music_processed/For_Elise.h"
#include "Music_processed/guo_ji_ge.h"
#include "Music_processed/Turkish_March.h"
#include "Music_processed/ye_feng_fei_wu.h"
#include "Music_processed/bao_wei_huang_he.h"
#include "Music_processed/bu_zai_you_yu.h"
#include "Music_processed/Casablanca.h"
#include "Music_processed/dong_fang_zhi_zhu.h"
#include "Music_processed/fan_fang_xiang_de_zhong.h"
#include "Music_processed/ge_chang_zu_guo.h"
#include "Music_processed/lan_ting_xu.h"
#include "Music_processed/tong_nian.h"
#include "Music_processed/wo_shi_yi_zhi_xiao_xiao_niao.h"
#include "Music_processed/xi_huan_ni.h"
#include "Music_processed/yi_sheng_you_ni.h"
#include "Music_processed/zhen_de_ai_ni.h"
#include "Music_processed/Fate.h"
#include "Music_processed/Ode_to_joy.h"
#include "Music_processed/radetzky_marsch.h"
#include "Music_processed/qing_tian.h"
#include "Music_processed/hua_hai.h"
#include "Music_processed/qi_li_xiang.h"
#include "Music_processed/yin_xing_de_chi_bang.h"
#include "Music_processed/five_hundred_miles.h"
#include "Music_processed/dong_fang_hong.h"
#include "Music_processed/chong_er_fei.h"
#include "Music_processed/fu_shi_shan_xia.h"
#include "Music_processed/jiang_nan.h"
#include "Music_processed/dao_xiang.h"
#include "Music_processed/gao_bai_qi_qiu.h"
#include "Music_processed/shi_nian.h"
#include "Music_processed/xia_ge_lu_kou_jian.h"
#include "Music_processed/zui_hou_yi_ye.h"
#include "Music_processed/ai_ni.h"
#include "Music_processed/ba_hui_yi_pin_hao_gei_ni.h"
#include "Music_processed/bu_de_bu_ai.h"
#include "Music_processed/ce_lian.h"
#include "Music_processed/dong_xi.h"
#include "Music_processed/he_tang_yue_ye.h"
#include "Music_processed/nan_fang_gu_niang.h"
#include "Music_processed/pi_pa_xing.h"
#include "Music_processed/shan_qiu.h"
#include "Music_processed/song_bie.h"
#include "Music_processed/su_yan.h"
#include "Music_processed/sugar.h"
#include "Music_processed/yan_yuan.h"
#include "Music_processed/zhe_shi_jie_na_mo_duo_ren.h"
#include "Music_processed/take_me_hand.h"
#include "Music_processed/shape_of_you.h"
#include "Music_processed/xiu_lian_ai_qing.h"
#include "Music_processed/gang_hao_yu_jian_ni.h"
#include "Music_processed/ai_qing_xun_xi.h"
#include "Music_processed/a_peng_you_zai_jian.h"
#include "Music_processed/counting_stars.h"
#include "Music_processed/dao_shu.h"
#include "Music_processed/duo_yuan_dou_yao_zai_yi_qi.h"
#include "Music_processed/for_ya.h"
#include "Music_processed/guang_hui_sui_yue.h"
#include "Music_processed/guo_huo.h"
#include "Music_processed/ke_bu_ke_yi.h"
#include "Music_processed/ming_tian_hui_geng_hao.h"
#include "Music_processed/ni_ruo_san_dong.h"
#include "Music_processed/nu_fang_de_sheng_ming.h"
#include "Music_processed/peng_you.h"
#include "Music_processed/ping_fan_zhi_lu.h"
#include "Music_processed/ruo_shui_san_qian.h"
#include "Music_processed/song_bie.h"
#include "Music_processed/wo_huai_nian_de.h"
#include "Music_processed/xiao_ping_guo.h"
#include "Music_processed/ye_qu.h"
#include "Music_processed/yuan_yang_xi.h"
#include "Music_processed/zhui_guang_zhe.h"
#include "Music_processed/ai_ru_chao_shui.h"
// 音阶频率（Hz）
#define NOTE_REST 0
#define NOTE_G3 196
#define NOTE_A3 220
#define NOTE_B3 247
#define NOTE_C4 262
#define NOTE_CS4 277
#define NOTE_D4 294
#define NOTE_DS4 311
#define NOTE_E4 330
#define NOTE_F4 349
#define NOTE_FS4 370
#define NOTE_G4 392
#define NOTE_GS4 415
#define NOTE_A4 440
#define NOTE_AS4 466
#define NOTE_B4 494
#define NOTE_C5 523
#define NOTE_CS5 554
#define NOTE_D5 587
#define NOTE_DS5 622
#define NOTE_E5 659
#define NOTE_F5 698
#define NOTE_FS5 740
#define NOTE_G5 784
#define NOTE_GS5 831
#define NOTE_A5 880
#define NOTE_AS5 932
#define NOTE_B5 988
// 音阶频率（Hz）
#define P0 	0	// 休止符频率

#define L1 262  // 低音频率
#define L2 294
#define L3 330
#define L4 349
#define L5 392
#define L6 440
#define L7 494

#define M1 523  // 中音频率
#define M2 587
#define M3 659
#define M4 698
#define M5 784
#define M6 880
#define M7 988

#define H1 1047 // 高音频率
#define H2 1175
#define H3 1319
#define H4 1397
#define H5 1568
#define H6 1760
#define H7 1976


#define DAHAI_TIME_OF_BEAT 714 // 大海节拍时间（ms）
// Song structure from Buzzer.cpp
typedef struct {
  const char* name;
  const int* melody;
  const int* durations;
  int length;
  int colorSchemeIndex;
} Song;

// Exposed for watchface hourly chime
extern const Song songs[] PROGMEM;
extern const int numSongs;
extern volatile bool stopBuzzerTask;
void Buzzer_Task(void *pvParameters);
void Buzzer_PlayMusic_Task(void *pvParameters);
void Buzzer_PlayMusic_Task(void *pvParameters);

// Main menu function
void Buzzer_Init();
void BuzzerMenu();

// New functions for direct playback by name
int findSongIndexByName(const String& name);
void setSongToPlay(int index);
void playSpecificSong();



// const int melody_da_hai[]  = {
//   L5,L6,M1,M1,M1,M1,L6,L5,M1,M1,M2,M1,L6,M1,M2,M2,M2,M2,M1,L6,M2,M2,M3,M2,M3,M5,M6,M6,M5,M6,M5,M3,M2,M2,M3,M2,M1,L6,L5,L6,M1,M1,M1,M1,L6,M1,L5,L6,M1,M1,M1,M1,L6,L5,M1,M1,M2,M1,L6,M1,M2,M2,M2,M2,M1,L6,M2,M2,M3,M2,M3,M5,M6,M6,M5,M6,H1,M6,M5,M3,M2,M1,L6,L5,L6,M1,M1,M1,M1,M2,M1,M1,M3,M5
// };
// const int durations_da_hai[] = {
//   DAHAI_TIME_OF_BEAT/2,DAHAI_TIME_OF_BEAT/2,DAHAI_TIME_OF_BEAT/2,DAHAI_TIME_OF_BEAT,DAHAI_TIME_OF_BEAT/2,DAHAI_TIME_OF_BEAT,DAHAI_TIME_OF_BEAT/2,DAHAI_TIME_OF_BEAT/2,DAHAI_TIME_OF_BEAT/2,DAHAI_TIME_OF_BEAT,DAHAI_TIME_OF_BEAT/2,DAHAI_TIME_OF_BEAT,DAHAI_TIME_OF_BEAT/2,DAHAI_TIME_OF_BEAT/2,
//   DAHAI_TIME_OF_BEAT/2,DAHAI_TIME_OF_BEAT,DAHAI_TIME_OF_BEAT/2,DAHAI_TIME_OF_BEAT,DAHAI_TIME_OF_BEAT/2,DAHAI_TIME_OF_BEAT/2,DAHAI_TIME_OF_BEAT/2,DAHAI_TIME_OF_BEAT,DAHAI_TIME_OF_BEAT/2,DAHAI_TIME_OF_BEAT,DAHAI_TIME_OF_BEAT/2,DAHAI_TIME_OF_BEAT/2,DAHAI_TIME_OF_BEAT/2,DAHAI_TIME_OF_BEAT,DAHAI_TIME_OF_BEAT/2,DAHAI_TIME_OF_BEAT,DAHAI_TIME_OF_BEAT/2,DAHAI_TIME_OF_BEAT/2,DAHAI_TIME_OF_BEAT/2,DAHAI_TIME_OF_BEAT/4,DAHAI_TIME_OF_BEAT/4,DAHAI_TIME_OF_BEAT/2,DAHAI_TIME_OF_BEAT/2,DAHAI_TIME_OF_BEAT,DAHAI_TIME_OF_BEAT/2,DAHAI_TIME_OF_BEAT/2,DAHAI_TIME_OF_BEAT/2,DAHAI_TIME_OF_BEAT,DAHAI_TIME_OF_BEAT/2,DAHAI_TIME_OF_BEAT,DAHAI_TIME_OF_BEAT,DAHAI_TIME_OF_BEAT*3,DAHAI_TIME_OF_BEAT/2,DAHAI_TIME_OF_BEAT/2,DAHAI_TIME_OF_BEAT/2,DAHAI_TIME_OF_BEAT,DAHAI_TIME_OF_BEAT/2,DAHAI_TIME_OF_BEAT,DAHAI_TIME_OF_BEAT/2,DAHAI_TIME_OF_BEAT/2,DAHAI_TIME_OF_BEAT/2,DAHAI_TIME_OF_BEAT,DAHAI_TIME_OF_BEAT/2,DAHAI_TIME_OF_BEAT,DAHAI_TIME_OF_BEAT/2,DAHAI_TIME_OF_BEAT/2,DAHAI_TIME_OF_BEAT/2,DAHAI_TIME_OF_BEAT,DAHAI_TIME_OF_BEAT/2,DAHAI_TIME_OF_BEAT,DAHAI_TIME_OF_BEAT/2,DAHAI_TIME_OF_BEAT/2,DAHAI_TIME_OF_BEAT/2,DAHAI_TIME_OF_BEAT,DAHAI_TIME_OF_BEAT/2,DAHAI_TIME_OF_BEAT,DAHAI_TIME_OF_BEAT/2,DAHAI_TIME_OF_BEAT/2,DAHAI_TIME_OF_BEAT/2,DAHAI_TIME_OF_BEAT/2,DAHAI_TIME_OF_BEAT,DAHAI_TIME_OF_BEAT/2,DAHAI_TIME_OF_BEAT,DAHAI_TIME_OF_BEAT/2,DAHAI_TIME_OF_BEAT/2,DAHAI_TIME_OF_BEAT/2,DAHAI_TIME_OF_BEAT/2,DAHAI_TIME_OF_BEAT/2,DAHAI_TIME_OF_BEAT/2,DAHAI_TIME_OF_BEAT,DAHAI_TIME_OF_BEAT/2,DAHAI_TIME_OF_BEAT/2,
  
//   DAHAI_TIME_OF_BEAT/2,DAHAI_TIME_OF_BEAT,DAHAI_TIME_OF_BEAT/2,DAHAI_TIME_OF_BEAT,DAHAI_TIME_OF_BEAT/2,DAHAI_TIME_OF_BEAT/2,DAHAI_TIME_OF_BEAT*3,DAHAI_TIME_OF_BEAT/2,
// };

// // 生日快乐
// const int melody_happy_birthday[] = {
//   NOTE_C4, NOTE_C4, NOTE_D4, NOTE_C4, NOTE_F4, NOTE_E4,
//   NOTE_C4, NOTE_C4, NOTE_D4, NOTE_C4, NOTE_G4, NOTE_F4,
//   NOTE_C4, NOTE_C4, NOTE_C5, NOTE_A4, NOTE_F4, NOTE_E4, NOTE_D4,
//   NOTE_A4, NOTE_A4, NOTE_A4, NOTE_F4, NOTE_G4, NOTE_F4
// };
// const int durations_happy_birthday[] = {
//   250, 250, 500, 500, 500, 1000,
//   250, 250, 500, 500, 500, 1000,
//   250, 250, 500, 500, 500, 500, 1000,
//   250, 250, 500, 500, 500, 1000
// };

#endif

#ifndef BUZZER_H
#define BUZZER_H

#define BUZZER_PIN 5

#include <Arduino.h>

// 播放模式枚举
enum PlayMode
{
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
#include "Music_processed/tian_hei_hei.h"


// 歌曲结构体
typedef struct
{
  const char *name;
  const int *melody;
  const int *durations;
  int length;
  int colorSchemeIndex;
} Song;

extern const Song songs[] PROGMEM;
extern const int numSongs;
extern volatile bool stopBuzzerTask;
/**
 * @brief [FreeRTOS Task] 循环播放音乐的任务。
 * @param pvParameters 指向要开始播放的歌曲索引（int*）的指针。
 * @details 此任务根据当前的播放模式（列表循环、单曲循环、随机播放）持续播放音乐库中的歌曲。
 *          它会处理暂停、停止和模式切换的逻辑。
 */
void Buzzer_Task(void *pvParameters);

/**
 * @brief [FreeRTOS Task] 播放单首音乐的任务。
 * @param pvParameters 指向要播放的歌曲索引（int*）的指针。
 * @details 这是一个一次性的播放任务，用于播放指定的单首歌曲，播放完毕后任务会自行删除。
 *          主要用于如开机音乐、整点报时等短时播放场景。
 */
void Buzzer_PlayMusic_Task(void *pvParameters);

/**
 * @brief 初始化蜂鸣器。
 * @details 将蜂鸣器连接的GPIO引脚设置为输出模式。
 */
void Buzzer_Init();

/**
 * @brief 音乐播放器的主菜单函数。
 * @details 提供一个交互式菜单，用户可以选择歌曲、播放、暂停、切换播放模式。
 *          它管理着音乐播放任务和LED灯效任务的生命周期。
 */
void BuzzerMenu();

/**
 * @brief 通过歌曲名称查找其索引。
 * @param name 要查找的歌曲的名称。
 * @return 如果找到，返回歌曲在 `songs` 数组中的索引；如果未找到，返回-1。
 */
int findSongIndexByName(const String &name);

/**
 * @brief 设置当前要播放的歌曲。
 * @param index 要播放的歌曲在 `songs` 数组中的索引。
 * @details 此函数用于在调用 `playSpecificSong()` 之前指定歌曲。
 */
void setSongToPlay(int index);

/**
 * @brief 播放由 `setSongToPlay` 指定的歌曲。
 * @details 启动一个 `Buzzer_PlayMusic_Task` 任务来播放指定的歌曲。
 */
void playSpecificSong();

/**
 * @brief 在后台播放指定的歌曲。
 * @param songIndex 要播放的歌曲的索引。
 * @details 此函数会停止任何正在后台播放的歌曲，然后创建一个新任务来播放指定的歌曲。
 */
void play_song_background(int songIndex);

/**
 * @brief 直接进入指定歌曲的完整播放UI。
 * @param songIndex 要播放的歌曲的索引。
 * @details 此函数会启动播放任务和灯效任务，并直接显示“正在播放”界面，
 *          允许用户控制播放，而不是从歌曲列表开始。
 */
void play_song_full_ui(int songIndex);

#endif

#ifndef ALARM_H
#define ALARM_H

#include <stdint.h>

extern volatile bool g_alarm_is_ringing; // Make this global flag accessible

// Moved from .cpp to be public
struct AlarmSetting {
  uint8_t hour;
  uint8_t minute;
  uint8_t days_of_week;
  bool enabled;
  bool triggered_today;
};

/**
 * @brief 初始化闹钟系统。
 * @details 此函数从EEPROM中加载之前保存的闹钟设置。
 *          同时，它会创建一个后台FreeRTOS任务，用于周期性地检查是否有闹钟需要触发。
 */
void Alarm_Init();

/**
 * @brief 显示并管理闹钟设置菜单。
 * @details 该函数是闹钟设置功能的用户界面入口。它允许用户通过旋转编码器和按钮
 *          来浏览、启用/禁用、添加、编辑和删除闹钟。
 */
void AlarmMenu();

/**
 * @brief 周期性检查闹钟触发。
 * @details 此函数应被一个定时任务反复调用。它会检查当前时间是否与任何已启用的
 *          闹钟匹配。如果匹配，并且当天尚未触发过，它将调用 `triggerAlarm` 函数。
 */
void Alarm_Loop_Check();

/**
 * @brief 显示闹钟响铃界面。
 * @details 当一个闹钟被触发时，此函数负责显示一个全屏界面，
 *          提示用户时间已到，并播放音乐。用户可以通过按下按钮来停止闹钟。
 */
void Alarm_ShowRingingScreen();

/**
 * @brief 停止正在播放的闹钟音乐。
 * @details 此函数用于在用户操作后停止闹钟音乐的播放，并重置响铃状态标志。
 */
void Alarm_StopMusic();

/**
 * @brief 获取当前设置的闹钟数量。
 * @return 返回当前存储的闹钟总数。
 */
int getAlarmCount();

/**
 * @brief 获取指定闹钟的详细信息。
 * @param index 要查询的闹钟的索引。
 * @param[out] settings 用于接收闹钟设置信息的结构体引用。
 * @return 如果索引有效，则返回true并填充settings结构体；否则返回false。
 */
bool getAlarmInfo(int index, AlarmSetting& settings);

/**
 * @brief 添加或更新一个闹钟。
 * @param index 要更新的闹钟的索引 (0-9)。如果超出当前闹钟数量，则会添加新闹钟（如果未满）。
 * @param hour 小时 (0-23)。
 * @param minute 分钟 (0-59)。
 * @param days_of_week 星期选择的位掩码 (e.g., 1<<0 for Sunday)。
 * @param enabled 闹钟是否启用。
 * @return 如果操作成功，返回true。
 */
bool Alarm_Update(int index, uint8_t hour, uint8_t minute, uint8_t days_of_week, bool enabled);


#endif // ALARM_H

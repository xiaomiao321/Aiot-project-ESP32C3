#ifndef TARGETSETTINGS_H
#define TARGETSETTINGS_H

#include <time.h>
#include <TFT_eSPI.h>

// EEPROM start address for this module's data.
// Alarm data is at address 0 and takes about 51 bytes. We'll start at 100 for safety.
#define EEPROM_TARGET_START_ADDR 100
#define EEPROM_TARGET_MAGIC_KEY 0xDA // Different magic key to avoid conflicts

// Data structure for the progress bar
struct ProgressBarInfo {
  time_t startTime;
  time_t endTime;
  char title[50]; // Title for the progress bar
};

// --- Public API ---

/**
 * @brief 初始化目标设置模块。
 * @details 此函数从EEPROM加载倒计时和进度条的设置。如果EEPROM中没有有效数据
 *          （通过一个“魔术数字”校验），它会加载一组默认值并保存。
 */
void TargetSettings_Init();

/**
 * @brief 显示并处理目标设置菜单。
 * @details 提供一个多级菜单，允许用户分别设置倒计时的目标日期时间、
 *          进度条的开始/结束日期以及进度条的标题。所有更改都会被保存到EEPROM。
 */
void TargetSettings_Menu();

/**
 * @brief 获取倒计时目标时间。
 * @return 返回一个 `time_t` 类型的值，表示倒计时的目标时间戳。
 */
time_t getCountdownTarget();

/**
 * @brief 获取进度条的设置信息。
 * @return 返回一个对 `ProgressBarInfo` 结构体的常量引用，包含标题、开始和结束时间。
 */
const ProgressBarInfo& getProgressBarInfo();

/**
 * @brief 将目标元素（倒计时和进度条）绘制到TFT sprite上。
 * @param sprite 指向 `TFT_eSprite` 对象的指针，所有元素将被绘制到这个sprite上。
 * @details 此函数根据当前时间和已保存的目标设置，计算并绘制剩余时间倒计时
 *          和一个可视化的进度条，包括百分比和起止日期。
 */
void drawTargetElements(TFT_eSprite* sprite);

#endif // TARGETSETTINGS_H

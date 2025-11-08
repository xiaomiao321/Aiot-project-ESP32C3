#ifndef COUNTDOWN_H
#define COUNTDOWN_H

#include <TFT_eSPI.h> // Assuming TFT_eSPI is needed for display
#include "RotaryEncoder.h" // Assuming RotaryEncoder is needed for input

extern TFT_eSPI tft; // Use the global tft object

/**
 * @brief 倒计时功能的主菜单函数。
 * @details 此函数提供一个完整的倒计时交互界面。用户可以通过旋转编码器设置
 *          分钟和秒数，通过短按按钮切换设置模式并启动倒计时。
 *          在倒计时过程中，可以短按暂停/继续。长按则退出该功能。
 *          界面会实时显示剩余时间、进度条以及当前状态（设置、运行、暂停、完成）。
 *          倒计时结束或最后5秒会有声音提示。
 */
void CountdownMenu();

#endif // COUNTDOWN_H
#ifndef STOPWATCH_H
#define STOPWATCH_H

#include <TFT_eSPI.h> // Assuming TFT_eSPI is needed for display
#include "RotaryEncoder.h" // Assuming RotaryEncoder is needed for input

extern TFT_eSPI tft; // Use the global tft object

/**
 * @brief 秒表功能的主菜单函数。
 * @details 此函数提供一个完整的秒表交互界面。用户可以通过单击按钮来
 *          启动、暂停和恢复计时。长按按钮可以重置秒表并退出此功能。
 *          界面会以“分:秒.百分之一秒”的格式高频刷新显示时间，
 *          并同时显示当前的状态（运行、暂停、就绪）。
 */
void StopwatchMenu();

#endif // STOPWATCH_H
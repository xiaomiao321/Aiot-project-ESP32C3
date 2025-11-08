#ifndef LED_H
#define LED_H

#include <Adafruit_NeoPixel.h> 

#define LED_PIN 3
#define NUM_LEDS 10
#define BRIGHTNESS 50

extern Adafruit_NeoPixel strip;

/**
 * @brief LED灯带控制的主菜单函数。
 * @details 提供一个交互式界面，允许用户通过旋转编码器调整NeoPixel灯带的
 *          亮度和色相。单击按钮可以在亮度和颜色模式之间切换，双击则退出菜单。
 */
void LEDMenu();

#endif
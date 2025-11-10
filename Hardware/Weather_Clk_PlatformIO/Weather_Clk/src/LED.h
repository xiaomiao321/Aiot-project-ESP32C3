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

/**
 * @brief 设置单个LED的颜色。
 * @param index 要设置的LED的索引 (0 to NUM_LEDS-1)。
 * @param r 红色值 (0-255)。
 * @param g 绿色值 (0-255)。
 * @param b 蓝色值 (0-255)。
 */
void led_set_single(uint8_t index, uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief 将所有LED设置为相同的颜色。
 * @param r 红色值 (0-255)。
 * @param g 绿色值 (0-255)。
 * @param b 蓝色值 (0-255)。
 */
void led_set_all(uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief 启动或更新彩虹动画模式。
 * @param speed 动画速度（两次刷新之间的延迟，毫秒）。
 */
void led_rainbow_mode(uint16_t speed);

/**
 * @brief 关闭所有LED。
 */
void led_off();

#endif
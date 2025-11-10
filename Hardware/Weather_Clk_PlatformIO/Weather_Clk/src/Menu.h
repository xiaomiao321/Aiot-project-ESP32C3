#ifndef MENU_H
#define MENU_H

#include "RotaryEncoder.h"
#include <TFT_eSPI.h>
#include "img.h"
#include "Internet.h" // Include for the new Internet menu

/**
 * @brief 主菜单项结构体。
 * @details 用于定义一个菜单项，包含其名称、图标和被选中时执行的函数。
 */
struct MenuItem {
    const char *name;              ///< 菜单项显示的名称。
    const uint16_t *image;         ///< 指向菜单项图标图像数据的指针。
    void (*action)();              ///< 当菜单项被选中时要调用的函数指针。
};

// 全局变量的前向声明
extern TFT_eSPI tft;
extern TFT_eSprite menuSprite;
extern int16_t display;
extern uint8_t picture_flag;
extern const MenuItem menuItems[];
extern const uint8_t MENU_ITEM_COUNT;


/**
 * @brief 处理主菜单的导航和交互。
 * @details 此函数应在主循环中被调用。它负责读取旋转编码器的输入以在菜单项之间切换，
 *          并检测按钮点击以执行所选菜单项的 `action`。
 *          它还处理菜单切换时的动画效果。
 */
void showMenu();

/**
 * @brief 绘制并显示主菜单的初始界面。
 * @details 该函数负责清空屏幕，并根据当前的选中项和滚动偏移量，
 *          绘制出所有可见的菜单图标和标题。
 */
void showMenuConfig();

/**
 * @brief 二次缓出缓动函数。
 * @param t 输入的时间因子，范围从 0.0 到 1.0。
 * @return 返回经过缓动计算后的值，范围同样在 0.0 到 1.0。
 *         其效果是开始时快，然后逐渐减速。
 */
float easeOutQuad(float t);

#endif // MENU_H


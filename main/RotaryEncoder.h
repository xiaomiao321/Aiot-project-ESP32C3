#ifndef ROTARY_ENCODER_H
#define ROTARY_ENCODER_H

#include <Arduino.h>
#define ENCODER_CLK 1  // CLK 引脚
#define ENCODER_DT  0  // DT 引脚
#define ENCODER_SW  7  // SW 引脚（按钮）
/**
 * @brief 初始化旋转编码器。
 * @details 将编码器的CLK、DT和SW（按钮）引脚设置为上拉输入模式，并初始化内部状态。
 */
void initRotaryEncoder();

/**
 * @brief 读取编码器的旋转状态。
 * @details 通过检测CLK和DT引脚的相位变化来判断旋转方向。
 *          内置了软件消抖逻辑。
 * @return 返回一个整数表示旋转方向：1 代表顺时针，-1 代表逆时针，0 代表没有旋转。
 */
int readEncoder();

/**
 * @brief 检测按钮的短按点击事件。
 * @details 此函数能识别一次完整的“按下后释放”的动作，并能区分短按和长按。
 *          只有当一次短按（按下时间小于长按阈值）发生并释放后，才返回true。
 * @return 如果检测到一次短按点击，返回true；否则返回false。
 */
int readButton();

/**
 * @brief 检测按钮的长按事件。
 * @details 当按钮被按住超过预设的阈值（`longPressThreshold`）时，此函数返回true。
 *          为了提供用户反馈，当按住时间超过`progressBarStartTime`后，屏幕上会显示一个进度条。
 *          在一个按压周期内，长按事件只会被触发一次。
 * @return 如果检测到长按事件，返回true；否则返回false。
 */
bool readButtonLongPress();

#endif
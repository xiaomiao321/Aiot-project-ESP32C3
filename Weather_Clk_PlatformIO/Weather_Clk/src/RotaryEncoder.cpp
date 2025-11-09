#include "RotaryEncoder.h"
#include <TFT_eSPI.h>
#include "Menu.h"

// --- 旋转编码器状态变量 ---
static volatile int encoderPos = 0; // 编码器的绝对位置（累加值）
static uint8_t lastEncoded = 0;     // 上一次的编码状态，用于检测变化
static int deltaSum = 0;            // 增量和，用于更可靠的四分相解码
static unsigned long lastEncoderTime = 0; // 上次读取编码器的时间，用于消抖

// --- 按钮状态机变量 ---
static unsigned long buttonPressStartTime = 0; // 按钮被按下的起始时间戳
static bool longPressHandled = false;          // 标志位，表示当前按键事件是否已被作为长按处理

// --- 按钮配置 ---
#define SWAP_CLK_DT 0 // 如果编码器方向反了，将此值设为1来交换CLK和DT引脚
static const unsigned long longPressThreshold = 2000;     // 长按阈值：2秒
static const unsigned long progressBarStartTime = 1000;   // 按下1秒后开始显示长按进度条

/**
 * @brief 初始化旋转编码器的引脚和状态
 */
void initRotaryEncoder() {
  pinMode(ENCODER_CLK, INPUT_PULLUP); // CLK引脚设为上拉输入
  pinMode(ENCODER_DT, INPUT_PULLUP);  // DT引脚设为上拉输入
  pinMode(ENCODER_SW, INPUT_PULLUP);  // SW（按钮）引脚设为上拉输入
  // 读取初始状态
  lastEncoded = (digitalRead(ENCODER_CLK) << 1) | digitalRead(ENCODER_DT);
  Serial.println("Rotary Encoder Initialized.");
}

/**
 * @brief 读取编码器的旋转
 * @return 1 表示顺时针, -1 表示逆时针, 0 表示无变化
 * @details 这是一个更稳定可靠的编码器读取实现。
 *          它通过检测CLK引脚的下降沿，并在此刻判断DT引脚的状态来确定旋转方向。
 *          这种方法能有效避免因机械抖动导致的误判。
 */
int readEncoder() {
   static int lastCLK_State = HIGH; // 静态变量，保存CLK引脚上一次的状态
   int currentCLK_State = digitalRead(ENCODER_CLK); // 读取CLK引脚当前状态
   int delta = 0; // 返回值，0=无变化，1=顺时针，-1=逆时针

   // 检查CLK引脚的状态是否发生了变化（即出现了边沿）
   if (currentCLK_State != lastCLK_State) {
     // 简单软件消抖：如果状态变化了，等待一小段时间再确认，避免抖动
     delayMicroseconds(500); // 等待500微秒
     // 再次读取CLK和DT引脚，确保状态稳定
     currentCLK_State = digitalRead(ENCODER_CLK);
     int currentDT_State = digitalRead(ENCODER_DT);
     
     #if SWAP_CLK_DT
       // 如果配置了交换引脚，则交换CLK和DT的角色
       int temp = currentCLK_State; currentCLK_State = currentDT_State; currentDT_State = temp;
     #endif

     // 判断逻辑：在CLK的下降沿（从HIGH变为LOW）时判断DT的状态
     if (currentCLK_State == LOW) {
       if (currentDT_State == LOW) {
         // CLK为低时DT也为低 -> 顺时针旋转
         delta = 1;
         encoderPos++;
       } else {
         // CLK为低时DT为高 -> 逆时针旋转
         delta = -1;
         encoderPos--;
       }
     }
     // 更新上一次的CLK状态，为下一次判断做准备
     lastCLK_State = currentCLK_State;
   }
   return delta;
 }

/**
 * @brief 检测按钮的短按事件
 * @return 如果检测到一个完整的短按（按下后释放，且未被识别为长按），则返回true
 */
int readButton() {
    int buttonState = digitalRead(ENCODER_SW);
    bool triggered = false;

    if (buttonState == HIGH) { // 按钮被释放
        if (buttonPressStartTime != 0) { // 如果它之前是按下的
            // 如果它是一个短按（即未被标记为已处理的长按）
            if (!longPressHandled) {
                triggered = true;
            }
            // 无论如何，重置状态以准备下一次按键
            buttonPressStartTime = 0;
        }
    } else { // 按钮被按下
        if (buttonPressStartTime == 0) { // 如果是刚按下的瞬间
            buttonPressStartTime = millis(); // 记录开始时间
            longPressHandled = false; // 重置长按处理标志
        }
    }
    return triggered;
}

/**
 * @brief 检测按钮的长按事件
 * @return 当按住时间达到阈值时，返回true一次
 * @details 此函数还负责在屏幕上绘制一个进度条来可视化长按过程。
 */
bool readButtonLongPress() {
  int buttonState = digitalRead(ENCODER_SW);
  bool triggered = false;

  // 进度条的绘制参数
  static const int BAR_X = 50;
  static const int BAR_Y = 30;
  static const int BAR_WIDTH = 140;
  static const int BAR_HEIGHT = 10;

  // 仅当按钮被按住且未被作为长按处理过时，才进行处理
  if (buttonPressStartTime != 0 && !longPressHandled) {
    unsigned long currentHoldTime = millis() - buttonPressStartTime;

    // 按下时间超过progressBarStartTime后，开始显示进度条
    if (currentHoldTime >= progressBarStartTime) {
        float progress = (float)(currentHoldTime - progressBarStartTime) / (longPressThreshold - progressBarStartTime);
        if (progress > 1.0) progress = 1.0;

        menuSprite.drawRect(BAR_X, BAR_Y, BAR_WIDTH, BAR_HEIGHT, TFT_WHITE);
        menuSprite.fillRect(BAR_X + 2, BAR_Y + 2, (int)((BAR_WIDTH - 4) * progress), BAR_HEIGHT - 4, TFT_BLUE);
        menuSprite.pushSprite(0,0);
    }

    // 检查是否达到长按阈值
    if (currentHoldTime >= longPressThreshold) {
        triggered = true;
        longPressHandled = true; // 标记为已处理，避免重复触发
    }
  }

  // 如果按钮被释放，需要清理可能已绘制的进度条
  if (buttonState == HIGH) {
      // 检查进度条是否可能被画出过
      if (millis() - buttonPressStartTime < longPressThreshold && millis() - buttonPressStartTime > progressBarStartTime) {
          menuSprite.fillRect(BAR_X, BAR_Y, BAR_WIDTH, BAR_HEIGHT, TFT_BLACK); // 用背景色覆盖
          menuSprite.pushSprite(0,0);
      }
  }

  return triggered;
}

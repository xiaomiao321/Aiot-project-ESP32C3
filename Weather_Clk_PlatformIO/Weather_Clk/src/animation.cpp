// 包含核心库和模块头文件
#include "animation.h"      // 本模块的头文件
#include "Alarm.h"          // 闹钟模块，用于访问全局响铃状态
#include "Menu.h"           // 主菜单模块，用于访问全局TFT对象
#include "MQTT.h"           // MQTT模块，用于访问全局退出标志
#include "RotaryEncoder.h"  // 旋转编码器，用于处理用户输入
#include "Buzzer.h"         // 蜂鸣器模块，用于播放声音
#include "LED.h"            // LED模块，用于控制NeoPixel灯带

// --- 模块内部全局变量 ---
// 用于从外部通知动画任务停止的标志，volatile确保在多任务环境下被正确访问
volatile bool stopAnimationTask = false;
// 动画任务的句柄，用于从外部控制任务（如此处用于删除任务）
TaskHandle_t animationTaskHandle = NULL;

/**
 * @brief [FreeRTOS Task] 动画效果的核心任务
 * @param pvParameters 未使用
 * @details 此任务在一个循环中持续执行，实现一个视听联动的动画效果：
 *          1. 在TFT屏幕的随机位置绘制一个颜色、大小、粗细、角度都随机的平滑弧形。
 *          2. 将弧形的颜色同步到NeoPixel灯带上，使灯带显示相同的颜色。
 *          3. 播放一个随机频率的蜂鸣器音效。
 *          4. 动画的速度会随着时间逐渐加快。
 *          任务会在接收到 `stopAnimationTask` 信号后，进行清理并自行删除。
 */
void Animation_task(void *pvParameters)
{
  int delay_ms = 500; // 初始延迟时间，控制动画速度

  // 任务主循环，直到 stopAnimationTask 标志被置为 true
  while(!stopAnimationTask)
  {
    // 1. 绘制动画帧：生成随机参数来绘制一个平滑弧形
    uint16_t fg_color = random(0x10000); // 随机前景色
    uint16_t bg_color = TFT_BLACK;      // 背景色为黑色
    uint16_t x = random(tft.width());   // 随机中心点x坐标
    uint16_t y = random(tft.height());  // 随机中心点y坐标
    uint8_t radius = random(20, tft.width()/4); // 随机外半径
    uint8_t thickness = random(1, radius / 4);  // 随机厚度
    uint8_t inner_radius = radius - thickness;    // 计算内半径
    uint16_t start_angle = random(361); // 随机起始角度
    uint16_t end_angle = random(361);   // 随机结束角度
    bool arc_end = random(2);           // 随机决定弧形末端是否为圆形
    tft.drawSmoothArc(x, y, radius, inner_radius, start_angle, end_angle, fg_color, bg_color, arc_end);

    // 2. 更新NeoPixel灯带：将弧形颜色应用到灯带
    // 将16位的TFT颜色(RGB565)转换为24位的NeoPixel颜色(RGB888)
    uint8_t r = (fg_color & 0xF800) >> 8;
    uint8_t g = (fg_color & 0x07E0) >> 3;
    uint8_t b = (fg_color & 0x001F) << 3;
    strip.fill(strip.Color(r, g, b)); // 使用全局strip对象填充颜色
    strip.show(); // 显示颜色

    // 3. 播放音效
    tone(BUZZER_PIN, random(800, 1500), delay_ms); // 播放一个随机频率的声音，持续时间与延迟相同

    // 4. 控制动画速度
    vTaskDelay(pdMS_TO_TICKS(delay_ms)); // 等待一段时间

    // 逐渐加快动画速度（减小延迟）
    if (delay_ms > 50) {
      delay_ms -= 10;
    }
  }

  // --- 任务退出前的清理工作 ---
  noTone(BUZZER_PIN); // 关闭蜂鸣器
  strip.clear();      // 清除灯带颜色
  strip.show();       // 更新灯带显示（熄灭）
  animationTaskHandle = NULL; // 清空任务句柄
  vTaskDelete(NULL); // 删除任务自身
}

/**
 * @brief 动画演示的入口函数。
 * @details 此函数清空屏幕并启动一个FreeRTOS任务(`Animation_task`)，
 *          该任务会持续在TFT屏幕上绘制随机的平滑弧形，
 *          同时驱动NeoPixel灯带显示匹配的颜色，并播放随机的蜂鸣器音效，
 *          形成一个视听联动的动画效果。
 *          函数会监听退出信号（如按钮按下或闹钟触发），并负责安全地停止和清理相关任务。
 */
void AnimationMenu()
{
  tft.fillScreen(TFT_BLACK); // 清空屏幕
  
  // 初始化NeoPixel灯带，确保所有灯都熄灭
  strip.show();

  stopAnimationTask = false; // 重置任务停止标志
  
  // 确保没有旧的动画任务在运行，如果有则删除
  if(animationTaskHandle != NULL) {
    vTaskDelete(animationTaskHandle);
    animationTaskHandle = NULL;
  }

  // 创建并启动动画任务，固定在核心0上运行
  xTaskCreatePinnedToCore(Animation_task, "Animation_task", 2048, NULL, 1, &animationTaskHandle, 0);

  initRotaryEncoder(); // 初始化旋转编码器以接收用户输入

  // 循环等待退出信号
  while(true)
  {
    // 检查全局子菜单退出标志
    if (exitSubMenu) {
        exitSubMenu = false; // 重置标志
        if (animationTaskHandle != NULL) {
            stopAnimationTask = true; // 通知动画任务停止
        }
        vTaskDelay(pdMS_TO_TICKS(200)); // 等待任务处理停止信号
        break; // 退出循环
    }
    // 检查全局闹钟响铃标志
    if (g_alarm_is_ringing) {
        if (animationTaskHandle != NULL) {
            stopAnimationTask = true; // 通知动画任务停止
        }
        vTaskDelay(pdMS_TO_TICKS(200)); // 等待任务处理停止信号
        break; // 退出循环
    }
    // 检查按钮短按事件
    if(readButton())
    {
      // 通知动画任务停止
      if (animationTaskHandle != NULL) {
        stopAnimationTask = true;
      }

      // 等待任务自行删除
      vTaskDelay(pdMS_TO_TICKS(200)); 

      break; // 退出循环
    }
    // 短暂延迟，避免CPU空转
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}
// 包含所有必需的头文件
#include "LED.h"
#include "Alarm.h"
#include "RotaryEncoder.h"
#include <TFT_eSPI.h>
#include "Menu.h"
#include "MQTT.h" // 用于访问全局的 menuSprite
#include <Adafruit_NeoPixel.h>

// 初始化NeoPixel灯带对象
// 参数: LED数量, LED引脚, 像素类型(GRB)和信号频率(800KHz)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

// 定义控制模式的枚举
enum ControlMode { 
    BRIGHTNESS_MODE, // 亮度控制模式
    COLOR_MODE       // 颜色控制模式
};

/**
 * @brief 检测双击事件
 * @return 如果在400ms内检测到两次单击，则返回true
 */
bool led_detectDoubleClick() {
    static unsigned long lastClickTime = 0;
    unsigned long currentTime = millis();
    // 检查两次点击的时间间隔是否在50ms到400ms之间
    if (currentTime - lastClickTime > 50 && currentTime - lastClickTime < 400) {
        lastClickTime = 0; // 重置以便下一次检测
        return true;
    }
    lastClickTime = currentTime;
    return false;
}

/**
 * @brief 绘制LED控制界面
 * @param brightness 当前亮度值 (0-255)
 * @param hue 当前色相值 (0-65535)
 * @param currentMode 当前的控制模式 (亮度或颜色)
 * @details 使用全局的 menuSprite 对象来绘制UI，实现双缓冲以避免闪烁。
 */
void drawLedControl(uint8_t brightness, uint16_t hue, ControlMode currentMode) {
    menuSprite.fillSprite(TFT_BLACK); // 清空Sprite以防止伪影
    menuSprite.setTextColor(TFT_WHITE, TFT_BLACK);
    menuSprite.setTextSize(2);
    menuSprite.setTextDatum(TL_DATUM); // 设置文本基准为左上角

    // --- 亮度控制部分 ---
    menuSprite.drawString("Brightness", 20, 20);
    if (currentMode == BRIGHTNESS_MODE) {
        menuSprite.setTextColor(TFT_YELLOW, TFT_BLACK);
        menuSprite.drawString("<", 200, 20); // 使用'<'作为活动指示符
    }
    menuSprite.setTextColor(TFT_WHITE, TFT_BLACK);
    // 绘制亮度条
    int brightnessBarWidth = map(brightness, 0, 255, 0, 200);
    menuSprite.fillRect(20, 50, 200, 20, TFT_DARKGREY); // 绘制背景条
    menuSprite.fillRect(20, 50, brightnessBarWidth, 20, TFT_YELLOW); // 绘制表示当前亮度的部分

    // --- 颜色控制部分 ---
    menuSprite.drawString("Color", 20, 90);
    if (currentMode == COLOR_MODE) {
        menuSprite.setTextColor(TFT_YELLOW, TFT_BLACK);
        menuSprite.drawString("<", 200, 90); // 活动指示符
    }
    menuSprite.setTextColor(TFT_WHITE, TFT_BLACK);
    // 绘制颜色条
    int hueBarWidth = map(hue, 0, 65535, 0, 200);
    // 将HSV颜色转换为RGB，以便在TFT上正确显示
    uint32_t color = strip.ColorHSV(hue);
    uint16_t r = (color >> 16) & 0xFF;
    uint16_t g = (color >> 8) & 0xFF;
    uint16_t b = color & 0xFF;
    uint16_t barColor = tft.color565(r, g, b);
    menuSprite.fillRect(20, 120, 200, 20, TFT_DARKGREY); // 绘制背景条
    menuSprite.fillRect(20, 120, hueBarWidth, 20, barColor); // 绘制表示当前颜色的部分

    // --- 操作说明 ---
    menuSprite.setTextDatum(BC_DATUM); // 设置文本基准为底部中心
    menuSprite.setTextSize(1);
    menuSprite.drawString("Click: Toggle | Dbl-Click: Exit", 120, 230);
}

/**
 * @brief LED控制的主菜单函数
 * @details 管理LED的亮度、颜色控制，并处理用户输入。
 */
void LEDMenu() {
    initRotaryEncoder(); // 初始化旋转编码器

    // 初始化状态变量
    ControlMode currentMode = BRIGHTNESS_MODE;
    uint8_t brightness = strip.getBrightness();
    if (brightness == 0) { // 如果灯是灭的，则设置一个默认亮度
        brightness = 128;
    }
    uint16_t hue = 0; // 从红色开始

    // 应用初始设置
    strip.setBrightness(brightness);
    strip.fill(strip.ColorHSV(hue));
    strip.show();

    // 初始绘制UI
    drawLedControl(brightness, hue, currentMode);
    menuSprite.pushSprite(0, 0);

    bool needsRedraw = true; // 标志位，用于判断是否需要重绘屏幕

    while (true) {
        // 检查全局退出条件
        if (exitSubMenu || g_alarm_is_ringing) {
            exitSubMenu = false;
            break;
        }

        int encoderChange = readEncoder();
        if (encoderChange != 0) { // 如果编码器被旋转
            needsRedraw = true;
            if (currentMode == BRIGHTNESS_MODE) {
                // 调整亮度，增加灵敏度
                int newBrightness = brightness + (encoderChange * 10); 
                if (newBrightness < 0) newBrightness = 0;
                if (newBrightness > 255) newBrightness = 255;
                brightness = newBrightness;
                strip.setBrightness(brightness);
            } else { // COLOR_MODE
                // 调整色相，增加灵敏度
                int newHue = hue + (encoderChange * 2048); 
                if (newHue < 0) newHue = 65535 + newHue;
                hue = newHue % 65536;
            }
        }

        if (readButton()) { // 如果按钮被按下
            needsRedraw = true;
            if (led_detectDoubleClick()) {
                break; // 双击退出菜单
            } else {
                // 单击切换控制模式
                currentMode = (currentMode == BRIGHTNESS_MODE) ? COLOR_MODE : BRIGHTNESS_MODE;
            }
        }

        if (needsRedraw) { // 如果需要重绘
            strip.fill(strip.ColorHSV(hue)); // 根据当前色相填充颜色
            strip.show(); // 更新灯带显示
            drawLedControl(brightness, hue, currentMode); // 绘制UI
            menuSprite.pushSprite(0, 0); // 将Sprite内容推送到屏幕
            needsRedraw = false; // 重置标志位
        }

        vTaskDelay(pdMS_TO_TICKS(20)); // 短暂延时，避免CPU空转
    }
}
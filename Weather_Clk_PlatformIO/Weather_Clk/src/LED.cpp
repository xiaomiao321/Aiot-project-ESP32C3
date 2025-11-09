// 包含所有必需的头文件
#include "LED.h"
#include "Alarm.h"
#include "RotaryEncoder.h"
#include <TFT_eSPI.h>
#include "Menu.h"
#include "MQTT.h" 
#include <Adafruit_NeoPixel.h>

// 初始化NeoPixel灯带对象
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

// 彩虹动画任务的句柄
TaskHandle_t rainbowTaskHandle = NULL;
// 彩虹动画的速度
uint16_t rainbowSpeed = 20;

// 定义控制模式的枚举
enum ControlMode { 
    BRIGHTNESS_MODE, 
    COLOR_MODE       
};

// 函数前向声明
void stop_rainbow_task_if_running();

/**
 * @brief 将0-255范围内的值转换为彩虹色。
 * @param WheelPos 色轮上的位置 (0-255)。
 * @return 32位颜色值。
 */
uint32_t wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

/**
 * @brief [FreeRTOS Task] 彩虹动画任务
 * @param pvParameters 未使用。
 */
void rainbow_task(void *pvParameters) {
    uint16_t j = 0;
    for(;;) { // 无限循环
        for(int i=0; i<strip.numPixels(); i++) {
            strip.setPixelColor(i, wheel(((i * 256 / strip.numPixels()) + j) & 255));
        }
        strip.show();
        j++;
        if(j >= 256*5) j=0; // 循环5次后重置
        vTaskDelay(pdMS_TO_TICKS(rainbowSpeed));
    }
}

/**
 * @brief 如果彩虹动画任务正在运行，则停止它。
 */
void stop_rainbow_task_if_running() {
    if (rainbowTaskHandle != NULL) {
        vTaskDelete(rainbowTaskHandle);
        rainbowTaskHandle = NULL;
        led_off(); // 关闭LED
    }
}

/**
 * @brief 设置单个LED的颜色。
 */
void led_set_single(uint8_t index, uint8_t r, uint8_t g, uint8_t b) {
    stop_rainbow_task_if_running();
    if (index < strip.numPixels()) {
        strip.setPixelColor(index, r, g, b);
        strip.show();
    }
}

/**
 * @brief 将所有LED设置为相同的颜色。
 */
void led_set_all(uint8_t r, uint8_t g, uint8_t b) {
    stop_rainbow_task_if_running();
    strip.fill(strip.Color(r, g, b));
    strip.show();
}

/**
 * @brief 启动或更新彩虹动画模式。
 */
void led_rainbow_mode(uint16_t speed) {
    stop_rainbow_task_if_running();
    rainbowSpeed = speed;
    xTaskCreate(rainbow_task, "RainbowTask", 1024, NULL, 5, &rainbowTaskHandle);
}

/**
 * @brief 关闭所有LED。
 */
void led_off() {
    stop_rainbow_task_if_running();
    strip.clear();
    strip.show();
}


/**
 * @brief 检测双击事件
 * @return 如果在400ms内检测到两次单击，则返回true
 */
bool led_detectDoubleClick() {
    static unsigned long lastClickTime = 0;
    unsigned long currentTime = millis();
    if (currentTime - lastClickTime > 50 && currentTime - lastClickTime < 400) {
        lastClickTime = 0; 
        return true;
    }
    lastClickTime = currentTime;
    return false;
}

/**
 * @brief 绘制LED控制界面
 */
void drawLedControl(uint8_t brightness, uint16_t hue, ControlMode currentMode) {
    menuSprite.fillSprite(TFT_BLACK); 
    menuSprite.setTextColor(TFT_WHITE, TFT_BLACK);
    menuSprite.setTextSize(2);
    menuSprite.setTextDatum(TL_DATUM); 

    // --- 亮度控制部分 ---
    menuSprite.drawString("Brightness", 20, 20);
    if (currentMode == BRIGHTNESS_MODE) {
        menuSprite.setTextColor(TFT_YELLOW, TFT_BLACK);
        menuSprite.drawString("<", 200, 20); 
    }
    menuSprite.setTextColor(TFT_WHITE, TFT_BLACK);
    int brightnessBarWidth = map(brightness, 0, 255, 0, 200);
    menuSprite.fillRect(20, 50, 200, 20, TFT_DARKGREY); 
    menuSprite.fillRect(20, 50, brightnessBarWidth, 20, TFT_YELLOW); 

    // --- 颜色控制部分 ---
    menuSprite.drawString("Color", 20, 90);
    if (currentMode == COLOR_MODE) {
        menuSprite.setTextColor(TFT_YELLOW, TFT_BLACK);
        menuSprite.drawString("<", 200, 90); 
    }
    menuSprite.setTextColor(TFT_WHITE, TFT_BLACK);
    int hueBarWidth = map(hue, 0, 65535, 0, 200);
    uint32_t color = strip.ColorHSV(hue);
    uint16_t r = (color >> 16) & 0xFF;
    uint16_t g = (color >> 8) & 0xFF;
    uint16_t b = color & 0xFF;
    uint16_t barColor = tft.color565(r, g, b);
    menuSprite.fillRect(20, 120, 200, 20, TFT_DARKGREY); 
    menuSprite.fillRect(20, 120, hueBarWidth, 20, barColor); 

    // --- 操作说明 ---
    menuSprite.setTextDatum(BC_DATUM); 
    menuSprite.setTextSize(1);
    menuSprite.drawString("Click: Toggle | Dbl-Click: Exit", 120, 230);
}

/**
 * @brief LED控制的主菜单函数
 */
void LEDMenu() {
    stop_rainbow_task_if_running(); // 确保彩虹模式已停止
    initRotaryEncoder(); 

    ControlMode currentMode = BRIGHTNESS_MODE;
    uint8_t brightness = strip.getBrightness();
    if (brightness == 0) { 
        brightness = 128;
    }
    uint16_t hue = 0; 

    strip.setBrightness(brightness);
    strip.fill(strip.ColorHSV(hue));
    strip.show();

    drawLedControl(brightness, hue, currentMode);
    menuSprite.pushSprite(0, 0);

    bool needsRedraw = true; 

    while (true) {
        if (exitSubMenu || g_alarm_is_ringing) {
            exitSubMenu = false;
            break;
        }

        int encoderChange = readEncoder();
        if (encoderChange != 0) { 
            needsRedraw = true;
            if (currentMode == BRIGHTNESS_MODE) {
                int newBrightness = brightness + (encoderChange * 10); 
                if (newBrightness < 0) newBrightness = 0;
                if (newBrightness > 255) newBrightness = 255;
                brightness = newBrightness;
                strip.setBrightness(brightness);
            } else { 
                int newHue = hue + (encoderChange * 2048); 
                if (newHue < 0) newHue = 65535 + newHue;
                hue = newHue % 65536;
            }
        }

        if (readButton()) { 
            needsRedraw = true;
            if (led_detectDoubleClick()) {
                break; 
            } else {
                currentMode = (currentMode == BRIGHTNESS_MODE) ? COLOR_MODE : BRIGHTNESS_MODE;
            }
        }

        if (needsRedraw) { 
            strip.fill(strip.ColorHSV(hue)); 
            strip.show(); 
            drawLedControl(brightness, hue, currentMode); 
            menuSprite.pushSprite(0, 0); 
            needsRedraw = false; 
        }

        vTaskDelay(pdMS_TO_TICKS(20)); 
    }
}
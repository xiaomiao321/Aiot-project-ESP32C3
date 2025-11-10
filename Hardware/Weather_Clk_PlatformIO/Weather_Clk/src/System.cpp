#include <Arduino.h>
#include <EEPROM.h>
#include <TFT_eSPI.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <WiFi.h>
#include <driver/adc.h>
#include <esp_system.h>
#include "Alarm.h"
#include "WiFiManager.h"
#include "Menu.h"
#include "LED.h"
#include "Buzzer.h"
#include "weather.h"
#include "performance.h"
#include "DS18B20.h"
#include "System.h"
#include "ADC.h"
#include "img.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include "TargetSettings.h"
#include "MQTT.h"

#define EEPROM_SIZE 256
#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 240

// --- 外部变量声明 ---
extern DallasTemperature sensors;
extern OneWire oneWire;
extern Adafruit_NeoPixel strip;

// --- 全局对象实例化 ---
TFT_eSPI tft = TFT_eSPI();
TFT_eSprite menuSprite = TFT_eSprite(&tft); // 用于双缓冲的主Sprite

// --- 日志系统变量 ---
int tft_log_y = 40; // 日志在屏幕上开始的Y坐标
int current_log_lines = 0; // 当前已显示的日志行数

/**
 * @brief 设置所有NeoPixel LED的颜色
 * @param r 红色分量 (0-255)
 * @param g 绿色分量 (0-255)
 * @param b 蓝色分量 (0-255)
 */
void setLEDColor(uint8_t r, uint8_t g, uint8_t b)
{
    for (int i = 0; i < NUM_LEDS; i++)
    {
        strip.setPixelColor(i, strip.Color(r, g, b));
    }
    strip.show();
}

/**
 * @brief 在TFT屏幕上以打字机效果显示文本
 * @param text 要显示的文本
 * @param x 起始X坐标
 * @param y 起始Y坐标
 * @param color 文本颜色
 * @param delayMs 每个字符之间的延迟（毫秒）
 * @param playSound 是否播放打字音效
 */
void typeWriterEffect(const char *text, int x, int y, uint32_t color = TFT_WHITE, int delayMs = 30, bool playSound = false)
{
    tft.setTextColor(color, TFT_BLACK);
    tft.setTextSize(1);
    for (int i = 0; text[i] != '\0'; i++)
    {
        tft.drawChar(text[i], x + i * 6, y);
        if (playSound) { /* tone(BUZZER_PIN, 800, 10); */ }
        delay(delayMs);
    }
}

/**
 * @brief 以命令行风格（带"> "提示符）显示打字机效果文本
 */
void typeWriterCommand(const char *text, int x, int y, uint32_t color = TFT_WHITE, int delayMs = 30)
{
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.setCursor(x, y);
    tft.print("> ");
    typeWriterEffect(text, x + 12, y, color, delayMs, true);
}

/**
 * @brief 在日志区域显示系统硬件信息
 */
void displaySystemInfoTypewriter()
{
    // 芯片信息
#ifdef CONFIG_IDF_TARGET_ESP32C3
    tftLogInfo("Chip: ESP32-C3");
#else
    tftLogInfo("Chip: Unknown ESP");
#endif

    // 核心数
    char coresInfo[20];
    sprintf(coresInfo, "Cores: %d", 1);
    tftLogInfo(coresInfo);

    // CPU频率
    char cpuInfo[25];
    sprintf(cpuInfo, "CPU: %dMHz", ESP.getCpuFreqMHz());
    tftLogInfo(cpuInfo);

    // 闪存大小
    char flashInfo[35];
    sprintf(flashInfo, "Flash: %dMB", ESP.getFlashChipSize() / (1024 * 1024));
    tftLogInfo(flashInfo);

    // 可用堆内存
    char heapInfo[30];
    sprintf(heapInfo, "Heap: %d bytes", ESP.getFreeHeap());
    tftLogInfo(heapInfo);

    // SDK版本
    char sdkInfo[25];
    sprintf(sdkInfo, "SDK: %s", ESP.getSdkVersion());
    tftLogInfo(sdkInfo);
}

/**
 * @brief 显示存储在EEPROM中的用户设置
 */
void displayEepromInfo()
{
    tftClearLog();
    tftLogInfo("TARGET SETTINGS (from EEPROM)");
    tft.drawFastHLine(0, 55, SCREEN_WIDTH, TFT_DARKCYAN);

    char buffer[64];
    struct tm time_info;

    // 获取并显示进度条信息
    const ProgressBarInfo &progress = getProgressBarInfo();
    snprintf(buffer, sizeof(buffer), "Progress Title: %s", progress.title);
    tftLogInfo(buffer);

    localtime_r(&progress.startTime, &time_info);
    strftime(buffer, sizeof(buffer), "Start: %Y-%m-%d", &time_info);
    tftLogInfo(buffer);

    localtime_r(&progress.endTime, &time_info);
    strftime(buffer, sizeof(buffer), "End:   %Y-%m-%d", &time_info);
    tftLogInfo(buffer);

    // 获取并显示倒计时目标
    time_t countdown = getCountdownTarget();
    localtime_r(&countdown, &time_info);
    strftime(buffer, sizeof(buffer), "Target: %Y-%m-%d %H:%M", &time_info);
    tftLogInfo(buffer);

    // 获取并显示闹钟信息
    int alarmCount = getAlarmCount();
    if (alarmCount > 0)
    {
        tftLogInfo(" "); // Add a blank line for spacing
        tftLogInfo("Alarms:");
        for (int i = 0; i < alarmCount; ++i)
        {
            AlarmSetting alarm;
            if (getAlarmInfo(i, alarm))
            {
                char days_str[8] = { 0 };
                const char *day_letters = "SMTWTFS";
                for (int d = 0; d < 7; ++d)
                {
                    if (alarm.days_of_week & (1 << d))
                    {
                        days_str[strlen(days_str)] = day_letters[d];
                    }
                }
                snprintf(buffer, sizeof(buffer), " - %02d:%02d %s %s", alarm.hour, alarm.minute, alarm.enabled ? "ON" : "OFF", days_str);
                tftLogInfo(buffer);
            }
        }
    }
}

/**
 * @brief 执行硬件自检并显示结果
 */
void hardwareTest()
{
    // 蜂鸣器测试
    tftLogInfo("Testing Buzzer...");
    tone(BUZZER_PIN, 1500, 200);
    delay(300);
    tftLogSuccess("Buzzer: OK");

    // DS18B20温度传感器测试
    tftLogInfo("Testing DS18B20...");
    sensors.begin();
    if (sensors.getDeviceCount() == 0)
    {
        tftLogError("No DS18B20 sensors detected");
    }
    else
    {
        tftLogSuccess("DS18B20: OK");
    }

    // LED灯带测试
    tftLogInfo("Testing LEDs...");
    setLEDColor(255, 0, 0); delay(200);
    setLEDColor(0, 255, 0); delay(200);
    setLEDColor(0, 0, 255); delay(200);
    setLEDColor(0, 0, 0);
    tftLogSuccess("LEDs: OK");

    // ADC测试
    tftLogInfo("Testing ADC...");
    char adcStr[25];
    sprintf(adcStr, "ADC: %lu OK", (unsigned long) adc1_get_raw(ADC1_CHANNEL_2));
    tftLogSuccess(adcStr);
}

/**
 * @brief 播放开机动画和显示启动信息
 */
void bootAnimation()
{
    // 播放开机音乐
    static int boot_song_index = numSongs - 1; // "Windows XP"
    xTaskCreatePinnedToCore(Buzzer_PlayMusic_Task, "BootSound", 8192, &boot_song_index, 1, NULL, 0);

    // 播放GIF动画
    const uint16_t *boot_gif[] = { huaji_0,huaji_1,huaji_2,huaji_3,huaji_4,huaji_5,huaji_6,huaji_7,huaji_8,huaji_9,huaji_10,huaji_11,huaji_12,huaji_13,huaji_14,huaji_15 };
    for (int k = 0; k < 3; k++)
    { // 播放3遍
        for (int i = 0; i < 16; i++)
        {
            menuSprite.fillScreen(TFT_WHITE);
            menuSprite.pushImage(46, 45, 150, 148, boot_gif[i]);
            menuSprite.pushSprite(0, 0);
            delay(50);
        }
    }

    tft.fillScreen(TFT_BLACK);
    typeWriterEffect("SYSTEM BOOT SEQUENCE", 20, 5, TFT_GREEN, 50);
    tft.drawFastHLine(0, 25, SCREEN_WIDTH, TFT_DARKGREEN);

    displaySystemInfoTypewriter(); // 显示系统信息
    delay(1000);
    hardwareTest(); // 执行硬件测试
    delay(1000);

    // 完成提示
    tft.fillScreen(TFT_BLACK);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_GREEN);
    tft.drawString("All systems operational", 120, 80);
    tft.drawString("Start Connecting to WiFi...", 120, 160);
    delay(1500);
}

/**
 * @brief 系统总初始化函数
 * @details 在 `setup()` 中被调用，负责所有硬件和软件模块的初始化。
 */
void bootSystem()
{
    Serial.begin(115200);
    EEPROM.begin(EEPROM_SIZE);

    // 初始化硬件
    Buzzer_Init();
    initRotaryEncoder();
    DS18B20_Init();
    createDS18B20Task(); // 创建后台温度读取任务
    tft.init();
    tft.setRotation(1);
    tft.fillScreen(TFT_BLACK);
    menuSprite.createSprite(240, 240); // 创建与屏幕同样大小的Sprite
    TargetSettings_Init();
    setupADC();
    startADC(); // 启动ADC后台读取任务
    startPerformanceMonitoring(); // 启动性能数据后台接收任务

    // 开机动画和自检
    bootAnimation();

    Alarm_Init(); // 初始化闹钟模块
    connectWiFi_with_Manager(); // 连接WiFi
    syncTime();
    xTaskCreate(TimeUpdate_Task, "Time Update Task", 2048, NULL, 5, NULL);
    setupMQTT(); // 设置MQTT
    connectMQTT(); // 连接MQTT

    tft.fillScreen(TFT_BLACK);
    showMenuConfig(); // 显示主菜单
}

// --- 日志功能辅助函数 ---

/**
 * @brief 清空屏幕上的日志区域
 */
void tftClearLog()
{
    tft.fillScreen(TFT_BLACK);
    tft_log_y = 40;
    current_log_lines = 0;
}

/**
 * @brief 在屏幕上以打字机效果显示一行日志
 * @param text 日志文本
 * @param color 文本颜色
 */
void tftLog(String text, uint16_t color)
{
    tft.setTextFont(1);
    tft.setTextSize(1);
    // 如果日志满了，则等待1秒后清屏
    if (current_log_lines >= LOG_MAX_LINES)
    {
        delay(1000);
        tftClearLog();
    }
    tft.fillRect(LOG_MARGIN, tft_log_y, tft.width() - LOG_MARGIN * 2, LOG_LINE_HEIGHT, TFT_BLACK);
    String fullText = "> " + text;
    typeWriterEffect(fullText.c_str(), LOG_MARGIN, tft_log_y, color, 8, true);
    tft_log_y += LOG_LINE_HEIGHT;
    current_log_lines++;
}

// --- 不同日志级别的便捷函数 ---
void tftLogInfo(const String &text) { tftLog(text, TFT_CYAN); }
void tftLogWarning(const String &text) { tftLog(text, TFT_YELLOW); }
void tftLogError(const String &text) { tftLog(text, TFT_RED); }
void tftLogSuccess(const String &text) { tftLog(text, TFT_GREEN); }
void tftLogDebug(const String &text) { tftLog(text, TFT_MAGENTA); }

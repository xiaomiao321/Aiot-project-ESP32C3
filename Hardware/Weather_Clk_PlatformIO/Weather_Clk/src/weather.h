#ifndef WEATHER_H
#define WEATHER_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <time.h>
#include "Menu.h"

// TFT 布局
#define DATA_X 3
#define DATA_Y 24
#define LINE_HEIGHT 16
#define TEXT_COLOR TFT_WHITE
#define BG_COLOR TFT_BLACK
#define TITLE_COLOR TFT_YELLOW
#define VALUE_COLOR TFT_CYAN
#define ERROR_COLOR TFT_RED

extern bool wifi_connected;
extern struct tm timeinfo; // Global timeinfo for watchfaces
extern char temperature[10];
extern char humidity[10];
extern char reporttime[25];
extern char lastSyncTimeStr[45]; // To display last sync time
extern char lastWeatherSyncStr[45]; // To display last weather sync status
extern bool synced;

/**
 * @brief 天气和时间功能的主菜单入口。
 * @details 此函数按顺序执行WiFi连接、NTP时间同步和天气数据获取。
 *          每个步骤都会在屏幕上显示详细的进度和结果。
 *          全部完成后，会自动进入表盘选择菜单(`WatchfaceMenu`)。
 */
void weatherMenu();

/**
 * @brief 同步网络时间。
 * @details 使用预设的NTP服务器来获取并设置系统的UTC时间。
 *          如果WiFi未连接，则会提示错误。同步成功或失败都会在屏幕上显示状态。
 */
void syncTime();

/**
 * @brief 连接到WiFi网络。
 * @details 扫描周边的WiFi，然后尝试连接到代码中预设的SSID和密码。
 *          连接过程和结果（包括IP地址、信号强度等）会详细显示在屏幕上。
 * @return 如果连接成功，返回 `true`；否则返回 `false`。
 */
bool connectWiFi();

/**
 * @brief 从API获取天气数据。
 * @details 在WiFi连接的情况下，从高德天气API获取指定城市的天气信息，
 *          包括温度、湿度和发布时间，并更新到全局变量中。
 * @return 如果成功获取并解析数据，返回 `true`；否则返回 `false`。
 */
bool fetchWeather();

/**
 * @brief 在后台静默同步时间。
 * @details 此函数用于在不打扰用户界面的情况下，在后台进行NTP时间同步。
 *          它会更新全局的时间同步状态字符串，但不会显示全屏UI。
 */
void silentSyncTime();

/**
 * @brief 在后台静默获取天气数据。
 * @details 用于在后台更新天气信息，而不需要显示全屏的加载界面。
 *          它会更新全局的天气数据和天气同步状态字符串。
 */
void silentFetchWeather();

/**
 * @brief 确保WiFi处于连接状态。
 * @details 这是一个非阻塞函数，用于检查和管理WiFi连接。如果未连接，
 *          它会启动一个带超时的连接过程。如果连接失败，它会进入一个带延迟的重试周期。
 * @return 如果当前WiFi已连接，返回 `true`；否则返回 `false`。
 */
bool ensureWiFiConnected();

/**
 * @brief [FreeRTOS Task] 时间更新任务。
 * @param pvParameters 任务创建时传入的参数（未使用）。
 * @details 这是一个周期性任务，每秒钟调用一次 `getLocalTime()` 来更新全局的 `timeinfo` 结构体，
 *          以确保系统时间（特别是秒数）的实时性。
 */
void TimeUpdate_Task(void *pvParameters);

#endif
#include "Stopwatch.h"
#include "Alarm.h"
#include "Menu.h"
#include "MQTT.h"
#include "Buzzer.h"
#include "RotaryEncoder.h"
#include "weather.h"

// --- 秒表状态的全局变量 ---
static unsigned long stopwatch_start_time = 0;   // 秒表开始或恢复运行的时间戳
static unsigned long stopwatch_elapsed_time = 0; // 秒表暂停时已经过的时间
static bool stopwatch_running = false;           // 秒表是否正在运行的标志
static unsigned long stopwatch_pause_time = 0;   // (未使用) 之前用于记录暂停时间

/**
 * @brief 显示秒表时间的用户界面
 * @param elapsed_millis 已经过的总毫秒数
 * @details 此函数负责在屏幕上绘制整个秒表界面，包括：
 *          - 顶部的当前系统时间
 *          - 中央大号字体显示的经过时间（分:秒.百分之一秒）
 *          - 底部的状态文本（运行、暂停、就绪）
 */
void displayStopwatchTime(unsigned long elapsed_millis) {
    menuSprite.fillScreen(TFT_BLACK); // 清空Sprite
    menuSprite.setTextDatum(TL_DATUM); // 设置文本基准为左上角

    // 在顶部显示当前时间
    if (getLocalTime(&timeinfo)) {
        char time_str[30];
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S %a", &timeinfo);
        menuSprite.setTextFont(2);
        menuSprite.setTextSize(1);
        menuSprite.setTextColor(TFT_WHITE, TFT_BLACK);
        menuSprite.setTextDatum(MC_DATUM);
        menuSprite.drawString(time_str, menuSprite.width() / 2, 10);
        menuSprite.setTextDatum(TL_DATUM);
    }

    // --- 时间计算 ---
    unsigned long total_seconds = elapsed_millis / 1000;
    long minutes = total_seconds / 60;
    long seconds = total_seconds % 60;
    long hundredths = (elapsed_millis % 1000) / 10;

    // --- 字体和位置计算 ---
    menuSprite.setTextFont(7); // 使用大号字体
    menuSprite.setTextSize(1);
    int num_w = menuSprite.textWidth("8");
    int colon_w = menuSprite.textWidth(":");
    int dot_w = menuSprite.textWidth(".");
    int num_h = menuSprite.fontHeight();
    int total_width = (num_w * 6) + colon_w + dot_w; // MM:SS.hh
    int current_x = (menuSprite.width() - total_width) / 2;
    int y_pos = (menuSprite.height() / 2) - (num_h / 2) - 20;
    char buf[3];

    // --- 绘制时间 ---
    sprintf(buf, "%02ld", minutes);
    menuSprite.drawString(buf, current_x, y_pos);
    current_x += num_w * 2;
    menuSprite.drawString(":", current_x, y_pos);
    current_x += colon_w;
    sprintf(buf, "%02ld", seconds);
    menuSprite.drawString(buf, current_x, y_pos);
    current_x += num_w * 2;
    menuSprite.drawString(".", current_x, y_pos);
    current_x += dot_w;
    sprintf(buf, "%02ld", hundredths);
    menuSprite.drawString(buf, current_x, y_pos);

    // --- 显示状态文本 ---
    menuSprite.setTextFont(2);
    menuSprite.setTextDatum(BC_DATUM); // 文本基准设为底部中心
    if (stopwatch_running) {
        menuSprite.drawString("RUNNING", menuSprite.width() / 2, menuSprite.height() - 80);
    } else if (elapsed_millis > 0) {
        menuSprite.drawString("PAUSED", menuSprite.width() / 2, menuSprite.height() - 80);
    } else {
        menuSprite.drawString("READY", menuSprite.width() / 2, menuSprite.height() - 80);
    }

    menuSprite.pushSprite(0, 0); // 将Sprite内容推送到屏幕
}

/**
 * @brief 秒表功能的主菜单函数
 * @details 管理秒表的启动、暂停、恢复和重置逻辑，并处理用户输入。
 */
void StopwatchMenu() {
    // 进入菜单时重置所有状态
    stopwatch_start_time = 0;
    stopwatch_elapsed_time = 0;
    stopwatch_running = false;
    stopwatch_pause_time = 0;
    displayStopwatchTime(0); // 初始绘制界面
    
    static unsigned long last_displayed_stopwatch_millis = 0;
    unsigned long last_realtime_clock_update = millis();

    while (true) {
        // 检查全局退出条件
        if (exitSubMenu || g_alarm_is_ringing) {
            exitSubMenu = false;
            return;
        }

        // 长按按钮重置并退出
        if (readButtonLongPress()) {
            tone(BUZZER_PIN, 1500, 100); // 退出提示音
            // 重置所有状态
            stopwatch_start_time = 0;
            stopwatch_elapsed_time = 0;
            stopwatch_running = false;
            menuSprite.setTextFont(1); menuSprite.setTextSize(1); // 恢复默认字体
            return; // 退出函数，返回主菜单
        }

        // 短按按钮用于开始/暂停
        if (readButton()) {
            tone(BUZZER_PIN, 2000, 50); // 确认音
            if (stopwatch_running) { // 如果正在运行 -> 暂停
                // 累加本次运行的时间
                stopwatch_elapsed_time += (millis() - stopwatch_start_time);
                stopwatch_running = false;
            } else { // 如果已暂停或停止 -> 开始/恢复
                stopwatch_start_time = millis(); // 记录新的开始时间
                stopwatch_running = true;
            }
            // 状态改变后立即更新显示
            unsigned long current_display_value = stopwatch_running ? 
                (stopwatch_elapsed_time + (millis() - stopwatch_start_time)) : 
                stopwatch_elapsed_time;
            displayStopwatchTime(current_display_value);
            last_displayed_stopwatch_millis = current_display_value;
        }

        // --- 频繁更新显示 ---
        unsigned long current_stopwatch_display_millis;
        if (stopwatch_running) {
            current_stopwatch_display_millis = stopwatch_elapsed_time + (millis() - stopwatch_start_time);
        } else {
            current_stopwatch_display_millis = stopwatch_elapsed_time;
        }
        
        // 如果百分之一秒发生变化，或者顶部的实时时钟需要更新，则重绘整个屏幕
        if (current_stopwatch_display_millis / 10 != last_displayed_stopwatch_millis / 10 || (millis() - last_realtime_clock_update) >= 1000) {
            displayStopwatchTime(current_stopwatch_display_millis);
            last_displayed_stopwatch_millis = current_stopwatch_display_millis;
            if ((millis() - last_realtime_clock_update) >= 1000) {
                last_realtime_clock_update = millis();
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10)); // 短暂延时，防止CPU空转
    }
}

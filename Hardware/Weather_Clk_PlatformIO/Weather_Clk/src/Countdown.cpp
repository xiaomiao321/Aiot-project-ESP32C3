// 包含所有必需的头文件
#include "Countdown.h"
#include "Alarm.h"
#include "Menu.h"
#include "MQTT.h"
#include "Buzzer.h"
#include "RotaryEncoder.h"
#include "weather.h"

// --- 状态变量 ---
static unsigned long countdown_target_millis = 0;   // 倒计时目标结束的毫秒时间戳
static unsigned long countdown_start_millis = 0;    // 倒计时开始的毫秒时间戳
static bool countdown_running = false;              // 倒计时是否正在运行的标志
static bool countdown_paused = false;               // 倒计时是否已暂停的标志
static unsigned long countdown_pause_time = 0;      // 倒计时被暂停的毫秒时间戳
static long countdown_duration_seconds = 5 * 60;    // 倒计时的总时长（秒），默认为5分钟
static unsigned long last_countdown_beep_time = 0;  // 用于最后5秒警告蜂鸣的时间记录

// --- UI控制状态 ---
// 定义了设置倒计时的不同模式
enum CountdownSettingMode
{
    MODE_MINUTES,         // 正在设置分钟
    MODE_SECONDS,         // 正在设置秒钟
    MODE_READY_TO_START   // 设置完成，准备开始
};
static CountdownSettingMode countdown_setting_mode = MODE_MINUTES; // 初始模式为设置分钟

/**
 * @brief 绘制倒计时用户界面
 * @param millis_left 剩余的倒计时毫秒数
 * @details 此函数负责在屏幕上绘制整个倒计时界面，包括：
 *          - 顶部的当前系统时间
 *          - 中央大号字体显示的剩余时间（分、秒、百分之一秒）
 *          - 根据当前状态（设置、运行、暂停、完成）显示不同的文本提示和按钮
 *          - 底部的进度条，可视化地展示时间流逝
 */
void displayCountdownTime(unsigned long millis_left)
{
    menuSprite.fillScreen(TFT_BLACK); // 清空Sprite以进行重绘
    menuSprite.setTextDatum(TL_DATUM); // 设置文本基准为左上角

    // 在顶部显示当前时间
    if (!getLocalTime(&timeinfo))
    {
    }
    else
    {
        char time_str[30];
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S %a", &timeinfo); // 格式化时间字符串
        menuSprite.setTextFont(2);
        menuSprite.setTextSize(1);
        menuSprite.setTextColor(TFT_WHITE, TFT_BLACK);
        menuSprite.setTextDatum(MC_DATUM); // 居中对齐
        menuSprite.drawString(time_str, menuSprite.width() / 2, 10); // 在屏幕顶部中心绘制
        menuSprite.setTextDatum(TL_DATUM); // 重置文本基准
    }

    // --- 时间计算 ---
    unsigned long total_seconds = millis_left / 1000;
    long minutes = total_seconds / 60;
    long seconds = total_seconds % 60;
    long hundredths = (millis_left % 1000) / 10; // 计算百分之一秒

    // --- 字体和位置计算 ---
    menuSprite.setTextFont(7); // 使用大号字体显示时间
    menuSprite.setTextSize(1);
    int num_w = menuSprite.textWidth("8");
    int colon_w = menuSprite.textWidth(":");
    int dot_w = menuSprite.textWidth(".");
    int num_h = menuSprite.fontHeight();
    bool show_hundredths = (countdown_running || countdown_paused); // 仅在运行时或暂停时显示百分之一秒
    int total_width = (num_w * 4) + colon_w + (show_hundredths ? (dot_w + num_w * 2) : 0);
    int current_x = (menuSprite.width() - total_width) / 2;
    int y_pos = (menuSprite.height() / 2) - (num_h / 2) - 20;
    char buf[3];

    // --- 根据设置模式高亮显示分钟和秒 ---
    // 设置分钟时，分钟部分黄底黑字
    if (!countdown_running && !countdown_paused && countdown_setting_mode == MODE_MINUTES)
    {
        menuSprite.setTextColor(TFT_BLACK, TFT_YELLOW);
    }
    else
    {
        menuSprite.setTextColor(TFT_WHITE, TFT_BLACK);
    }
    sprintf(buf, "%02ld", minutes);
    menuSprite.drawString(buf, current_x, y_pos);
    current_x += num_w * 2;

    menuSprite.setTextColor(TFT_WHITE, TFT_BLACK); // 冒号永不高亮
    menuSprite.drawString(":", current_x, y_pos);
    current_x += colon_w;

    // 设置秒钟时，秒钟部分黄底黑字
    if (!countdown_running && !countdown_paused && countdown_setting_mode == MODE_SECONDS)
    {
        menuSprite.setTextColor(TFT_BLACK, TFT_YELLOW);
    }
    else
    {
        menuSprite.setTextColor(TFT_WHITE, TFT_BLACK);
    }
    sprintf(buf, "%02ld", seconds);
    menuSprite.drawString(buf, current_x, y_pos);
    current_x += num_w * 2;

    menuSprite.setTextColor(TFT_WHITE, TFT_BLACK);

    // --- 如果需要，绘制百分之一秒 ---
    if (show_hundredths)
    {
        menuSprite.drawString(".", current_x, y_pos);
        current_x += dot_w;
        sprintf(buf, "%02ld", hundredths);
        menuSprite.drawString(buf, current_x, y_pos);
    }

    // --- 显示状态文本 ---
    menuSprite.setTextFont(2);
    menuSprite.setTextDatum(BC_DATUM); // 文本基准设为底部中心
    if (countdown_running)
    {
        menuSprite.drawString("RUNNING", menuSprite.width() / 2, menuSprite.height() - 80);
    }
    else if (countdown_paused)
    {
        menuSprite.drawString("PAUSED", menuSprite.width() / 2, menuSprite.height() - 80);
    }
    else if (millis_left == 0 && countdown_duration_seconds > 0)
    {
        menuSprite.drawString("FINISHED", menuSprite.width() / 2, menuSprite.height() - 80);
    }
    else
    { // 准备开始或正在设置时间
        if (countdown_setting_mode == MODE_MINUTES)
        {
            menuSprite.drawString("SET MINUTES", menuSprite.width() / 2, menuSprite.height() - 40);
        }
        else if (countdown_setting_mode == MODE_SECONDS)
        {
            menuSprite.drawString("SET SECONDS", menuSprite.width() / 2, menuSprite.height() - 40);
        }
        else
        { // MODE_READY_TO_START
             // 绘制一个绿色的“START”按钮
            menuSprite.fillRoundRect(80, menuSprite.height() - 60, 80, 40, 5, TFT_GREEN);
            menuSprite.setTextColor(TFT_BLACK);
            menuSprite.drawString("START", menuSprite.width() / 2, menuSprite.height() - 40);
        }
    }

    // --- 绘制进度条 ---
    float progress = (countdown_duration_seconds > 0) ? 1.0 - (float) millis_left / (countdown_duration_seconds * 1000.0) : 0.0;
    if (progress < 0) progress = 0; if (progress > 1) progress = 1;
    menuSprite.drawRect(20, menuSprite.height() / 2 + 40, menuSprite.width() - 40, 20, TFT_WHITE); // 边框
    menuSprite.fillRect(22, menuSprite.height() / 2 + 42, (int) ((menuSprite.width() - 44) * progress), 16, TFT_GREEN); // 填充

    menuSprite.pushSprite(0, 0); // 将Sprite内容推送到屏幕
}

/**
 * @brief 倒计时功能的主菜单函数
 * @details 此函数是倒计时功能的入口点。它管理着一个循环，处理用户输入（旋转编码器、按钮），
 *          并根据当前状态（设置、运行、暂停）更新倒计时逻辑和UI显示。
 */
void CountdownMenu()
{
    // 初始化状态
    countdown_running = false;
    countdown_paused = false;
    countdown_duration_seconds = 5 * 60; // 重置为默认的5分钟
    countdown_setting_mode = MODE_MINUTES; // 从设置分钟模式开始

    displayCountdownTime(countdown_duration_seconds * 1000); // 初始绘制界面

    unsigned long last_display_update_time = millis();

    while (true)
    {
        // 检查全局退出条件
        if (exitSubMenu) { exitSubMenu = false; return; }
        if (g_alarm_is_ringing) { return; }
        if (readButtonLongPress())
        { // 长按退出
            tone(BUZZER_PIN, 1500, 100);
            menuSprite.setTextFont(1); menuSprite.setTextSize(1); // 恢复默认字体
            return;
        }

        int encoder_value = readEncoder();
        bool button_pressed = readButton();

        // --- 逻辑：当计时器未运行时 (设置阶段) ---
        if (!countdown_running && !countdown_paused)
        {
            if (encoder_value != 0)
            { // 旋转编码器调整时间
                if (countdown_setting_mode == MODE_MINUTES)
                {
                    countdown_duration_seconds += encoder_value * 60;
                }
                else if (countdown_setting_mode == MODE_SECONDS)
                {
                    long current_minutes = countdown_duration_seconds / 60;
                    long current_seconds = countdown_duration_seconds % 60;
                    current_seconds += encoder_value;
                    // 处理秒数的进位和借位
                    if (current_seconds >= 60) { current_seconds = 0; current_minutes++; }
                    else if (current_seconds < 0) { current_seconds = 59; if (current_minutes > 0) current_minutes--; }
                    countdown_duration_seconds = (current_minutes * 60) + current_seconds;
                }
                if (countdown_duration_seconds < 0) countdown_duration_seconds = 0; // 防止负数
                displayCountdownTime(countdown_duration_seconds * 1000);
                tone(BUZZER_PIN, 1000, 20); // 提示音
            }

            if (button_pressed)
            { // 短按按钮
                tone(BUZZER_PIN, 2000, 50);
                if (countdown_setting_mode == MODE_READY_TO_START)
                {
                    // 准备就绪，开始计时
                    if (countdown_duration_seconds > 0)
                    {
                        countdown_start_millis = millis();
                        countdown_target_millis = countdown_start_millis + (countdown_duration_seconds * 1000);
                        countdown_running = true;
                        countdown_paused = false;
                        last_countdown_beep_time = 0; // 重置蜂鸣计时器
                    }
                    else
                    { // 如果时间为0，则重置模式
                        countdown_setting_mode = MODE_MINUTES;
                    }
                }
                else
                {
                    // 循环切换设置模式: 分 -> 秒 -> 准备开始
                    countdown_setting_mode = (CountdownSettingMode) ((countdown_setting_mode + 1) % 3);
                }
                displayCountdownTime(countdown_duration_seconds * 1000);
            }
        }
        // --- 逻辑：当计时器正在运行或已暂停时 ---
        else
        {
            if (button_pressed)
            { // 短按按钮用于暂停/继续
                tone(BUZZER_PIN, 2000, 50);
                if (countdown_running)
                { // 如果正在运行 -> 暂停
                    countdown_pause_time = millis();
                    countdown_running = false;
                    countdown_paused = true;
                }
                else
                { // 如果已暂停 -> 继续
                             // 调整开始时间，以补偿暂停期间经过的时间
                    countdown_start_millis += (millis() - countdown_pause_time);
                    countdown_target_millis = countdown_start_millis + (countdown_duration_seconds * 1000);
                    countdown_running = true;
                    countdown_paused = false;
                }
            }
        }

        // --- 如果正在运行，则更新显示 ---
        if (countdown_running)
        {
            unsigned long current_millis = millis();
            long millis_left = countdown_target_millis - current_millis;
            if (millis_left < 0) millis_left = 0;

            // 频繁更新倒计时显示（每百分之一秒）
            if (millis_left / 10 != (countdown_target_millis - last_display_update_time) / 10)
            {
                displayCountdownTime(millis_left);
                last_display_update_time = current_millis;
            }

            // 最后5秒警告：每秒蜂鸣一次
            if (millis_left > 0 && millis_left <= 5000 && (current_millis - last_countdown_beep_time >= 1000 || last_countdown_beep_time == 0))
            {
                tone(BUZZER_PIN, 1000, 100);
                last_countdown_beep_time = current_millis;
            }

            // 倒计时结束
            if (millis_left == 0)
            {
                countdown_running = false;
                countdown_paused = false;
                tone(BUZZER_PIN, 3000, 2000); // 长鸣2秒作为结束提示
                displayCountdownTime(0);
            }
        }
        else
        { // 如果倒计时未运行（暂停或设置中），则只更新顶部的实时时钟
            unsigned long current_millis = millis();
            if (current_millis - last_display_update_time >= 1000)
            { // 每秒更新一次
                displayCountdownTime(countdown_duration_seconds * 1000);
                last_display_update_time = current_millis;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10)); // 短暂延时，防止CPU空转
    }
}
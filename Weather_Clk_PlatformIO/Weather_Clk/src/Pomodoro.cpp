#include "Pomodoro.h"
#include "Alarm.h"
#include "Menu.h"
#include "RotaryEncoder.h"
#include "Buzzer.h"
#include "animation.h" // 用于 drawSmoothArc
#include "weather.h"

// --- 配置常量 ---
const unsigned long WORK_DURATION_SECS = 25 * 60;       // 工作时长（25分钟）
const unsigned long SHORT_BREAK_DURATION_SECS = 5 * 60; // 短休息时长（5分钟）
const unsigned long LONG_BREAK_DURATION_SECS = 15 * 60; // 长休息时长（15分钟）
const int SESSIONS_BEFORE_LONG_BREAK = 4;               // 多少个工作会话后进行一次长休息

// --- 状态机枚举 ---
enum PomodoroState
{
    STATE_IDLE,        // 空闲状态，等待开始
    STATE_WORK,        // 工作状态
    STATE_SHORT_BREAK, // 短休息状态
    STATE_LONG_BREAK,  // 长休息状态
    STATE_PAUSED       // 暂停状态
};

// --- 模块内部静态状态变量 ---
static PomodoroState currentState = STATE_IDLE;      // 当前状态
static PomodoroState stateBeforePause = STATE_IDLE;  // 暂停前的状态，用于恢复
static unsigned long session_end_millis = 0;         // 当前会话结束的毫秒时间戳
static unsigned long remaining_on_pause = 0;         // 暂停时剩余的毫秒数
static int sessions_completed = 0;                   // 已完成的工作会话次数
static unsigned long last_pomodoro_beep_time = 0;    // 用于最后5秒警告蜂鸣的时间记录

// =====================================================================================
//                                     UI绘制逻辑
// =====================================================================================

/**
 * @brief 绘制番茄钟的用户界面
 * @param remaining_secs 剩余的秒数
 * @param total_secs 当前会话的总秒数
 * @details 此函数负责在屏幕上绘制整个番茄钟界面，包括：
 *          - 顶部的当前系统时间
 *          - 中央的圆形进度条和剩余时间
 *          - 当前的状态文本（工作、休息等）
 *          - 底部的已完成会话标记
 */
static void drawPomodoroUI(unsigned long remaining_secs, unsigned long total_secs)
{
    menuSprite.fillScreen(TFT_BLACK); // 清空Sprite

    // 在顶部显示当前时间
    if (getLocalTime(&timeinfo))
    {
        char time_str[30];
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S %a", &timeinfo);
        menuSprite.setTextFont(2);
        menuSprite.setTextSize(1);
        menuSprite.setTextColor(TFT_WHITE, TFT_BLACK);
        menuSprite.setTextDatum(MC_DATUM);
        menuSprite.drawString(time_str, menuSprite.width() / 2, 10);
        menuSprite.setTextDatum(TL_DATUM);
    }

    menuSprite.setTextDatum(MC_DATUM); // 设置文本基准为中中对齐

    // --- 绘制标题/当前状态 ---
    menuSprite.setTextFont(1);
    menuSprite.setTextColor(TFT_WHITE);
    const char *state_text = "";
    uint16_t arc_color = TFT_DARKGREY; // 进度条颜色
    switch (currentState)
    {
    case STATE_IDLE:        state_text = "Ready";       arc_color = TFT_SKYBLUE; break;
    case STATE_WORK:        state_text = "Work";         arc_color = TFT_ORANGE;  break;
    case STATE_SHORT_BREAK: state_text = "Short Break";  arc_color = TFT_GREEN;   break;
    case STATE_LONG_BREAK:  state_text = "Long Break";   arc_color = TFT_CYAN;    break;
    case STATE_PAUSED:      state_text = "Paused";       arc_color = TFT_YELLOW;  break;
    }
    menuSprite.setTextSize(2);
    menuSprite.drawString(state_text, 120, 180);
    menuSprite.setTextSize(1);

    // --- 绘制圆形进度条 ---
    float progress = (total_secs > 0) ? (float) remaining_secs / total_secs : 0;
    int angle = 360 * progress;
    // 绘制背景圆环
    menuSprite.drawSmoothArc(120, 120, 100, 80, 0, 360, TFT_DARKGREY, TFT_BLACK, true);
    // 绘制表示进度的前景圆弧
    menuSprite.drawSmoothArc(120, 120, 100 - 5, 80 + 5, 0, angle, arc_color, TFT_BLACK, true);

    // --- 绘制中央的剩余时间 ---
    menuSprite.setTextFont(7);
    menuSprite.setTextSize(1);
    menuSprite.setTextColor(TFT_WHITE);
    char time_buf[6];
    sprintf(time_buf, "%02lu:%02lu", remaining_secs / 60, remaining_secs % 60);
    menuSprite.drawString(time_buf, 120, 115);

    // --- 绘制已完成会话的标记 ---
    int marker_y = 230;
    int marker_total_width = SESSIONS_BEFORE_LONG_BREAK * 20;
    int marker_start_x = 120 - (marker_total_width / 2);
    for (int i = 0; i < SESSIONS_BEFORE_LONG_BREAK; i++)
    {
        if (i < sessions_completed)
        {
            menuSprite.fillCircle(marker_start_x + (i * 20), marker_y, 6, TFT_GREEN); // 已完成为绿色实心圆
        }
        else
        {
            menuSprite.drawCircle(marker_start_x + (i * 20), marker_y, 6, TFT_DARKGREY); // 未完成为空心圆
        }
    }

    menuSprite.pushSprite(0, 0); // 将Sprite内容推送到屏幕
}

// =====================================================================================
//                                     核心逻辑
// =====================================================================================

/**
 * @brief 开始一个新的会话（工作或休息）
 * @param nextState 要进入的下一个状态
 */
static void startNewSession(PomodoroState nextState)
{
    currentState = nextState;
    unsigned long duration_secs = 0;

    switch (currentState)
    {
    case STATE_WORK:        duration_secs = WORK_DURATION_SECS; break;
    case STATE_SHORT_BREAK: duration_secs = SHORT_BREAK_DURATION_SECS; break;
    case STATE_LONG_BREAK:
        duration_secs = LONG_BREAK_DURATION_SECS;
        sessions_completed = 0; // 长休息后重置计数
        break;
    default: break;
    }

    session_end_millis = millis() + (duration_secs * 1000); // 计算结束时间戳
    drawPomodoroUI(duration_secs, duration_secs);
    last_pomodoro_beep_time = 0; // 重置蜂鸣计时器
}

/**
 * @brief 番茄钟功能的主菜单函数
 */
void PomodoroMenu()
{
    currentState = STATE_IDLE;
    sessions_completed = 0;
    drawPomodoroUI(WORK_DURATION_SECS, WORK_DURATION_SECS); // 初始绘制IDLE界面
    unsigned long last_draw_time = 0;
    unsigned long last_realtime_clock_update = millis();

    while (true)
    {
        // 检查全局退出条件
        if (g_alarm_is_ringing) { return; }
        if (readButtonLongPress())
        {
            tone(BUZZER_PIN, 1500, 100);
            menuSprite.setTextFont(1); menuSprite.setTextSize(1); // 恢复默认字体
            return; // 退出菜单
        }

        if (readButton())
        { // 短按按钮逻辑
            tone(BUZZER_PIN, 2000, 50);
            if (currentState == STATE_IDLE)
            { // 如果是空闲状态，则开始工作
                startNewSession(STATE_WORK);
            }
            else if (currentState == STATE_PAUSED)
            { // 如果是暂停状态，则恢复
                currentState = stateBeforePause;
                session_end_millis = millis() + remaining_on_pause;
            }
            else
            { // 如果正在运行，则暂停
                remaining_on_pause = session_end_millis - millis();
                stateBeforePause = currentState;
                currentState = STATE_PAUSED;
                // 计算暂停时总时长用于UI显示
                unsigned long total_secs_on_pause = 0;
                switch (stateBeforePause)
                {
                case STATE_WORK: total_secs_on_pause = WORK_DURATION_SECS; break;
                case STATE_SHORT_BREAK: total_secs_on_pause = SHORT_BREAK_DURATION_SECS; break;
                case STATE_LONG_BREAK: total_secs_on_pause = LONG_BREAK_DURATION_SECS; break;
                default: break;
                }
                drawPomodoroUI(remaining_on_pause / 1000, total_secs_on_pause);
            }
        }

        // --- 计时器运行逻辑 ---
        if (currentState != STATE_IDLE && currentState != STATE_PAUSED)
        {
            unsigned long time_now = millis();
            unsigned long remaining_ms = (time_now >= session_end_millis) ? 0 : session_end_millis - time_now;

            // 每秒重绘一次UI以更新倒计时
            if (time_now - last_draw_time >= 990)
            {
                unsigned long total_secs = 0;
                switch (currentState)
                {
                case STATE_WORK: total_secs = WORK_DURATION_SECS; break;
                case STATE_SHORT_BREAK: total_secs = SHORT_BREAK_DURATION_SECS; break;
                case STATE_LONG_BREAK: total_secs = LONG_BREAK_DURATION_SECS; break;
                default: break;
                }
                drawPomodoroUI(remaining_ms / 1000, total_secs);
                last_draw_time = time_now;
            }

            // 最后5秒警告：每秒蜂鸣一次
            if (remaining_ms > 0 && remaining_ms <= 5000 && (time_now - last_pomodoro_beep_time >= 1000 || last_pomodoro_beep_time == 0))
            {
                tone(BUZZER_PIN, 1000, 100);
                last_pomodoro_beep_time = time_now;
            }

            // 当前会话结束
            if (remaining_ms == 0)
            {
                tone(BUZZER_PIN, 3000, 3000); // 长鸣提示结束
                if (currentState == STATE_WORK)
                {
                    sessions_completed++;
                    if (sessions_completed >= SESSIONS_BEFORE_LONG_BREAK)
                    {
                        startNewSession(STATE_LONG_BREAK); // 开始长休息
                    }
                    else
                    {
                        startNewSession(STATE_SHORT_BREAK); // 开始短休息
                    }
                }
                else
                { // 休息结束
                    startNewSession(STATE_WORK); // 开始新的工作
                }
            }
        }
        else
        { // 如果未在运行状态（空闲或暂停），则只更新顶部的实时时钟
            unsigned long current_millis = millis();
            if (current_millis - last_realtime_clock_update >= 1000)
            {
                unsigned long current_remaining_secs = 0;
                unsigned long current_total_secs = 0;

                if (currentState == STATE_IDLE)
                {
                    current_remaining_secs = WORK_DURATION_SECS;
                    current_total_secs = WORK_DURATION_SECS;
                }
                else if (currentState == STATE_PAUSED)
                {
                    current_remaining_secs = remaining_on_pause / 1000;
                    switch (stateBeforePause)
                    {
                    case STATE_WORK: current_total_secs = WORK_DURATION_SECS; break;
                    case STATE_SHORT_BREAK: current_total_secs = SHORT_BREAK_DURATION_SECS; break;
                    case STATE_LONG_BREAK: current_total_secs = LONG_BREAK_DURATION_SECS; break;
                    default: current_total_secs = WORK_DURATION_SECS; break; // Fallback
                    }
                }
                drawPomodoroUI((currentState == STATE_IDLE) ? WORK_DURATION_SECS : remaining_on_pause / 1000,
                    (currentState == STATE_IDLE) ? WORK_DURATION_SECS : ((stateBeforePause == STATE_WORK) ? WORK_DURATION_SECS : ((stateBeforePause == STATE_SHORT_BREAK) ? SHORT_BREAK_DURATION_SECS : LONG_BREAK_DURATION_SECS)));
                last_realtime_clock_update = current_millis;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(20)); // 短暂延时，避免CPU空转
    }
}
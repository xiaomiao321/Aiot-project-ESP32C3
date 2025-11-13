// 包含所有必需的头文件
#include "RotaryEncoder.h"
#include <Arduino.h>
#include "Alarm.h"
#include "img.h"
#include "LED.h"
#include "Buzzer.h"
#include "Pomodoro.h"
#include "weather.h"
#include "performance.h"
#include "DS18B20.h"
#include "animation.h"
#include "Games.h"
#include "ADC.h"
#include "Watchface.h"
#include "MQTT.h"
#include "MusicMenuLite.h"
#include "space_menu.h"
#include "Internet.h"
#include <TFT_eSPI.h>
#include "Countdown.h"
#include "Stopwatch.h"

// --- 布局配置 ---
// 调整这些值可以改变菜单布局
static const int ICON_SIZE = 200;     // 图标的尺寸 (例如, 200x200)
static const int ICON_SPACING = 220;  // 图标之间的水平间距 (应大于ICON_SIZE)
static const int SCREEN_WIDTH = 240;  // 屏幕宽度
static const int SCREEN_HEIGHT = 240; // 屏幕高度

// --- 根据上述配置计算出的布局值 ---
static const int ICON_Y_POS = (SCREEN_HEIGHT / 2) - (ICON_SIZE / 2); // 垂直居中图标
static const int INITIAL_X_OFFSET = (SCREEN_WIDTH / 2) - (ICON_SIZE / 2); // 水平居中第一个图标
static const int TRIANGLE_BASE_Y = ICON_Y_POS - 5; // 选择指示三角形的底部Y坐标
static const int TRIANGLE_PEAK_Y = TRIANGLE_BASE_Y - 20; // 三角形顶部的Y坐标

// --- 状态变量 ---
int16_t display = INITIAL_X_OFFSET; // 图标初始的x轴偏移量
uint8_t picture_flag = 0;           // 当前选中的菜单项索引

// 菜单项结构体在 "Menu.h" 中定义

// 菜单项数组
// const MenuItem menuItems[] = {
//     {"Clock", Weather, &weatherMenu},
//     {"Countdown", Countdown, &CountdownMenu},
//     {"Alarm", alarm_img, &AlarmMenu}, 
//     {"Pomodoro", tomato, &PomodoroMenu},
//     {"Stopwatch", Timer, &StopwatchMenu},
//     {"Music", Music, &BuzzerMenu},
//     {"Music Lite", music_lite, &MusicMenuLite},
//     {"Space",Space_img,&SpaceMenuScreen}, 
//     {"Internet", Internet, &InternetMenuScreen}, 
//     {"Performance", Performance, &performanceMenu},
//     {"Temperature",Temperature, &DS18B20Menu},
//     {"Animation",Animation, &AnimationMenu},
//     {"Games", Games, &GamesMenu},
//     {"LED", LED, &LEDMenu},
//     {"ADC", ADC, &ADCMenu},
// };
const MenuItem menuItems[] = {
    {"Clock", Weather, &weatherMenu},
    {"Music", Music, &BuzzerMenu},
    {"Internet", Internet, &InternetMenuScreen},
    {"Space", Space_img, &SpaceMenuScreen},
    {"Alarm", alarm_img, &AlarmMenu},
    {"Countdown", Timer, &CountdownMenu},
    {"Pomodoro", Timer, &PomodoroMenu},
    {"Stopwatch", Timer, &StopwatchMenu},
    {"Music Lite", Music, &MusicMenuLite},
    {"Performance", Performance, &performanceMenu},
    {"Temperature", Temperature, &DS18B20Menu},
    {"Animation", LED, &AnimationMenu},
    {"Games", Games, &GamesMenu},
    {"ADC", ADC, &ADCMenu},
    {"LED", LED, &LEDMenu},
};
const uint8_t MENU_ITEM_COUNT = sizeof(menuItems) / sizeof(menuItems[0]); // 菜单项总数

// 菜单状态枚举
enum MenuState
{
    MAIN_MENU, // 正在主菜单
    SUB_MENU,  // 在子菜单中
    ANIMATING  // 正在执行滚动动画
};

// 全局状态变量
static MenuState current_state = MAIN_MENU;
static const uint8_t ANIMATION_STEPS = 20; // 动画的步数

// --- 缓动函数 ---
// 用于实现平滑的动画效果

// 先快后慢
float easeOutQuad(float t)
{
    return 1.0f - (1.0f - t) * (1.0f - t);
}

// 带回弹效果的缓动
float easeOutBack(float t)
{
    const float c1 = 1.70158f;
    const float c3 = c1 + 1.0f;
    float t_minus_1 = t - 1.0f;
    return 1.0f + c3 * pow(t_minus_1, 3) + c1 * pow(t_minus_1, 2);
}

/**
 * @brief 绘制主菜单图标界面
 * @param offset 当前图标滚动的X轴偏移量
 */
void drawMenuIcons(int16_t offset)
{
    // 清理需要重绘的区域
    menuSprite.fillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, TFT_BLACK);

    // 绘制底部的选择三角形指示器
    int16_t triangle_x = offset + (picture_flag * ICON_SPACING) + (ICON_SIZE / 2);
    menuSprite.fillTriangle(triangle_x, SCREEN_HEIGHT - 25, triangle_x - 12, SCREEN_HEIGHT - 5, triangle_x + 12, SCREEN_HEIGHT - 5, TFT_WHITE);

    // 绘制可见范围内的图标
    for (int i = 0; i < MENU_ITEM_COUNT; i++)
    {
        int16_t x = offset + (i * ICON_SPACING);
        if (x >= -ICON_SIZE && x < SCREEN_WIDTH)
        {
            menuSprite.pushImage(x, ICON_Y_POS, ICON_SIZE, ICON_SIZE, menuItems[i].image);
        }
    }

    // 绘制顶部的菜单项名称
    menuSprite.setTextColor(TFT_WHITE, TFT_BLACK);
    menuSprite.setTextSize(2);
    menuSprite.setTextDatum(TC_DATUM); // 文本基准居中
    menuSprite.drawString(menuItems[picture_flag].name, SCREEN_WIDTH / 2, 10);

    menuSprite.pushSprite(0, 0); // 将Sprite内容推送到屏幕
}

/**
 * @brief 显示主菜单的配置（通常在从子菜单返回时调用）
 */
void showMenuConfig()
{
    tft.fillScreen(TFT_BLACK);
    drawMenuIcons(display);
}

/**
 * @brief 主菜单导航和逻辑处理
 */
void showMenu()
{
    menuSprite.setTextFont(1);
    menuSprite.setTextSize(1);

    // 如果闹钟正在响铃，则优先显示响铃界面
    if (g_alarm_is_ringing)
    {
        Alarm_ShowRingingScreen();
        showMenuConfig(); // 闹钟结束后重绘主菜单
        return;
    }

    // 如果当前在子菜单中，则不执行主菜单逻辑
    if (current_state != MAIN_MENU) return;

    int direction = readEncoder();
    if (direction != 0)
    { // 如果编码器被旋转
        current_state = ANIMATING; // 进入动画状态

        if (direction == 1)
        { // 向右
            picture_flag = (picture_flag + 1) % MENU_ITEM_COUNT;
        }
        else if (direction == -1)
        { // 向左
            picture_flag = (picture_flag == 0) ? MENU_ITEM_COUNT - 1 : picture_flag - 1;
        }

        int16_t start_display = display;
        int16_t target_display = INITIAL_X_OFFSET - (picture_flag * ICON_SPACING);

        // 执行带缓动效果的滚动动画
        for (uint8_t i = 0; i <= ANIMATION_STEPS; i++)
        {
            float t = (float) i / ANIMATION_STEPS;
            float eased_t = easeOutBack(t); // 应用回弹缓动效果
            display = start_display + (target_display - start_display) * eased_t;
            drawMenuIcons(display);
            vTaskDelay(pdMS_TO_TICKS(5));
        }

        display = target_display; // 确保动画结束在精确位置
        drawMenuIcons(display);
        current_state = MAIN_MENU; // 动画结束，返回主菜单状态
    }

    if (readButton())
    { // 如果按钮被按下
        vTaskDelay(pdMS_TO_TICKS(50)); // 短暂延时以播放声音或防抖

        if (menuItems[picture_flag].action)
        {
            exitSubMenu = false; // 在进入子菜单前重置退出标志
            menuItems[picture_flag].action(); // 执行选定菜单项对应的函数
            showMenuConfig(); // 从子菜单返回后，重绘主菜单
        }
    }
}

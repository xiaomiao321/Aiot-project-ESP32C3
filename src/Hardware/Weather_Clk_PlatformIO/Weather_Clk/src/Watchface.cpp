// 包含所有必需的头文件
#include <TFT_eSPI.h>      // TFT屏幕驱动库
#include <cmath>           // C++数学库，用于sin, cos等函数
#include "Buzzer.h"        // 蜂鸣器相关功能
#include "Alarm.h"         // 闹钟相关功能
#include <vector>          // C++标准库，用于动态数组
#include "Watchface.h"     // 本文件的头文件
#include "MQTT.h"          // MQTT通信功能
#include "RotaryEncoder.h" // 旋转编码器输入处理
#include "weather.h"       // 天气和时间同步功能
#include "img.h"           // 图像资源
#include <time.h>          // C标准时间库，用于tm结构体
#include "DS18B20.h"       // DS18B20温度传感器
#include "TargetSettings.h"// 目标设置菜单

// --- 宏定义 ---
#define MENU_FONT 1 // 菜单使用的默认字体
#define WEATHER_INTERVAL_MIN 5 // 天气数据自动更新间隔（分钟）
#define TIME_INTERVAL_MIN 60   // 时间自动同步间隔（分钟）

// --- 时间和日期颜色定义 ---
#define TIME_MAIN_COLOR     TFT_CYAN   // 时间（时分秒）主颜色
#define TIME_TENTH_COLOR    TFT_WHITE  // 时间（毫秒/十分之一秒）颜色
#define DATE_WEEKDAY_COLOR  TFT_YELLOW // 日期（星期）颜色
#define DATE_DATE_COLOR     TFT_YELLOW // 日期（年月日）颜色

// --- 通用UI元素颜色定义 ---
#define WEATHER_DATA_COLOR          TFT_CYAN   // 天气数据（温湿度）颜色
#define WEATHER_REPORT_TIME_COLOR   TFT_CYAN   // 天气数据上报时间颜色
#define LAST_SYNC_TIME_COLOR        TFT_CYAN   // 最后NTP时间同步状态颜色
#define LAST_SYNC_WEATHER_COLOR     TFT_YELLOW // 最后天气同步状态颜色
#define DS18B20_TEMP_COLOR          TFT_ORANGE // DS18B20温度显示颜色
#define WIFI_CONNECTED_COLOR        TFT_GREEN  // WiFi连接状态颜色
#define WIFI_DISCONNECTED_COLOR     TFT_RED    // WiFi断开状态颜色

// =================================================================================================
// 全局变量与函数前向声明
// =================================================================================================
extern TFT_eSPI tft;           // 外部引用的TFT屏幕对象
extern TFT_eSprite menuSprite; // 外部引用的屏幕缓冲区(Sprite)
extern void showMenuConfig();  // 外部函数，显示主配置菜单
extern char wifiStatusStr[];   // 外部字符数组，用于显示WiFi状态

// --- 所有表盘函数的静态前向声明 ---
static void SimpleClockWatchface();
static void VectorScanWatchface();
static void VectorScrollWatchface();
static void TerminalSimWatchface();
static void CodeRainWatchface();
static void SnowWatchface();
static void WavesWatchface();
static void NenoWatchface();
static void BallsWatchface();
static void SandBoxWatchface();
static void ProgressBarWatchface();
static void ChargeWatchface();
static void Cube3DWatchface();
static void GalaxyWatchface();
static void SimClockWatchface();
static void PlaceholderWatchface();
static void VectorScrollWatchface_SEG();
static void VectorScanWatchface_SEG();

/**
 * @brief 定义表盘菜单项的结构体
 */
struct WatchfaceItem
{
    const char *name; // 表盘名称
    void (*show)();   // 指向表盘显示函数的指针
};

/**
 * @brief 表盘菜单项数组
 * @details 每个元素包含表盘的名称和对应的启动函数。
 */
const WatchfaceItem watchfaceItems[] = {
    {"Target Settings", TargetSettings_Menu},
    // {"Charge", ChargeWatchface},
    {"Scan", VectorScanWatchface},
    {"Scan_SEG",VectorScanWatchface_SEG},
    {"Scroll", VectorScrollWatchface},
    {"Scroll_SEG",VectorScrollWatchface_SEG},
    {"Progress Bar", ProgressBarWatchface},
    {"Sim Clock", SimClockWatchface},
    {"Galaxy", GalaxyWatchface},
    {"Terminal Sim", TerminalSimWatchface},
    {"Simple Clock", SimpleClockWatchface},
    {"Code Rain", CodeRainWatchface},
    {"Snow", SnowWatchface},
    {"Waves", WavesWatchface},
    {"Neno", NenoWatchface},
    {"Bouncing Balls", BallsWatchface},
    {"Sand Box", SandBoxWatchface},
    {"3D Cube", Cube3DWatchface},
};
// 计算表盘总数
const int WATCHFACE_COUNT = sizeof(watchfaceItems) / sizeof(watchfaceItems[0]);

// --- 表盘菜单分页 ---
const int VISIBLE_WATCHFACES = 5; // 菜单每页可见的表盘数量

/**
 * @brief 绘制表盘选择列表
 * @param selectedIndex 当前选中的项目索引，用于高亮显示
 * @param displayOffset 列表的滚动偏移量，用于实现分页
 */
static void displayWatchfaceList(int selectedIndex, int displayOffset)
{
    menuSprite.fillSprite(TFT_BLACK); // 清空缓冲区
    menuSprite.setTextFont(MENU_FONT);
    menuSprite.setTextDatum(MC_DATUM); // 设置文本对齐方式为居中
    menuSprite.setTextColor(TFT_WHITE, TFT_BLACK);
    menuSprite.setTextSize(2);
    menuSprite.drawString("Select Watchface", tft.width() / 2, 20); // 绘制标题

    // 循环绘制可见的菜单项
    for (int i = 0; i < VISIBLE_WATCHFACES; i++)
    {
        int itemIndex = displayOffset + i;
        if (itemIndex >= WATCHFACE_COUNT) break; // 如果超出范围则停止

        // 根据是否被选中，设置不同的大小和颜色
        menuSprite.setTextSize(itemIndex == selectedIndex ? 2 : 1);
        menuSprite.setTextColor(itemIndex == selectedIndex ? TFT_YELLOW : TFT_WHITE, TFT_BLACK);
        menuSprite.drawString(watchfaceItems[itemIndex].name, tft.width() / 2, 60 + i * 30);
    }
    menuSprite.pushSprite(0, 0); // 将缓冲区内容推送到屏幕
}

/**
 * @brief 表盘选择的主菜单函数
 * @details 处理用户输入（旋转编码器、单击、长按），并导航到所选的表盘。
 */
void WatchfaceMenu()
{
    int selectedIndex = 0;     // 当前选中的项目索引
    int displayOffset = 0;     // 列表的滚动偏移量
    unsigned long lastClickTime = 0; // 用于检测双击
    bool singleClick = false;  // 单击事件标志

    displayWatchfaceList(selectedIndex, displayOffset); // 首次进入时绘制列表

    while (1)
    {
        // 检查全局退出标志
        if (exitSubMenu)
        {
            exitSubMenu = false; // 重置标志
            return;
        }
        // 如果闹钟响起，则退出
        if (g_alarm_is_ringing) { return; }

        // 长按退出菜单
        if (readButtonLongPress())
        {
            tone(BUZZER_PIN, 1500, 100);
            return;
        }

        // 读取编码器变化以滚动菜单
        int encoderChange = readEncoder();
        if (encoderChange != 0)
        {
            // 更新选中索引，并处理循环
            selectedIndex = (selectedIndex + encoderChange + WATCHFACE_COUNT) % WATCHFACE_COUNT;

            // 根据需要调整显示偏移，实现滚动效果
            if (selectedIndex < displayOffset)
            {
                displayOffset = selectedIndex;
            }
            else if (selectedIndex >= displayOffset + VISIBLE_WATCHFACES)
            {
                displayOffset = selectedIndex - VISIBLE_WATCHFACES + 1;
            }
            displayWatchfaceList(selectedIndex, displayOffset); // 重绘列表
            tone(BUZZER_PIN, 1000, 50); // 播放提示音
        }

        // 读取按钮点击
        if (readButton())
        {
            // 简单的双击检测：短时间内两次点击则进入配置菜单
            if (millis() - lastClickTime < 300) { showMenuConfig(); return; }
            lastClickTime = millis();
            singleClick = true;
        }

        // 处理单击事件（在双击检测窗口超时后）
        if (singleClick && (millis() - lastClickTime > 300))
        {
            singleClick = false;
            tone(BUZZER_PIN, 2000, 50);
            watchfaceItems[selectedIndex].show(); // 调用选中的表盘函数
            displayWatchfaceList(selectedIndex, displayOffset); // 从表盘返回后，重绘菜单
        }
        vTaskDelay(pdMS_TO_TICKS(10)); // 短暂延时，让出CPU
    }
}

// =================================================================================================
// API 适配器层与辅助函数
// =================================================================================================

extern struct tm timeinfo; // 外部引用的标准时间结构体
typedef uint8_t byte;      // 定义 byte 类型为 uint8_t
typedef uint32_t millis_t; // 定义 millis_t 类型为 uint32_t
/**
 * @brief 定义一个简化的时间结构体
 */
typedef struct { byte hour, mins, secs; char ampm; } time_s;
/**
 * @brief 定义一个包含时间和日期的完整结构体
 */
typedef struct { time_s time; byte day, month, year; } timeDate_s;
timeDate_s g_watchface_timeDate; // 全局的表盘时间日期对象
timeDate_s *timeDate = &g_watchface_timeDate; // 指向全局对象的指针，用于兼容旧代码

// --- 伪随机数生成器 ---
static uint32_t util_seed = 1; // 随机数种子
/**
 * @brief 设置随机数种子
 */
void util_randomSeed(uint32_t newSeed) { util_seed = newSeed; }
/**
 * @brief 生成一个最大值范围内的随机数
 * @param max 随机数的最大值（不包含）
 * @return long 类型的随机数
 */
long util_random(long max)
{
    static bool seeded = false;
    if (!seeded)
    {
        // 组合多种时间源作为种子，以增加随机性
        unsigned long seed = millis() + micros();
        randomSeed(seed);
        seeded = true;
    }
    return random(max);
}
/**
 * @brief 生成一个指定范围内的随机数
 * @param min 随机数的最小值
 * @param max 随机数的最大值（不包含）
 * @return long 类型的随机数
 */
long util_random_range(long min, long max) { return min + util_random(max - min); }

/**
 * @brief 在Sprite上绘制单个字符
 * @param x 字符的X坐标
 * @param y 字符的Y坐标
 * @param c 要绘制的字符
 * @param color 字符颜色
 * @param bg 背景颜色
 * @param size 字符大小
 */
void draw_char(int16_t x, int16_t y, char c, uint16_t color, uint16_t bg, uint8_t size)
{
    menuSprite.setTextSize(size);
    menuSprite.setTextColor(color, bg);
    char str[2] = { c, '\0' }; // 创建一个包含单个字符的字符串
    menuSprite.drawString(str, x, y);
}

// =================================================================================================
// 整点报时逻辑
// =================================================================================================
static int g_lastChimeSecond = -1; // 记录上一次报时的秒数，防止重复触发
static TaskHandle_t g_hourlyMusicTaskHandle = NULL; // 整点音乐播放任务的句柄
static int g_hourlySongIndex = 0; // 要播放的整点音乐索引

/**
 * @brief 处理整点报时和倒计时逻辑
 * @details 此函数应在每个表盘的主循环中被频繁调用。
 *          它包含一个内部计时器，确保逻辑每秒只执行一次。
 *          - 在每小时的59分55秒到59秒，每秒蜂鸣一次作为倒计时。
 *          - 在整点（0分0秒）时，播放一声长音，并延迟后播放一首随机音乐。
 */
static void handleHourlyChime()
{
    static unsigned long lastChimeCheck = 0; // 上次检查时间
    static unsigned long lastBeepTime = 0;   // 上次蜂鸣时间
    static bool waitingForMusic = false;     // 是否正在等待播放音乐的标志

    // 使用millis()进行节流，确保此函数的核心逻辑大约每秒执行一次
    if (millis() - lastChimeCheck < 1000)
    {
        return;
    }
    lastChimeCheck = millis();

    getLocalTime(&timeinfo); // 获取当前时间

    // 检查音乐任务是否已自行结束
    if (g_hourlyMusicTaskHandle != NULL && eTaskGetState(g_hourlyMusicTaskHandle) == eDeleted)
    {
        g_hourlyMusicTaskHandle = NULL;
    }

    // 如果当前处于“等待播放音乐”状态
    if (waitingForMusic)
    {
        // 在整点长音后等待3秒
        if (millis() - lastBeepTime >= 3000)
        {
            // 如果已有音乐在播放，先停止它
            if (g_hourlyMusicTaskHandle != NULL)
            {
                vTaskDelete(g_hourlyMusicTaskHandle);
            }

            // 根据当前小时数选择一首歌进行播放
            g_hourlySongIndex = timeinfo.tm_hour % numSongs;
            stopBuzzerTask = false; // 重置停止标志
            // 创建一个新的后台任务来播放音乐
            xTaskCreatePinnedToCore(Buzzer_PlayMusic_Task, "Buzzer_Music_Task", 8192, &g_hourlySongIndex, 1, &g_hourlyMusicTaskHandle, 0);

            waitingForMusic = false; // 重置状态
        }
        g_lastChimeSecond = timeinfo.tm_sec;
        return; // 在等待期间不处理其他逻辑
    }

    // 在每小时的最后5秒进行倒计时蜂鸣
    if (timeinfo.tm_min == 59 && timeinfo.tm_sec >= 55)
    {
        if (g_lastChimeSecond != timeinfo.tm_sec) // 每秒只响一次
        {
            int freq = 1000;
            tone(BUZZER_PIN, freq, 100);
            g_lastChimeSecond = timeinfo.tm_sec;
        }
    }
    // 在整点0分0秒时，触发长音并进入等待播放音乐的状态
    else if (timeinfo.tm_min == 0 && timeinfo.tm_sec == 0)
    {
        tone(BUZZER_PIN, 3000, 1000); // 播放一声长音
        waitingForMusic = true;      // 设置等待标志
        lastBeepTime = millis();     // 记录当前时间
        g_lastChimeSecond = timeinfo.tm_sec;
    }
    else
    {
        // 其他时间，重置状态
        g_lastChimeSecond = -1;
        waitingForMusic = false;
    }
}
// =================================================================================================
// 通用表盘逻辑
// =================================================================================================

static unsigned long lastSyncMillis_Weather = 0; // 上次天气同步的毫秒数
static unsigned long lastSyncMillis_Time = 0;    // 上次NTP时间同步的毫秒数
const unsigned long syncInterval_Weather = WEATHER_INTERVAL_MIN * 60 * 1000; // 天气同步间隔
const unsigned long syncInterval_Time = TIME_INTERVAL_MIN * 60 * 1000;       // 时间同步间隔

/**
 * @brief 处理周期性的后台数据同步（天气和时间）
 * @details 此函数应在每个表盘的主循环中被调用。
 *          它会检查自上次同步以来是否已超过预设的间隔，如果超过，则会
 *          调用相应的静默同步函数。
 */
static void handlePeriodicSync()
{
    if (millis() - lastSyncMillis_Weather > syncInterval_Weather)
    {
        silentFetchWeather(); // 静默获取天气
        lastSyncMillis_Weather = millis();
    }
    if (millis() - lastSyncMillis_Time > syncInterval_Time)
    {
        silentSyncTime(); // 静默同步NTP时间
        lastSyncMillis_Time = millis();
    }
}

/**
 * @brief 绘制所有表盘共有的UI元素
 * @details 包括天气信息、日期、星期、同步状态、传感器温度和WiFi状态。
 *          这些元素通常位于屏幕的顶部和底部，为中间的核心表盘内容留出空间。
 */
static void drawCommonElements()
{
    menuSprite.setTextFont(1); // 设置为1号字体

    // --- 右上角：天气信息 ---
    menuSprite.setTextDatum(TR_DATUM); // 设置文本对齐方式为右上角
    menuSprite.setTextSize(2);
    menuSprite.setTextColor(WEATHER_DATA_COLOR, TFT_BLACK);
    String weatherStr = String(temperature) + " " + String(humidity);
    menuSprite.drawString(weatherStr, tft.width() - 15, 5);

    // --- 左上角：天气数据上报时间 ---
    char reportTimeOnlyStr[9]; // 缓冲区用于存放 HH:MM:SS
    int len = strlen(reporttime);
    if (len >= 8)
    {
        // 从 "YYYY-MM-DD HH:MM:SS" 格式的字符串中提取最后8个字符
        strncpy(reportTimeOnlyStr, reporttime + len - 8, 8);
        reportTimeOnlyStr[8] = '\0';
    }
    else
    {
        strcpy(reportTimeOnlyStr, "N/A"); // 如果格式不符，则显示N/A
    }
    menuSprite.setTextSize(2);
    menuSprite.setTextDatum(TL_DATUM); // 设置文本对齐方式为左上角
    menuSprite.setTextColor(WEATHER_REPORT_TIME_COLOR, TFT_BLACK);
    menuSprite.drawString(reportTimeOnlyStr, 0, 5);

    // --- 中部：日期和星期 ---
    menuSprite.setTextDatum(MC_DATUM); // 设置文本对齐方式为居中
    const char *weekDayStr[] = { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };

    // 第一行：星期
    menuSprite.setTextSize(3);
    menuSprite.setTextColor(DATE_WEEKDAY_COLOR, TFT_BLACK);
    menuSprite.drawString(weekDayStr[timeinfo.tm_wday], tft.width() / 2, 45);

    // 第二行：年月日
    char dateStr[20];
    sprintf(dateStr, "%d-%02d-%02d", timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday);
    menuSprite.setTextSize(3);
    menuSprite.setTextColor(DATE_DATE_COLOR, TFT_BLACK);
    menuSprite.drawString(dateStr, tft.width() / 2, 75);

    // --- 底部中央：最后同步时间状态 ---
    menuSprite.setTextDatum(BC_DATUM); // 设置文本对齐方式为底部居中
    menuSprite.setTextSize(1);
    menuSprite.setTextColor(LAST_SYNC_TIME_COLOR, TFT_BLACK);
    menuSprite.drawString(lastSyncTimeStr, 120, tft.height() - 5);

    // --- 底部中央：最后同步天气状态 ---
    menuSprite.setTextDatum(BC_DATUM);
    menuSprite.setTextSize(1);
    menuSprite.setTextColor(LAST_SYNC_WEATHER_COLOR, TFT_BLACK);
    menuSprite.drawString(lastWeatherSyncStr, 120, tft.height() - 15);

    // --- 底部中央：DS18B20温度 ---
    menuSprite.setTextDatum(BC_DATUM);
    menuSprite.setTextFont(1);
    menuSprite.setTextSize(1);
    float temp = getDS18B20Temp();
    String tempStr = "DS18B20: " + String(temp, 1) + " C";
    menuSprite.setTextColor(DS18B20_TEMP_COLOR, TFT_BLACK);
    menuSprite.drawString(tempStr, 120, tft.height() - 35);

    // --- 底部中央：WiFi状态 ---
    menuSprite.setTextDatum(BC_DATUM);
    menuSprite.setTextSize(1);
    if (ensureWiFiConnected()) // 检查WiFi状态并设置相应颜色
    {
        menuSprite.setTextColor(WIFI_CONNECTED_COLOR, TFT_BLACK);
    }
    else
    {
        menuSprite.setTextColor(WIFI_DISCONNECTED_COLOR, TFT_BLACK);
    }
    menuSprite.drawString(wifiStatusStr, 120, tft.height() - 25);
    menuSprite.setTextColor(TIME_MAIN_COLOR, TFT_BLACK); // 恢复默认颜色
}

/**
 * @brief 绘制包含目标设定元素的通用UI组件
 * @details 在通用元素的基础上，额外绘制来自TargetSettings模块的UI。
 */
static void drawAdvancedCommonElements()
{
    drawCommonElements();
    drawTargetElements(&menuSprite);
}

// =================================================================================================
// WATCHFACE IMPLEMENTATIONS
// =================================================================================================


// The map function, as seen in charge/main.c
static long mymap(long x, long in_min, long in_max, long out_min, long out_max)
{
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

// Helper to draw a bitmap with a transparent background
static void drawXBitmap_transparent(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, uint16_t color)
{
    int16_t i, j, byteWidth = (w + 7) / 8;
    uint8_t b;

    for (j = 0; j < h; j++)
    {
        for (i = 0; i < w; i++)
        {
            if (i & 7) b <<= 1;
            else      b = bitmap[j * byteWidth + i / 8];
            if (b & 0x80) menuSprite.drawPixel(x + i, y + j, color);
        }
    }
}

// Data from charge/main.c
static const unsigned char battery_outline[] = {
    0xfc, 0xfe, 0x7, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x7, 0xfe, 0xfc, 0x0, 0x0, 0x0,
        0xff, 0xff, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xff, 0xff, 0xf0, 0xf0, 0xe0,
        0xff, 0xff, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xff, 0xff, 0xf, 0xf, 0x7,
        0x3f, 0x7f, 0xe0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xe0, 0x7f, 0x3f, 0x0, 0x0, 0x0,
};

static const unsigned char water_array[30][128] = {
        {0xfe, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,0xff, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,0xff, 0xfe, 0xfc, 0xf8, 0xf0, 0xf0, 0xe0, 0xc0, 0xc0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xc, 0x1e, 0x1e, 0x1e, 0x1c, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,0xff, 0xff, 0xff, 0xff, 0x7f, 0x7f, 0x3f, 0x1f, 0x1f, 0x7, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x3, 0x7, 0x2, 0x0, 0x0, 0x0, 0x0, 0x0,},
        {0xfe, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x10, 0x38, 0x10, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,0xff, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,0xff, 0xff, 0xfe, 0xfc, 0xf8, 0xf0, 0xe0, 0xc0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1e, 0x1e, 0x1f, 0x3e, 0x1c, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,0xff, 0xff, 0xff, 0xff, 0x7f, 0x3f, 0x3f, 0xf, 0x3, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x3, 0x7, 0x2, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,},
        // ... (rest of the water_array data) ...
};

/**
 * @brief 充电表盘：模拟电池充电动画，并显示当前时间。
 * @details 此表盘显示一个电池轮廓，内部填充的电量会根据秒数变化，模拟充电过程。
 *          同时显示当前时间，并集成通用UI元素（天气、日期等）。
 */
static void ChargeWatchface()
{
    static int anim_frame = 0; // 动画帧计数器，用于控制水波动画
    int xoff = (tft.width() - 70) / 2; // 电池轮廓的X轴偏移量，使其居中
    int yoff = 150;                    // 电池轮廓的Y轴偏移量

    while (1)
    {
        // 检查退出子菜单标志，如果为真则退出当前表盘
        if (exitSubMenu)
        {
            exitSubMenu = false; // 重置标志
            return;              // 退出表盘函数
        }
        // 如果闹钟正在响铃，则退出表盘，让闹钟优先处理
        if (g_alarm_is_ringing) { return; }
        // 处理周期性数据同步（天气和NTP时间）
        handlePeriodicSync();
        // 处理整点报时逻辑
        handleHourlyChime();

        // 检测按钮点击，如果点击则退出当前表盘
        if (readButton())
        {
            tone(BUZZER_PIN, 1500, 50); // 播放按键音
            menuSprite.setTextFont(MENU_FONT); // 恢复菜单字体设置
            return; // 退出表盘函数
        }

        getLocalTime(&timeinfo); // 获取当前本地时间
        menuSprite.fillSprite(TFT_BLACK); // 清空屏幕缓冲区，填充黑色背景
        drawAdvancedCommonElements();     // 绘制高级通用UI元素（天气、日期、DS18B20温度、WiFi状态等）

        menuSprite.setTextDatum(MC_DATUM); // 设置文本对齐方式为居中
        char timeStr[10];                  // 时间字符串缓冲区
        // 格式化时间为 HH:MM:SS
        sprintf(timeStr, "%02d:%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
        menuSprite.setTextSize(5);         // 设置时间字体大小
        menuSprite.setTextColor(TIME_MAIN_COLOR, TFT_BLACK); // 设置时间颜色
        menuSprite.drawString(timeStr, tft.width() / 2, 115); // 在屏幕中央绘制时间

        // 绘制电池轮廓
        menuSprite.drawXBitmap(xoff, yoff, battery_outline, 70, 40, TFT_WHITE, TFT_BLACK);

        // 根据当前秒数计算电池填充的宽度，模拟充电进度
        // 将秒数从0-59映射到0-64像素宽度
        long fill_width = mymap(timeinfo.tm_sec, 0, 59, 0, 64);

        // 如果填充宽度大于0，则绘制电池内部的填充条
        if (fill_width > 0)
        {
            menuSprite.fillRect(xoff + 3, yoff + 3, fill_width, 34, TIME_MAIN_COLOR); // 绘制填充矩形
        }

        // 计算水波动画的绘制位置
        int water_x = xoff + 3 + fill_width - 16; // 水波X坐标，跟随填充进度
        int water_y = yoff + 4;                   // 水波Y坐标
        // 绘制水波动画，使用透明背景
        drawXBitmap_transparent(water_x, water_y, water_array[anim_frame], 32, 32, TFT_CYAN);

        anim_frame = (anim_frame + 1) % 30; // 更新动画帧，循环播放水波动画

        menuSprite.pushSprite(0, 0); // 将屏幕缓冲区内容推送到TFT屏幕显示
        vTaskDelay(pdMS_TO_TICKS(50)); // 短暂延时，控制动画速度和CPU占用
    }
}




// --- 3D Cube ---
#define CUBE_SIZE 25 // 定义3D立方体的大小（半边长）

// 立方体的初始顶点坐标，每个顶点由(x, y, z)三维坐标表示
static const int16_t cube_vertices_start[] = {
    -CUBE_SIZE, -CUBE_SIZE, -CUBE_SIZE, // 顶点0
    CUBE_SIZE, -CUBE_SIZE, -CUBE_SIZE,  // 顶点1
    CUBE_SIZE, CUBE_SIZE, -CUBE_SIZE,   // 顶点2
    -CUBE_SIZE, CUBE_SIZE, -CUBE_SIZE,  // 顶点3
    -CUBE_SIZE, -CUBE_SIZE, CUBE_SIZE,  // 顶点4
    CUBE_SIZE, -CUBE_SIZE, CUBE_SIZE,   // 顶点5
    CUBE_SIZE, CUBE_SIZE, CUBE_SIZE,    // 顶点6
    -CUBE_SIZE, CUBE_SIZE, CUBE_SIZE    // 顶点7
};

// 立方体的边索引，每对数字表示一条边连接的两个顶点
static const uint8_t cube_indices[] = {
    0, 1, 1, 2, 2, 3, 3, 0, // 前面
    4, 5, 5, 6, 6, 7, 7, 4, // 后面
    0, 4, 1, 5, 2, 6, 3, 7  // 连接前后面的边
};

/**
 * @brief 3D立方体表盘：显示一个旋转的3D线框立方体和当前时间。
 * @details 立方体在X、Y、Z轴上以不同速度旋转，模拟3D效果。
 *          用户可以通过旋转编码器调整立方体的旋转速度。
 */
static void Cube3DWatchface()
{
    static float rot = 0;      // 立方体当前旋转角度
    static float rotInc = 1;   // 立方体旋转增量（速度）
    int16_t cube_vertices[8 * 3]; // 用于存储旋转和平移后的顶点坐标

    while (1)
    {
        // 检查退出子菜单标志，如果为真则退出当前表盘
        if (exitSubMenu)
        {
            exitSubMenu = false; // 重置标志
            // 如果有整点报时音乐正在播放，则停止它
            if (g_hourlyMusicTaskHandle != NULL)
            {
                vTaskDelete(g_hourlyMusicTaskHandle); // 删除音乐任务
                g_hourlyMusicTaskHandle = NULL;       // 重置任务句柄
                noTone(BUZZER_PIN);                   // 停止蜂鸣器发声
                stopBuzzerTask = true;                // 设置停止蜂鸣器任务标志
            }
            return; // 退出表盘函数
        }
        // 如果闹钟正在响铃，则退出表盘，让闹钟优先处理
        if (g_alarm_is_ringing) { return; }
        // 处理周期性数据同步（天气和NTP时间）
        handlePeriodicSync();
        // 处理整点报时逻辑
        handleHourlyChime();

        // 读取编码器变化，调整旋转速度
        int encoderChange = readEncoder();
        if (encoderChange != 0)
        {
            rotInc += encoderChange; // 增加或减少旋转速度
            if (rotInc > 10) rotInc = 10; // 限制最大速度
            if (rotInc < 1) rotInc = 1;   // 限制最小速度
            tone(BUZZER_PIN, 1000, 50);   // 播放按键音
        }

        // 检测按钮点击，如果点击则退出当前表盘
        if (readButton())
        {
            // 如果有整点报时音乐正在播放，则停止它
            if (g_hourlyMusicTaskHandle != NULL)
            {
                vTaskDelete(g_hourlyMusicTaskHandle);
                g_hourlyMusicTaskHandle = NULL;
                noTone(BUZZER_PIN);
                stopBuzzerTask = true;
            }
            tone(BUZZER_PIN, 1500, 50); // 播放按键音
            menuSprite.setTextFont(MENU_FONT); // 恢复菜单字体设置
            return; // 退出表盘函数
        }

        getLocalTime(&timeinfo); // 获取当前本地时间
        menuSprite.fillSprite(TFT_BLACK); // 清空屏幕缓冲区，填充黑色背景

        // 复制初始顶点数据到可变数组
        memcpy(cube_vertices, cube_vertices_start, sizeof(cube_vertices_start));

        rot += rotInc / 10.0; // 更新旋转角度
        if (rot >= 360) rot = 0; // 角度归零
        else if (rot < 0) rot = 359; // 角度调整

        // 将角度转换为弧度
        float rot_rad = rot * M_PI / 180.0;
        float rot_rad2 = (rot * 1.5) * M_PI / 180.0; // 调整不同轴的旋转速度
        float rot_rad3 = (rot * 2.0) * M_PI / 180.0;

        // 计算旋转所需的正弦和余弦值
        float sin_rot = sin(rot_rad);
        float cos_rot = cos(rot_rad);
        float sin_rot2 = sin(rot_rad2);
        float cos_rot2 = cos(rot_rad2);
        float sin_rot3 = sin(rot_rad3);
        float cos_rot3 = cos(rot_rad3);

        // 对每个顶点进行3D旋转和透视投影
        for (int i = 0; i < 8; ++i)
        {
            int16_t *vertex = &cube_vertices[i * 3]; // 获取当前顶点的指针
            int16_t x = vertex[0], y = vertex[1], z = vertex[2]; // 原始坐标

            // 绕Z轴旋转
            int16_t x_new = x * cos_rot - y * sin_rot;
            int16_t y_new = y * cos_rot + x * sin_rot;
            x = x_new; y = y_new;

            // 绕Y轴旋转
            x_new = x * cos_rot2 - z * sin_rot2;
            z = z * cos_rot2 + x * sin_rot2;
            x = x_new;

            // 绕X轴旋转
            y_new = y * cos_rot3 - z * sin_rot3;
            z = z * cos_rot3 + y * sin_rot3;
            y = y_new;

            // 透视投影
            int16_t z_new = z + 80; // 调整Z轴，使立方体在屏幕前方
            x = (x * 128) / z_new;  // 投影到2D屏幕
            y = (y * 128) / z_new;

            // 将投影后的坐标平移到屏幕中心
            x += tft.width() / 2;
            y += tft.height() / 2;

            vertex[0] = x; // 更新顶点X坐标
            vertex[1] = y; // 更新顶点Y坐标
        }

        // 绘制立方体的所有边
        for (int i = 0; i < 12; ++i)
        {
            const uint8_t *indices = &cube_indices[i * 2]; // 获取当前边的两个顶点索引
            int16_t *p1 = &cube_vertices[indices[0] * 3];  // 第一个顶点坐标
            int16_t *p2 = &cube_vertices[indices[1] * 3];  // 第二个顶点坐标
            menuSprite.drawLine(p1[0], p1[1], p2[0], p2[1], TIME_TENTH_COLOR); // 绘制线段
        }

        char timeStr[20]; // 时间字符串缓冲区
        int tenth = (millis() % 1000) / 100; // 获取十分之一秒
        // 格式化时间为 HH:MM:SS.D
        sprintf(timeStr, "%02d:%02d:%02d.%d", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec, tenth);
        menuSprite.setTextFont(1);         // 设置字体
        menuSprite.setTextDatum(TC_DATUM); // 设置文本对齐方式为顶部居中
        menuSprite.setTextSize(4);         // 设置字体大小
        menuSprite.setTextColor(TIME_MAIN_COLOR, TFT_BLACK); // 设置时间颜色
        menuSprite.drawString(timeStr, tft.width() / 2, 15); // 在屏幕顶部居中绘制时间

        menuSprite.pushSprite(0, 0); // 将屏幕缓冲区内容推送到TFT屏幕显示
        vTaskDelay(pdMS_TO_TICKS(20)); // 短暂延时，控制动画速度和CPU占用
    }
}

// --- Galaxy ---
/**
 * @brief 定义星系表盘中星星的结构体
 */
struct GalaxyStar
{
    int x;     // 星星的X坐标
    int y;     // 星星的Y坐标
    int time;  // 星星的生命周期或闪烁计时器
};

static GalaxyStar galaxy_stars[50]; // 存储星系中所有星星的数组

/**
 * @brief 将角度转换为弧度
 * @param angle 角度值
 * @return 弧度值
 */
static float galaxy_radians(float angle)
{
    return angle * M_PI / 180.0;
}

/**
 * @brief 绘制螺旋星系中的星星
 * @param num_stars 要绘制的星星数量
 * @param arm_length 螺旋臂的长度
 * @param spread 星星在螺旋臂上的分布密度
 * @param rotation_offset 螺旋臂的旋转偏移量
 */
static void galaxy_draw_spiral_stars(uint16_t num_stars, uint16_t arm_length, uint16_t spread, uint16_t rotation_offset)
{
    for (int i = 0; i < num_stars; i++)
    {
        int angle = i * spread + rotation_offset; // 计算星星的角度
        int length = arm_length * i / num_stars;   // 计算星星到中心的距离

        // 根据角度和距离计算星星的X、Y坐标
        int x = length * cos(galaxy_radians(angle)) * 3 / 5;
        int y = length * sin(galaxy_radians(angle)) * 3 / 5;

        // 在屏幕上绘制星星，并将其平移到屏幕中心
        menuSprite.drawPixel(x + tft.width() / 2, y + tft.height() / 2, TFT_WHITE);
    }
}

/**
 * @brief 绘制星系的主体结构（螺旋臂）
 */
static void galaxy_draw_main()
{
    static uint16_t rotation_angle = 0; // 星系旋转角度
    // 绘制四个螺旋臂
    for (int i = 0; i < 4; i++)
    {
        galaxy_draw_spiral_stars(15, 130, 10, i * 90 + rotation_angle);
    }
    // 更新旋转角度，实现旋转动画
    if (++rotation_angle >= 360)
    {
        rotation_angle = 0;
    }
}

/**
 * @brief 随机绘制闪烁的星星
 */
static void galaxy_random_drawStars()
{
    static uint32_t time_counter = 0; // 计时器，用于控制星星生成频率
    if (time_counter % 2 == 0) // 每隔一段时间生成新的星星
    {
        int i = 0;
        // 查找一个未使用的星星槽位
        while (galaxy_stars[i].time && i < 50)
        {
            i++;
        }
        if (i < 50) // 如果找到槽位
        {
            galaxy_stars[i].time = 30; // 设置星星的生命周期
            galaxy_stars[i].x = util_random(tft.width());  // 随机X坐标
            galaxy_stars[i].y = util_random(tft.height()); // 随机Y坐标
        }
    }

    // 绘制并更新所有星星的状态
    for (int i = 0; i < 50; i++)
    {
        if (galaxy_stars[i].time > 0) // 如果星星还在生命周期内
        {
            galaxy_stars[i].time--; // 生命周期减1
            menuSprite.drawPixel(galaxy_stars[i].x, galaxy_stars[i].y, TFT_WHITE); // 绘制星星
        }
    }
    time_counter++; // 更新计时器
}

/**
 * @brief 星系表盘：显示一个旋转的螺旋星系和随机闪烁的星星，并显示当前时间。
 */
static void GalaxyWatchface()
{
    // 初始化所有星星的生命周期为0
    for (int i = 0; i < 50; ++i)
    {
        galaxy_stars[i].time = 0;
    }

    while (1)
    {
        // 检查退出子菜单标志，如果为真则退出当前表盘
        if (exitSubMenu)
        {
            exitSubMenu = false; // 重置标志
            // 如果有整点报时音乐正在播放，则停止它
            if (g_hourlyMusicTaskHandle != NULL)
            {
                vTaskDelete(g_hourlyMusicTaskHandle);
                g_hourlyMusicTaskHandle = NULL;
                noTone(BUZZER_PIN);
                stopBuzzerTask = true;
            }
            return; // 退出表盘函数
        }
        // 如果闹钟正在响铃，则退出表盘，让闹钟优先处理
        if (g_alarm_is_ringing) { return; }
        // 处理周期性数据同步（天气和NTP时间）
        handlePeriodicSync();
        // 处理整点报时逻辑
        handleHourlyChime();

        // 检测按钮点击，如果点击则退出当前表盘
        if (readButton())
        {
            // 如果有整点报时音乐正在播放，则停止它
            if (g_hourlyMusicTaskHandle != NULL)
            {
                vTaskDelete(g_hourlyMusicTaskHandle);
                g_hourlyMusicTaskHandle = NULL;
                noTone(BUZZER_PIN);
                stopBuzzerTask = true;
            }
            tone(BUZZER_PIN, 1500, 50); // 播放按键音
            menuSprite.setTextFont(MENU_FONT); // 恢复菜单字体设置
            return; // 退出表盘函数
        }

        getLocalTime(&timeinfo); // 获取当前本地时间
        menuSprite.fillSprite(TFT_BLACK); // 清空屏幕缓冲区，填充黑色背景

        galaxy_draw_main();        // 绘制星系主体
        galaxy_random_drawStars(); // 绘制随机闪烁的星星

        char timeStr[20]; // 时间字符串缓冲区
        int tenth = (millis() % 1000) / 100; // 获取十分之一秒
        // 格式化时间为 HH:MM:SS.D
        sprintf(timeStr, "%02d:%02d:%02d.%d", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec, tenth);
        menuSprite.setTextFont(1);         // 设置字体
        menuSprite.setTextDatum(TC_DATUM); // 设置文本对齐方式为顶部居中
        menuSprite.setTextSize(4);         // 设置字体大小
        menuSprite.setTextColor(TIME_MAIN_COLOR, TFT_BLACK); // 设置时间颜色
        menuSprite.drawString(timeStr, tft.width() / 2, 15); // 在屏幕顶部居中绘制时间

        menuSprite.pushSprite(0, 0); // 将屏幕缓冲区内容推送到TFT屏幕显示
        vTaskDelay(pdMS_TO_TICKS(20)); // 短暂延时，控制动画速度和CPU占用
    }
}


/**
 * @brief 模拟时钟表盘：显示一个带有指针的模拟时钟，并显示当前时间。
 * @details 绘制一个圆形表盘，包含刻度、小时数字，并根据当前时间绘制时针、分针和秒针。
 */
static void SimClockWatchface()
{
    while (1)
    {
        // 检查退出子菜单标志，如果为真则退出当前表盘
        if (exitSubMenu)
        {
            exitSubMenu = false; // 重置标志
            // 如果有整点报时音乐正在播放，则停止它
            if (g_hourlyMusicTaskHandle != NULL)
            {
                vTaskDelete(g_hourlyMusicTaskHandle);
                g_hourlyMusicTaskHandle = NULL;
                noTone(BUZZER_PIN);
                stopBuzzerTask = true;
            }
            return; // 退出表盘函数
        }
        // 如果闹钟正在响铃，则退出表盘，让闹钟优先处理
        if (g_alarm_is_ringing) { return; }
        // 处理周期性数据同步（天气和NTP时间）
        handlePeriodicSync();
        // 处理整点报时逻辑
        handleHourlyChime();

        // 检测按钮点击，如果点击则退出当前表盘
        if (readButton())
        {
            // 如果有整点报时音乐正在播放，则停止它
            if (g_hourlyMusicTaskHandle != NULL)
            {
                vTaskDelete(g_hourlyMusicTaskHandle);
                g_hourlyMusicTaskHandle = NULL;
                noTone(BUZZER_PIN);
                stopBuzzerTask = true;
            }
            tone(BUZZER_PIN, 1500, 50); // 播放按键音
            menuSprite.setTextFont(MENU_FONT); // 恢复菜单字体设置
            return; // 退出表盘函数
        }

        getLocalTime(&timeinfo); // 获取当前本地时间
        menuSprite.fillSprite(TFT_BLACK); // 清空屏幕缓冲区，填充黑色背景

        int centerX = tft.width() / 2;  // 屏幕中心X坐标
        int centerY = tft.height() / 2; // 屏幕中心Y坐标
        int radius = std::min(tft.width(), tft.height()) / 2 - 10; // 时钟半径，留出边距

        // 绘制时钟外圆
        menuSprite.drawCircle(centerX, centerY, radius, TFT_WHITE);

        // 绘制分钟刻度
        for (int i = 0; i < 60; i++)
        {
            if (i % 5 != 0) // 只为非小时刻度绘制
            {
                float angle = (i * 6 - 90) * M_PI / 180.0; // 计算刻度角度
                // 计算刻度内外端点坐标
                int startX = centerX + (int) ((radius - 4) * cos(angle));
                int startY = centerY + (int) ((radius - 4) * sin(angle));
                int endX = centerX + (int) (radius * cos(angle));
                int endY = centerY + (int) (radius * sin(angle));
                menuSprite.drawLine(startX, startY, endX, endY, TFT_DARKGREY); // 绘制刻度线
            }
        }

        // 绘制小时数字
        menuSprite.setTextDatum(MC_DATUM); // 设置文本对齐方式为居中
        menuSprite.setTextSize(2);         // 设置字体大小
        menuSprite.setTextColor(TFT_WHITE, TFT_BLACK); // 设置字体颜色
        for (int i = 1; i <= 12; i++)
        {
            float angle = (i * 30 - 90) * M_PI / 180.0; // 计算数字角度
            // 计算数字绘制位置
            int numX = centerX + (int) ((radius - 12) * cos(angle));
            int numY = centerY + (int) ((radius - 12) * sin(angle));
            menuSprite.drawString(String(i), numX, numY); // 绘制小时数字
        }

        // 绘制中心圆点
        menuSprite.fillCircle(centerX, centerY, 3, TFT_WHITE);

        // 绘制时针
        float hourAngle = (timeinfo.tm_hour % 12 + timeinfo.tm_min / 60.0) * 30 - 90; // 计算时针角度
        int hourX = centerX + (int) (0.5 * radius * cos(hourAngle * M_PI / 180.0));  // 计算时针X端点
        int hourY = centerY + (int) (0.5 * radius * sin(hourAngle * M_PI / 180.0));  // 计算时针Y端点
        menuSprite.drawLine(centerX, centerY, hourX, hourY, TFT_RED); // 绘制时针

        // 绘制分针
        float minAngle = (timeinfo.tm_min + timeinfo.tm_sec / 60.0) * 6 - 90; // 计算分针角度
        int minX = centerX + (int) (0.8 * radius * cos(minAngle * M_PI / 180.0)); // 计算分针X端点
        int minY = centerY + (int) (0.8 * radius * sin(minAngle * M_PI / 180.0)); // 计算分针Y端点
        menuSprite.drawLine(centerX, centerY, minX, minY, TFT_GREEN); // 绘制分针

        // 绘制秒针
        float secAngle = timeinfo.tm_sec * 6 - 90; // 计算秒针角度
        int secX = centerX + (int) (0.9 * radius * cos(secAngle * M_PI / 180.0)); // 计算秒针X端点
        int secY = centerY + (int) (0.9 * radius * sin(secAngle * M_PI / 180.0)); // 计算秒针Y端点
        menuSprite.drawLine(centerX, centerY, secX, secY, TFT_BLUE); // 绘制秒针

        menuSprite.pushSprite(0, 0); // 将屏幕缓冲区内容推送到TFT屏幕显示
        vTaskDelay(pdMS_TO_TICKS(50)); // 短暂延时，确保秒针平滑更新
    }
}

/**
 * @brief 占位符表盘：一个简单的占位符表盘，显示“Placeholder”文本和通用UI元素。
 * @details 用于尚未实现具体功能的表盘项，提供一个基本的显示界面。
 */
static void PlaceholderWatchface()
{
    while (1)
    {
        // 检查退出子菜单标志，如果为真则退出当前表盘
        if (exitSubMenu)
        {
            exitSubMenu = false; // 重置标志
            // 如果有整点报时音乐正在播放，则停止它
            if (g_hourlyMusicTaskHandle != NULL)
            {
                vTaskDelete(g_hourlyMusicTaskHandle);
                g_hourlyMusicTaskHandle = NULL;
                noTone(BUZZER_PIN);
                stopBuzzerTask = true;
            }
            return; // 退出表盘函数
        }
        // 如果闹钟正在响铃，则退出表盘，让闹钟优先处理
        if (g_alarm_is_ringing) { return; }
        // 处理周期性数据同步（天气和NTP时间）
        handlePeriodicSync();
        // 处理整点报时逻辑
        handleHourlyChime();
        // 检测按钮点击，如果点击则退出当前表盘
        if (readButton())
        {
            // 如果有整点报时音乐正在播放，则停止它
            if (g_hourlyMusicTaskHandle != NULL)
            {
                vTaskDelete(g_hourlyMusicTaskHandle);
                g_hourlyMusicTaskHandle = NULL;
                noTone(BUZZER_PIN);
                stopBuzzerTask = true;
            }
            tone(BUZZER_PIN, 1500, 50); // 播放按键音
            menuSprite.setTextFont(MENU_FONT); // 恢复菜单字体设置
            return; // 退出表盘函数
        }

        getLocalTime(&timeinfo); // 获取当前本地时间
        menuSprite.fillSprite(TFT_BLACK); // 清空屏幕缓冲区，填充黑色背景
        drawAdvancedCommonElements();     // 绘制高级通用UI元素（天气、日期、DS18B20温度、WiFi状态等）

        menuSprite.setTextFont(1);         // 设置字体
        menuSprite.setTextDatum(MC_DATUM); // 设置文本对齐方式为居中
        menuSprite.setTextSize(2);         // 设置字体大小
        menuSprite.drawString("Placeholder", tft.width() / 2, tft.height() / 2 + 20); // 绘制占位符文本

        menuSprite.pushSprite(0, 0); // 将屏幕缓冲区内容推送到TFT屏幕显示
        vTaskDelay(pdMS_TO_TICKS(100)); // 短暂延时
    }
}

// --- Vector Scan ---
#define TICKER_GAP 4 // 数字滚动时的间隔

/**
 * @brief 定义数字滚动显示的数据结构
 */
struct TickerData
{
    int16_t x, y, w, h; // 数字的X、Y坐标，宽度和高度
    uint8_t val, max_val; // 当前显示的值和最大值（用于循环）
    bool moving;        // 标志，指示数字是否正在滚动
    int16_t y_pos;      // 滚动动画的当前Y位置
};

/**
 * @brief 绘制单个数字的扫描动画效果
 * @param data 指向TickerData结构体的指针，包含数字的绘制信息和动画状态
 */
static void drawTickerNum(TickerData *data)
{
    int16_t y_pos = data->y_pos; // 动画的当前Y位置
    int16_t x = data->x, y = data->y, w = data->w, h = data->h; // 数字的坐标和尺寸
    // 计算上一个数字的值（用于滚动动画）
    uint8_t prev_val = (data->val == 0) ? data->max_val : data->val - 1;
    char current_char[2] = { (char) ('0' + data->val), '\0' }; // 当前数字的字符表示
    char prev_char[2] = { (char) ('0' + prev_val), '\0' };     // 上一个数字的字符表示

    // 如果数字没有在移动，或者动画刚开始（y_pos为0），则直接绘制当前数字
    if (!data->moving || y_pos == 0)
    {
        menuSprite.drawString(current_char, x, y);
        return;
    }

    // 计算当前数字需要显示的高度（从底部向上扫描）
    int16_t reveal_height = y_pos;
    if (reveal_height > h) reveal_height = h; // 限制最大显示高度为数字高度

    // 设置视口，只显示当前数字的底部一部分
    menuSprite.setViewport(x, y + h - reveal_height, w, reveal_height);
    menuSprite.drawString(current_char, 0, -(h - reveal_height)); // 绘制当前数字，向上偏移以显示底部
    menuSprite.resetViewport(); // 重置视口

    // 设置视口，只显示上一个数字的顶部一部分
    menuSprite.setViewport(x, y, w, h - reveal_height);
    menuSprite.drawString(prev_char, 0, 0); // 绘制上一个数字
    menuSprite.resetViewport(); // 重置视口

    // 绘制扫描线
    menuSprite.drawFastHLine(x, y + h - reveal_height, w, TFT_WHITE);
    // 绘制连接线（从屏幕底部中心到扫描线）
    menuSprite.drawLine(tft.width() / 2, tft.height() - 1, x + w / 2, y + h - reveal_height, TFT_GREEN);
}

// --- Vector Scroll ---
/**
 * @brief 绘制单个数字的滚动动画效果
 * @param data 指向TickerData结构体的指针，包含数字的绘制信息和动画状态
 */
static void drawVectorScrollTickerNum(TickerData *data)
{
    int16_t y_pos = data->y_pos; // 动画的当前Y位置
    int16_t x = data->x, y = data->y, h = data->h; // 数字的X、Y坐标和高度
    // 计算上一个数字的值（用于滚动动画）
    uint8_t prev_val = (data->val == 0) ? data->max_val : data->val - 1;
    char current_char[2] = { (char) ('0' + data->val), '\0' }; // 当前数字的字符表示
    char prev_char[2] = { (char) ('0' + prev_val), '\0' };     // 上一个数字的字符表示

    // 如果数字没有在移动，或者动画刚开始/结束，则直接绘制当前数字
    if (!data->moving || y_pos == 0 || y_pos > h + TICKER_GAP)
    {
        menuSprite.drawString(current_char, x, y);
        return;
    }

    // 绘制从底部向上滚入的新数字
    // 它从 y + h + TICKER_GAP 位置开始，移动到 y 位置
    int16_t new_y = y + h + TICKER_GAP - y_pos;
    menuSprite.drawString(current_char, x, new_y);

    // 绘制向上滚出屏幕的旧数字
    // 它从 y 位置开始，移动到 y - (h + TICKER_GAP) 位置
    int16_t old_y = y - y_pos;
    menuSprite.drawString(prev_char, x, old_y);
}

/**
 * @brief 矢量滚动表盘：以滚动动画效果显示时间（时、分、秒）。
 * @details 数字从底部滚入，旧数字从顶部滚出，形成平滑的滚动效果。
 *          小时和分钟数字较大，秒数较小并位于右下方。
 */
static void VectorScrollWatchface()
{
    time_s last_time;     // 存储上一次的时间，用于检测时间变化
    TickerData tickers[6]; // 存储6个数字（HHMMSS）的滚动数据

    int num_w = 35, num_h = 50; // 小时和分钟数字的宽度和高度
    int colon_w = 15;           // 冒号的宽度（用于间距）
    // 计算起始X坐标，使整个时间显示居中
    int start_x = (tft.width() - (num_w * 4 + colon_w * 2 + num_w * 2)) / 2;
    int y_main = (tft.height() - num_h) / 2; // 主数字的Y坐标

    // 初始化：获取当前时间并设置TickerData
    getLocalTime(&timeinfo);
    last_time.hour = timeinfo.tm_hour;
    last_time.mins = timeinfo.tm_min;
    last_time.secs = timeinfo.tm_sec;

    // 初始化小时、分钟、秒的TickerData
    tickers[0] = { start_x, y_main, num_w, num_h, (uint8_t) (last_time.hour / 10), 2, false, 0 }; // 小时十位
    tickers[1] = { start_x + num_w + 5, y_main, num_w, num_h, (uint8_t) (last_time.hour % 10), 9, false, 0 }; // 小时个位
    tickers[2] = { start_x + num_w * 2 + colon_w + 20, y_main, num_w, num_h, (uint8_t) (last_time.mins / 10), 5, false, 0 }; // 分钟十位
    tickers[3] = { start_x + num_w * 3 + colon_w + 20 + 5, y_main, num_w, num_h, (uint8_t) (last_time.mins % 10), 9, false, 0 }; // 分钟个位

    // 秒数现在更小并位于分钟下方
    int sec_num_w = 20, sec_num_h = 25; // 秒数数字的宽度和高度
    int sec_x_offset = tickers[3].x + num_w + 5; // 秒数X偏移量，位于分钟个位右侧
    int sec_y_offset = y_main + num_h - sec_num_h + 5; // 秒数Y偏移量，位于分钟下方

    tickers[4] = { sec_x_offset, sec_y_offset, sec_num_w, sec_num_h, (uint8_t) (last_time.secs / 10), 5, false, 0 }; // 秒数十位
    tickers[5] = { sec_x_offset + sec_num_w, sec_y_offset, sec_num_w, sec_num_h, (uint8_t) (last_time.secs % 10), 9, false, 0 }; // 秒数个位

    while (1)
    {
        // 检查退出子菜单标志，如果为真则退出当前表盘
        if (exitSubMenu)
        {
            exitSubMenu = false; // 重置标志
            // 如果有整点报时音乐正在播放，则停止它
            if (g_hourlyMusicTaskHandle != NULL)
            {
                vTaskDelete(g_hourlyMusicTaskHandle);
                g_hourlyMusicTaskHandle = NULL;
                noTone(BUZZER_PIN);
                stopBuzzerTask = true;
            }
            return; // 退出表盘函数
        }
        // 如果闹钟正在响铃，则退出表盘，让闹钟优先处理
        if (g_alarm_is_ringing) { return; }
        // 处理周期性数据同步（天气和NTP时间）
        handlePeriodicSync();
        // 处理整点报时逻辑
        handleHourlyChime();
        // 检测按钮点击，如果点击则退出当前表盘
        if (readButton())
        {
            // 如果有整点报时音乐正在播放，则停止它
            if (g_hourlyMusicTaskHandle != NULL)
            {
                vTaskDelete(g_hourlyMusicTaskHandle);
                g_hourlyMusicTaskHandle = NULL;
                noTone(BUZZER_PIN);
                stopBuzzerTask = true;
            }
            tone(BUZZER_PIN, 1500, 50); // 播放按键音
            menuSprite.setTextFont(MENU_FONT); // 恢复菜单字体设置
            return; // 退出表盘函数
        }

        getLocalTime(&timeinfo); // 获取当前本地时间
        g_watchface_timeDate.time.hour = timeinfo.tm_hour;
        g_watchface_timeDate.time.mins = timeinfo.tm_min;
        g_watchface_timeDate.time.secs = timeinfo.tm_sec;

        // 检测时间变化，并设置对应数字的滚动标志和初始Y位置
        if (g_watchface_timeDate.time.hour / 10 != last_time.hour / 10) { tickers[0].moving = true; tickers[0].y_pos = 0; tickers[0].val = g_watchface_timeDate.time.hour / 10; }
        if (g_watchface_timeDate.time.hour % 10 != last_time.hour % 10) { tickers[1].moving = true; tickers[1].y_pos = 0; tickers[1].val = g_watchface_timeDate.time.hour % 10; }
        if (g_watchface_timeDate.time.mins / 10 != last_time.mins / 10) { tickers[2].moving = true; tickers[2].y_pos = 0; tickers[2].val = g_watchface_timeDate.time.mins / 10; }
        if (g_watchface_timeDate.time.mins % 10 != last_time.mins % 10) { tickers[3].moving = true; tickers[3].y_pos = 0; tickers[3].val = g_watchface_timeDate.time.mins % 10; }
        if (g_watchface_timeDate.time.secs / 10 != last_time.secs / 10) { tickers[4].moving = true; tickers[4].y_pos = 0; tickers[4].val = g_watchface_timeDate.time.secs / 10; }
        if (g_watchface_timeDate.time.secs % 10 != last_time.secs % 10) { tickers[5].moving = true; tickers[5].y_pos = 0; tickers[5].val = g_watchface_timeDate.time.secs % 10; }
        last_time = g_watchface_timeDate.time; // 更新上一次的时间

        menuSprite.fillSprite(TFT_BLACK); // 清空屏幕缓冲区，填充黑色背景
        drawAdvancedCommonElements();     // 绘制高级通用UI元素
        menuSprite.setTextDatum(TL_DATUM); // 设置文本对齐方式为左上角

        // 更新并绘制所有数字的滚动动画
        for (int i = 0; i < 6; ++i)
        {
            bool is_sec = (i >= 4); // 判断是否为秒数数字
            int current_h = is_sec ? sec_num_h : num_h; // 获取当前数字的高度
            menuSprite.setTextSize(is_sec ? 3 : 7); // 设置字体大小（秒数较小）

            if (tickers[i].moving)
            {
                int16_t *yPos = &tickers[i].y_pos; // 获取当前数字的Y位置指针
                // 根据Y位置调整滚动速度，实现加速和减速效果
                if (*yPos <= 3) (*yPos)++;
                else if (*yPos <= 6) (*yPos) += 3;
                else if (*yPos <= 16) (*yPos) += 5;
                else if (*yPos <= 22) (*yPos) += 3;
                else if (*yPos <= current_h + TICKER_GAP) (*yPos)++;

                // 如果滚动动画完成，重置标志和Y位置
                if (*yPos > current_h + TICKER_GAP)
                {
                    tickers[i].moving = false;
                    tickers[i].y_pos = 0;
                }
            }
            drawVectorScrollTickerNum(&tickers[i]); // 绘制滚动数字
        }

        // 绘制闪烁的冒号（HH和MM之间）
        if (millis() % 1000 < 500) // 每500毫秒闪烁一次
        {
            menuSprite.setTextSize(4); // 冒号字体大小
            menuSprite.setTextColor(TIME_MAIN_COLOR, TFT_BLACK); // 冒号颜色
            menuSprite.drawString(":", start_x + num_w * 2 + 10, y_main + 5); // 绘制冒号
        }

        // 绘制0.1秒数字
        int tenth = (millis() % 1000) / 100; // 获取十分之一秒
        menuSprite.setTextSize(1);         // 字体大小
        menuSprite.setTextColor(TIME_TENTH_COLOR, TFT_BLACK); // 颜色
        menuSprite.setTextDatum(TL_DATUM); // 左上角对齐
        int x_pos = tickers[5].x + tickers[5].w; // 0.1秒数字的X位置
        int y_pos = tickers[5].y + tickers[5].h - 8; // 0.1秒数字的Y位置
        menuSprite.drawString(String(tenth), x_pos, y_pos); // 绘制0.1秒数字

        menuSprite.pushSprite(0, 0); // 将屏幕缓冲区内容推送到TFT屏幕显示
        vTaskDelay(pdMS_TO_TICKS(20)); // 短暂延时，控制动画速度和CPU占用
    }
}

/**
 * @brief 矢量扫描表盘：以扫描动画效果显示时间（时、分、秒）。
 * @details 数字从底部向上扫描显示，旧数字从顶部向下消失，形成扫描效果。
 *          小时和分钟数字较大，秒数较小并位于右下方。
 */
static void VectorScanWatchface()
{
    time_s last_time;     // 存储上一次的时间，用于检测时间变化
    TickerData tickers[6]; // 存储6个数字（HHMMSS）的扫描数据

    int num_w = 35, num_h = 50; // 小时和分钟数字的宽度和高度
    int colon_w = 15;           // 冒号的宽度（用于间距）
    // 计算起始X坐标，使整个时间显示居中
    int start_x = (tft.width() - (num_w * 4 + colon_w * 2 + num_w * 2)) / 2;
    int y_main = (tft.height() - num_h) / 2; // 主数字的Y坐标

    // 初始化：获取当前时间并设置TickerData
    getLocalTime(&timeinfo);
    last_time.hour = timeinfo.tm_hour;
    last_time.mins = timeinfo.tm_min;
    last_time.secs = timeinfo.tm_sec;

    // 初始化小时、分钟、秒的TickerData
    tickers[0] = { start_x, y_main, num_w, num_h, (uint8_t) (last_time.hour / 10), 2, false, 0 }; // 小时十位
    tickers[1] = { start_x + num_w + 5, y_main, num_w, num_h, (uint8_t) (last_time.hour % 10), 9, false, 0 }; // 小时个位
    tickers[2] = { start_x + num_w * 2 + colon_w + 20, y_main, num_w, num_h, (uint8_t) (last_time.mins / 10), 5, false, 0 }; // 分钟十位
    tickers[3] = { start_x + num_w * 3 + colon_w + 20 + 5, y_main, num_w, num_h, (uint8_t) (last_time.mins % 10), 9, false, 0 }; // 分钟个位

    // 秒数现在更小并位于分钟下方
    int sec_num_w = 20, sec_num_h = 25; // 秒数数字的宽度和高度
    int sec_x_offset = tickers[3].x + num_w + 5; // 秒数X偏移量，位于分钟个位右侧
    int sec_y_offset = y_main + num_h - sec_num_h + 5; // 秒数Y偏移量，位于分钟下方

    tickers[4] = { sec_x_offset, sec_y_offset, sec_num_w, sec_num_h, (uint8_t) (last_time.secs / 10), 5, false, 0 }; // 秒数十位
    tickers[5] = { sec_x_offset + sec_num_w, sec_y_offset, sec_num_w, sec_num_h, (uint8_t) (last_time.secs % 10), 9, false, 0 }; // 秒数个位

    while (1)
    {
        // 检查退出子菜单标志，如果为真则退出当前表盘
        if (exitSubMenu)
        {
            exitSubMenu = false; // 重置标志
            // 如果有整点报时音乐正在播放，则停止它
            if (g_hourlyMusicTaskHandle != NULL)
            {
                vTaskDelete(g_hourlyMusicTaskHandle);
                g_hourlyMusicTaskHandle = NULL;
                noTone(BUZZER_PIN);
                stopBuzzerTask = true;
            }
            return; // 退出表盘函数
        }
        // 如果闹钟正在响铃，则退出表盘，让闹钟优先处理
        if (g_alarm_is_ringing) { return; }
        // 处理周期性数据同步（天气和NTP时间）
        handlePeriodicSync();
        // 处理整点报时逻辑
        handleHourlyChime();
        // 检测按钮点击，如果点击则退出当前表盘
        if (readButton())
        {
            // 如果有整点报时音乐正在播放，则停止它
            if (g_hourlyMusicTaskHandle != NULL)
            {
                vTaskDelete(g_hourlyMusicTaskHandle);
                g_hourlyMusicTaskHandle = NULL;
                noTone(BUZZER_PIN);
                stopBuzzerTask = true;
            }
            tone(BUZZER_PIN, 1500, 50); // 播放按键音
            menuSprite.setTextFont(MENU_FONT); // 恢复菜单字体设置
            return; // 退出表盘函数
        }

        getLocalTime(&timeinfo); // 获取当前本地时间
        g_watchface_timeDate.time.hour = timeinfo.tm_hour;
        g_watchface_timeDate.time.mins = timeinfo.tm_min;
        g_watchface_timeDate.time.secs = timeinfo.tm_sec;

        // 检测时间变化，并设置对应数字的扫描标志和初始Y位置
        if (g_watchface_timeDate.time.hour / 10 != last_time.hour / 10) { tickers[0].moving = true; tickers[0].y_pos = 0; tickers[0].val = g_watchface_timeDate.time.hour / 10; }
        if (g_watchface_timeDate.time.hour % 10 != last_time.hour % 10) { tickers[1].moving = true; tickers[1].y_pos = 0; tickers[1].val = g_watchface_timeDate.time.hour % 10; }
        if (g_watchface_timeDate.time.mins / 10 != last_time.mins / 10) { tickers[2].moving = true; tickers[2].y_pos = 0; tickers[2].val = g_watchface_timeDate.time.mins / 10; }
        if (g_watchface_timeDate.time.mins % 10 != last_time.mins % 10) { tickers[3].moving = true; tickers[3].y_pos = 0; tickers[3].val = g_watchface_timeDate.time.mins % 10; }
        if (g_watchface_timeDate.time.secs / 10 != last_time.secs / 10) { tickers[4].moving = true; tickers[4].y_pos = 0; tickers[4].val = g_watchface_timeDate.time.secs / 10; }
        if (g_watchface_timeDate.time.secs % 10 != last_time.secs % 10) { tickers[5].moving = true; tickers[5].y_pos = 0; tickers[5].val = g_watchface_timeDate.time.secs % 10; }
        last_time = g_watchface_timeDate.time; // 更新上一次的时间

        menuSprite.fillSprite(TFT_BLACK); // 清空屏幕缓冲区，填充黑色背景
        drawAdvancedCommonElements();     // 绘制高级通用UI元素
        menuSprite.setTextDatum(TL_DATUM); // 设置文本对齐方式为左上角
        menuSprite.setTextColor(TIME_MAIN_COLOR, TFT_BLACK); // 设置时间颜色

        // 更新并绘制所有数字的扫描动画
        for (int i = 0; i < 6; ++i)
        {
            menuSprite.setTextSize((i < 4) ? 7 : 3); // 设置字体大小（HH:MM较大，SS较小）
            if (tickers[i].moving)
            {
                tickers[i].y_pos += 5; // 增加Y位置，模拟扫描
                // 如果扫描动画完成，重置标志
                if (tickers[i].y_pos >= ((i < 4) ? num_h : sec_num_h) + TICKER_GAP) tickers[i].moving = false;
            }
            drawTickerNum(&tickers[i]); // 绘制扫描数字
        }

        // 绘制闪烁的冒号（HH和MM之间）
        if (millis() % 1000 < 500) // 每500毫秒闪烁一次
        {
            menuSprite.setTextSize(4); // 冒号字体大小
            menuSprite.setTextColor(TFT_WHITE, TFT_BLACK); // 冒号颜色
            menuSprite.drawString(":", start_x + num_w * 2 + 10, y_main + 5); // 绘制冒号
        }

        // 绘制0.1秒数字
        int tenth = (millis() % 1000) / 100; // 获取十分之一秒
        menuSprite.setTextSize(1);         // 字体大小
        menuSprite.setTextColor(TIME_TENTH_COLOR, TFT_BLACK); // 颜色
        menuSprite.setTextDatum(TL_DATUM); // 左上角对齐
        int x_pos = tickers[5].x + tickers[5].w; // 0.1秒数字的X位置
        int y_pos = tickers[5].y + tickers[5].h - 8; // 0.1秒数字的Y位置
        menuSprite.drawString(String(tenth), x_pos, y_pos); // 绘制0.1秒数字

        menuSprite.pushSprite(0, 0); // 将屏幕缓冲区内容推送到TFT屏幕显示
        vTaskDelay(pdMS_TO_TICKS(20)); // 短暂延时，控制动画速度和CPU占用
    }
}

// --- Simple Clock ---
/**
 * @brief 简单时钟表盘：以大字体显示当前时间（时、分、秒和十分之一秒），并集成通用UI元素。
 */
static void SimpleClockWatchface()
{
    while (1)
    {
        // 检查退出子菜单标志，如果为真则退出当前表盘
        if (exitSubMenu)
        {
            exitSubMenu = false; // 重置标志
            // 如果有整点报时音乐正在播放，则停止它
            if (g_hourlyMusicTaskHandle != NULL)
            {
                vTaskDelete(g_hourlyMusicTaskHandle);
                g_hourlyMusicTaskHandle = NULL;
                noTone(BUZZER_PIN);
                stopBuzzerTask = true;
            }
            return; // 退出表盘函数
        }
        // 如果闹钟正在响铃，则退出表盘，让闹钟优先处理
        if (g_alarm_is_ringing) { return; }
        // 处理周期性数据同步（天气和NTP时间）
        handlePeriodicSync();
        // 处理整点报时逻辑
        handleHourlyChime();

        // 检测按钮点击，如果点击则退出当前表盘
        if (readButton())
        {
            // 如果有整点报时音乐正在播放，则停止它
            if (g_hourlyMusicTaskHandle != NULL)
            {
                vTaskDelete(g_hourlyMusicTaskHandle);
                g_hourlyMusicTaskHandle = NULL;
                noTone(BUZZER_PIN);
                stopBuzzerTask = true;
            }
            tone(BUZZER_PIN, 1500, 50); // 播放按键音
            menuSprite.setTextFont(MENU_FONT); // 恢复菜单字体设置
            return; // 退出表盘函数
        }

        getLocalTime(&timeinfo); // 获取当前本地时间
        menuSprite.fillSprite(TFT_BLACK); // 清空屏幕缓冲区，填充黑色背景
        drawAdvancedCommonElements();     // 绘制高级通用UI元素

        menuSprite.setTextFont(1);         // 设置字体
        menuSprite.setTextDatum(MC_DATUM); // 设置文本对齐方式为居中
        char timeStr[10];                  // 时间字符串缓冲区
        // 格式化时间为 HH:MM:SS
        sprintf(timeStr, "%02d:%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
        menuSprite.setTextSize(5);         // 设置时间字体大小
        menuSprite.setTextColor(TIME_MAIN_COLOR, TFT_BLACK); // 设置时间颜色

        int timeWidth = menuSprite.textWidth(timeStr); // 获取时间字符串的宽度
        int timeHeight = menuSprite.fontHeight();     // 获取字体高度
        int timeX = tft.width() / 2;                  // 时间X坐标（居中）
        int timeY = (tft.height() - timeHeight) / 2 + 10; // 时间Y坐标（居中偏下）
        menuSprite.drawString(timeStr, timeX, timeY); // 绘制时间字符串

        // 绘制十分之一秒数字
        int tenth = (millis() % 1000) / 100; // 获取十分之一秒
        menuSprite.setTextSize(1);         // 设置字体大小
        menuSprite.drawString(String(tenth), timeX + timeWidth / 2 + 10, timeY + 10); // 绘制十分之一秒数字

        menuSprite.pushSprite(0, 0); // 将屏幕缓冲区内容推送到TFT屏幕显示
        vTaskDelay(pdMS_TO_TICKS(100)); // 短暂延时
    }
}

// --- Terminal Sim & Code Rain ---
#define RAIN_COLS 30 // 雨滴（字符）列数
char rain_chars[RAIN_COLS]; // 存储每列的字符
int rain_pos[RAIN_COLS];    // 存储每列字符的Y坐标
int rain_speed[RAIN_COLS];  // 存储每列字符的下落速度

/**
 * @brief 共享的雨滴动画逻辑，用于Terminal Sim和Code Rain表盘。
 * @param color 雨滴字符的颜色。
 * @details 模拟字符雨下落的效果，同时显示当前时间。
 */
static void shared_rain_logic(uint16_t color)
{
    util_randomSeed(millis()); // 使用当前时间作为随机数种子
    // 初始化每列雨滴的Y坐标、速度和字符
    for (int i = 0; i < RAIN_COLS; i++)
    {
        rain_pos[i] = util_random_range(0, tft.height()); // 随机初始Y坐标
        rain_speed[i] = util_random_range(1, 5);          // 随机下落速度
        rain_chars[i] = (char) util_random_range(33, 126); // 随机ASCII字符
    }
    while (1)
    {
        // 检查退出子菜单标志，如果为真则退出当前表盘
        if (exitSubMenu)
        {
            exitSubMenu = false; // 重置标志
            // 如果有整点报时音乐正在播放，则停止它
            if (g_hourlyMusicTaskHandle != NULL)
            {
                vTaskDelete(g_hourlyMusicTaskHandle);
                g_hourlyMusicTaskHandle = NULL;
                noTone(BUZZER_PIN);
                stopBuzzerTask = true;
            }
            return; // 退出表盘函数
        }
        // 如果闹钟正在响铃，则退出表盘，让闹钟优先处理
        if (g_alarm_is_ringing) { return; }
        // 处理周期性数据同步（天气和NTP时间）
        handlePeriodicSync();
        // 处理整点报时逻辑
        handleHourlyChime();
        // 检测按钮点击，如果点击则退出当前表盘
        if (readButton())
        {
            // 如果有整点报时音乐正在播放，则停止它
            if (g_hourlyMusicTaskHandle != NULL)
            {
                vTaskDelete(g_hourlyMusicTaskHandle);
                g_hourlyMusicTaskHandle = NULL;
                noTone(BUZZER_PIN);
                stopBuzzerTask = true;
            }
            tone(BUZZER_PIN, 1500, 50); // 播放按键音
            menuSprite.setTextFont(MENU_FONT); // 恢复菜单字体设置
            return; // 退出表盘函数
        }

        getLocalTime(&timeinfo); // 获取当前本地时间
        menuSprite.fillSprite(TFT_BLACK); // 清空屏幕缓冲区，填充黑色背景

        // 更新并绘制每列雨滴
        for (int i = 0; i < RAIN_COLS; i++)
        {
            rain_pos[i] += rain_speed[i]; // 更新Y坐标
            if (rain_pos[i] > tft.height()) // 如果超出屏幕底部
            {
                rain_pos[i] = 0; // 重置到顶部
                rain_chars[i] = (char) util_random_range(33, 126); // 重新生成字符
            }
            draw_char(i * 8, rain_pos[i], rain_chars[i], color, TFT_BLACK, 1); // 绘制字符
        }

        drawAdvancedCommonElements(); // 绘制高级通用UI元素

        menuSprite.setTextFont(1);         // 设置字体
        menuSprite.setTextDatum(MC_DATUM); // 设置文本对齐方式为居中
        char timeStr[10];                  // 时间字符串缓冲区
        // 格式化时间为 HH:MM:SS
        sprintf(timeStr, "%02d:%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
        menuSprite.setTextSize(4);         // 设置时间字体大小
        menuSprite.setTextColor(TIME_MAIN_COLOR, TFT_BLACK); // 设置时间颜色

        int timeWidth = menuSprite.textWidth(timeStr); // 获取时间字符串的宽度
        int timeHeight = menuSprite.fontHeight();     // 获取字体高度
        int timeX = tft.width() / 2;                  // 时间X坐标（居中）
        int timeY = (tft.height() - timeHeight) / 2 + 10; // 时间Y坐标（居中偏下）
        menuSprite.drawString(timeStr, timeX, timeY); // 绘制时间字符串

        // 绘制十分之一秒数字
        int tenth = (millis() % 1000) / 100; // 获取十分之一秒
        menuSprite.setTextSize(1);         // 设置字体大小
        menuSprite.drawString(String(tenth), timeX + timeWidth / 2 + 10, timeY + 10); // 绘制十分之一秒数字

        menuSprite.pushSprite(0, 0); // 将屏幕缓冲区内容推送到TFT屏幕显示
        vTaskDelay(pdMS_TO_TICKS(20)); // 短暂延时，控制动画速度和CPU占用
    }
}
/**
 * @brief 终端模拟表盘：以绿色字符雨效果显示时间。
 */
static void TerminalSimWatchface() { shared_rain_logic(TFT_GREEN); }
/**
 * @brief 代码雨表盘：以青色字符雨效果显示时间。
 */
static void CodeRainWatchface() { shared_rain_logic(TFT_CYAN); }

// --- Snow ---
#define SNOW_PARTICLES 100 // 雪花粒子数量
std::vector<std::pair<int, int>> snow_particles(SNOW_PARTICLES); // 存储雪花粒子坐标的向量

/**
 * @brief 雪花表盘：模拟雪花飘落的效果，并显示当前时间。
 */
static void SnowWatchface()
{
    util_randomSeed(millis()); // 使用当前时间作为随机数种子
    // 初始化所有雪花粒子的随机位置
    for (auto &p : snow_particles) { p.first = util_random_range(0, tft.width()); p.second = util_random_range(0, tft.height()); }
    while (1)
    {
        // 检查退出子菜单标志，如果为真则退出当前表盘
        if (exitSubMenu)
        {
            exitSubMenu = false; // 重置标志
            // 如果有整点报时音乐正在播放，则停止它
            if (g_hourlyMusicTaskHandle != NULL)
            {
                vTaskDelete(g_hourlyMusicTaskHandle);
                g_hourlyMusicTaskHandle = NULL;
                noTone(BUZZER_PIN);
                stopBuzzerTask = true;
            }
            return; // 退出表盘函数
        }
        // 如果闹钟正在响铃，则退出表盘，让闹钟优先处理
        if (g_alarm_is_ringing) { return; }
        // 处理周期性数据同步（天气和NTP时间）
        handlePeriodicSync();
        // 处理整点报时逻辑
        handleHourlyChime();
        // 检测按钮点击，如果点击则退出当前表盘
        if (readButton())
        {
            // 如果有整点报时音乐正在播放，则停止它
            if (g_hourlyMusicTaskHandle != NULL)
            {
                vTaskDelete(g_hourlyMusicTaskHandle);
                g_hourlyMusicTaskHandle = NULL;
                noTone(BUZZER_PIN);
                stopBuzzerTask = true;
            }
            tone(BUZZER_PIN, 1500, 50); // 播放按键音
            menuSprite.setTextFont(MENU_FONT); // 恢复菜单字体设置
            return; // 退出表盘函数
        }

        getLocalTime(&timeinfo); // 获取当前本地时间
        menuSprite.fillSprite(TFT_BLACK); // 清空屏幕缓冲区，填充黑色背景

        // 更新并绘制雪花粒子
        for (auto &p : snow_particles)
        {
            p.second += 1; // 雪花向下移动
            if (p.second > tft.height()) { p.second = 0; p.first = util_random_range(0, tft.width()); } // 如果超出屏幕底部，重置到顶部随机位置
            menuSprite.drawPixel(p.first, p.second, TFT_WHITE); // 绘制雪花
        }

        drawAdvancedCommonElements(); // 绘制高级通用UI元素

        menuSprite.setTextFont(1);         // 设置字体
        menuSprite.setTextDatum(MC_DATUM); // 设置文本对齐方式为居中
        char timeStr[10];                  // 时间字符串缓冲区
        // 格式化时间为 HH:MM:SS
        sprintf(timeStr, "%02d:%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
        menuSprite.setTextSize(4);         // 设置时间字体大小
        menuSprite.setTextColor(TIME_MAIN_COLOR, TFT_BLACK); // 设置时间颜色

        int timeWidth = menuSprite.textWidth(timeStr); // 获取时间字符串的宽度
        int timeHeight = menuSprite.fontHeight();     // 获取字体高度
        int timeX = tft.width() / 2;                  // 时间X坐标（居中）
        int timeY = (tft.height() - timeHeight) / 2 + 10; // 时间Y坐标（居中偏下）
        menuSprite.drawString(timeStr, timeX, timeY); // 绘制时间字符串

        // 绘制十分之一秒数字
        int tenth = (millis() % 1000) / 100; // 获取十分之一秒
        menuSprite.setTextSize(1);         // 设置字体大小
        menuSprite.drawString(String(tenth), timeX + timeWidth / 2 + 10, timeY + 10); // 绘制十分之一秒数字

        menuSprite.pushSprite(0, 0); // 将屏幕缓冲区内容推送到TFT屏幕显示
        vTaskDelay(pdMS_TO_TICKS(30)); // 短暂延时，控制动画速度和CPU占用
    }
}

// --- Waves ---
/**
 * @brief 波浪表盘：显示多条不同颜色和频率的动态波浪线，并显示当前时间。
 */
static void WavesWatchface()
{
    float time = 0; // 动画时间参数，用于控制波浪的动态变化
    while (1)
    {
        // 检查退出子菜单标志，如果为真则退出当前表盘
        if (exitSubMenu)
        {
            exitSubMenu = false; // 重置标志
            // 如果有整点报时音乐正在播放，则停止它
            if (g_hourlyMusicTaskHandle != NULL)
            {
                vTaskDelete(g_hourlyMusicTaskHandle);
                g_hourlyMusicTaskHandle = NULL;
                noTone(BUZZER_PIN);
                stopBuzzerTask = true;
            }
            return; // 退出表盘函数
        }
        // 如果闹钟正在响铃，则退出表盘，让闹钟优先处理
        if (g_alarm_is_ringing) { return; }
        // 处理周期性数据同步（天气和NTP时间）
        handlePeriodicSync();
        // 处理整点报时逻辑
        handleHourlyChime();
        // 检测按钮点击，如果点击则退出当前表盘
        if (readButton())
        {
            // 如果有整点报时音乐正在播放，则停止它
            if (g_hourlyMusicTaskHandle != NULL)
            {
                vTaskDelete(g_hourlyMusicTaskHandle);
                g_hourlyMusicTaskHandle = NULL;
                noTone(BUZZER_PIN);
                stopBuzzerTask = true;
            }
            tone(BUZZER_PIN, 1500, 50); // 播放按键音
            menuSprite.setTextFont(MENU_FONT); // 恢复菜单字体设置
            return; // 退出表盘函数
        }

        getLocalTime(&timeinfo); // 获取当前本地时间
        menuSprite.fillSprite(TFT_BLACK); // 清空屏幕缓冲区，填充黑色背景

        // 绘制三条不同参数的波浪线
        for (int x = 0; x < tft.width(); x++)
        {
            // 第一条波浪线 (青色)
            menuSprite.drawPixel(x, sin((float) x / 20 + time) * 20 + 60, TFT_CYAN);
            // 第二条波浪线 (洋红色)
            menuSprite.drawPixel(x, cos((float) x / 15 + time) * 20 + 120, TFT_MAGENTA);
            // 第三条波浪线 (黄色)
            menuSprite.drawPixel(x, sin((float) x / 10 + time * 2) * 20 + 180, TFT_YELLOW);
        }
        time += 0.1; // 更新时间参数，使波浪线动态变化

        drawAdvancedCommonElements(); // 绘制高级通用UI元素

        menuSprite.setTextFont(1);         // 设置字体
        menuSprite.setTextDatum(MC_DATUM); // 设置文本对齐方式为居中
        char timeStr[10];                  // 时间字符串缓冲区
        // 格式化时间为 HH:MM:SS
        sprintf(timeStr, "%02d:%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
        menuSprite.setTextSize(4);         // 设置时间字体大小
        menuSprite.setTextColor(TIME_MAIN_COLOR, TFT_BLACK); // 设置时间颜色

        int timeWidth = menuSprite.textWidth(timeStr); // 获取时间字符串的宽度
        int timeHeight = menuSprite.fontHeight();     // 获取字体高度
        int timeX = tft.width() / 2;                  // 时间X坐标（居中）
        int timeY = (tft.height() - timeHeight) / 2 + 10; // 时间Y坐标（居中偏下）
        menuSprite.drawString(timeStr, timeX, timeY); // 绘制时间字符串

        // 绘制十分之一秒数字
        int tenth = (millis() % 1000) / 100; // 获取十分之一秒
        menuSprite.setTextSize(1);         // 设置字体大小
        menuSprite.drawString(String(tenth), timeX + timeWidth / 2 + 10, timeY + 10); // 绘制十分之一秒数字

        menuSprite.pushSprite(0, 0); // 将屏幕缓冲区内容推送到TFT屏幕显示
        vTaskDelay(pdMS_TO_TICKS(20)); // 短暂延时，控制动画速度和CPU占用
    }
}

// --- Neno (Neon Lines) ---
/**
 * @brief 霓虹灯线条表盘：显示动态变化的霓虹灯效果线条，并显示当前时间。
 */
static void NenoWatchface()
{
    float time = 0; // 动画时间参数，用于控制线条的动态变化
    while (1)
    {
        // 检查退出子菜单标志，如果为真则退出当前表盘
        if (exitSubMenu)
        {
            exitSubMenu = false; // 重置标志
            // 如果有整点报时音乐正在播放，则停止它
            if (g_hourlyMusicTaskHandle != NULL)
            {
                vTaskDelete(g_hourlyMusicTaskHandle);
                g_hourlyMusicTaskHandle = NULL;
                noTone(BUZZER_PIN);
                stopBuzzerTask = true;
            }
            return; // 退出表盘函数
        }
        // 如果闹钟正在响铃，则退出表盘，让闹钟优先处理
        if (g_alarm_is_ringing) { return; }
        // 处理周期性数据同步（天气和NTP时间）
        handlePeriodicSync();
        // 处理整点报时逻辑
        handleHourlyChime();
        // 检测按钮点击，如果点击则退出当前表盘
        if (readButton())
        {
            // 如果有整点报时音乐正在播放，则停止它
            if (g_hourlyMusicTaskHandle != NULL)
            {
                vTaskDelete(g_hourlyMusicTaskHandle);
                g_hourlyMusicTaskHandle = NULL;
                noTone(BUZZER_PIN);
                stopBuzzerTask = true;
            }
            tone(BUZZER_PIN, 1500, 50); // 播放按键音
            menuSprite.setTextFont(MENU_FONT); // 恢复菜单字体设置
            return; // 退出表盘函数
        }

        getLocalTime(&timeinfo); // 获取当前本地时间
        menuSprite.fillSprite(TFT_BLACK); // 清空屏幕缓冲区，填充黑色背景

        // 计算两条动态线条的端点坐标
        float x1 = tft.width() / 2 + sin(time) * 50, y1 = tft.height() / 2 + cos(time) * 50;
        float x2 = tft.width() / 2 + sin(time + PI) * 50, y2 = tft.height() / 2 + cos(time + PI) * 50;
        // 绘制第一条红色线条
        menuSprite.drawLine(x1, y1, x2, y2, TFT_RED);
        // 绘制第二条蓝色线条（镜像效果）
        menuSprite.drawLine(tft.width() - x1, y1, tft.width() - x2, y2, TFT_BLUE);
        time += 0.05; // 更新时间参数，使线条动态变化

        drawAdvancedCommonElements(); // 绘制高级通用UI元素

        menuSprite.setTextFont(1);         // 设置字体
        menuSprite.setTextDatum(MC_DATUM); // 设置文本对齐方式为居中
        char timeStr[10];                  // 时间字符串缓冲区
        // 格式化时间为 HH:MM:SS
        sprintf(timeStr, "%02d:%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
        menuSprite.setTextSize(4);         // 设置时间字体大小
        menuSprite.setTextColor(TIME_MAIN_COLOR, TFT_BLACK); // 设置时间颜色

        int timeWidth = menuSprite.textWidth(timeStr); // 获取时间字符串的宽度
        int timeHeight = menuSprite.fontHeight();     // 获取字体高度
        int timeX = tft.width() / 2;                  // 时间X坐标（居中）
        int timeY = (tft.height() - timeHeight) / 2 + 10; // 时间Y坐标（居中偏下）
        menuSprite.drawString(timeStr, timeX, timeY); // 绘制时间字符串

        // 绘制十分之一秒数字
        int tenth = (millis() % 1000) / 100; // 获取十分之一秒
        menuSprite.setTextSize(1);         // 设置字体大小
        menuSprite.drawString(String(tenth), timeX + timeWidth / 2 + 10, timeY + 10); // 绘制十分之一秒数字

        menuSprite.pushSprite(0, 0); // 将屏幕缓冲区内容推送到TFT屏幕显示
        vTaskDelay(pdMS_TO_TICKS(20)); // 短暂延时，控制动画速度和CPU占用
    }
}

// --- Bouncing Balls ---
#define BALL_COUNT 5 // 弹跳球的数量
/**
 * @brief 定义弹跳球的数据结构
 */
struct Ball { float x, y, vx, vy; uint16_t color; };
std::vector<Ball> balls(BALL_COUNT); // 存储所有弹跳球的向量

/**
 * @brief 弹跳球表盘：显示多个在屏幕上弹跳的彩色球，并显示当前时间。
 */
static void BallsWatchface()
{
    util_randomSeed(millis()); // 使用当前时间作为随机数种子
    // 初始化所有弹跳球的随机位置、速度和颜色
    for (auto &b : balls)
    {
        b.x = util_random_range(10, tft.width() - 10); b.y = util_random_range(10, tft.height() - 10);
        b.vx = util_random_range(-3, 3); b.vy = util_random_range(-3, 3);
        b.color = util_random(0xFFFF); // 随机颜色
    }
    while (1)
    {
        // 检查退出子菜单标志，如果为真则退出当前表盘
        if (exitSubMenu)
        {
            exitSubMenu = false; // 重置标志
            // 如果有整点报时音乐正在播放，则停止它
            if (g_hourlyMusicTaskHandle != NULL)
            {
                vTaskDelete(g_hourlyMusicTaskHandle);
                g_hourlyMusicTaskHandle = NULL;
                noTone(BUZZER_PIN);
                stopBuzzerTask = true;
            }
            return; // 退出表盘函数
        }
        // 如果闹钟正在响铃，则退出表盘，让闹钟优先处理
        if (g_alarm_is_ringing) { return; }
        // 处理周期性数据同步（天气和NTP时间）
        handlePeriodicSync();
        // 处理整点报时逻辑
        handleHourlyChime();
        // 检测按钮点击，如果点击则退出当前表盘
        if (readButton())
        {
            // 如果有整点报时音乐正在播放，则停止它
            if (g_hourlyMusicTaskHandle != NULL)
            {
                vTaskDelete(g_hourlyMusicTaskHandle);
                g_hourlyMusicTaskHandle = NULL;
                noTone(BUZZER_PIN);
                stopBuzzerTask = true;
            }
            tone(BUZZER_PIN, 1500, 50); // 播放按键音
            menuSprite.setTextFont(MENU_FONT); // 恢复菜单字体设置
            return; // 退出表盘函数
        }

        getLocalTime(&timeinfo); // 获取当前本地时间
        menuSprite.fillSprite(TFT_BLACK); // 清空屏幕缓冲区，填充黑色背景

        // 更新并绘制所有弹跳球
        for (auto &b : balls)
        {
            b.x += b.vx; b.y += b.vy; // 更新球的位置
            // 碰撞检测和反弹
            if (b.x < 5 || b.x > tft.width() - 5) b.vx *= -1;
            if (b.y < 5 || b.y > tft.height() - 5) b.vy *= -1;
            menuSprite.fillCircle(b.x, b.y, 5, b.color); // 绘制球
        }

        drawAdvancedCommonElements(); // 绘制高级通用UI元素

        menuSprite.setTextFont(1);         // 设置字体
        menuSprite.setTextDatum(MC_DATUM); // 设置文本对齐方式为居中
        char timeStr[10];                  // 时间字符串缓冲区
        // 格式化时间为 HH:MM:SS
        sprintf(timeStr, "%02d:%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
        menuSprite.setTextSize(4);         // 设置时间字体大小
        menuSprite.setTextColor(TIME_MAIN_COLOR, TFT_BLACK); // 设置时间颜色

        int timeWidth = menuSprite.textWidth(timeStr); // 获取时间字符串的宽度
        int timeHeight = menuSprite.fontHeight();     // 获取字体高度
        int timeX = tft.width() / 2;                  // 时间X坐标（居中）
        int timeY = (tft.height() - timeHeight) / 2 + 10; // 时间Y坐标（居中偏下）
        menuSprite.drawString(timeStr, timeX, timeY); // 绘制时间字符串

        // 绘制十分之一秒数字
        int tenth = (millis() % 1000) / 100; // 获取十分之一秒
        menuSprite.setTextSize(1);         // 设置字体大小
        menuSprite.drawString(String(tenth), timeX + timeWidth / 2 + 10, timeY + 10); // 绘制十分之一秒数字

        menuSprite.pushSprite(0, 0); // 将屏幕缓冲区内容推送到TFT屏幕显示
        vTaskDelay(pdMS_TO_TICKS(20)); // 短暂延时，控制动画速度和CPU占用
    }
}

// --- Sand Box ---
#define SAND_WIDTH 120  // 沙盒模拟区域的宽度
#define SAND_HEIGHT 120 // 沙盒模拟区域的高度
byte sand_grid[SAND_WIDTH][SAND_HEIGHT] = { 0 }; // 沙盒网格，0表示空，1表示沙粒

/**
 * @brief 沙盒表盘：模拟沙粒下落堆积的效果，并显示当前时间。
 */
static void SandBoxWatchface()
{
    while (1)
    {
        // 检查退出子菜单标志，如果为真则退出当前表盘
        if (exitSubMenu)
        {
            exitSubMenu = false; // 重置标志
            // 如果有整点报时音乐正在播放，则停止它
            if (g_hourlyMusicTaskHandle != NULL)
            {
                vTaskDelete(g_hourlyMusicTaskHandle);
                g_hourlyMusicTaskHandle = NULL;
                noTone(BUZZER_PIN);
                stopBuzzerTask = true;
            }
            return; // 退出表盘函数
        }
        // 如果闹钟正在响铃，则退出表盘，让闹钟优先处理
        if (g_alarm_is_ringing) { return; }
        // 处理周期性数据同步（天气和NTP时间）
        handlePeriodicSync();
        // 处理整点报时逻辑
        handleHourlyChime();
        // 检测按钮点击，如果点击则退出当前表盘
        if (readButton())
        {
            // 如果有整点报时音乐正在播放，则停止它
            if (g_hourlyMusicTaskHandle != NULL)
            {
                vTaskDelete(g_hourlyMusicTaskHandle);
                g_hourlyMusicTaskHandle = NULL;
                noTone(BUZZER_PIN);
                stopBuzzerTask = true;
            }
            tone(BUZZER_PIN, 1500, 50); // 播放按键音
            menuSprite.setTextFont(MENU_FONT); // 恢复菜单字体设置
            return; // 退出表盘函数
        }

        getLocalTime(&timeinfo); // 获取当前本地时间

        // 随机在顶部生成新的沙粒
        if (util_random(100) < 20) sand_grid[util_random(SAND_WIDTH)][0] = 1;
        menuSprite.fillSprite(TFT_BLACK); // 清空屏幕缓冲区，填充黑色背景

        // 模拟沙粒下落逻辑
        for (int y = SAND_HEIGHT - 2; y >= 0; y--) // 从倒数第二行开始向上遍历
        {
            for (int x = 0; x < SAND_WIDTH; x++) // 遍历每一列
            {
                if (sand_grid[x][y] == 1) // 如果当前位置有沙粒
                {
                    if (sand_grid[x][y + 1] == 0) { sand_grid[x][y] = 0; sand_grid[x][y + 1] = 1; } // 如果正下方为空，则沙粒下落
                    else if (x > 0 && sand_grid[x - 1][y + 1] == 0) { sand_grid[x][y] = 0; sand_grid[x - 1][y + 1] = 1; } // 如果左下方为空，则沙粒向左下落
                    else if (x < SAND_WIDTH - 1 && sand_grid[x + 1][y + 1] == 0) { sand_grid[x][y] = 0; sand_grid[x + 1][y + 1] = 1; } // 如果右下方为空，则沙粒向右下落
                }
            }
        }
        // 绘制沙粒
        for (int y = 0; y < SAND_HEIGHT; y++)
        {
            for (int x = 0; x < SAND_WIDTH; x++)
            {
                if (sand_grid[x][y] == 1) menuSprite.drawPixel(x * 2, y * 2, TFT_YELLOW); // 将沙粒绘制为黄色像素
            }
        }

        drawAdvancedCommonElements(); // 绘制高级通用UI元素

        menuSprite.setTextFont(1);         // 设置字体
        menuSprite.setTextDatum(MC_DATUM); // 设置文本对齐方式为居中
        char timeStr[10];                  // 时间字符串缓冲区
        // 格式化时间为 HH:MM:SS
        sprintf(timeStr, "%02d:%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
        menuSprite.setTextSize(4);         // 设置时间字体大小
        menuSprite.setTextColor(TIME_MAIN_COLOR, TFT_BLACK); // 设置时间颜色

        int timeWidth = menuSprite.textWidth(timeStr); // 获取时间字符串的宽度
        int timeHeight = menuSprite.fontHeight();     // 获取字体高度
        int timeX = tft.width() / 2;                  // 时间X坐标（居中）
        int timeY = (tft.height() - timeHeight) / 2 + 10; // 时间Y坐标（居中偏下）
        menuSprite.drawString(timeStr, timeX, timeY); // 绘制时间字符串

        // 绘制十分之一秒数字
        int tenth = (millis() % 1000) / 100; // 获取十分之一秒
        menuSprite.setTextSize(1);         // 设置字体大小
        menuSprite.drawString(String(tenth), timeX + timeWidth / 2 + 10, timeY + 10); // 绘制十分之一秒数字

        menuSprite.pushSprite(0, 0); // 将屏幕缓冲区内容推送到TFT屏幕显示
        vTaskDelay(pdMS_TO_TICKS(10)); // 短暂延时，控制动画速度和CPU占用
    }
}

// --- Progress Bar Watchface ---
#define PB_DATA_X 3 // 进度条的X坐标
#define PB_BAR_WIDTH 180 // 进度条的宽度
#define PB_BAR_HEIGHT 10 // 进度条的高度
#define PB_PERCENTAGE_TEXT_X (PB_DATA_X + PB_BAR_WIDTH + 10) // 百分比文本的X坐标

/**
 * @brief 进度条表盘：显示当前时间，并以进度条形式展示一天、一小时和一分钟的进度。
 */
static void ProgressBarWatchface()
{
    while (1)
    {
        // 检查退出子菜单标志，如果为真则退出当前表盘
        if (exitSubMenu)
        {
            exitSubMenu = false; // 重置标志
            // 如果有整点报时音乐正在播放，则停止它
            if (g_hourlyMusicTaskHandle != NULL)
            {
                vTaskDelete(g_hourlyMusicTaskHandle);
                g_hourlyMusicTaskHandle = NULL;
                noTone(BUZZER_PIN);
                stopBuzzerTask = true;
            }
            return; // 退出表盘函数
        }
        // 如果闹钟正在响铃，则退出表盘，让闹钟优先处理
        if (g_alarm_is_ringing) { return; }
        // 处理周期性数据同步（天气和NTP时间）
        handlePeriodicSync();
        // 处理整点报时逻辑
        handleHourlyChime();
        // 检测按钮点击，如果点击则退出当前表盘
        if (readButton())
        {
            // 如果有整点报时音乐正在播放，则停止它
            if (g_hourlyMusicTaskHandle != NULL)
            {
                vTaskDelete(g_hourlyMusicTaskHandle);
                g_hourlyMusicTaskHandle = NULL;
                noTone(BUZZER_PIN);
                stopBuzzerTask = true;
            }
            tone(BUZZER_PIN, 1500, 50); // 播放按键音
            menuSprite.setTextFont(MENU_FONT); // 恢复菜单字体设置
            return; // 退出表盘函数
        }

        getLocalTime(&timeinfo); // 获取当前本地时间
        menuSprite.fillSprite(TFT_BLACK); // 清空屏幕缓冲区，填充黑色背景
        drawCommonElements();             // 绘制通用UI元素

        menuSprite.setTextFont(1);         // 设置字体
        // --- 添加时间显示 ---
        menuSprite.setTextDatum(MC_DATUM); // 设置文本对齐方式为居中
        char timeStr[10];                  // 时间字符串缓冲区
        // 格式化时间为 HH:MM:SS
        sprintf(timeStr, "%02d:%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
        menuSprite.setTextSize(4);         // 设置时间字体大小
        menuSprite.setTextColor(TIME_MAIN_COLOR, TFT_BLACK); // 设置时间颜色

        int timeWidth = menuSprite.textWidth(timeStr); // 获取时间字符串的宽度
        int timeX = tft.width() / 2;                  // 时间X坐标（居中）
        int timeY = 125;                              // 时间Y坐标
        menuSprite.drawString(timeStr, timeX, timeY); // 绘制时间字符串

        // 绘制十分之一秒数字
        int tenth = (millis() % 1000) / 100; // 获取十分之一秒
        menuSprite.setTextSize(1);         // 设置字体大小
        menuSprite.drawString(String(tenth), timeX + timeWidth / 2 + 10, timeY + 10); // 绘制十分之一秒数字

        // --- 进度条 ---
        menuSprite.setTextDatum(TL_DATUM); // 设置文本对齐方式为左上角
        char buf[32];                      // 缓冲区用于格式化文本
        menuSprite.setTextSize(1);         // 设置字体大小
        menuSprite.setTextColor(TFT_CYAN, TFT_BLACK); // 设置文本颜色

        const int bar_y_start = 140; // 进度条起始Y坐标
        const int bar_y_spacing = 15; // 进度条之间的垂直间距

        // 一天进度条
        long seconds_in_day = timeinfo.tm_hour * 3600 + timeinfo.tm_min * 60 + timeinfo.tm_sec; // 计算一天中的总秒数
        float day_progress = (float) seconds_in_day / (24.0 * 3600.0 - 1.0); // 计算一天进度百分比
        menuSprite.fillRect(PB_DATA_X, bar_y_start, PB_BAR_WIDTH, PB_BAR_HEIGHT, TFT_DARKGREY); // 绘制背景条
        menuSprite.fillRect(PB_DATA_X, bar_y_start, (int) (PB_BAR_WIDTH * day_progress), PB_BAR_HEIGHT, TFT_RED); // 绘制进度条
        sprintf(buf, "Day: %.0f%%", day_progress * 100); // 格式化百分比文本
        menuSprite.drawString(buf, PB_PERCENTAGE_TEXT_X, bar_y_start); // 绘制百分比文本

        // 一小时进度条
        float hour_progress = (float) (timeinfo.tm_min * 60 + timeinfo.tm_sec) / 3599.0; // 计算一小时进度百分比
        menuSprite.fillRect(PB_DATA_X, bar_y_start + bar_y_spacing, PB_BAR_WIDTH, PB_BAR_HEIGHT, TFT_DARKGREY); // 绘制背景条
        menuSprite.fillRect(PB_DATA_X, bar_y_start + bar_y_spacing, (int) (PB_BAR_WIDTH * hour_progress), PB_BAR_HEIGHT, TFT_BLUE); // 绘制进度条
        sprintf(buf, "Hor: %.0f%%", hour_progress * 100); // 格式化百分比文本
        menuSprite.drawString(buf, PB_PERCENTAGE_TEXT_X, bar_y_start + bar_y_spacing); // 绘制百分比文本

        // 一分钟进度条
        float minute_progress = (float) timeinfo.tm_sec / 59.0; // 计算一分钟进度百分比
        menuSprite.fillRect(PB_DATA_X, bar_y_start + bar_y_spacing * 2, PB_BAR_WIDTH, PB_BAR_HEIGHT, TFT_DARKGREY); // 绘制背景条
        menuSprite.fillRect(PB_DATA_X, bar_y_start + bar_y_spacing * 2, (int) (PB_BAR_WIDTH * minute_progress), PB_BAR_HEIGHT, TFT_GREEN); // 绘制进度条
        sprintf(buf, "Min: %.0f%%", minute_progress * 100); // 格式化百分比文本
        menuSprite.drawString(buf, PB_PERCENTAGE_TEXT_X, bar_y_start + bar_y_spacing * 2); // 绘制百分比文本

        menuSprite.pushSprite(0, 0); // 将屏幕缓冲区内容推送到TFT屏幕显示
        vTaskDelay(pdMS_TO_TICKS(100)); // 短暂延时
    }
}


#define HOUR_FONT 7        // 小时字体编号
#define HOUR_SIZE 1        // 小时字体大小倍数
#define MINUTE_FONT 7      // 分钟字体编号
#define MINUTE_SIZE 1      // 分钟字体大小倍数
#define SECOND_FONT 7      // 秒钟字体编号
#define SECOND_SIZE 1      // 秒钟字体大小倍数
#define TENTH_FONT 4       // 0.1秒字体编号
#define TENTH_SIZE 1       // 0.1秒字体大小倍数
#define COLON_FONT 7       // 冒号字体编号
#define COLON_SIZE 1       // 冒号字体大小倍数
/**
 * @brief 分段矢量滚动表盘：以分段滚动动画效果显示时间（时、分、秒）。
 * @details 类似于VectorScrollWatchface，但可能针对特定字体或显示方式进行了优化。
 */
static void VectorScrollWatchface_SEG()
{
    static time_s last_time;     // 存储上一次的时间，用于检测时间变化
    static TickerData tickers[6]; // 存储6个数字（HHMMSS）的滚动数据
    static bool firstRun = true;  // 首次运行标志

    // 静态变量用于存储计算出的位置，避免每次循环重复计算
    static int y_main;
    static int colon1_x, colon2_x;
    static int colon_y;

    while (1)
    {
        // 检查退出子菜单标志，如果为真则退出当前表盘
        if (exitSubMenu)
        {
            exitSubMenu = false; // 重置标志
            // 如果有整点报时音乐正在播放，则停止它
            if (g_hourlyMusicTaskHandle != NULL)
            {
                vTaskDelete(g_hourlyMusicTaskHandle);
                g_hourlyMusicTaskHandle = NULL;
                noTone(BUZZER_PIN);
                stopBuzzerTask = true;
            }
            return; // 退出表盘函数
        }
        // 如果闹钟正在响铃，则退出表盘，让闹钟优先处理
        if (g_alarm_is_ringing) { return; }
        // 处理周期性数据同步（天气和NTP时间）
        handlePeriodicSync();
        // 处理整点报时逻辑
        handleHourlyChime();
        // 检测按钮点击，如果点击则退出当前表盘
        if (readButton())
        {
            // 如果有整点报时音乐正在播放，则停止它
            if (g_hourlyMusicTaskHandle != NULL)
            {
                vTaskDelete(g_hourlyMusicTaskHandle);
                g_hourlyMusicTaskHandle = NULL;
                noTone(BUZZER_PIN);
                stopBuzzerTask = true;
            }
            tone(BUZZER_PIN, 1500, 50); // 播放按键音
            menuSprite.setTextFont(MENU_FONT); // 恢复菜单字体设置
            return; // 退出表盘函数
        }

        getLocalTime(&timeinfo); // 获取当前本地时间
        g_watchface_timeDate.time.hour = timeinfo.tm_hour;
        g_watchface_timeDate.time.mins = timeinfo.tm_min;
        g_watchface_timeDate.time.secs = timeinfo.tm_sec;

        // 首次运行时计算并初始化数字和冒号的位置
        if (firstRun)
        {
            firstRun = false;
            // 计算数字的宽度和高度
            int num_w_hour = menuSprite.textWidth("88", HOUR_FONT);
            int num_h_hour = menuSprite.fontHeight(HOUR_FONT);
            int num_w_min = menuSprite.textWidth("88", MINUTE_FONT);
            int num_h_min = menuSprite.fontHeight(MINUTE_FONT);
            int num_w_sec = menuSprite.textWidth("88", SECOND_FONT);
            int num_h_sec = menuSprite.fontHeight(SECOND_FONT);
            int colon_w_val = menuSprite.textWidth(":", COLON_FONT);
            int colon_h_val = menuSprite.fontHeight(COLON_FONT);

            // 计算主数字（小时和分钟）的Y坐标
            y_main = (tft.height() - num_h_hour) / 2;

            // 计算冒号的X和Y坐标
            colon1_x = tft.width() / 2 - num_w_hour - colon_w_val / 2;
            colon2_x = tft.width() / 2 + num_w_min + colon_w_val / 2;
            colon_y = y_main + (num_h_hour - colon_h_val) / 2;

            // 初始化TickerData结构体
            // 小时十位
            tickers[0] = { (int16_t)(tft.width() / 2 - num_w_hour * 2 - colon_w_val), (int16_t)y_main, (int16_t)num_w_hour, (int16_t)num_h_hour, (uint8_t)(last_time.hour / 10), 2, false, 0 };
            // 小时个位
            tickers[1] = { (int16_t)(tft.width() / 2 - num_w_hour - colon_w_val), (int16_t)y_main, (int16_t)num_w_hour, (int16_t)num_h_hour, (uint8_t)(last_time.hour % 10), 9, false, 0 };
            // 分钟十位
            tickers[2] = { (int16_t)(tft.width() / 2 + colon_w_val), (int16_t)y_main, (int16_t)num_w_min, (int16_t)num_h_min, (uint8_t)(last_time.mins / 10), 5, false, 0 };
            // 分钟个位
            tickers[3] = { (int16_t)(tft.width() / 2 + num_w_min + colon_w_val), (int16_t)y_main, (int16_t)num_w_min, (int16_t)num_h_min, (uint8_t)(last_time.mins % 10), 9, false, 0 };
            // 秒数十位
            tickers[4] = { (int16_t)(tft.width() / 2 - num_w_sec - colon_w_val), (int16_t)(y_main + num_h_hour + 5), (int16_t)num_w_sec, (int16_t)num_h_sec, (uint8_t)(last_time.secs / 10), 5, false, 0 };
            // 秒数个位
            tickers[5] = { (int16_t)(tft.width() / 2 + colon_w_val), (int16_t)(y_main + num_h_hour + 5), (int16_t)num_w_sec, (int16_t)num_h_sec, (uint8_t)(last_time.secs % 10), 9, false, 0 };
        }

        // 检测时间变化，并设置对应数字的滚动标志和初始Y位置
        if (g_watchface_timeDate.time.hour / 10 != last_time.hour / 10) { tickers[0].moving = true; tickers[0].y_pos = 0; tickers[0].val = g_watchface_timeDate.time.hour / 10; }
        if (g_watchface_timeDate.time.hour % 10 != last_time.hour % 10) { tickers[1].moving = true; tickers[1].y_pos = 0; tickers[1].val = g_watchface_timeDate.time.hour % 10; }
        if (g_watchface_timeDate.time.mins / 10 != last_time.mins / 10) { tickers[2].moving = true; tickers[2].y_pos = 0; tickers[2].val = g_watchface_timeDate.time.mins / 10; }
        if (g_watchface_timeDate.time.mins % 10 != last_time.mins % 10) { tickers[3].moving = true; tickers[3].y_pos = 0; tickers[3].val = g_watchface_timeDate.time.mins % 10; }
        if (g_watchface_timeDate.time.secs / 10 != last_time.secs / 10) { tickers[4].moving = true; tickers[4].y_pos = 0; tickers[4].val = g_watchface_timeDate.time.secs / 10; }
        if (g_watchface_timeDate.time.secs % 10 != last_time.secs % 10) { tickers[5].moving = true; tickers[5].y_pos = 0; tickers[5].val = g_watchface_timeDate.time.secs % 10; }
        last_time = g_watchface_timeDate.time; // 更新上一次的时间

        menuSprite.fillSprite(TFT_BLACK); // 清空屏幕缓冲区，填充黑色背景
        drawAdvancedCommonElements();     // 绘制高级通用UI元素
        menuSprite.setTextDatum(TL_DATUM); // 设置文本对齐方式为左上角

        // 更新并绘制所有数字的滚动动画
        for (int i = 0; i < 6; ++i)
        {
            int current_h = tickers[i].h; // 获取当前数字的高度
            menuSprite.setTextFont((i < 4) ? HOUR_FONT : SECOND_FONT); // 设置字体（小时/分钟使用HOUR_FONT，秒使用SECOND_FONT）
            menuSprite.setTextSize((i < 4) ? HOUR_SIZE : SECOND_SIZE); // 设置字体大小

            if (tickers[i].moving)
            {
                int16_t *yPos = &tickers[i].y_pos; // 获取当前数字的Y位置指针
                // 根据Y位置调整滚动速度，实现加速和减速效果
                if (*yPos <= 3) (*yPos)++;
                else if (*yPos <= 6) (*yPos) += 3;
                else if (*yPos <= 16) (*yPos) += 5;
                else if (*yPos <= 22) (*yPos) += 3;
                else if (*yPos <= current_h + TICKER_GAP) (*yPos)++;

                // 如果滚动动画完成，重置标志和Y位置
                if (*yPos > current_h + TICKER_GAP)
                {
                    tickers[i].moving = false;
                    tickers[i].y_pos = 0;
                }
            }
            drawVectorScrollTickerNum(&tickers[i]); // 绘制滚动数字
        }

        // 绘制闪烁的冒号
        if (millis() % 1000 < 500) // 每500毫秒闪烁一次
        {
            menuSprite.setTextFont(COLON_FONT); // 冒号字体
            menuSprite.setTextSize(COLON_SIZE); // 冒号字体大小
            menuSprite.setTextColor(TIME_MAIN_COLOR, TFT_BLACK); // 冒号颜色
            menuSprite.drawString(":", colon1_x, colon_y); // 绘制第一个冒号
            menuSprite.drawString(":", colon2_x, colon_y); // 绘制第二个冒号
        }

        // 绘制0.1秒数字
        int tenth = (millis() % 1000) / 100; // 获取十分之一秒
        menuSprite.setTextFont(TENTH_FONT); // 字体
        menuSprite.setTextSize(TENTH_SIZE); // 字体大小
        menuSprite.setTextColor(TIME_TENTH_COLOR, TFT_BLACK); // 颜色
        menuSprite.setTextDatum(TL_DATUM); // 左上角对齐
        int x_pos = tickers[5].x + tickers[5].w; // 0.1秒数字的X位置
        int y_pos = tickers[5].y + tickers[5].h - 8; // 0.1秒数字的Y位置
        menuSprite.drawString(String(tenth), x_pos, y_pos); // 绘制0.1秒数字

        menuSprite.pushSprite(0, 0); // 将屏幕缓冲区内容推送到TFT屏幕显示
        vTaskDelay(pdMS_TO_TICKS(20)); // 短暂延时，控制动画速度和CPU占用
    }
}

static void VectorScanWatchface_SEG()
{
    static time_s last_time;     // 存储上一次的时间，用于检测时间变化
    static TickerData tickers[6]; // 存储6个数字（HHMMSS）的扫描数据
    static bool firstRun = true;  // 首次运行标志

    // 静态变量用于存储计算出的位置，避免每次循环重复计算
    static int y_main;
    static int colon1_x, colon2_x;
    static int colon_y;

    while (1)
    {
        // 检查退出子菜单标志，如果为真则退出当前表盘
        if (exitSubMenu)
        {
            exitSubMenu = false; // 重置标志
            // 如果有整点报时音乐正在播放，则停止它
            if (g_hourlyMusicTaskHandle != NULL)
            {
                vTaskDelete(g_hourlyMusicTaskHandle);
                g_hourlyMusicTaskHandle = NULL;
                noTone(BUZZER_PIN);
                stopBuzzerTask = true;
            }
            return; // 退出表盘函数
        }
        // 如果闹钟正在响铃，则退出表盘，让闹钟优先处理
        if (g_alarm_is_ringing) { return; }
        // 处理周期性数据同步（天气和NTP时间）
        handlePeriodicSync();
        // 处理整点报时逻辑
        handleHourlyChime();
        // 检测按钮点击，如果点击则退出当前表盘
        if (readButton())
        {
            // 如果有整点报时音乐正在播放，则停止它
            if (g_hourlyMusicTaskHandle != NULL)
            {
                vTaskDelete(g_hourlyMusicTaskHandle);
                g_hourlyMusicTaskHandle = NULL;
                noTone(BUZZER_PIN);
                stopBuzzerTask = true;
            }
            tone(BUZZER_PIN, 1500, 50); // 播放按键音
            menuSprite.setTextFont(MENU_FONT); // 恢复菜单字体设置
            return; // 退出表盘函数
        }

        getLocalTime(&timeinfo); // 获取当前本地时间
        g_watchface_timeDate.time.hour = timeinfo.tm_hour;
        g_watchface_timeDate.time.mins = timeinfo.tm_min;
        g_watchface_timeDate.time.secs = timeinfo.tm_sec;

        // 首次运行时计算并初始化数字和冒号的位置
        if (firstRun)
        {
            firstRun = false;
            // 计算数字的宽度和高度
            int num_w_hour = menuSprite.textWidth("88", HOUR_FONT);
            int num_h_hour = menuSprite.fontHeight(HOUR_FONT);
            int num_w_min = menuSprite.textWidth("88", MINUTE_FONT);
            int num_h_min = menuSprite.fontHeight(MINUTE_FONT);
            int num_w_sec = menuSprite.textWidth("88", SECOND_FONT);
            int num_h_sec = menuSprite.fontHeight(SECOND_FONT);
            int colon_w_val = menuSprite.textWidth(":", COLON_FONT);
            int colon_h_val = menuSprite.fontHeight(COLON_FONT);

            // 计算主数字（小时和分钟）的Y坐标
            y_main = (tft.height() - num_h_hour) / 2;

            // 计算冒号的X和Y坐标
            colon1_x = tft.width() / 2 - num_w_hour - colon_w_val / 2;
            colon2_x = tft.width() / 2 + num_w_min + colon_w_val / 2;
            colon_y = y_main + (num_h_hour - colon_h_val) / 2;

            // 初始化TickerData结构体
            // 小时十位
            tickers[0] = { (int16_t)(tft.width() / 2 - num_w_hour * 2 - colon_w_val), (int16_t)y_main, (int16_t)num_w_hour, (int16_t)num_h_hour, (uint8_t)(last_time.hour / 10), 2, false, 0 };
            // 小时个位
            tickers[1] = { (int16_t)(tft.width() / 2 - num_w_hour - colon_w_val), (int16_t)y_main, (int16_t)num_w_hour, (int16_t)num_h_hour, (uint8_t)(last_time.hour % 10), 9, false, 0 };
            // 分钟十位
            tickers[2] = { (int16_t)(tft.width() / 2 + colon_w_val), (int16_t)y_main, (int16_t)num_w_min, (int16_t)num_h_min, (uint8_t)(last_time.mins / 10), 5, false, 0 };
            // 分钟个位
            tickers[3] = { (int16_t)(tft.width() / 2 + num_w_min + colon_w_val), (int16_t)y_main, (int16_t)num_w_min, (int16_t)num_h_min, (uint8_t)(last_time.mins % 10), 9, false, 0 };
            // 秒数十位
            tickers[4] = { (int16_t)(tft.width() / 2 - num_w_sec - colon_w_val), (int16_t)(y_main + num_h_hour + 5), (int16_t)num_w_sec, (int16_t)num_h_sec, (uint8_t)(last_time.secs / 10), 5, false, 0 };
            // 秒数个位
            tickers[5] = { (int16_t)(tft.width() / 2 + colon_w_val), (int16_t)(y_main + num_h_hour + 5), (int16_t)num_w_sec, (int16_t)num_h_sec, (uint8_t)(last_time.secs % 10), 9, false, 0 };
        }

        // 检测时间变化，并设置对应数字的扫描标志和初始Y位置
        if (g_watchface_timeDate.time.hour / 10 != last_time.hour / 10) { tickers[0].moving = true; tickers[0].y_pos = 0; tickers[0].val = g_watchface_timeDate.time.hour / 10; }
        if (g_watchface_timeDate.time.hour % 10 != last_time.hour % 10) { tickers[1].moving = true; tickers[1].y_pos = 0; tickers[1].val = g_watchface_timeDate.time.hour % 10; }
        if (g_watchface_timeDate.time.mins / 10 != last_time.mins / 10) { tickers[2].moving = true; tickers[2].y_pos = 0; tickers[2].val = g_watchface_timeDate.time.mins / 10; }
        if (g_watchface_timeDate.time.mins % 10 != last_time.mins % 10) { tickers[3].moving = true; tickers[3].y_pos = 0; tickers[3].val = g_watchface_timeDate.time.mins % 10; }
        if (g_watchface_timeDate.time.secs / 10 != last_time.secs / 10) { tickers[4].moving = true; tickers[4].y_pos = 0; tickers[4].val = g_watchface_timeDate.time.secs / 10; }
        if (g_watchface_timeDate.time.secs % 10 != last_time.secs % 10) { tickers[5].moving = true; tickers[5].y_pos = 0; tickers[5].val = g_watchface_timeDate.time.secs % 10; }
        last_time = g_watchface_timeDate.time; // 更新上一次的时间

        menuSprite.fillSprite(TFT_BLACK); // 清空屏幕缓冲区，填充黑色背景
        drawAdvancedCommonElements();     // 绘制高级通用UI元素
        menuSprite.setTextDatum(TL_DATUM); // 设置文本对齐方式为左上角
        menuSprite.setTextColor(TIME_MAIN_COLOR, TFT_BLACK); // 设置时间颜色

        // 更新并绘制所有数字的扫描动画
        for (int i = 0; i < 6; ++i)
        {
            int current_h = tickers[i].h; // 获取当前数字的高度
            menuSprite.setTextFont((i < 4) ? HOUR_FONT : SECOND_FONT); // 设置字体（小时/分钟使用HOUR_FONT，秒使用SECOND_FONT）
            menuSprite.setTextSize((i < 4) ? HOUR_SIZE : SECOND_SIZE); // 设置字体大小

            if (tickers[i].moving)
            {
                tickers[i].y_pos += 5; // 增加Y位置，模拟扫描
                // 如果扫描动画完成，重置标志
                if (tickers[i].y_pos >= current_h + TICKER_GAP) tickers[i].moving = false;
            }
            drawTickerNum(&tickers[i]); // 绘制扫描数字
        }

        // 绘制闪烁的冒号
        if (millis() % 1000 < 500) // 每500毫秒闪烁一次
        {
            menuSprite.setTextFont(COLON_FONT); // 冒号字体
            menuSprite.setTextSize(COLON_SIZE); // 冒号字体大小
            menuSprite.setTextColor(TIME_MAIN_COLOR, TFT_BLACK); // 冒号颜色
            menuSprite.drawString(":", colon1_x, colon_y); // 绘制第一个冒号
            menuSprite.drawString(":", colon2_x, colon_y); // 绘制第二个冒号
        }

        // 绘制0.1秒数字
        int tenth = (millis() % 1000) / 100; // 获取十分之一秒
        menuSprite.setTextFont(TENTH_FONT); // 字体
        menuSprite.setTextSize(TENTH_SIZE); // 字体大小
        menuSprite.setTextColor(TIME_TENTH_COLOR, TFT_BLACK); // 颜色
        menuSprite.setTextDatum(TL_DATUM); // 左上角对齐
        int x_pos = tickers[5].x + tickers[5].w; // 0.1秒数字的X位置
        int y_pos = tickers[5].y + tickers[5].h - 8; // 0.1秒数字的Y位置
        menuSprite.drawString(String(tenth), x_pos, y_pos); // 绘制0.1秒数字

        menuSprite.pushSprite(0, 0); // 将屏幕缓冲区内容推送到TFT屏幕显示
        vTaskDelay(pdMS_TO_TICKS(20)); // 短暂延时，控制动画速度和CPU占用
    }
}
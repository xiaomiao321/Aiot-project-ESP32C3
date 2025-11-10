#ifndef INTERNET_H
#define INTERNET_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "RotaryEncoder.h" // For encoder functions
#include "System.h" // For tftLog functions

// Structs to hold parsed data from each API

struct SayLoveData {
    String content;
};

struct EverydayEnglishData {
    String content;
    String note;
};

struct FortuneData {
    String sign;
    String description;
    String luckyColor;
    int luckyNumber;
    String message; // For error/info messages like "今天已经抽过签了"
    bool success; // To check if data was successfully retrieved
};

struct ShiciData {
    String content;
    String author;
    String dynasty;
    String title;
    String full_content;
    int popularity;
};

struct DuilianData {
    String content;
};

struct FxRateData {
    String money;
};

struct RandomEnWordData {
    String headWord;
    String tranCn;
    String phrases_en; // Concatenated phrases
    String phrases_cn; // Concatenated phrases
};

struct YiyanData {
    String hitokoto;
};

struct LzmyData {
    String saying;
    String transl;
    String source;
};

struct VerseData {
    String content;
    String source;
    String author;
};

struct TianqishijuData {
    String content;
    String author;
    String source;
};

struct HsjzData {
    String content;
};

struct BrainTeaser {
    String quest;
    String result;
};

struct BrainTeaserData {
    BrainTeaser teasers[3];
    int count = 0;
};

struct HealthTipData {
    String content;
};

struct NewsItem {
    String title;
    String digest;
};

struct NewsData {
    NewsItem items[5];
    int count = 0;
};

struct GitHubRepo {
    String name;
    String description;
    String stars;
};

struct GitHubData {
    GitHubRepo repos[10];
    int count = 0;
};

struct HistoryEvent {
    String title;
    String lsdate;
};

struct HistoryData {
    HistoryEvent events[20];
    int count = 0;
};

struct TenWhyData {
    String title;
    String content;
};

struct WeatherData {
    String FeelsLikeC;
    String temp_C;
    String observation_time;
    String precipMM;
    String weatherDesc;
    String humidity;
    String pressure;
    String visibility;
    String winddir16Point;
    String windspeedKmph;
};

struct StockDayData {
    float close;
    float change_percent;
};

struct StockInfo {
    String symbol;
    StockDayData daily_data[5];
    int count;
};

struct CurrencyData {
    String pair_name;
    float rate;
};

// Global data storage for Internet menu
extern SayLoveData g_say_love_data;
extern EverydayEnglishData g_everyday_english_data;
extern FortuneData g_fortune_data;
extern ShiciData g_shici_data;
extern DuilianData g_duilian_data;
extern FxRateData g_fx_rate_data;
extern RandomEnWordData g_random_en_word_data;
extern YiyanData g_yiyan_data;
extern LzmyData g_lzmy_data;
extern VerseData g_verse_data;
extern TianqishijuData g_tianqishiju_data;
extern HsjzData g_hsjz_data;
extern BrainTeaserData g_brain_teaser_data;
extern HealthTipData g_health_tip_data;
extern NewsData g_news_data;
extern GitHubData g_github_data;
extern HistoryData g_history_data;
extern TenWhyData g_ten_why_data;
extern WeatherData g_weather_data;
extern StockInfo g_stock_data[6];
extern CurrencyData g_currency_data[6];

/**
 * @brief 初始化网络菜单。
 * @details 检查WiFi连接状态，连接成功后，会预加载所有API的数据，
 *          并在屏幕上显示加载状态。
 */
void internet_menu_init();

/**
 * @brief 绘制当前页面的网络数据。
 * @details 根据全局变量 `g_current_internet_page` 的值，
 *          将对应API获取并解析好的数据显示在屏幕上。
 */
void internet_menu_draw();

/**
 * @brief 切换到下一页。
 * @details 将页面索引加一，并使用模运算实现循环切换。
 */
void internet_menu_next_page();

/**
 * @brief 切换到上一页。
 * @details 将页面索引减一，并使用模运算实现循环切换。
 */
void internet_menu_prev_page();

/**
 * @brief 处理返回按钮事件。
 * @return 返回 `true`，表示应当退出当前菜单。
 */
bool internet_menu_back_press();

/**
 * @brief 网络信息浏览功能的主入口函数。
 * @details 负责初始化网络菜单，并进入一个循环，处理用户的旋转和点击输入，
 *          以实现页面切换、数据刷新和退出菜单等功能。
 */
void InternetMenuScreen();

/** @defgroup FetchFunctions API数据获取函数
 *  @brief 一系列用于从特定API获取数据的函数。
 *  @{
 */

/** @brief 获取健康小贴士。 */
void fetchHealthTip();
/** @brief 获取GitHub趋势项目。 */
void fetchGitHubTrending();
/** @brief 获取历史上的今天事件。 */
void fetchHistory();
/** @brief 获取“十万个为什么”条目。 */
void fetchTenWhy();
/** @brief 获取在线天气信息。 */
void fetchonlineweather();
/** @brief 获取一句土味情话。 */
void fetchSayLove();
/** @brief 获取每日英语句子。 */
void fetchEverydayEnglish();
/** @brief 获取每日运势。 */
void fetchFortune();
/** @brief 获取一首随机诗词。 */
void fetchShici();
/** @brief 获取一副对联。 */
void fetchDuilian();
/** @brief 获取美元对人民币的汇率。 */
void fetchFxRate();
/** @brief 获取一个随机英文单词及其释义。 */
void fetchRandomEnWord();
/** @brief 获取一言（Hitokoto）。 */
void fetchYiyan();
/** @brief 获取一句励志名言。 */
void fetchLzmy();
/** @brief 获取一句优美诗句。 */
void fetchVerse();
/** @brief 获取一句与天气相关的诗句。 */
void fetchTianqishiju();
/** @brief 获取一句“网抑云”热评。 */
void fetchHsjz();
/** @brief 获取一组脑筋急转弯。 */
void fetchBrainTeaser();
/** @brief 获取多支股票的近期数据。 */
void fetchStockData();
/** @brief 获取多种货币的汇率。 */
void fetchCurrencyData();

/** @} */

#endif // INTERNET_H

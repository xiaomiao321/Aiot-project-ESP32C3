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

// Function declarations
void internet_menu_init();
void internet_menu_loop();
void internet_menu_draw();
void internet_menu_next_page();
void internet_menu_prev_page();
bool internet_menu_back_press();
void InternetMenuScreen();

// Individual API fetch functions
void fetchHealthTip();
void fetchNews();
void fetchGitHubTrending();
void fetchHistory();
void fetchTenWhy();
void fetchonlineweather();
void fetchSayLove();
void fetchEverydayEnglish();
void fetchFortune();
void fetchShici();
void fetchDuilian();
void fetchFxRate();
void fetchRandomEnWord();
void fetchYiyan();
void fetchLzmy();
void fetchVerse();
void fetchTianqishiju();
void fetchHsjz();
void fetchBrainTeaser();
void fetchStockData();
void fetchCurrencyData();

#endif // INTERNET_H

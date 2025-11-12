// 包含所有必需的头文件
#include "Internet.h"
#include "System.h"        // 用于 tftLog 函数
#include "RotaryEncoder.h" // 用于读取编码器和按钮输入
#include "Menu.h"          // 用于菜单相关的函数和对象
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include "font_12.h"       // 包含自定义的中文字体

// 全局显示对象 (在其他文件中定义)
extern TFT_eSPI tft;
extern TFT_eSprite menuSprite;

// --- API 和数据配置 ---

// 定义API密钥
const char *INTERNET_API_KEY = "738b541a5f7a";

// 各个API的URL地址
const char *SAY_LOVE_API_URL = "http://whyta.cn/api/tx/saylove?key=";
const char *EVERYDAY_ENGLISH_API_URL = "http://whyta.cn/api/tx/everyday?key=";
const char *FORTUNE_API_URL = "http://whyta.cn/api/fortune?key=";
const char *SHICI_API_URL = "http://whyta.cn/api/shici?key=";
const char *DUILIAN_API_URL = "http://whyta.cn/api/tx/duilian?key=";
const char *FX_RATE_API_URL = "http://whyta.cn/api/tx/fxrate?key=";
const char *RANDOM_EN_WORD_API_URL = "http://ilovestudent.cn/api/commonApi/randomEnWord";
const char *YIYAN_API_URL = "http://whyta.cn/api/yiyan?key=";
const char *LZMY_API_URL = "http://whyta.cn/api/tx/lzmy?key=";
const char *VERSE_API_URL = "http://whyta.cn/api/tx/verse?key=";
const char *TIANQISHIJU_API_URL = "http://whyta.cn/api/tx/tianqishiju?key=";
const char *HSJZ_API_URL = "http://whyta.cn/api/tx/hsjz?key=";
const char *NAOWAN_API_URL = "http://whyta.cn/api/tx/naowan?key=";
const char *HEALTH_TIP_API_URL = "http://whyta.cn/api/tx/healthtip?key=";
const char *NEWS_API_URL = "http://whyta.cn/api/tx/bulletin?key=";
const char *GITHUB_API_URL = "http://whyta.cn/api/github?key=";
const char *HISTORY_API_URL = "http://whyta.cn/api/tx/lishi?key=";
const char *TEN_WHY_API_URL = "http://whyta.cn/api/tx/tenwhy?key=";
const char *WEATHER_API_URL = "http://whyta.cn/api/tianqi?key=";
const char *STOCK_API_URLS[] = {
    "http://api.marketstack.com/v1/eod?access_key=863ffb92f1e41bd9185cc3d98d464358&symbols=AMD&limit=5",
    "http://api.marketstack.com/v1/eod?access_key=863ffb92f1e41bd9185cc3d98d464358&symbols=AAPL&limit=5",
    "http://api.marketstack.com/v1/eod?access_key=863ffb92f1e41bd9185cc3d98d464358&symbols=NVDA&limit=5",
    "http://api.marketstack.com/v1/eod?access_key=863ffb92f1e41bd9185cc3d98d464358&symbols=TSLA&limit=5",
    "http://api.marketstack.com/v1/eod?access_key=863ffb92f1e41bd9185cc3d98d464358&symbols=MSFT&limit=5",
    "http://api.marketstack.com/v1/eod?access_key=863ffb92f1e41bd9185cc3d98d464358&symbols=INTC&limit=5"
};
const char *CURRENCY_API_URLS[] = {
    "http://www.amdoren.com/api/currency.php?api_key=MgRyTWeJe3brcWSTbxJqxSwSEgKtWa&from=USD&to=CNY",
    "http://www.amdoren.com/api/currency.php?api_key=MgRyTWeJe3brcWSTbxJqxSwSEgKtWa&from=EUR&to=CNY",
    "http://www.amdoren.com/api/currency.php?api_key=MgRyTWeJe3brcWSTbxJqxSwSEgKtWa&from=CNY&to=JPY",
    "http://www.amdoren.com/api/currency.php?api_key=MgRyTWeJe3brcWSTbxJqxSwSEgKtWa&from=GBP&to=CNY",
    "http://www.amdoren.com/api/currency.php?api_key=MgRyTWeJe3brcWSTbxJqxSwSEgKtWa&from=CAD&to=CNY",
    "http://www.amdoren.com/api/currency.php?api_key=MgRyTWeJe3brcWSTbxJqxSwSEgKtWa&from=CHF&to=CNY"
};
const char *CURRENCY_PAIRS[] = {
    "USD to CNY", "EUR to CNY", "CNY to JPY", "GBP to CNY", "CAD to CNY", "CHF to CNY"
};

// 用于存储从各个API获取的数据的全局结构体变量
SayLoveData g_say_love_data;
EverydayEnglishData g_everyday_english_data;
FortuneData g_fortune_data;
ShiciData g_shici_data;
DuilianData g_duilian_data;
FxRateData g_fx_rate_data;
RandomEnWordData g_random_en_word_data;
YiyanData g_yiyan_data;
LzmyData g_lzmy_data;
VerseData g_verse_data;
TianqishijuData g_tianqishiju_data;
HsjzData g_hsjz_data;
BrainTeaserData g_brain_teaser_data;
HealthTipData g_health_tip_data;
GitHubData g_github_data;
HistoryData g_history_data;
TenWhyData g_ten_why_data;
WeatherData g_weather_data;
StockInfo g_stock_data[6];
CurrencyData g_currency_data[6];


// --- 内部状态变量 ---
static int g_current_internet_page = 0; // 当前显示的互联网信息页面索引
static const int MAX_INTERNET_PAGES = 20; // 总页面数量

// 脑筋急转弯页面的特定状态
static int g_brain_teaser_index = 0;       // 当前显示的题目索引
static bool g_show_brain_teaser_answer = false; // 是否显示答案

/**
 * @brief 执行HTTP GET请求的辅助函数
 * @param url 要请求的URL
 * @param maxRetries 最大重试次数
 * @return 成功时返回响应体字符串，失败时返回 "N/A"
 */
String httpGETRequest(const char *url, int maxRetries = 5)
{
    if (WiFi.status() != WL_CONNECTED)
    {
        tftLog("HTTP: No WiFi", TFT_RED);
        return "N/A";
    }

    tftLog("HTTP: Start...", TFT_YELLOW);
    Serial.printf("HTTP GET: %s\n", url);

    WiFiClient client;
    HTTPClient http;
    String payload = "N/A";
    bool success = false;
    char log_buffer[100];

    // 绘制重试进度条的边框
    tft.drawRect(20, tft.height() - 20, 202, 17, TFT_WHITE);

    for (int i = 0; i < maxRetries; i++)
    {
        // 更新进度条
        tft.fillRect(21, tft.height() - 18, (i + 1) * (200 / maxRetries), 13, TFT_BLUE);
        sprintf(log_buffer, "HTTP Try %d/%d", i + 1, maxRetries);
        tftLog(log_buffer, TFT_YELLOW);

        // 每次循环前清理状态
        client.stop();
        http.end();
        delay(100);

        if (http.begin(client, url))
        {
            http.setTimeout(5000); // 5秒超时
            http.addHeader("User-Agent", "ESP32-HTTP-Client/1.0");
            http.addHeader("Connection", "close"); // 避免长连接问题

            int httpCode = http.GET();

            if (httpCode > 0)
            {
                sprintf(log_buffer, "HTTP: %d", httpCode);
                tftLog(log_buffer, (httpCode == HTTP_CODE_OK) ? TFT_GREEN : TFT_RED);

                if (httpCode == HTTP_CODE_OK)
                {
                    payload = http.getString();
                    tftLog("Success!", TFT_GREEN);
                    success = true;
                    http.end();
                    break; // 成功则跳出循环
                }
            }
            else
            {
                snprintf(log_buffer, sizeof(log_buffer), "Failed: %s", http.errorToString(httpCode).c_str());
                tftLog(log_buffer, TFT_RED);
            }
            http.end();
        }
        else
        {
            tftLog("Begin failed", TFT_RED);
        }
        delay(1000); // 重试间隔
    }

    // 根据最终结果更新进度条颜色
    if (success)
    {
        tft.fillRect(21, tft.height() - 18, 200, 13, TFT_GREEN);
    }
    else
    {
        tftLog("HTTP FAILED", TFT_RED);
    }

    return success ? payload : "N/A";
}

// --- 单个API的数据获取函数 ---

/**
 * @brief 获取土味情话数据
 */
void fetchSayLove()
{
    String url = String(SAY_LOVE_API_URL) + INTERNET_API_KEY;
    String payload = httpGETRequest(url.c_str());
    if (payload != "N/A")
    {
        DynamicJsonDocument doc(1024);
        if (deserializeJson(doc, payload).code() == DeserializationError::Ok)
        {
            g_say_love_data.content = doc["result"]["content"].as<String>();
            tftLogInfo("Say Love fetched.");
        }
        else
        {
            tftLogError("Say Love JSON error");
        }
    }
}

/**
 * @brief 获取每日英语数据
 */
void fetchEverydayEnglish()
{
    String url = String(EVERYDAY_ENGLISH_API_URL) + INTERNET_API_KEY;
    String payload = httpGETRequest(url.c_str());
    if (payload != "N/A")
    {
        DynamicJsonDocument doc(1024);
        if (deserializeJson(doc, payload).code() == DeserializationError::Ok)
        {
            g_everyday_english_data.content = doc["result"]["content"].as<String>();
            g_everyday_english_data.note = doc["result"]["note"].as<String>();
            tftLogInfo("Everyday English fetched.");
        }
        else
        {
            tftLogError("Everyday English JSON error");
        }
    }
}


void fetchFortune()
{
    String url = String(FORTUNE_API_URL) + INTERNET_API_KEY;
    String payload = httpGETRequest(url.c_str());
    if (payload.length() > 0)
    {
        DynamicJsonDocument doc(1024);
        DeserializationError error = deserializeJson(doc, payload);
        if (!error)
        {
            g_fortune_data.success = doc["success"].as<bool>();
            if (g_fortune_data.success)
            {
                g_fortune_data.sign = doc["data"]["sign"].as<String>();
                g_fortune_data.description = doc["data"]["description"].as<String>();
                g_fortune_data.luckyColor = doc["data"]["luckyColor"].as<String>();
                g_fortune_data.luckyNumber = doc["data"]["luckyNumber"].as<int>();
                g_fortune_data.message = ""; // Clear any previous message
                Serial.println("Fortune: " + g_fortune_data.sign + ", " + g_fortune_data.description + ", Color: " + g_fortune_data.luckyColor + ", Number: " + String(g_fortune_data.luckyNumber));
            }
            else
            {
                g_fortune_data.message = doc["message"].as<String>();
                Serial.println("Fortune Error: " + g_fortune_data.message);
            }
            tftLogInfo("Fortune fetched.");
        }
        else
        {
            tftLogError("Fortune JSON error: " + String(error.c_str()));
        }
    }
}

/**
 * @brief 获取古诗词数据
 */
void fetchShici()
{
    String url = String(SHICI_API_URL) + INTERNET_API_KEY;
    String payload = httpGETRequest(url.c_str());
    if (payload != "N/A")
    {
        DynamicJsonDocument doc(4096); // 为完整诗歌内容增加容量
        if (deserializeJson(doc, payload).code() == DeserializationError::Ok)
        {
            g_shici_data.title = doc["data"]["origin"]["title"].as<String>();
            g_shici_data.author = doc["data"]["origin"]["author"].as<String>();
            g_shici_data.dynasty = doc["data"]["origin"]["dynasty"].as<String>();

            // 从JSON数组中拼接完整的诗歌内容
            g_shici_data.full_content = "";
            JsonArray content_array = doc["data"]["origin"]["content"].as<JsonArray>();
            for (JsonVariant line : content_array)
            {
                g_shici_data.full_content += line.as<String>() + "\n";
            }
            tftLogInfo("Shici fetched.");
        }
        else
        {
            tftLogError("Shici JSON error");
        }
    }
}

void fetchDuilian()
{
    String url = String(DUILIAN_API_URL) + INTERNET_API_KEY;
    String payload = httpGETRequest(url.c_str());
    if (payload.length() > 0)
    {
        DynamicJsonDocument doc(1024);
        DeserializationError error = deserializeJson(doc, payload);
        if (!error)
        {
            g_duilian_data.content = doc["result"]["content"].as<String>();
            tftLogInfo("Duilian fetched.");
            Serial.println("Duilian: " + g_duilian_data.content);
        }
        else
        {
            tftLogError("Duilian JSON error: " + String(error.c_str()));
        }
    }
}

void fetchFxRate()
{
    String url = String(FX_RATE_API_URL) + INTERNET_API_KEY + "&fromcoin=USD&tocoin=CNY&money=1";
    String payload = httpGETRequest(url.c_str());
    if (payload.length() > 0)
    {
        DynamicJsonDocument doc(1024);
        DeserializationError error = deserializeJson(doc, payload);
        if (!error)
        {
            g_fx_rate_data.money = doc["result"]["money"].as<String>();
            tftLogInfo("FxRate fetched.");
            Serial.println("FxRate (1 USD to CNY): " + g_fx_rate_data.money);
        }
        else
        {
            tftLogError("FxRate JSON error: " + String(error.c_str()));
        }
    }
}

void fetchRandomEnWord()
{
    String url = String(RANDOM_EN_WORD_API_URL);
    String payload = httpGETRequest(url.c_str());
    if (payload.length() > 0)
    {
        DynamicJsonDocument doc(2048); // Increased capacity for potentially larger word data
        DeserializationError error = deserializeJson(doc, payload);
        if (!error)
        {
            g_random_en_word_data.headWord = doc["data"]["headWord"].as<String>();
            g_random_en_word_data.tranCn = doc["data"]["tranCn"].as<String>();

            g_random_en_word_data.phrases_en = "";
            g_random_en_word_data.phrases_cn = "";
            JsonArray phrases = doc["data"]["phrases"].as<JsonArray>();
            for (JsonObject phrase : phrases)
            {
                if (g_random_en_word_data.phrases_en.length() > 0) g_random_en_word_data.phrases_en += "\n";
                if (g_random_en_word_data.phrases_cn.length() > 0) g_random_en_word_data.phrases_cn += "\n";
                g_random_en_word_data.phrases_en += phrase["content"].as<String>();
                g_random_en_word_data.phrases_cn += phrase["cn"].as<String>();
            }
            tftLogInfo("Random English Word fetched.");
            Serial.println("Random Word: " + g_random_en_word_data.headWord + " - " + g_random_en_word_data.tranCn);
            if (g_random_en_word_data.phrases_en.length() > 0)
            {
                Serial.println("Phrases:\n" + g_random_en_word_data.phrases_en + "\n" + g_random_en_word_data.phrases_cn);
            }
        }
        else
        {
            tftLogError("Random En Word JSON error: " + String(error.c_str()));
        }
    }
}

void fetchYiyan()
{
    String url = String(YIYAN_API_URL) + INTERNET_API_KEY;
    String payload = httpGETRequest(url.c_str());
    if (payload.length() > 0)
    {
        DynamicJsonDocument doc(1024);
        DeserializationError error = deserializeJson(doc, payload);
        if (!error)
        {
            g_yiyan_data.hitokoto = doc["hitokoto"].as<String>();
            tftLogInfo("Yiyan fetched.");
            Serial.println("Yiyan: " + g_yiyan_data.hitokoto);
        }
        else
        {
            tftLogError("Yiyan JSON error: " + String(error.c_str()));
        }
    }
}

void fetchLzmy()
{
    String url = String(LZMY_API_URL) + INTERNET_API_KEY;
    String payload = httpGETRequest(url.c_str());
    if (payload.length() > 0)
    {
        DynamicJsonDocument doc(1024);
        DeserializationError error = deserializeJson(doc, payload);
        if (!error && doc["code"] == 200)
        {
            g_lzmy_data.saying = doc["result"]["saying"].as<String>();
            g_lzmy_data.transl = doc["result"]["transl"].as<String>();
            g_lzmy_data.source = doc["result"]["source"].as<String>();
            tftLogInfo("Lzmy fetched.");
            Serial.println("Lzmy: " + g_lzmy_data.saying);
        }
        else
        {
            tftLogError("Lzmy JSON error or API error.");
        }
    }
}

void fetchVerse()
{
    String url = String(VERSE_API_URL) + INTERNET_API_KEY;
    String payload = httpGETRequest(url.c_str());
    if (payload.length() > 0)
    {
        DynamicJsonDocument doc(1024);
        DeserializationError error = deserializeJson(doc, payload);
        if (!error && doc["code"] == 200)
        {
            JsonObject item = doc["result"]["list"][0];
            g_verse_data.content = item["content"].as<String>();
            g_verse_data.source = item["source"].as<String>();
            g_verse_data.author = item["author"].as<String>();
            tftLogInfo("Verse fetched.");
            Serial.println("Verse: " + g_verse_data.content);
        }
        else
        {
            tftLogError("Verse JSON error or API error.");
        }
    }
}

void fetchTianqishiju()
{
    String url = String(TIANQISHIJU_API_URL) + INTERNET_API_KEY;
    String payload = httpGETRequest(url.c_str());
    if (payload.length() > 0)
    {
        DynamicJsonDocument doc(1024);
        DeserializationError error = deserializeJson(doc, payload);
        if (!error && doc["code"] == 200)
        {
            g_tianqishiju_data.content = doc["result"]["content"].as<String>();
            g_tianqishiju_data.author = doc["result"]["author"].as<String>();
            g_tianqishiju_data.source = doc["result"]["source"].as<String>();
            tftLogInfo("Tianqishiju fetched.");
            Serial.println("Tianqishiju: " + g_tianqishiju_data.content);
        }
        else
        {
            tftLogError("Tianqishiju JSON error or API error.");
        }
    }
}
void fetchHsjz()
{

    String url = String(HSJZ_API_URL) + INTERNET_API_KEY;

    String payload = httpGETRequest(url.c_str());

    if (payload.length() > 0)
    {

        DynamicJsonDocument doc(1024);

        DeserializationError error = deserializeJson(doc, payload);

        if (!error && doc["code"] == 200)
        {

            g_hsjz_data.content = doc["result"]["content"].as<String>();

            tftLogInfo("Hsjz fetched.");
            Serial.println("Hsjz: " + g_hsjz_data.content);

        }
        else
        {

            tftLogError("Hsjz JSON error or API error.");

        }

    }

}

void fetchBrainTeaser()
{
    String url = String(NAOWAN_API_URL) + INTERNET_API_KEY + "&num=3";
    String payload = httpGETRequest(url.c_str());
    if (payload.length() > 0)
    {
        DynamicJsonDocument doc(2048);
        DeserializationError error = deserializeJson(doc, payload);
        if (!error && doc["code"] == 200)
        {
            JsonArray list = doc["result"]["list"].as<JsonArray>();
            g_brain_teaser_data.count = 0;
            for (JsonObject item : list)
            {
                if (g_brain_teaser_data.count < 3)
                {
                    g_brain_teaser_data.teasers[g_brain_teaser_data.count].quest = item["quest"].as<String>();
                    g_brain_teaser_data.teasers[g_brain_teaser_data.count].result = item["result"].as<String>();
                    g_brain_teaser_data.count++;
                }
            }
            g_brain_teaser_index = 0;
            g_show_brain_teaser_answer = false;
            tftLogInfo("Brain Teaser fetched.");
            Serial.println("Brain Teaser fetched: " + String(g_brain_teaser_data.count) + " items.");
        }
        else
        {
            tftLogError("Brain Teaser JSON error or API error.");
        }
    }
}

void fetchHealthTip()
{
    String url = String(HEALTH_TIP_API_URL) + INTERNET_API_KEY;
    String payload = httpGETRequest(url.c_str());
    if (payload.length() > 0 && payload != "N/A")
    {
        DynamicJsonDocument doc(1024);
        DeserializationError error = deserializeJson(doc, payload);
        if (!error && doc["code"] == 200)
        {
            g_health_tip_data.content = doc["result"]["content"].as<String>();
            tftLogInfo("Health Tip fetched.");
            Serial.println("Health Tip: " + g_health_tip_data.content);
        }
        else
        {
            tftLogError("Health Tip JSON error.");
        }
    }
}

void fetchGitHubTrending()
{
    String url = String(GITHUB_API_URL) + INTERNET_API_KEY;
    String payload = httpGETRequest(url.c_str());
    if (payload.length() > 0 && payload != "N/A")
    {
        DynamicJsonDocument doc(8192);
        DeserializationError error = deserializeJson(doc, payload);
        if (!error && doc["status"] == "success")
        {
            JsonArray items = doc["items"].as<JsonArray>();
            g_github_data.count = 0;
            for (JsonObject item : items)
            {
                if (g_github_data.count < 12)
                { // Limit to 12
                    g_github_data.repos[g_github_data.count].name = item["name"].as<String>();
                    g_github_data.repos[g_github_data.count].description = item["description"].as<String>();
                    g_github_data.repos[g_github_data.count].stars = item["stars"].as<String>();
                    g_github_data.count++;
                }
            }
            tftLogInfo("GitHub fetched.");
            Serial.println("GitHub fetched: " + String(g_github_data.count) + " items.");
        }
        else
        {
            tftLogError("GitHub JSON error.");
        }
    }
}

void fetchHistory()
{
    String url = String(HISTORY_API_URL) + INTERNET_API_KEY;
    String payload = httpGETRequest(url.c_str());
    if (payload.length() > 0 && payload != "N/A")
    {
        DynamicJsonDocument doc(8192);
        DeserializationError error = deserializeJson(doc, payload);
        if (!error && doc["code"] == 200)
        {
            JsonArray list = doc["result"]["list"].as<JsonArray>();
            g_history_data.count = 0;
            for (JsonObject item : list)
            {
                if (g_history_data.count < 10)
                { // Limit to 5
                    g_history_data.events[g_history_data.count].title = item["title"].as<String>();
                    g_history_data.events[g_history_data.count].lsdate = item["lsdate"].as<String>();
                    g_history_data.count++;
                }
            }
            tftLogInfo("History fetched.");
            Serial.println("History fetched: " + String(g_history_data.count) + " items.");
        }
        else
        {
            tftLogError("History JSON error.");
        }
    }
}

void fetchTenWhy()
{
    String url = String(TEN_WHY_API_URL) + INTERNET_API_KEY;
    String payload = httpGETRequest(url.c_str());
    if (payload.length() > 0 && payload != "N/A")
    {
        DynamicJsonDocument doc(2048);
        DeserializationError error = deserializeJson(doc, payload);
        if (!error && doc["code"] == 200)
        {
            JsonObject item = doc["result"]["list"][0];
            g_ten_why_data.title = item["title"].as<String>();
            g_ten_why_data.content = item["content"].as<String>();
            tftLogInfo("Ten Why fetched.");
            Serial.println("Ten Why: " + g_ten_why_data.title);
        }
        else
        {
            tftLogError("Ten Why JSON error.");
        }
    }
}

void fetchonlineweather()
{
    String url = String(WEATHER_API_URL) + INTERNET_API_KEY + "&city=Hangzhou";
    String payload = httpGETRequest(url.c_str());
    if (payload.length() > 0 && payload != "N/A")
    {
        DynamicJsonDocument doc(2048);
        DeserializationError error = deserializeJson(doc, payload);
        if (!error && doc["status"].as<int>() == 1)
        {
            JsonObject data = doc["data"];
            g_weather_data.FeelsLikeC = data["FeelsLikeC"].as<String>();
            g_weather_data.temp_C = data["temp_C"].as<String>();
            g_weather_data.observation_time = data["observation_time"].as<String>();
            g_weather_data.precipMM = data["precipMM"].as<String>();
            g_weather_data.weatherDesc = data["weatherDesc"][0]["value"].as<String>();
            g_weather_data.humidity = data["humidity"].as<String>();
            g_weather_data.pressure = data["pressure"].as<String>();
            g_weather_data.visibility = data["visibility"].as<String>();
            g_weather_data.winddir16Point = data["winddir16Point"].as<String>();
            g_weather_data.windspeedKmph = data["windspeedKmph"].as<String>();
            tftLogInfo("Weather fetched.");
            Serial.println("Weather fetched for Hangzhou.");
        }
        else
        {
            tftLogError("Weather JSON error.");
        }
    }
}

void fetchStockData()
{
    for (int i = 0; i < 6; i++)
    {
        String payload = httpGETRequest(STOCK_API_URLS[i]);
        if (payload.length() > 0 && payload != "N/A")
        {
            DynamicJsonDocument doc(4096);
            DeserializationError error = deserializeJson(doc, payload);
            if (!error)
            {
                JsonArray data = doc["data"].as<JsonArray>();
                g_stock_data[i].count = data.size();
                if (g_stock_data[i].count > 0)
                {
                    g_stock_data[i].symbol = data[0]["symbol"].as<String>();

                    // Data is newest to oldest, we need to reverse it.
                    for (int j = 0; j < g_stock_data[i].count; j++)
                    {
                        int reversed_index = g_stock_data[i].count - 1 - j;
                        g_stock_data[i].daily_data[j].close = data[reversed_index]["close"].as<float>();
                    }

                    // Calculate percentage change
                    g_stock_data[i].daily_data[0].change_percent = 0.0f;
                    for (int j = 1; j < g_stock_data[i].count; j++)
                    {
                        float prev_close = g_stock_data[i].daily_data[j - 1].close;
                        if (prev_close != 0)
                        {
                            g_stock_data[i].daily_data[j].change_percent =
                                ((g_stock_data[i].daily_data[j].close - prev_close) / prev_close) * 100.0f;
                        }
                        else
                        {
                            g_stock_data[i].daily_data[j].change_percent = 0.0f;
                        }
                    }
                }
                tftLogInfo("Stock data fetched for " + g_stock_data[i].symbol);
                Serial.println("Stock data fetched for " + g_stock_data[i].symbol);
            }
            else
            {
                tftLogError("Stock JSON error for URL " + String(i));
            }
        }
    }
}

void fetchCurrencyData()
{
    for (int i = 0; i < 6; i++)
    {
        String payload = httpGETRequest(CURRENCY_API_URLS[i]);
        if (payload.length() > 0 && payload != "N/A")
        {
            DynamicJsonDocument doc(1024);
            DeserializationError error = deserializeJson(doc, payload);
            if (!error && doc["error"] == 0)
            {
                g_currency_data[i].pair_name = CURRENCY_PAIRS[i];
                g_currency_data[i].rate = doc["amount"].as<float>();
                tftLogInfo("Currency fetched for " + g_currency_data[i].pair_name);
                Serial.println("Currency fetched for " + g_currency_data[i].pair_name);
            }
            else
            {
                tftLogError("Currency JSON error for " + String(CURRENCY_PAIRS[i]));
            }
        }
    }
}

// --- 互联网信息菜单核心功能 ---

/**
 * @brief 初始化互联网信息菜单
 * @details 检查WiFi连接，连接成功后加载所有数据，并准备显示第一页。
 */
void internet_menu_init()
{
    tftClearLog();
    tftLogInfo("Initializing Internet Menu...");
    tft.fillScreen(TFT_BLACK);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(2);
    tft.drawString("Connecting WiFi...", tft.width() / 2, tft.height() / 2 - 10);

    // 显示连接中的动画
    int dot_x = tft.width() / 2 + tft.textWidth("Connecting WiFi...") / 2;
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        tft.drawString(".", dot_x, tft.height() / 2 - 10);
        dot_x += tft.textWidth(".");
        if (dot_x > tft.width() - 10)
        {
            tft.fillRect(tft.width() / 2 + tft.textWidth("Connecting WiFi...") / 2, tft.height() / 2 - 10, 100, 20, TFT_BLACK);
            dot_x = tft.width() / 2 + tft.textWidth("Connecting WiFi...") / 2;
        }
    }
    tft.fillScreen(TFT_BLACK);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_GREEN);
    tft.drawString("WiFi Connected!", tft.width() / 2, tft.height() / 2);
    delay(1000);

    menuSprite.createSprite(tft.width(), tft.height());
    menuSprite.fillScreen(TFT_BLACK);
    menuSprite.setTextColor(TFT_WHITE);
    menuSprite.setTextDatum(MC_DATUM);
    menuSprite.drawString("Loading Data...", tft.width() / 2, tft.height() / 2);
    menuSprite.pushSprite(0, 0);

    // 首次进入时获取所有数据
    // (可以根据需要取消注释)
    // fetchSayLove();
    // fetchEverydayEnglish();
    // fetchFortune();
    // fetchShici();
    // fetchDuilian();
    // fetchFxRate();
    // fetchRandomEnWord();
    // fetchYiyan();
    // fetchLzmy();
    // fetchVerse();
    // fetchTianqishiju();
    // fetchBrainTeaser();
    // fetchHealthTip();
    // fetchGitHubTrending();
    // fetchHistory();
    // fetchTenWhy();
    // fetchonlineweather();
    // fetchStockData();
    // fetchCurrencyData();

    tftLogInfo("Internet Menu Initialized.");
    g_current_internet_page = 0; // 从第一页开始
    internet_menu_draw(); // 绘制第一页
}

/**
 * @brief 在Sprite上绘制带自动换行的文本
 * @param sprite 要绘制的TFT_eSprite对象
 * @param text 要绘制的文本
 * @param x 起始X坐标
 * @param y 起始Y坐标
 * @param maxWidth 最大宽度
 * @param font 字体编号 (如果为0则使用已加载的字体)
 * @param color 文本颜色
 */
void drawWrappedText(TFT_eSprite &sprite, String text, int x, int y, int maxWidth, int font, uint16_t color)
{
    sprite.setTextColor(color);
    if (font > 0) sprite.setTextFont(font);
    sprite.setTextWrap(true);
    sprite.setCursor(x, y);
    sprite.print(text);
}

/**
 * @brief 绘制当前页面的内容
 */
void internet_menu_draw()
{
    menuSprite.fillScreen(TFT_BLACK);
    menuSprite.setTextDatum(TL_DATUM);
    menuSprite.setTextColor(TFT_WHITE);

    // 绘制页面指示器 (例如 "1/20")
    menuSprite.setTextFont(1);
    menuSprite.setTextSize(2);
    menuSprite.setTextDatum(TR_DATUM);
    menuSprite.setTextColor(TFT_LIGHTGREY);
    menuSprite.drawString(String(g_current_internet_page + 1) + "/" + String(MAX_INTERNET_PAGES), menuSprite.width() - 5, 5);
    menuSprite.setTextDatum(TL_DATUM);

    int content_x = 5;
    int content_y = 45;
    int content_width = menuSprite.width() - 10;
    int english_font_num = 1;

    // 根据当前页面索引，绘制不同的内容
    switch (g_current_internet_page)
    {
    case 0: // 土味情话
        menuSprite.setTextFont(1);
        menuSprite.setTextSize(2);
        menuSprite.setTextColor(TFT_PINK);
        menuSprite.drawString("Love Talk", 5, 25);
        menuSprite.loadFont(font_12); // 加载中文字体
        drawWrappedText(menuSprite, g_say_love_data.content, content_x, content_y, content_width, 0, TFT_WHITE);
        menuSprite.unloadFont(); // 卸载中文字体
        break;
    case 1: // 每日英语
        menuSprite.setTextFont(1);
        menuSprite.setTextSize(2);
        menuSprite.setTextColor(TFT_GREEN);
        menuSprite.drawString("Daily English", 5, 25);
        drawWrappedText(menuSprite, g_everyday_english_data.content, content_x, content_y, content_width, english_font_num, TFT_WHITE);
        content_y = menuSprite.getCursorY() + 20;
        menuSprite.loadFont(font_12);
        drawWrappedText(menuSprite, g_everyday_english_data.note, content_x, content_y, content_width, 0, TFT_YELLOW);
        menuSprite.unloadFont();
        break;
    case 2: // Fortune
        menuSprite.setTextFont(1);
        menuSprite.setTextSize(2);
        menuSprite.setTextColor(TFT_ORANGE);
        menuSprite.drawString("Daily Fortune", 5, 25);
        if (g_fortune_data.success)
        {
            menuSprite.loadFont(font_12);
            drawWrappedText(menuSprite, "签：" + g_fortune_data.sign, content_x, content_y, content_width, 0, TFT_WHITE);
            content_y = menuSprite.getCursorY() + 15;
            drawWrappedText(menuSprite, "描述: " + g_fortune_data.description, content_x, content_y, content_width, 0, TFT_WHITE);
            content_y = menuSprite.getCursorY() + 15;
            drawWrappedText(menuSprite, "颜色: " + g_fortune_data.luckyColor, content_x, content_y, content_width, 0, TFT_WHITE);
            menuSprite.unloadFont();
            content_y = menuSprite.getCursorY() + 15;
            menuSprite.setTextFont(1);
            menuSprite.setTextSize(2);
            drawWrappedText(menuSprite, "数字: " + String(g_fortune_data.luckyNumber), content_x, content_y, content_width, english_font_num, TFT_WHITE);
        }
        else
        {
            menuSprite.loadFont(font_12);
            drawWrappedText(menuSprite, g_fortune_data.message, content_x, content_y, content_width, 0, TFT_WHITE);
            menuSprite.unloadFont();
        }
        break;
    case 3: // Shici (Poem)
    {
        menuSprite.loadFont(font_12);
        menuSprite.setTextColor(TFT_CYAN, TFT_BLACK);
        menuSprite.setTextDatum(MC_DATUM);
        menuSprite.drawString(g_shici_data.title, menuSprite.width() / 2, 20);
        String author_info = g_shici_data.author + "[ " + g_shici_data.dynasty + "]";
        menuSprite.drawString(author_info, menuSprite.width() / 2, 45);
        menuSprite.unloadFont();
        menuSprite.setTextFont(1);
        menuSprite.setTextSize(2);
        menuSprite.setTextColor(TFT_ORANGE, TFT_BLACK);
        menuSprite.setTextDatum(BC_DATUM);
        menuSprite.drawString("Popularity: " + String(g_shici_data.popularity), 120, 240 - 5);
        menuSprite.setTextDatum(TL_DATUM);
        menuSprite.loadFont(font_12);
        drawWrappedText(menuSprite, g_shici_data.full_content, content_x, 70, content_width, 0, TFT_WHITE);
        menuSprite.unloadFont();
    }
    break;
    case 4: // Duilian (Couplet)
        menuSprite.setTextFont(1);
        menuSprite.setTextSize(2);
        menuSprite.setTextColor(TFT_BLUE);
        menuSprite.drawString("Couplet", 5, 25);
        menuSprite.loadFont(font_12);
        drawWrappedText(menuSprite, g_duilian_data.content, content_x, content_y, content_width, 0, TFT_WHITE);
        menuSprite.unloadFont();
        break;
    case 5: // FxRate (Exchange Rate)
        menuSprite.setTextFont(1);
        menuSprite.setTextSize(2);
        menuSprite.setTextDatum(TC_DATUM);
        menuSprite.setTextColor(TFT_GREENYELLOW);
        menuSprite.drawString("USD to CNY (1 USD)", 120, 25);
        menuSprite.setTextFont(7);
        menuSprite.setTextSize(1);
        menuSprite.setTextDatum(MC_DATUM);
        menuSprite.drawString(g_fx_rate_data.money, 120, 120);
        menuSprite.setTextFont(1);
        menuSprite.setTextSize(2);
        break;
    case 6: // Random English Word
        menuSprite.setTextFont(1);
        menuSprite.setTextSize(2);
        menuSprite.setTextColor(TFT_VIOLET);
        menuSprite.drawString("Random Word", 5, 25);
        menuSprite.setTextFont(1);
        menuSprite.setTextSize(2);
        drawWrappedText(menuSprite, g_random_en_word_data.headWord, content_x, content_y, content_width, english_font_num, TFT_WHITE);
        content_y = menuSprite.getCursorY() + 15;
        menuSprite.loadFont(font_12);
        drawWrappedText(menuSprite, g_random_en_word_data.tranCn, content_x, content_y, content_width, 0, TFT_WHITE);
        menuSprite.unloadFont();
        if (g_random_en_word_data.phrases_en.length() > 0)
        {
            content_y = menuSprite.getCursorY() + 15;
            menuSprite.setTextFont(1);
            menuSprite.setTextSize(2);
            drawWrappedText(menuSprite, g_random_en_word_data.phrases_en, content_x, content_y, content_width, english_font_num, TFT_WHITE);
            content_y = menuSprite.getCursorY() + 15;
            menuSprite.loadFont(font_12);
            drawWrappedText(menuSprite, g_random_en_word_data.phrases_cn, content_x, content_y, content_width, 0, TFT_WHITE);
            menuSprite.unloadFont();
        }
        break;
    case 7: // Yiyan
        menuSprite.setTextFont(1);
        menuSprite.setTextSize(2);
        menuSprite.setTextColor(TFT_SKYBLUE);
        menuSprite.drawString("Daily Saying", 5, 25);
        menuSprite.loadFont(font_12);
        drawWrappedText(menuSprite, g_yiyan_data.hitokoto, content_x, content_y, content_width, 0, TFT_WHITE);
        menuSprite.unloadFont();
        break;
    case 8: // Lzmy
        menuSprite.setTextFont(1);
        menuSprite.setTextSize(2);
        menuSprite.setTextColor(TFT_GOLD);
        menuSprite.drawString("Inspiring Saying", 5, 25);
        menuSprite.loadFont(font_12);
        drawWrappedText(menuSprite, g_lzmy_data.saying, content_x, content_y, content_width, 0, TFT_WHITE);
        content_y = menuSprite.getCursorY() + 15;
        drawWrappedText(menuSprite, g_lzmy_data.transl, content_x, content_y, content_width, 0, TFT_YELLOW);
        content_y = menuSprite.getCursorY() + 15;
        drawWrappedText(menuSprite, g_lzmy_data.source, content_x, content_y, content_width, 0, TFT_YELLOW);
        menuSprite.unloadFont();
        break;
    case 9: // Verse
        menuSprite.setTextFont(1);
        menuSprite.setTextSize(2);
        menuSprite.setTextColor(TFT_CYAN);
        menuSprite.drawString("Beautiful Verse", 5, 25);
        menuSprite.loadFont(font_12);
        drawWrappedText(menuSprite, g_verse_data.content, content_x, content_y, content_width, 0, TFT_WHITE);
        content_y = menuSprite.getCursorY() + 15;
        drawWrappedText(menuSprite, g_verse_data.author + "《 " + g_verse_data.source + " 》", content_x, content_y, content_width, 0, TFT_YELLOW);
        menuSprite.unloadFont();
        break;
    case 10: // Tianqishiju
        menuSprite.setTextFont(1);
        menuSprite.setTextSize(2);
        menuSprite.setTextColor(TFT_YELLOW);
        menuSprite.drawString("Weather Poem", 5, 25);
        menuSprite.loadFont(font_12);
        drawWrappedText(menuSprite, g_tianqishiju_data.content, content_x, content_y, content_width, 0, TFT_WHITE);
        content_y = menuSprite.getCursorY() + 15;
        drawWrappedText(menuSprite, g_tianqishiju_data.author + "《 " + g_tianqishiju_data.source + " 》", content_x, content_y, content_width, 0, TFT_YELLOW);
        menuSprite.unloadFont();
        break;
    case 11: // Hsjz
        menuSprite.setTextFont(1);
        menuSprite.setTextSize(2);
        menuSprite.setTextColor(TFT_WHITE);
        menuSprite.drawString("Sad Sentence", 5, 25);
        menuSprite.loadFont(font_12);
        drawWrappedText(menuSprite, g_hsjz_data.content, content_x, content_y, content_width, 0, TFT_WHITE);
        menuSprite.unloadFont();
        break;
    case 12: // 脑筋急转弯
        menuSprite.setTextFont(1);
        menuSprite.setTextSize(2);
        menuSprite.setTextColor(TFT_MAGENTA);
        menuSprite.drawString("Brain Teaser", 5, 25);
        menuSprite.loadFont(font_12);
        if (g_brain_teaser_data.count > 0)
        {
            drawWrappedText(menuSprite, g_brain_teaser_data.teasers[g_brain_teaser_index].quest, content_x, content_y, content_width, 0, TFT_WHITE);
            if (g_show_brain_teaser_answer)
            { // 如果需要显示答案
                content_y = menuSprite.getCursorY() + 15;
                drawWrappedText(menuSprite, g_brain_teaser_data.teasers[g_brain_teaser_index].result, content_x, content_y, content_width, 0, TFT_GREEN);
            }
        }
        else
        {
            drawWrappedText(menuSprite, "Loading...", content_x, content_y, content_width, 0, TFT_WHITE);
        }
        menuSprite.unloadFont();
        break;
    case 13: // Health Tip
        menuSprite.setTextFont(1);
        menuSprite.setTextSize(2);
        menuSprite.setTextColor(TFT_GREEN);
        menuSprite.drawString("Health Tip", 5, 25);
        menuSprite.loadFont(font_12);
        drawWrappedText(menuSprite, g_health_tip_data.content, content_x, content_y, content_width, 0, TFT_WHITE);
        menuSprite.unloadFont();
        break;
    case 14: // GitHub Trending
    {
        menuSprite.setTextFont(1);
        menuSprite.setTextSize(2);
        menuSprite.setTextColor(TFT_CYAN);
        menuSprite.drawString("GitHub Trending", 5, 5);
        String github_text = "";
        menuSprite.setTextSize(1);
        for (int i = 0; i < g_github_data.count; i++)
        {
            github_text += String(i + 1) + ". " + g_github_data.repos[i].name + "*" + g_github_data.repos[i].stars + "\n";
            github_text += "   " + g_github_data.repos[i].description + "\n";
        }
        drawWrappedText(menuSprite, github_text, content_x, content_y - 20, content_width, 0, TFT_WHITE);
    }
    break;
    case 15: // History Today
    {
        menuSprite.setTextFont(1);
        menuSprite.setTextSize(2);
        menuSprite.setTextColor(TFT_YELLOW);
        menuSprite.drawString("History Today", 5, 5);
        menuSprite.loadFont(font_12);
        String history_text = "";
        for (int i = 0; i < g_history_data.count; i++)
        {
            history_text += g_history_data.events[i].lsdate.substring(0, 4) + " " + g_history_data.events[i].title + "\n";
        }
        drawWrappedText(menuSprite, history_text, content_x, content_y - 20, content_width, 0, TFT_WHITE);
        menuSprite.unloadFont();
    }
    break;
    case 16: // Ten Why
        menuSprite.setTextFont(1);
        menuSprite.setTextSize(2);
        menuSprite.setTextColor(TFT_ORANGE);
        menuSprite.loadFont(font_12);
        menuSprite.drawString(g_ten_why_data.title, 5, 25);
        drawWrappedText(menuSprite, g_ten_why_data.content, content_x, content_y, content_width, 0, TFT_WHITE);
        menuSprite.unloadFont();
        break;
    case 17: // Weather
    {
        menuSprite.setTextFont(1);
        menuSprite.setTextSize(2);
        menuSprite.setTextColor(TFT_BLUE);
        menuSprite.setTextFont(2);
        menuSprite.setTextSize(1);
        menuSprite.drawString("Weather (Hangzhou)", 5, 25);
        String weather_text = "Temp: " + g_weather_data.temp_C + "C\n";
        weather_text += "Feels Like: " + g_weather_data.FeelsLikeC + "C\n";
        weather_text += "Weather: " + g_weather_data.weatherDesc + "\n";
        weather_text += "Humidity: " + g_weather_data.humidity + "\%\n";
        weather_text += "Precipitation: " + g_weather_data.precipMM + "mm\n";
        weather_text += "Pressure: " + g_weather_data.pressure + "hPa\n";
        weather_text += "Visibility: " + g_weather_data.visibility + "km\n";
        weather_text += "Wind: " + g_weather_data.winddir16Point + " " + g_weather_data.windspeedKmph + "km/h\n";
        weather_text += "Observation: " + g_weather_data.observation_time + " UTC";
        drawWrappedText(menuSprite, weather_text, content_x, content_y, content_width, 0, TFT_WHITE);
    }
    break;
    case 18: // Stocks
    {
        menuSprite.setTextFont(1);
        menuSprite.setTextSize(1);
        menuSprite.setTextColor(TFT_GOLD);
        menuSprite.drawString("Stock Market", 5, 5);
        menuSprite.setTextSize(1);

        int y_pos = 15;
        for (int i = 0; i < 6; i++)
        {
            if (g_stock_data[i].count > 0)
            {
                // Row 1: Symbol
                menuSprite.setTextColor(TFT_WHITE);
                menuSprite.drawString(g_stock_data[i].symbol, 5, y_pos);
                y_pos += 12;

                // Row 2: Close Prices
                String prices_line = "";
                for (int j = 0; j < g_stock_data[i].count; j++)
                {
                    prices_line += String(g_stock_data[i].daily_data[j].close, 2);
                    if (j < g_stock_data[i].count - 1) prices_line += " ";
                }
                menuSprite.drawString(prices_line, 5, y_pos);
                y_pos += 12;

                // Row 3: Percentage Changes
                int x_pos = 5;
                for (int j = 0; j < g_stock_data[i].count; j++)
                {
                    float change = g_stock_data[i].daily_data[j].change_percent;
                    uint16_t color = (change > 0) ? TFT_RED : ((change < 0) ? TFT_GREEN : TFT_DARKGREY);
                    menuSprite.setTextColor(color);
                    String change_str;
                    if (change > 0)
                    {
                        change_str = "+" + String(change, 2) + "%";
                    }
                    else
                    {
                        change_str = String(change, 2) + "%";
                    }
                    menuSprite.drawString(change_str, x_pos, y_pos);
                    x_pos += menuSprite.textWidth(change_str) + 5;
                }
                y_pos += 15; // Extra space for next stock
            }
        }
    }
    break;
    case 19: // Currency
    {
        menuSprite.setTextFont(1);
        menuSprite.setTextSize(2);
        menuSprite.setTextColor(TFT_WHITE);
        menuSprite.drawString("Currency Rates:", 5, 5);
        menuSprite.setTextSize(1);

        int y_pos = 25;
        for (int i = 0; i < 6; i++)
        {
            // Line 1: Currency Pair
            menuSprite.setTextColor(TFT_WHITE);
            menuSprite.drawString(g_currency_data[i].pair_name, 5, y_pos);
            y_pos += 12;

            // Line 2: Rate
            menuSprite.setTextColor(TFT_YELLOW);
            menuSprite.drawString(String(g_currency_data[i].rate, 4), 5, y_pos);
            y_pos += 18; // Extra space
        }
    }
    break;
    }

    menuSprite.pushSprite(0, 0); // 将Sprite内容推送到屏幕
}

/**
 * @brief 切换到下一页
 */
void internet_menu_next_page()
{
    g_current_internet_page = (g_current_internet_page + 1) % MAX_INTERNET_PAGES;
}

/**
 * @brief 切换到上一页
 */
void internet_menu_prev_page()
{
    g_current_internet_page = (g_current_internet_page - 1 + MAX_INTERNET_PAGES) % MAX_INTERNET_PAGES;
}

/**
 * @brief 处理返回按钮事件
 * @return 返回true表示应该退出菜单
 */
bool internet_menu_back_press()
{
    return true;
}

/**
 * @brief 互联网信息菜单的主屏幕函数
 * @details 包含主循环，处理用户输入并调用相应的绘制和数据获取函数。
 */
void InternetMenuScreen()
{
    internet_menu_init(); // 初始化
    while (true)
    {
        int rotation = readEncoder();
        bool button_clicked = readButton();
        bool long_press = readButtonLongPress();

        if (long_press)
        { // 长按退出
            internet_menu_back_press();
            return;
        }

        if (rotation != 0)
        { // 旋转编码器翻页
            if (rotation > 0)
            {
                internet_menu_next_page();
            }
            else
            {
                internet_menu_prev_page();
            }
            internet_menu_draw(); // 翻页后重绘
        }

        if (button_clicked)
        { // 短按按钮
            if (g_current_internet_page == 12)
            { // 特殊处理脑筋急转弯页面
                if (!g_show_brain_teaser_answer)
                {
                    g_show_brain_teaser_answer = true; // 显示答案
                }
                else
                {
                    g_brain_teaser_index++; // 切换到下一个问题
                    g_show_brain_teaser_answer = false;
                    if (g_brain_teaser_index >= g_brain_teaser_data.count)
                    {
                        tftClearLog();
                        tftLogInfo("Fetching new brain teasers...");
                        fetchBrainTeaser(); // 看完所有题目后重新获取
                    }
                }
            }
            else
            {
                // 对于其他页面，短按重新获取当前页面的数据
                tftClearLog();
                tftLogInfo("Re-fetching data for current page...");
                switch (g_current_internet_page)
                {
                case 0: fetchSayLove(); break;
                case 1: fetchEverydayEnglish(); break;
                case 2: fetchFortune(); break;
                case 3: fetchShici(); break;
                case 4: fetchDuilian(); break;
                case 5: fetchFxRate(); break;
                case 6: fetchRandomEnWord(); break;
                case 7: fetchYiyan(); break;
                case 8: fetchLzmy(); break;
                case 9: fetchVerse(); break;
                case 10: fetchTianqishiju(); break;
                case 11: fetchHsjz(); break;
                case 13: fetchHealthTip(); break;
                case 14: fetchGitHubTrending(); break;
                case 15: fetchHistory(); break;
                case 16: fetchTenWhy(); break;
                case 17: fetchonlineweather(); break;
                case 18: fetchStockData(); break;
                case 19: fetchCurrencyData(); break;
                }
            }
            internet_menu_draw(); // 重绘以显示新数据或新状态
        }

        vTaskDelay(pdMS_TO_TICKS(10)); // 短暂延时
    }
}
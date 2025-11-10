#include "space_api.h"
#include "System.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>

// --- 内部变量 ---
static SpaceData g_space_data; // 用于存储从API获取的所有空间相关数据的全局结构体
static unsigned long g_last_update = 0; // 上次更新数据的时间戳
static bool g_first_update = true; // 标志位，用于判断是否是模块的首次更新
const unsigned long UPDATE_INTERVAL = 2 * 60 * 1000; // 数据更新间隔（2分钟）

// --- API URL ---
const char* ASTRO_API_URL = "http://api.open-notify.org/astros.json"; // 获取当前在太空中的宇航员信息的API
const char* ISS_NOW_API_URL = "http://api.open-notify.org/iss-now.json";   // 获取国际空间站当前位置的API

// --- 全局变量 (从其他模块引用) ---
extern struct tm timeinfo; // 引用在别处定义的全局时间信息结构体
extern bool getLocalTime(struct tm* info, uint32_t ms); // 引用获取本地时间的函数
struct tm g_last_successful_update_time; // 用于记录上次成功更新数据的时间

// --- 内部函数声明 ---
static void fetchAstronauts();
static void fetchISSPosition();

// --- 公共函数实现 ---

/**
 * @brief 初始化空间API模块
 */
void space_api_init() {
    // 目前为空，可用于未来的初始化需求
}

/**
 * @brief 获取空间数据的只读指针
 * @return 返回一个指向全局 `g_space_data` 结构体的常量指针
 */
const SpaceData* space_api_get_data() {
    return &g_space_data;
}

/**
 * @brief 更新空间数据
 * @details 此函数检查是否到达了更新时间间隔。如果是，则调用内部函数从API获取最新的宇航员和ISS位置数据。
 *          首次更新时会显示详细的日志信息。
 * @return 如果执行了更新操作，返回true；否则返回false。
 */
bool space_api_update() {
    // 检查是否是首次更新或是否已超过更新间隔
    if (g_first_update || (millis() - g_last_update > UPDATE_INTERVAL)) {
        if (g_first_update) {
            tftLogInfo("--- Starting Space API Update ---");
        }
        g_last_update = millis(); // 更新最后更新时间戳

        // 调用函数分别获取宇航员和ISS位置数据
        fetchAstronauts();
        fetchISSPosition();
        
        if (g_first_update) {
            tftLogSuccess("--- Space API Update Finished ---");
            delay(2000); // 首次更新后暂停2秒，让用户看到日志
        }
        
        // 记录本次成功更新的时间
        getLocalTime(&g_last_successful_update_time, 0);

        g_first_update = false; // 关闭首次更新标志，后续更新将不再显示详细日志
        return true; // 表示成功执行了更新
    }
    return false; // 表示未到更新时间
}

// --- 内部函数实现 ---

/**
 * @brief 从API获取当前在太空中的宇航员信息
 */
static void fetchAstronauts() {
    HTTPClient http;
    if (g_first_update) tftLogInfo("Fetching astronaut data...");
    http.begin(ASTRO_API_URL);
    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        DynamicJsonDocument doc(1024);
        DeserializationError error = deserializeJson(doc, payload);

        if (error) {
            if (g_first_update) tftLogError("Astronaut JSON failed: " + String(error.c_str()));
            return;
        }

        // 解析JSON数据并填充到g_space_data结构体中
        g_space_data.astronaut_count = doc["number"];
        g_space_data.astronaut_list_count = 0;
        JsonArray people = doc["people"].as<JsonArray>();
        for (JsonObject person : people) {
            if (g_space_data.astronaut_list_count < 15) { // 最多存储15个宇航员信息
                g_space_data.astronauts[g_space_data.astronaut_list_count].name = person["name"].as<String>();
                g_space_data.astronauts[g_space_data.astronaut_list_count].craft = person["craft"].as<String>();
                g_space_data.astronaut_list_count++;
            }
        }
        if (g_first_update) tftLogDebug("Astronauts: " + String(g_space_data.astronaut_count));
    } else {
        if (g_first_update) tftLogError("Astronaut HTTP failed: " + String(httpCode));
    }
    http.end();
}

/**
 * @brief 从API获取国际空间站（ISS）的当前位置
 */
static void fetchISSPosition() {
    HTTPClient http;
    if (g_first_update) tftLogInfo("Fetching ISS position...");
    http.begin(ISS_NOW_API_URL);
    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        DynamicJsonDocument doc(256);
        DeserializationError error = deserializeJson(doc, payload);

        if (error) {
            if (g_first_update) tftLogError("ISS JSON failed: " + String(error.c_str()));
            return;
        }

        // 解析JSON数据并填充到g_space_data结构体中
        g_space_data.iss_lat = doc["iss_position"]["latitude"];
        g_space_data.iss_lon = doc["iss_position"]["longitude"];
        if (g_first_update) tftLogDebug("ISS Pos: " + String(g_space_data.iss_lat) + ", " + String(g_space_data.iss_lon));
    } else {
        if (g_first_update) tftLogError("ISS HTTP failed: " + String(httpCode));
    }
    http.end();
}
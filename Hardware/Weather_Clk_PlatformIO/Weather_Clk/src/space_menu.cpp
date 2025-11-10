#include "space_menu.h"
#include "space_api.h"
#include "System.h"
#include <TFT_eSPI.h> 
#include "RotaryEncoder.h"
#include "img.h"

// --- 全局变量 (从其他模块引用) ---
extern TFT_eSPI tft;
extern struct tm timeinfo; // 引用全局时间信息
extern bool getLocalTime(struct tm* info, uint32_t ms); // 引用获取本地时间的函数
extern struct tm g_last_successful_update_time; // 引用上次成功更新数据的时间

// --- 内部变量 ---
static int g_current_page = 0; // 当前显示的页面索引
static const int MAX_PAGES = 2; // 总页面数量

// --- 内部绘图函数声明 ---
static void draw_astronaut_page(const SpaceData* data);
static void draw_iss_page(const SpaceData* data);
static void draw_loading_page();
static void draw_page_indicator();
static void _draw_real_time_clock(); // 绘制实时时钟
static void _draw_status_bar();      // 绘制状态栏

/**
 * @brief 空间信息菜单的主屏幕函数
 * @details 这是空间信息功能的入口点。它包含一个主循环，负责：
 *          - 定期调用 `space_api_update()` 来获取最新数据。
 *          - 处理用户输入（旋转编码器翻页，按钮返回）。
 *          - 根据需要重绘屏幕。
 *          - 实时更新屏幕上的时钟。
 */
void SpaceMenuScreen() {
    extern void tftClearLog();
    tftClearLog(); // 清除可能存在的日志信息

    space_api_update(); // 首次进入时立即更新一次数据
    space_menu_draw();  // 绘制初始界面

    while(true) {
        bool data_just_updated = space_api_update(); // 尝试更新数据

        int rotation = readEncoder();
        bool button_pressed = readButton();
        bool needs_redraw = false;

        if (rotation != 0) { // 旋转编码器进行翻页
            if (rotation > 0) space_menu_next_page();
            else space_menu_prev_page();
            needs_redraw = true;
        }

        if (button_pressed) { // 按下按钮返回主菜单
            if (space_menu_back_press()) {
                break; 
            }
        }

        if (data_just_updated) { // 如果API数据刚刚更新了，也需要重绘
            needs_redraw = true;
        }

        if (needs_redraw) {
            space_menu_draw();
        }

        _draw_real_time_clock(); // 实时更新时钟显示

        delay(50); // 短暂延时
    }
    extern void showMenu();
    showMenu(); // 返回主菜单
}

// --- 页面导航函数实现 ---

/**
 * @brief 切换到下一页
 */
void space_menu_next_page() {
    g_current_page = (g_current_page + 1) % MAX_PAGES;
}

/**
 * @brief 切换到上一页
 */
void space_menu_prev_page() {
    g_current_page = (g_current_page - 1 + MAX_PAGES) % MAX_PAGES;
}

/**
 * @brief 处理返回按钮事件
 * @return 返回true表示应该退出菜单
 */
bool space_menu_back_press() {
    extern void tftClearLog();
    tftClearLog();
    return true; 
}

/**
 * @brief 绘制当前页面的主函数
 * @details 根据当前页面索引 `g_current_page` 调用相应的页面绘制函数。
 */
void space_menu_draw() {
    const SpaceData* data = space_api_get_data(); // 获取最新的空间数据

    switch (g_current_page) {
        case 0: draw_astronaut_page(data); break;
        case 1: draw_iss_page(data); break;
        default: break;
    }
    
    draw_page_indicator(); // 绘制页面指示器
    _draw_status_bar();      // 绘制状态栏
}

// --- 内部绘图函数实现 ---

/**
 * @brief 在屏幕左上角绘制实时时钟
 */
static void _draw_real_time_clock() {
    char time_str[9]; // HH:MM:SS\0
    getLocalTime(&timeinfo, 0);
    strftime(time_str, sizeof(time_str), "%H:%M:%S", &timeinfo);

    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.setTextFont(2);
    tft.setTextSize(1);
    tft.drawString(time_str, 5, 5);
}

/**
 * @brief 在屏幕顶部中心绘制状态栏（显示上次数据更新时间）
 */
static void _draw_status_bar() {
    char status_str[15];
    strftime(status_str, sizeof(status_str), "%H:%M:%S", &g_last_successful_update_time);

    tft.setTextDatum(TC_DATUM); 
    tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    tft.setTextSize(1);
    tft.setTextFont(2);
    tft.drawString(status_str, 120, 5);
}

/**
 * @brief 在屏幕右上角绘制页面指示器 (例如 "1 / 2")
 */
static void draw_page_indicator() {
    tft.setTextDatum(TR_DATUM); 
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextFont(2);
    String indicator = String(g_current_page + 1) + " / " + String(MAX_PAGES);
    tft.setTextSize(1);
    tft.drawString(indicator, 235, 5);
}

/**
 * @brief 绘制加载中页面
 */
static void draw_loading_page() {
    tft.fillScreen(TFT_BLACK);
    tft.setTextDatum(MC_DATUM); 
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString("Loading Space Data...", 120, 120);
}

/**
 * @brief 绘制宇航员信息页面
 * @param data 指向包含宇航员数据的 SpaceData 结构体的指针
 */
static void draw_astronaut_page(const SpaceData* data) {
    tft.fillScreen(TFT_BLACK);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.setTextSize(1);
    tft.drawString("Astronauts In Space", 120, 30);

    tft.setTextFont(7); // 使用大号字体显示人数
    tft.setTextColor(TFT_WHITE, TFT_BLACK);

    if (data->astronaut_count == -1) { // -1 通常表示数据无效
        tft.drawString("N/A", 120, 85);
    } else {
        tft.drawString(String(data->astronaut_count), 120, 85);
        
        // 列表显示宇航员姓名和飞船
        tft.setTextFont(2);
        tft.setTextDatum(TC_DATUM); 
        int list_y_start = 130;
        int line_height = 22;
        for(int i=0; i < data->astronaut_list_count && i < 5; i++) { // 最多显示5个
            String astronaut_info = String(i+1) + ". " + data->astronauts[i].name + " (" + data->astronauts[i].craft + ")";
            tft.drawString(astronaut_info, 120, list_y_start + (i * line_height));
        }
    }
}

/**
 * @brief 绘制国际空间站（ISS）位置页面
 * @param data 指向包含ISS位置数据的 SpaceData 结构体的指针
 */
static void draw_iss_page(const SpaceData* data) {
    tft.fillScreen(TFT_BLACK);

    // 绘制世界地图背景
    int map_y_offset = (240 - 134) / 2; // 垂直居中地图
    tft.pushImage(0, map_y_offset, 240, 134, map_img);

    // 绘制标题
    tft.setTextDatum(TC_DATUM);
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.setTextFont(2);
    tft.setTextSize(1);
    tft.drawString("ISS Real-time Position", 120, 30);

    tft.setTextDatum(BC_DATUM);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);

    if (data->astronaut_count == -1) { // 如果数据无效
        tft.drawString("Data not available", 120, 225);
    } else {
        // 将经纬度坐标转换为屏幕坐标
        int iss_x = (int)(120 + (data->iss_lon * (240.0 / 360.0)));
        int iss_y = (int)((90.0 - data->iss_lat) * (134.0 / 180.0));

        // 在地图上绘制ISS的位置标记（一个红点加一个白圈）
        tft.fillCircle(iss_x, iss_y + map_y_offset, 4, TFT_RED);
        tft.drawCircle(iss_x, iss_y + map_y_offset, 5, TFT_WHITE);

        // 在底部显示经纬度文本
        String lat_lon_str = "Lat: " + String(data->iss_lat, 2) + " Lon: " + String(data->iss_lon, 2);
        tft.drawString(lat_lon_str, 120, 225);
    }
}
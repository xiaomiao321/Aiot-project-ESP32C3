#ifndef SPACE_API_H
#define SPACE_API_H

#include <Arduino.h>

// 用于存储宇航员信息
struct Astronaut {
    String name;
    String craft;
};

// 统一存储所有太空数据
struct SpaceData {
    // Page 1: Astronauts
    int astronaut_count = -1; // 默认为-1，表示尚未获取
    Astronaut astronauts[15];
    int astronaut_list_count = 0;

    // Page 2: ISS Position
    float iss_lat = 0;
    float iss_lon = 0;
};


// --- 函数声明 ---

/**
 * @brief 初始化太空API模块。
 * @details 此函数目前为空，为未来可能的初始化需求保留。
 */
void space_api_init();

/**
 * @brief 更新太空相关数据。
 * @details 此函数会检查自上次更新以来是否已超过设定的时间间隔（`UPDATE_INTERVAL`）。
 *          如果需要更新，它将调用内部函数从API获取最新的在轨宇航员信息和国际空间站（ISS）的实时位置。
 * @return 如果执行了网络更新，则返回 `true`；如果未到更新时间，则返回 `false`。
 */
bool space_api_update();

/**
 * @brief 获取太空数据的只读指针。
 * @details 提供对当前存储的 `SpaceData` 结构体的访问，该结构体包含宇航员和ISS位置等信息。
 * @return 返回一个指向 `SpaceData` 结构体的常量指针。
 */
const SpaceData* space_api_get_data();

extern struct tm g_last_successful_update_time; // Added: Last successful update time

#endif // SPACE_API_H

#ifndef PERFORMANCE_H
#define PERFORMANCE_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include "img.h"

// -----------------------------
// 宏定义
// -----------------------------
#define BG_COLOR       0x0000
#define TITLE_COLOR    0xFFFF
#define VALUE_COLOR    0xFFFF
#define ERROR_COLOR    0xF800
#define LINE_HEIGHT    24
#define DATA_X         95
#define DATA_Y         10
#define LOGO_X         5
#define LOGO_Y_TOP     5
#define LOGO_Y_BOTTOM  75
#define BUFFER_SIZE    512

// Combined Chart Dimensions and Position
#define COMBINED_CHART_WIDTH    200
#define COMBINED_CHART_HEIGHT   80
#define COMBINED_CHART_X        20
#define COMBINED_CHART_Y        155

#define VALUE_OFFSET_X 40
#define VALUE_WIDTH    100
#define LEGEND_RADIUS  5
#define LEGEND_TEXT_OFFSET 8
#define LEGEND_X_START 95 // Shifted to the right
#define LEGEND_Y_POS 110
#define LEGEND_SPACING_WIDE 80
#define LEGEND_LINE_HEIGHT 15

// Arc display settings
#define ARC_CPU_X 40
#define ARC_CPU_Y 60
#define ARC_GPU_X 120
#define ARC_GPU_Y 60
#define ARC_RADIUS_MAX 35
#define ARC_RADIUS_MIN 15
#define ARC_THICKNESS 8
#define ARC_BG_COLOR TFT_DARKGREY
#define ARC_CPU_COLOR TFT_GREEN
#define ARC_GPU_COLOR TFT_BLUE


struct PCData
{
    char cpuName[64];
    char gpuName[64];
    int cpuTemp;
    int cpuLoad;
    int gpuTemp;
    int gpuLoad;
    float ramLoad;
    bool valid; // 数据是否有效
};
// -----------------------------
// 函数声明
// -----------------------------
void startPerformanceMonitoring();

/**
 * @brief 性能监控菜单的入口函数。
 * @details 此函数作为性能监控功能的主入口，负责清空屏幕、
 *          创建并启动用于界面初始化、数据更新和串口通信的FreeRTOS任务。
 *          它会持续运行，直到用户通过按钮操作或特定标志位退出子菜单。
 */
void performanceMenu();

/**
 * @brief 绘制性能监控界面的静态元素。
 * @details 该函数负责在TFT屏幕上绘制所有不会随数据更新而改变的静态内容，
 *          包括背景、Logo、数据标签（如 "CPU:", "GPU:"）、图表框架和图例。
 */
void drawPerformanceStaticElements();

/**
 * @brief 更新并显示实时性能数据。
 * @details 此函数从全局数据结构中获取最新的PC性能指标（CPU/GPU负载和温度、RAM使用率）
 *          以及ESP32自身的温度，然后将这些动态数据更新到TFT屏幕的相应位置，并更新图表。
 */
void updatePerformanceData();

/**
 * @brief 在屏幕上显示错误信息。
 * @param msg 要显示的错误消息字符串。
 * @details 当无法获取或解析性能数据时，调用此函数在屏幕上显示指定的错误提示。
 */
void showPerformanceError(const char *msg);

/**
 * @brief 重置串口接收缓冲区。
 * @details 该函数将串口数据接收缓冲区的索引清零，并置入字符串结束符，
 *          为下一次接收新的数据字符串做准备。
 */
void resetBuffer();

/**
 * @brief 解析从PC接收的性能数据。
 * @details 此函数处理通过串口接收到的原始字符串，
 *          使用字符串匹配和转换来提取CPU/GPU的名称、温度、负载以及RAM使用率等信息，
 *          并更新到全局的PCData结构体中。解析成功后会触发蜂鸣器提示。
 */
void parsePCData();

/**
 * @brief [FreeRTOS Task] 性能监控界面的初始化任务。
 * @param pvParameters 任务创建时传入的参数（未使用）。
 * @details 这是一个一次性执行的FreeRTOS任务，调用 `drawPerformanceStaticElements()`
 *          函数来完成界面的静态布局绘制，任务完成后将自行删除。
 */
void Performance_Init_Task(void *pvParameters);

/**
 * @brief [FreeRTOS Task] 性能数据显示更新任务。
 * @param pvParameters 任务创建时传入的参数（未使用）。
 * @details 这是一个周期性执行的FreeRTOS任务，它定时读取ESP32的内部温度，
 *          然后调用 `updatePerformanceData()` 函数刷新屏幕上的所有动态性能数据。
 */
void Performance_Task(void *pvParameters);

/**
 * @brief [FreeRTOS Task] 串口数据接收任务。
 * @param pvParameters 任务创建时传入的参数（未使用）。
 * @details 这是一个持续运行的FreeRTOS任务，负责监听并从串口读取数据。
 *          当接收到完整的一行数据（以换行符结尾）时，它会调用 `parsePCData()`
 *          函数进行数据解析，然后重置缓冲区。
 */
void SERIAL_Task(void *pvParameters);

#endif

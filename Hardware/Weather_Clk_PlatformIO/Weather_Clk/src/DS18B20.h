#ifndef __DS18B20_H
#define __DS18B20_H

extern float g_currentTemperature;

/**
 * @brief 初始化DS18B20温度传感器。
 * @details 此函数调用DallasTemperature库的begin方法，启动传感器，为后续的温度读取做准备。
 */
void DS18B20_Init();

/**
 * @brief 创建一个后台任务以持续更新温度数据。
 * @details 该函数使用FreeRTOS的xTaskCreate创建一个独立的任务，
 *          该任务会周期性地（每2秒）请求并更新全局的温度变量`g_currentTemperature`。
 */
void createDS18B20Task();

/**
 * @brief 获取最新的DS18B20温度读数。
 * @return 返回一个float类型的温度值（摄氏度）。
 *         该值由后台任务`updateTempTask`更新。
 */
float getDS18B20Temp();

/**
 * @brief DS18B20温度监控菜单的入口函数。
 * @details 此函数负责启动一个在TFT屏幕上显示实时温度和历史曲线的任务。
 *          同时，它会监听退出信号（如按钮按下或闹钟触发），并负责安全地停止和清理相关任务。
 */
void DS18B20Menu();

#endif
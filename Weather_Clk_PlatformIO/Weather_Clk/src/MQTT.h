#ifndef MQTT_H
#define MQTT_H

#include <Arduino.h>
#include "performance.h"

// --- Action Request System ---
// Used to request UI-blocking actions from a non-UI task (like MQTT callback)
// The main loop will check these and execute the action.
extern volatile void (*requestedSongAction)(int);
extern volatile int requestedSongIndex;

extern volatile bool exitSubMenu; 

/**
 * @brief 初始化MQTT客户端。
 * @details 该函数设置MQTT服务器地址、端口号和消息回调函数。
 *          同时，它会订阅用于接收命令和消息的主题。
 */
void setupMQTT();

/**
 * @brief 显示MQTT连接过程的可视化界面。
 * @details 此函数会在TFT屏幕上实时显示连接到MQTT服务器的状态和进度，
 *          包括连接尝试、成功或失败等信息，提供友好的用户交互体验。
 */
void connectMQTT();

/**
 *@brief 向华为云发布数据
 *
 * @param Temp DS18B20温度数据
 * @param pcdata 电脑信息数据
 * @param lux 光照强度数据
 */
void publishData(float Temp, struct PCData *pcdata, float lux);

#endif

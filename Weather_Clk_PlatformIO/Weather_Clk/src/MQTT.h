#ifndef MQTT_H
#define MQTT_H

#include <Arduino.h>


extern volatile void (*requestedMenuAction)();
extern volatile bool exitSubMenu; // 这两个都是之前想法用到的，现在不需要了，但是在很多地方引用了，于是保留在这里

/**
 * @brief 初始化MQTT客户端。
 * @details 该函数设置MQTT服务器地址、端口号和消息回调函数。
 *          同时，它会订阅用于接收命令和消息的主题。
 */
void setupMQTT();

/**
 * @brief 重新连接到MQTT服务器。
 * @details 当客户端与MQTT服务器的连接断开时，此函数将尝试重新建立连接。
 *          如果连接成功，它会重新订阅之前的主题。
 */
void reconnect();

/**
 * @brief 显示MQTT连接过程的可视化界面。
 * @details 此函数会在TFT屏幕上实时显示连接到MQTT服务器的状态和进度，
 *          包括连接尝试、成功或失败等信息，提供友好的用户交互体验。
 */
void connectMQTT();

/**
 * @brief 维护MQTT连接的循环函数。
 * @details 该函数应在主循环`loop()`中被反复调用。
 *          它负责检查客户端是否仍连接到MQTT服务器，如果断开则会触发重连机制。
 */
void loopMQTT();

/**
 * @brief 发布DS18B20传感器的温度数据。
 * @details 此函数从DS18B20温度传感器读取当前的温度值，
 *          然后将此数据封装成JSON格式，并通过MQTT发布到预设的主题上。
 */
void publishDS18B20SensorData(float Temp);

#endif

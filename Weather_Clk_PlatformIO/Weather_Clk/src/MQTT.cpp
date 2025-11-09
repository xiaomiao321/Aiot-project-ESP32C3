#include "MQTT.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include "Menu.h"
#include "Buzzer.h"
#include "System.h"
#include "DS18B20.h"
#include "performance.h"
// --- 配置信息 ---

// WiFi网络配置
#define WIFI_SSID "xiaomiao_hotspot"
#define WIFI_PASSWORD "xiaomiao123"

// 华为云物联网平台设备信息 (三元组)
#define DEVICE_ID        "690a1b110a9ab42ae58b933d_Weather_Clock_Dev"
#define DEVICE_SECRET    "xiaomiao123" // 设备密钥
const char *CLIENT_ID = "690a1b110a9ab42ae58b933d_Weather_Clock_Dev_0_0_2025110415"; // MQTT客户端ID，通常包含时间戳
const char *MQTT_USER = "690a1b110a9ab42ae58b933d_Weather_Clock_Dev"; // MQTT用户名
const char *MQTT_PASSWORD = "d227e5b623d1b403795dff448bb8535bfdca0b4e6a9edd86b8dda7cead3c4015"; // 通过工具生成的MQTT密码
const char *MQTT_SERVER = "827782c6ea.st1.iotda-device.cn-east-3.myhuaweicloud.com";  // 华为云MQTT服务器地址
const int   MQTT_PORT = 1883; // MQTT端口

// MQTT主题和消息格式定义
#define SERVICE_ID        "\"Property\"" // 物模型中的服务ID
#define MQTT_BODY_FORMAT  "{\"services\":[{\"service_id\":" SERVICE_ID ",\"properties\":{%s"
#define MQTT_TOPIC_REPORT      "$oc/devices/" DEVICE_ID "/sys/properties/report"           // 属性上报主题
#define MQTT_TOPIC_GET         "$oc/devices/" DEVICE_ID "/sys/messages/down"               // 平台下行消息主题
#define MQTT_TOPIC_COMMANDS    "$oc/devices/" DEVICE_ID "/sys/commands/"                   // 平台命令下发主题
#define MQTT_TOPIC_CMD_RESPONSE "$oc/devices/" DEVICE_ID "/sys/commands/response/request_id="  // 命令响应主题
#define RESPONSE_DATA     "{\"result_code\": 0,\"response_name\": \"COMMAND_RESPONSE\",\"paras\": {\"result\": \"success\"}}" // 通用成功响应

// --- 全局变量 ---
WiFiClient espClient; // WiFi客户端实例
PubSubClient client(espClient); // PubSubClient实例，用于处理MQTT协议
long lastMqttReconnectAttempt = 0; // 上次尝试重连MQTT的时间戳
long Time = 0; // 一个简单的时间计数器，用于上报数据
long lastReportTime = 0;       // 上次上报时间
volatile bool exitSubMenu = false; // 全局子菜单退出标志
extern float g_currentTemperature;
extern float g_lux;
extern struct PCData pcData;
// --- 函数前向声明 ---
void callback(char *topic, byte *payload, unsigned int length);
void reconnect();
void connectMQTT();
void sendCommandResponse(char *topic);

/**
 * @brief 初始化MQTT客户端
 */
void setupMQTT()
{
  client.setServer(MQTT_SERVER, MQTT_PORT); // 设置MQTT服务器和端口
  client.setCallback(callback); // 设置接收消息的回调函数

  // 订阅需要的主题
  client.subscribe(MQTT_TOPIC_COMMANDS);
  client.subscribe(MQTT_TOPIC_GET);
}

/**
 * @brief MQTT主循环函数
 * @details 在主`loop()`中被调用，用于保持MQTT客户端的连接和处理消息。
 */
void loopMQTT()
{
  if (!client.connected()) // 如果客户端断开连接
  {
    reconnect();
  }
  client.loop(); // 保持客户端心跳和处理传入消息
  // 定时上报数据(每5秒)
  long currentTime = millis();
  if (currentTime - lastReportTime > 2000)
  {
    lastReportTime = currentTime;
    publishData(g_currentTemperature, &pcData, g_lux);
  }
}

/**
 * @brief 重新连接到MQTT服务器
 */
void reconnect()
{
  Serial.println("尝试连接到MQTT服务器...");
  if (client.connect(CLIENT_ID, MQTT_USER, MQTT_PASSWORD))
  {
    Serial.println("MQTT连接成功");
    // 重新订阅主题
    client.subscribe(MQTT_TOPIC_COMMANDS);
    client.subscribe(MQTT_TOPIC_GET);
  }
  else
  {
    Serial.print("failed, rc=");
    Serial.print(client.state());
    Serial.print(" - ");
    // 打印详细的错误信息
    switch (client.state())
    {
    case -4: Serial.print("MQTT_CONNECTION_TIMEOUT"); break;
    case -3: Serial.print("MQTT_CONNECTION_LOST"); break;
    case -2: Serial.print("MQTT_CONNECT_FAILED"); break;
    case -1: Serial.print("MQTT_DISCONNECTED"); break;
    case 1: Serial.print("MQTT_CONNECT_BAD_PROTOCOL"); break;
    case 2: Serial.print("MQTT_CONNECT_BAD_CLIENT_ID"); break;
    case 3: Serial.print("MQTT_CONNECT_UNAVAILABLE"); break;
    case 4: Serial.print("MQTT_CONNECT_BAD_CREDENTIALS"); break;
    case 5: Serial.print("MQTT_CONNECT_UNAUTHORIZED"); break;
    default: Serial.print("UNKNOWN_ERROR"); break;
    }
    Serial.println(" 5秒后重试");
  }
}

/**
 * @brief 连接到MQTT（带有可视化界面）
 * @details 在屏幕上显示连接过程和日志，提供更友好的用户体验。
 */
void connectMQTT()
{
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(2);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("Connecting MQTT...", 120, 20);
  tft_log_y = 40; // 重置屏幕日志的Y坐标

  char log_buffer[100];
  tftLog("========= MQTT Connect ========", TFT_YELLOW);
  sprintf(log_buffer, "Broker: %s:%d", MQTT_SERVER, MQTT_PORT);
  tftLog(log_buffer, TFT_CYAN);

  int attempts = 0;
  int max_attempts = 5;

  while (!client.connected() && attempts < max_attempts)
  {
    // 绘制并更新连接进度条
    tft.drawRect(20, tft.height() - 20, 202, 17, TFT_WHITE);
    tft.fillRect(21, tft.height() - 18, (attempts + 1) * (200 / max_attempts), 13, TFT_GREEN);

    sprintf(log_buffer, "Attempt %d/%d...", attempts + 1, max_attempts);
    tftLog(log_buffer, TFT_YELLOW);

    if (client.connect(CLIENT_ID, MQTT_USER, MQTT_PASSWORD))
    {
      tftLogSuccess("MQTT Connected!");
      client.subscribe(MQTT_TOPIC_COMMANDS);
      client.subscribe(MQTT_TOPIC_GET);
      sprintf(log_buffer, "Subscribed to: %s", MQTT_TOPIC_COMMANDS);
      tftLog(log_buffer, TFT_GREEN);
    }
    else
    {
      tftLogError("MQTT Connection Failed!");
      sprintf(log_buffer, "State: %d", client.state());
      tftLog(log_buffer, TFT_RED);
      // 在屏幕上显示详细错误
      switch (client.state())
      {
      case -4: tftLogError("Timeout"); break;
      case -2: tftLogError("Connect Failed"); break;
      case 4: tftLogError("Bad Credentials"); break;
      default: tftLogError("Unknown Error"); break;
      }
      delay(2000);
    }
    attempts++;
  }

  if (client.connected())
  {
    tftLogSuccess("Setup Complete");
    delay(1500);
  }
  else
  {
    tftLogError("Failed to connect MQTT after all attempts.");
    delay(3000);
  }
}

/**
 *@brief 向华为云发布数据
 *
 * @param Temp DS18B20温度数据
 * @param pcdata 电脑信息数据
 * @param lux 光照强度数据
 */
void publishData(float Temp, struct PCData *pcdata, float lux)
{
  Time++;
  Serial.printf("温度: %.2f *C\n", Temp);

  // 构建JSON消息
  char properties[128];
  char jsonBuf[256];
  sprintf(properties, "\"Temperature\":%.2f,\"Time\":%ld,\"Lux\":%d,\"GPULoad\":%d,\"CPULoad\":%d,\"GPUTemp\":%d,\"RAMLoad\":%d}}}", Temp, Time, g_lux, pcdata->gpuLoad, pcdata->cpuLoad, pcdata->gpuTemp, pcdata->ramLoad);
  sprintf(jsonBuf, MQTT_BODY_FORMAT, properties);

  // 发布消息到属性上报主题
  client.publish(MQTT_TOPIC_REPORT, jsonBuf);
  Serial.println("上报数据:");
  Serial.println(jsonBuf);
  Serial.println("MQTT数据上报成功");
}


/**
 * @brief MQTT消息回调函数
 * @param topic 接收到消息的主题
 * @param payload 消息内容
 * @param length 消息长度
 * @details 当客户端接收到已订阅主题的消息时，此函数被调用。
 */
void callback(char *topic, byte *payload, unsigned int length)
{
  Serial.println("\n收到平台下发消息:");
  Serial.print("主题: ");
  Serial.println(topic);

  payload[length] = '\0'; // 确保payload以空字符结尾
  Serial.print("内容: ");
  Serial.println((char *) payload);

  // 如果是命令下发主题
  if (strstr(topic, MQTT_TOPIC_COMMANDS))
  {
    DynamicJsonDocument doc(256);
    DeserializationError error = deserializeJson(doc, payload);
    if (error)
    {
      Serial.println("JSON解析失败");
      return;
    }

    // 从主题中提取 request_id
    char requestId[100] = { 0 };
    char *pstr = strstr(topic, "request_id=");
    if (pstr)
    {
      strcpy(requestId, pstr + strlen("request_id="));
    }

    Serial.print("Request ID: ");
    Serial.println(requestId);

    // 构建响应主题并发送响应
    char responseTopic[200] = { 0 };
    strcat(responseTopic, MQTT_TOPIC_CMD_RESPONSE);
    strcat(responseTopic, requestId);
    sendCommandResponse(responseTopic);
  }
  // 如果是其他下行消息
  else if (strstr(topic, MQTT_TOPIC_GET))
  {
    Serial.println("收到下行消息，未处理");
  }
}

/**
 * @brief 发送命令响应到平台
 * @param topic 响应应该发送到的主题
 */
void sendCommandResponse(char *topic)
{
  if (client.publish(topic, RESPONSE_DATA))
  {
    Serial.println("命令响应发送成功");
  }
  else
  {
    Serial.println("命令响应发送失败");
  }
  Serial.println("------------------------");
}

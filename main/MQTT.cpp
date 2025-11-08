#include "MQTT.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include "Menu.h" // Include Menu.h to access the menuItems array
#include "Buzzer.h"
#include "System.h" // For TFT logging functions
#include "DS18B20.h"
// --- Configuration ---
// WiFi Configuration
#define WIFI_SSID "xiaomiao_hotspot"
#define WIFI_PASSWORD "xiaomiao123"

// 华为云设备信息（三元组）
// 三元组生成链接：https://iot-tool.obs-website.cn-north-4.myhuaweicloud.com/
#define DEVICE_ID        "690a1b110a9ab42ae58b933d_Weather_Clock_Dev"
#define DEVICE_SECRET    "xiaomiao123"
const char *CLIENT_ID = "690a1b110a9ab42ae58b933d_Weather_Clock_Dev_0_0_2025110415";
const char *MQTT_USER = "690a1b110a9ab42ae58b933d_Weather_Clock_Dev";
const char *MQTT_PASSWORD = "d227e5b623d1b403795dff448bb8535bfdca0b4e6a9edd86b8dda7cead3c4015";
const char *MQTT_SERVER = "827782c6ea.st1.iotda-device.cn-east-3.myhuaweicloud.com";  // 华为云MQTT服务器地址
const int   MQTT_PORT = 1883;

// MQTT主题和消息格式定义
#define SERVICE_ID        "\"Arduino\""
#define MQTT_BODY_FORMAT  "{\"services\":[{\"service_id\":" SERVICE_ID ",\"properties\":{%s"
#define MQTT_TOPIC_REPORT      "$oc/devices/" DEVICE_ID "/sys/properties/report"           // 属性上报主题
#define MQTT_TOPIC_GET         "$oc/devices/" DEVICE_ID "/sys/messages/down"               // 下行消息主题
#define MQTT_TOPIC_COMMANDS    "$oc/devices/" DEVICE_ID "/sys/commands/"                   // 命令下发主题
#define MQTT_TOPIC_CMD_RESPONSE "$oc/devices/" DEVICE_ID "/sys/commands/response/request_id="  // 命令响应主题
#define RESPONSE_DATA     "{\"result_code\": 0,\"response_name\": \"COMMAND_RESPONSE\",\"paras\": {\"result\": \"success\"}}"
// --- Global Variables ---
WiFiClient espClient;
PubSubClient client(espClient);
long lastMqttReconnectAttempt = 0;
long Time = 0;
// Definition of the volatile function pointer
volatile void (*requestedMenuAction)() = nullptr;
volatile bool exitSubMenu = false; // Definition for the exit flag

// --- Forward Declarations ---
void callback(char *topic, byte *payload, unsigned int length);
void reconnect();
void connectMQTT(); // New function for visual MQTT connection

// --- Core Functions ---

void setupMQTT()
{
  client.setServer(MQTT_SERVER, MQTT_PORT);
  client.setCallback(callback);

  // 订阅必要的主题
  client.subscribe(MQTT_TOPIC_COMMANDS);
  client.subscribe(MQTT_TOPIC_GET);
}

void loopMQTT()
{
  if (!client.connected())
  {
    long now = millis();
    // Try to reconnect every 5 seconds without blocking.
    if (now - lastMqttReconnectAttempt > 5000)
    {
      lastMqttReconnectAttempt = now;
      reconnect();
    }
  }
}

// --- Helper Functions ---

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
    // Add detailed error descriptions
    switch (client.state())
    {
    case -4:
      Serial.print("MQTT_CONNECTION_TIMEOUT");
      break;
    case -3:
      Serial.print("MQTT_CONNECTION_LOST");
      break;
    case -2:
      Serial.print("MQTT_CONNECT_FAILED");
      break;
    case -1:
      Serial.print("MQTT_DISCONNECTED");
      break;
    case 1:
      Serial.print("MQTT_CONNECT_BAD_PROTOCOL");
      break;
    case 2:
      Serial.print("MQTT_CONNECT_BAD_CLIENT_ID");
      break;
    case 3:
      Serial.print("MQTT_CONNECT_UNAVAILABLE");
      break;
    case 4:
      Serial.print("MQTT_CONNECT_BAD_CREDENTIALS");
      break;
    case 5:
      Serial.print("MQTT_CONNECT_UNAUTHORIZED");
      break;
    default:
      Serial.print("UNKNOWN_ERROR");
      break;
    }
    Serial.println(" try again in 5 seconds");
  }
}

void connectMQTT()
{
  tft.fillScreen(TFT_BLACK); // Use TFT_BLACK from TFT_eSPI.h
  tft.setTextSize(2);
  tft.setTextDatum(MC_DATUM); // Center datum
  tft.drawString("Connecting MQTT...", 120, 20);
  tft_log_y = 40; // Reset log Y position

  char log_buffer[100];
  tftLog("========= MQTT Connect ========", TFT_YELLOW);
  sprintf(log_buffer, "Broker: %s:%d", MQTT_SERVER, MQTT_PORT);
  tftLog(log_buffer, TFT_CYAN);
  sprintf(log_buffer, "Client ID: %s", CLIENT_ID);
  tftLog(log_buffer, TFT_CYAN);

  int attempts = 0;
  int max_attempts = 5;

  while (!client.connected() && attempts < max_attempts)
  {
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
      // Add detailed error descriptions
      switch (client.state())
      {
      case -4: tftLogError("MQTT_CONNECTION_TIMEOUT"); break;
      case -3: tftLogError("MQTT_CONNECTION_LOST"); break;
      case -2: tftLogError("MQTT_CONNECT_FAILED"); break;
      case -1: tftLogError("MQTT_DISCONNECTED"); break;
      case 1: tftLogError("MQTT_CONNECT_BAD_PROTOCOL"); break;
      case 2: tftLogError("MQTT_CONNECT_BAD_CLIENT_ID"); break;
      case 3: tftLogError("MQTT_CONNECT_UNAVAILABLE"); break;
      case 4: tftLogError("MQTT_CONNECT_BAD_CREDENTIALS"); break;
      case 5: tftLogError("MQTT_CONNECT_UNAUTHORIZED"); break;
      default: tftLogError("UNKNOWN_ERROR"); break;
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

void publishDS18B20SensorData()
{
  Time++;
  // 读取传感器数据
  float Temp = getDS18B20Temp();

  // 打印传感器数据到串口
  Serial.printf("温度: %.2f *C\n", Temp);
  // 构建JSON消息
  char properties[256];
  char jsonBuf[512];

  sprintf(properties,
    "\"Temperature\":%.2f,\"Time\":%d}}]}",
    Temperature, Time);

  sprintf(jsonBuf, MQTT_BODY_FORMAT, properties);

  // 发布消息
  client.publish(MQTT_TOPIC_REPORT, jsonBuf);
  Serial.println("上报数据:");
  Serial.println(jsonBuf);
  Serial.println("MQTT数据上报成功");
}

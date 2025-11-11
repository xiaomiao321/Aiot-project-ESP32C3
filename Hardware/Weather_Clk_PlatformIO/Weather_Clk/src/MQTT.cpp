#include "MQTT.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include "Menu.h"
#include "Buzzer.h"
#include "System.h"
#include "DS18B20.h"
#include "performance.h"
#include "LED.h"
#include "Alarm.h"
#include "MusicMenuLite.h"

// --- 配置信息 ---
#define WIFI_SSID "xiaomiao_hotspot"
#define WIFI_PASSWORD "xiaomiao123"

#define DEVICE_ID        "690a1b110a9ab42ae58b933d_Weather_Clock_Dev"
#define DEVICE_SECRET    "xiaomiao123"
const char *CLIENT_ID = "690a1b110a9ab42ae58b933d_Weather_Clock_Dev_0_0_2025110415";
const char *MQTT_USER = "690a1b110a9ab42ae58b933d_Weather_Clock_Dev";
const char *MQTT_PASSWORD = "d227e5b623d1b403795dff448bb8535bfdca0b4e6a9edd86b8dda7cead3c4015";
const char *MQTT_SERVER = "827782c6ea.st1.iotda-device.cn-east-3.myhuaweicloud.com";
const int   MQTT_PORT = 1883;

#define SERVICE_ID        "\"Property\""
#define MQTT_BODY_FORMAT  "{\"services\":[{\"service_id\":" SERVICE_ID ",\"properties\":{%s}}]}"
#define MQTT_TOPIC_REPORT      "$oc/devices/" DEVICE_ID "/sys/properties/report"
#define MQTT_TOPIC_GET         "$oc/devices/" DEVICE_ID "/sys/messages/down"
#define MQTT_TOPIC_COMMANDS    "$oc/devices/" DEVICE_ID "/sys/commands/"
#define MQTT_TOPIC_CMD_RESPONSE "$oc/devices/" DEVICE_ID "/sys/commands/response/request_id="
#define RESPONSE_DATA     "{\"result_code\": 0,\"response_name\": \"COMMAND_RESPONSE\",\"paras\": {\"result\": \"success\"}}"

// --- 全局变量 ---
WiFiClient espClient;
PubSubClient client(espClient);
long Time = 0;
volatile bool exitSubMenu = false;
extern float g_currentTemperature;
extern float g_lux;
extern struct PCData pcData;
extern float esp32c3_temp;

// --- 函数前向声明 ---
void callback(char *topic, byte *payload, unsigned int length);
void sendCommandResponse(char *topic);
void MQTT_Task(void *pvParameters);

/**
 * @brief 初始化MQTT客户端并启动MQTT主任务
 */
void setupMQTT()
{
  client.setServer(MQTT_SERVER, MQTT_PORT);
  client.setCallback(callback);

  // 创建并启动集成的MQTT任务
  xTaskCreate(MQTT_Task, "MQTT_Task", 4096, NULL, 5, NULL);
}

/**
 * @brief [FreeRTOS Task] MQTT主任务
 * @details 此任务全权负责所有MQTT相关活动：
 *          - 保持与MQTT服务器的连接。
 *          - 如果断开连接，则使用阻塞方式重连。
 *          - 循环处理传入和传出的MQTT消息。
 *          - 定时上报传感器数据。
 */
void MQTT_Task(void *pvParameters)
{
  long lastReportTime = 0;

  for (;;)
  {
    // 检查WiFi和MQTT连接，如果断开则循环重连
    if (!client.connected())
    {
      Serial.println("MQTT断开连接，尝试重连...");
      while (!client.connected())
      {
        if (client.connect(CLIENT_ID, MQTT_USER, MQTT_PASSWORD))
        {
          Serial.println("MQTT连接成功!");
          // 重新订阅主题
          client.subscribe(MQTT_TOPIC_COMMANDS);
          client.subscribe(MQTT_TOPIC_GET);
        }
        else
        {
          Serial.printf("MQTT连接失败, rc=%d. 3秒后重试\n", client.state());
          // 等待3秒再重试，避免频繁失败导致系统不稳定
          vTaskDelay(pdMS_TO_TICKS(3000));
        }
      }
    }

    // 保持客户端心跳和处理传入消息
    client.loop();

    // 定时上报数据(每1秒)
    if (millis() - lastReportTime > 1000)
    {
      lastReportTime = millis();
      publishData(g_currentTemperature, &pcData, g_lux);
    }

    // 短暂延时，让出CPU给其他任务
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}


/**
 * @brief 连接到MQTT（带有可视化界面）
 * @details 此函数现在仅用于初次启动时显示连接动画，实际的连接逻辑在MQTT_Task中。
 */
void connectMQTT()
{
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(2);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("Connecting MQTT...", 120, 20);
  tft_log_y = 40;

  char log_buffer[100];
  tftLog("========= MQTT Connect ========", TFT_YELLOW);
  sprintf(log_buffer, "Broker: %s:%d", MQTT_SERVER, MQTT_PORT);
  tftLog(log_buffer, TFT_CYAN);

  // 等待MQTT_Task建立连接
  int wait_time = 0;
  while (!client.connected() && wait_time < 15000) // 最多等待15秒
  {
    tft.drawRect(20, tft.height() - 20, 202, 17, TFT_WHITE);
    tft.fillRect(21, tft.height() - 18, (wait_time / 1000) * (200 / 15), 13, TFT_GREEN);
    vTaskDelay(pdMS_TO_TICKS(100));
    wait_time += 100;
  }

  if (client.connected())
  {
    tftLogSuccess("MQTT Connected!");
    delay(1500);
  }
  else
  {
    tftLogError("MQTT Connection Failed!");
    delay(3000);
  }
}

/**
 *@brief 向华为云发布数据
 */
void publishData(float Temp, struct PCData *pcdata, float lux)
{
  Time++;

  char properties[256];
  char jsonBuf[300];
  sprintf(properties, "\"Temperature\":%.2f,\"Time\":%ld,\"Lux\":%.2f,\"GPULoad\":%d,\"CPULoad\":%d,\"GPUTemp\":%d,\"RAMLoad\":%.1f,\"ESP32Temp\":%.1f",
    Temp, Time, lux, pcdata->gpuLoad, pcdata->cpuLoad, pcdata->gpuTemp, pcdata->ramLoad, esp32c3_temp);
  sprintf(jsonBuf, MQTT_BODY_FORMAT, properties);

  if (client.publish(MQTT_TOPIC_REPORT, jsonBuf))
  {
    Serial.println("上报数据:");
    Serial.println(jsonBuf);
    Serial.println("MQTT数据上报成功");
  }
  else
  {
    Serial.println("MQTT数据上报失败");
  }
}


/**
 * @brief MQTT消息回调函数
 */
void callback(char *topic, byte *payload, unsigned int length)
{
  Serial.println("\n收到平台下发消息:");
  Serial.print("主题: ");
  Serial.println(topic);

  payload[length] = '\0';
  Serial.print("内容: ");
  Serial.println((char *) payload);

  if (strstr(topic, MQTT_TOPIC_COMMANDS))
  {
    DynamicJsonDocument doc(512);
    DeserializationError error = deserializeJson(doc, payload);
    if (error)
    {
      Serial.print("JSON解析失败: ");
      Serial.println(error.c_str());
      return;
    }

    const char *command_name = doc["command_name"];
    JsonObject paras = doc["paras"];

    if (command_name && strcmp(command_name, "RGB") == 0)
    {
      const char *mode = paras["mode"];

      if (strcmp(mode, "single") == 0)
      {
        int index = paras["index"];
        int r = paras["Red"];
        int g = paras["Green"];
        int b = paras["Blue"];
        led_set_single(index, r, g, b);
      }
      else if (strcmp(mode, "all") == 0)
      {
        int r = paras["Red"];
        int g = paras["Green"];
        int b = paras["Blue"];
        led_set_all(r, g, b);
      }
      else if (strcmp(mode, "rainbow") == 0)
      {
        uint16_t speed = paras["speed"] | 20; // Default to 20 if not provided
        led_rainbow_mode(speed);
      }
      else if (strcmp(mode, "off") == 0)
      {
        led_off();
      }
    }
    else if (command_name && strcmp(command_name, "Alarm") == 0)
    {
      uint8_t hour = paras["Hour"];
      uint8_t minute = paras["Minute"];
      const char *week_str = paras["Week"];
      bool enabled = paras["On"];

      uint8_t days_mask = 0;
      if (strcmp(week_str, "Sunday") == 0) days_mask = (1 << 0);
      else if (strcmp(week_str, "Monday") == 0) days_mask = (1 << 1);
      else if (strcmp(week_str, "Tuesday") == 0) days_mask = (1 << 2);
      else if (strcmp(week_str, "Wednesday") == 0) days_mask = (1 << 3);
      else if (strcmp(week_str, "Thursday") == 0) days_mask = (1 << 4);
      else if (strcmp(week_str, "Friday") == 0) days_mask = (1 << 5);
      else if (strcmp(week_str, "Saturday") == 0) days_mask = (1 << 6);

      // For now, update alarm at index 0
      Alarm_Update(0, hour, minute, days_mask, enabled);
    }
    else if (command_name && strcmp(command_name, "play_song") == 0)
    {
      int songIndex = paras["Song_index"];
      const char *ui_mode = paras["UI"];

      if (strcmp(ui_mode, "Full") == 0)
      {
        requestedSongIndex = songIndex;
        requestedSongAction = (volatile void (*)(int)) & play_song_full_ui;
      }
      else if (strcmp(ui_mode, "Lite") == 0)
      {
        requestedSongIndex = songIndex;
        requestedSongAction = (volatile void (*)(int)) & play_song_lite_ui;
      }
      else if (strcmp(ui_mode, "No") == 0)
      {
        play_song_background(songIndex);
      }
    }

    char requestId[100] = { 0 };
    char *pstr = strstr(topic, "request_id=");
    if (pstr)
    {
      strcpy(requestId, pstr + strlen("request_id="));
    }

    Serial.print("Request ID: ");
    Serial.println(requestId);

    char responseTopic[200] = { 0 };
    strcat(responseTopic, MQTT_TOPIC_CMD_RESPONSE);
    strcat(responseTopic, requestId);
    sendCommandResponse(responseTopic);
  }
  else if (strstr(topic, MQTT_TOPIC_GET))
  {
    Serial.println("收到下行消息，未处理");
  }
}

/**
 * @brief 发送命令响应到平台
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

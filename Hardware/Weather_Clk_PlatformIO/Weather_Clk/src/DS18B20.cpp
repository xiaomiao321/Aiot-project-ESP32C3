// 包含所有必需的头文件
#include "DS18B20.h"
#include "Alarm.h"
#include <TFT_eSPI.h>
#include <TFT_eWidget.h>
#include "MQTT.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include "Menu.h"
#include "RotaryEncoder.h"
#include "weather.h"

// --- 图表尺寸和位置定义 ---
#define TEMP_GRAPH_WIDTH  200 // 图表宽度
#define TEMP_GRAPH_HEIGHT 135 // 图表高度
#define TEMP_GRAPH_X      20  // 图表左上角X坐标
#define TEMP_GRAPH_Y      90  // 图表左上角Y坐标

// --- 温度值显示位置 ---
#define TEMP_VALUE_X      20
#define TEMP_VALUE_Y      20

// --- 状态消息位置 ---
#define STATUS_MESSAGE_X  10
#define STATUS_MESSAGE_Y  230 // 调整到图表下方

// --- DS18B20传感器引脚定义 ---
#define DS18B20_PIN 10

// --- 库对象实例化 ---
OneWire oneWire(DS18B20_PIN); // 创建OneWire实例
DallasTemperature sensors(&oneWire); // 将OneWire实例传递给DallasTemperature库

// --- 全局变量 ---
float g_currentTemperature = -127.0; // 全局变量，存储当前温度值。-127.0为设备断开时的特殊值。
volatile bool stopDS18B20Task = false; // 停止DS18B20任务的标志，volatile确保多任务可见性

// --- TFT_eWidget 图表和描线对象 ---
GraphWidget gr = GraphWidget(&tft); // 图表控件
TraceWidget tr = TraceWidget(&gr);  // 描线控件

/**
 * @brief 初始化DS18B20传感器
 */
void DS18B20_Init()
{
  sensors.begin(); // 启动DallasTemperature库
}

/**
 * @brief [FreeRTOS Task] 后台更新温度任务
 * @param pvParameters 未使用
 * @details 此任务在后台运行，每2秒请求一次温度读数，并更新全局温度变量 `g_currentTemperature`。
 *          这样可以确保主显示任务总能获取到较新的温度数据，而无需自己进行耗时的查询。
 */
void updateTempTask(void *pvParameters)
{
  while (1)
  {
    sensors.requestTemperatures(); // 发送命令以获取温度
    float temp = sensors.getTempCByIndex(0); // 从第一个传感器获取摄氏温度
    if (temp != DEVICE_DISCONNECTED_C) // DEVICE_DISCONNECTED_C 是库定义的错误码
    {
      g_currentTemperature = temp; // 更新全局温度值
    }
    vTaskDelay(pdMS_TO_TICKS(2000)); // 任务延时2秒
  }
}

/**
 * @brief 创建后台温度更新任务
 */
void createDS18B20Task()
{
  xTaskCreate(
    updateTempTask,
    "DS18B20 Update Task",
    1024, // 任务堆栈大小
    NULL, // 任务参数
    1,    // 任务优先级
    NULL  // 任务句柄
  );
}

/**
 * @brief 获取当前DS18B20温度值
 * @return 返回全局变量中存储的最新温度值
 */
float getDS18B20Temp()
{
  return g_currentTemperature;
}

/**
 * @brief [FreeRTOS Task] DS18B20数据显示主任务
 * @param pvParameters 未使用
 * @details 此任务负责在屏幕上绘制温度曲线图和实时温度值。
 *          它会初始化图表，然后在一个循环中不断获取温度数据，更新屏幕显示，
 *          并将数据点添加到图表中形成曲线。
 */
void DS18B20_Task(void *pvParameters)
{
  float lastTemp = -274; // 上一次的温度值，用于比较
  float gx = 0.0; // 图表X轴的当前位置

  // --- 初始化图表 ---
  gr.createGraph(TEMP_GRAPH_WIDTH, TEMP_GRAPH_HEIGHT, tft.color565(5, 5, 5)); // 创建图表背景
  gr.setGraphScale(0.0, 100.0, 0.0, 40.0); // 设置X轴(0-100)和Y轴(0-40°C)的范围
  gr.setGraphGrid(0.0, 25.0, 0.0, 10.0, TFT_DARKGREY); // 设置网格线
  gr.drawGraph(TEMP_GRAPH_X, TEMP_GRAPH_Y); // 绘制图表框架
  tr.startTrace(TFT_YELLOW); // 开始以黄色描绘曲线

  // --- 绘制坐标轴标签 ---
  tft.setTextFont(1);
  tft.setTextSize(1);
  tft.setTextDatum(MR_DATUM); // 中右对齐
  tft.setTextColor(TFT_WHITE, tft.color565(5, 5, 5));
  // 绘制Y轴标签 (40, 30, 20, 10, 0)
  for (int i = 40; i >= 0; i -= 10)
  {
    tft.drawNumber(i, gr.getPointX(0.0) - 5, gr.getPointY(i));
  }

  tft.setTextDatum(TC_DATUM); // 上中对齐
  // 绘制X轴标签 (0, 25, 50, 75, 100)
  for (int i = 0; i <= 100; i += 25)
  {
    tft.drawNumber(i, gr.getPointX(i), gr.getPointY(0.0) + 5);
  }

  // --- 主循环 ---
  while (1)
  {
    if (stopDS18B20Task) break; // 检查退出标志

    float tempC = getDS18B20Temp(); // 获取最新温度

    if (tempC != DEVICE_DISCONNECTED_C && tempC > -50 && tempC < 150) // 检查温度值是否有效
    {
      tft.fillRect(0, 0, tft.width(), TEMP_GRAPH_Y - 5, TFT_BLACK); // 清除图表上方的区域

      // 在顶部显示当前时间
      if (getLocalTime(&timeinfo, 1))
      {
        char time_str[30];
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S %a", &timeinfo);
        tft.setTextFont(2);
        tft.setTextSize(1);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.setTextDatum(MC_DATUM);
        tft.drawString(time_str, tft.width() / 2, 10);
        tft.setTextDatum(TL_DATUM);
      }

      // --- 显示实时温度值 ---
      char tempStr[10];
      dtostrf(tempC, 4, 2, tempStr); // 将float转为字符串
      char fullTempStr[15];
      sprintf(fullTempStr, "%s C", tempStr);

      tft.setTextFont(7); // 使用大号字体
      tft.setTextSize(1);
      int text_width = tft.textWidth(fullTempStr);
      int text_height = tft.fontHeight();
      int x_pos = (tft.width() - text_width) / 2;
      int y_pos = (TEMP_GRAPH_Y - 5 - text_height) / 2 + 20; // 计算Y坐标，使其在时间下方居中

      tft.setTextDatum(TL_DATUM);
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      tft.drawString(fullTempStr, x_pos, y_pos);

      // --- 向图表添加数据点 ---
      tr.addPoint(gx, tempC); // 添加当前温度点
      gx += 1.0; // X轴步进

      // 如果图表画满了，则重置
      if (gx > 100.0)
      {
        gx = 0.0;
        gr.drawGraph(TEMP_GRAPH_X, TEMP_GRAPH_Y); // 重绘图表背景
        tr.startTrace(TFT_YELLOW); // 重新开始描线
      }

      lastTemp = tempC;
    }
    else // 如果传感器读取错误
    {
      if (stopDS18B20Task) break;
      tft.fillScreen(TFT_BLACK); // 清屏
      tft.setCursor(10, 30);
      tft.setTextSize(2);
      tft.setTextColor(TFT_RED);
      tft.println("Sensor Error!");
    }

    vTaskDelay(pdMS_TO_TICKS(500)); // 每500ms更新一次屏幕
  }

  vTaskDelete(NULL); // 任务结束时自我删除
}

/**
 * @brief DS18B20温度显示功能的菜单入口函数
 * @details 此函数负责启动DS18B20的显示任务，并处理用户的退出操作。
 *          当用户通过按钮、闹钟或全局退出标志请求退出时，它会设置停止标志并安全地清理任务。
 */
void DS18B20Menu()
{
  stopDS18B20Task = false; // 重置停止标志
  tft.fillScreen(TFT_BLACK); // 清屏

  // 创建并启动DS18B20显示任务，固定在核心0上运行
  xTaskCreatePinnedToCore(DS18B20_Task, "DS18B20_Task", 4096, NULL, 1, NULL, 0);

  // 循环等待退出信号
  while (1)
  {
    if (exitSubMenu)
    {
      exitSubMenu = false; // 重置标志
      stopDS18B20Task = true; // 通知任务停止
      vTaskDelay(pdMS_TO_TICKS(150)); // 等待任务处理停止信号
      break; // 退出循环
    }
    if (g_alarm_is_ringing)
    {
      stopDS18B20Task = true; // 通知任务停止
      vTaskDelay(pdMS_TO_TICKS(150)); // 等待任务处理停止信号
      break; // 退出循环
    }
    if (readButton())
    {
      stopDS18B20Task = true; // 通知任务停止
      // 等待任务确认删除
      TaskHandle_t task = xTaskGetHandle("DS18B20_Task");
      for (int i = 0; i < 150 && task != NULL; i++)
      {
        if (eTaskGetState(task) == eDeleted) break;
        vTaskDelay(pdMS_TO_TICKS(10));
      }
      break; // 退出循环
    }
    vTaskDelay(pdMS_TO_TICKS(10)); // 短暂延迟，避免CPU空转
  }
}
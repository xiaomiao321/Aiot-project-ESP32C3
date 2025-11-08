#include "performance.h"
#include "Alarm.h"
#include <TFT_eWidget.h>
#include "Menu.h"
#include "MQTT.h"
#include "RotaryEncoder.h"
#include "Alarm.h"

// 全局变量
struct PCData
{
  char cpuName[64];
  char gpuName[64];
  int cpuTemp;
  int cpuLoad;
  int gpuTemp;
  int gpuLoad;
  float ramLoad;
  bool valid;
};
float esp32c3_temp = 0.0f;
struct PCData pcData = { .cpuName = "Unknown",
                        .gpuName = "Unknown",
                        .cpuTemp = 0,
                        .cpuLoad = 0,
                        .gpuTemp = 0,
                        .gpuLoad = 0,
                        .ramLoad = 0.0,
                        .valid = false };
char inputBuffer[BUFFER_SIZE];
uint16_t bufferIndex = 0;
bool stringComplete = false;
SemaphoreHandle_t xPCDataMutex = NULL;
extern TFT_eSPI tft; // 声明外部 TFT 对象

// 图表对象
GraphWidget combinedChart = GraphWidget(&tft);
TraceWidget cpuLoadTrace = TraceWidget(&combinedChart);
TraceWidget gpuLoadTrace = TraceWidget(&combinedChart);
TraceWidget cpuTempTrace = TraceWidget(&combinedChart);
TraceWidget gpuTempTrace = TraceWidget(&combinedChart);

// 绘制静态元素
void drawPerformanceStaticElements()
{
  tft.fillScreen(BG_COLOR);
  tft.pushImage(LOGO_X, LOGO_Y_TOP, NVIDIA_HEIGHT, NVIDIA_WIDTH, NVIDIA);
  tft.pushImage(LOGO_X, LOGO_Y_BOTTOM, INTEL_HEIGHT, INTEL_WIDTH, intel);
  tft.setTextFont(1);
  tft.setTextSize(2);
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(TFT_GREEN, BG_COLOR);
  tft.drawString("CPU:", DATA_X, DATA_Y);
  tft.setTextColor(TFT_BLUE, BG_COLOR);
  tft.drawString("GPU:", DATA_X, DATA_Y + LINE_HEIGHT);
  tft.setTextColor(TFT_RED, BG_COLOR);
  tft.drawString("RAM:", DATA_X, DATA_Y + 2 * LINE_HEIGHT);
  tft.setTextColor(TFT_ORANGE, BG_COLOR);
  tft.drawString("ESP:", DATA_X, DATA_Y + 3 * LINE_HEIGHT);

  // 图标对象初始化
  combinedChart.createGraph(COMBINED_CHART_WIDTH, COMBINED_CHART_HEIGHT, tft.color565(5, 5, 5));
  combinedChart.setGraphScale(0.0, 100.0, 0.0, 100.0); // X and Y scale from 0 to 100
  combinedChart.setGraphGrid(0.0, 25.0, 0.0, 25.0, TFT_DARKGREY);
  combinedChart.drawGraph(COMBINED_CHART_X, COMBINED_CHART_Y);

  // 图例
  tft.setTextSize(1);

  tft.fillCircle(LEGEND_X_START, LEGEND_Y_POS, LEGEND_RADIUS, TFT_GREEN);
  tft.setTextColor(TFT_GREEN, BG_COLOR);
  tft.drawString("CPU Load", LEGEND_X_START + LEGEND_TEXT_OFFSET, LEGEND_Y_POS);

  tft.fillCircle(LEGEND_X_START + LEGEND_SPACING_WIDE, LEGEND_Y_POS, LEGEND_RADIUS, TFT_BLUE);
  tft.setTextColor(TFT_BLUE, BG_COLOR);
  tft.drawString("GPU Load", LEGEND_X_START + LEGEND_SPACING_WIDE + LEGEND_TEXT_OFFSET, LEGEND_Y_POS);

  tft.fillCircle(LEGEND_X_START, LEGEND_Y_POS + LEGEND_LINE_HEIGHT, LEGEND_RADIUS, TFT_RED);
  tft.setTextColor(TFT_RED, BG_COLOR);
  tft.drawString("RAM", LEGEND_X_START + LEGEND_TEXT_OFFSET, LEGEND_Y_POS + LEGEND_LINE_HEIGHT);

  tft.fillCircle(LEGEND_X_START + LEGEND_SPACING_WIDE, LEGEND_Y_POS + LEGEND_LINE_HEIGHT, LEGEND_RADIUS, TFT_ORANGE);
  tft.setTextColor(TFT_ORANGE, BG_COLOR);
  tft.drawString("GPU Temp", LEGEND_X_START + LEGEND_SPACING_WIDE + LEGEND_TEXT_OFFSET, LEGEND_Y_POS + LEGEND_LINE_HEIGHT);

  // 坐标轴
  tft.setTextColor(TITLE_COLOR, BG_COLOR); // Axis value color
  tft.drawString("0", COMBINED_CHART_X - 15, combinedChart.getPointY(0));
  tft.drawString("50", COMBINED_CHART_X - 15, combinedChart.getPointY(50));
  tft.drawString("100", COMBINED_CHART_X - 15, combinedChart.getPointY(100));

  // 设置线颜色
  cpuLoadTrace.startTrace(TFT_GREEN);
  gpuLoadTrace.startTrace(TFT_BLUE);
  cpuTempTrace.startTrace(TFT_RED);
  gpuTempTrace.startTrace(TFT_ORANGE);
}

// 更新 PC 数据
void updatePerformanceData()
{
  if (xSemaphoreTake(xPCDataMutex, 10) != pdTRUE) // 互斥信号量
    return;
  tft.setTextColor(VALUE_COLOR, BG_COLOR);
  tft.setTextFont(1);
  tft.setTextSize(2);
  // if (pcData.valid)
  // {
    // CPU Data
  tft.fillRect(DATA_X + VALUE_OFFSET_X, DATA_Y, VALUE_WIDTH, LINE_HEIGHT, BG_COLOR);
  tft.setTextColor(TFT_GREEN, BG_COLOR);
  tft.drawString(String(pcData.cpuLoad) + "% " + String(pcData.cpuTemp) + "C", DATA_X + VALUE_OFFSET_X, DATA_Y);

  // GPU Data
  tft.fillRect(DATA_X + VALUE_OFFSET_X, DATA_Y + LINE_HEIGHT, VALUE_WIDTH, LINE_HEIGHT, BG_COLOR);
  tft.setTextColor(TFT_BLUE, BG_COLOR);
  tft.drawString(String(pcData.gpuLoad) + "% " + String(pcData.gpuTemp) + "C", DATA_X + VALUE_OFFSET_X, DATA_Y + LINE_HEIGHT);

  // RAM Data
  tft.fillRect(DATA_X + VALUE_OFFSET_X, DATA_Y + 2 * LINE_HEIGHT, VALUE_WIDTH, LINE_HEIGHT, BG_COLOR);
  tft.setTextColor(TFT_RED, BG_COLOR);
  tft.drawString(String(pcData.ramLoad, 1) + "%", DATA_X + VALUE_OFFSET_X, DATA_Y + 2 * LINE_HEIGHT);

  static float gx = 0.0;
  cpuLoadTrace.addPoint(gx, pcData.cpuLoad);
  gpuLoadTrace.addPoint(gx, pcData.gpuLoad);
  cpuTempTrace.addPoint(gx, pcData.ramLoad);
  gpuTempTrace.addPoint(gx, pcData.gpuTemp);
  gx += 1.0;
  if (gx > 100.0)
  {
    gx = 0.0;
    combinedChart.drawGraph(COMBINED_CHART_X, COMBINED_CHART_Y);
    cpuLoadTrace.startTrace(TFT_GREEN);
    gpuLoadTrace.startTrace(TFT_BLUE);
    cpuTempTrace.startTrace(TFT_RED);
    gpuTempTrace.startTrace(TFT_ORANGE);
  }

  // }
  // else
  // {
  //   tft.fillRect(DATA_X + VALUE_OFFSET_X, DATA_Y, VALUE_WIDTH, 3 * LINE_HEIGHT, BG_COLOR);
  //   tft.setTextColor(ERROR_COLOR, BG_COLOR);
  //   tft.setTextSize(2);
  //   tft.drawString("No Data", DATA_X + VALUE_OFFSET_X, DATA_Y);
  // }

  // ESP32 Temp
  tft.fillRect(DATA_X + VALUE_OFFSET_X, DATA_Y + 3 * LINE_HEIGHT, VALUE_WIDTH, LINE_HEIGHT, BG_COLOR);
  tft.setTextColor(TFT_ORANGE, BG_COLOR);
  tft.setTextSize(2);
  tft.drawString(String(esp32c3_temp, 1) + " C", DATA_X + VALUE_OFFSET_X, DATA_Y + 3 * LINE_HEIGHT);

  xSemaphoreGive(xPCDataMutex);
}

// 重置缓冲区
void resetBuffer()
{
  bufferIndex = 0;
  inputBuffer[0] = '\0';
}

// 解析 PC 数据
void parsePCData()
{
  struct PCData newData = { .cpuName = "Unknown",
                           .gpuName = "Unknown",
                           .cpuTemp = 0,
                           .cpuLoad = 0,
                           .gpuTemp = 0,
                           .gpuLoad = 0,
                           .ramLoad = 0.0,
                           .valid = false };
  char *ptr = nullptr;
  ptr = strstr(inputBuffer, "C");
  if (ptr)
  {
    char *degree = strchr(ptr, 'c');
    char *limit = strchr(ptr, '%');
    if (degree && limit && degree < limit)
    {
      char temp[8] = { 0 }, load[8] = { 0 };
      strncpy(temp, ptr + 1, degree - ptr - 1);
      strncpy(load, degree + 2, limit - degree - 2);
      newData.cpuTemp = atoi(temp);
      newData.cpuLoad = atoi(load);
    }
  }
  ptr = strstr(inputBuffer, "G");
  if (ptr)
  {
    char *degree = strstr(ptr, "c");
    char *limit = strchr(ptr, '%');
    if (degree && limit && degree < limit)
    {
      char temp[8] = { 0 }, load[8] = { 0 };
      strncpy(temp, ptr + 1, degree - ptr - 1);
      strncpy(load, degree + 2, limit - degree - 2);
      newData.gpuTemp = atoi(temp);
      newData.gpuLoad = atoi(load);
    }
  }
  ptr = strstr(inputBuffer, "RL");
  if (ptr)
  {
    ptr += 2;
    char *end = strchr(ptr, '|');
    if (end)
    {
      char load[8] = { 0 };
      strncpy(load, ptr, end - ptr);
      newData.ramLoad = atof(load);
    }
  }
  ptr = strstr(inputBuffer, "CPU:");
  if (ptr)
  {
    ptr += 4;
    char *end = strstr(ptr, "GPU:");
    if (!end)
      end = inputBuffer + strlen(inputBuffer);
    if (end - ptr < sizeof(newData.cpuName) - 1)
    {
      strncpy(newData.cpuName, ptr, end - ptr);
      newData.cpuName[end - ptr] = '\0';
      for (int i = strlen(newData.cpuName) - 1;
        i >= 0 && newData.cpuName[i] == ' '; i--)
        newData.cpuName[i] = '\0';
    }
  }
  ptr = strstr(inputBuffer, "GPU:");
  if (ptr)
  {
    ptr += 4;
    char *end = strchr(ptr, '|');
    if (!end)
      end = inputBuffer + strlen(inputBuffer);
    if (end - ptr < sizeof(newData.gpuName) - 1)
    {
      strncpy(newData.gpuName, ptr, end - ptr);
      newData.gpuName[end - ptr] = '\0';
      for (int i = strlen(newData.gpuName) - 1;
        i >= 0 && newData.gpuName[i] == ' '; i--)
        newData.gpuName[i] = '\0';
    }
  }
  if (newData.cpuTemp > 0 || newData.gpuTemp > 0 || newData.ramLoad > 0)
  {
    newData.valid = true;
  }
  if (xSemaphoreTake(xPCDataMutex, portMAX_DELAY) == pdTRUE)
  {
    pcData = newData;
    xSemaphoreGive(xPCDataMutex);
  }
  // 临时测试用
  newData.valid = true;
  newData.cpuTemp = 50;
  newData.cpuLoad = 30;
  newData.gpuTemp = 60;
  newData.gpuLoad = 70;
  newData.ramLoad = 45.5;
}

// -----------------------------
// 性能监控初始化任务
// -----------------------------
void Performance_Init_Task(void *pvParameters)
{
  drawPerformanceStaticElements();
  vTaskDelete(NULL);
}

// -----------------------------
// 性能监控显示任务
// -----------------------------
void Performance_Task(void *pvParameters)
{
  for (;;)
  {
    esp32c3_temp = temperatureRead();
    updatePerformanceData();
    vTaskDelay(pdMS_TO_TICKS(500)); // Update every second for smoother chart updates
  }
}

// 串口接收任务
void SERIAL_Task(void *pvParameters)
{
  for (;;)
  {
    if (Serial.available())
    {
      char inChar = (char) Serial.read();
      if (bufferIndex < BUFFER_SIZE - 1)
      {
        inputBuffer[bufferIndex++] = inChar;
        inputBuffer[bufferIndex] = '\0';
      }
      else
      {
        resetBuffer();
      }
      if (inChar == '\n')
      {
        stringComplete = true;
      }
    }
    if (stringComplete)
    {
      parsePCData();
      stringComplete = false;
      resetBuffer();
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

// -----------------------------
// 性能监控菜单入口
// -----------------------------
void performanceMenu()
{
  tft.fillScreen(TFT_BLACK); // 清屏
  drawPerformanceStaticElements();
  xPCDataMutex = xSemaphoreCreateMutex();
  xTaskCreatePinnedToCore(Performance_Init_Task, "Perf_Init", 8192, NULL, 2, NULL, 0);
  xTaskCreatePinnedToCore(Performance_Task, "Perf_Show", 8192, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(SERIAL_Task, "Serial_Rx", 2048, NULL, 1, NULL, 0);// 创建任务
  while (1)
  {
    if (exitSubMenu)
    {
      exitSubMenu = false; // Reset flag
      vTaskDelete(xTaskGetHandle("Perf_Show"));
      vTaskDelete(xTaskGetHandle("Serial_Rx"));
      vSemaphoreDelete(xPCDataMutex);
      break;
    }
    if (g_alarm_is_ringing)
    {
      break;
    }
    if (readButton())
    {
      vTaskDelete(xTaskGetHandle("Perf_Show"));
      vTaskDelete(xTaskGetHandle("Serial_Rx"));
      vSemaphoreDelete(xPCDataMutex);
      break;
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

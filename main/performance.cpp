#include "performance.h"
#include "Alarm.h"
#include <TFT_eWidget.h>
#include "Menu.h"
#include "MQTT.h"
#include "RotaryEncoder.h"
#include "Alarm.h"
#include "Buzzer.h"
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
  struct PCData newData = {
      .cpuName = "Unknown",
      .gpuName = "Unknown",
      .cpuTemp = 0,
      .cpuLoad = 0,
      .gpuTemp = 0,
      .gpuLoad = 0,
      .ramLoad = 0.0,
      .valid = false
  };

  // --- Parse CPU Load from "CCc <load>%" ---
  char *ptr = strstr(inputBuffer, "CCc ");
  if (ptr)
  {
    char *pct = strchr(ptr, '%');
    if (pct && pct > ptr + 4)
    {
      int len = pct - (ptr + 4);
      if (len > 0 && len < 8)
      {
        char loadStr[8] = { 0 };
        strncpy(loadStr, ptr + 4, len);
        newData.cpuLoad = atoi(loadStr);
        newData.cpuTemp = 0; // unknown
      }
    }
  }

  // --- Parse GPU from "G<temp>c <load>%" (skip "GPU:") ---
  ptr = strstr(inputBuffer, "G");
  while (ptr)
  {
    if (ptr[1] >= '0' && ptr[1] <= '9')
    { // G followed by digit
      char *cPos = strchr(ptr, 'c');
      char *pct = strchr(ptr, '%');
      if (cPos && pct && cPos < pct)
      {
        // Temperature
        int tempLen = cPos - (ptr + 1);
        if (tempLen > 0 && tempLen < 8)
        {
          char tempStr[8] = { 0 };
          strncpy(tempStr, ptr + 1, tempLen);
          newData.gpuTemp = atoi(tempStr);
        }
        // Load (skip space after 'c')
        const char *loadStart = cPos + 1;
        while (*loadStart == ' ') loadStart++;
        int loadLen = pct - loadStart;
        if (loadLen > 0 && loadLen < 8)
        {
          char loadStr[8] = { 0 };
          strncpy(loadStr, loadStart, loadLen);
          newData.gpuLoad = atoi(loadStr);
        }
        break;
      }
    }
    ptr = strstr(ptr + 1, "G");
  }

  // --- Parse RAM from "RL<value>|" ---
  ptr = strstr(inputBuffer, "RL");
  if (ptr)
  {
    ptr += 2;
    char *bar = strchr(ptr, '|');
    if (bar)
    {
      int len = bar - ptr;
      if (len > 0 && len < 8)
      {
        char ramStr[8] = { 0 };
        strncpy(ramStr, ptr, len);
        newData.ramLoad = atof(ramStr);
      }
    }
  }

  // --- Parse CPU Name ---
  ptr = strstr(inputBuffer, "CPU:");
  if (ptr)
  {
    ptr += 4;
    char *end = strstr(ptr, "GPU:");
    if (!end) end = inputBuffer + strlen(inputBuffer);
    int len = end - ptr;
    if (len > 0 && len < (int) sizeof(newData.cpuName))
    {
      strncpy(newData.cpuName, ptr, len);
      newData.cpuName[len] = '\0';
      // Trim trailing spaces
      for (int i = len - 1; i >= 0 && newData.cpuName[i] == ' '; i--)
        newData.cpuName[i] = '\0';
    }
  }

  // --- Parse GPU Name ---
  ptr = strstr(inputBuffer, "GPU:");
  if (ptr)
  {
    ptr += 4;
    char *end = strchr(ptr, '|');
    if (!end) end = inputBuffer + strlen(inputBuffer);
    int len = end - ptr;
    if (len > 0 && len < (int) sizeof(newData.gpuName))
    {
      strncpy(newData.gpuName, ptr, len);
      newData.gpuName[len] = '\0';
      for (int i = len - 1; i >= 0 && newData.gpuName[i] == ' '; i--)
        newData.gpuName[i] = '\0';
    }
  }

  // Mark as valid if any useful data
  if (newData.cpuLoad > 0 || newData.gpuTemp > 0 || newData.ramLoad > 0)
  {
    newData.valid = true;
  }

  // Update global data (no mutex)
  pcData = newData;

  // 蜂鸣器响一声：1kHz，50ms
  tone(BUZZER_PIN, 1000);   // 1kHz
  delay(50);                // 阻塞50ms
  noTone(BUZZER_PIN);
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
  unsigned long lastCharTime = 0;

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
        // Buffer full, reset and add the new character.
        resetBuffer();
        inputBuffer[bufferIndex++] = inChar;
        inputBuffer[bufferIndex] = '\0';
      }
      lastCharTime = millis(); // Update time of last received character

      // If we receive a newline, the string is complete.
      if (inChar == '\n' || inChar == '\r')
      {
        stringComplete = true;
      }
    }

    // Timeout logic: if we have received some data, but then there's a pause of 50ms.
    if (!stringComplete && bufferIndex > 0 && (millis() - lastCharTime > 50)) {
        stringComplete = true;
    }

    if (stringComplete)
    {
      parsePCData();       // 解析并触发蜂鸣器
      stringComplete = false;
      resetBuffer();
    }
    
    vTaskDelay(pdMS_TO_TICKS(10)); // Yield to other tasks
  }
}

// -----------------------------
// 性能监控菜单入口
// -----------------------------
void performanceMenu()
{
  tft.fillScreen(TFT_BLACK); // 清屏
  drawPerformanceStaticElements();
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
      break;
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

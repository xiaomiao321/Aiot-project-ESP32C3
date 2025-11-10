#include "performance.h"
#include "Alarm.h"
#include <TFT_eWidget.h>
#include "Menu.h"
#include "MQTT.h"
#include "RotaryEncoder.h"
#include "Buzzer.h"
#include "freertos/semphr.h"

// --- 全局变量 ---

float esp32c3_temp = 0.0f; // 存储ESP32-C3芯片自身的温度
struct PCData pcData; // 全局PC数据结构体实例
SemaphoreHandle_t pcDataMutex; // 用于保护pcData的互斥锁

// 串口接收缓冲区
char inputBuffer[BUFFER_SIZE];
uint16_t bufferIndex = 0;
bool stringComplete = false; // 串口数据接收完成标志

extern TFT_eSPI tft; // 声明在其他文件中定义的外部TFT对象

// --- 图表控件对象 ---
GraphWidget combinedChart = GraphWidget(&tft); // 组合图表
TraceWidget cpuLoadTrace = TraceWidget(&combinedChart); // CPU负载曲线
TraceWidget gpuLoadTrace = TraceWidget(&combinedChart); // GPU负载曲线
TraceWidget cpuTempTrace = TraceWidget(&combinedChart); // CPU温度曲线 (此处实际绘制的是RAM使用率)
TraceWidget gpuTempTrace = TraceWidget(&combinedChart); // GPU温度曲线

/**
 * @brief 初始化PCData结构体
 * @param pcdata 指向要初始化的PCData结构体的指针
 */
void PCData_Init(struct PCData *pcdata)
{
  memset(pcdata, 0, sizeof(*pcdata)); // 将结构体内存清零

  // 安全地复制CPU和GPU的默认名称
  strncpy(pcdata->cpuName, "Intel Core i7-14650HX", sizeof(pcdata->cpuName) - 1);
  pcdata->cpuName[sizeof(pcdata->cpuName) - 1] = '\0';
  strncpy(pcdata->gpuName, "NVIDIA GeForce RTX 4060 Laptop GPU", sizeof(pcdata->gpuName) - 1);
  pcdata->gpuName[sizeof(pcdata->gpuName) - 1] = '\0';

  pcdata->valid = false; // 初始时数据无效
}

/**
 * @brief 绘制性能监控界面的静态元素
 * @details 包括背景、Logo、文本标签、图表框架和图例。这些元素在进入菜单时只绘制一次。
 */
void drawPerformanceStaticElements()
{
  tft.startWrite();
  tft.fillScreen(BG_COLOR);
  // 绘制NVIDIA和Intel的Logo
  tft.pushImage(LOGO_X, LOGO_Y_TOP, NVIDIA_HEIGHT, NVIDIA_WIDTH, NVIDIA);
  tft.pushImage(LOGO_X, LOGO_Y_BOTTOM, INTEL_HEIGHT, INTEL_WIDTH, intel);

  // 绘制文本标签
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

  // 初始化图表
  combinedChart.createGraph(COMBINED_CHART_WIDTH, COMBINED_CHART_HEIGHT, tft.color565(5, 5, 5));
  combinedChart.setGraphScale(0.0, 100.0, 0.0, 100.0); // X和Y轴范围均为0-100
  combinedChart.setGraphGrid(0.0, 25.0, 0.0, 25.0, TFT_DARKGREY); // 设置网格线
  combinedChart.drawGraph(COMBINED_CHART_X, COMBINED_CHART_Y);

  // 绘制图例
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

  // 绘制坐标轴数值
  tft.setTextColor(TITLE_COLOR, BG_COLOR);
  tft.drawString("0", COMBINED_CHART_X - 15, combinedChart.getPointY(0));
  tft.drawString("50", COMBINED_CHART_X - 15, combinedChart.getPointY(50));
  tft.drawString("100", COMBINED_CHART_X - 15, combinedChart.getPointY(100));

  // 设置曲线颜色
  cpuLoadTrace.startTrace(TFT_GREEN);
  gpuLoadTrace.startTrace(TFT_BLUE);
  cpuTempTrace.startTrace(TFT_RED);
  gpuTempTrace.startTrace(TFT_ORANGE);
  tft.endWrite();
}

/**
 * @brief 更新性能监控界面的动态数据
 * @details 包括CPU/GPU/RAM的负载和温度，以及ESP32自身的温度。同时更新图表曲线。
 */
void updatePerformanceData()
{
    struct PCData localPcData;

    // 使用互斥锁安全地复制共享数据
    if (pcDataMutex != NULL && xSemaphoreTake(pcDataMutex, (TickType_t)10) == pdTRUE)
    {
        localPcData = pcData;
        xSemaphoreGive(pcDataMutex);
    }
    else
    {
        return; 
    }

    tft.startWrite();
    tft.setTextColor(VALUE_COLOR, BG_COLOR);
    tft.setTextFont(1);
    tft.setTextSize(2);

    // 清除旧数据并绘制新数据
    // CPU
    tft.fillRect(DATA_X + VALUE_OFFSET_X, DATA_Y, VALUE_WIDTH, LINE_HEIGHT, BG_COLOR);
    tft.setTextColor(TFT_GREEN, BG_COLOR);
    tft.drawString(String(localPcData.cpuLoad) + "% " + String(localPcData.cpuTemp) + "C", DATA_X + VALUE_OFFSET_X, DATA_Y);

    // GPU
    tft.fillRect(DATA_X + VALUE_OFFSET_X, DATA_Y + LINE_HEIGHT, VALUE_WIDTH, LINE_HEIGHT, BG_COLOR);
    tft.setTextColor(TFT_BLUE, BG_COLOR);
    tft.drawString(String(localPcData.gpuLoad) + "% " + String(localPcData.gpuTemp) + "C", DATA_X + VALUE_OFFSET_X, DATA_Y + LINE_HEIGHT);

    // RAM
    tft.fillRect(DATA_X + VALUE_OFFSET_X, DATA_Y + 2 * LINE_HEIGHT, VALUE_WIDTH, LINE_HEIGHT, BG_COLOR);
    tft.setTextColor(TFT_RED, BG_COLOR);
    tft.drawString(String(localPcData.ramLoad, 1) + "%", DATA_X + VALUE_OFFSET_X, DATA_Y + 2 * LINE_HEIGHT);

    // ESP32 温度
    tft.fillRect(DATA_X + VALUE_OFFSET_X, DATA_Y + 3 * LINE_HEIGHT, VALUE_WIDTH, LINE_HEIGHT, BG_COLOR);
    tft.setTextColor(TFT_ORANGE, BG_COLOR);
    tft.drawString(String(esp32c3_temp, 1) + " C", DATA_X + VALUE_OFFSET_X, DATA_Y + 3 * LINE_HEIGHT);

    // 更新图表
    static float gx = 0.0; // 图表X轴当前位置
    cpuLoadTrace.addPoint(gx, localPcData.cpuLoad);
    gpuLoadTrace.addPoint(gx, localPcData.gpuLoad);
    cpuTempTrace.addPoint(gx, localPcData.ramLoad); // 注意：这里绘制的是RAM使用率
    gpuTempTrace.addPoint(gx, localPcData.gpuTemp);
    gx += 1.0;
    if (gx > 100.0)
    { // 如果图表画满了
        gx = 0.0;
        combinedChart.drawGraph(COMBINED_CHART_X, COMBINED_CHART_Y); // 重绘图表背景
        // 重新开始描线
        cpuLoadTrace.startTrace(TFT_GREEN);
        gpuLoadTrace.startTrace(TFT_BLUE);
        cpuTempTrace.startTrace(TFT_RED);
        gpuTempTrace.startTrace(TFT_ORANGE);
    }
    tft.endWrite();
}

/**
 * @brief 重置串口接收缓冲区
 */
void resetBuffer()
{
  bufferIndex = 0;
  inputBuffer[0] = '\0';
}

/**
 * @brief 解析从串口接收到的PC性能数据字符串
 * @details 从特定格式的字符串中提取CPU、GPU、RAM等信息，并填充到全局的pcData结构体中。
 */
void parsePCData()
{
    struct PCData parsedValues;
    memset(&parsedValues, 0, sizeof(parsedValues));
    parsedValues.valid = false;

    char *ptr;

    ptr = strstr(inputBuffer, "CCc ");
    if (ptr) { parsedValues.cpuLoad = atoi(ptr + 4); }

    ptr = strstr(inputBuffer, "G");
    if (ptr && ptr[1] >= '0' && ptr[1] <= '9') {
        char *cPos = strchr(ptr, 'c');
        if (cPos) {
            parsedValues.gpuTemp = atoi(ptr + 1);
            parsedValues.gpuLoad = atoi(cPos + 1);
        }
    }

    ptr = strstr(inputBuffer, "RL");
    if (ptr) { parsedValues.ramLoad = atof(ptr + 2); }

    if (parsedValues.cpuLoad > 0 || parsedValues.gpuTemp > 0 || parsedValues.ramLoad > 0) {
        parsedValues.valid = true;
    }

    // 使用互斥锁安全地更新全局结构体
    if (pcDataMutex != NULL && xSemaphoreTake(pcDataMutex, (TickType_t)10) == pdTRUE)
    {
        // 仅更新解析出的值，保留固定的名称
        pcData.cpuLoad = parsedValues.cpuLoad;
        pcData.cpuTemp = parsedValues.cpuTemp;
        pcData.gpuLoad = parsedValues.gpuLoad;
        pcData.gpuTemp = parsedValues.gpuTemp;
        pcData.ramLoad = parsedValues.ramLoad;
        pcData.valid = parsedValues.valid;
        xSemaphoreGive(pcDataMutex);
    }
}


/**
 * @brief [FreeRTOS Task] 性能监控显示任务
 * @details 循环读取ESP32自身温度，并调用updatePerformanceData()函数更新屏幕上的动态数据。
 */
void Performance_Task(void *pvParameters)
{
  for (;;)
  {
    esp32c3_temp = temperatureRead();
    updatePerformanceData();
    vTaskDelay(pdMS_TO_TICKS(500)); // 每500ms更新一次
  }
}

/**
 * @brief [FreeRTOS Task] 串口接收任务
 * @details 循环监听串口输入，将接收到的字符存入缓冲区。
 *          当接收到换行符或超时时，将数据标记为完整，并调用解析函数。
 */
void SERIAL_Task(void *pvParameters)
{
  PCData_Init(&pcData);
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
      lastCharTime = millis();

      if (inChar == '\n' || inChar == '\r')
      {
        stringComplete = true;
      }
    }

    // 50ms超时判断
    if (!stringComplete && bufferIndex > 0 && (millis() - lastCharTime > 50))
    {
      stringComplete = true;
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

void startPerformanceMonitoring()
{
    pcDataMutex = xSemaphoreCreateMutex();
    xTaskCreatePinnedToCore(SERIAL_Task, "Serial_Rx", 2048, NULL, 1, NULL, 0);
}

/**
 * @brief 性能监控菜单的入口函数
 * @details 创建并管理性能显示任务和串口接收任务。处理退出逻辑。
 */
void performanceMenu()
{
  tft.fillScreen(TFT_BLACK);
  drawPerformanceStaticElements(); // 绘制静态背景

  // 创建显示任务
  xTaskCreatePinnedToCore(Performance_Task, "Perf_Show", 8192, NULL, 1, NULL, 0);

  while (1)
  {
    // 检查退出条件
    if (exitSubMenu || g_alarm_is_ringing || readButton())
    {
      exitSubMenu = false;
      // 删除创建的任务以释放资源
      vTaskDelete(xTaskGetHandle("Perf_Show"));
      break; // 退出循环，返回主菜单
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}
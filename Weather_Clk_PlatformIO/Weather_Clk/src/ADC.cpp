#include <Arduino.h>
#include "ADC.h"
#include "Alarm.h"
#include "esp_adc_cal.h"
#include <TFT_eSPI.h>
#include <TFT_eWidget.h>
#include "Menu.h"
#include "MQTT.h"
#include "RotaryEncoder.h"
#include <math.h>
#include "driver/adc.h"

// 创建一个仪表盘控件对象，用于显示电压
MeterWidget volts = MeterWidget(&tft);

// ADC引脚和参数定义
#define BAT_PIN         2            // ADC连接的GPIO引脚（光敏电阻）
#define ADC_CHANNEL     ADC1_CHANNEL_2 // ADC通道 (GPIO2 对应 ADC1_CH2)
#define ADC_ATTEN       ADC_ATTEN_DB_11 // ADC衰减设置为11dB，满量程电压约为3.3V
#define ADC_WIDTH       ADC_WIDTH_BIT_12 // ADC采样位宽设置为12位 (0-4095)

// 用于存储ADC校准特性的结构体
static esp_adc_cal_characteristics_t adc1_chars;
// 标志位，表示ADC校准是否成功启用
bool cali_enable = false;
// 标志位，用于通知ADC显示任务停止运行
bool stopADCDisplayTask = false;
// 全局光照强度变量
float g_lux = 0.0f;

/**
 * @brief 初始化ADC校准
 * @details 检查eFuse中是否有预烧录的校准值。
 *          如果存在，则加载这些校准值以提高ADC测量的线性度和准确性。
 * @return 如果校准成功初始化，返回true；否则返回false。
 */
static bool adc_calibration_init()
{
    // 检查eFuse中是否存在Two Point校准值
    esp_err_t ret = esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_TP);
    if (ret == ESP_ERR_NOT_SUPPORTED)
    {
        Serial.println("ADC校准: eFuse中不支持Two Point校准值");
        return false;
    }
    if (ret == ESP_ERR_INVALID_VERSION)
    {
        Serial.println("ADC校准: eFuse中的校准版本无效");
        return false;
    }
    if (ret == ESP_OK)
    {
        Serial.println("ADC校准: eFuse中存在有效的Two Point校准值");
        // 加载校准特性
        esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN, ADC_WIDTH, 0, &adc1_chars);
        return true;
    }
    return false;
}

/**
 * @brief 设置ADC
 * @details 配置ADC的采样位宽和通道衰减，并尝试初始化ADC校准。
 */
void setupADC()
{
    // 设置ADC1的采样位宽
    adc1_config_width(ADC_WIDTH);
    // 设置ADC1特定通道的衰减
    adc1_config_channel_atten(ADC_CHANNEL, ADC_ATTEN);
    // 尝试进行ADC校准
    cali_enable = adc_calibration_init();
}

/**
 * @brief [FreeRTOS Task] ADC后台读取任务
 * @param pvParameters 任务创建时传入的参数（未使用）
 * @details 这是一个在后台持续运行的周期性任务，负责：
 *          1. 多次采样ADC原始值并取平均，以减少噪声。
 *          2. 将原始值转换为电压值。
 *          3. 根据电压值计算光敏电阻的阻值，并估算环境光照强度（Lux），更新全局变量 g_lux。
 */
void ADC_Task(void *pvParameters)
{
    const float R_FIXED = 20000.0f;
    const float R10 = 8000.0f;
    const float GAMMA = 0.6f;

    for (;;) {
        uint32_t sum = 0;
        const int samples = 50;
        for (int i = 0; i < samples; i++)
        {
            sum += adc1_get_raw(ADC_CHANNEL);
            delay(1);
        }
        sum /= samples;

        float voltage_v = 0;
        if (cali_enable)
        {
            uint32_t voltage_mv = esp_adc_cal_raw_to_voltage(sum, &adc1_chars);
            voltage_v = voltage_mv / 1000.0f;
        }
        else
        {
            voltage_v = (sum * 3.3) / 4095.0;
        }

        float r_photo = (voltage_v * R_FIXED) / (3.3f - voltage_v);
        g_lux = pow((r_photo / R10), (1.0f / -GAMMA)) * 10.0f;

        vTaskDelay(pdMS_TO_TICKS(500)); // 每500ms更新一次
    }
}

/**
 * @brief 启动ADC后台任务
 */
void startADC()
{
    xTaskCreatePinnedToCore(ADC_Task, "ADC_Bg_Task", 2048, NULL, 1, NULL, 0);
}


/**
 * @brief [FreeRTOS Task] ADC数据显示任务
 * @param pvParameters 任务创建时传入的参数（未使用）
 * @details 这是一个在ADC菜单激活时运行的周期性任务，负责在屏幕上显示当前的ADC和光照强度信息。
 */
void ADC_Display_Task(void *pvParameters)
{
    volts.analogMeter(0, 0, 3.3f, "V", "0", "0.8", "1.6", "2.4", "3.3");

    while (!stopADCDisplayTask)
    {
        // 直接使用全局变量 g_lux，由后台任务更新
        float current_lux = g_lux;

        // --- ADC原始值和电压的计算（为了显示，这里需要重新计算） ---
        uint32_t sum = 0;
        const int samples = 10; // 显示任务可以减少采样次数
        for (int i = 0; i < samples; i++)
        {
            sum += adc1_get_raw(ADC_CHANNEL);
            delay(1);
        }
        sum /= samples;

        float voltage_v = 0;
        if (cali_enable)
        {
            uint32_t voltage_mv = esp_adc_cal_raw_to_voltage(sum, &adc1_chars);
            voltage_v = voltage_mv / 1000.0f;
        }
        else
        {
            voltage_v = (sum * 3.3) / 4095.0;
        }

        // --- 使用Sprite进行无闪烁更新 ---
        menuSprite.fillSprite(TFT_BLACK);
        menuSprite.setTextSize(2);
        menuSprite.setTextFont(1);
        menuSprite.setTextColor(TFT_WHITE, TFT_BLACK);

        // 显示光照强度
        char luxStr[10];
        dtostrf(current_lux, 4, 1, luxStr);
        menuSprite.setCursor(20, 10);
        menuSprite.print("LUX: "); menuSprite.print(luxStr);

        // 显示电压和ADC原始值
        char voltStr[10];
        dtostrf(voltage_v, 4, 2, voltStr);
        menuSprite.setCursor(20, 35);
        menuSprite.print("VOL: "); menuSprite.print(voltStr);

        menuSprite.setCursor(20, 60);
        menuSprite.print("ADC: "); menuSprite.print(sum);

        // 绘制光照强度进度条
        float constrainedLux = constrain(current_lux, 0.0f, 1000.0f);
        int barWidth = map((long) constrainedLux, 0L, 1000L, 0L, 200L); // 映射范围调整为0-1000
        menuSprite.drawRect(20, 85, 202, 22, TFT_WHITE);
        menuSprite.fillRect(21, 86, 200, 20, TFT_BLACK);
        menuSprite.fillRect(21, 86, barWidth, 20, TFT_GREEN);

        menuSprite.pushSprite(0, 130);

        vTaskDelay(pdMS_TO_TICKS(100));
    }
    vTaskDelete(NULL);
}


/**
 * @brief ADC菜单的入口函数
 * @details 此函数负责启动ADC数据显示任务，并处理用户的退出操作。
 */
void ADCMenu()
{
    stopADCDisplayTask = false;
    tft.fillScreen(TFT_BLACK);

    // 创建并启动ADC显示任务
    xTaskCreatePinnedToCore(ADC_Display_Task, "ADC_Display", 4096, NULL, 1, NULL, 0);

    while (1)
    {
        if (exitSubMenu || g_alarm_is_ringing || readButton())
        {
            exitSubMenu = false;
            stopADCDisplayTask = true;
            vTaskDelay(pdMS_TO_TICKS(200)); // 等待任务结束
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
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
// 标志位，用于通知ADC任务停止运行
bool stopADCTask = false;

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
 * @brief [FreeRTOS Task] ADC数据显示任务
 * @param pvParameters 任务创建时传入的参数（未使用）
 * @details 这是一个周期性任务，负责：
 *          1. 多次采样ADC原始值并取平均，以减少噪声。
 *          2. 将原始值转换为电压值（如果校准成功则使用校准曲线）。
 *          3. 根据电压值计算光敏电阻的阻值，并估算环境光照强度（Lux）。
 *          4. 在屏幕上通过仪表盘、文本和进度条显示电压、ADC原始值和光照强度。
 */
void ADC_Task(void *pvParameters)
{
    // 初始化电压表盘控件
    volts.analogMeter(0, 0, 3.3f, "V", "0", "0.8", "1.6", "2.4", "3.3");

    // --- 光照强度计算相关常量 ---
    const float R_FIXED = 20000.0f; // 分压电路中的固定电阻阻值 (20KΩ)
    const float R10 = 8000.0f;      // 光敏电阻在10 Lux光照下的参考阻值 (8KΩ)
    const float GAMMA = 0.6f;       // 光敏电阻的GAMMA值，描述其阻值与光照的非线性关系

    // 任务主循环
    while (!stopADCTask)
    {
        // --- 多重采样求平均值 ---
        uint32_t sum = 0;
        const int samples = 50; // 采样次数
        for (int i = 0; i < samples; i++)
        {
            sum += adc1_get_raw(ADC_CHANNEL);
            delay(1); // 短暂延迟
        }
        sum /= samples; // 计算平均值

        // --- ADC原始值转电压 ---
        float voltage_v = 0;
        if (cali_enable)
        {
            // 如果校准成功，使用校准函数将原始值转换为毫伏，再转为伏
            uint32_t voltage_mv = esp_adc_cal_raw_to_voltage(sum, &adc1_chars);
            voltage_v = voltage_mv / 1000.0f;
        }
        else
        {
            // 如果未校准，使用简单的线性比例关系计算电压
            voltage_v = (sum * 3.3) / 4095.0;
        }
        // 更新仪表盘指针
        menuSprite.setTextSize(1);
        volts.updateNeedle(voltage_v, 0);

        // --- 使用Sprite进行无闪烁更新 ---
        menuSprite.fillSprite(TFT_BLACK); // 清空Sprite
        menuSprite.setTextSize(2);
        menuSprite.setTextFont(1);// 设置字体和大小防止混乱
        menuSprite.setTextColor(TFT_WHITE, TFT_BLACK);

        // --- 计算并显示光照强度(Lux) ---
        // 根据分压公式计算光敏电阻的实时阻值
        float r_photo = (voltage_v * R_FIXED) / (3.3f - voltage_v);
        // 根据光敏电阻的特性曲线公式估算光照强度
        float lux = pow((r_photo / R10), (1.0f / -GAMMA)) * 10.0f;

        char luxStr[10];
        dtostrf(lux, 4, 1, luxStr); // 将float转为字符串
        menuSprite.setCursor(20, 10);
        menuSprite.print("LUX: "); menuSprite.print(luxStr);

        // --- 显示电压和ADC原始值 ---
        char voltStr[10];
        dtostrf(voltage_v, 4, 2, voltStr);
        menuSprite.setCursor(20, 35);
        menuSprite.print("VOL: "); menuSprite.print(voltStr);

        menuSprite.setCursor(20, 60);
        menuSprite.print("ADC: "); menuSprite.print(sum);

        // --- 绘制光照强度进度条 ---
        // 将Lux值限制在0-1000范围内，并映射到0-100的范围用于进度条
        float constrainedLux = constrain(lux, 0.0f, 1000.0f);
        int barWidth = map((long) constrainedLux, 0L, 100L, 0L, 200L);
        menuSprite.drawRect(20, 85, 202, 22, TFT_WHITE); // 进度条边框
        menuSprite.fillRect(21, 86, 200, 20, TFT_BLACK); // 进度条背景
        menuSprite.fillRect(21, 86, barWidth, 20, TFT_GREEN); // 进度条填充

        // 将Sprite内容推送到屏幕的指定位置
        menuSprite.pushSprite(0, 130);

        // 任务延迟，控制刷新率
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    // 任务结束前删除自身
    vTaskDelete(NULL);
}

/**
 * @brief ADC菜单的入口函数
 * @details 此函数负责启动ADC数据显示任务，并处理用户的退出操作。
 *          当用户通过按钮或闹钟等方式退出时，它会设置停止标志并安全地清理任务。
 */
void ADCMenu()
{
    stopADCTask = false; // 重置停止标志
    tft.fillScreen(TFT_BLACK); // 清屏

    // 创建并启动ADC任务，固定在核心0上运行
    xTaskCreatePinnedToCore(ADC_Task, "ADC_Task", 4096, NULL, 1, NULL, 0);

    // 循环等待退出信号
    while (1)
    {
        // 检查全局子菜单退出标志
        if (exitSubMenu)
        {
            exitSubMenu = false; // 重置标志
            stopADCTask = true; // 通知ADC任务停止
            vTaskDelay(pdMS_TO_TICKS(200)); // 等待任务处理停止信号
            break; // 退出循环
        }
        // 检查全局闹钟响铃标志
        if (g_alarm_is_ringing)
        {
            stopADCTask = true; // 通知ADC任务停止
            vTaskDelay(pdMS_TO_TICKS(200)); // 等待任务处理停止信号
            break; // 退出循环
        }
        // 检查按钮短按事件
        if (readButton())
        {
            stopADCTask = true; // 通知ADC任务停止
            vTaskDelay(pdMS_TO_TICKS(200)); // 等待任务处理停止信号
            break; // 退出循环
        }
        // 短暂延迟，避免CPU空转
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
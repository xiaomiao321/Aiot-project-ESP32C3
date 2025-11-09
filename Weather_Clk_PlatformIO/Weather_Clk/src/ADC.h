#ifndef ADC_H
#define ADC_H


/**
 * @brief 配置并初始化ADC
 * @details 此函数设置ADC的位宽（12位）和衰减（12dB），
 *          并尝试加载eFuse中的校准值以提高测量精度。
 */
void setupADC();

/**
 * @brief 启动ADC后台读取任务
 * @details 创建一个FreeRTOS任务，在后台持续读取ADC值并更新全局的光照强度变量g_lux。
 */
void startADC();

/**
 * @brief ADC功能演示的菜单入口函数。
 * @details 该函数清空屏幕，然后创建一个FreeRTOS任务(ADC_Task)来持续读取
 *          光敏电阻的电压值，并将其转换为光照强度（Lux）和电压值显示在屏幕上。
 *          同时，它会监听退出信号（如按钮按下或闹钟触发），并负责安全地停止和清理相关任务。
 */
void ADCMenu();
#endif 
#ifndef SYSTEM_H
#define SYSTEM_H


extern int tft_log_y;
extern int current_log_lines;
const int LOG_MARGIN = 5;
const int LOG_LINE_HEIGHT = 12;
const int LOG_MAX_LINES = 15;


/**
 * @brief 系统启动函数。
 * @details 这是系统的总入口点，负责所有硬件和软件模块的初始化，
 *          包括串口、EEPROM、蜂鸣器、编码器、传感器、TFT屏幕、ADC、WiFi、MQTT等。
 *          它还会执行一个开机动画，并同步网络时间，最后显示主菜单。
 */
void bootSystem();

/**
 * @brief 在TFT屏幕上打印一行带打字机效果的日志。
 * @param text 要显示的日志文本。
 * @param color 文本的颜色。
 * @details 日志会自动滚屏。当日志满一屏时，会暂停一秒后清屏。
 */
void tftLog(String text, uint16_t color);

/**
 * @brief 打印一条信息级别的日志（青色）。
 * @param text 日志文本。
 */
void tftLogInfo(const String& text);

/**
 * @brief 打印一条警告级别的日志（黄色）。
 * @param text 日志文本。
 */
void tftLogWarning(const String& text);

/**
 * @brief 打印一条错误级别的日志（红色）。
 * @param text 日志文本。
 */
void tftLogError(const String& text);

/**
 * @brief 打印一条成功级别的日志（绿色）。
 * @param text 日志文本。
 */
void tftLogSuccess(const String& text);

/**
 * @brief 打印一条调试级别的日志（品红色）。
 * @param text 日志文本。
 */
void tftLogDebug(const String& text);

/**
 * @brief 打印一条带图标的日志。
 * @param text 日志文本。
 * @param color 文本颜色。
 * @param icon 要显示在文本前的图标字符串（例如 "✓" 或 "✗"）。
 */
void tftLogWithIcon(const String& text, uint16_t color, const char* icon);

/**
 * @brief 打印一条带进度信息的日志。
 * @param text 日志主题文本。
 * @param progress 当前进度。
 * @param total 总进度。
 * @details 根据进度显示不同的状态，例如 "Loading..."、"Loading (50%)" 或 "Loading ✓"。
 */
void tftLogProgress(const String& text, int progress, int total);

/**
 * @brief 清空TFT屏幕上的日志区域。
 * @details 将屏幕填充为黑色，并将日志打印的垂直位置重置到起始点。
 */
void tftClearLog();
#endif // !SYSTEM_H
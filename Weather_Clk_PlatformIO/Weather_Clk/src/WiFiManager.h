#ifndef WIFIMANAGER_H
#define WIFIMANAGER_H

/**
 * @brief 尝试使用保存的凭据连接WiFi，如果失败则启动配置门户。
 * @details 此函数首先会尝试使用存储在Preferences（EEPROM）中的SSID和密码进行连接。
 *          如果连接失败，它会启动一个AP（接入点）模式，其SSID为 "WeatherClock_Setup"。
 *          用户可以连接到此AP，并通过浏览器访问 "192.168.4.1" 来扫描、选择并输入新WiFi的凭据。
 *          保存新凭据后，设备将自动重启并尝试使用新设置进行连接。
 * @return 如果设备在调用时已经或成功连接到WiFi，则返回 `true`。
 *         如果启动了配置门户，则函数会进入一个无限循环来处理网页请求，不会返回。
 */
bool connectWiFi_with_Manager();

#endif // WIFIMANAGER_H

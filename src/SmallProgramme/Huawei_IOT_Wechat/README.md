# 微信小程序 

本项目是与 AIoT 多功能时钟配套的微信小程序代码，用于实现设备数据的可视化、远程控制以及用户交互。

## 概览

### 功能特性

*   **设备数据展示**: 实时显示来自 AIoT 时钟的温度、光照强度、电脑性能（GPU、CPU、RAM）和 ESP32 芯片温度等传感器数据。
*   **远程控制**: 通过小程序远程控制设备的 RGB LED 颜色和开关，支持单灯、全灯控制及彩虹模式。
*   **音乐播放控制**: 远程选择和控制设备上播放的音乐，并可选择设备端的 UI 模式。
*   **闹钟设置**: 远程设置设备的闹钟时间、重复星期和开关状态。
*   **华为云集成**: 与华为云 IoTDA 平台进行数据交互，包括获取设备影子和下发命令。
*   **刷新频率设置**: 用户可自定义设备数据刷新频率。

### 核心文件与目录说明

*   **app.js**: 小程序的入口文件，处理应用生命周期事件（如 `onLaunch`）和全局数据。
*   **app.json**: 小程序的全局配置文件，定义页面路径、窗口表现、网络超时等。
*   **app.wxss**: 小程序的全局样式文件，定义公共样式。
*   **project.config.json**: 开发者工具的项目配置文件，包含 `appid`、编译设置、项目名称等。
*   **pages/**: 存放所有功能页面。
    *   **pages/device/**: 设备主页，负责获取 Token 和显示设备状态数据。
    *   **pages/led_control/**: LED 控制页面，用于远程控制 RGB 灯。
    *   **pages/music_player/**: 音乐播放器页面，用于远程控制音乐播放。
    *   **pages/alarm_setting/**: 闹钟设置页面，用于远程设置闹钟。
    *   **pages/README.md**: 详细描述了 `pages` 目录下各个页面的功能。
*   **utils/util.js**: 存放一些公共的工具函数。

## 配置与部署

1.  **导入项目**: 使用微信开发者工具，选择导入项目，导入当前目录 (`SmallProgramme/Huawei_IOT_Wechat/`)。
2.  **AppID 配置**: 在 `project.config.json` 文件中，将 `appid` 替换为您的微信小程序 AppID。
3.  **华为云连接参数**: 在 `pages/device/device.js`、`pages/led_control/led_control.js`、`pages/music_player/music_player.js` 和 `pages/alarm_setting/alarm_setting.js` 文件中，需要修改以下硬编码的华为云连接参数，替换为实际项目信息：
    *   `domainname`
    *   `username`
    *   `password`
    *   `projectId`
    *   `deviceId`
    *   `iamhttps`
    *   `iotdahttps`
4.  **运行**: 配置完成后，即可在微信开发者工具中预览和调试小程序。

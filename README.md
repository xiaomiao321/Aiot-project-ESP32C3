# AIoT 多功能时钟 (AIoT Multifunctional Clock)

**25-26秋学期智能物联网(AIoT)系统设计期末大作业——第十组**

## 实物展示 

![正面](img/正面.jpg)
![背面](img/背面.jpg)

## 功能概述 

一个能连接华为云的多功能时钟，具有以下不同的功能：

1.  **时钟**: NTP网络授时，多款表盘，天气显示。
2.  **音乐播放器**: 蜂鸣器播放内置音乐，支持多种播放模式。
3.  **互联网资讯**: 获取土味情话、每日英语、诗词、汇率、热榜等。
4.  **太空信息**: 查看国际空间站位置和宇航员信息。
5.  **实用工具**: 闹钟、倒计时、番茄钟、秒表。
6.  **电脑性能监测**: 通过串口实时显示PC性能数据。
7.  **环境监测**: 监测温度、光照强度并上传华为云。
8.  **娱乐**: 内置动画和Flappy Bird等小游戏。
9.  **LED控制**: 可通过设备或华为云远程设置背面RGB灯效。

更详细的功能介绍、硬件选型、电路设计和外壳结构，请参考 [实验报告](实验报告.md)。

## 项目结构 

```
.
├── Hardware/             # 硬件相关代码 (PlatformIO)
│   ├── Weather_Clk_PlatformIO/  <- 核心固件项目 (Main Firmware)
│   └── Test_*_PlatformIO/       <- 各功能模块的测试项目 (Test Projects)
├── SmallProgramme/       # 微信小程序代码
│   └── Huawei_IOT_Wechat/       <- 核心小程序项目 (Main Mini Program)
├── 实验报告.md           # 详细项目报告
└── README.md             # 本文档
```

-   `./Hardware`: 存放所有硬件相关的 PlatformIO 项目。
    -   `Weather_Clk_PlatformIO` 是设备的主程序项目。
    -   其他 `Test_*` 目录是开发过程中的功能测试项目。
-   `./SmallProgramme`: 存放微信小程序的前端代码。
    -   `Huawei_IOT_Wechat` 是与硬件配套的小程序项目。

## 常见问题与解决 

1.  **Wi-Fi 连接失败**:
    *   **现象**: 设备反复尝试但无法连接到 Wi-Fi。
    *   **解决方案**:
        1.  尝试关闭手机热点的“省电模式”。
        2.  在固件中，已通过以下代码优化连接稳定性，确保它们被执行：
            ```cpp
            WiFi.mode(WIFI_STA);
            WiFi.setAutoReconnect(true);
            WiFi.persistent(true);
            WiFi.setSleep(false); // 关闭 Wi-Fi 休眠
            WiFi.setTxPower(WIFI_POWER_19_5dBm); // 设置最大发射功率
            WiFi.begin(ssid, pass);
            ```
        3.  用手按住开发板的芯片，事实证明这个最管用，怀疑开发板有虚焊部分
2.  **华为云连接失败 (DNS Fails)**:
    
    *   **现象**: 日志输出 `DNS Failed` 错误。
        ![DNS_Failed](img/DNS_Failed.png)
    *   **解决方案**: 这个问题的触发条件比较奇怪。关闭作为热点的手机的省电模式，让设备重新连接热点后，通常能解决。

## 源码

所有源代码，包括硬件固件和微信小程序，都已托管在 GitHub:

[https://github.com/xiaomiao321/Aiot-project-ESP32C3](https://github.com/xiaomiao321/Aiot-project-ESP32C3)

# 硬件固件说明

本目录包含了项目所有的硬件相关代码，均使用 [PlatformIO](https://platformio.org/) 进行开发和管理。主要是ArduinoIDE编译实在是太慢了，而且VSCode无法自动补全，之前尝试了半天，加入屏幕代码总是无限重启，参考[【原创奖励】esp32s3使用platformio 点亮1.69寸TFT历程-面包板社区](https://mbb.eet-china.com/blog/3924658-463999.html)，更改配置文件`platformio.ini`后才得以解决。

所有测试代码都可在`Test_*_PlatformIO/`查看

## 目录结构

-   **`Weather_Clk_PlatformIO/`**: **主固件项目**。这是最终烧录在设备上的完整程序，实现了项目的所有功能。
-   **`Test_*/`**: 开发过程中用于测试特定功能（如 WIFI、TFT 屏幕、传感器等）的独立小项目。这些项目不用于最终部署，仅供调试和参考。

## 开发环境与重要配置

主控板为 `ESP32-C3-SuperMini`。在开发过程中，遇到了一些需要特别注意的配置问题，记录如下。

### TFT 屏幕 (TFT_eSPI) 移植要点

将使用 `TFT_eSPI` 库的 Arduino 项目移植到 PlatformIO 时，可能会遇到屏幕无法点亮或设备反复重启的问题。

关键在于 `platformio.ini` 的配置。必须指定一个兼容的 `espressif32` platform 版本。经过测试，`6.5.0` 版本可以成功驱动屏幕。

以下是 `Weather_Clk_PlatformIO` 项目中 `platformio.ini` 的关键配置，可供参考：

```ini
[env:esp32-c3-devkitc-02]
platform = espressif32@6.5.0
board = esp32-c3-devkitc-02
framework = arduino
build_flags = 
    -D ARDUINO_USB_MODE=1
    -D ARDUINO_USB_CDC_ON_BOOT=1
board_build.partitions = partition.csv
```

**注意**:
1.  `platform = espressif32@6.5.0` 是成功点亮屏幕的关键。更高版本的 `espressif32` 平台可能存在兼容性问题。
2.  `TFT_eSPI` 库本身的配置文件 (`User_Setup.h` 或 `User_Setup_Select.h`) 仍需根据你使用的屏幕型号和引脚连接进行正确配置。具体配置方法可参考库的文档或 `Test_PlatformIO_TFT` 项目中的示例。

![屏幕点亮](../img/屏幕点亮.jpg)

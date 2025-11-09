Arduino编译实在是太慢了，而且几乎每次都要重头编译所有的库，对于文件稍微多一些的项目就要编译将近十分钟，非常不便于调试，所以我一直有把ArduinoIDE上的项目移植到PlatformIO的想法，但是苦于TFT_eSPI库移植只有屏幕一直无法点亮，似乎一直在重启。只能一直搁置，饱受缓慢编译速度的折磨。
直到今天，看到了一篇文章[【原创奖励】esp32s3使用platformio 点亮1.69寸TFT历程-面包板社区](https://mbb.eet-china.com/blog/3924658-463999.html)

其中提到，在platformIO设置`platformio.ini`配置文件为以下内容

```ini
[env:esp32-c3-devkitc-02]
platform = espressif32@6.5.0
board = esp32-c3-devkitc-02
framework = arduino
build_flags = 
    -D ARDUINO_USB_MODE=1
    -D ARDUINO_USB_CDC_ON_BOOT=1
```

其中`@6.5.0`似乎是最关键的部分，更高版本的无法兼容，修改后成功点亮

测试代码：

```cpp
#include <Arduino.h>
#include <TFT_eSPI.h>       
TFT_eSPI tft = TFT_eSPI();

void setup()
{
  tft.init();
  tft.setRotation(0);
  tft.fillScreen(TFT_RED);
  tft.setTextColor(TFT_WHITE, TFT_RED);
  tft.drawString("Hello ESP32-C3 Super Mini!", 10, 10, 2);
}

void loop()
{
}
```

效果展示
![屏幕点亮](..\img\屏幕点亮.jpg)

`TFT_eSPI`库的配置参考[Arduino IDE下的ESP32-C3 驱动ST7789TFT屏幕(TFT_eSPI库)_arduino st7789-CSDN博客](https://blog.csdn.net/xiaomiao2006/article/details/151123327?spm=1011.2124.3001.6209)
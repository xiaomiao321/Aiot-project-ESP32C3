#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <LittleFS.h>

#include "Menu.h"
#include "System.h"
#include "MQTT.h" 

// --- Global Action Request System ---
volatile void (*requestedSongAction)(int) = NULL;
volatile int requestedSongIndex = -1;

/**
 * @brief 程序入口点和初始化函数
 * @details 此函数在设备启动时仅运行一次。
 *          它调用 bootSystem() 函数来执行所有的初始设置，
 *          包括硬件初始化、文件系统挂载、WiFi连接、时间同步等。
 */
void setup()
{
    bootSystem(); // 调用系统启动函数
}

/**
 * @brief 主循环函数
 * @details setup() 函数执行完毕后，此函数会反复执行。
 *          它构成了程序的主事件循环。
 *          - showMenu(): 处理和显示主菜单界面及各个子菜单的逻辑。
 *          - vTaskDelay(): FreeRTOS的延时函数，让出CPU给其他任务，避免空转。
 */
void loop()
{
    // 检查是否有从后台任务（如MQTT）发起的UI操作请求
    if (requestedSongAction != NULL) {
        void (*action)(int) = (void (*)(int))requestedSongAction; // 复制函数指针
        int songIndex = requestedSongIndex; // 复制歌曲索引

        // 清除请求标志
        requestedSongAction = NULL;
        requestedSongIndex = -1;
        
        action(songIndex); // 执行请求的函数 (例如 play_song_full_ui)
        
        // 当阻塞的UI函数返回后，重新绘制主菜单以确保UI状态一致
        showMenuConfig(); 
    }

    showMenu(); // 显示和处理菜单逻辑
    vTaskDelay(pdMS_TO_TICKS(15)); // 短暂延时，让出CPU
}

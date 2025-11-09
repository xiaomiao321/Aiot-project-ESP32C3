#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <LittleFS.h>

#include "Menu.h"
#include "System.h"
#include "MQTT.h" 



void setup()
{
    bootSystem();
}

void loop()
{
    showMenu();
    loopMQTT();
    vTaskDelay(pdMS_TO_TICKS(15));
}
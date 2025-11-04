#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <LittleFS.h>

#include "Menu.h"
#include "System.h"
#include "MQTT.h" 
#include "weather.h"
#include <random>
#include "DS18B20.h"
#include "WiFiManager.h"
#include "Alarm.h"
#include "TargetSettings.h"



void setup() {
    bootSystem();
}

void loop() {
    showMenu();
    vTaskDelay(pdMS_TO_TICKS(15));
}
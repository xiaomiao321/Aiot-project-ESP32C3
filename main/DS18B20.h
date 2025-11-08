#ifndef __DS18B20_H
#define __DS18B20_H

extern float g_currentTemperature;

void DS18B20_Init();
void createDS18B20Task();
float getDS18B20Temp();
void DS18B20Menu();

#endif
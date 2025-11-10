#include <Arduino.h>

#define LED_PIN       8
int myFunction(int, int);

void setup()
{
  Serial.begin(9600);
  pinMode(LED_PIN, OUTPUT);

}

void loop()
{

  digitalWrite(LED_PIN, HIGH);
  Serial.println("Hello, World!");
  delay(500);
  digitalWrite(LED_PIN, LOW);
  delay(500);

}
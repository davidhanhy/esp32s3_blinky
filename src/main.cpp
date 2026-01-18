#include <Arduino.h>
#define BLYNK_TEMPLATE_ID "TMPL6_W46wHzD"
#define BLYNK_TEMPLATE_NAME "LED ESP32 1"
#define BLYNK_AUTH_TOKEN "gmEzgo1Ck-CNKovfdwFRdq8kQPDkTvRG"
#include <BlynkSimpleEsp32.h>
#include "wifiConfig.h"
const int PIN_LED_TARGET = 48;
const int PIN_LDR = 1;
Config wifiConfig;
BlynkTimer timer;
int ledMode = 0;
bool swState = 0;
int ledState = LOW;
unsigned long previousMillis = 0;
BLYNK_CONNECTED() { Blynk.syncVirtual(V1, V2, V3, V4); }
void sendSensorData()
{
    if (Blynk.connected())
    {
        int rawValue = analogRead(PIN_LDR);
        int lightPercent = map(rawValue, 0, 4095, 0, 100);
        Blynk.virtualWrite(V2, lightPercent);
        if (swState == 0) // AUTO MODE
        {
            if (lightPercent <= 20)
            {
                ledMode = 1;
                Blynk.virtualWrite(V3, "Troi toi!!!");
            }
            else
            {
                ledMode = 0;
                Blynk.virtualWrite(V3, "Troi sang!!!");
            }
        }

        Serial.print("Raw ADC: ");
        Serial.print(rawValue);
        Serial.print(" | Do sang: ");
        Serial.print(lightPercent);
        Serial.println(" %");
    }
}
void handleLedEffect()
{
    unsigned long currentMillis = millis();
    switch (ledMode)
    {
    case 0:
        analogWrite(PIN_LED_TARGET, 0);
        break;
    case 1:
        if (currentMillis - previousMillis >= 100)
        {
            previousMillis = currentMillis;
            ledState = !ledState;
            analogWrite(PIN_LED_TARGET, ledState ? 255 : 0);
        }
        break;
    }
}
BLYNK_WRITE(V1)
{
    ledMode++;
    if (ledMode > 1)
        ledMode = 0;
}
BLYNK_WRITE(V4)
{
    if (param.asInt() == 1)
    {
        swState = 1;
    }
    else
        swState = 0;
}
void setup()
{
    Serial.begin(115200);
    pinMode(PIN_LED_TARGET, OUTPUT);
    pinMode(6, OUTPUT);
    pinMode(PIN_LDR, INPUT);
    analogReadResolution(12);

    wifiConfig.begin();
    Blynk.config(BLYNK_AUTH_TOKEN);

    timer.setInterval(1000L, sendSensorData);
}
void loop()
{
    wifiConfig.run();
    timer.run();

    if (wifiMode == 1 && WiFi.status() == WL_CONNECTED)
    {
        if (Blynk.connected())
            Blynk.run();
        else
        {
            static unsigned long lastTry = 0;
            if (millis() - lastTry > 5000)
            {
                Blynk.connect(1000);
                lastTry = millis();
            }
        }
    }
    handleLedEffect();
}
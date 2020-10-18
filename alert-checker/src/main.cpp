#include "MotionDetector.h"
#include <Arduino.h>
#include <Countdown.h>
#include <DallasTemperature.h>
#include <ESP8266WiFi.h>
#include <OneWire.h>

#define ENABLE_TEMP_SENSOR
//#define TEST_MODE

// -------------------------------------------------------------------------------------------------

struct Resources
{

    // ---------------------------------------------------------------------------------------------

    struct EarlyInit
    {
        EarlyInit()
        {
            Serial.begin(230400);
            while(!Serial)
                delay(10);
        }
    } _;

#ifdef ENABLE_TEMP_SENSOR
    OneWire temp_ow{ D5 };
    DallasTemperature temp_sensors{ &temp_ow };
    DeviceAddress temperature_sensor_address{ 0xff };
#ifdef TEST_MODE
    Countdown temperature_probe_trigger{ 6 };
#else
    Countdown temperature_probe_trigger{ 60 };
#endif
    std::function<void()> on_temperature_timeout{ [&]() {
        measureTemperature();
        updateTemperature();
        temperature_probe_trigger.reset();
    } };
#endif

#ifdef TEST_MODE
    MotionDetector monitor{ D3, D2, 20, true };
    Countdown standby_officer{ 5 };
#else
    MotionDetector monitor{ D3, D2, 20, false };
    Countdown standby_officer{ 5 };
#endif
    std::function<void()> on_timeout_trigger{ [&]() { onTriggerEnd(); } };

    // ---------------------------------------------------------------------------------------------

    void setup()
    {
        WiFi.persistent(false);
        onTriggerEnd();
        WiFi.setOutputPower(21);

#ifdef ENABLE_TEMP_SENSOR
        temp_sensors.begin();
        if(!temp_sensors.getAddress(temperature_sensor_address, 0))
            Serial.printf("failed to find sensor address\n");
        else
        {
            Serial.printf("found sensor on address: ");
            for(uint8_t i = 0; i < 8; i++)
            {
                if(temperature_sensor_address[i] < 16)
                    Serial.print("0");
                Serial.print(temperature_sensor_address[i], HEX);
            }
            Serial.println();
        }
#endif

        //#ifdef TEST_MODE
        pinMode(LED_BUILTIN, OUTPUT);
        //#else
        //        wifi_status_led_uninstall();
        //#endif

        temperature_probe_trigger.enable();

        monitor.setup(
        [&](bool movement_detected) { return onMovementChangeCallback(movement_detected); });
    }

    // ---------------------------------------------------------------------------------------------

    void process()
    {
#ifdef ENABLE_TEMP_SENSOR
        temperature_probe_trigger.process(on_temperature_timeout);
#endif
        monitor.process();
        standby_officer.process(on_timeout_trigger);
    }

#ifdef ENABLE_TEMP_SENSOR
    // ---------------------------------------------------------------------------------------------

    void measureTemperature()
    {
        temp_sensors.requestTemperaturesByAddress(temperature_sensor_address);
    }

    // ---------------------------------------------------------------------------------------------

    void updateTemperature()
    {
        monitor.setTemperatureC(static_cast<int16_t>(roundf(temp_sensors.getTempC(temperature_sensor_address))));
    }
#endif

    // ---------------------------------------------------------------------------------------------

    void onMovementChangeCallback(bool movement_detected)
    {
        if(movement_detected)
        {
            standby_officer.disable();
            standby_officer.reset();
            onTrigger();
        }
        else
        {
            standby_officer.enable();
        }
    }

    // ---------------------------------------------------------------------------------------------

    void onTrigger()
    {
        if(WIFI_AP != WiFi.getMode())
        {
            digitalWrite(LED_BUILTIN, LOW);
            Serial.println("signal detected movement");
            WiFi.softAP("x", nullptr, 1);
            WiFi.enableAP(true);
        }
    }
    void onTriggerEnd()
    {
        digitalWrite(LED_BUILTIN, HIGH);
        Serial.println("detected movement timed out");
        WiFi.enableAP(false);
        WiFi.forceSleepBegin();
        monitor.skipSamples(5);
    }

} r;

// ---------------------------------------------------------------------------------------------

void setup() { r.setup(); }

void loop() { r.process(); }

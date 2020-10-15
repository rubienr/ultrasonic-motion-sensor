#include "MovementMonitor.h"
#include <Arduino.h>
#include <Countdown.h>
#include <ESP8266WiFi.h>

#define TEST_MODE
struct Resources
{
    struct EarlyInit
    {
        EarlyInit()
        {
            Serial.begin(230400);
            while(!Serial)
                delay(10);
        }
    } _;

#ifdef TEST_MODE
    MovementMonitor monitor{ D3, D2, 20, true};
    Countdown standby_officer{ 5 };
#else
    MovementMonitor monitor{ D3, D2, 20, false };
    Countdown standby_officer{20};
#endif
    std::function<void()> on_timeout_trigger {[&](){return onTriggerEnd();}};

    void setup()
    {
        WiFi.persistent(false);
        onTriggerEnd();
        wifi_status_led_uninstall();
        WiFi.setOutputPower(21);

        pinMode(LED_BUILTIN, OUTPUT);

        monitor.setup(
        [&](bool movement_detected) { return onMovementChangeCallback(movement_detected); });
    }
    void process() {
        monitor.process();
        standby_officer.process(on_timeout_trigger);
    }

    void onMovementChangeCallback(bool movement_detected)
    {
        if(movement_detected)
        {
            digitalWrite(LED_BUILTIN, LOW);
            standby_officer.disable();
            onTrigger();
        }
        else
        {
            digitalWrite(LED_BUILTIN, HIGH);
            if (!standby_officer.isEnabled())
            {
                standby_officer.reset();
                standby_officer.enable();
            }
        }
    }

    void onTrigger() {
        if (WIFI_AP != WiFi.getMode())
        {
            Serial.println("signal detected movement");
            WiFi.softAP("x", nullptr,1);
            WiFi.enableAP(true);
        }
    }
    void onTriggerEnd()
    {
        Serial.println("detected movement timed out");
        WiFi.enableAP(false);
        WiFi.forceSleepBegin();
        monitor.skipSamples(5);
    }

} r;

void setup() { r.setup(); }

void loop() { r.process(); }

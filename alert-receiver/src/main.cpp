#include <Arduino.h>
#include <Countdown.h>
#include <Display.h>
#include <ESP8266WiFi.h>
#include <elapsedMillis.h>

//#define TEST_MODE
#define BUZZER_PIN D8
#define BUTTON_PIN D3

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

    // -----

#ifdef TEST_MODE
    const bool verbose{ true };
#else
    const bool verbose{ false };
#endif

    // -----

    Display display;
    elapsedMillis time_since_last_display_refresh_ms;
    const uint8_t refresh_display_every_ms{ 125 };

    // -----

#ifdef TEST_MODE
    const uint8_t min_scans_no_wifi_disable_alert{ 20 };
#else
    const uint8_t min_scans_no_wifi_disable_alert{ 30 };
#endif

    // -----

    struct Alerting
    {
        bool is_active{ false };
        elapsedMillis alert_age{ 0 };
        uint16_t total_count{ 0 };
        uint8_t silence_next_alerts_count{ 0 };

        int32_t channel{ -1 };
        int32_t rssi{ -200 };

    } alerting;

    // -----

    void setup()
    {
        WiFi.persistent(false);
        WiFi.mode(WIFI_STA);
        WiFi.disconnect();
        wifi_status_led_uninstall();

        pinMode(BUZZER_PIN, OUTPUT);
        pinMode(BUTTON_PIN, INPUT);

        display.setRotation(2);
        display.clearDisplay();
        display.setup();
        display.dim(false);
    };

    void process()
    {
        doAsyncScan();
        updateScreen();

        bool button_pressed = !digitalRead(BUTTON_PIN);
        if(button_pressed && alerting.is_active)
        {
            alerting.silence_next_alerts_count = 1;
        }
    };

    ///! scan start to end marked with { and }
    ///! alert evaluation start to end marked with (N and ) with N being the number of scanned SSIDs
    void doAsyncScan()
    {
        const int8_t scan_result{ WiFi.scanComplete() };
        if(WIFI_SCAN_RUNNING == scan_result)
            return;

        // ----- evaluate
        if(WIFI_SCAN_FAILED != scan_result)
        {
            const int8_t ssid_count{ scan_result };
            bool alert_signal_detected {false};

            alerting.channel = (-1);
            alerting.rssi = -200;

            if(ssid_count > 0)
            {
                alert_signal_detected = WiFi.SSID(0).compareTo("x") == 0;
                alerting.channel = WiFi.channel(0);
                alerting.rssi = WiFi.RSSI(0);
            }

            if(verbose)
                Serial.printf("(%d", ssid_count);
            computeAlert(alert_signal_detected);
            if(verbose)
                Serial.print(")}\n");
        }

        // ----- scan
        if(verbose)
            Serial.print('{');
        char ssid[]{ "x" };
        WiFi.scanNetworks(true, false, 1, static_cast<uint8 *>(static_cast<void *>(&ssid)));
    }

    ///! alert start and end marked with [ and ]
    ///! number of retries marked with <N> with N the pending retry count
    void computeAlert(bool alert_signal_detected)
    {
        static uint8_t no_signal_found_retries{ min_scans_no_wifi_disable_alert };

        if(alert_signal_detected)
        {
            if(!alerting.is_active)
            {
                if(verbose)
                    Serial.print("[");
                alerting.is_active = true;
                alerting.alert_age = 0;
                alerting.total_count++;
            }
            if(verbose)
                Serial.printf("<%d>", no_signal_found_retries);
            no_signal_found_retries = min_scans_no_wifi_disable_alert;
        }
        else
        {
            if(no_signal_found_retries > 0)
            {
                no_signal_found_retries--;
                if(verbose)
                    Serial.printf("<%d>", no_signal_found_retries);
            }
            if(alerting.is_active && no_signal_found_retries <= 0)
            {
                alerting.is_active = false;
                if(alerting.silence_next_alerts_count > 0)
                    alerting.silence_next_alerts_count--;
                if(verbose)
                    Serial.print("]");
            }
        }
        if(verbose)
            Serial.print(')');
    }

    ///! state marked with ! (alerting) and . (idle)
    void updateScreen()
    {
        static bool is_inverted{ false };

        if(time_since_last_display_refresh_ms < refresh_display_every_ms)
            return;
        time_since_last_display_refresh_ms = 0;

        if(alerting.is_active)
        {
            Serial.print('!');
            display.invertDisplay(is_inverted);
            is_inverted = !is_inverted;

            if(alerting.silence_next_alerts_count <= 0)
                digitalWrite(BUZZER_PIN, is_inverted);
            else
                digitalWrite(BUZZER_PIN, LOW);
        }
        else
        {
            Serial.print('.');
            display.invertDisplay(false);
            digitalWrite(BUZZER_PIN, LOW);
        }

        display.clearDisplay();
        display.setCursor(0, 0);
        display.printf("alerts       %3d\n", alerting.total_count);
        display.printf("alert age [s]%3lu\n", ((alerting.is_active) ? (alerting.alert_age / 1000) : 0));
        display.printf("silence next %3d\n", alerting.silence_next_alerts_count);
        display.printf("ch %2d RSSI %5d\n", alerting.channel, alerting.rssi);
        display.printf("\n\n\n            %9lu", millis() / 1000);
        display.display();
    }
} r;

void setup() { r.setup(); }

void loop() { r.process(); }

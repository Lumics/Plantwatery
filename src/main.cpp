#include <Arduino.h>

#include "driver/adc.h"
#include "time.h"
#include <PubSubClient.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <esp_bt.h>
#include <esp_sleep.h>
#include <esp_wifi.h>

// load Wifi credentials
#include "credentials.h"

/**********************************************************************************************/
#define mqtt_server "lumicsraspi4"  // CHANGE MQTT Broker address
#define mqtt_user \
    "your_username"  // not used, make a change in the mqtt initializer
#define mqtt_password \
    "your_password"  // not used, make a change in the mqtt initializer

#define uS_TO_S_FACTOR \
    1000000L                /* Conversion factor for micro seconds to seconds */
#define wifi_timeout 5000L  // 5 seconds in milliseconds

String ssid = WIFI_SSID;             // CHANGE (WiFi Name)
String pwd = WIFI_PASSWD;            // CHANGE (WiFi password)
String sensor_location = "balcony";  // define sensor location

// sensor settings
const int hum_threshold = 75;  // In percentage, If soil humidity under
                               // hum_threshold it will trigger the watering
const int hum_offset =
    1300;  // define humidity offset in raw ticks, measure by puting sensor into
           // water and take this raw value as offset

// timezone management
const char* ntpServer = "de.pool.ntp.org";
const char* timeZone = "CET-1CEST,M3.5.0,M10.5.0/3";

// watering time
const int watering_hour_am = 8;       // 8 am
const int watering_hour_pm = 16;      // 8 pm
const int watering_time = 30 * 1000;  // in mili seconds

// *** Hardware Definitions ***
#define Pin_pump 32         // define wemos pin for triggering pump
#define Pin_humi_sensor 33  // define analog pin to read from humidity sensor
// when wifi active only ADC1 available GPIO32 - GPIO39

// *** persistent data ***
RTC_DATA_ATTR int bootCount =
    0;  // Boot count should be stored in the non volatile RTC Memory
RTC_DATA_ATTR struct tm
    last_watering_time;  // Parameters see:
                         // http://www.cplusplus.com/reference/ctime/tm/
RTC_DATA_ATTR time_t now;

/***********************************************************************************************/

int hum_raw = 0;

String mac;
String MAC_ADDRESS;
WiFiClient espClient;
PubSubClient client(espClient);
String sensor_topic = "sensors/soil";
String device_type = "Arduino";
String device_name = "esp32_plantwatery";
int wifi_setup_timer = 0;

int sleepTimeS = 1 * 60L;  // 1 min for debugging

void setup_wifi() {
    char ssid1[30];
    char pass1[30];
    ssid.toCharArray(ssid1, 30);
    pwd.toCharArray(pass1, 30);
    Serial.println();
    log_i("Connecting to: %s", ssid1);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid1, pass1);
    while (WiFi.status() != WL_CONNECTED) {
        delay(10);
        Serial.print(".");
        wifi_setup_timer++;
        if (wifi_setup_timer >
            999) {  // Restart the ESP if no connection can be established
            ESP.restart();
        }
    }
    Serial.println("");
    log_i("WiFi connected");
    log_i("IP address: %d", WiFi.localIP());
}

void goToDeepSleep() {
    // testing
    sleepTimeS = 60;
    log_i("Going to sleep for %d seconds", sleepTimeS);
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    btStop();

    esp_wifi_stop();
    esp_bt_controller_disable();

    // Configure the timer to wake us up!
    esp_sleep_enable_timer_wakeup(sleepTimeS * uS_TO_S_FACTOR);
    Serial.flush();
    // give enough time to print everything to serial
    delay(100);
    // Go to sleep! Zzzz
    esp_deep_sleep_start();
    log_e("this should not happen");
}

void print_wakeup_reason() {
    esp_sleep_wakeup_cause_t wakeup_reason;

    wakeup_reason = esp_sleep_get_wakeup_cause();

    switch (wakeup_reason) {
        case 1:
            log_i("Wakeup caused by external signal using RTC_IO");
            break;
        case 2:
            log_i("Wakeup caused by external signal using RTC_CNTL");
            break;
        case 3:
            log_i("Wakeup caused by timer");
            break;
        case 4:
            log_i("Wakeup caused by touchpad");
            break;
        case 5:
            log_i("Wakeup caused by ULP program");
            break;
        default:
            log_i("Wakeup was not caused by deep sleep");
            break;
    }
}

void setTimeZone() {
    configTime(0, 0, ntpServer);
    delay(100);
    setenv("TZ", timeZone, 1);
    delay(100);
}

bool isTimeToWater() {
    time(&now);
    struct tm curr_time;
    localtime_r(&now, &curr_time);
    char strftime_buf[64];
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &curr_time);

    if (curr_time.tm_hour != watering_hour_am &&
        curr_time.tm_hour != watering_hour_pm) {
        log_i("don't water yet it's only %s", strftime_buf);
        return false;
    }
    log_i("it's time to water the plants");
    return true;
}

void setSleepTime() {
    time(&now);
    struct tm curr_time;
    localtime_r(&now, &curr_time);
    char strftime_buf[64];
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &curr_time);
    log_i("current time: %s", strftime_buf);
    double seconds = 0.f;

    if (curr_time.tm_hour >= watering_hour_am &&
        curr_time.tm_hour < watering_hour_pm) {
        // compute watering time in the evening
        struct tm evening;
        localtime_r(&now, &evening);
        evening.tm_hour = watering_hour_pm;
        evening.tm_min = 0;
        evening.tm_sec = 0;
        seconds = difftime(mktime(&evening), now);
        strftime(strftime_buf, sizeof(strftime_buf), "%c", &evening);
        log_i("Time to water in the evening: %s", strftime_buf);
    } else {
        struct tm morning;
        localtime_r(&now, &morning);
        morning.tm_hour = watering_hour_am;
        morning.tm_min = 0;
        morning.tm_sec = 0;
        morning.tm_mday = curr_time.tm_mday + 1;  // next day
        seconds = difftime(mktime(&morning), now);
        strftime(strftime_buf, sizeof(strftime_buf), "%c", &morning);
        log_i("Time to water in the morning: %s", strftime_buf);
    }

    sleepTimeS = (int)seconds;
    log_i("Set sleep time to %d [s]", sleepTimeS);
}

void printLocalTime() {
    time(&now);
    struct tm curr_time;
    localtime_r(&now, &curr_time);
    char strftime_buf[64];
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &curr_time);
    log_i("The current date/time in Zurich is: %s", strftime_buf);
}

void printLastWateringTime() {
    char strftime_buf[64];
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &last_watering_time);
    log_i("Last watering time is: %s",
          asctime_r(&last_watering_time, strftime_buf));
}

void setLastWateringTime() {
    time(&now);
    localtime_r(&now, &last_watering_time);
    char strftime_buf[64];
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &last_watering_time);
    log_i("Set last watering time to: %s", strftime_buf);
}

// Reconnect to the MQTT server
void reconnect() {
    // keep track of when we started our attemtp to get a wifi connection
    unsigned long startAttemptTime = millis();
    while (!client.connected() && millis() - startAttemptTime < wifi_timeout) {
        log_i("Attempting MQTT connection...");
        char id[mac.length() + 1];
        mac.toCharArray(id, mac.length() + 1);
        if (client.connect(id)) {
            log_i("connected");
        } else {
            log_i("failed, rc= %s try again in 5 seconds", client.state());
            delay(100);
        }
    }
}

void setup() {
    Serial.begin(115200);
    pinMode(Pin_pump, OUTPUT);

    log_i("Setup");
    // Wifi
    setup_wifi();                         // connect to wifi
    client.setServer(mqtt_server, 1883);  // connect wo MQTT server
    MAC_ADDRESS = WiFi.macAddress();      // get MAC address
    Serial.println(MAC_ADDRESS);
    mac = MAC_ADDRESS.substring(9, 11) + MAC_ADDRESS.substring(12, 14) +
          MAC_ADDRESS.substring(
              15,
              17);  // shorten MAC address to the last four digits (without ":")

    if (WiFi.status() != WL_CONNECTED) {
        reconnect();
    }
    // initialize time zone and get the time
    setTimeZone();
    printLocalTime();
    log_i("Setup done.");
}

void loop() {
    if (!client.connected() && WiFi.status() == WL_CONNECTED) {
        reconnect();
    }
    client.loop();
    log_i("Sensor location: %s", sensor_location);
    bootCount++;
    log_i("Boot count: %d", bootCount);
    print_wakeup_reason();
    printLastWateringTime();

    hum_raw = analogRead(Pin_humi_sensor);
    log_i("Raw Soil Sensor value: %d", hum_raw);
    // float soil_hum = (1024-moi)/1024.0*100; // as it is mapped to 0 to 1024,
    // like that we get the percentage
    float soil_hum =
        (4095 - hum_raw + hum_offset) / 4095.0 *
        100;  // as it is mapped to 0 to 4095, like that we get the percentage
    log_i("Moisture: %.1f %%", soil_hum);
    client.publish(
        (sensor_topic + "/Humidity").c_str(),
        ("SOIL_RH,site=" + sensor_location + " value=" + String(soil_hum))
            .c_str(),
        false);
    delay(100);

    if (isTimeToWater()) {
        if (soil_hum < hum_threshold) {
            digitalWrite(Pin_pump, HIGH);
            log_i("the soil is too dry. water plants now for %d s",
                  watering_time / 1000);
            delay(watering_time);
            digitalWrite(Pin_pump, LOW);
            setLastWateringTime();
            if (!client.connected() && WiFi.status() == WL_CONNECTED) {
                reconnect();
            }
            client.publish(
                (sensor_topic + "/Watered").c_str(),
                ("WATERED,site=" + sensor_location + " value=" + String(1))
                    .c_str(),
                false);
            log_i("Publish watered data");
            delay(500);

        } else {
            client.publish(
                (sensor_topic + "/Watered").c_str(),
                ("WATERED,site=" + sensor_location + " value=" + String(0))
                    .c_str(),
                false);
            log_i("MQTT Publish too humid");
            delay(300);
        }
    } else {
        client.publish(
            (sensor_topic + "/Watered").c_str(),
            ("WATERED,site=" + sensor_location + " value=" + String(0)).c_str(),
            false);
        log_i("MQTT Publish Woke up too early");
        delay(300);
    }

    setSleepTime();
    goToDeepSleep();
}
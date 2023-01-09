#include <Arduino.h>

#include "driver/adc.h"
#include "time.h"
#include <PubSubClient.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <esp_bt.h>
#include <esp_sleep.h>
#include <esp_wifi.h>


//OTA
#include <HTTPClient.h>
#include <ESP32httpUpdate.h>


// load Wifi credentials
#include "config.h"

/**********************************************************************************************/
const char* mqtt_server = MQTT_SERVER; //CHANGE MQTT Broker address
uint16_t mqtt_port = MQTT_PORT; // define MQTT port
String mqtt_user = MQTT_USER; //not used, make a change in the mqtt initializer
String mqtt_password = MQTT_PASSWORD; //not used, make a change in the mqtt initializer
String ssid = WIFI_SSID;             // CHANGE (WiFi Name)
String pwd = WIFI_PASSWD;            // CHANGE (WiFi password)
String sensor_location = LOCATION; // define sensor location

#define sw_version 10 // Define Software Version

// sensor settings
const int hum_threshold = 70; // In percentage, If soil humidity under
                              // hum_threshold it will trigger the watering
const int hum_offset = 1300;  // define humidity offset in raw ticks, measure by putting
                              // sensor into water and take this raw value as offset

// OTA management
const char* fwUrlBase = URL_to_Update_Server; // OTA URL
bool new_update_available = false;


// timezone management
const char* ntpServer = "pool.ntp.org";
const char* timeZone = "CET-1CEST,M3.5.0,M10.5.0/3";

// watering time
const int watering_hour_am = 7;       // 7 am
const int watering_hour_pm = 19;      // 7 pm
const int watering_time = 120 * 1000;  // in milli seconds

// *** Hardware Definitions ***
#define Pin_pump 33         // define esp pin for triggering pump
#define Pin_humi_sensor 35  // define analog pin to read from humidity sensor
#define Pin_voltage_divider 32  // define analog pin to read from voltage divider for battery voltage
// when wifi active only ADC1 available GPIO32 - GPIO39


uint64_t uS_TO_S_FACTOR = 1000000; //Conversion factor for micro seconds to seconds 
#define wifi_timeout 5000L  // 5 seconds in milliseconds

// *** persistent data ***
RTC_DATA_ATTR int bootCount = 0;  // Boot count should be stored in the non volatile RTC Memory
RTC_DATA_ATTR struct tm last_watering_time;  // Parameters see:
                         // http://www.cplusplus.com/reference/ctime/tm/
RTC_DATA_ATTR time_t now;

/***********************************************************************************************/

int hum_raw = 0;

String mac;
String MAC_ADDRESS;
WiFiClient espClient;
PubSubClient client(espClient);
String sensor_topic = "sensors/soil";
String device_type = "ESP32";
String device_name = "esp32_plantwatery";
int wifi_setup_timer = 0;


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
    log_i("IP address: %s", WiFi.localIP().toString());
}

void check_for_OTA(){
  new_update_available = false;  
  String fwURL = String( fwUrlBase );
  String fwVersionURL = fwURL;
  fwVersionURL.concat( "firmware.version" );

  log_i( "Checking for firmware updates." );
  log_i( "Firmware version URL: %s ", fwVersionURL);

  HTTPClient httpClient;
  httpClient.begin( fwVersionURL );
  int httpCode = httpClient.GET();
  if( httpCode == 200 ) {
    String newFWVersion = httpClient.getString();

    log_i( "Current firmware version: %d", sw_version );
    log_i( "Available firmware version: %s", newFWVersion );

    int newVersion = newFWVersion.toInt();

    if( newVersion > sw_version ) {
        new_update_available = true;
      log_i( "Preparing to update" );

      String fwImageURL = fwURL;
      fwImageURL.concat( "firmware.bin" );
      t_httpUpdate_return ret = ESPhttpUpdate.update( fwImageURL );

      switch(ret) {
        case HTTP_UPDATE_FAILED:
          log_i("HTTP_UPDATE_FAILD Error (%d): %s", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
          break;

        case HTTP_UPDATE_NO_UPDATES:
          log_i("HTTP_UPDATE_NO_UPDATES");
          break;

        case HTTP_UPDATE_OK:
          log_i("HTTP_UPDATE_OK");
          break;
      }
    }
    else {
      log_i( "Already on latest version" );
    }
  }
  else {
    log_i( "Firmware version check failed, got HTTP response code %d", httpCode );
  }
  httpClient.end();
}

void goToDeepSleep(uint64_t sleepTime_sec) {
    // testing
    // sleepTime_sec = 30;
    log_i("Going to sleep for %f seconds", (double)sleepTime_sec);
    client.disconnect();
    delay(300);
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    btStop();
    delay(300);
    esp_wifi_stop();
    esp_bt_controller_disable();

    // Configure the timer to wake us up!
    uint64_t sleeptime = sleepTime_sec * uS_TO_S_FACTOR; // needed to sleep long enough
    esp_err_t error = esp_sleep_enable_timer_wakeup(sleeptime);
    Serial.println(error);
    //Serial.flush();
    // give enough time to print everything to serial
    delay(300);
    // Go to sleep! Zzzz
    esp_deep_sleep_start();
    log_e("this should not happen");
}

void print_wakeup_reason() {
    esp_sleep_wakeup_cause_t wakeup_reason;

    wakeup_reason = esp_sleep_get_wakeup_cause();

    switch (wakeup_reason) {
        case 1:
            log_i("Not a wakeup cause, used to disable all wakeup sources");
            break;
        case 2:
            log_i("Wakeup caused by external signal using RTC_IO");
            break;
        case 3:
            log_i("Wakeup caused by external signal using RTC_CNTL");
            break;
        case 4:
            log_i("Wakeup caused by timer");
            break;
        case 5:
            log_i("Wakeup caused by touchpad");
            break;
        case 6:
            log_i("Wakeup caused by ULP program");
            break;
        case 7:
            log_i("Wakeup caused by GPIO");
            break;
        case 8:
            log_i("Wakeup caused by UART");
            break;
        default:
            log_i("Wakeup was not caused by deep sleep");
            break;
    }
}

void setTimeZone() {
    struct tm timeinfo;
    configTime(0, 0, ntpServer);
    if(!getLocalTime(&timeinfo)){
        log_e("Failed to obtain time");
    } else {
        log_i("Got the time from the NTP Server");
    }
    delay(100);
    setenv("TZ", timeZone, 1);
    delay(200);
    tzset();
}

bool isTimeToWater() {
    time(&now);
    struct tm curr_time;
    localtime_r(&now, &curr_time);
    char strftime_buf[64];
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &curr_time);

    if (curr_time.tm_mday == last_watering_time.tm_mday && 
                curr_time.tm_hour == last_watering_time.tm_hour){
      strftime(strftime_buf, sizeof(strftime_buf), "%c", &last_watering_time);
      log_i("Just watered at %s, go back to sleep", strftime_buf);
        return false;
    } else if (curr_time.tm_hour == watering_hour_am ||
        curr_time.tm_hour == watering_hour_pm) {
        log_i("It's time to water the plants");
        return true;
    } 
    log_i("Don't water yet it's only %s", strftime_buf);
    return false;
}

uint64_t setSleepTime() {
    uint64_t sleepTimeS = 43000;  // define standard as about 12h 
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
        log_i("%f seconds until next watering from now", seconds);
        strftime(strftime_buf, sizeof(strftime_buf), "%c", &evening);
        log_i("Time to water in the evening: %s", strftime_buf);
    } else if (curr_time.tm_hour >= 0 &&
        curr_time.tm_hour < watering_hour_am) {
        // compute watering time between midnight and morning of the same day
        struct tm early_morning;
        localtime_r(&now, &early_morning);
        early_morning.tm_hour = watering_hour_am;
        early_morning.tm_min = 0;
        early_morning.tm_sec = 0;
        seconds = difftime(mktime(&early_morning), now);
        log_i("%f seconds until next watering from now", seconds);
        strftime(strftime_buf, sizeof(strftime_buf), "%c", &early_morning);
        log_i("Time to water in the morning: %s", strftime_buf);
    } else {
        struct tm morning;
        localtime_r(&now, &morning);
        morning.tm_hour = watering_hour_am;
        morning.tm_min = 0;
        morning.tm_sec = 0;
        morning.tm_mday = curr_time.tm_mday + 1;  // next day
        seconds = difftime(mktime(&morning), now);
        log_i("%f seconds until next watering from now", seconds);
        strftime(strftime_buf, sizeof(strftime_buf), "%c", &morning);
        log_i("Time to water in the morning: %s", strftime_buf);
    }
 
    sleepTimeS = (uint64_t)seconds;
    //sleepTimeS = (uint64_t) 300; // to debug, wake up every 5min
    log_i("Set sleep time to %f [s]", (double)sleepTimeS);
    return sleepTimeS;
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
    // keep track of when we started our attempt to get a wifi connection
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

float measure_battery_level() {
    float batteryLevel = map(analogRead(Pin_voltage_divider), 0.0f, 4095.0f, 0, 100);
    log_i("Battery level measured is: %f", batteryLevel);
    return batteryLevel;
}

void setup() {
    delay(200); //Helps with common issue that ESP32 is not waking up after deep sleep 
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


    // One time Execution before deep sleep, therefore not in the loop
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
    float soil_hum = map(hum_raw, hum_offset, 4095.0f, 0, 100);
    log_i("Moisture: %.1f %%", soil_hum);
    client.publish(
        (sensor_topic + "/Humidity").c_str(),
        ("SOIL_RH,site=" + sensor_location + " value=" + String(soil_hum))
            .c_str(),
        false);
    client.publish(
        (sensor_topic + "/BatteryVoltage").c_str(),
        ("BatteryVoltage,site=" + sensor_location + " value=" + String(measure_battery_level())).c_str(),
        false);
    client.publish(
            (sensor_topic + "/SoftwareVS").c_str(),
            ("SoftwareVS,site=" + sensor_location + " value=" + String(sw_version)).c_str(),
            false);
    log_i("MQTT Published Soil RH, Battery Voltage and Software Version: %d", sw_version);
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
            log_i("MQTT Publish too humid, did not water plant");
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
    
    if (!client.connected() && WiFi.status() == WL_CONNECTED) {
        reconnect();
    }
    client.loop();
    check_for_OTA();
    goToDeepSleep(setSleepTime());
}

void loop() {
// not used due to deep sleep
}
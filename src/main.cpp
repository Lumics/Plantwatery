#include <Arduino.h>

#include <WiFi.h>
#include "time.h"
#include "driver/adc.h"
#include <esp_wifi.h>
#include <esp_bt.h>
#include <PubSubClient.h>
#include <WiFiClient.h>
// #include <Time.h>
// Update these with values suitable for your network.

// load Wifi credentials
#include "credentials.h"

/**********************************************************************************************/
#define mqtt_server "lumicsraspi4"    //CHANGE MQTT Broker address
#define mqtt_user "your_username"     //not used, make a change in the mqtt initializer
#define mqtt_password "your_password" //not used, make a change in the mqtt initializer

#define uS_TO_S_FACTOR 1000000 /* Conversion factor for micro seconds to seconds */
#define wifi_timeout 5000      // 5 seconds in milliseconds

String ssid = WIFI_SSID;            //CHANGE (WiFi Name)
String pwd = WIFI_PASSWD;           //CHANGE (WiFi password)
String sensor_location = "balcony"; // define sensor location

//const int sleepTimeS = 8ULL * 60 *60; // 8 hour
int sleepTimeS = 1 * 60;             // 1 min for debugging
const int hum_threshold = 75;        // In percentage, If soil humidity under hum_threshold it will trigger the watering
const int watering_time = 30 * 1000; // in mili seconds
const int hum_offset = 280;          // define humidity offset in raw ticks

// to get time
const char *ntpServer = "de.pool.ntp.org";
const long gmtOffset_sec = 2 * 3600; // GMT +2
const int daylightOffset_sec = 0;

// *** Hardware Definitions ***
#define Pin_motor 21       // define wemos pin for triggering pump
#define Pin_humi_sensor 32 // define analog pin to read from humidity sensor
// when wifi active only ADC1 available GPIO32 - GPIO39
/***********************************************************************************************/

RTC_DATA_ATTR int bootCount = 0; // Boot count should be stored in the non volatile RTC Memory
RTC_DATA_ATTR int am_pm = 0;     // current watering cycle, am: 0, pm: 1

int hum_raw = 0;

String mac;
String MAC_ADDRESS;
WiFiClient espClient;
PubSubClient client(espClient);
String sensor_topic = "sensors/soil/";
String device_type = "Arduino";
String device_name = "wemos_plantwatery";
int wifi_setup_timer = 0;
struct tm timeinfo; //Parameters see: http://www.cplusplus.com/reference/ctime/tm/
time_t now;

void setup_wifi()
{
  char ssid1[30];
  char pass1[30];
  ssid.toCharArray(ssid1, 30);
  pwd.toCharArray(pass1, 30);
  Serial.println();
  Serial.print("Connecting to: " + String(ssid1));
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid1, pass1);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(10);
    Serial.print(".");
    wifi_setup_timer++;
    if (wifi_setup_timer > 999)
    { //Restart the ESP if no connection can be established
      ESP.restart();
    }
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void goToDeepSleep()
{
  log_i("Going to sleep for %d seconds", sleepTimeS);
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  btStop();

  esp_wifi_stop();
  esp_bt_controller_disable();

  // Configure the timer to wake us up!
  esp_sleep_enable_timer_wakeup(sleepTimeS * 1000000L);

  // Go to sleep! Zzzz
  esp_deep_sleep_start();
}

void getSleepTime()
{
  time(&now);
  struct tm curr_time;
  localtime_r(&now, &curr_time);
  char strftime_buf[64];
  strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
  log_i("last watering time: %s", strftime_buf);
  double seconds = 0.f;
  if (timeinfo.tm_hour < 12)
  {
    // compute watering time in the evening
    struct tm evening;
    localtime_r(&now, &evening);
    evening.tm_hour = 20;
    evening.tm_min = 0;
    evening.tm_sec = 0;
    seconds = difftime(mktime(&evening), now);
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &evening);
    log_i("time to water in the evening: %s", strftime_buf);
  }
  else
  {
    struct tm morning;
    localtime_r(&now, &morning);
    morning.tm_hour = 8;
    morning.tm_min = 0;
    morning.tm_sec = 0;
    morning.tm_mday = timeinfo.tm_mday + 1;
    seconds = difftime(mktime(&morning), now);
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &morning);
    log_i("time to water in the morning: %s", strftime_buf);
  }

  log_i("seconds until next watering s %.f", seconds);
  sleepTimeS = (int)seconds;
}

// TODO find better way to get time
void printLocalTime()
{
  //Parameters see: http://www.cplusplus.com/reference/ctime/strftime/
  setenv("TZ", "CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00", 1);
  tzset();
  time(&now);
  localtime_r(&now, &timeinfo);
  char strftime_buf[64];
  strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
  log_i("The current date/time in Zurich is: %s", strftime_buf);
}

//Reconnect to the MQTT server
void reconnect()
{
  // keep track of when we started our attemtp to get a wifi connection
  unsigned long startAttemptTime = millis();
  while (!client.connected() && millis() - startAttemptTime < wifi_timeout)
  {
    log_i("Attempting MQTT connection...");
    char id[mac.length() + 1];
    mac.toCharArray(id, mac.length() + 1);
    if (client.connect(id))
    {
      log_i("connected");
    }
    else
    {
      log_i("failed, rc= %s try again in 5 seconds", client.state());
      delay(100);
    }
  }
}

void setup()
{
  Serial.begin(115200);
  pinMode(Pin_motor, OUTPUT);

  //Wifi
  setup_wifi();                        //connect to wifi
  client.setServer(mqtt_server, 1883); //connect wo MQTT server
  MAC_ADDRESS = WiFi.macAddress();     //get MAC address
  Serial.println(MAC_ADDRESS);
  mac = MAC_ADDRESS.substring(9, 11) + MAC_ADDRESS.substring(12, 14) + MAC_ADDRESS.substring(15, 17); //shorten MAC address to the last four digits (without ":")
  if (!getLocalTime(&timeinfo))
  {
    if (WiFi.status() != WL_CONNECTED)
    {
      reconnect();
    }
    //init and get the time
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    setenv("TZ", "CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00", 1);
    tzset();
    printLocalTime();
  }
}

void loop()
{
  if (!client.connected() && WiFi.status() == WL_CONNECTED)
  {
    reconnect();
  }
  client.loop();
  printLocalTime();
  hum_raw = analogRead(Pin_humi_sensor);
  log_i("Raw Soil Sensor value: %d", hum_raw);
  //float soil_hum = (1024-moi)/1024.0*100; // as it is mapped to 0 to 1024, like that we get the percentage
  float soil_hum = (1024 - hum_raw + hum_offset) / 1024.0 * 100; // as it is mapped to 0 to 1024, like that we get the percentage
  log_i("Moisture: %f %", soil_hum);
  log_i("Sensor location: %s", sensor_location);
  client.publish((sensor_topic + "/Humidity").c_str(), ("SOIL_RH,site=" + sensor_location + " value=" + String(soil_hum)).c_str(), false);
  delay(500);
  if (soil_hum < hum_threshold)
  {
    digitalWrite(Pin_motor, HIGH);
    log_i("watering plants now for %d s", watering_time / 1000);
    delay(watering_time);
    digitalWrite(Pin_motor, LOW);
  }
  bootCount++;
  log_i("Boot count: %d | RTC time is: ", bootCount);
  printLocalTime();
  getSleepTime();
  goToDeepSleep();
}
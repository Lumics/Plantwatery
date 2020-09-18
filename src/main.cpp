#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>

// Update these with values suitable for your network.

/**********************************************************************************************/
#define mqtt_server "lumicsraspi4" //CHANGE MQTT Broker address
#define mqtt_user "your_username" //not used, make a change in the mqtt initializer
#define mqtt_password "your_password" //not used, make a change in the mqtt initializer
String  ssid = "4M";      //CHANGE (WiFi Name)
String pwd = "wldW56@18";      //CHANGE (WiFi password)
const int sleepTimeS = 3*60*60; // 3 hour , note that max deep sleep the wemos can handle is about 3h 20min
//const int sleepTimeS = 1*60; // 1 min for debugging
const int hum_threshold = 75; // In percentage, If soil humidity under hum_threshold it will trigger the watering
const int watering_time = 60*1000; // in mili seconds
const int hum_offset = 280; // define humidity offset in raw ticks
#define Pin_motor D1 // define wemos pin for triggering pump
/***********************************************************************************************/

int hum_raw = 0;  

String mac;
String MAC_ADDRESS;
WiFiClient espClient;
PubSubClient client(espClient);
String sensor_topic = "sensors/soil/"; // v1/<user_id>/<measurement_session_id>
String device_type = "Arduino";
String device_name = "wemos_plantwatery";
int wifi_setup_timer = 0;

void setup_wifi() {
  char ssid1[30];
  char pass1[30];
  ssid.toCharArray(ssid1, 30);
  pwd.toCharArray(pass1, 30);
  Serial.println();
  Serial.print("Connecting to: " + String(ssid1));
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid1, pass1);
  while (WiFi.status() != WL_CONNECTED) {
    delay(10);
    Serial.print(".");
    wifi_setup_timer ++;
    if (wifi_setup_timer > 999) {                      //Restart the ESP if no connection can be established
      ESP.restart();                  
    }
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {                                                  //Reconnect to the MQTT server
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    char id[mac.length() + 1];
    mac.toCharArray(id, mac.length() + 1);
    if (client.connect(id)) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(100);
    }
  }
}



void setup()
{
  Serial.begin(115200);
  pinMode(Pin_motor, OUTPUT);

  //Wifi
  setup_wifi();                                      //connect to wifi
  client.setServer(mqtt_server, 1883);               //connect wo MQTT server
  MAC_ADDRESS = WiFi.macAddress();                    //get MAC address
  Serial.println(MAC_ADDRESS);
  mac =  MAC_ADDRESS.substring(9, 11) + MAC_ADDRESS.substring(12, 14) + MAC_ADDRESS.substring(15, 17);    //shorten MAC address to the last four digits (without ":")
 
}

void loop()
{
  if (!client.connected() && WiFi.status() == WL_CONNECTED) {
    reconnect();
  }
  client.loop();
  hum_raw = analogRead(0); 
  Serial.print("Raw Soil Sensor value: ");
  Serial.println(hum_raw);
  //float soil_hum = (1024-moi)/1024.0*100; // as it is mapped to 0 to 1024, like that we get the percentage 
  float soil_hum = (1024-hum_raw+hum_offset)/1024.0*100; // as it is mapped to 0 to 1024, like that we get the percentage 
  Serial.print("Moisture: ");
  Serial.println(soil_hum);  
  client.publish((sensor_topic + "/Hummidity").c_str(), ("SOIL_RH,site=balcony value=" + String(soil_hum)).c_str(), false);
  if(soil_hum<hum_threshold){
    digitalWrite(Pin_motor, HIGH);
    delay(watering_time);
    digitalWrite(Pin_motor, LOW);
  }
  Serial.print("Sleep mode");
  ESP.deepSleep(sleepTimeS * 1000000);
}
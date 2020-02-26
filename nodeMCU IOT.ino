#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include  <Ticker.h>//Ticker Library

// light pin
#define light D0

// DHT sensor config.
#define DHTPIN D1         // Digital pin connected to the DHT sensor 
#define DHTTYPE DHT22     // DHT 22 (AM2302)
DHT_Unified dht(DHTPIN, DHTTYPE);
uint32_t delayMS = 0;

// 設定無線基地台SSID跟密碼
const char* ssid     = "******";
const char* password = "******";
// 設定樹莓派 MQTT Broker 的 IP
const char* mqtt_server = "192.168.50.207";
// 初始化 espClient.
WiFiClient espClient;
PubSubClient client(espClient);

// process incoming subscribed messages and maintain its connection to the server.
Ticker subPoll;

void callback(String topic, byte* message, unsigned int length);  // 當設備發訊息給一個標題(topic)時，這段函式會被執行
void reconnect();                                                 // ESP8266 重新連接到 MQTT Broker 
void subPolling();                                                // process incoming subscribed messages and maintain its connection to the server.

void setup() {
  pinMode(light, OUTPUT);
  digitalWrite(light, HIGH);  // set HIGH to turn off Light
  
  Serial.begin(115200);
  
  dht.begin();
  sensor_t sensor;
  dht.temperature().getSensor(&sensor);   // Set delay between sensor readings based on sensor details.
  delayMS = sensor.min_delay / 1000;      // Delay between measurements.
  
  // 連接無線網路
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("WiFi connected - ESP IP address: ");
  Serial.println(WiFi.localIP());

  client.setServer(mqtt_server, 1883);    // 設定 mqtt server 及連接 port
  client.setCallback(callback);           // 設定 mqtt broker 並設定 callback function
  subPoll.attach(0.5, subPolling);      //Initialize subPoll every 0.5s
}

void loop() {
  if (!client.connected())
    reconnect();
  /*
  if(!client.loop())
    client.connect("ESP8266Client");
  */
  float temp, humi;
  sensors_event_t event;
  
  dht.temperature().getEvent(&event); // Get temperature event and print its value.
  if (isnan(event.temperature))
    Serial.println(F("Error reading temperature!"));
  else
    temp = event.temperature;

  dht.humidity().getEvent(&event);  // Get humidity event and print its value.
  if (isnan(event.relative_humidity))
    Serial.println(F("Error reading humidity!"));
  else
    humi = event.relative_humidity;

  // dtostrf 將 float 數字改成文字
  static char temperatureTemp[7];
  dtostrf(temp, 6, 2, temperatureTemp);
  static char humidityTemp[7];
  dtostrf(humi, 6, 2, humidityTemp);

  // Publishes Temperature and Humidity values
  client.publish("room/temperature", temperatureTemp);
  client.publish("room/humidity", humidityTemp);
  Serial.print("Humidity: ");
  Serial.print(humi);
  Serial.print(" %\t Temperature: ");
  Serial.print(temp);
  Serial.println(" *C ");
  // Delay between measurements.
  delay(delayMS);
} 

// 當設備發訊息給一個標題(topic)時，這段函式會被執行
void callback(String topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;
  
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();

  // 假使收到訊息給主題 room/light, 可以檢查訊息是 on 或 off. 根據訊息開啟 GPIO
  if(topic=="room/light"){
      Serial.print("Changing Room light to ");
      if(messageTemp == "ON"){
        digitalWrite(light, LOW);
        Serial.print("ON");
      }
      else if(messageTemp == "OFF"){
        digitalWrite(light, HIGH);
        Serial.print("OFF");
      }
  }
  Serial.println();
}

// ESP8266 重新連接到 MQTT Broker 
void reconnect() {
  // 持續迴圈直到連線
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");

    if (client.connect("ESP8266Client")) {
      Serial.println("connected");  
      // 訂閱一個主題，可以設定多個主題
      client.subscribe("room/light");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}
// process incoming subscribed messages and maintain its connection to the server.
void subPolling() {
  if(!client.loop())
    client.connect("ESP8266Client");
}
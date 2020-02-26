#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <Ticker.h>//Ticker Library
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSans12pt7b.h>

#define OLED_RESET D4
Adafruit_SSD1306 display(OLED_RESET);

#define XPOS 0
#define YPOS 1
#define DELTAY 2

#if (SSD1306_LCDHEIGHT != 32)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

//IP button (SD3)
#define IPbutton 10

// light pin
#define light D0

// DHT sensor config.
#define DHTPIN D3         // Digital pin connected to the DHT sensor 
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

// display fliper
uint8_t changer = 0;

const unsigned char PROGMEM comet [] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0xC0, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7F, 0xF0, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xFF, 0xFE, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xFF, 0xFF, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0F, 0xC0, 0x1F, 0xC0, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1F, 0xDF, 0x00, 0x07, 0xE0, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7F, 0xFF, 0x30, 0x02, 0xF0, 0x00,
  0x00, 0x1F, 0x01, 0xF0, 0x38, 0x38, 0x7F, 0xE3, 0xFF, 0xC0, 0xFF, 0xFF, 0xFE, 0x00, 0xF0, 0x00,
  0x00, 0xFF, 0x87, 0xFC, 0x3C, 0x78, 0x7F, 0xE7, 0xFF, 0x80, 0xFE, 0x7F, 0xFF, 0xC0, 0x78, 0x00,
  0x01, 0xFF, 0x8F, 0xFC, 0x7C, 0xF8, 0xFF, 0xC7, 0xFF, 0x80, 0x7F, 0xF0, 0xFF, 0xFF, 0x7C, 0x00,
  0x03, 0xE0, 0x1E, 0x1E, 0x7C, 0xF8, 0xE0, 0x00, 0x70, 0x00, 0x3F, 0xE0, 0x0F, 0xFF, 0xFC, 0x00,
  0x07, 0x80, 0x1C, 0x0E, 0x6D, 0xF8, 0xE0, 0x00, 0x70, 0x00, 0x0F, 0xE0, 0x00, 0x7F, 0xFF, 0xE0,
  0x07, 0x80, 0x38, 0x0E, 0x6D, 0xB8, 0xE0, 0x00, 0x70, 0x00, 0x03, 0xF0, 0x00, 0x07, 0xFF, 0xF8,
  0x07, 0x00, 0x38, 0x0E, 0xEF, 0x30, 0xFF, 0x80, 0x70, 0x00, 0x03, 0xF8, 0x00, 0x00, 0x1F, 0xFC,
  0x0F, 0x00, 0x38, 0x0E, 0xCF, 0x31, 0xFF, 0x80, 0xF0, 0x00, 0x03, 0xFC, 0x00, 0x00, 0x1F, 0xFE,
  0x0E, 0x00, 0x38, 0x1C, 0xCE, 0x31, 0xFF, 0x80, 0xE0, 0x00, 0x03, 0xF8, 0x00, 0x00, 0x1F, 0xFC,
  0x0E, 0x00, 0x70, 0x1C, 0xCE, 0x71, 0xC0, 0x00, 0xE0, 0x00, 0x03, 0xF0, 0x00, 0x00, 0xFF, 0xFC,
  0x0E, 0x00, 0x70, 0x3D, 0xC0, 0x71, 0xC0, 0x00, 0xE0, 0x00, 0x0F, 0xE0, 0x00, 0x3F, 0xFF, 0xE0,
  0x0F, 0x00, 0x78, 0x79, 0x80, 0x63, 0x80, 0x00, 0xE0, 0x00, 0x1F, 0xE0, 0x01, 0xFF, 0xFF, 0x00,
  0x07, 0xFE, 0x3F, 0xF1, 0x80, 0x63, 0xFE, 0x01, 0xC0, 0x00, 0x7F, 0xF0, 0xFF, 0xFF, 0xFC, 0x00,
  0x07, 0xFE, 0x3F, 0xE3, 0x80, 0x63, 0xFF, 0x01, 0xC0, 0x00, 0xFF, 0x7F, 0xFF, 0xFC, 0x78, 0x00,
  0x01, 0xFC, 0x0F, 0xC3, 0x80, 0xE3, 0xFF, 0x01, 0xC0, 0x00, 0xFE, 0xFF, 0xFE, 0x40, 0xF8, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xF0, 0x00, 0xF0, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3F, 0xFF, 0x00, 0x07, 0xE0, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x1F, 0xC0, 0x0F, 0xC0, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0xF8, 0x7F, 0x80, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xFF, 0xFE, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xF8, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1F, 0xC0, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

void callback(String topic, byte* message, unsigned int length);  // 當設備發訊息給一個標題(topic)時，這段函式會被執行
void reconnect();                                                 // ESP8266 重新連接到 MQTT Broker 
void subPolling();                                                // process incoming subscribed messages and maintain its connection to the server.

void setup() {
  // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 128x32)
  // init done
  display.clearDisplay();
  display.drawBitmap(0, 0,  comet, 128, 32, 1);
  display.display();
  
  pinMode(light, OUTPUT);
  digitalWrite(light, HIGH);  // set HIGH to turn off Light
  pinMode(IPbutton, INPUT_PULLUP);
  
  Serial.begin(115200);
  
  dht.begin();
  sensor_t sensor;
  dht.temperature().getSensor(&sensor);   // Set delay between sensor readings based on sensor details.
  delayMS = sensor.min_delay / 1000;      // Delay between measurements.
  
  delay(700);
  display.clearDisplay();
  
  // 連接無線網路
  Serial.println();
  Serial.print("Connecting to");
  String line1 = "Connecting to";
  display.setFont();
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.print(line1);
  display.display();
  
  Serial.println(ssid);
  String line2 = ssid;
  display.setFont(&FreeSans12pt7b);
  display.setCursor(0, 30);
  display.print(line2);
  display.display();
  display.setFont();  // set font back to default
  WiFi.begin(ssid, password);
  uint8_t dotCounter = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    line1 += ".";
    dotCounter++;
    if( dotCounter == 9 ){
      line1 = "Connecting to";
      dotCounter = 0;
      display.clearDisplay();
      display.setFont(&FreeSans12pt7b);
      display.setCursor(0, 30);
      display.print(line2);
      display.setFont();  // set font back to default
    }
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.print(line1);
    display.display();
  }
  Serial.println("");
  display.clearDisplay();
  
  Serial.print("WiFi connected - ESP IP address: ");
  line1 = "IP address: ";
  display.setCursor(0, 0);
  display.print(line1);
  display.display();
  
  Serial.println(WiFi.localIP());
  IPAddress locIP = WiFi.localIP();
  display.setCursor(0, 15);
  display.print(locIP);
  display.display();
  
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
  display.clearDisplay();
  
  String line1;
  String line2;
  display.setFont();  // set font back to default
  if( digitalRead(IPbutton) ){
    if( changer%2 ){
      line1 = "Humidity";
      line2 = humi;
      line2 += "%";
    }
    else{
      line1 = "Temperature";
      line2 = temp;
      line2 += "*C";
    }
    changer++;
    display.setFont();
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.print(line1);
  
    display.setFont(&FreeSans12pt7b);
  
    display.setCursor(45, 30);
    display.print(line2);
    display.display();
  }
  else{
    Serial.print("WiFi connected - ESP IP address: ");
    line1 = "IP address: ";
    display.setCursor(0, 0);
    display.print(line1);
    display.display();
    
    Serial.println(WiFi.localIP());
    IPAddress locIP = WiFi.localIP();
    display.setCursor(0, 15);
    display.print(locIP);
    display.display();
  }
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
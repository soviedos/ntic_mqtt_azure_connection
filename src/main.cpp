
#include <SPI.h>
#include <Arduino.h>
#include <ArduinoMqttClient.h>
#include <ArduinoJson.h>
#include <WiFi101.h> // change to #include <WiFi101.h> for MKR1000
#include <Adafruit_GFX.h> // Graphics core library
#include <Adafruit_SSD1306.h> // Library for Monochrome OLEDs display
#include <DHT.h>
#include <DHT_U.h>
#include "credentials.h"

/////// Enter your sensitive data in credentials.h
const char ssid[]        = SECRET_SSID;
const char pass[]        = SECRET_PASS;
const char broker[]      = SECRET_BROKER;
String     deviceId      = SECRET_DEVICE_ID;
String     password      = AZURE_PASS;
String     api_version   = "/?api-version=2018-06-30";
int        port          = 8883;

// WiFi credentials
WiFiSSLClient   wifiClient;            // Used for the TCP socket connection
MqttClient      mqttClient(wifiClient);

//WiFi parameters
int status = WL_IDLE_STATUS;
long rssi;

// Configuring network parameters
IPAddress ip(192,168,0,5);
IPAddress gateway(192,168,0,1);
IPAddress subnet(255,255,255,0);
IPAddress dns(8,8,8,8);

// Display instance
Adafruit_SSD1306 display = Adafruit_SSD1306(128, 32, &Wire);

// Oled FeatherWings buttons map
#define BUTTON_A  9
#define BUTTON_B  6
#define BUTTON_C  5

// Sensor parameters
#define DHTPIN 6     // Digital pin connected to the DHT sensor 
#define DHTTYPE    DHT11     // DHT 11

// Sensor instance
DHT_Unified dht(DHTPIN, DHTTYPE);

// Configuring battery measuring pin
#define VBATPIN A7

unsigned long lastMillis = 0;

unsigned long getTime() {
  // get the current time from the WiFi module
  return WiFi.getTime();
}

void displaySetUp (int fontSize, int cursoX, int cursorY) {
  display.clearDisplay();
  display.display();
  display.setTextSize(fontSize);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(cursoX, cursorY);
}

void displayInit() {
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C); // Address 0x3C for 128x32
  display.display();
  delay(2000);
}

/////////////// Sub Routine to measure battery voltage ///////////////
void batteryMeasure() {
    float measuredvbat = analogRead(VBATPIN);
    measuredvbat *= 2;    // we divided by 2, so multiply back
    measuredvbat *= 3.3;  // Multiply by 3.3V, our reference voltage
    measuredvbat /= 1024; // convert to voltage
    displaySetUp(1, 0, 0);
    display.print("VBat: "); display.println(measuredvbat);
    display.display();
}

/////////////// Sub Routine to print WiFi data ///////////////
void printWiFiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");

  // Display values
  displaySetUp(1, 0, 0);
  display.print("WiFi connected ");
  display.println(ssid);
  display.print("Ip: ");
  ip = WiFi.localIP();
  display.println(ip);
  display.print("Gateway: ");
  gateway = WiFi.gatewayIP();
  display.println(gateway);
  rssi = WiFi.RSSI();
  display.print("Signal: ");
  display.print(rssi);
  display.println(" dBm");
  display.display();
  delay(2000);
}

/////////////// Sub Routine for the batteryMeasure and printWiFiStatus interruption ///////////////
void interruption() {
  if (!digitalRead(BUTTON_B)) printWiFiStatus();
  if (!digitalRead(BUTTON_C)) batteryMeasure();
  delay(2000);
}

/////////////// Sub Routine for initialization of DTH 11 sensor ///////////////
void initSensor() {
  dht.begin();
}

/////////////// Sub Routine for temperature reading ///////////////
float readTemperature() {

  float temp;
  interruption();
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.display();
  sensor_t sensor;
  dht.temperature().getSensor(&sensor);
  sensors_event_t event;
  dht.temperature().getEvent(&event);
  temp = event.temperature;

  if (isnan(event.temperature)) {
      Serial.println(F("Error reading temperature!"));
    }
    else {
      display.print("Temperature: ");
      display.print(event.temperature);
      display.println(" C");
      display.display();
      delay(2000);
    }

    return temp;
}

/////////////// Sub Routine for humidity reading ///////////////
void readHumidity()
{
  interruption();
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.display();
  sensor_t sensor;
  dht.humidity().getSensor(&sensor);
  sensors_event_t event;
  dht.humidity().getEvent(&event);
  if (isnan(event.relative_humidity)) {
      Serial.println(F("Error reading humidity"));
    }
    else {
      display.print("Humidity: ");
      display.print(event.relative_humidity);
      display.println(" %");
      display.display();
      delay(2000);
    }
}

void connectWiFi() {
  displaySetUp(1, 0, 0);
  display.print("Connecting to ");
  display.println(ssid);
  display.display();

  WiFi.config(ip, dns,gateway, subnet);

  while (WiFi.begin(ssid, pass) != WL_CONNECTED) {
    // failed, retry
    Serial.println("Connecting...");
    delay(1000);
  }
  printWiFiStatus();

  Serial.print("You're connected to the network: ");
  Serial.println(ssid);
}

////////////////////////////// Sub Routine to send data to Azure IoT Hub /////////////////////////////
void connectMQTT() {

  displaySetUp(1, 0, 0);
  display.println("Connecting to MQTT Server: ");
  display.println(broker);
  display.display();

  while (!mqttClient.connect(broker, port)) {
    Serial.print("Connecting Mqtt broker...");
    Serial.println(mqttClient.connectError());
    delay(2000);
  }

  displaySetUp(1, 0, 0);
  display.println("You're connected to: ");
  display.println(broker);
  display.display();

}

void publishMessage() {

  Serial.println("Publishing message");

  // send message, the Print interface can be used to set the message contents
  mqttClient.beginMessage("devices/" + deviceId + "/messages/events/");
  //mqttClient.print("Temperature: ");
  mqttClient.print(readTemperature());
  mqttClient.print(millis());
  mqttClient.endMessage();
}

void setup() {

  WiFi.setPins(8,7,4,2);

  Serial.begin(115200);

  // Initialization for DHT_11 sensor
  initSensor();
  
  // Display Initialization
  displayInit();

  // Board's buttons pinout
  pinMode(BUTTON_A, INPUT_PULLUP);
  pinMode(BUTTON_B, INPUT_PULLUP);
  pinMode(BUTTON_C, INPUT_PULLUP);

  // WiFi Init
  connectWiFi();

  mqttClient.setId(deviceId);

  // Set the username to "<broker>/<device id>/?api-version=2018-06-30"
  String username;

  username += broker;
  username += "/";
  username += deviceId;
  username += api_version;

  mqttClient.setUsernamePassword(username, AZURE_PASS);

  if (!mqttClient.connected()) {
    // MQTT client is disconnected, connect
    connectMQTT();
  }
}

void loop() {
  
  // poll for new MQTT messages and send keep alives
  mqttClient.poll();

  // publish a message roughly every 5 seconds.
  if (millis() - lastMillis > 5000) {
    lastMillis = millis();

    publishMessage();
  }
}

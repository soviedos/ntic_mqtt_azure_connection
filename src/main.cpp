
#include <Arduino.h>
#include <ArduinoMqttClient.h>
#include <WiFi101.h> // change to #include <WiFi101.h> for MKR1000
#include "credentials.h"

/////// Enter your sensitive data in arduino_secrets.h
const char ssid[]        = SECRET_SSID;
const char pass[]        = SECRET_PASS;
const char broker[]      = SECRET_BROKER;
String     deviceId      = SECRET_DEVICE_ID;
String     password      = AZURE_PASS;
String     api_version   = "/?api-version=2018-06-30";
int        port          = 8883;

WiFiSSLClient    wifiClient;            // Used for the TCP socket connection
MqttClient    mqttClient(wifiClient);

// Configuring network parameters
IPAddress ip(192,168,0,5);
IPAddress gateway(192,168,0,1);
IPAddress subnet(255,255,255,0);
IPAddress dns(8,8,8,8);

unsigned long lastMillis = 0;

void setup() {
  WiFi.setPins(8,7,4,2);
  Serial.begin(9600);
  while (!Serial);

  mqttClient.setId(deviceId);

  // Set the username to "<broker>/<device id>/?api-version=2018-06-30"
  String username;

  username += broker;
  username += "/";
  username += deviceId;
  username += api_version;

  mqttClient.setUsernamePassword(username, AZURE_PASS);

  // Set the message callback, this function is
  // called when the MQTTClient receives a message
  mqttClient.onMessage(onMessageReceived);
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }

  if (!mqttClient.connected()) {
    // MQTT client is disconnected, connect
    connectMQTT();
  }

  // poll for new MQTT messages and send keep alives
  mqttClient.poll();

  // publish a message roughly every 5 seconds.
  if (millis() - lastMillis > 5000) {
    lastMillis = millis();

    publishMessage();
  }
}

unsigned long getTime() {
  // get the current time from the WiFi module
  return WiFi.getTime();
}

void connectWiFi() {
  WiFi.config(ip, dns,gateway, subnet);
  Serial.print("Attempting to connect to SSID: ");
  Serial.print(ssid);
  Serial.print(" ");

  while (WiFi.begin(ssid, pass) != WL_CONNECTED) {
    // failed, retry
    Serial.print(".");
    delay(1000);
  }
  Serial.println();

  Serial.println("You're connected to the network");
  Serial.println();
}

void connectMQTT() {
  Serial.print("Attempting to MQTT broker: ");
  Serial.print(broker);
  Serial.println(" ");

  while (!mqttClient.connect(broker, port)) {
    // failed, retry
    Serial.print(".");
    Serial.println(mqttClient.connectError());
    delay(5000);
  }
  Serial.println();

  Serial.println("You're connected to the MQTT broker");
  Serial.println();

  // subscribe to a topic
  mqttClient.subscribe("devices/" + deviceId + "/messages/devicebound/#");
}

void publishMessage() {
  Serial.println("Publishing message");

  // send message, the Print interface can be used to set the message contents
  mqttClient.beginMessage("devices/" + deviceId + "/messages/events/");
  mqttClient.print("hello ");
  mqttClient.print(millis());
  mqttClient.endMessage();
}

void onMessageReceived(int messageSize) {
  // we received a message, print out the topic and contents
  Serial.print("Received a message with topic '");
  Serial.print(mqttClient.messageTopic());
  Serial.print("', length ");
  Serial.print(messageSize);
  Serial.println(" bytes:");

  // use the Stream interface to print the contents
  while (mqttClient.available()) {
    Serial.print((char)mqttClient.read());
  }
  Serial.println();

  Serial.println();
}
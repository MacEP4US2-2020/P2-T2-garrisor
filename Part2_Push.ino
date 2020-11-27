#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <iostream>
#include <chrono>
#include <ctime> 
#include "NewPing.h" 
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

//Define pins for ultrasonic range finder
#define trigPin 15
#define echoPin 2

//Initialize ultrasonic range finder
NewPing sonar(trigPin, echoPin, 500);

//Inialize bluetooth stuff
int scanTime = 5; //In seconds
BLEScan* pBLEScan;
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {       
      Serial.printf("Name: %s \n", advertisedDevice.getName().c_str()); //print device name - this is normally empty
      Serial.printf("MAC Address: %s \n", advertisedDevice.getAddress().toString().c_str()); // print MAC address 
      Serial.printf("RSSI: %d \n", advertisedDevice.getRSSI()); // print RSSI 
      client.publish("esp32/name", advertisedDevice.getName().c_str()); //publish to mqtt broker
      client.publish("esp32/RSSI", advertisedDevice.getRSSI()); //publish to mqtt broker
      client.publish("esp32/MACID", dvertisedDevice.getAddress().toString().c_str()); //publish to mqtt broker
    }
};

// Replace the next variables with your SSID/Password combination
const char* ssid = "92 Westwood";
const char* password = "mariocart";

// Add your MQTT Broker IP address, example:
const char* mqtt_server = "192.168.1.122";

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;

void setup() {
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  pinMode(0,OUTPUT); //IO 0 Output

  
  Serial.println("Scanning...");
  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan(); //create new scan
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);  // less or equal setInterval value
}

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;
  
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();

  // Feel free to add more if statements to control more GPIOs with MQTT

  // If a message is received on the topic esp32/output, you check if the message is either "on" or "off". 
  // Changes the output state according to the message
  if (String(topic) == "esp32/output") {
    Serial.print("Changing output to ");
    if(messageTemp == "on"){
      Serial.println("on");
      //digitalWrite(ledPin, HIGH);
    }
    else if(messageTemp == "off"){
      Serial.println("off");
      //digitalWrite(ledPin, LOW);
    }
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("Ronan's ESP32")) {
      Serial.println("connected");
      // Subscribe
      client.subscribe("esp32/output");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  long now = millis();
  if (now - lastMsg > 5000) {
    lastMsg = now;

    //Publish range to MQTT broker
    float distance = sonar.ping_cm(500); //ping for distance in cm
    Serial.print(distance); //print to serial monitor
    char distanceString[8]; // Convert the value to a char array
    dtostrf(distance, 1, 2, distanceString); //not sure what this does
    client.publish("esp32/distance", distanceString); //publish to mqtt broker

    //BLUETOOTH SECTION
    // put your main code here, to run repeatedly:
    BLEScanResults foundDevices = pBLEScan->start(scanTime, false);
    //Serial.print("Devices found: ");
    //Serial.println(foundDevices.getCount());
    Serial.println("Scan done!");
    pBLEScan->clearResults();   // delete results fromBLEScan buffer to release memory
    delay(2000);
  }
}

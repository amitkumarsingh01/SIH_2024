#include <WiFi.h>
#include <HTTPClient.h>
#include <LoRa.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>
#include <math.h>
#include "HardwareSerial.h"
#include "DFRobotDFPlayerMini.h"
#include <ArduinoJson.h>


// Pin definitions for Arduino Mega (LoRa)
#define LORA_NSS 5
#define LORA_RST 2
#define LORA_DIO0 -1

// Pin definitions for ESP32-WROOM (TFT Display)
#define TFT_DC 12
#define TFT_CS 13
#define TFT_MOSI 14
#define TFT_CLK 27
#define TFT_RST 0
#define TFT_MISO 0

// WiFi credentials
const char *ssid = "Project";
const char *password = "12345678";
const char *server = "https://tv35gx0m-8000.inc1.devtunnels.ms/api/core/sensors/";
const String apiKey = "pk-nvs-=5458b82d18714e9faf6a23df864c0291=";

// Push button pin definition
#define BUTTON_PIN 15

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, TFT_RST);

// Meter dimensions
#define METER_RADIUS  30
#define METER_CENTER_X 80
#define METER_CENTER_Y 70
#define VALUE_Y_OFFSET 105

int sensor1Value = 0;
int sensor2Value = 0;
int sensor3Value = 0;
int currentSensor = 1;
bool buttonPressed = false;

HardwareSerial myHardwareSerial(2);
DFRobotDFPlayerMini myDFPlayer;

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // Initialize WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("Connected to WiFi");

  // Initialize LoRa
  LoRa.setPins(LORA_NSS, LORA_RST, LORA_DIO0);
  if (!LoRa.begin(433E6)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }
  Serial.println("LoRa Receiver Initialized");

  // Initialize the display
  tft.initR(INITR_BLACKTAB);
  tft.setRotation(3);
  tft.fillScreen(ST7735_BLACK);
  tft.setTextColor(ST7735_RED);
  tft.setTextSize(1);
  tft.setCursor(0, 0);
  tft.println("LoRa Receiver");

  // Initialize DFPlayer Mini module
  myHardwareSerial.begin(9600, SERIAL_8N1, 16, 17);
  Serial.println();
  Serial.println(F("DFRobot DFPlayer Mini"));
  Serial.println(F("Initializing DFPlayer module ... Wait!"));

  if (!myDFPlayer.begin(myHardwareSerial)) {
    Serial.println(F("Not initialized:"));
    Serial.println(F("1. Check the DFPlayer Mini connections"));
    Serial.println(F("2. Insert an SD card"));
    while (true);
  }

  Serial.println(F("DFPlayer Mini module initialized!"));
  myDFPlayer.volume(30);
}

void loop() {
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    String packet = "";
    while (LoRa.available()) {
      packet += (char)LoRa.read();
    }

    Serial.print("Received packet: ");
    Serial.println(packet);

    if (packet.startsWith("Sensor1:")) {
      sensor1Value = packet.substring(packet.indexOf("Sensor1:") + 8, packet.indexOf(';')).toInt();
    }
    if (packet.indexOf("Sensor2:") != -1) {
      sensor2Value = packet.substring(packet.indexOf("Sensor2:") + 8, packet.indexOf(';', packet.indexOf("Sensor2:"))).toInt();
    }
    if (packet.indexOf("Sensor3:") != -1) {
      sensor3Value = packet.substring(packet.indexOf("Sensor3:") + 8).toInt();
    }

    displaySensorValue();
    sendDataToServer();  // Send data to the backend server
  }

  if (digitalRead(BUTTON_PIN) == LOW) {
    if (!buttonPressed) {
      currentSensor = (currentSensor % 3) + 1;
      buttonPressed = true;
      displaySensorValue();
    }
  } else {
    buttonPressed = false;
  }
}

void displaySensorValue() {
  tft.fillScreen(ST7735_BLACK);

  if (currentSensor == 1) {
    drawRoundMeter(sensor1Value, "Sensor1");
    if (sensor1Value < 50) {
      myDFPlayer.play(1);
    }
  } else if (currentSensor == 2) {
    drawRoundMeter(sensor2Value, "Sensor2");
    if (sensor2Value < 50) {
      myDFPlayer.play(2);
    }
  } else if (currentSensor == 3) {
    drawRoundMeter(sensor3Value, "Sensor3");
    if (sensor3Value < 50) {
      myDFPlayer.play(3);
    }
  }
}

void drawRoundMeter(int value, String sensorName) {
  tft.drawCircle(METER_CENTER_X, METER_CENTER_Y, METER_RADIUS, ST7735_WHITE);
  tft.drawCircle(METER_CENTER_X, METER_CENTER_Y, METER_RADIUS + 1, ST7735_WHITE);

  int startAngle = -90;
  int endAngle = map(value, 0, 100, 0, 180);

  for (int angle = startAngle; angle <= endAngle; angle++) {
    float angleRad = radians(angle);
    int x = METER_CENTER_X + METER_RADIUS * cos(angleRad);
    int y = METER_CENTER_Y + METER_RADIUS * sin(angleRad);
    tft.drawLine(METER_CENTER_X, METER_CENTER_Y, x, y, ST7735_GREEN);
  }

  tft.setCursor(0, 0);
  tft.setTextColor(ST7735_YELLOW);
  tft.setTextSize(2);
  tft.print(sensorName);

  tft.setCursor(0, VALUE_Y_OFFSET);
  tft.setTextColor(ST7735_WHITE);
  tft.setTextSize(2);
  tft.print("Value: ");
  tft.print(value);
  tft.print("%");
}

void sendDataToServer() {
  if (WiFi.status() == WL_CONNECTED) {

    HTTPClient http;
    http.begin(server);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", "DeviceToken " + apiKey);

    StaticJsonDocument<256> doc;
    doc["temperature"] = "0";
    doc["humidity"] = "0";
    JsonObject soilMoisture = doc.createNestedObject("soil_moisture");
    soilMoisture["s1"] = sensor1Value;
    soilMoisture["s2"] = sensor2Value;
    soilMoisture["s3"] = sensor3Value;
    doc["nitrogen"] = "0";
    doc["phosphorus"] = "0";
    doc["potassium"] = "0";
    //doc["ph"] = "0";

    String requestBody;
    serializeJson(doc, requestBody);

    int httpResponseCode = http.POST(requestBody);

    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println("Response code: " + String(httpResponseCode));
      Serial.println("Response: " + response);
    } else {
      Serial.println("Error on sending POST: " + String(httpResponseCode));
    }

    http.end();
  } else {
    Serial.println("WiFi Disconnected");
  }
}

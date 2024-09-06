#include <WiFi.h>
#include <esp_now.h>

// Structure to receive data
typedef struct struct_message {
  int moisture;
} struct_message;

struct_message incomingData;

// Callback when data is received (new signature to match the ESP-NOW API)
void onDataRecv(const esp_now_recv_info *info, const uint8_t *incomingDataBytes, int len) {
  // Copy the received bytes into the struct
  memcpy(&incomingData, incomingDataBytes, sizeof(incomingData));

  // Print the received soil moisture value to Serial Monitor
  Serial.print("Received Soil Moisture Value: ");
  Serial.println(incomingData.moisture);
}

void setup() {
  Serial.begin(115200);

  // Initialize Wi-Fi in station mode
  WiFi.mode(WIFI_STA);

  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Register receive callback (new callback signature)
  esp_now_register_recv_cb(onDataRecv);
}

void loop() {
  Serial.print("Received Soil Moisture Value: ");
  Serial.println(incomingData.moisture);
}

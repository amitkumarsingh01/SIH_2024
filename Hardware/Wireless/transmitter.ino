#include <WiFi.h>
#include <esp_now.h>

int soilMoisturePin = 33;  // Pin connected to the soil moisture sensor

// MAC address of the receiver
uint8_t receiverAddress[] = {0xD0, 0xEF, 0x76, 0x48, 0x0F, 0x34};

typedef struct struct_message {
  int moisture;
} struct_message;

struct_message myData;

// Callback when data is sent
void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("Last Packet Send Status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
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

  // Register send callback
  esp_now_register_send_cb(onDataSent);

  // Add peer (receiver)
  esp_now_peer_info_t peerInfo;
  memcpy(peerInfo.peer_addr, receiverAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }
}

void loop() {
  int moistureValue = analogRead(soilMoisturePin);
  myData.moisture = moistureValue;

  Serial.print("Soil Moisture Value: ");
  Serial.println(moistureValue);


  esp_now_send(receiverAddress, (uint8_t *) &myData, sizeof(myData));

  delay(2000);
}

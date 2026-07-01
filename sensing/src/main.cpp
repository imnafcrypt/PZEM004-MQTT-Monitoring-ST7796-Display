
#define RELAY_PIN 23

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <PZEM004Tv30.h>

// --- Configuration ---
const char* ssid = "ssid";
const char* password = "12345678";

const char* mqtt_server = "xbcf2a20.ala.asia-southeast1.emqxsl.com";
const int mqtt_port = 8883; 
const char* mqtt_user = "paijokompa";
const char* mqtt_pass = "paijokompa";
const char* pubTopic = "esp32/sensors/data";
const char* subTopic = "esp32/commands";

// --- PZEM-004T Setup ---
// Using ESP32 Hardware Serial2 (RX = GPIO 16, TX = GPIO 17)
#define PZEM_RX_PIN 16
#define PZEM_TX_PIN 17
PZEM004Tv30 pzem(Serial2, PZEM_RX_PIN, PZEM_TX_PIN);

WiFiClientSecure espClient;
PubSubClient client(espClient);

// Task Handles
TaskHandle_t NetworkTaskHandle;
TaskHandle_t SensorTaskHandle;

// Callback function (Runs inside the Network Task)
// Callback function (Runs when a message is received)
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] Payload: ");

  // 1. Convert the payload into a String
  String message = "";
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.println(message);

  // 2. Check if the message is on our command topic
  if (String(topic) == subTopic) {
    
    // Clean up the string (removes accidental spaces or newlines)
    message.trim();
    // Convert to uppercase so "on", "On", and "ON" all work
    message.toUpperCase(); 

    // 3. Toggle the relay
    if (message == "ON") {
      digitalWrite(RELAY_PIN, HIGH);
      Serial.println("Action: RELAY TURNED ON");
    } 
    else if (message == "OFF") {
      digitalWrite(RELAY_PIN, LOW);
      Serial.println("Action: RELAY TURNED OFF");
    }
    else {
      Serial.println("Action: Unknown command. Send 'ON' or 'OFF'.");
    }
  }

}
void NetworkTask(void *pvParameters) {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    vTaskDelay(500 / portTICK_PERIOD_MS); 
    Serial.print(".");
  }
  Serial.println("Wi-Fi Connected!");

  espClient.setInsecure();
  espClient.setTimeout(15);
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  for(;;) {
    if (!client.connected() && WiFi.status() == WL_CONNECTED) {
      String clientId = "ESP32-" + String(random(0xffff), HEX);
      if (client.connect(clientId.c_str(), mqtt_user, mqtt_pass)) {
        Serial.println("EMQX Connected!");
        client.subscribe(subTopic);
      } else {
        vTaskDelay(5000 / portTICK_PERIOD_MS); 
      }
    }
    
    if (client.connected()) {
      client.loop();
    }

    vTaskDelay(10 / portTICK_PERIOD_MS); 
  }
}

void SensorTask(void *pvParameters) {
  for(;;) {
    if (client.connected()) {
      // Read values from PZEM
      float voltage = pzem.voltage();
      float current = pzem.current();
      float power = pzem.power();
      float energy = pzem.energy();
      float frequency = pzem.frequency();
      float pf = pzem.pf();

      // If voltage is NaN, the sensor is not communicating properly
      if (isnan(voltage)) {
        Serial.println("Error reading PZEM-004T sensor! Check wiring.");
      } else {
        // Construct JSON payload
        String payload = "{";
        payload += "\"voltage\":" + String(voltage, 1) + ",";
        payload += "\"current\":" + String(current, 3) + ",";
        payload += "\"power\":" + String(power, 1) + ",";
        payload += "\"energy\":" + String(energy, 3) + ",";
        payload += "\"frequency\":" + String(frequency, 1) + ",";
        payload += "\"pf\":" + String(pf, 2);
        payload += "}";
        
        if (client.publish(pubTopic, payload.c_str())) {
          Serial.println("Published: " + payload);
        }
      }
    }
    vTaskDelay(10000 / portTICK_PERIOD_MS); // Read every 10 seconds
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW); // Start with the relay turned OFF

  xTaskCreatePinnedToCore(NetworkTask, "NetworkTask", 8192, NULL, 1, &NetworkTaskHandle, 1);
  xTaskCreatePinnedToCore(SensorTask,"SensorTask", 4096, NULL, 1, &SensorTaskHandle, 1);
}

void loop() {
  // Handled by FreeRTOS
}
#define REFRESH_DELAY 1 //In Seconds
#define RELAY_PIN 23

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <PZEM004Tv30.h>

// --- Configuration ---
const char* ssid = "<Network SSID>";
const char* password = "<Network Password>";

const char* mqtt_server = "<MQTT_Server>"; 
const int mqtt_port = 8883;                 //MQTT_PORT
const char* mqtt_user = "<MQTT_Username>";
const char* mqtt_pass = "<MQTT_Password>";
const char* pubTopic = "esp32/sensors";     // We listen to the PZEM here
const char* subTopic = "esp32/commands";   // We send Relay commands here

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

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] Payload: ");

  String message = "";
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.println(message);

  if (String(topic) == subTopic) {
    message.trim();
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

      if (isnan(voltage)) {
        Serial.println("Error reading PZEM-004T sensor! Check wiring.");
      } else {
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
    vTaskDelay(REFRESH_DELAY*1000 / portTICK_PERIOD_MS); 
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW); 

  xTaskCreatePinnedToCore(NetworkTask, "NetworkTask", 8192, NULL, 1, &NetworkTaskHandle, 1);
  xTaskCreatePinnedToCore(SensorTask,"SensorTask", 4096, NULL, 1, &SensorTaskHandle, 1);
}

void loop() {}
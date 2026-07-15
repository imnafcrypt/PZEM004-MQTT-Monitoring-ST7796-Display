#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <lvgl.h>
#include "ui/screens.h"
#include "ui/ui.h"
#include "ui/actions.h"
#include <WiFi.h>
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include "ui/vars.h"

// --- Configuration ---
const char* ssid = "<Network SSID>";
const char* password = "<Network Password>";

const char* mqtt_server = "<MQTT_Server>"; 
const int mqtt_port = 8883;                 //MQTT_PORT
const char* mqtt_user = "<MQTT_Username>";
const char* mqtt_pass = "<MQTT_Password>";
const char* pubTopic = "esp32/sensors";     // We listen to the PZEM here
const char* subTopic = "esp32/commands";   // We send Relay commands here

bool relay = false;

char volt[100] = { 0 };
const char *get_var_volt() {
    return volt;
}
void set_var_volt(const char *value) {
    strncpy(volt, value, sizeof(volt) / sizeof(char));
    volt[sizeof(volt) / sizeof(char) - 1] = 0;
}

char ampere[100] = { 0 };
const char *get_var_ampere() {
    return ampere;
}
void set_var_ampere(const char *value) {
    strncpy(ampere, value, sizeof(ampere) / sizeof(char));
    ampere[sizeof(ampere) / sizeof(char) - 1] = 0;
}

char watt[100] = { 0 };
const char *get_var_watt() {
    return watt;
}
void set_var_watt(const char *value) {
    strncpy(watt, value, sizeof(watt) / sizeof(char));
    watt[sizeof(watt) / sizeof(char) - 1] = 0;
}

char watthour[100] = { 0 };
const char *get_var_watthour() {
    return watthour;
}
void set_var_watthour(const char *value) {
    strncpy(watthour, value, sizeof(watthour) / sizeof(char));
    watthour[sizeof(watthour) / sizeof(char) - 1] = 0;
}

char hertz[100] = { 0 };
const char *get_var_hertz() {
    return hertz;
}
void set_var_hertz(const char *value) {
    strncpy(hertz, value, sizeof(hertz) / sizeof(char));
    hertz[sizeof(hertz) / sizeof(char) - 1] = 0;
}

char powerfactor[100] = { 0 };
const char *get_var_powerfactor() {
    return powerfactor;
}
void set_var_powerfactor(const char *value) {
    strncpy(powerfactor, value, sizeof(powerfactor) / sizeof(char));
    powerfactor[sizeof(powerfactor) / sizeof(char) - 1] = 0;
}


bool network;
bool get_var_network() {
    return network;
}
void set_var_network(bool value) {
    network = value;
}


#define XPT2046_CS  21
#define XPT2046_IRQ -1

#define TOUCH_X_MIN  200
#define TOUCH_X_MAX  3800
#define TOUCH_Y_MIN  200
#define TOUCH_Y_MAX  3800

static const uint16_t SCREEN_W = 480;
static const uint16_t SCREEN_H = 320;

static lv_disp_draw_buf_t draw_buf;
static lv_color_t        *buf1 = NULL;
static lv_color_t        *buf2 = NULL;

static TFT_eSPI           tft;
static XPT2046_Touchscreen touch(XPT2046_CS, XPT2046_IRQ);

static void disp_flush(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p)
{
    uint32_t w = area->x2 - area->x1 + 1;
    uint32_t h = area->y2 - area->y1 + 1;

    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.pushColors((uint16_t *)color_p, w * h, true);
    tft.endWrite();

    lv_disp_flush_ready(drv);
}

static void touchpad_read(lv_indev_drv_t *drv, lv_indev_data_t *data)
{
    if (touch.tirqTouched() && touch.touched()) {
        TS_Point p = touch.getPoint();

        int16_t x = map(p.x, TOUCH_X_MIN, TOUCH_X_MAX, 0, SCREEN_W - 1);
        int16_t y = map(p.y, TOUCH_Y_MIN, TOUCH_Y_MAX, 0, SCREEN_H - 1);

        data->point.x = constrain(x, 0, SCREEN_W - 1);
        data->point.y = constrain(y, 0, SCREEN_H - 1);
        data->state   = LV_INDEV_STATE_PR;
    } else {
        data->state = LV_INDEV_STATE_REL;
    }
}

WiFiClientSecure espClient;
PubSubClient client(espClient);

// Task Handles
TaskHandle_t NetworkTaskHandle;
TaskHandle_t DisplayTaskHandle;


void action_switch(lv_event_t *e) {
  relay=!relay;
  if (client.connected()) {
    if(relay){
      client.publish(pubTopic, "ON");
    }else{
      client.publish(pubTopic, "OFF");
    }
  } else {
    Serial.println("Cannot send: Not connected to MQTT.");
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  // Convert payload to String
  String message = "";
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  Serial.print(message);

  if (String(topic) == subTopic) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, message);

    if (error) {
      Serial.print("JSON Parse failed: ");
      Serial.println(error.c_str());
      return;
    }

    // Extract ALL the values
    float voltage = doc["voltage"];
    float current = doc["current"];
    float power = doc["power"];
    float energy = doc["energy"];
    float frequency = doc["frequency"];
    float pf = doc["pf"]; 

    char buffer[10];
    char format[10];

    dtostrf(voltage, 4, 1, buffer);
    snprintf(format, sizeof(format), "%s V", buffer);
    set_var_volt(format);

    dtostrf(current, 4, 3, buffer);
    snprintf(format, sizeof(format), "%s A", buffer);
    set_var_ampere(format);

    dtostrf(power, 4, 1, buffer);
    snprintf(format, sizeof(format), "%s W", buffer);
    set_var_watt(format);

    dtostrf(energy, 4, 3, buffer);
    set_var_watthour(buffer);

    dtostrf(frequency, 4, 1, buffer);
    snprintf(format, sizeof(format), "%s Hz", buffer);
    set_var_hertz(format);

    dtostrf(pf, 4, 2, buffer);
    set_var_powerfactor(buffer);
  }
}


void NetworkTask(void *pvParameters) {
  WiFi.begin(ssid, password);
  Serial.print("Connecting...");
  while (WiFi.status() != WL_CONNECTED) {
    vTaskDelay(500 / portTICK_PERIOD_MS); 
    Serial.print(".");
  }
  Serial.println("Wi-Fi Connected!");

  espClient.setInsecure();
  espClient.setTimeout(4);
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  for(;;) {
    if (!client.connected() && WiFi.status() == WL_CONNECTED) {
      String clientId = "ESP32-Monitor-" + String(random(0xffff), HEX);
      if (client.connect(clientId.c_str(), mqtt_user, mqtt_pass)) {
        Serial.println("EMQX Connected! Listening for PZEM data...");
          set_var_network(false);
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

void DisplayTask(void *pvParameters) {
  ui_init();
  for (;;) { 
    static uint32_t last_tick = 0;
    uint32_t current_tick = millis();
    uint32_t delta = current_tick - last_tick;
    last_tick = current_tick;

    lv_tick_inc(delta); 
    
    lv_task_handler();  
    ui_tick();
    vTaskDelay(5 / portTICK_PERIOD_MS);
  }
}

void setup() {
  Serial.begin(115200);
  tft.init();
  tft.setRotation(1);
  touch.begin();
  touch.setRotation(3);

  set_var_network(true);

  lv_init();

  uint32_t buffer_pixels = SCREEN_W * (SCREEN_H / 10);
  buf1 = (lv_color_t *)malloc(buffer_pixels * sizeof(lv_color_t));
  buf2 = (lv_color_t *)malloc(buffer_pixels * sizeof(lv_color_t));

  lv_disp_draw_buf_init(&draw_buf, buf1, buf2, buffer_pixels);

  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res  = SCREEN_W;
  disp_drv.ver_res  = SCREEN_H;
  disp_drv.flush_cb = disp_flush;
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register(&disp_drv);

  static lv_indev_drv_t indev_drv;
  lv_indev_drv_init(&indev_drv);
  indev_drv.type    = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = touchpad_read;
  lv_indev_drv_register(&indev_drv);

  xTaskCreatePinnedToCore(NetworkTask, "NetworkTask", 8192, NULL, 1, &NetworkTaskHandle, 1);
  xTaskCreatePinnedToCore(DisplayTask,"DisplayTask", 4096, NULL, 1, &DisplayTaskHandle, 1);
  }

void loop() {}
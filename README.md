# PZEM004-MQTT-Monitoring-ST7796-Display

A complete, end-to-end IoT energy monitoring and control system utilizing two ESP32 microcontrollers communicating via a secure MQTT broker. 

This system is divided into two parts:
1. **The Sensing Node (`sensing`):** Runs headless, reads AC electrical data from a PZEM-004T (v3.0) module, publishes the data as JSON, and listens for relay control commands.
2. **The GUI Node (`gui`):** Features an LVGL-powered touch interface that subscribes to the telemetry data for real-time visualization and provides a UI button to toggle the remote relay.

Both nodes utilize **FreeRTOS** to separate network operations from UI rendering and sensor polling, ensuring highly responsive and stable performance.

---

## ✨ System Features

* **Comprehensive Energy Reading:** Monitors real-time Voltage, Current, Power (W), Energy (kWh), Frequency (Hz), and Power Factor.
* **Remote Relay Control:** Bi-directional MQTT communication allows the GUI Node to seamlessly toggle a high-voltage relay connected to the Sensing Node.
* **Secure MQTT (TLS/SSL):** Both nodes utilize `WiFiClientSecure` for encrypted communication with modern MQTT brokers (e.g., EMQX).
* **Smooth UI (LVGL):** The GUI Node provides a responsive touch interface without freezing during network drops.
* **Resilient Connectivity:** Automated Wi-Fi and MQTT reconnection logic on both devices.

---

## 📂 Repository Structure

This project is built using **PlatformIO**. It contains two separate environments within the workspace:

```text
/
├── gui/                      # Display Node (LVGL Touchscreen)
│   ├── include/
│   ├── lib/
│   ├── src/                  # Main source code (main.cpp)
│   ├── test/
│   ├── PCB.png               # Custom PCB layout reference
│   ├── Schematic.png         # Hardware schematic reference
│   └── platformio.ini        # PlatformIO configuration for GUI
├── sensing/                  # Sensor & Relay Node (PZEM-004T)
│   ├── include/
│   ├── lib/
│   ├── src/                  # Main source code (main.cpp)
│   ├── test/
│   ├── platformio.ini        # PlatformIO configuration for Sensor
│   └── wiring.PNG            # Wiring diagram reference
└── README.md
```

---

## 🛠 Hardware Requirements

### Node 1: Sensing & Relay Unit (`sensing`)
* **ESP32** Development Board
* **PZEM-004T (v3.0)** AC Energy Monitor
* **Relay Module** (Active HIGH)

| Component | ESP32 Pin | Note |
| :--- | :--- | :--- |
| **PZEM TX** | `GPIO 16` | Hardware Serial 2 (RX) |
| **PZEM RX** | `GPIO 17` | Hardware Serial 2 (TX) |
| **Relay Signal**| `GPIO 23` | Toggles the load |

*⚠️ **Warning:** The PZEM-004T must be safely wired to mains AC power. AC voltage is extremely dangerous; proceed with strict caution and proper isolation.*

### Node 2: GUI Unit (`gui`)
* **ESP32** Development Board
* **TFT Display** (480x320 resolution recommended) compatible with `TFT_eSPI`.
* **XPT2046 Touch Controller** (Touch CS pin configured to `GPIO 21`).

---

## 📦 Software Dependencies

Since this is a PlatformIO project, dependencies should be managed via your respective `platformio.ini` files. 

**Shared Dependencies (Both Nodes):**
* `knolleary/PubSubClient`
* `bblanchon/ArduinoJson`

**Sensing Node Specific:**
* `mandulaj/PZEM-004T-v30`

**GUI Node Specific:**
* `bodmer/TFT_eSPI` (Configure `User_Setup.h` for your display)
* `paulstoffregen/XPT2046_Touchscreen`
* `lvgl/lvgl` (Version 8.x, ensure `lv_conf.h` is properly set up)

---

## 🚀 Setup & Configuration

### 1. Network & Broker Settings
You must configure the exact same Wi-Fi and MQTT broker credentials in the main source files (e.g., `src/main.cpp`) for **both** nodes:

```cpp
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

const char* mqtt_server = "YOUR_MQTT_BROKER_URL_OR_IP";
const int mqtt_port = 8883; // 8883 for SSL/TLS, 1883 for unencrypted TCP
const char* mqtt_user = "YOUR_MQTT_USERNAME";
const char* mqtt_pass = "YOUR_MQTT_PASSWORD";
```

### 2. Topic Architecture
The system relies on two specific topics. The nodes are pre-configured to cross-communicate perfectly:

* **Telemetry Topic:** `esp32/sensors/data`
  * *Sensing Node:* Publishes to this topic every 10 seconds.
  * *GUI Node:* Subscribes to this topic to update the UI.
* **Command Topic:** `esp32/commands`
  * *GUI Node:* Publishes `ON` or `OFF` when the touch button is pressed.
  * *Sensing Node:* Subscribes to this topic to toggle GPIO 23.

### 3. JSON Data Format
If you are debugging the MQTT broker, the telemetry payload formatted by the Sensing Node looks like this:
```json
{
  "voltage": 225.4,
  "current": 1.150,
  "power": 250.2,
  "energy": 45.120,
  "frequency": 50.0,
  "pf": 0.95
}
```

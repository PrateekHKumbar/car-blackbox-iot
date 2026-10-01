# Wireless Car Black Box System

  ## Overview
  ESP32-based IoT telemetry system for real-time vehicle health monitoring and accident/fire event  
  logging. Integrated with A7670C 4G LTE module for automated emergency alerts and GPS location     
  tracking.

  ## Hardware Components
  - **ESP32 microcontroller** (development board)
  - **A7670C 4G LTE module** for cellular connectivity
  - **ESP32-CAM** for video streaming capability
  - **Vibration acceleration sensor** (ADXL335/Arduino)
  - **GPS module** (Module-based RTK positioning)
  - **12V power management system**
  - **LCD display** (16x2 I2C for local status)
  - **Push-button** for manual emergency triggers
  - **Battery backup system** (Li-Po for continuous operation)

  ## Key Features
  - Real-time data streaming at 10 Hz over 4G LTE
  - Automated emergency alert system (accident/fire detection)
  - GPS tracking and location reporting
  - Vehicle diagnostics (RPM, temperature, voltage)
  - Video recording with timestamp
  - Cloud storage integration
  - Mobile app notification system
  - Solar charging capability (for field deployments)

  ## Technical Architecture
  ESP32 ↔ LTE Module ↔ Cloud Server ↔ Mobile App ↔ Emergency Services
       ↘ Sensors → Local Storage → Alert Trigger

  ## Technologies Used
  - **Hardware:** ESP32, A7670C, ADXL335, GPS Module, ESP32-CAM
  - **Communication:** UART, I2C, SPI, 4G LTE
  - **Programming:** Embedded C, Arduino IDE, PlatformIO
  - **Cloud:** HTTP REST APIs, MQTT protocol
  - **Mobile:** Android (Kotlin), iOS (Swift) companion apps
  - **Storage:** MongoDB (cloud), Local SD card backup
  - **UI:** Web dashboard (React.js), Mobile apps

  ## Software Implementation

  ### ESP32 Firmware (Key Snippets)

  ```cpp
  // /src/main.cpp - Core telemetry logic
  #include <WiFi.h>
  #include <HTTPClient.h>
  #include <Adafruit_Sensor.h>
  #include <Adafruit_ADXL335.h>
  #include <TinyGPSPlus.h>
  #include <esp_camera.h>

  #define LED_GPIO 2
  #define BUTTON_PIN 15
  #define VIBRATION_PIN A0

  // Global objects
  Adafruit_ADXL335 vibration;
  HardwareSerial gpsSerial(1);
  TinyGPSPlus gps;
  unsigned long lastDataSend = 0;
  unsigned long lastGPS = 0;
  bool emergencyMode = false;

  void setup() {
    Serial.begin(115200);
    pinMode(LED_GPIO, OUTPUT);
    pinMode(BUTTON_PIN, INPUT_PULLUP);

    // Initialize vibration sensor
    if (!vibration.begin()) {
      Serial.println("Failed to initialize vibration sensor");
      while (1);
    }

    // Initialize camera
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = 5;
    config.pin_d1 = 4;
    config.pin_d2 = 0;
    config.pin_d3 = 2;
    config.pin_d4 = 18;
    config.pin_d5 = 19;
    config.pin_d6 = 27;
    config.pin_d7 = 26;
    config.pin_xclk = 21;
    config.pin_pclk = 22;
    config.pin_vsync = 25;
    config.pin_href = 23;
    config.pin_sscb_sda = 32;
    config.pin_sscb_scl = 33;
    config.sclk_speed = 20000000;
    config.xclk_freq = 20000000;

    esp_camera_init(&config);
  }

  void loop() {
    // Read sensor data
    float vibrationReading = vibration.read();
    bool buttonPressed = digitalRead(BUTTON_PIN) == LOW;
    float voltage = analogRead(VIBRATION_PIN) * (3.3 / 4095.0);

    // GPS update
    if (millis() - lastGPS > 1000) {
      while (gpsSerial.available()) {
        gps.encode(gpsSerial.read());
        lastGPS = millis();
      }
    }

    // Check emergency conditions
    if (vibrationReading > 2.5 || buttonPressed || !gps.location.isValid()) {
      emergencyMode = true;
      sendEmergencyAlert();
    }

    // Send telemetry data every 5 seconds
    if (millis() - lastDataSend > 5000) {
      sendTelemetryData(vibrationReading, voltage, gps.location.lat(), gps.location.lng());
      lastDataSend = millis();
    }

    // LED indicator
    digitalWrite(LED_GPIO, emergencyMode ? HIGH : LOW);
    delay(100);
  }

  void sendTelemetryData(float vibration, float voltage, double lat, double lng) {
    HTTPClient http;

    // Construct JSON payload
    String jsonString = "{";
    jsonString += "\"vibration\":" + String(vibration) + ",";
    jsonString += "\"voltage\":" + String(voltage) + ",";
    jsonString += "\"latitude\":" + String(lat, 6) + ",";
    jsonString += "\"longitude\":" + String(lng, 6) + ",";
    jsonString += "\"timestamp\":" + String(millis()) + ",";
    jsonString += "\"emergency\":" + String(emergencyMode ? "true" : "false") + ",";
    jsonString += "\"gps_valid\":" + String(gps.location.isValid() ? "true" : "false");
    jsonString += "}";

    http.begin("http://your-server.com/api/telemetry");
    http.addHeader("Content-Type", "application/json");

    int httpResponseCode = http.POST(jsonString);

    if (httpResponseCode == 200) {
      Serial.println("Data sent successfully");
    } else {
      Serial.println("Error sending data: " + String(httpResponseCode));
    }

    http.end();
  }

  void sendEmergencyAlert() {
    HTTPClient http;

    String emergencyData = "{";
    emergencyData += "\"type\":\"critical_emergency\",";
    emergencyData += "\"location\":\"" + String(gps.location.lat(), 6) + "," +
  String(gps.location.lng(), 6) + "\",";
    emergencyData += "\"vehicle_id\":\"CB001\",";
    emergencyData += "\"timestamp\":" + String(millis());
    emergencyData += "}";

    http.begin("http://your-server.com/api/emergency");
    http.addHeader("Content-Type", "application/json");
    http.addHeader("X-Emergency-Priority", "HIGH");

    int response = http.POST(emergencyData);

    if (response == 200) {
      Serial.println("Emergency alert sent!");
      // Notify mobile app
      notifyMobileApp("Emergency detected", "Vehicle needs immediate assistance");
    }

    http.end();
  }

  void notifyMobileApp(String title, String message) {
    // Implement push notification logic here
    // Could use Firebase Cloud Messaging, Pusher, or custom webhook
  }

  Web Dashboard (React.js)

  // /frontend/src/App.js - Real-time monitoring dashboard
  import React, { useState, useEffect } from 'react';
  import axios from 'axios';

  function App() {
    const [telemetryData, setTelemetryData] = useState([]);
    const [emergencyAlerts, setEmergencyAlerts] = useState([]);
    const [vehicleStatus, setVehicleStatus] = useState('ONLINE');

    useEffect(() => {
      // WebSocket connection for real-time updates
      const ws = new WebSocket('ws://your-server.com/telemetry');

      ws.onmessage = (event) => {
        const data = JSON.parse(event.data);
        if (data.type === 'telemetry') {
          setTelemetryData(prev => [...prev.slice(-50), data]);
        } else if (data.type === 'emergency') {
          setEmergencyAlerts(prev => [data, ...prev]);
        }
      };

      return () => ws.close();
    }, []);

    const sendEmergencyCommand = (command) => {
      axios.post('/api/emergency', {
        command: command,
        vehicleId: 'CB001'
      }).then(response => {
        alert('Emergency command sent: ' + command);
      });
    };

    return (
      <div className="app">
        <header>
          <h1>🚗 Car Black Box - Live Monitoring</h1>
          <div className={`status ${vehicleStatus.toLowerCase()}`}>
            {vehicleStatus}
          </div>
        </header>

        <div className="grid">
          <div className="card">
            <h3>📊 Live Telemetry</h3>
            {telemetryData.slice(-10).map((data, index) => (
              <div key={index} className="telemetry-item">
                <span>Vibration: {data.vibration.toFixed(2)}</span>
                <span>Voltage: {data.voltage.toFixed(2)}V</span>
                <span>Lat: {data.latitude.toFixed(6)}</span>
                <span>Lng: {data.longitude.toFixed(6)}</span>
              </div>
            ))}
          </div>

          <div className="card">
            <h3>🚨 Emergency Controls</h3>
            <button onClick={() => sendEmergencyCommand('SOS')} className="emergency-btn">
              Send SOS
            </button>
            <button onClick={() => sendEmergencyCommand('STOP')} className="stop-btn">
              Stop Engine
            </button>
            <button onClick={() => sendEmergencyCommand('LOCATE')} className="locate-btn">
              Locate Vehicle
            </button>
          </div>

          <div className="card">
            <h3>⚠️ Recent Alerts</h3>
            {emergencyAlerts.map((alert, index) => (
              <div key={index} className="alert-item emergency">
                <strong>{new Date(alert.timestamp).toLocaleTimeString()}</strong>
                <p>{alert.message}</p>
              </div>
            ))}
          </div>
        </div>
      </div>
    );
  }

  export default App;

  ---

  📁 Repository Structure

  car-blackbox-iot/
  ├── README.md                    # Project overview
  ├── src/
  │   ├── main.cpp                 # ESP32 firmware
  │   ├── sensors.cpp             # Sensor integration
  │   └── network.cpp             # Network communication
  ├── docs/
  │   ├── SCHEMATIC.pdf           # Circuit diagram
  │   ├── WIRING_GUIDE.md         # Wiring instructions
  │   └── INSTALLATION.md         # Setup guide
  ├── frontend/
  │   ├── src/
  │   │   └── App.js              # Web dashboard
  │   ├── public/
  │   │   └── index.html
  │   └── package.json
  ├── videos/
  │   ├── demo_recorded.mp4        # Demonstration footage
  │   └── tutorial.mp4            # Setup tutorial
  ├── images/
  │   ├── schematic.png           # Circuit diagram
  │   └── wiring_diagram.jpg      # Assembly guide
  ├── assets/
  │   ├── emergency_protocol.pdf  # Emergency procedures
  │   └── gps_coordinates.txt     # GPS data samples
  └── .github/
      └── workflows/
          └── ci.yml               # GitHub Actions for testing

/*
 * Project: IoT-Enabled Wireless Car Black Box System
 * Platform: Arduino Uno (ATmega328P)
 * Authors: Prateek H Kumbar & Team
 * Department: Electronics & Telecommunication Engineering, BIT Bengaluru
 * 
 * Hardware Interfacing:
 * - MPU6050 Accelerometer: I2C (SDA -> Pin A4, SCL -> Pin A5)
 * - Flame Sensor (IR): Digital Pin 10
 * - Neo-6M GPS Module: SoftwareSerial (RX -> Pin 4, TX -> Pin 3)
 * - SIM A7670C 4G LTE Module: SoftwareSerial (RX -> Pin 7, TX -> Pin 8)
 */

#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <SoftwareSerial.h>
#include <TinyGPS++.h>

// --- Pin Definitions ---
#define FLAME_SENSOR_PIN 10
#define GPS_RX_PIN 4
#define GPS_TX_PIN 3
#define GSM_RX_PIN 7
#define GSM_TX_PIN 8

// --- System Thresholds & Parameters ---
const float ACCEL_THRESHOLD = 25.0;            // Impact threshold in m/s^2 (~2.5g)
const unsigned long COOLDOWN_INTERVAL = 20000; // Cooldown timer (20 seconds) in milliseconds
const char EMERGENCY_PHONE_NUMBER[] = "+919113996868"; // Target emergency mobile number

// --- Global Objects & Variables ---
Adafruit_MPU6050 mpu;
TinyGPSPlus gps;
SoftwareSerial gpsSerial(GPS_RX_PIN, GPS_TX_PIN);
SoftwareSerial gsmSerial(GSM_RX_PIN, GSM_TX_PIN);

unsigned long lastAlertTime = 0;
float currentLatitude = 0.0;
float currentLongitude = 0.0;

// --- Function Declarations ---
void initMPU6050();
void initGSM();
void updateGPSLocation();
void sendEmergencyAlert(const char* eventType);

void setup() {
  // Initialize Serial Monitors
  Serial.begin(9600);
  gpsSerial.begin(9600);
  gsmSerial.begin(9600);

  // Initialize Digital Pins
  pinMode(FLAME_SENSOR_PIN, INPUT);

  Serial.println(F("=============================================="));
  Serial.println(F("   IoT Wireless Car Black Box - Starting POST "));
  Serial.println(F("=============================================="));

  // Initialize Hardware Modules
  initMPU6050();
  initGSM();

  Serial.println(F("[SYSTEM] Power-On Self-Test (POST) Complete. Monitoring active..."));
}

void loop() {
  // 1. Continuously Parse GPS NMEA Sentences
  updateGPSLocation();

  // 2. Read Acceleration Data from MPU6050
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  // Calculate Euclidean Norm Vector Magnitude: |A| = sqrt(ax^2 + ay^2 + az^2)
  float netAcceleration = sqrt(pow(a.acceleration.x, 2) + 
                               pow(a.acceleration.y, 2) + 
                               pow(a.acceleration.z, 2));

  // 3. Read Flame Sensor State (Logic LOW = Flame Detected)
  bool fireDetected = (digitalRead(FLAME_SENSOR_PIN) == LOW);
  bool impactDetected = (netAcceleration >= ACCEL_THRESHOLD);

  // 4. Decision Node & Cooldown Verification
  if ((impactDetected || fireDetected) && (millis() - lastAlertTime >= COOLDOWN_INTERVAL)) {
    if (impactDetected && fireDetected) {
      Serial.println(F("[ALERT CRITICAL] Simultaneous Collision & Fire Hazard Detected!"));
      sendEmergencyAlert("CRITICAL: COLLISION & FIRE DETECTED");
    } 
    else if (impactDetected) {
      Serial.print(F("[ALERT] Collision Impact Detected! Magnitude: "));
      Serial.print(netAcceleration);
      Serial.println(F(" m/s^2"));
      sendEmergencyAlert("CRASH/IMPACT DETECTED");
    } 
    else if (fireDetected) {
      Serial.println(F("[ALERT] Thermal Fire Hazard Detected in Vehicle Cabin!"));
      sendEmergencyAlert("FIRE HAZARD DETECTED");
    }

    lastAlertTime = millis(); // Reset Cooldown Timer
  }
}

// --- Helper Functions ---

void initMPU6050() {
  Serial.print(F("[INIT] Connecting to MPU6050... "));
  if (!mpu.begin(0x68)) {
    Serial.println(F("FAILED! Check Wire connections. System halted."));
    while (1) { delay(10); }
  }
  Serial.println(F("SUCCESS."));
  
  // Configure MPU6050 Ranges
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
}

void initGSM() {
  Serial.print(F("[INIT] Initializing SIM A7670C 4G LTE Module... "));
  gsmSerial.listen();
  delay(1000);
  
  gsmSerial.println(F("AT"));
  delay(500);
  gsmSerial.println(F("AT+CMGF=1")); // Set GSM Module to Text Mode
  delay(500);
  Serial.println(F("SUCCESS."));
}

void updateGPSLocation() {
  gpsSerial.listen();
  unsigned long startTime = millis();
  
  // Non-blocking parse loop for 200ms
  while (millis() - startTime < 200) {
    while (gpsSerial.available() > 0) {
      gps.encode(gpsSerial.read());
    }
  }

  if (gps.location.isValid()) {
    currentLatitude = gps.location.lat();
    currentLongitude = gps.location.lng();
  }
}

void sendEmergencyAlert(const char* eventType) {
  gsmSerial.listen();
  delay(100);

  Serial.println(F("[GSM] Dispatching Emergency SMS Payload over LTE..."));

  // AT Command Sequence for Text Mode Payload
  gsmSerial.println(F("AT+CMGF=1"));
  delay(500);
  
  gsmSerial.print(F("AT+CMGS=\""));
  gsmSerial.print(EMERGENCY_PHONE_NUMBER);
  gsmSerial.println(F("\""));
  delay(500);

  // Construct SMS Body
  gsmSerial.print(F("EMERGENCY ALERT: "));
  gsmSerial.println(eventType);
  gsmSerial.println(F("Vehicle Black Box System triggered."));
  
  if (currentLatitude != 0.0 && currentLongitude != 0.0) {
    gsmSerial.print(F("Location: https://maps.google.com/?q="));
    gsmSerial.print(currentLatitude, 6);
    gsmSerial.print(F(","));
    gsmSerial.println(currentLongitude, 6);
  } else {
    gsmSerial.println(F("Location: GPS Locking... (Acquiring Satellites)"));
  }

  delay(500);
  gsmSerial.write(26); // ASCII character 26 (Ctrl+Z) to finalize SMS transmission
  delay(5000);

  Serial.println(F("[GSM] Emergency SMS Payload Dispatched Successfully."));
}

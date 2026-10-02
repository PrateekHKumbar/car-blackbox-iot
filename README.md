# IoT-Enabled Wireless Car Black Box System 🚗📡

A standalone automotive telemetry and safety system designed for real-time collision detection, thermal hazard monitoring, and autonomous 4G LTE distress dispatch without smartphone dependency.

---

### 📌 System Overview

In severe vehicle collisions, response latency during the post-crash "Golden Hour" directly impacts occupant survivability. This embedded system continuously monitors 6-axis vehicle kinematics and thermal states. Upon detecting critical impact force or fire outbreaks, it acquires satellite GPS coordinates and dispatches automated cellular SMS emergency alerts containing Google Maps location links in under 8 seconds.

---

### ⚡ Technical Highlights & Engineering Specifications

- **Impact Vector Calculation:** Computes Euclidean norm acceleration vectors $\sqrt{a_x^2 + a_y^2 + a_z^2}$ from 6-axis MEMS accelerometer data (MPU6050) to filter operational vibrations.
- **Low-Latency Interrupt:** Triggers an Interrupt Service Routine (ISR) when impact forces exceed $2.5g$ ($25\text{ m/s}^2$) with a response latency of $<100\text{ ms}$.
- **Geospatial Precision:** Neo-6M GPS module acquires valid coordinate locks ($CEP < 10\text{ m}$) via NMEA packet parsing using the `TinyGPS++` library.
- **Cellular Telemetry:** Automated Hayes AT command sequencing over the SIM A7670C 4G LTE transceiver dispatches SMS alerts with Google Maps hyperlinks to emergency contacts.
- **Dual Hazard Monitoring:** Integrates an optical IR flame sensor (760nm–1100nm spectrum) for immediate vehicle thermal fire detection.

---

### 🛠️ Hardware Stack & Components

- **Microcontroller:** Arduino Uno (ATmega328P MCU)
- **Inertial Measurement:** MPU6050 6-DOF MEMS Accelerometer & Gyroscope (I²C)
- **Cellular Transceiver:** SIM A7670C 4G LTE Module (SoftwareSerial UART)
- **Positioning Module:** Neo-6M GPS Receiver
- **Sensors:** Optical IR Photodiode Flame Sensor
- **Power Regulation:** LM2596 DC-DC Buck Converter (12V vehicle battery down to 5V rail)

---

### 📂 Project Documentation

Full technical implementation details, circuit diagrams, flowcharts, and test results are documented in the project report:
- 📄 **[View Full Project Report PDF](./Car_Black_box.pdf)**

---

### 🌐 Author & Portfolio

- **Developer:** Prateek H Kumbar
- **Department:** Electronics & Telecommunication Engineering, Bangalore Institute of Technology
- **Live Portfolio:** [prateekhkumbar.github.io/my-portfolio](https://prateekhkumbar.github.io/my-portfolio/)

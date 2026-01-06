

# 🩺 Guardian – Smart Medicine Box Web System

**A Web-Based Smart Medicine Box for Elderly Care Using IoT and QR-Based Prescription**

---

## 📌 Project Overview

**Guardian** is a web-based smart medicine management system designed to help elderly people and patients take the **right medicine at the right time**.
The system uses **QR-based digital prescriptions**, **Firebase real-time database**, and an **ESP32-based IoT medicine box**.

This repository contains **two web applications**:

1. **Prescription QR Code Generator Website**
2. **Guardian Smart Medicine Box Control Website**

These two websites work together to automate medicine scheduling and drawer-based medicine dispensing.

---

## 🌐 Website 1: Prescription QR Code Generator

### 🔍 Purpose

This website is used to **generate a QR code from a digital prescription**.
The QR code contains structured JSON data that is readable by the Guardian Smart Medicine Box system.

### ✨ Features

* Input multiple medicines with:

  * Time (`morning`, `noon`, `evening`, `night`)
  * Medicine name
  * Quantity
* Fixed schedule mapping:

  * Morning → 08:00
  * Noon → 13:00
  * Evening → 18:00
  * Night → 22:00
* Automatically generates:

  * Structured JSON prescription
  * QR code from JSON
* Downloadable QR image for later use

### 🧾 Example Input

```
morning,Napa,1
noon,Ace,1
evening,Vitamin,2
night,Melatonin,1
```

### 🧠 Generated JSON Structure

```json
{
  "schedule": {
    "morning": { "hour": 8, "minute": 0 },
    "noon": { "hour": 13, "minute": 0 },
    "evening": { "hour": 18, "minute": 0 },
    "night": { "hour": 22, "minute": 0 }
  },
  "medicines": {
    "morning": [{ "name": "Napa", "qty": 1 }],
    "noon": [{ "name": "Ace", "qty": 1 }],
    "evening": [{ "name": "Vitamin", "qty": 2 }],
    "night": [{ "name": "Melatonin", "qty": 1 }]
  }
}
```

### 🛠 Technologies Used

* HTML5
* CSS3
* JavaScript
* QRCode.js Library

📁 Folder: `qr-generator/`
📄 Main files: `index.html`, `script.js`, `style.css`

---

## 🌐 Website 2: Guardian Smart Medicine Box Control Panel

### 🔍 Purpose

This website is used by caregivers or patients to **manage the smart medicine box**, import prescriptions, and monitor drawer schedules.

### ✨ Features

* Real-time **device online/offline status**
* Scan prescription QR using:

  * Mobile / laptop camera
  * Uploaded QR image
* Automatic prescription import:

  * Converts QR data into drawer-based schedule
  * Saves data to Firebase
* Manual drawer time configuration
* Add medicines manually (name, quantity, drawer)
* **Medicine Loading Guide**

  * Shows which medicines go into which drawer
  * Displays scheduled time per drawer
* Fully synced with ESP32 via Firebase

### 🧠 Drawer Mapping Logic

| Time    | Drawer   |
| ------- | -------- |
| Morning | Drawer 1 |
| Noon    | Drawer 2 |
| Evening | Drawer 3 |
| Night   | Drawer 4 |

### 🛠 Technologies Used

* HTML5
* CSS3
* JavaScript
* Firebase Realtime Database
* HTML5 QR Scanner Library

📁 Folder: `guardian-web/`
📄 Main files: `index.html`, `script.js`, `style.css`

---

## 🔗 System Workflow

1. Doctor or caregiver creates a digital prescription
2. Prescription is converted into a QR code (Website 1)
3. QR code is scanned using Guardian Control Panel (Website 2)
4. Data is stored in Firebase
5. ESP32 smart medicine box reads data from Firebase
6. Drawer opens at scheduled time with correct medicine

---

## 🚀 How to Run Locally

1. Clone the repository

```bash
git clone https://github.com/your-username/guardian-smart-medicine-box.git
```

2. Open either website:

* `qr-generator/index.html`
* `guardian-web/index.html`

⚠️ Note: Firebase features require internet access.

---

## 🎓 Use Cases

* Elderly care at home
* Smart healthcare IoT systems
* Academic IoT & Web projects
* Conference / journal research prototype



# 🔌 ESP32 Firmware – Guardian Smart Medicine Box

## 📌 Overview

This firmware runs on an **ESP32 microcontroller** and acts as the **core controller** of the Guardian Smart Medicine Box.
It connects to **Wi-Fi**, synchronizes with **Firebase Realtime Database**, tracks **real-time schedules**, and controls **drawer indicators and sensors** to ensure correct medicine intake.

📄 Source file: `sketch_jan5a.ino`

---

## 🧠 Key Responsibilities

* Connect to Wi-Fi network
* Synchronize with Firebase Realtime Database
* Read medicine schedules and drawer data
* Trigger drawer reminders at correct times
* Display medicine information on I2C LCD
* Detect drawer open/close using reed switches
* Confirm medicine intake
* Update device status to Firebase in real time

---

## ⚙️ Hardware Components Used

| Component              | Description                 |
| ---------------------- | --------------------------- |
| ESP32 Dev Board        | Main controller with Wi-Fi  |
| DS3231 RTC             | Accurate real-time clock    |
| 16×2 I2C LCD           | Medicine & status display   |
| Magnetic Reed Switches | Drawer open/close detection |
| LEDs (Red/Green)       | Drawer indication           |
| Power Supply           | 5V regulated input          |

---

## 🌐 Firebase Data Structure (Used by ESP32)

```json
{
  "device": {
    "online": true,
    "activeDrawer": 1,
    "rtcTime": "08:00:00"
  },
  "schedule": {
    "1": { "hour": 8, "minute": 0 },
    "2": { "hour": 13, "minute": 0 },
    "3": { "hour": 18, "minute": 0 },
    "4": { "hour": 22, "minute": 0 }
  },
  "drawers": {
    "1": { "medicines": {} },
    "2": { "medicines": {} },
    "3": { "medicines": {} },
    "4": { "medicines": {} }
  }
}
```

---

## ⏰ Drawer–Time Mapping

| Drawer   | Time    |
| -------- | ------- |
| Drawer 1 | Morning |
| Drawer 2 | Noon    |
| Drawer 3 | Evening |
| Drawer 4 | Night   |

---

## 🖥 LCD Display Behavior

* Shows current **RTC time**
* Displays **drawer number** when alarm triggers
* Displays **medicine name and quantity**
* Updates automatically when drawer is opened/closed
* Clears and switches to next medicine after confirmation

---

## 🚨 Sensor & Indicator Logic

* **Reed Switch**

  * Closed → Drawer closed
  * Open → Drawer opened (medicine taken)
* **LEDs**

  * Red LED → Reminder active
  * Green LED → Drawer correctly opened
* Drawer closes → LEDs turn off → Next drawer waits

---

## 🔐 Network & Security

* Wi-Fi connection handled at startup
* Firebase authentication via database secret
* Device online/offline status updated in Firebase

---

## 🛠 Libraries Used

* `WiFi.h`
* `Firebase_ESP_Client.h`
* `RTClib.h`
* `LiquidCrystal_I2C.h`
* `Wire.h`
* `time.h`

---

## 🚀 How to Upload the Firmware

1. Open **Arduino IDE**
2. Select:

   * Board: **ESP32 Dev Module**
   * Port: Your ESP32 COM port
3. Install required libraries
4. Open `sketch_jan5a.ino`
5. Click **Upload**

---

## 🧪 Debug & Monitoring

* Serial Monitor shows:

  * Wi-Fi connection status
  * Firebase connection status
  * RTC time
  * Drawer open/close state
  * Active alarms

Recommended baud rate:

```
115200
```

---

## 🎯 Use Cases

* Elderly medication adherence
* IoT healthcare monitoring
* Smart home medical devices
* Research & academic prototypes

---

## 👨‍💻 Author

**Abdullah Al Galib Tonmoy**
CSE, BAUST
Project: *Guardian – A Web-Based Smart Medicine Box for Elderly Care Using IoT and QR-Based Prescription*

---

## 📜 License

This firmware is intended for **academic and research purposes**.

---



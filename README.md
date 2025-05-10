# 🌬️ Smart Swamp Cooler Controller (CPE 301 Final Project)

An Arduino Mega-based smart swamp cooler controller designed by Romina Mendez, Anmol Sandhu, and Alex Robinson for CPE 301 – Spring 2025. This system automates a fan based on temperature, humidity, and water level using real-time sensor inputs and displays its state on an LCD screen with LEDs and buttons for user interaction.

---

## 🧠 Features

- DHT11 temperature & humidity sensor 📈
- Analog water level sensor 🪣
- LCD (16x2) real-time display 📟
- Button-controlled START & RESET interface 🔘
- LED indicators for system state (DISABLED, IDLE, RUNNING, ERROR) 💡
- Fan motor control with NPN transistor 🔁
- Optional stepper motor for airflow adjustment
- RTC (real-time clock) for timestamp logging ⏱️

---

## 🛠️ Hardware Setup

| Component        | Arduino Pin | Notes |
|------------------|-------------|-------|
| DHT11            | D9          | Data only – VCC & GND also required |
| Water Sensor     | A0          | Analog output |
| START Button     | D2          | Uses interrupt |
| RESET Button     | D4          | Polled input |
| Fan Motor        | D8          | Controlled via NPN + flyback diode |
| LCD RS           | D37         | Parallel mode |
| LCD EN           | D35         |  |
| LCD D4-D7        | D33–27      |  |
| LCD VO           | Potentiometer | For contrast adjustment |
| LED - DISABLED   | D22         | Yellow |
| LED - IDLE       | D23         | Green |
| LED - RUNNING    | D24         | Blue |
| LED - ERROR      | D25         | Red |

📷 Check out `schematic.png` for the full wiring diagram.

---

## 🚦 System Behavior

| State      | Trigger                         | Display / Action                     |
|------------|----------------------------------|--------------------------------------|
| DISABLED   | Power on or press START         | Yellow LED ON, LCD shows DISABLED   |
| IDLE       | Normal temp, good water level   | Green LED ON, LCD shows IDLE        |
| RUNNING    | Temp > 28°C                     | Blue LED ON, fan ON, LCD shows RUNNING |
| ERROR      | Water level < 300               | Red LED ON, fan OFF, LCD shows ERROR |
| RESET      | Press RESET (D4) in ERROR state | Returns to IDLE if water restored    |

---

## 🧪 How to Use

1. Upload `SwampCooler_Final.ino` using Arduino IDE
2. Wire components using `schematic.png`
3. Open Serial Monitor (9600 baud) to see log timestamps
4. Press **START** to begin system
5. Blow on DHT11 or dry water sensor to test state transitions
6. Use **RESET** to recover from low water

---

## 🧑‍💻 Authors

- Romina Mendez
- Anmol Sandhu
- Alex Robinson

---

## 📂 Files Included

- `CPE301-Final Project.ino` – Full Arduino source code
- `CPE301-Schematic.png` – Wiring diagram of breadboard circuit
- `IMG_7203 (1).mov` – Video
- `CPE301 Final Project.pdf` – PDF final report
- `README.md` – This file

---

## 🏁 Status

✅ 100% Working as of May 2025  
📚 Final submitted version for CPE 301 Spring 2025

---

## 🧠 Special Notes

- Make sure GND is shared across **all** modules (LCD, sensors, fan)
- Use external power or barrel jack when running fan + stepper
- If LCD shows blocks: check contrast pin or D3 conflict

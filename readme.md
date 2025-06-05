# Water Monitoring Project – Codebase Walkthrough

This project is a **water level monitoring system** using an ESP32/ESP8266 microcontroller, a rain/water sensor, and a C++ TCP server for data collection, logging, and analysis. The system is designed for real-time monitoring and historical data analysis.

---

## Table of Contents

- Project Structure
- How It Works
- Key Components
  - Arduino/ESP Code
  - C++ TCP Server
  - Data Handling & Analysis
- How to Build and Run
- Troubleshooting
- Customization
- File Overview

---

## Project Structure

```
Fork1FinproAlprog/
│
├── waterleveliot.ino                  # ESP32 code for WiFi + sensor + TCP client
├── simplerainsensor.ino               # Simple Arduino sketch for rain sensor + LEDs
│
├── water_monitoring_project/
│   ├── arduino/
│   │   ├── simplerainsensor.ino       # (duplicate, for reference)
│   │   └── sensorair.cpp              # ESP8266 + ThingSpeak example
│   └── src/
│       ├── mainairfinal.cpp           # Main C++ TCP server (with menu, logging, analysis)
│       ├── mainair.cpp                # Alternate/older server version
│       ├── mainlogic (REDUNDANT).cpp  # Serial port logger (legacy)
│       ├── data.json                  # Exported data (JSON)
│
├── serverfile.cpp                     # Minimal TCP server (demo)
│
└── .vscode/                           # VS Code config (build, debug, intellisense)
```

---

## How It Works

1. **Sensor Node (ESP32/ESP8266):**
   - Reads water/rain sensor value.
   - Connects to WiFi.
   - Sends sensor data as JSON to the server via TCP.

2. **Server (C++):**
   - Listens for TCP connections on port 8080.
   - Receives JSON data, parses `rainValue`, maps it to a status.
   - Logs timestamped status to a binary file (`data.bin`).
   - Provides a menu for exporting, searching, and sorting data.

---

## Key Components

### Arduino/ESP Code

#### waterleveliot.ino
- Connects ESP32 to WiFi.
- Reads analog value from rain sensor.
- Lights up LEDs based on water level.
- Sends JSON data (e.g., `{"rainValue": 2750}`) to the server via TCP.
- Handles server responses.

#### `sensorair.cpp`
- ESP8266 version, sends data to ThingSpeak (for IoT cloud logging).

#### simplerainsensor.ino
- Minimal sketch: reads sensor, prints value, controls LEDs.

---

### C++ TCP Server

#### `mainairfinal.cpp`
- **Menu-driven terminal UI**:
  1. **Monitoring**: Starts TCP server, logs incoming data.
  2. **Export to JSON**: Converts binary log to data.json.
  3. **Search by Date**: Finds entries for a specific date.
  4. **Sort by Status**: Displays all entries sorted by status.
  5. **Quit**: Exits program.
- **TCPServer class**:
  - Accepts multiple clients (threaded).
  - Parses incoming JSON, maps `rainValue` to status:
    - `>= 3000`: Status 0 (High Level / Critical)
    - `1500 < value < 3000`: Status 1 (Stable)
    - `<= 1500`: Status 2 (Low Level / Critical)
  - Logs status with timestamp to `data.bin`.
- **Data Handling**:
  - Binary log for efficiency.
  - Export to JSON for analysis/visualization.
  - Search and sort features for historical review.

#### `mainair.cpp`
- Alternate/older version with similar logic.

#### `mainlogic (REDUNDANT).cpp`
- Reads data from serial port (legacy, not TCP).

#### serverfile.cpp
- Minimal TCP server for demo/testing.

---

### Data Handling & Analysis

- **Binary Log (`data.bin`)**: Stores timestamp and status for each event.
- **JSON Export (data.json)**: Human-readable, includes both timestamp and formatted datetime.
- **Search/Sort**: Find data by date or status for reporting.

---

## How to Build and Run

### Prerequisites

- **Windows** (MinGW recommended)
- **g++** in PATH (see c_cpp_properties.json)
- **VS Code** (optional, for easier build/debug)

### Build Command

Open terminal in the project directory and run:

```
g++ -o mainairfinalwithtime.exe water_monitoring_project/src/mainairfinal.cpp -lws2_32
```

- The `-lws2_32` links the Winsock library (required for networking on Windows).

### Run

```
./mainairfinalwithtime.exe
```

Follow the menu prompts in the terminal.

---

## Troubleshooting

- **Missing `inet_ntop` error**:  
  Make sure you are compiling with MinGW-w64 and linking with `-lws2_32`.  
  If you still get errors, replace `inet_ntop` with `InetNtopA` and include `<Ws2tcpip.h>`.

- **Entry point errors**:  
  Rebuild the executable on your current machine to avoid C++ runtime mismatches.

- **Port in use**:  
  Make sure no other program is using port 8080.

---

## Customization

- **Change WiFi credentials**: Edit `ssid` and `password` in your `.ino` file.
- **Change server IP/port**: Update `server_ip` and `server_port` in the ESP code and server code.
- **Adjust thresholds**: Modify the logic in `mapRainValueToStatus()` in the server and in the ESP code for your sensor's calibration.

---

## File Overview

| File/Folder                                         | Purpose                                                                 |
|-----------------------------------------------------|-------------------------------------------------------------------------|
| waterleveliot.ino                                 | Main ESP32 firmware for WiFi + sensor + TCP client                      |
| simplerainsensor.ino                              | Minimal sensor + LED sketch                                             |
| sensorair.cpp    | ESP8266 + ThingSpeak IoT example                                        |
| mainairfinal.cpp     | Main C++ TCP server with menu, logging, and analysis                    |
| mainair.cpp          | Alternate/older server version                                          |
| `water_monitoring_project/src/mainlogic (REDUNDANT).cpp` | Serial port logger (legacy)                                        |
| serverfile.cpp                                   | Minimal TCP server for demo/testing                                     |
| data.json            | Exported data (JSON)                                                    |
| .vscode                                          | VS Code configuration (build/debug/intellisense)                        |

---

## Summary

- **ESP32/ESP8266** reads sensor, sends data to server.
- **C++ server** logs, analyzes, and exports data.
- **Menu-driven UI** for easy operation and data review.
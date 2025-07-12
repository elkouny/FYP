# Smart Chessboard (Final Year Project)

A fully interactive physical chessboard that detects piece positions, validates moves, highlights legal squares, and allows online/offline gameplay — powered by embedded hardware, RFID, Bluetooth Low Energy (BLE), and CNC-controlled magnet-based movement.

---

## 🚀 Features

- 🧠 **Piece Detection:** RFID-based detection of each individual piece with position mapping.
- 💡 **Move Highlighting:** Legal moves shown via NeoPixel LEDs on the board.
- ♟️ **Rule Enforcement:** Board enforces all rules — including captures, en passant, castling, and promotions.
- 📱 **Mobile App Integration:** BLE connection to a Flutter app for real-time control, move validation, and Lichess support.
- 🌐 **Lichess Sync:** Stream games online, play against bots, or upload completed PGNs for analysis.
- 🤖 **Automated Piece Movement:** CNC machine (H-bot kinematics) with magnet actuator moves pieces under the board.

---

## 🎬 Demo Highlights

### 🧩 Full 1v1 Game  
[![1v1 Game](https://img.youtube.com/vi/lSETz-mvSjk/maxresdefault.jpg)](https://youtu.be/lSETz-mvSjk)

### ⚙️ CNC Movement System  
[![CNC Movement](https://img.youtube.com/vi/WEJAxF2dIqA/maxresdefault.jpg)](https://youtu.be/WEJAxF2dIqA)

### 🛠️ Setup & Validation  
[![Setup & Validation](https://img.youtube.com/vi/VHgs-Omi_UQ/maxresdefault.jpg)](https://youtu.be/VHgs-Omi_UQ)

### 🏃 En Passant  
[![En Passant](https://img.youtube.com/vi/Dw1L1SJ10iw/maxresdefault.jpg)](https://youtu.be/Dw1L1SJ10iw)

### 👑 Castling  
[![Castling](https://img.youtube.com/vi/q60sbwWMyoU/maxresdefault.jpg)](https://youtu.be/q60sbwWMyoU)

---

## 🧠 System Overview

### Hardware

- **ESP32 (Nano):** RFID scanning, LED feedback, BLE communication, CNC interface  
- **RFID Readers (MFRC522):** Under each square, controlled via shift registers  
- **NeoPixels (WS2812):** 64 addressable LEDs for visual feedback  
- **Servo + Magnet:** Engages to pick/place pieces from below  
- **CNC Motion (H-bot):** Arduino Uno running GRBL firmware

### Software

- **Arduino Firmware (C++):** BLE, RFID polling, CNC G-code streaming
- **Flutter App:** BLE control, board UI, Lichess integration, rule enforcement    
- **Lichess API:** PGN import/export, live play against bots

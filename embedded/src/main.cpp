#include <Adafruit_NeoPixel.h>
#include <Arduino.h>
#include <BLE2902.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BiMap.h>
#include <Board.h>
#include <ESP32Servo.h>
#include <MFRC522.h>
#include <SPI.h>
#include <Wire.h>
// RFID
#define RST_PIN 5
#define SS_PIN 10
#define SER 4
#define CLK 3
#define CLR 2
// Neopixel
#define NUM_PIXELS 64
#define DATA_PIN 18 // actually d9 on arduino nano esp32
#define WIDTH 8
#define HEIGHT 8
// Servo
Servo myServo;
const int SERVO_PIN = 8;
// GRBL over UART
HardwareSerial &grbl = Serial1;
const int GRBL_RX = 6;
const int GRBL_TX = 7;
// BLE UUIDs
#define SERVICE_UUID "0000180C-0000-1000-8000-00805F9B34FB"
#define CHARACTERISTIC_UUID "00002A56-0000-1000-8000-00805F9B34FB"
// State
bool deviceConnected = false;
BLECharacteristic *statusChar = nullptr;
MFRC522 mfrc522(SS_PIN, RST_PIN);
Adafruit_NeoPixel strip(NUM_PIXELS, DATA_PIN, NEO_GRB + NEO_KHZ800);
// Board state
const int numReaders = 64;
bool gameReady = false;
bool hasNotifiedReady = false;
bool gameStarted = false;
BiMap<std::string, XYPos> boardState; // byte to chess id
std::string hovering;
// White Pieces
std::set<std::string> whitePawnUIDs = {"1D0BDB5D0D1080", "1D0CDB5D0D1080", "1D0DDB5D0D1080", "1D0EDB5D0D1080", "1D9BDB5D0D1080", "1DA7DA5D0D1080", "1DA8DA5D0D1080", "1DAADA5D0D1080"};
std::set<std::string> whiteRookUIDs = {"1DA0DA5D0D1080", "1DA6DA5D0D1080"};
std::set<std::string> whiteKnightUIDs = {"1DA5DA5D0D1080", "1DA9DA5D0D1080"};
std::set<std::string> whiteBishopUIDs = {"1DA1DA5D0D1080", "1DA4DA5D0D1080"};
std::set<std::string> whiteKingUIDs = {"1DA2DA5D0D1080"};
std::set<std::string> whiteQueenUIDs = {"1DA3DA5D0D1080"};
// Black Pieces
std::set<std::string> blackPawnUIDs = {"1D11DB5D0D1080", "1D12DB5D0D1080", "1D13DB5D0D1080", "1D14DB5D0D1080", "1D17DB5D0D1080", "1D19DB5D0D1080", "1D1ADB5D0D1080", "1D1EDB5D0D1080"};
std::set<std::string> blackRookUIDs = {"1D0FDB5D0D1080", "1D1DDB5D0D1080"};
std::set<std::string> blackKnightUIDs = {"1D10DB5D0D1080", "1D1CDB5D0D1080"};
std::set<std::string> blackBishopUIDs = {"1D15DB5D0D1080", "1D16DB5D0D1080"};
std::set<std::string> blackQueenUIDs = {"1D18DB5D0D1080"};
std::set<std::string> blackKingUIDs = {"1D1BDB5D0D1080"};

// FreeRTOS
#define WRITE_QUEUE_LEN 10
#define WRITE_MSG_LEN 64
QueueHandle_t writeQueue;

struct WriteMessage {
    char msg[WRITE_MSG_LEN];
};

// === Start animation helpers ===
const float CENTER_X = (WIDTH - 1) / 2.0;
const float CENTER_Y = (HEIGHT - 1) / 2.0;

// Precomputed distance of each pixel from the center
float pixelDist[NUM_PIXELS];

// Wave parameters
float waveRadius = 0.0;      // Current radius of the ring
const float speed = 0.1;     // Expansion speed
const float widthBand = 0.4; // Ring thickness

// Map (x, y) to strip index for serpentine wiring
uint16_t XY(uint8_t x, uint8_t y) {
    if (y & 0x01) {
        // Odd rows run backwards
        return y * WIDTH + (WIDTH - 1 - x);
    } else {
        // Even rows run forwards
        return y * WIDTH + x;
    }
}

// Color wheel helper: hue ∈ [0..255]
uint32_t Wheel(byte hue) {
    hue = 255 - hue;
    if (hue < 85) {
        return strip.Color(255 - hue * 3, 0, hue * 3);
    } else if (hue < 170) {
        hue -= 85;
        return strip.Color(0, hue * 3, 255 - hue * 3);
    } else {
        hue -= 170;
        return strip.Color(hue * 3, 255 - hue * 3, 0);
    }
}
void drawRadiatingWaveFrame() {
    strip.clear();
    for (uint16_t i = 0; i < NUM_PIXELS; i++) {
        float d = pixelDist[i];
        if (fabs(d - waveRadius) < widthBand) {
            byte hue = (byte)fmod(waveRadius * 10.0, 255.0);
            strip.setPixelColor(i, Wheel(hue));
        }
    }
    strip.show();
    waveRadius += speed;
}
// === Pin helpers ===
void sendBit(int val) {
    digitalWrite(SER, val);
    digitalWrite(CLK, LOW);
    digitalWrite(CLK, HIGH);
}

void sendByte(uint8_t val, int order) {
    for (int i = 0; i < 8; i++) {
        if (order == MSBFIRST) {
            sendBit((val & 128) != 0);
            val <<= 1;
        } else {
            sendBit((val & 1) != 0);
            val >>= 1;
        }
    }
}

void activateReader(int readerIndex) {
    const int totalRegisters = 8; // 8 shift registers
    uint8_t bytes[totalRegisters] = {0};

    // Set the correct bit in the corresponding register
    int reg = readerIndex / 8;
    int bit = readerIndex % 8;
    bytes[reg] = 1 << bit;

    // Shift out the entire register chain
    for (int i = totalRegisters - 1; i >= 0; i--) {
        sendByte(bytes[i], MSBFIRST);
    }
}

void clearRegisters() {
    digitalWrite(CLR, LOW);
    digitalWrite(CLR, HIGH);
}

std::string uidToString(const MFRC522::Uid &uid) {
    std::string uidStr = "";
    for (byte i = 0; i < uid.size; i++) {
        char hex[3];
        sprintf(hex, "%02X", uid.uidByte[i]); // Format as two-digit hex
        uidStr += hex;
    }
    return uidStr;
}

XYPos readerToXYPos(int readerIndex) {
    int x = readerIndex % 8 + 1;
    int y = readerIndex / 8 + 1;
    return XYPos(x, y);
}

int stringPosToIndex(const std::string &pos) {
    int file = pos[0] - 'a';
    int rank = pos[1] - '1';
    return rank * 8 + file;
}

std::string stringPosToGcode(const std::string &pos, int feedRate = 6000) {
    int file = pos[0] - 'a';
    int rank = pos[1] - '1';
    return "G0 X" + std::to_string(file * 60 + 30) + " Y" + std::to_string(rank * 60 + 30) + " F" + std::to_string(feedRate);
}

void scanBoard() {
    for (int i = 0; i < numReaders; i++) {
        clearRegisters();
        activateReader(i);
        delayMicroseconds(1000);
        mfrc522.PCD_Init();
        mfrc522.PCD_SetAntennaGain(mfrc522.RxGain_max);
        XYPos currentPos = readerToXYPos(i);
        String message;

        byte v = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
        if (!mfrc522.PCD_PerformSelfTest() || v == 0x00 || v == 0xFF) {
            Serial.println("Error at " + currentPos.toString());
            clearRegisters();
            activateReader(i);
            mfrc522.PCD_Init();
            mfrc522.PCD_SetAntennaGain(mfrc522.RxGain_max);
            v = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
        }

        if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) { // is there a piece on this square?
            std::string uid = uidToString(mfrc522.uid);
            if (!boardState.containsUid(uid)) {
                strip.setPixelColor(i, strip.Color(255, 0, 0));
                strip.show();
                while (true) {
                    Serial.println("Error please remove this peice from the square it shouldnt be on the board");
                    bool removed = !mfrc522.PICC_ReadCardSerial() && !mfrc522.PICC_ReadCardSerial();
                    if (removed) {
                        strip.setPixelColor(i, strip.Color(0, 0, 0));
                        strip.show();
                        break;
                    }
                }
            } else if (boardState.containsXYPos(currentPos)) {           // was there a piece on this square before?
                if (boardState.getFromXYPos(currentPos) != uid) {        // did the piece on this square a different uid?
                    String from = boardState.getFromUid(uid).toString(); // attacker's origin
                    String to = currentPos.toString();                   // capture destination
                    String message = "capture:" + from + to;
                    statusChar->setValue(message.c_str());
                    statusChar->notify();
                    Serial.println("Message sent" + message);
                } else {
                    // nothing to do the piece was in the same position
                    if (hovering == uid) {
                        // if the piece was hovering mark as no longer hovering
                        hovering = "";
                        statusChar->setValue("clear");
                        statusChar->notify();
                        Serial.println("Undoing hovering at " + currentPos.toString());
                    }
                }

            } else {
                // move
                message = "move:" + boardState.getFromUid(uid).toString() + currentPos.toString();
                statusChar->setValue(message.c_str());
                statusChar->notify();
                Serial.println("Message sent " + message);
            }
        } else {
            if (boardState.containsXYPos(currentPos) && hovering == "") {
                // hover
                message = "hover:" + currentPos.toString();
                statusChar->setValue(message.c_str());
                statusChar->notify();
                hovering = boardState.getFromXYPos(currentPos);
                Serial.println(message);
            }
        }
    }
}

void resetBoard() {
    myServo.write(0);
    delay(50);
    grbl.println("$H");
    gameReady = false;
    hasNotifiedReady = false;
    gameStarted = false;
    boardState.clear();
    strip.clear();
    strip.show();
    Serial.println("Reseting board state");
}

void initializeBoard() {
    std::set<int> invalidPlacementIndexes;
    Serial.println("Waiting for all 32 pieces to be placed in their correct starting positions.");

    // Light up all valid starting squares in green
    for (int i = 0; i < 16; i++) {
        strip.setPixelColor(i, strip.Color(0, 255, 0));      // Ranks 1 & 2
        strip.setPixelColor(i + 48, strip.Color(0, 255, 0)); // Ranks 7 & 8
    }
    strip.show();

    while (true) {
        bool allPiecesCorrectlyPlaced = boardState.forward.size() == 32 && invalidPlacementIndexes.empty();

        if (allPiecesCorrectlyPlaced) {
            Serial.println("Initial board setup complete and valid!");
            gameReady = true;
            float maxDist = sqrt(CENTER_X * CENTER_X + CENTER_Y * CENTER_Y);
            for (int cycle = 0; cycle < 2; cycle++) {
                waveRadius = 0.0;
                while (waveRadius <= maxDist + widthBand) {
                    drawRadiatingWaveFrame();
                    delay(10);
                }
            }
            strip.clear();
            strip.show();
            break; // Exit the while loop
        }

        // Scan each square on the board
        for (int i = 0; i < numReaders; i++) {
            clearRegisters();
            activateReader(i);
            delayMicroseconds(1000);

            mfrc522.PCD_Init();

            XYPos currentPos = readerToXYPos(i);

            // Check if a piece is present on the current square
            if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
                std::string uid = uidToString(mfrc522.uid);
                bool isPositionValidForPiece = false;

                // --- Validate piece type against its position using the UID sets ---
                int x = int(currentPos.x) - 1; // 0-7
                int y = currentPos.y - 1;      // 0-7

                // White Pawns (Rank 2)
                if (y == 1 && whitePawnUIDs.count(uid)) isPositionValidForPiece = true;
                // White Back Rank (Rank 1)
                else if (y == 0) {
                    if ((x == 0 || x == 7) && whiteRookUIDs.count(uid)) isPositionValidForPiece = true;
                    if ((x == 1 || x == 6) && whiteKnightUIDs.count(uid)) isPositionValidForPiece = true;
                    if ((x == 2 || x == 5) && whiteBishopUIDs.count(uid)) isPositionValidForPiece = true;
                    if (x == 3 && whiteQueenUIDs.count(uid)) isPositionValidForPiece = true;
                    if (x == 4 && whiteKingUIDs.count(uid)) isPositionValidForPiece = true;
                }
                // Black Pawns (Rank 7)
                else if (y == 6 && blackPawnUIDs.count(uid))
                    isPositionValidForPiece = true;
                // Black Back Rank (Rank 8)
                else if (y == 7) {
                    if ((x == 0 || x == 7) && blackRookUIDs.count(uid)) isPositionValidForPiece = true;
                    if ((x == 1 || x == 6) && blackKnightUIDs.count(uid)) isPositionValidForPiece = true;
                    if ((x == 2 || x == 5) && blackBishopUIDs.count(uid)) isPositionValidForPiece = true;
                    if (x == 3 && blackQueenUIDs.count(uid)) isPositionValidForPiece = true;
                    if (x == 4 && blackKingUIDs.count(uid)) isPositionValidForPiece = true;
                }

                // --- Update board state based on validation ---
                if (isPositionValidForPiece) {
                    if (!boardState.containsXYPos(currentPos)) {
                        boardState.insert(uid, currentPos);
                        Serial.println(("Correct piece placed at " + currentPos.toString()).c_str());
                        strip.setPixelColor(i, strip.Color(0, 0, 0)); // Turn off light for correctly placed piece
                        strip.show();
                        invalidPlacementIndexes.erase(i);
                    }
                } else {
                    // Invalid placement: either wrong piece type for the square or not a starting square
                    if (!invalidPlacementIndexes.count(i)) {
                        strip.setPixelColor(i, strip.Color(255, 0, 0)); // Red for error
                        strip.show();
                        Serial.println(("Invalid piece or position at " + currentPos.toString() + ". Please place the correct piece.").c_str());
                        invalidPlacementIndexes.insert(i);
                    }
                }
                mfrc522.PICC_HaltA();
            } else { // No card detected on this square
                bool isStartingSquare = (currentPos.y <= 2 || currentPos.y >= 7);

                // If a piece was previously here, remove it from the board state
                if (boardState.containsXYPos(currentPos)) {
                    Serial.println(("Piece removed from " + boardState.getFromXYPos(currentPos)).c_str());
                    boardState.eraseByXYPos(currentPos);
                }

                // If the square was marked as invalid, clear the red light
                if (invalidPlacementIndexes.count(i)) {
                    invalidPlacementIndexes.erase(i);
                }

                // If it's a starting square that is now empty, turn its light back to green
                if (isStartingSquare) {
                    strip.setPixelColor(i, strip.Color(0, 255, 0));
                } else {
                    strip.setPixelColor(i, strip.Color(0, 0, 0)); // Ensure non-starting squares are off
                }
                strip.show();
            }
        }
    }
}
// === BLE Callbacks ===
class ServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer *pServer) override {
        deviceConnected = true;
        Serial.println("BLE client connected");
    }

    void onDisconnect(BLEServer *pServer) override {
        deviceConnected = false;
        Serial.println("BLE client disconnected");
        resetBoard();
        BLEDevice::startAdvertising(); // Restart advertising
    }
};

class StatusCharCallback : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) override {
        std::string value = pCharacteristic->getValue();
        if (value.length() > 0) {
            Serial.print("BLE received: ");
            if (!gameStarted && value == "start_confirmed") {
                Serial.println("Game start confirmed by app!");
                gameStarted = true;

            } else if (value.rfind("clear_piece", 0) == 0) {
                std::string lights = value.substr(9);
                for (int i = 0; i < lights.size(); i += 2) {
                    int index = stringPosToIndex(lights.substr(i, 2));
                    if (boardState.containsXYPos(readerToXYPos(index))) {
                        strip.setPixelColor(index, strip.Color(0, 0, 255));
                        strip.show();
                    }
                }
            }

            else if (value.rfind("move_cnc:", 0) == 0) {
                myServo.write(0);
                delay(150);
                std::string from = value.substr(9, 2);
                std::string to = value.substr(11, 2);
                grbl.println(stringPosToGcode(from, 120000).c_str());
                delay(50);
                Serial.println("Moving to 'from' position: " + String(from.c_str()));

                // Wait for the 'from' move to complete
                while (true) {
                    grbl.println("?"); // Ask GRBL for its status

                    // Check if there's a response from GRBL
                    if (grbl.available()) {
                        String resp = grbl.readStringUntil('\n');
                        Serial.println("GRBL Status: " + resp);

                        // Correctly check if the response contains "Idle"
                        if (resp.indexOf("Idle") > 0) {
                            delay(1500);
                            Serial.println("Arrived at 'from' position. Engaging magnet.");
                            myServo.write(120); // Engage magnet←
                            delay(250);
                            break; // Exit this loop and proceed to the next move
                        }
                    }
                }

                // --- Move to the 'to' position ---
                grbl.println(stringPosToGcode(to, 2000).c_str());
                delay(50);
                Serial.println("Moving to 'to' position: " + String(to.c_str()));

                // Wait for the 'to' move to complete
                while (true) {
                    grbl.println("?"); // Ask GRBL for its status
                    if (grbl.available()) {
                        String resp = grbl.readStringUntil('\n');
                        Serial.println("GRBL Status: " + resp);

                        // Correctly check if the response contains "Idle"
                        if (resp.indexOf("Idle") > 0) {
                            delay(1500);
                            myServo.write(0); // Disengage magnet
                            delay(150);
                            Serial.println("Arrived at 'to' position. Disengaging magnet.");
                            break; // Exit this loop, the full move is complete
                        }
                    }
                }
            } else if (value.rfind("move_ack:", 0) == 0) {
                std::string from = value.substr(9, 2);
                std::string to = value.substr(11, 2);
                std::string uid = boardState.getFromXYPos(XYPos(from));
                boardState.eraseByXYPos(from);
                boardState.insert(uid, XYPos(to));
                hovering = "";

            } else if (value.rfind("capture_ack:", 0) == 0) {
                std::string move = value.substr(12);
                std::string from = move.substr(0, 2);
                std::string to = move.substr(2, 2);
                std::string uid_captured = boardState.getFromXYPos(XYPos(to));
                std::string uid = boardState.getFromXYPos(XYPos(from));
                boardState.eraseByUid(uid_captured);
                hovering = "";
                boardState.eraseByXYPos(from);
                boardState.insert(uid, XYPos(to));
                Serial.println(("Capture ACK processed: " + from + " -> " + to).c_str());

            } else if (value == "game_ended") {
                resetBoard();
                delay(100);
                statusChar->setValue("connected");
                statusChar->notify();

            } else if (value.rfind("light_on", 0) == 0) {
                Serial.println(value.c_str());
                // std::string ackMessage = "ack:" + value;
                // statusChar->setValue(ackMessage.c_str());
                // statusChar->notify();
                std::string lights = value.substr(9);
                for (int i = 0; i < lights.size(); i += 2) {
                    int index = stringPosToIndex(lights.substr(i, 2));
                    if (i != 0 && boardState.containsXYPos(readerToXYPos(index)))
                        strip.setPixelColor(index, strip.Color(255, 0, 0));
                    else
                        strip.setPixelColor(index, strip.Color(0, 255, 0));
                }
                strip.show();

            } else if (value == "light_off") {
                Serial.println(value.c_str());
                // std::string ackMessage = "ack:" + value;
                // statusChar->setValue(ackMessage.c_str());
                // statusChar->notify();
                strip.clear();
                strip.show();
            } else if (value.rfind("in_check", 0) == 0) {
                int index = stringPosToIndex(value.substr(9, 2));
                strip.setPixelColor(index, strip.Color(255, 0, 0));
                strip.show();
            }
        }
    }
};

void setup() {
    Serial.begin(115200);
    SPI.begin();
    strip.begin();
    strip.show();
    myServo.write(0);

    pinMode(CLK, OUTPUT);
    pinMode(SER, OUTPUT);
    pinMode(CLR, OUTPUT);
    digitalWrite(CLR, HIGH);
    clearRegisters();

    // BLE Init
    BLEDevice::init("SmartChessBoard");
    BLEServer *pServer = BLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks());
    BLEService *pService = pServer->createService(SERVICE_UUID);
    statusChar = pService->createCharacteristic(
        CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_WRITE);
    statusChar->addDescriptor(new BLE2902());
    statusChar->setValue("waiting");
    statusChar->setCallbacks(new StatusCharCallback());
    pService->start();
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(false);
    pAdvertising->setMinPreferred(0x06);
    pAdvertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();
    Serial.println("BLE advertising started");
    // Neopixel Setup
    for (uint8_t y = 0; y < HEIGHT; y++) {
        for (uint8_t x = 0; x < WIDTH; x++) {
            uint16_t i = XY(x, y);
            float dx = x - CENTER_X;
            float dy = y - CENTER_Y;
            pixelDist[i] = sqrt(dx * dx + dy * dy);
        }
    }
    // GRBL Setup
    grbl.begin(115200, SERIAL_8N1, GRBL_RX, GRBL_TX);
    Serial.println("--- ESP32 → GRBL + Servo Ready ---");

    // attach servo (will use LEDC channel under the hood)
    myServo.setPeriodHertz(50); // 50 Hz for most servos
    myServo.attach(SERVO_PIN, 500, 2400);
    grbl.println("$X");
    grbl.println("$H");
}

void loop() {
    if (!deviceConnected) return;
    if (!gameReady) {
        initializeBoard();
    }

    if (gameReady && !hasNotifiedReady) {
        myServo.write(0);
        delay(50);
        grbl.println("$X");
        grbl.println("$H");
        delay(2000);
        statusChar->setValue("ready_to_start");
        statusChar->notify();
        hasNotifiedReady = true;
        Serial.println("Notified app: ready_to_start");
    }

    if (gameStarted) {
        scanBoard();
    }
}

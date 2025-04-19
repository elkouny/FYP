#include <Arduino.h>
#include <BLE2902.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BiMap.h>
#include <Board.h>
#include <MFRC522.h>
#include <SPI.h>
#include <Wire.h>
// RFID + register control
#define RST_PIN 9
#define SS_PIN 10
#define SER 4
#define CLK 3
#define CLR 2

// BLE UUIDs
#define SERVICE_UUID "0000180C-0000-1000-8000-00805F9B34FB"
#define CHARACTERISTIC_UUID "00002A56-0000-1000-8000-00805F9B34FB"

// State
bool deviceConnected = false;
BLECharacteristic *statusChar = nullptr;
MFRC522 mfrc522(SS_PIN, RST_PIN);

// Board state
int state = 0;
const int numReaders = 64;
bool gameReady = false;
bool hasNotifiedReady = false;
bool gameStarted = false;
// byte to chess id
BiMap<std::string, XYPos> boardState;
std::set<std::string> hovering;

// === Pin helpers ===
void sendBit(int val) {
    digitalWrite(SER, val);
    delayMicroseconds(10);
    digitalWrite(CLK, LOW);
    delayMicroseconds(10);
    digitalWrite(CLK, HIGH);
    delayMicroseconds(10);
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
        delayMicroseconds(10);
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
    return std::string(reinterpret_cast<const char *>(uid.uidByte), uid.size);
}

XYPos readerToXYPos(int readerIndex) {
    int x = readerIndex % 8 + 1;
    int y = readerIndex / 8 + 1;
    return XYPos(x, y);
}

void scanBoard() {
    for (int i = 0; i < numReaders; i++) {
        clearRegisters();
        activateReader(i);
        mfrc522.PCD_Init();
        mfrc522.PCD_SetAntennaGain(mfrc522.RxGain_max);
        XYPos currentPos = readerToXYPos(i);
        String message;

        byte v = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
        while (!mfrc522.PCD_PerformSelfTest() || v == 0x00 || v == 0xFF) {
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
                while (true) {
                    Serial.println("Error please remove this peice from the square it shouldnt be on the board");
                    bool read = mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial();
                    if (!read || uid != uidToString(mfrc522.uid)) {
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
                    if (hovering.count(uid)) {
                        // if the piece was hovering mark as no longer hovering
                        hovering.erase(uid);
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
            if (boardState.containsXYPos(currentPos) && !hovering.count(boardState.getFromXYPos(currentPos))) {
                // hover
                message = "hover:" + currentPos.toString();
                statusChar->setValue(message.c_str());
                statusChar->notify();
                hovering.insert(boardState.getFromXYPos(currentPos));
                Serial.println(message);
            }
        }
    }
}

void initializeBoard() {

    Serial.println(" Waiting for all 32 pieces to be placed in valid positions");

    while (true) {

        for (int i = 0; i < numReaders; i++) {
            clearRegisters();
            activateReader(i);
            mfrc522.PCD_Init();
            mfrc522.PCD_SetAntennaGain(mfrc522.RxGain_max);
            XYPos currentPos = readerToXYPos(i);

            byte v = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
            if (!mfrc522.PCD_PerformSelfTest() || v == 0x00 || v == 0xFF) {
                Serial.println("Retrying reader at " + currentPos.toString());
                mfrc522.PCD_DumpVersionToSerial();
                clearRegisters();
                activateReader(i);
                mfrc522.PCD_Init();
                mfrc522.PCD_SetAntennaGain(mfrc522.RxGain_max);
                v = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
            }

            if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
                if (currentPos.y == 1 || currentPos.y == 2 || currentPos.y == 7 || currentPos.y == 8) {
                    std::string uid = uidToString(mfrc522.uid);
                    if (!boardState.containsXYPos(currentPos) || boardState.getFromXYPos(currentPos) != uid) {
                        boardState.insert(uid, currentPos);
                        Serial.println("Piece placed at " + currentPos.toString());
                    }
                } else {
                    Serial.println("Invalid piece at row " + String(currentPos.y) + " — only 1–2 or 7–8 allowed.");
                }
            }
            else{
                if(boardState.containsXYPos(currentPos)){
                    boardState.eraseByXYPos(currentPos);
                }
            }
        }

        Serial.print("Valid pieces placed: ");
        Serial.print(boardState.forward.size());
        Serial.println(" / 32");

        if (boardState.forward.size() == 32) {
            Serial.println("Initial board setup complete and valid!");
            gameReady = true;
            break;
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
        gameReady = false;
        hasNotifiedReady = false;
        gameStarted = false;
        boardState.clear();
        Serial.println("BLE client disconnected");
        BLEDevice::startAdvertising(); // Restart advertising
    }
};

class StatusCharCallback : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) override {
        std::string value = pCharacteristic->getValue();

        if (value.length() > 0) {
            Serial.print("App wrote: ");
            Serial.println(value.c_str());
            if (!gameStarted && value == "start_confirmed") {
                Serial.println("Game start confirmed by app!");
                gameStarted = true;
            }
            if (value.rfind("move_ack:", 0) == 0) {
                std::string from = value.substr(9, 2).c_str();
                std::string to = value.substr(11, 2).c_str();
                Serial.print("Move confirmed by app: ");
                Serial.println(("From: " + from + " To: " + to).c_str());
                std::string uid = boardState.getFromXYPos(XYPos(from));
                boardState.eraseByXYPos(from);
                boardState.insert(uid, XYPos(to));
                hovering.erase(uid);
            }
        }
        if (value.rfind("capture_ack:", 0) == 0) {
            std::string move = value.substr(12); // skip "capture_ack:"
            std::string from = move.substr(0, 2);
            std::string to = move.substr(2, 2);

            std::string uid_captured = boardState.getFromXYPos(XYPos(to));
            std::string uid = boardState.getFromXYPos(XYPos(from));
            boardState.eraseByUid(uid_captured);
            hovering.erase(uid);
            boardState.eraseByXYPos(from);     // remove from old position
            boardState.insert(uid, XYPos(to)); // insert at captured square
            Serial.println(("Capture ACK processed: " + from + " -> " + to).c_str());
        }
    }
};

void setup() {
    Serial.begin(9600);
    SPI.begin();

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
}

void loop() {
    if (!deviceConnected) return;
    if (!gameReady) {
        initializeBoard();
    }

    if (gameReady && !hasNotifiedReady) {
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

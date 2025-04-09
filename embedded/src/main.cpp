#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
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
#define SERVICE_UUID        "0000180C-0000-1000-8000-00805F9B34FB"
#define CHARACTERISTIC_UUID "00002A56-0000-1000-8000-00805F9B34FB"

// State
bool deviceConnected = false;
BLECharacteristic* statusChar = nullptr;
MFRC522 mfrc522(SS_PIN, RST_PIN);

// Board state
int state = 0;
const int numReaders = 64;
bool gameReady = false;
bool hasNotifiedReady = false;
std::unordered_map<XYPos, std::array<byte, 4>> boardState;
Board board;

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
  sendByte(1 << (readerIndex % 8), MSBFIRST);
  readerIndex -= 8;
  while (readerIndex >= 0) {
    sendByte(0, MSBFIRST);
    readerIndex -= 8;
  }
}

void clearRegisters() {
  digitalWrite(CLR, LOW);
  digitalWrite(CLR, HIGH);
}

XYPos readerToXYPos(int readerIndex) {
  int x = readerIndex % 8 + 1;
  int y = readerIndex / 8 + 1;
  return XYPos(x, y);
}

// === BLE Callbacks ===
class ServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) override {
    deviceConnected = true;
    Serial.println("✅ BLE client connected");
  }

  void onDisconnect(BLEServer* pServer) override {
    deviceConnected = false;
    Serial.println("❌ BLE client disconnected");
    BLEDevice::startAdvertising();  // Restart advertising
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
  BLEServer* pServer = BLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks());

  BLEService* pService = pServer->createService(SERVICE_UUID);
  statusChar = pService->createCharacteristic(
    CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
  );
  statusChar->addDescriptor(new BLE2902());
  statusChar->setValue("waiting");
  pService->start();

  BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();

  Serial.println("✅ BLE advertising started");
}

void loop() {
  if (!deviceConnected) return;

  for (int i = 0; i < numReaders; i++) {
    clearRegisters();
    activateReader(i);
    mfrc522.PCD_Init();
    mfrc522.PCD_SetAntennaGain(mfrc522.RxGain_max);

    byte v = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
    while (!mfrc522.PCD_PerformSelfTest() || v == 0x00 || v == 0xFF) {
      Serial.print("error waiting");
      v = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
    }

    XYPos currentPos = readerToXYPos(i);
    if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
      std::array<byte, 4> uid;
      std::copy(mfrc522.uid.uidByte, mfrc522.uid.uidByte + 4, uid.begin());
      auto existing = boardState.find(currentPos);
      if (existing == boardState.end() || existing->second != uid) {
        Serial.println("Piece placed at " + currentPos.toString());
        boardState[currentPos] = uid;
      }
    } else {
      if (boardState.count(currentPos)) {
        Serial.println("Piece removed at " + currentPos.toString());
        boardState.erase(currentPos);
      }
    }
  }

  if (!gameReady && boardState.size() == 32) {
    gameReady = true;
    Serial.println("All 32 pieces detected.");
  }


  if (gameReady && !hasNotifiedReady) {
    statusChar->setValue("ready_to_start");
    statusChar->notify();
    hasNotifiedReady = true;
    Serial.println("📤 Notified app: ready_to_start");
  }
}

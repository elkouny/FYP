#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <Arduino.h>
#define RST_PIN 9 // Reset pin
// #define DEBUG
#define UNUSED_PIN UINT8_MAX
#define SS_PIN 10 // Slave select pin
#define SER 4     // serial data to the shift register
#define CLK 3     // Shift Register Clock
#define CLR 2     // Clear the registers active low
unsigned long totalTime = 0;
unsigned long startTime;
unsigned long iterationCount = 0;
MFRC522 mfrc522(SS_PIN, RST_PIN);

// Send bit to the storage register
void sendBit(int val)
{
  digitalWrite(SER, val);
  digitalWrite(CLK, LOW);
  digitalWrite(CLK, HIGH);
}

void printBinary(uint8_t value)
{
  for (int i = 7; i >= 0; i--)
  { // Start from the most significant bit
    Serial.print((value & (1 << i)) ? '1' : '0');
  }
  Serial.println(); // Move to the next line after printing
}

// Send Byte to the storage register
void sendByte(uint8_t val, int order)
{

  for (int i = 0; i < 8; i++)
  {
    if (order == MSBFIRST)
    {
      sendBit((val & 128) != 0);
      val = val << 1;
    }
    else
    {
      sendBit((val & (1)) != 0);
      val = val >> 1;
    }
  }
}

void activateReader(int readerIndex)
{
  sendByte(1 << (readerIndex % 8), MSBFIRST);
  readerIndex -= 8;
  while (readerIndex >= 0)
  {
    sendByte(0, MSBFIRST);
    readerIndex -= 8;
  }
}

void clearRegisters()
{
  digitalWrite(CLR, LOW);
  delayMicroseconds(10);
  digitalWrite(CLR, HIGH);
  delayMicroseconds(10);
}

void setup()
{
  Serial.begin(9600); // Initialize serial communication
  pinMode(CLK, OUTPUT);
  pinMode(SER, OUTPUT);
  pinMode(CLR, OUTPUT);
  digitalWrite(CLR, HIGH);
  clearRegisters();
  while (!Serial)
    ; // Wait for serial connection
  // mfrc522.PCD_Init();
  SPI.begin();

  Serial.print("Ready to scan ! ");
}

int state = 0;
int numReaders = 64;
int val = 1;
void loop()
{
#ifdef DEBUG
  sendByte((val << state), MSBFIRST);
  delayMicroseconds(10);
  Serial.println("\n--------------------------------------");
  Serial.print("Current reader is : ");
  Serial.println(state);
  Serial.println("Press SPACEBAR to continue...");

  // Wait for spacebar input
  while (Serial.available() == 0)
    ; // Wait for input

  char input = Serial.read();
  if (input != ' ')
    return; // Ignore input unless it's a space (' ')

  Serial.println("Resuming...");
  mfrc522.PCD_Init();
  delay(10);
  Serial.println(F("*****************************"));
  Serial.println(F("MFRC522 Digital self test"));
  Serial.println(F("*****************************"));
  mfrc522.PCD_DumpVersionToSerial(); // Show version of PCD - MFRC522 Card Reader
  Serial.println(F("-----------------------------"));
  Serial.println(F("Only known versions supported"));
  Serial.println(F("-----------------------------"));
  Serial.println(F("Performing test..."));
  bool result = mfrc522.PCD_PerformSelfTest(); // perform the test
  Serial.println(F("-----------------------------"));
  Serial.print(F("Result: "));
  if (result)
    Serial.println(F("OK"));
  else
  {
    Serial.println(F("DEFECT or UNKNOWN"));
    clearRegisters();
    delayMicroseconds(10);
    sendByte((val << state), MSBFIRST);
    delayMicroseconds(10);
  }
  Serial.println();
  mfrc522.PCD_Init();
  delay(10);
#endif
  startTime = millis();
  clearRegisters();
  activateReader(state);
  // sendByte(1,MSBFIRST);
  delayMicroseconds(100);
  mfrc522.PCD_Init();
  delayMicroseconds(100);
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial())
  {
    Serial.print("Reader Index : ");
    Serial.println(state);
    Serial.print(" Card UID: ");
    for (byte i = 0; i < mfrc522.uid.size; i++)
    {
      Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "); // This is for fomatting eg 5 --> 05;
      Serial.print(mfrc522.uid.uidByte[i], HEX);
    }
    Serial.println();
  }
  state = (state + 1) % numReaders;

  totalTime += (millis() - startTime);
  iterationCount++;

  if (iterationCount >= 16)
  {
    unsigned long avgTime = totalTime / iterationCount;
    Serial.print("Average iteration time: ");
    Serial.print(avgTime);
    Serial.println(" ms");
    totalTime = 0;      // Reset total time
    iterationCount = 0; // Reset iteration count
  }
}

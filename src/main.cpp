#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#define RST_PIN 9 // Reset pin
#define SS_PIN 10 // Slave select pin
#define SER 4     // serial data to the shift register
#define RCLK 3    // Register Clock , this sends the data of the shift registers to the output on falling edge
#define SRCLK 2   // Shift Register Clock
unsigned long totalTime = 0;
unsigned long startTime;
unsigned long iterationCount = 0;
MFRC522 mfrc522(SS_PIN, RST_PIN); // Create MFRC522 instance

// Send bit to the storage register
void sendBit(int val)
{
  digitalWrite(SER, val);
  delayMicroseconds(10);
  digitalWrite(SRCLK, LOW);
  delayMicroseconds(10);
  digitalWrite(SRCLK, HIGH);
  delayMicroseconds(10);
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

void sendToRegisterOutput()
{
  digitalWrite(RCLK, LOW);
  delayMicroseconds(10);
  digitalWrite(RCLK, HIGH);
  delayMicroseconds(10);
}

void clearRegisters()
{
  sendByte(0b00000000, MSBFIRST);
  sendToRegisterOutput();
}

void setup()
{
  Serial.begin(9600); // Initialize serial communication
  pinMode(RCLK, OUTPUT);
  pinMode(SRCLK, OUTPUT);
  pinMode(SER, OUTPUT);
  sendByte(0, MSBFIRST); // Clear Registers
  sendToRegisterOutput();
  while (!Serial)
    ;                 // Wait for serial connection
  SPI.begin();        // Init SPI bus
  mfrc522.PCD_Init(); // Init MFRC522
  Serial.print("Ready to scan ! ");
}

uint8_t state = 0;
void loop()
{
  startTime = millis(); // Record the start time of the iteration
  // Look for new cards
  sendByte((1 << (state + 3)), MSBFIRST); // active HIGH
  sendToRegisterOutput();
  mfrc522.PCD_Init();
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
  }

  // Update total time and iteration count

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
  // Halt PICC
  state = (state + 1) % 4;
}

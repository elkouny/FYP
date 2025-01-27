#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#define RST_PIN 9 // Reset pin
#define SS_PIN 10 // Slave select pin
#define SRCLK 8   // Shift Register Clock
#define RCLK 7   // Register Clock , this sends the data of the shift registers to the output on falling edge
#define SER 6     // serial data to the shift register




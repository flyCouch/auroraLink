/* * PROJECT:  Aurora-Link v164
 * STRATEGY: 9600 BPS / Hamming(7,4) / XMODEM Binary
 * FILE:     auroraLink164.ino
 * COMPILED: Mar 03, 2026 | 19:45:00
 */

#include <Arduino.h>
#include "pico/multicore.h"
#include "pico/util/queue.h"

const uint32_t TARGET_BPS = 9600; 
const uint32_t BIT_TIME_US = 1000000 / TARGET_BPS;

const int LASER_PIN = 2;
const int SENSOR_PIN = 26; 

const int THRESH_HIGH = 750;
const int THRESH_LOW  = 250;
bool beamState = false; 

queue_t txQueue, rxQueue;

// --- HAMMING(7,4) ENGINE ---
uint8_t encodeHamming(uint8_t d) {
  bool d1 = (d >> 3) & 0x01; bool d2 = (d >> 2) & 0x01;
  bool d3 = (d >> 1) & 0x01; bool d4 = (d >> 0) & 0x01;
  bool p1 = d1 ^ d2 ^ d4; 
  bool p2 = d1 ^ d3 ^ d4; 
  bool p3 = d2 ^ d3 ^ d4;
  return (p1 << 6) | (p2 << 5) | (d1 << 4) | (p3 << 3) | (d2 << 2) | (d3 << 1) | d4;
}

uint8_t decodeHamming(uint8_t f) {
  bool p1 = (f >> 6) & 0x01; bool p2 = (f >> 5) & 0x01;
  bool d1 = (f >> 4) & 0x01; bool p3 = (f >> 3) & 0x01;
  bool d2 = (f >> 2) & 0x01; bool d3 = (f >> 1) & 0x01;
  bool d4 = (f >> 0) & 0x01;
  bool s1 = p1 ^ d1 ^ d2 ^ d4; 
  bool s2 = p2 ^ d1 ^ d3 ^ d4; 
  bool s3 = p3 ^ d2 ^ d3 ^ d4;
  int errorBit = (s3 << 2) | (s2 << 1) | s1;
  if (errorBit != 0) {
    switch(errorBit) {
      case 3: d1 = !d1; break;
      case 5: d2 = !d2; break; 
      case 6: d3 = !d3; break; 
      case 7: d4 = !d4; break;
    }
  }
  return (d1 << 3) | (d2 << 2) | (d3 << 1) | d4;
}

// --- CORE 1: LASER HARDWARE TIMING ---
void core1_entry() {
  while (1) {
    uint8_t txFrame;
    if (queue_try_remove(&txQueue, &txFrame)) {
      digitalWrite(LASER_PIN, LOW); // Start
      delayMicroseconds(BIT_TIME_US);
      for (int i = 6; i >= 0; i--) {
        digitalWrite(LASER_PIN, (txFrame >> i) & 0x01 ? LOW : HIGH);
        delayMicroseconds(BIT_TIME_US);
      }
      digitalWrite(LASER_PIN, HIGH); // Stop
      delayMicroseconds(BIT_TIME_US * 2);
    }

    int val = analogRead(SENSOR_PIN);
    if (val > THRESH_HIGH) beamState = true;
    else if (val < THRESH_LOW) beamState = false;

    if (!beamState) { 
      delayMicroseconds(BIT_TIME_US + (BIT_TIME_US / 2));
      uint8_t rxFrame = 0;
      for (int i = 6; i >= 0; i--) {
        int v = analogRead(SENSOR_PIN);
        if (v < THRESH_LOW) rxFrame |= (1 << i);
        delayMicroseconds(BIT_TIME_US);
      }
      queue_try_add(&rxQueue, &rxFrame);
    }
  }
}

// --- CORE 0: INTERFACE ---
void setup() {
  Serial.begin(115200);
  
  // Custom Project Header
  Serial.println(F("AURORA-LINK v181"));
  Serial.print(F("DATE: ")); Serial.println(F(__DATE__));
  Serial.print(F("TIME: ")); Serial.println(F(__TIME__));
  Serial.print(F("FILE: ")); Serial.println(F(__FILE__));

  pinMode(LASER_PIN, OUTPUT);
  digitalWrite(LASER_PIN, HIGH);

  queue_init(&txQueue, sizeof(uint8_t), 256);
  queue_init(&rxQueue, sizeof(uint8_t), 256);
  multicore_launch_core1(core1_entry);
}

void loop() {
  // PC to Laser
  if (Serial.available()) {
    uint8_t in = Serial.read();
    uint8_t h = encodeHamming(in >> 4);
    uint8_t l = encodeHamming(in & 0x0F);
    queue_add_blocking(&txQueue, &h);
    queue_add_blocking(&txQueue, &l);
  }

  // Laser to PC
  static uint8_t highNibble = 0;
  static bool waitingForLow = false;
  uint8_t rxFrame;
  if (queue_try_remove(&rxQueue, &rxFrame)) {
    uint8_t decoded = decodeHamming(rxFrame);
    if (!waitingForLow) {
      highNibble = (decoded << 4);
      waitingForLow = true;
    } else {
      Serial.write(highNibble | (decoded & 0x0F));
      waitingForLow = false;
    }
  }
}

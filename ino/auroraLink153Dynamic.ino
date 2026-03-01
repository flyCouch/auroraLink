/* * PROJECT:  Aurora-Link v152 (BBS Optimized)
 * STRATEGY: 9600 BPS / Hamming(7,4) / XON-XOFF Flow Control
 * FILE:     auroraLink152_Dynamic.ino
 * COMPILED: Mar 01, 2026 | 15:20:00
 */

#include <Arduino.h>
#include "pico/multicore.h"
#include "pico/util/queue.h"

// --- SETTINGS ---
const uint32_t TARGET_BPS = 9600; 
const uint32_t BIT_TIME_US = 1000000 / TARGET_BPS; 
const uint32_t FRAME_TIME_US = BIT_TIME_US * 10; // 1 Start + 7 Hamming + 2 Stop bits

// --- HARDWARE PINS ---
const int LASER_PIN = 2;   
const int SENSOR_PIN = 26; 

// --- HYSTERESIS ---
const int THRESH_HIGH = 750;
const int THRESH_LOW  = 250;
bool beamState = false;

queue_t txQueue, rxQueue;

// ========================================================================
// HAMMING(7,4) ENGINE
// ========================================================================
uint8_t encodeHamming(uint8_t d) {
  bool d1 = (d >> 3) & 0x01;
  bool d2 = (d >> 2) & 0x01;
  bool d3 = (d >> 1) & 0x01;
  bool d4 = (d >> 0) & 0x01;
  bool p1 = d1 ^ d2 ^ d4;
  bool p2 = d1 ^ d3 ^ d4;
  bool p3 = d2 ^ d3 ^ d4;
  return (p1 << 6) | (p2 << 5) | (d1 << 4) | (p3 << 3) | (d2 << 2) | (d3 << 1) | d4;
}

uint8_t decodeHamming(uint8_t f) {
  bool p1 = (f >> 6) & 0x01;
  bool p2 = (f >> 5) & 0x01;
  bool d1 = (f >> 4) & 0x01;
  bool p3 = (f >> 3) & 0x01;
  bool d2 = (f >> 2) & 0x01;
  bool d3 = (f >> 1) & 0x01;
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

// ========================================================================
// CORE 1: LASER TIMING
// ========================================================================
bool readLaser() {
  int val = analogRead(SENSOR_PIN);
  if (val > THRESH_HIGH) beamState = true;
  else if (val < THRESH_LOW) beamState = false;
  return beamState;
}

void core1_entry() {
  while (1) {
    // TX: Pull from Queue and Pulse Laser
    uint8_t txFrame;
    if (queue_try_remove(&txQueue, &txFrame)) {
      digitalWrite(LASER_PIN, LOW); // Start Bit (Laser ON for Active-Low)
      delayMicroseconds(BIT_TIME_US);
      for (int i = 6; i >= 0; i--) {
        digitalWrite(LASER_PIN, (txFrame >> i) & 0x01 ? LOW : HIGH);
        delayMicroseconds(BIT_TIME_US);
      }
      digitalWrite(LASER_PIN, HIGH); // Stop Bits (Laser OFF)
      delayMicroseconds(BIT_TIME_US * 2);
    }

    // RX: Sample Laser and Push to Queue
    if (!readLaser()) { 
      delayMicroseconds(BIT_TIME_US + (BIT_TIME_US / 2));
      uint8_t rxFrame = 0;
      for (int i = 6; i >= 0; i--) {
        if (!readLaser()) rxFrame |= (1 << i);
        delayMicroseconds(BIT_TIME_US);
      }
      queue_try_add(&rxQueue, &rxFrame);
    }
  }
}

// ========================================================================
// CORE 0: INTERFACE & FLOW CONTROL
// ========================================================================
void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000); 

  Serial.println(F("----------------------------------------"));
  Serial.println(F("PROJECT:  AURORA-LINK v152 (BBS-DYNAMIC)"));
  Serial.print(F("FILE:     ")); Serial.println(F(__FILE__));
  Serial.print(F("COMPILED: ")); Serial.print(F(__DATE__));
  Serial.print(F(" | "));      Serial.println(F(__TIME__));
  Serial.println(F("MODE:     XON/XOFF Flow Control Enabled"));
  Serial.println(F("----------------------------------------\n"));

  pinMode(LASER_PIN, OUTPUT);
  digitalWrite(LASER_PIN, HIGH); // Safe Start

  queue_init(&txQueue, sizeof(uint8_t), 64);
  queue_init(&rxQueue, sizeof(uint8_t), 64);
  multicore_launch_core1(core1_entry);
}

void loop() {
  // XON/XOFF Flow Control Logic
  // 19 = XOFF (Stop), 17 = XON (Go)
  if (queue_get_level(&txQueue) > 50) {
    Serial.write(19); 
  } else if (queue_get_level(&txQueue) < 10) {
    Serial.write(17);
  }

  // Serial (BBS) to Laser
  if (Serial.available()) {
    uint8_t in = Serial.read();
    uint8_t h = encodeHamming(in >> 4);
    uint8_t l = encodeHamming(in & 0x0F);
    queue_try_add(&txQueue, &h);
    queue_try_add(&txQueue, &l);
  }

  // Laser to Serial (BBS)
  uint8_t rxFrame;
  static uint8_t nibBuf = 0;
  static bool second = false;
  if (queue_try_remove(&rxQueue, &rxFrame)) {
    uint8_t d = decodeHamming(rxFrame);
    if (second) {
      Serial.write(nibBuf | (d & 0x0F));
      second = false;
    } else {
      nibBuf = (d << 4);
      second = true;
    }
  }
}

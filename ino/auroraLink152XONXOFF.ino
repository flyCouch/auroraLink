/* * PROJECT:  Aurora-Link v130 (Pico-Zero Edition)
 * STRATEGY: Full-Duplex / Hamming(7,4) / Dual-Core ARQ
 * HARDWARE: RP2040-Zero, 9V Inverted Laser (Active-Low), Reverse-Bias PD
 * FILE:     AuroraLink_v130_9600_Hysteresis.ino
 */

#include <Arduino.h>
#include "pico/multicore.h"
#include "pico/util/queue.h"

// --- SETTINGS ---
const uint32_t TARGET_BPS = 9600; 
const uint32_t BIT_TIME_US = 1000000 / TARGET_BPS; // ~104us

// --- HARDWARE PINS ---
const int LASER_PIN = 2;   
const int SENSOR_PIN = 26; 

// --- HYSTERESIS SETTINGS ---
// On a 10-bit ADC (0-1023), these create a "Safe Zone" in the middle
const int THRESH_HIGH = 750; // Must cross this to be "ON"
const int THRESH_LOW  = 250; // Must drop below this to be "OFF"
bool beamState = false;      // Internal state tracker

// --- IPC QUEUES ---
queue_t txQueue; 
queue_t rxQueue; 

// ========================================================================
// HELPER: SANE HYSTERESIS SENSING
// ========================================================================
bool readLaser() {
  int val = analogRead(SENSOR_PIN);
  if (val > THRESH_HIGH) beamState = true;
  else if (val < THRESH_LOW) beamState = false;
  return beamState;
}

// ========================================================================
// HAMMING(7,4) UTILITIES
// ========================================================================
uint8_t encodeHamming(uint8_t data) {
  bool d1 = (data >> 0) & 1; bool d2 = (data >> 1) & 1;
  bool d3 = (data >> 2) & 1; bool d4 = (data >> 3) & 1;
  bool p1 = d1 ^ d2 ^ d4;
  bool p2 = d1 ^ d3 ^ d4;
  bool p3 = d2 ^ d3 ^ d4;
  return (p1 << 6) | (p2 << 5) | (d1 << 4) | (p3 << 3) | (d2 << 2) | (d3 << 1) | (d4 << 0);
}

uint8_t decodeHamming(uint8_t received) {
  bool p1 = (received >> 6) & 1; bool p2 = (received >> 5) & 1;
  bool d1 = (received >> 4) & 1; bool p3 = (received >> 3) & 1;
  bool d2 = (received >> 2) & 1; bool d3 = (received >> 1) & 1;
  bool d4 = (received >> 0) & 1;
  uint8_t s1 = p1 ^ d1 ^ d2 ^ d4;
  uint8_t s2 = p2 ^ d1 ^ d3 ^ d4;
  uint8_t s3 = p3 ^ d2 ^ d3 ^ d4;
  uint8_t syndrome = (s3 << 2) | (s2 << 1) | s1;
  if (syndrome != 0) {
    received ^= (1 << (7 - syndrome)); 
  }
  return ( (received >> 0) & 1 ) | ((received >> 1) & 1) << 1 | 
         ( (received >> 2) & 1 ) << 2 | ((received >> 4) & 1) << 3;
}

// ========================================================================
// CORE 1: THE BLINKER (TIMING ENGINE)
// ========================================================================
void core1_entry() {
  while (1) {
    // 1. TRANSMIT
    uint8_t frameOut;
    if (queue_try_remove(&txQueue, &frameOut)) {
      digitalWrite(LASER_PIN, LOW); // Start Bit
      delayMicroseconds(BIT_TIME_US);
      for (int i = 6; i >= 0; i--) {
        digitalWrite(LASER_PIN, ((frameOut >> i) & 1) ? LOW : HIGH);
        delayMicroseconds(BIT_TIME_US);
      }
      digitalWrite(LASER_PIN, HIGH); // Stop Bit
      delayMicroseconds(BIT_TIME_US);
    }

    // 2. RECEIVE (With Hysteresis and Center-Sampling)
    if (readLaser() == true) { 
      // Start Bit Detected
      delayMicroseconds(BIT_TIME_US + (BIT_TIME_US / 2)); 
      uint8_t frameIn = 0;
      for (int i = 6; i >= 0; i--) {
        if (readLaser() == true) frameIn |= (1 << i);
        delayMicroseconds(BIT_TIME_US);
      }
      queue_try_add(&rxQueue, &frameIn);
      // Wait for beam to clear (Idle is Laser OFF / Low signal)
      while(readLaser() == true) { tight_loop_contents(); }
    }
  }
}

// ========================================================================
// CORE 0: THE THINKER (BBS & LOGIC)
// ========================================================================
void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000); 

  // --- COMPULSORY PROJECT HEADER ---
  Serial.println(F("----------------------------------------"));
  Serial.println(F("PROJECT:  AURORA-LINK v151"));
  Serial.print(F("FILE:     ")); Serial.println(F(__FILE__));
  Serial.print(F("COMPILED: ")); Serial.print(F(__DATE__));
  Serial.print(F(" | "));      Serial.println(F(__TIME__));
  Serial.print(F("SPEED:    ")); Serial.print(TARGET_BPS); Serial.println(F(" BPS"));
  Serial.println(F("MODE:     Full-Duplex / 9600 / Hysteresis"));
  Serial.println(F("----------------------------------------\n"));

  pinMode(LASER_PIN, OUTPUT);
  digitalWrite(LASER_PIN, HIGH); // Safe Start

  queue_init(&txQueue, sizeof(uint8_t), 64);
  queue_init(&rxQueue, sizeof(uint8_t), 64);
  multicore_launch_core1(core1_entry);
}

void loop() {
// Check if buffer is getting full (threshold of 50 in a 64-byte queue)
  if (queue_get_level(&txQueue) > 50) {
    Serial.write(19); // Send XOFF to tell PuTTY to PAUSE
  } 
  else if (queue_get_level(&txQueue) < 10) {
    Serial.write(17); // Send XON to tell PuTTY to RESUME
  }

  // Serial to Laser
  if (Serial.available()) {
    uint8_t in = Serial.read();
    uint8_t h = encodeHamming(in >> 4);
    uint8_t l = encodeHamming(in & 0x0F);
    queue_try_add(&txQueue, &h);
    queue_try_add(&txQueue, &l);
  }

  // Laser to Serial
  uint8_t frameIn;
  static uint8_t nibBuffer = 0;
  static bool secondNib = false;
  if (queue_try_remove(&rxQueue, &frameIn)) {
    uint8_t decoded = decodeHamming(frameIn);
    if (!secondNib) {
      nibBuffer = (decoded << 4);
      secondNib = true;
    } else {
      nibBuffer |= (decoded & 0x0F);
      Serial.write(nibBuffer);
      secondNib = false;
    }
  }
}

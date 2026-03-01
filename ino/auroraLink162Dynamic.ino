/* * PROJECT:  Aurora-Link v161 (Hybrid Audio/BBS)
 * STRATEGY: 9600 BPS / Dynamic Frame Timing / Auto-Squelch / XON-XOFF
 * HARDWARE: RP2040-Zero, 5V Level Shifter, 2N2222 Speaker Amp, Electret Mic
 * FILE:     auroraLink161_Dynamic.ino
 * COMPILED: Mar 01, 2026 | 15:25:00
 */

#include <Arduino.h>
#include "pico/multicore.h"
#include "pico/util/queue.h"

// --- SETTINGS ---
const uint32_t TARGET_BPS = 9600; 
const uint32_t BIT_TIME_US = 1000000 / TARGET_BPS; 
const uint32_t FRAME_TIME_US = BIT_TIME_US * 10; // 1 Start + 7 Hamming + 2 Stop bits

// --- HARDWARE PINS ---
const int LASER_PIN = 2;    // To 5V Level Shifter
const int SENSOR_PIN = 26;  // Photodiode
const int MIC_PIN = 27;     // Electret Mic (with 3.3V Bias)
const int SPEAKER_PIN = 10; // 2N2222 Base (with 1k Resistor)
const int PTT_PIN = 3;      // Push-To-Talk (Active LOW)
const int STATUS_LED = 16;  // Onboard RGB

// --- HYSTERESIS & SQUELCH ---
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
// CORE 1: LASER TIMING (CRITICAL)
// ========================================================================
bool readLaser() {
  int val = analogRead(SENSOR_PIN);
  if (val > THRESH_HIGH) beamState = true;
  else if (val < THRESH_LOW) beamState = false;
  return beamState;
}

void core1_entry() {
  while (1) {
    // TX: Laser Output (Inverted Logic for 2N2222 Buffer)
    uint8_t txFrame;
    if (queue_try_remove(&txQueue, &txFrame)) {
      digitalWrite(LASER_PIN, LOW); // Start Bit (Active Low)
      delayMicroseconds(BIT_TIME_US);
      for (int i = 6; i >= 0; i--) {
        digitalWrite(LASER_PIN, (txFrame >> i) & 0x01 ? LOW : HIGH);
        delayMicroseconds(BIT_TIME_US);
      }
      digitalWrite(LASER_PIN, HIGH); // Stop Bits
      delayMicroseconds(BIT_TIME_US * 2);
    }

    // RX: Photodiode Input
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
// CORE 0: INTERFACE & AUTO-SQUELCH
// ========================================================================
void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000); 

  Serial.println(F("----------------------------------------"));
  Serial.println(F("PROJECT:  AURORA-LINK v161 (HYBRID)"));
  Serial.print(F("FILE:     ")); Serial.println(F(__FILE__));
  Serial.print(F("COMPILED: ")); Serial.print(F(__DATE__));
  Serial.print(F(" | "));      Serial.println(F(__TIME__));
  Serial.println(F("MODE:     PTT Voice / XON-XOFF BBS"));
  Serial.println(F("----------------------------------------\n"));

  pinMode(LASER_PIN, OUTPUT);
  pinMode(SPEAKER_PIN, OUTPUT);
  pinMode(PTT_PIN, INPUT_PULLUP);
  digitalWrite(LASER_PIN, HIGH); 

  queue_init(&txQueue, sizeof(uint8_t), 64);
  queue_init(&rxQueue, sizeof(uint8_t), 64);
  multicore_launch_core1(core1_entry);
}

void loop() {
  bool pttPressed = (digitalRead(PTT_PIN) == LOW);

  // --- TRANSMIT ---
  if (pttPressed) {
    uint16_t sample = analogRead(MIC_PIN);
    uint8_t crushed = map(sample, 0, 1023, 0, 15);
    uint8_t frame = encodeHamming(crushed);
    if(queue_try_add(&txQueue, &frame)) {
      delayMicroseconds(FRAME_TIME_US); // Dynamic Sync
    }
  } else {
    // Flow Control
    if (queue_get_level(&txQueue) > 50) Serial.write(19); // XOFF
    else if (queue_get_level(&txQueue) < 10) Serial.write(17); // XON

    if (Serial.available()) {
      uint8_t in = Serial.read();
      uint8_t h = encodeHamming(in >> 4);
      uint8_t l = encodeHamming(in & 0x0F);
      queue_try_add(&txQueue, &h);
      queue_try_add(&txQueue, &l);
    }
  }

  // --- RECEIVE & SQUELCH ---
  uint8_t rxFrame;
  static uint8_t nibBuf = 0;
  static bool secondNib = false;
  static uint32_t lastVoiceTime = 0;

  if (queue_try_remove(&rxQueue, &rxFrame)) {
    uint8_t d = decodeHamming(rxFrame);
    
    // Auto-Squelch Decision:
    // If not in middle of BBS byte, treat as possible Voice
    if (!secondNib) {
      // Is this voice? Check timing or if PTT-like stream
      // 4-bit voice maps to PWM output
      analogWrite(SPEAKER_PIN, map(d, 0, 15, 0, 255));
      lastVoiceTime = millis();
      
      // Also treat as first nibble for BBS just in case
      nibBuf = (d << 4);
      secondNib = true;
    } else {
      // This is the second half of a BBS character
      analogWrite(SPEAKER_PIN, 0); // Squelch
      Serial.write(nibBuf | (d & 0x0F));
      secondNib = false;
    }
  } else if (millis() - lastVoiceTime > 50) {
    analogWrite(SPEAKER_PIN, 0); // Hard Squelch if silence
  }
}

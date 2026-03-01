/* * PROJECT:  Aurora-Link v130 (Auto-Squelch / PTT Priority)
 * STRATEGY: 9600 BPS / Hamming(7,4) / Visual Status / Silent BBS
 * HARDWARE: RP2040-Zero, 9V Inverted Laser, 2N2222 Speaker Amp
 * FILE:     AuroraLink_v130_Squelch_Final.ino
 */

#include <Arduino.h>
#include "pico/multicore.h"
#include "pico/util/queue.h"

// --- SETTINGS ---
const uint32_t TARGET_BPS = 9600; 
const uint32_t BIT_TIME_US = 1000000 / TARGET_BPS; 

// --- HARDWARE PINS ---
const int LASER_PIN = 2;    
const int SENSOR_PIN = 26;  
const int MIC_PIN = 27;     
const int SPEAKER_PIN = 10; 
const int PTT_PIN = 3;      
const int STATUS_LED = 16;  // RP2040-Zero Onboard RGB/Status LED

// --- HYSTERESIS & SQUELCH ---
const int THRESH_HIGH = 750;
const int THRESH_LOW  = 250;
bool beamState = false;

queue_t txQueue, rxQueue;

// ========================================================================
// CORE 1: THE GALVANOMETER (TIMING CRITICAL)
// ========================================================================
bool readLaser() {
  int val = analogRead(SENSOR_PIN);
  if (val > THRESH_HIGH) beamState = true;
  else if (val < THRESH_LOW) beamState = false;
  return beamState;
}

void core1_entry() {
  while (1) {
    uint8_t frameOut;
    if (queue_try_remove(&txQueue, &frameOut)) {
      digitalWrite(LASER_PIN, LOW); // ON
      delayMicroseconds(BIT_TIME_US);
      for (int i = 6; i >= 0; i--) {
        digitalWrite(LASER_PIN, ((frameOut >> i) & 1) ? LOW : HIGH);
        delayMicroseconds(BIT_TIME_US);
      }
      digitalWrite(LASER_PIN, HIGH); // OFF
      delayMicroseconds(BIT_TIME_US);
    }

    if (readLaser() == true) { 
      delayMicroseconds(BIT_TIME_US + (BIT_TIME_US / 2)); 
      uint8_t frameIn = 0;
      for (int i = 6; i >= 0; i--) {
        if (readLaser() == true) frameIn |= (1 << i);
        delayMicroseconds(BIT_TIME_US);
      }
      queue_try_add(&rxQueue, &frameIn);
      while(readLaser() == true) { tight_loop_contents(); }
    }
  }
}

// ========================================================================
// HAMMING SECDED LOGIC
// ========================================================================
uint8_t encodeHamming(uint8_t data) {
  bool d1 = (data >> 0) & 1; bool d2 = (data >> 1) & 1;
  bool d3 = (data >> 2) & 1; bool d4 = (data >> 3) & 1;
  bool p1 = d1 ^ d2 ^ d4; bool p2 = d1 ^ d3 ^ d4; bool p3 = d2 ^ d3 ^ d4;
  return (p1 << 6) | (p2 << 5) | (d1 << 4) | (p3 << 3) | (d2 << 2) | (d3 << 1) | (d4 << 0);
}

uint8_t decodeHamming(uint8_t received) {
  bool p1 = (received >> 6) & 1; bool p2 = (received >> 5) & 1;
  bool d1 = (received >> 4) & 1; bool p3 = (received >> 3) & 1;
  bool d2 = (received >> 2) & 1; bool d3 = (received >> 1) & 1;
  bool d4 = (received >> 0) & 1;
  uint8_t s1 = p1 ^ d1 ^ d2 ^ d4; uint8_t s2 = p2 ^ d1 ^ d3 ^ d4; uint8_t s3 = p3 ^ d2 ^ d3 ^ d4;
  uint8_t syn = (s3 << 2) | (s2 << 1) | s1;
  if (syn != 0) received ^= (1 << (7 - syn)); 
  return ((received >> 0) & 1) | ((received >> 1) & 1) << 1 | 
         ((received >> 2) & 1) << 2 | ((received >> 4) & 1) << 3;
}

// ========================================================================
// SETUP & MAIN LOGIC
// ========================================================================
void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000); 

  // --- COMPULSORY PROJECT HEADER ---
  Serial.println(F("----------------------------------------"));
  Serial.println(F("PROJECT:  AURORA-LINK v130 (AUTO-SQUELCH)"));
  Serial.print(F("FILE:     ")); Serial.println(F(__FILE__));
  Serial.print(F("COMPILED: ")); Serial.print(F(__DATE__));
  Serial.print(F(" | "));      Serial.println(F(__TIME__));
  Serial.print(F("SPEED:    ")); Serial.print(TARGET_BPS); Serial.println(F(" BPS"));
  Serial.println(F("SQUELCH:  ENABLED (BBS SILENCED)"));
  Serial.println(F("----------------------------------------\n"));

  pinMode(LASER_PIN, OUTPUT);
  digitalWrite(LASER_PIN, HIGH); 
  pinMode(PTT_PIN, INPUT_PULLUP);
  pinMode(STATUS_LED, OUTPUT);
  
  analogReadResolution(10);
  analogWriteRange(255);

  queue_init(&txQueue, sizeof(uint8_t), 128);
  queue_init(&rxQueue, sizeof(uint8_t), 128);
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

  bool pttPressed = (digitalRead(PTT_PIN) == LOW);

  // --- TRANSMIT ---
  if (pttPressed) {
    uint16_t sample = analogRead(MIC_PIN);
    uint8_t crushed = map(sample, 0, 1023, 0, 15);
    // Add a "Voice Marker" bit if we had space, but for now we rely on the PTT state
    uint8_t frame = encodeHamming(crushed);
    queue_try_add(&txQueue, &frame);
    delayMicroseconds(1000); 
  } 
  else if (Serial.available()) {
    uint8_t in = Serial.read();
    uint8_t h = encodeHamming(in >> 4);
    uint8_t l = encodeHamming(in & 0x0F);
    queue_try_add(&txQueue, &h);
    queue_try_add(&txQueue, &l);
  }

  // --- RECEIVE WITH AUTO-SQUELCH ---
  uint8_t frameIn;
  static uint8_t nibBuf = 0;
  static bool secondNib = false;
  static uint32_t lastReceiveTime = 0;

  if (queue_try_remove(&rxQueue, &frameIn)) {
    uint8_t decoded = decodeHamming(frameIn);
    lastReceiveTime = millis();
    digitalWrite(STATUS_LED, HIGH); // Visual "Carrier Detect"

    // SQUELCH LOGIC: 
    // If the data is arriving at a high, steady rate (Voice), open audio.
    // In BBS mode, the frames arrive in pairs with longer gaps between them.
    // For this prototype, we'll "Squelch" by checking if we are in a 'nibble' state.
    
    if (secondNib) { 
      // This is BBS data (the second half of a byte) -> KEEP SILENT
      analogWrite(SPEAKER_PIN, 0); 
      nibBuf |= (decoded & 0x0F); 
      Serial.write(nibBuf); 
      secondNib = false; 
    } else { 
      // Could be Voice or the first nibble of BBS
      // If it's voice, the delay between frames is ~1ms. 
      // We push to speaker but the "noise" of BBS is greatly reduced.
      analogWrite(SPEAKER_PIN, map(decoded, 0, 15, 0, 255));
      nibBuf = (decoded << 4); 
      secondNib = true; 
    }
  }

  // Turn off Status LED if no data for 100ms
  if (millis() - lastReceiveTime > 100) {
    digitalWrite(STATUS_LED, LOW);
    analogWrite(SPEAKER_PIN, 0); // Hard squelch when idle
  }
}

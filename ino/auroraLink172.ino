/* * PROJECT:  LASER DARKNET v163 (Selective Squelch Full Duplex)
 * STRATEGY: 9600 BPS / NPN Inverter Logic / PTT Data-Halt / Remote Voice-Gate
 * FILE:     v163_LaserDarknet_FullDuplex.ino
 * COMPILED: Mar 02, 2026 | 17:15:00
 * CIRCUIT:  GP2 -> 1k -> NPN Base | Collector -> 1k Pullup to 5V & Laser TTL
 */

#include <Arduino.h>
#include "pico/multicore.h"
#include "pico/util/queue.h"

// --- SETTINGS ---
const uint32_t TARGET_BPS = 9600; 
const uint32_t BIT_TIME_US = 1000000 / TARGET_BPS; 
const uint32_t FRAME_TIME_US = BIT_TIME_US * 10; 

// --- HARDWARE PINS ---
const int LASER_PIN = 2;    
const int SENSOR_PIN = 26;  
const int MIC_PIN = 27;     
const int SPEAKER_PIN = 10; 
const int PTT_PIN = 3;      

// --- HYSTERESIS ---
const int THRESH_HIGH = 750;
const int THRESH_LOW  = 250;
bool beamState = false;

queue_t txQueue, rxQueue;

// ========================================================================
// HAMMING(7,4) ENGINE
// ========================================================================
uint8_t encodeHamming(uint8_t d) {
  bool d1 = (d >> 3) & 0x01; bool d2 = (d >> 2) & 0x01;
  bool d3 = (d >> 1) & 0x01; bool d4 = (d >> 0) & 0x01;
  bool p1 = d1 ^ d2 ^ d4; bool p2 = d1 ^ d3 ^ d4; bool p3 = d2 ^ d3 ^ d4;
  return (p1 << 6) | (p2 << 5) | (d1 << 4) | (p3 << 3) | (d2 << 2) | (d3 << 1) | d4;
}

uint8_t decodeHamming(uint8_t f) {
  bool p1 = (f >> 6) & 0x01; bool p2 = (f >> 5) & 0x01; bool d1 = (f >> 4) & 0x01;
  bool p3 = (f >> 3) & 0x01; bool d2 = (f >> 2) & 0x01; bool d3 = (f >> 1) & 0x01;
  bool d4 = (f >> 0) & 0x01;
  bool s1 = p1 ^ d1 ^ d2 ^ d4; bool s2 = p2 ^ d1 ^ d3 ^ d4; bool s3 = p3 ^ d2 ^ d3 ^ d4;
  int err = (s3 << 2) | (s2 << 1) | s1;
  if (err == 3) d1 = !d1; else if (err == 5) d2 = !d2;
  else if (err == 6) d3 = !d3; else if (err == 7) d4 = !d4;
  return (d1 << 3) | (d2 << 2) | (d3 << 1) | d4;
}

// ========================================================================
// CORE 1: PHYSICAL LAYER (Inverted for NPN Buffer)
// ========================================================================
void core1_entry() {
  while (1) {
    uint8_t txF;
    if (queue_try_remove(&txQueue, &txF)) {
      // INVERTED LOGIC: Pico HIGH = NPN Collector LOW = Laser ON
      digitalWrite(LASER_PIN, HIGH); delayMicroseconds(BIT_TIME_US); 
      for (int i = 6; i >= 0; i--) {
        digitalWrite(LASER_PIN, (txF >> i) & 0x01 ? HIGH : LOW);
        delayMicroseconds(BIT_TIME_US);
      }
      digitalWrite(LASER_PIN, LOW); delayMicroseconds(BIT_TIME_US * 2);
    }

    int v = analogRead(SENSOR_PIN);
    if (v > THRESH_HIGH) beamState = true; else if (v < THRESH_LOW) beamState = false;
    
    if (!beamState) { // Start bit detected (Laser hit sensor)
      delayMicroseconds(BIT_TIME_US + (BIT_TIME_US / 2));
      uint8_t rxF = 0;
      for (int i = 6; i >= 0; i--) {
        if (analogRead(SENSOR_PIN) < THRESH_LOW) rxF |= (1 << i);
        delayMicroseconds(BIT_TIME_US);
      }
      queue_try_add(&rxQueue, &rxF);
    }
  }
}

// ========================================================================
// CORE 0: LOGIC & SELECTIVE SQUELCH
// ========================================================================
void setup() {
  Serial.begin(115200);
  pinMode(LASER_PIN, OUTPUT); 
  digitalWrite(LASER_PIN, LOW); // SAFE START: Pico LOW = NPN OFF = Laser OFF
  
  pinMode(SPEAKER_PIN, OUTPUT); 
  pinMode(PTT_PIN, INPUT_PULLUP);

  Serial.println(F("========================================"));
  Serial.println(F("   Welcome to the LASER DARKNET v163    "));
  Serial.println(F("   Status: NPN Inverter Logic Active    "));
  Serial.print(F("FILE:     ")); Serial.println(F(__FILE__));
  Serial.print(F("COMPILED: ")); Serial.print(F(__DATE__));
  Serial.print(F(" | "));      Serial.println(F(__TIME__));
  Serial.println(F("========================================\n"));

  queue_init(&txQueue, sizeof(uint8_t), 128);
  queue_init(&rxQueue, sizeof(uint8_t), 128);
  multicore_launch_core1(core1_entry);
}

void loop() {
  // 1. TRANSMIT: Local Voice overrides Local Data
  if (digitalRead(PTT_PIN) == LOW) {
    uint8_t crushed = map(analogRead(MIC_PIN), 0, 1023, 0, 15);
    uint8_t f = encodeHamming(crushed);
    if(queue_try_add(&txQueue, &f)) delayMicroseconds(FRAME_TIME_US);
  } else {
    if (Serial.available()) {
      uint8_t in = Serial.read();
      uint8_t h = encodeHamming(in >> 4); uint8_t l = encodeHamming(in & 0x0F);
      queue_try_add(&txQueue, &h); queue_try_add(&txQueue, &l);
    }
    // Flow Control for BBS
    if (queue_get_level(&txQueue) > 100) Serial.write(19); 
    else if (queue_get_level(&txQueue) < 20) Serial.write(17);
  }

  // 2. RECEIVE: Selective Squelch (Gating)
  uint8_t rxF;
  static uint8_t nib = 0; static bool sec = false;
  static uint32_t lastF = 0; static int vConf = 0;

  if (queue_try_remove(&rxQueue, &rxF)) {
    uint8_t d = decodeHamming(rxF);
    uint32_t now = millis();
    
    // Voice frames are constant; BBS frames are bursty.
    if ((now - lastF) < 5) vConf++; else vConf = 0; 
    lastF = now;

    // Gate Logic: Only play to speaker if stream is steady (Voice)
    if (vConf > 6) analogWrite(SPEAKER_PIN, map(d, 0, 15, 0, 255));
    else analogWrite(SPEAKER_PIN, 0); 

    // Always attempt to send data to Serial (BBS)
    if (!sec) { nib = (d << 4); sec = true; }
    else { Serial.write(nib | (d & 0x0F)); sec = false; }
  } else if (millis() - lastF > 40) {
    analogWrite(SPEAKER_PIN, 0); // Hard Squelch
    vConf = 0;
  }
}

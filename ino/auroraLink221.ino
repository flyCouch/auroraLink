/* * PROJECT:  Aurora-Link v200-Unified
 * STRATEGY: Hamming(7,4) / Analog Hysteresis / Dual-Switch
 * FILE:     auroraLink200_DualSwitch.ino
 * DATE:     Mar 04, 2026
 */

// --- CONFIGURATION ---
const uint32_t BPS = 9600; 
const uint32_t BIT_TIME = 1000000 / BPS;

// --- HARDWARE PINS ---
const int LASER_PIN   = 2;  
const int SENSOR_PIN  = A0;  
const int TARGET_SW   = 3; // BOSS: Laser Override
const int REPORT_SW   = 4; // WORKER: Signal Feedback

// --- HYSTERESIS SETTINGS ---
const int THRESH_HIGH = 650;
const int THRESH_LOW  = 350;
bool beamState = false;     

// --- HAMMING(7,4) ENGINE ---
uint8_t encodeHamming(uint8_t d) {
  bool d1 = (d >> 3) & 0x01; bool d2 = (d >> 2) & 0x01;
  bool d3 = (d >> 1) & 0x01; bool d4 = (d >> 0) & 0x01;
  bool p1 = d1 ^ d2 ^ d4; bool p2 = d1 ^ d3 ^ d4; bool p3 = d2 ^ d3 ^ d4;
  return (p1 << 6) | (p2 << 5) | (d1 << 4) | (p3 << 3) | (d2 << 2) | (d3 << 1) | d4;
}

uint8_t decodeHamming(uint8_t f) {
  bool p1 = (f >> 6) & 0x01; bool p2 = (f >> 5) & 0x01;
  bool d1 = (f >> 4) & 0x01; bool p3 = (f >> 3) & 0x01;
  bool d2 = (f >> 2) & 0x01; bool d3 = (f >> 1) & 0x01;
  bool d4 = (f >> 0) & 0x01;
  bool s1 = p1 ^ d1 ^ d2 ^ d4; bool s2 = p2 ^ d1 ^ d3 ^ d4; bool s3 = p3 ^ d2 ^ d3 ^ d4;
  int err = (s3 << 2) | (s2 << 1) | s1;
  if (err != 0) {
    switch(err) {
      case 3: d1 = !d1; break; case 5: d2 = !d2; break; 
      case 6: d3 = !d3; break; case 7: d4 = !d4; break;
    }
  }
  return (d1 << 3) | (d2 << 2) | (d3 << 1) | d4;
}

// --- SENSOR LOGIC ---
bool checkLaser() {
  int val = analogRead(SENSOR_PIN);
  
  // REPORTING OVERRIDE (Switch 4)
  if (digitalRead(REPORT_SW) == LOW) {
    Serial.print(F("ANALOG_VAL: ")); Serial.print(val);
    if (val > THRESH_HIGH) Serial.println(F(" [SIGNAL ACTIVE]"));
    else if (val < THRESH_LOW) Serial.println(F(" [DARK/NOISE]"));
    else Serial.println(F(" [HYSTERESIS DEADZONE]"));
    delay(100); 
  }

  if (val > THRESH_HIGH) beamState = true;
  else if (val < THRESH_LOW) beamState = false;
  return beamState;
}

void setup() {
  Serial.begin(115200); 
  
  // MANDATORY PROJECT HEADER
  Serial.println(F("\n========================================"));
  Serial.println(F("PROJECT:  Aurora Link v200-Unified"));
  Serial.print(F("FILE:     ")); Serial.println(F(__FILE__));
  Serial.print(F("DATE:     ")); Serial.println(F(__DATE__));
  Serial.print(F("TIME:     ")); Serial.println(F(__TIME__));
  Serial.println(F("STRATEGY: Boss/Worker Dual-Switch Mode"));
  Serial.println(F("========================================\n"));

  pinMode(LASER_PIN, OUTPUT);
  pinMode(TARGET_SW, INPUT_PULLUP);
  pinMode(REPORT_SW, INPUT_PULLUP);
  
  digitalWrite(LASER_PIN, HIGH); // Inverted Off
}

void loop() {
// 1. INDEPENDENT REPORTING (Switch 4)
  if (digitalRead(REPORT_SW) == LOW) {
    int val = analogRead(SENSOR_PIN);
    Serial.print(F("ANALOG: ")); Serial.println(val);
    delay(100);
  }

  // 2. INDEPENDENT TARGETING (Switch 3)
  if (digitalRead(TARGET_SW) == LOW) {
    digitalWrite(LASER_PIN, LOW); // Force Laser ON
    return; // Block other TX/RX while aiming
  }

  // 2. TRANSMITTER (Boss)
  if (Serial.available()) {
    uint8_t b = Serial.read();
    uint8_t nibs[2] = { (uint8_t)(b >> 4), (uint8_t)(b & 0x0F) };
    
    // Preamble for optical stabilization
    digitalWrite(LASER_PIN, LOW); 
    delay(10); 

    for (int n = 0; n < 2; n++) {
      uint8_t frame = encodeHamming(nibs[n]);
      digitalWrite(LASER_PIN, LOW); // Start Bit
      delayMicroseconds(BIT_TIME);
      for (int i = 6; i >= 0; i--) {
        digitalWrite(LASER_PIN, (frame >> i) & 0x01 ? LOW : HIGH);
        delayMicroseconds(BIT_TIME);
      }
      digitalWrite(LASER_PIN, HIGH); // Stop Bit
      delayMicroseconds(BIT_TIME);
    }
  }

  // 3. RECEIVER (Worker)
  if (checkLaser()) { 
    delayMicroseconds(BIT_TIME + (BIT_TIME / 2));
    static uint8_t nBuf = 0;
    static bool isSecond = false;
    uint8_t rxFrame = 0;
    for (int i = 6; i >= 0; i--) {
      if (checkLaser()) rxFrame |= (1 << i);
      delayMicroseconds(BIT_TIME);
    }
    uint8_t d = decodeHamming(rxFrame);
    if (isSecond) {
      Serial.write(nBuf | (d & 0x0F));
      isSecond = false;
    } else {
      nBuf = (d << 4);
      isSecond = true;
    }
  }
}

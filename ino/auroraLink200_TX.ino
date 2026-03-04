/* * PROJECT:  Aurora-Link v195-TX (Final Targeting)
 * STRATEGY: Idle-Low / Single Wake-up / Hamming / Targeting Toggle
 * FILE:     auroraLink195_TX.ino
 * COMPILED: Mar 04, 2026 | 08:05:00
 */

uint32_t userBaud = 9600; 
const int LASER_PIN  = 2;
const int TARGET_PIN = 3; // Ground this pin to LOCK LASER ON for aiming

uint32_t bitTime;
unsigned long lastByteTime = 0;
bool laserAwake = false;

uint8_t encodeHamming(uint8_t d) {
  bool d1 = (d >> 3) & 0x01; bool d2 = (d >> 2) & 0x01;
  bool d3 = (d >> 1) & 0x01; bool d4 = (d >> 0) & 0x01;
  bool p1 = d1 ^ d2 ^ d4; bool p2 = d1 ^ d3 ^ d4; bool p3 = d2 ^ d3 ^ d4;
  return (p1 << 6) | (p2 << 5) | (d1 << 4) | (p3 << 3) | (d2 << 2) | (d3 << 1) | d4;
}

void sendFrame(uint8_t frame) {
  digitalWrite(LASER_PIN, LOW); // Start Bit (Laser OFF)
  delayMicroseconds(bitTime);
  for (int i = 6; i >= 0; i--) {
    digitalWrite(LASER_PIN, (frame >> i) & 0x01 ? HIGH : LOW);
    delayMicroseconds(bitTime);
  }
  digitalWrite(LASER_PIN, HIGH); // Stop Bit (Laser ON)
  delayMicroseconds(bitTime);
}

void setup() {
  bitTime = 1000000 / userBaud;
  Serial.begin(115200); 
  
  pinMode(LASER_PIN, OUTPUT);
  pinMode(TARGET_PIN, INPUT_PULLUP); // High by default
  digitalWrite(LASER_PIN, LOW);      // Start Dark

  // --- Mandatory Project Header ---
  Serial.println(F("========================================"));
  Serial.println(F("PROJECT: AURORA-LINK v195-TX"));
  Serial.print(F("FILE:    ")); Serial.println(F(__FILE__));
  Serial.print(F("DATE:    ")); Serial.println(F(__DATE__));
  Serial.print(F("TIME:    ")); Serial.println(F(__TIME__));
  Serial.print(F("BAUD:    ")); Serial.println(userBaud);
  Serial.println(F("TARGET:  Ground Pin 3 to Force Laser ON"));
  Serial.println(F("========================================"));
}

void loop() {
  // 1. TARGETING OVERRIDE: If Pin 3 is grounded, lock laser ON and exit loop
  if (digitalRead(TARGET_PIN) == LOW) {
    digitalWrite(LASER_PIN, HIGH);
    return; // Skip data processing while aiming
  }

  // 2. DATA PROCESSING
  if (Serial.available()) {
    if (!laserAwake) {
      digitalWrite(LASER_PIN, HIGH); // Wake Up
      delay(10); // Preamble for 740k circuit stability
      laserAwake = true;
    }
    
    uint8_t in = Serial.read();
    sendFrame(encodeHamming(in >> 4));
    sendFrame(encodeHamming(in & 0x0F));
    lastByteTime = millis();
  }

  // 3. AUTO-SLEEP: Return to Stealth Mode after 50ms of silence
  if (laserAwake && (millis() - lastByteTime > 50)) {
    digitalWrite(LASER_PIN, LOW);
    laserAwake = false;
  }
}

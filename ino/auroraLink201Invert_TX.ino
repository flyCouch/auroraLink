/* * PROJECT:  Aurora-Link v200-TX (Inverted TTL)
 * STRATEGY: Idle-High (Laser Off) / Active-Low (Laser On)
 * FILE:     auroraLink200_TX_Inverted.ino
 * COMPILED: Mar 04, 2026 | 10:15:00
 */

uint32_t userBaud = 9600; 
const int LASER_PIN  = 2;
const int TARGET_PIN = 3; 

uint32_t bitTime;
unsigned long lastByteTime = 0;
bool laserAwake = false;

// Hamming stays the same - we flip the physics at the digitalWrite level
uint8_t encodeHamming(uint8_t d) {
  bool d1 = (d >> 3) & 0x01; bool d2 = (d >> 2) & 0x01;
  bool d3 = (d >> 1) & 0x01; bool d4 = (d >> 0) & 0x01;
  bool p1 = d1 ^ d2 ^ d4; bool p2 = d1 ^ d3 ^ d4; bool p3 = d2 ^ d3 ^ d4;
  return (p1 << 6) | (p2 << 5) | (d1 << 4) | (p3 << 3) | (d2 << 2) | (d3 << 1) | d4;
}

void sendFrame(uint8_t frame) {
  // START BIT: Inverted logic means Laser ON = LOW
  digitalWrite(LASER_PIN, LOW); 
  delayMicroseconds(bitTime);

  for (int i = 6; i >= 0; i--) {
    // If bit is 1 (Light), Pin is LOW. If bit is 0 (Dark), Pin is HIGH.
    digitalWrite(LASER_PIN, (frame >> i) & 0x01 ? LOW : HIGH);
    delayMicroseconds(bitTime);
  }
  
  // STOP BIT: Laser OFF = HIGH
  digitalWrite(LASER_PIN, HIGH); 
  delayMicroseconds(bitTime);
}

void setup() {
  bitTime = 1000000 / userBaud;
  Serial.begin(115200); 
  
  pinMode(LASER_PIN, OUTPUT);
  pinMode(TARGET_PIN, INPUT_PULLUP); 
  
  // INITIAL STATE: HIGH = Laser OFF for your driver
  digitalWrite(LASER_PIN, HIGH);      

  Serial.println(F("========================================"));
  Serial.println(F("PROJECT: AURORA-LINK v200-TX (INVERTED)"));
  Serial.print(F("FILE:    ")); Serial.println(F(__FILE__));
  Serial.print(F("DATE:    ")); Serial.println(F(__DATE__));
  Serial.print(F("TIME:    ")); Serial.println(F(__TIME__));
  Serial.println(F("LOGIC:   HIGH=OFF, LOW=ON (TTL Inverted)"));
  Serial.println(F("========================================"));
}

void loop() {
  // 1. TARGETING OVERRIDE: Ground Pin 3 to Force Laser ON (LOW)
  if (digitalRead(TARGET_PIN) == LOW) {
    digitalWrite(LASER_PIN, LOW); 
    return; 
  }

  if (Serial.available()) {
    if (!laserAwake) {
      digitalWrite(LASER_PIN, LOW); // Wake Up (Laser ON)
      delay(10); 
      laserAwake = true;
    }
    
    uint8_t in = Serial.read();
    sendFrame(encodeHamming(in >> 4));
    sendFrame(encodeHamming(in & 0x0F));
    lastByteTime = millis();
  }

  // 3. AUTO-SLEEP: Return to HIGH (Laser OFF)
  if (laserAwake && (millis() - lastByteTime > 50)) {
    digitalWrite(LASER_PIN, HIGH);
    laserAwake = false;
  }
}

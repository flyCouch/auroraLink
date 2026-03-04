/* * PROJECT:  Aurora-Link v200-RX (Master Bridge)
 * FILE:     auroraLink200_RX.ino
 * COMPILED: Mar 04, 2026 | 17:30:00
 */

#include <SoftwareSerial.h>

SoftwareSerial linkToTX(8, 9); // RX on 8 (NC), TX on 9 -> TX Nano D0
uint32_t userBaud = 9600; 
const int SENSOR_PIN = A0;
const int REPORT_PIN = 3; 

const int THRESH_HIGH = 650;
const int THRESH_LOW  = 350;
bool beamState = false;      

uint32_t bitTime;
uint8_t  highNibble = 0;
bool     waitingForLow = false;
bool     linkActive = false;

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

bool getSample() {
  int val = analogRead(SENSOR_PIN);
  if (digitalRead(REPORT_PIN) == LOW) {
    Serial.print(F("SIG: ")); Serial.println(val);
    delay(40);
  }
  if (val > THRESH_HIGH) beamState = true;
  else if (val < THRESH_LOW) beamState = false;
  return beamState;
}

void setup() {
  bitTime = 1000000 / userBaud;
  Serial.begin(115200);   // PC Port
  linkToTX.begin(9600);   // Bridge Port
  pinMode(REPORT_PIN, INPUT_PULLUP);

  Serial.println(F("========================================"));
  Serial.println(F("PROJECT: AURORA-LINK v200-RX (BRIDGE)"));
  Serial.println(F("__FILE__ __DATE__ __TIME__"));
  Serial.print(F("FILE: ")); Serial.println(F(__FILE__));
  Serial.print(F("DATE: ")); Serial.println(F(__DATE__));
  Serial.print(F("TIME: ")); Serial.println(F(__TIME__));
  Serial.println(F("========================================"));
}

void loop() {
  if (Serial.available()) linkToTX.write(Serial.read()); 

  if (!linkActive) {
    if (getSample()) { 
      linkActive = true;
      while(getSample()) { delayMicroseconds(10); }
    }
  } else {
    delayMicroseconds(bitTime + (bitTime / 2));
    uint8_t frame = 0;
    for (int i = 6; i >= 0; i--) {
      if (getSample()) frame |= (1 << i);
      delayMicroseconds(bitTime);
    }
    uint8_t decoded = decodeHamming(frame);
    if (!waitingForLow) { highNibble = (decoded << 4); waitingForLow = true; }
    else { Serial.write(highNibble | (decoded & 0x0F)); waitingForLow = false; }
    
    unsigned long timeout = micros();
    bool sawLight = false;
    while (micros() - timeout < (bitTime * 5)) {
      if (getSample()) { sawLight = true; break; }
    }
    if (!sawLight) linkActive = false;
  }
}

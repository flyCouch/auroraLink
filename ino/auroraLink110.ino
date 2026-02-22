/*
 * Aurora-Link v4.7.0 - "The Clean Conversation Hub"
 * - Logic: Dedicated paths for CdS (North), Visible (East), and IR (South)
 * - Feature: Adaptive Origin Tagging (Only prints header on origin change)
 * - Speeds: 110 (A0), 1200 (D3), 9600 (D4) 
 */

// Speed Settings
const long baudCdS     = 110;   
const long baudVisible = 1200;  
const long baudStealth = 9600;  

// Tracking Variables
long currentActiveBaud = 110; 
int lastOrigin = -1; // 0=North, 1=East, 2=South

// Pin Mapping
const int ledPin       = 2;    
const int invLedPin    = 5;    
const int signalLedPin = 13;   
const int sensorCdS    = A0;   
const int sensorVisible = 3;   
const int sensorIR      = 4;   

float referenceLevel = 512.0;  
const float deltaPercent = 0.05; 

void setup() {
  Serial.begin(115200); 

  // --- START INTERNAL ID SNIPPET ---
  Serial.println(F("\n========================================"));
  Serial.print(F("FILE:     ")); Serial.println(F(__FILE__));
  Serial.print(F("COMPILED: ")); Serial.print(F(__DATE__));
  Serial.print(F(" | "));      Serial.println(F(__TIME__));
  Serial.println(F("========================================\n"));
  // --- END INTERNAL ID SNIPPET ---

  pinMode(ledPin, OUTPUT);
  pinMode(invLedPin, OUTPUT);
  pinMode(signalLedPin, OUTPUT);
  pinMode(sensorVisible, INPUT_PULLUP);
  pinMode(sensorIR, INPUT_PULLUP);

  digitalWrite(ledPin, LOW); 
  digitalWrite(invLedPin, HIGH); 

// --- ROBUST CALIBRATION (THE 100-POINT AVERAGE) ---
  digitalWrite(signalLedPin, HIGH);
  Serial.print(F("SYSTEM: Calibrating North Link..."));

  float sum = 0;
  int samples = 100;

  for (int i = 0; i < samples; i++) {
    sum += (float)analogRead(sensorCdS);
    delay(10); // 100 samples * 10ms = 1000ms (Exactly 1 Second)
  }
  
  referenceLevel = sum / (float)samples; 
  
  Serial.println(F(" DONE."));
  digitalWrite(signalLedPin, LOW);
}

void loop() {
  // --- RECEIVER POLLING ---
  
  // 1. Check NORTH (CdS)
  int cdsVal = analogRead(sensorCdS);
  float change = ((float)cdsVal - referenceLevel) / referenceLevel;
  if (change > deltaPercent) { 
    if (lastOrigin != 0) {
      Serial.print(F("\n[NORTH]: "));
      lastOrigin = 0;
    }
    currentActiveBaud = baudCdS;
    processStream(sensorCdS, true, baudCdS);
  } 
  
  // 2. Check EAST (Visible)
  else if (digitalRead(sensorVisible) == LOW) { 
    if (lastOrigin != 1) {
      Serial.print(F("\n[EAST]:  "));
      lastOrigin = 1;
    }
    currentActiveBaud = baudVisible;
    processStream(sensorVisible, false, baudVisible);
  }
  
  // 3. Check SOUTH (IR)
  else if (digitalRead(sensorIR) == LOW) { 
    if (lastOrigin != 2) {
      Serial.print(F("\n[SOUTH]: "));
      lastOrigin = 2;
    }
    currentActiveBaud = baudStealth;
    processStream(sensorIR, false, baudStealth);
  }

  // --- ADAPTIVE TRANSMITTER ---
  if (Serial.available() > 0) {
    char outChar = (char)Serial.read();
    transmitByte(outChar, currentActiveBaud);
  }
}

void processStream(int pin, bool isAnalog, long speed) {
  digitalWrite(signalLedPin, HIGH);
  char c = receiveByte(pin, isAnalog, speed);
  if (c > 0) Serial.write(c);
  digitalWrite(signalLedPin, LOW);
}

void transmitByte(char c, long speed) {
  long bitTime = 1000000 / speed; 
  digitalWrite(ledPin, HIGH); 
  digitalWrite(invLedPin, LOW); 
  delayMicroseconds(bitTime);

  for (int i = 0; i < 8; i++) {
    bool b = bitRead(c, i);
    digitalWrite(ledPin, b);
    digitalWrite(invLedPin, !b); 
    delayMicroseconds(bitTime);
  }
  
  digitalWrite(ledPin, LOW); 
  digitalWrite(invLedPin, HIGH); 
  delayMicroseconds(bitTime);
}

char receiveByte(int pin, bool isAnalog, long speed) {
  long bitTime = 1000000 / speed;
  delayMicroseconds(bitTime * 1.5); 
  
  char result = 0;
  for (int i = 0; i < 8; i++) {
    bool detected = false;
    if (isAnalog) {
      float diff = ((float)analogRead(pin) - referenceLevel) / referenceLevel;
      detected = (diff > (deltaPercent / 2));
    } else {
      detected = (digitalRead(pin) == LOW); 
    }
    if (detected) bitSet(result, i);
    delayMicroseconds(bitTime);
  }
  return result;
}

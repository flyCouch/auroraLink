/*
 * Aurora-Link v3.9.7 - "The Constant Listener"
 * - Fix: Receiver is now ALWAYS active, even during Sighting Mode.
 * - Logic: A 20% light jump triggers an immediate decode attempt.
 * - Timing: Optimized for 10 Baud (100ms bits).

 EDIT: I found setting deltaPercent to .06 works a lot better.
 
 */

long baudRate = 10; 

const int ledPin = 2;           
const int signalLedPin = 13;    
const int sensorPin = A0;       

bool sightingMode = true;       
unsigned long lastActivity = 0;
const unsigned long timeout = 15000; 

float referenceLevel = 512.0;    
const float deltaPercent = 0.20; 

void setup() {
  Serial.begin(baudRate);
  pinMode(ledPin, OUTPUT);
  pinMode(signalLedPin, OUTPUT);
  
  delay(1000); 
  referenceLevel = (float)analogRead(sensorPin);
  digitalWrite(ledPin, HIGH); 
  lastActivity = millis();
}

void loop() {
  unsigned long currentTime = millis();
  int sensorValue = analogRead(sensorPin);
  float change = ((float)sensorValue - referenceLevel) / referenceLevel;
  
  // 1. CONSTANT RECEIVER
  // If we see a jump > 20%, we assume a Start Bit is happening.
  if (change > deltaPercent) {
    digitalWrite(signalLedPin, HIGH);
    
    // If we were in Sighting Mode, drop it immediately so we don't blind ourselves
    if (sightingMode) {
      sightingMode = false;
      digitalWrite(ledPin, LOW); 
      // We don't delay here; we need to jump straight into decoding the bits
    }
    
    char receivedChar = receiveByte();
    if (receivedChar > 0) {
      Serial.write(receivedChar); 
      lastActivity = millis(); // Refresh the 15s timer
    }
  } else {
    digitalWrite(signalLedPin, LOW);
  }

  // 2. TRANSMITTER (From PuTTY)
  if (Serial.available() > 0) {
    char outChar = (char)Serial.read();
    lastActivity = millis();
    sightingMode = false; 
    transmitByte(outChar); 
  }

  // 3. AUTOMATIC SIGHTING BEACON
  // This ONLY controls the light, it no longer blocks the receiver logic.
  if (!sightingMode && (currentTime - lastActivity > timeout)) {
    sightingMode = true;
    referenceLevel = (float)analogRead(sensorPin); // Re-calibrate to current room light
  }
  
  // Apply Sighting State to LED
  // (But only if we aren't currently transmitting)
  if (sightingMode) {
    digitalWrite(ledPin, HIGH);
  } else if (Serial.available() == 0) {
    digitalWrite(ledPin, LOW);
  }
}

void transmitByte(char c) {
  int bitTime = 100; 
  digitalWrite(ledPin, HIGH); // Start Bit
  delay(bitTime);
  for (int i = 0; i < 8; i++) {
    digitalWrite(ledPin, bitRead(c, i));
    delay(bitTime);
  }
  digitalWrite(ledPin, LOW); // Stop Bit
  delay(bitTime);
}

char receiveByte() {
  int bitTime = 100; 
  char result = 0;
  
  // Wait 150ms to get past the Start Bit and into the center of Bit 0
  delay(150); 
  
  for (int i = 0; i < 8; i++) {
    int val = analogRead(sensorPin);
    float diff = ((float)val - referenceLevel) / referenceLevel;
    
    if (diff > (deltaPercent / 2)) {
      bitSet(result, i);
      digitalWrite(signalLedPin, HIGH); // Visual feedback of '1' bits
    } else {
      digitalWrite(signalLedPin, LOW);
    }
    
    delay(bitTime);
  }
  
  digitalWrite(signalLedPin, LOW);
  return result;
}


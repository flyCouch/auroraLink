/*
 * Aurora-Link v3.9.8 - "The Locked Reference"
 * - Logic: referenceLevel is set ONLY during setup() / Reset.
 * - Recovery: 15s timer returns to Sighting Mode without re-calibrating.
 * - Speed: 10 Baud (100ms bits).
 */

long baudRate = 10; 

const int ledPin = 2;           
const int signalLedPin = 13;    
const int sensorPin = A0;       

bool sightingMode = true;       
unsigned long lastActivity = 0;
const unsigned long timeout = 15000; 

float referenceLevel = 512.0;    
const float deltaPercent = 0.05; 

void setup() {
  Serial.begin(baudRate);
  pinMode(ledPin, OUTPUT);
  pinMode(signalLedPin, OUTPUT);
  
  // Flash Pin 13 to show calibration is happening
  digitalWrite(signalLedPin, HIGH);
  delay(1000); 
  
  // LOCK the reference level here once.
  referenceLevel = (float)analogRead(sensorPin);
  
  digitalWrite(signalLedPin, LOW);
  digitalWrite(ledPin, HIGH); // Startup in Sighting Mode
  lastActivity = millis();
}

void loop() {
  unsigned long currentTime = millis();
  int sensorValue = analogRead(sensorPin);
  float change = ((float)sensorValue - referenceLevel) / referenceLevel;
  
  // 1. CONSTANT RECEIVER
  if (change > deltaPercent) {
    digitalWrite(signalLedPin, HIGH);
    
    if (sightingMode) {
      sightingMode = false;
      digitalWrite(ledPin, LOW); 
      // Jump straight into the start bit
    }
    
    char receivedChar = receiveByte();
    if (receivedChar > 0) {
      Serial.write(receivedChar); 
      lastActivity = millis(); 
    }
  } else {
    digitalWrite(signalLedPin, LOW);
  }

  // 2. TRANSMITTER
  if (Serial.available() > 0) {
    char outChar = (char)Serial.read();
    lastActivity = millis();
    sightingMode = false; 
    transmitByte(outChar); 
  }

  // 3. AUTOMATIC SIGHTING BEACON (Reference is NOT reset here)
  if (!sightingMode && (currentTime - lastActivity > timeout)) {
    sightingMode = true;
  }
  
  // Apply Sighting State
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
  
  // Sample at 150% of bit time to hit the middle of Bit 0
  delay(150); 
  
  for (int i = 0; i < 8; i++) {
    int val = analogRead(sensorPin);
    float diff = ((float)val - referenceLevel) / referenceLevel;
    
    if (diff > (deltaPercent / 2)) {
      bitSet(result, i);
      digitalWrite(signalLedPin, HIGH);
    } else {
      digitalWrite(signalLedPin, LOW);
    }
    delay(bitTime);
  }
  
  digitalWrite(signalLedPin, LOW);
  return result;
}

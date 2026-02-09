/*
 * Aurora-Link v3.9.9 - "The Adaptive Sentinel"
 * - Logic: referenceLevel is set ONLY during setup() / Reset.
 * - Recovery: 15s timer returns to Sighting Mode without re-calibrating.
 * - Dynamic Speed: Transmit and Receive timing now follow the baudRate variable.
 */

long baudRate = 10;             // Change this to adjust speed (e.g., 10, 1200)

const int ledPin = 2;           
const int signalLedPin = 13;    
const int sensorPin = A0;       

bool sightingMode = true;       
unsigned long lastActivity = 0;
const unsigned long timeout = 15000; 

float referenceLevel = 512.0;    
const float deltaPercent = 0.05; // 5% jump threshold

void setup() {
  Serial.begin(baudRate);       // Sets USB speed
  pinMode(ledPin, OUTPUT);
  pinMode(signalLedPin, OUTPUT);
  
  digitalWrite(signalLedPin, HIGH);
  delay(1000); 
  
  referenceLevel = (float)analogRead(sensorPin); // LOCK reference
  
  digitalWrite(signalLedPin, LOW);
  digitalWrite(ledPin, HIGH); 
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

  // 3. AUTOMATIC SIGHTING BEACON
  if (!sightingMode && (currentTime - lastActivity > timeout)) {
    sightingMode = true;
  }
  
  if (sightingMode) {
    digitalWrite(ledPin, HIGH);
  } else if (Serial.available() == 0) {
    digitalWrite(ledPin, LOW);
  }
}

void transmitByte(char c) {
  // Calculate bitTime in milliseconds
  int bitTime = 1000 / baudRate; 
  
  digitalWrite(ledPin, HIGH);   // Start Bit
  delay(bitTime);
  
  for (int i = 0; i < 8; i++) {
    digitalWrite(ledPin, bitRead(c, i)); // Data Bits
    delay(bitTime);
  }
  
  digitalWrite(ledPin, LOW);    // Stop Bit
  delay(bitTime);
}

char receiveByte() {
  // Dynamic timing for receiver
  int bitTime = 1000 / baudRate;
  int sampleOffset = bitTime * 1.5; // Hits the middle of Bit 0
  
  char result = 0;
  
  delay(sampleOffset);
  
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

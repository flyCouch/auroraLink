/*
 * Aurora-Link v3.9.12
 * - Fix: signalLedPin (Pin 13) now stays ON during reception.
 * - Fix: ledPin (Pin 2) only goes LOW when transmitting data.
 * - Logic: Reference is LOCKED on Reset.
 * - Speed: 20 Baud.
 */

long baudRate = 20; 

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
  
  digitalWrite(signalLedPin, HIGH);
  delay(1000); 
  
  referenceLevel = (float)analogRead(sensorPin);
  
  digitalWrite(signalLedPin, LOW);
  digitalWrite(ledPin, HIGH); 
  lastActivity = millis();
}

void loop() {
  unsigned long currentTime = millis();
  int sensorValue = analogRead(sensorPin);
  float change = ((float)sensorValue - referenceLevel) / referenceLevel;
  
  // 1. CONSTANT RECEIVER & SIGNAL LIGHT
  // Pin 13 now purely reflects the sensor state regardless of mode.
  if (change > deltaPercent) {
    digitalWrite(signalLedPin, HIGH);
    
    // If we were in Sighting Mode, typing elsewhere or receiving
    // data should technically move us into Data Mode.
    if (sightingMode) {
      sightingMode = false;
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
    
    // The LED is handled inside transmitByte (High for Start, Low for Stop)
    transmitByte(outChar);
  }

  // 3. AUTOMATIC SIGHTING BEACON TIMER
  if (!sightingMode && (currentTime - lastActivity > timeout)) {
    sightingMode = true;
  }
  
  // 4. LED IDLE STATE (ONLY GOES LOW IF SENDING DATA OR IN DATA MODE)
  if (sightingMode) {
    digitalWrite(ledPin, HIGH);
  } else {
    // Stays LOW while in data mode until transmitByte takes over.
    digitalWrite(ledPin, LOW); 
  }
}

void transmitByte(char c) {
  int bitTime = 1000 / baudRate; 
  
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
  int bitTime = 1000 / baudRate;
  int sampleOffset = bitTime * 1.5; 
  
  char result = 0;
  // Wait to hit center of Bit 0
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
  
  // Let the signal light reflect the real-time sensor state again after decoding
  return result;
}

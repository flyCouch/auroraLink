/*
 * Aurora-Link v3.9.31
 * - Modification: Beacon function (Sighting Mode) and timer completely removed.
 * - Logic: Pin 2 (Emitter) is LOW by default; pulses only for data.
 * - Logic: Reference is LOCKED on Reset.
 * - Speed: 20 Baud.
 */

long baudRate = 110; 

const int ledPin = 2;           
const int signalLedPin = 13;
const int sensorPin = A0;       

float referenceLevel = 512.0;    
const float deltaPercent = 0.05; 

void setup() {
  Serial.begin(baudRate);
    // --- START INTERNAL ID SNIPPET ---
  Serial.println(F("\n========================================"));
  Serial.print(F("FILE:     ")); Serial.println(F(__FILE__)); // This shows the full path
  Serial.print(F("COMPILED: ")); Serial.print(F(__DATE__)); 
  Serial.print(F(" | "));      Serial.println(F(__TIME__));
  Serial.println(F("========================================\n"));
  // --- END INTERNAL ID SNIPPET ---
  pinMode(ledPin, OUTPUT);
  pinMode(signalLedPin, OUTPUT);
  
  // Emitter starts LOW and stays LOW until transmitting
  digitalWrite(ledPin, LOW); 
  
  // Indicator for calibration window
  digitalWrite(signalLedPin, HIGH);
  delay(1000); 
  
  // Lock the baseline reference based on current ambient light
  referenceLevel = (float)analogRead(sensorPin);
  
  digitalWrite(signalLedPin, LOW);
}

void loop() {
  int sensorValue = analogRead(sensorPin);
  float change = ((float)sensorValue - referenceLevel) / referenceLevel;

  // 1. RECEIVER & SIGNAL MONITOR
  // Pin 13 purely reflects the sensor state relative to the reference.
  if (change > deltaPercent) {
    digitalWrite(signalLedPin, HIGH);
    
    char receivedChar = receiveByte();
    if (receivedChar > 0) {
      Serial.write(receivedChar); 
    }
  } else {
    digitalWrite(signalLedPin, LOW);
  }

  // 2. TRANSMITTER
  if (Serial.available() > 0) {
    char outChar = (char)Serial.read();
    
    // The LED is handled inside transmitByte (High for Start, Low for Stop)
    transmitByte(outChar);
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
  
  digitalWrite(ledPin, LOW); // Stop Bit / Idle State
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
  
  return result;
}

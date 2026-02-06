/*
 * Aurora-Link v3.3 - "The Single-Pot Steward"
 * * FEATURES:
 * - Starts in Sighting Mode (Steady Green).
 * - Uses a single Potentiometer + CdS voltage divider on A0.
 * - Switches to Data Mode instantly upon PC or Light activity.
 * - Auto-Reverts to Sighting Mode after 15 seconds of silence.
 * - "!!!" command via Serial forces Sighting Mode back on.
 */

const int ledPin = 3;           // Main Green Beacon/Data LED
const int signalLedPin = 13;    // Onboard LED for signal monitoring
const int sensorPin = A0;       // Center tap of CdS + Pot voltage divider

String inputBuffer = "";
bool sightingMode = true;
unsigned long lastActivity = 0;
const unsigned long timeout = 15000;

// Fixed software threshold; physical Potentiometer tunes the hardware to match this.
const int threshold = 512; 
const int hysteresis = 20; 

void setup() {
  Serial.begin(1200);
  pinMode(ledPin, OUTPUT);
  pinMode(signalLedPin, OUTPUT);
  inputBuffer.reserve(10);
  
  digitalWrite(ledPin, HIGH);
  lastActivity = millis();
}

void loop() {
  unsigned long currentTime = millis();
  int sensorValue = analogRead(sensorPin);

  // 1. MONITOR PC DATA (USB)
  if (Serial.available() > 0) {
    char inChar = (char)Serial.read();
    lastActivity = currentTime;
    
    if (sightingMode) {
      sightingMode = false;
    }

    // Process "!!!" switch
    if (inChar == '\r' || inChar == '\n') {
      if (inputBuffer == "!!!") {
        sightingMode = true;
        digitalWrite(ledPin, HIGH);
      }
      inputBuffer = "";
    } else {
      inputBuffer += inChar;
      if (inputBuffer.length() > 8) inputBuffer = "";
    }

    if (!sightingMode) {
      transmitByte(inChar);
    }
  }

  // 2. MONITOR INCOMING LIGHT (CdS with Hysteresis)
  static bool isLightDetected = false;

  if (!isLightDetected && sensorValue > (threshold + hysteresis)) {
    isLightDetected = true;
    digitalWrite(signalLedPin, HIGH);
    lastActivity = currentTime;
    
    if (sightingMode) {
      sightingMode = false;
    }
    // Incoming light detected - Logic ready for BBS bit-stream
  } 
  else if (isLightDetected && sensorValue < (threshold - hysteresis)) {
    isLightDetected = false;
    digitalWrite(signalLedPin, LOW);
  }

  // 3. THE HEALING LOGIC
  if (!sightingMode && (currentTime - lastActivity > timeout)) {
    sightingMode = true;
    digitalWrite(ledPin, HIGH);
  }
}

void transmitByte(char c) {
  unsigned int bitTime = 833; // 1200 baud bit duration
  digitalWrite(ledPin, LOW);  // Start bit
  delayMicroseconds(bitTime);
  for (int i = 0; i < 8; i++) {
    digitalWrite(ledPin, bitRead(c, i));
    delayMicroseconds(bitTime);
  }
  digitalWrite(ledPin, HIGH); // Stop bit
  delayMicroseconds(bitTime);
}

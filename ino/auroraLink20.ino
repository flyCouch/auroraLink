/*
 * Aurora-Link v3.6 - "The Pulsing Sentinel"
 * * FEATURES:
 * - Starts in Sighting Mode (Steady Green).
 * - "!!!" : Toggles between Sighting Mode and Hard Disable (Force Off).
 * - "@@@" : Wakes from Hard Disable into Pulse Mode (2Hz blink).
 * - Pulse Mode or Sighting Mode will auto-wake to Data Mode on activity.
 */

const int ledPin = 3;
const int signalLedPin = 13;
const int sensorPin = A0;

String inputBuffer = "";
bool sightingMode = true;
bool hardDisable = false;
bool pulseMode = false;         // New state for 2Hz blink
unsigned long lastActivity = 0;
const unsigned long timeout = 15000;

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
    
    if (inChar == '\r' || inChar == '\n') {
      // Toggle Hard Disable / Sighting
      if (inputBuffer == "!!!") {
        pulseMode = false;
        if (!hardDisable) {
          hardDisable = true;
          sightingMode = false;
          digitalWrite(ledPin, LOW);
        } else {
          hardDisable = false;
          sightingMode = true;
          digitalWrite(ledPin, HIGH);
        }
      } 
      // Wake up with Pulse Mode
      else if (inputBuffer == "@@@") {
        hardDisable = false;
        sightingMode = false;
        pulseMode = true;
      }
      inputBuffer = "";
    } else {
      inputBuffer += inChar;
      if (inputBuffer.length() > 8) inputBuffer = "";
    }

    if (!hardDisable) {
      lastActivity = currentTime;
      if (sightingMode || pulseMode) {
        sightingMode = false;
        pulseMode = false;
      }
      transmitByte(inChar);
    }
  }

  // 2. MONITOR INCOMING LIGHT (CdS)
  static bool isLightDetected = false;
  if (!isLightDetected && sensorValue > (threshold + hysteresis)) {
    isLightDetected = true;
    digitalWrite(signalLedPin, HIGH);
    if (!hardDisable) {
      lastActivity = currentTime;
      sightingMode = false;
      pulseMode = false;
    }
  } 
  else if (isLightDetected && sensorValue < (threshold - hysteresis)) {
    isLightDetected = false;
    digitalWrite(signalLedPin, LOW);
  }

  // 3. BEACON & PULSE LOGIC
  if (hardDisable) {
    digitalWrite(ledPin, LOW);
  } else if (sightingMode) {
    digitalWrite(ledPin, HIGH);
  } else if (pulseMode) {
    // 2Hz Blink (250ms ON, 250ms OFF)
    bool blinkState = (millis() / 250) % 2;
    digitalWrite(ledPin, blinkState);
  } else {
    // HEALING LOGIC: Auto-revert to sighting if inactive
    if (currentTime - lastActivity > timeout) {
      sightingMode = true;
      digitalWrite(ledPin, HIGH);
    }
  }
}

void transmitByte(char c) {
  unsigned int bitTime = 833; 
  digitalWrite(ledPin, LOW); // Start
  delayMicroseconds(bitTime);
  for (int i = 0; i < 8; i++) {
    digitalWrite(ledPin, bitRead(c, i));
    delayMicroseconds(bitTime);
  }
  digitalWrite(ledPin, HIGH); // Stop
  delayMicroseconds(bitTime);
}

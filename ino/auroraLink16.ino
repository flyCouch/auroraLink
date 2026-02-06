/*
 * Aurora-Link v3.5 - "The Disciplined Steward"
 * * FEATURES:
 * - Starts in Sighting Mode (Steady Green).
 * - "!!!" (once): Forces Sighting Mode (Steady Green).
 * - "!!!" (twice): Hard Disables Beacon (Force Off). 
 * - In Hard Disable, system will NOT auto-wake from light or data.
 * - Requires manual "!!!" to re-enable operation.
 */

const int ledPin = 3;           // Main Green Beacon/Data LED
const int signalLedPin = 13;    // Onboard status LED
const int sensorPin = A0;       // CdS Voltage Divider

String inputBuffer = "";
bool sightingMode = true;       // Start in sighting mode
bool hardDisable = false;       // Manual override lock
unsigned long lastActivity = 0;
const unsigned long timeout = 15000; // 15 seconds

const int threshold = 512;      // Software midpoint
const int hysteresis = 20; 

void setup() {
  Serial.begin(1200);
  pinMode(ledPin, OUTPUT);
  pinMode(signalLedPin, OUTPUT);
  inputBuffer.reserve(10);
  
  digitalWrite(ledPin, HIGH);   // Initial Sighting Beacon
  lastActivity = millis();
}

void loop() {
  unsigned long currentTime = millis();
  int sensorValue = analogRead(sensorPin);

  // 1. MONITOR PC DATA (USB)
  if (Serial.available() > 0) {
    char inChar = (char)Serial.read();
    
    // Always listen for the "!!!" toggle command
    if (inChar == '\r' || inChar == '\n') {
      if (inputBuffer == "!!!") {
        if (!hardDisable) {
          // If active, enter Hard Disable
          hardDisable = true;
          sightingMode = false;
          digitalWrite(ledPin, LOW);
        } else {
          // If disabled, toggle back to Sighting Mode
          hardDisable = false;
          sightingMode = true;
          digitalWrite(ledPin, HIGH);
        }
      }
      inputBuffer = "";
    } else {
      inputBuffer += inChar;
      if (inputBuffer.length() > 8) inputBuffer = "";
    }

    // Only process data if NOT in Hard Disable
    if (!hardDisable) {
      lastActivity = currentTime;
      if (sightingMode) sightingMode = false;
      transmitByte(inChar);
    }
  }

  // 2. MONITOR INCOMING LIGHT (CdS)
  static bool isLightDetected = false;
  if (!isLightDetected && sensorValue > (threshold + hysteresis)) {
    isLightDetected = true;
    digitalWrite(signalLedPin, HIGH);
    
    // Only allow wake-up and activity tracking if NOT Hard Disabled
    if (!hardDisable) {
      lastActivity = currentTime;
      if (sightingMode) sightingMode = false;
    }
  } 
  else if (isLightDetected && sensorValue < (threshold - hysteresis)) {
    isLightDetected = false;
    digitalWrite(signalLedPin, LOW);
  }

  // 3. THE HEALING LOGIC
  // Only auto-revert to beacon if system is active and not hard-disabled
  if (!sightingMode && !hardDisable && (currentTime - lastActivity > timeout)) {
    sightingMode = true;
    digitalWrite(ledPin, HIGH);
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

/*
 * Aurora-Link v3.6.1 - "The Agile Sentinel"
 * * FEATURES:
 * - Single variable 'baudRate' controls both USB and Optical timing.
 * - Supports wide range: 16bps (visual) to 12000bps (high-speed).
 * - Starts in Sighting Mode (Steady Green).
 * - "!!!" : Toggles between Sighting Mode and Hard Disable (Force Off).
 * - "@@@" : Wakes from Hard Disable into Pulse Mode (1Hz blink).
 * - Pulse Mode or Sighting Mode will auto-wake to Data Mode on activity.
 */

// --- ADJUST THIS VARIABLE ONLY ---
long baudRate = 1200;           // Set your desired speed (e.g., 16, 1200, 12000)

const int ledPin = 2;           // Main Green Beacon/Data LED
const int signalLedPin = 13;    // Onboard status LED
const int sensorPin = A0;       // CdS Voltage Divider

String inputBuffer = "";
bool sightingMode = true;       // Start in sighting mode
bool hardDisable = false;       // Manual override lock
bool pulseMode = false;         // New state for 1Hz blink
unsigned long lastActivity = 0;
const unsigned long timeout = 15000; // 15 seconds

const int threshold = 512;      // Software midpoint
const int hysteresis = 20;      // Noise buffer

void setup() {
  Serial.begin(baudRate);       // Automatically uses your baudRate variable
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
    // 1Hz Blink (500ms ON, 500ms OFF)
    bool blinkState = (millis() / 500) % 2;
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
  // Calculates bitTime in microseconds based on baudRate
  // For 1200bps, this yields 833us. For 16bps, this yields 62500us.
  unsigned int bitTime = 1000000 / baudRate; 

  digitalWrite(ledPin, LOW);    // Start bit
  delayMicroseconds(bitTime);

  for (int i = 0; i < 8; i++) {
    digitalWrite(ledPin, bitRead(c, i)); // Data bits
    delayMicroseconds(bitTime);
  }

  digitalWrite(ledPin, HIGH);   // Stop bit
  delayMicroseconds(bitTime);
}

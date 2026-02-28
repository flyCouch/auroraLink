/*
 * Aurora-Link v121-Visible
 * - Hardware: Optimized for Laser Emitter and Digital Receiver
 * - Speed: 1200 Baud (Fast Blue/Visible)
 */

// Speed Settings
const long baudVisible = 1200;  

// Pin Mapping
const int ledPin       = 2;    
const int invLedPin    = 5;    
const int signalLedPin = 13;   
const int sensorVisible = 3;   

void setup() {
  Serial.begin(115200); 

  // --- START INTERNAL ID SNIPPET ---
  Serial.println(F("\n========================================"));
  Serial.println(F("PROJECT:  Aurora Link"));
  Serial.print(F("FILE:     ")); Serial.println(F(__FILE__));
  Serial.print(F("COMPILED: ")); Serial.print(F(__DATE__));
  Serial.print(F(" | "));      Serial.println(F(__TIME__));
  Serial.println(F("========================================\n"));
  // --- END INTERNAL ID SNIPPET ---

  pinMode(ledPin, OUTPUT);
  pinMode(invLedPin, OUTPUT);
  pinMode(signalLedPin, OUTPUT);
  
  // v120 Stability: Internal pull-up to prevent floating errors
  pinMode(sensorVisible, INPUT_PULLUP); 

  digitalWrite(ledPin, LOW); 
  digitalWrite(invLedPin, HIGH);

  Serial.println(F("SYSTEM: Visible Link Active (1200 Baud)."));
}

void loop() {
  // --- RECEIVER POLLING ---
  
  // Check Visible Module (D3) - Standard Digital Logic
  if (digitalRead(sensorVisible) == LOW) { 
    processStream(sensorVisible, baudVisible);
  }

  // --- TRANSMITTER ---
  if (Serial.available() > 0) {
    char outChar = (char)Serial.read();
    transmitByte(outChar, baudVisible);
  }
}

void processStream(int pin, long speed) {
  digitalWrite(signalLedPin, HIGH);
  char c = receiveByte(pin, speed);
  if (c > 0) Serial.write(c);
  digitalWrite(signalLedPin, LOW);
}

void transmitByte(char c, long speed) {
  long bitTime = 1000000 / speed;
  
  // Start Bit
  digitalWrite(ledPin, HIGH); 
  digitalWrite(invLedPin, LOW); 
  delayMicroseconds(bitTime);

  // Data Bits (8-N-1)
  for (int i = 0; i < 8; i++) {
    bool b = bitRead(c, i);
    digitalWrite(ledPin, b);
    digitalWrite(invLedPin, !b); 
    delayMicroseconds(bitTime);
  }
  
  // Stop Bit
  digitalWrite(ledPin, LOW); 
  digitalWrite(invLedPin, HIGH); 
  delayMicroseconds(bitTime);
}

char receiveByte(int pin, long speed) {
  long bitTime = 1000000 / speed;
  
  // Jump to middle of the first data bit
  delayMicroseconds(bitTime * 1.5); 
  
  char result = 0;
  for (int i = 0; i < 8; i++) {
    // Logic is inverted: LOW = Light Detected
    bool detected = (digitalRead(pin) == LOW);
    if (detected) bitSet(result, i);
    delayMicroseconds(bitTime);
  }
  return result;
}

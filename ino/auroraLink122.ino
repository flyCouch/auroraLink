/*
 * Aurora-Link v125-NonBlocking 
 * - Strategy: State-Machine transmitter (No delays)
 * - Goal: Support BBS interaction with Synchronet
 * - Speed: 1200 Baud (833us per bit)
 */

// Speed Settings
const long baudVisible = 1200;
const long bitTime = 1000000 / baudVisible; 

// Pin Mapping
const int ledPin       = 2;
const int invLedPin    = 5;    
const int signalLedPin = 13;   
const int sensorVisible = 3;

// Transmitter State Machine
enum TxState { TX_IDLE, TX_START, TX_DATA, TX_STOP };
TxState currentTxState = TX_IDLE;
char txBuffer = 0;
int txBitIndex = 0;
unsigned long lastTxMicros = 0;

void setup() {
  Serial.begin(115200); 

  // --- START INTERNAL ID SNIPPET ---
  Serial.println(F("\n========================================"));
  Serial.println(F("PROJECT:  Aurora Link - NON-BLOCKING"));
  Serial.print(F("FILE:     ")); Serial.println(F(__FILE__));
  Serial.print(F("COMPILED: ")); Serial.print(F(__DATE__));
  Serial.print(F(" | "));      Serial.println(F(__TIME__));
  Serial.println(F("========================================\n"));

  pinMode(ledPin, OUTPUT);
  pinMode(invLedPin, OUTPUT);
  pinMode(signalLedPin, OUTPUT);
  pinMode(sensorVisible, INPUT_PULLUP); 

  digitalWrite(ledPin, LOW); 
  digitalWrite(invLedPin, HIGH);

  Serial.println(F("SYSTEM: Non-Blocking Transmit Active."));
}

void loop() {
  unsigned long currentMicros = micros();

  // 1. NON-BLOCKING TRANSMITTER
  handleTransmitter(currentMicros);

  // 2. RECEIVER POLLING
  if (digitalRead(sensorVisible) == LOW) { 
    processStream(sensorVisible, baudVisible);
  }
}

void handleTransmitter(unsigned long now) {
  switch (currentTxState) {
    case TX_IDLE:
      if (Serial.available() > 0) {
        txBuffer = (char)Serial.read();
        txBitIndex = 0;
        currentTxState = TX_START;
        lastTxMicros = now;
        digitalWrite(ledPin, HIGH); 
        digitalWrite(invLedPin, LOW); 
      }
      break;

    case TX_START:
      if (now - lastTxMicros >= bitTime) {
        currentTxState = TX_DATA;
        lastTxMicros = now;
        writeCurrentBit();
      }
      break;

    case TX_DATA:
      if (now - lastTxMicros >= bitTime) {
        lastTxMicros = now;
        txBitIndex++;
        if (txBitIndex < 8) {
          writeCurrentBit();
        } else {
          currentTxState = TX_STOP;
          digitalWrite(ledPin, LOW); 
          digitalWrite(invLedPin, HIGH); 
        }
      }
      break;

    case TX_STOP:
      if (now - lastTxMicros >= bitTime) {
        currentTxState = TX_IDLE;
      }
      break;
  }
}

void writeCurrentBit() {
  bool b = bitRead(txBuffer, txBitIndex);
  digitalWrite(ledPin, b);
  digitalWrite(invLedPin, !b); 
}

void processStream(int pin, long speed) {
  digitalWrite(signalLedPin, HIGH);
  char c = receiveByte(pin, speed);
  if (c > 0) Serial.write(c);
  digitalWrite(signalLedPin, LOW);
}

char receiveByte(int pin, long speed) {
  long bTime = 1000000 / speed;
  delayMicroseconds(bTime * 1.5); 
  char result = 0;
  for (int i = 0; i < 8; i++) {
    if (digitalRead(pin) == LOW) bitSet(result, i);
    delayMicroseconds(bTime);
  }
  return result;
}

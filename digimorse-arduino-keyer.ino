// 
// digimorse-arduino-keyer, an Arduino Nano-based Morse key/paddle <-> USB Serial interface and simple keyer
// for use in the digimorse project.
//
// Libraries required:
// TimerOne
//
// (C) 2020 Matt Gumbley M0CUV
//

#include <Arduino.h> 
#include <TimerOne.h>
#include "SCoop.h"

// INPUTS ON PINS --------------------------------------------------------------------------------------------------------------
                           //      76543210
const int padBIn = 4;      // PIND    x  -- paddle
const int padBInBit = 0x10;
const int padAIn = 5;      // PIND   x   -- straight key
const int padAInBit = 0x20;

// Port Manipulation
//    B (digital pin 8 to 13)
//    The two high bits (6 & 7) map to the crystal pins and are not usable 
//    C (analog input pins)
//    D (digital pins 0 to 7) 
//    The two low bits (0 & 1) are for serial comms and shouldn't be changed.

// PIND:
// 7     6     5     4     3     2     -     -
// PADA  PADB  

// OUTPUTS ON PINS -------------------------------------------------------------------------------------------------------------

const int ledOut = 13;

// TODO will need an analogue output via an RC filter to a small amp and speaker, for sidetone generation.
const int sidetoneOut = 5;  // PWM, with RC low-pass filter network to convert
                            // to analogue. On OCR3A, PWM phase correct.



// EVENT MANAGEMENT ------------------------------------------------------------------------------------------------------------

// We have several events that we can react to....
const uint16_t PADA_RELEASE = 0;
const uint16_t PADA_PRESS = 1;
const uint16_t PADB_RELEASE = 2;
const uint16_t PADB_PRESS = 3;

// Events detected by the interrupt handler are enqueued on this FIFO queue, 
// which is read by processNextEvent (in non-interrupt time).
defineFifo(eventFifo, uint16_t, 100)

// Enqueue an event on the FIFO queue.
static char zut[20];
void eventOccurred(const uint16_t eventCode) {
#ifdef DEBUGEVENT
    sprintf(zut, ">ev:0x%02X", eventCode);
    Serial.println(zut);
#endif    
    if (!eventFifo.putInt(eventCode)) {
        Serial.println("FIFO overrun");
    }
}

void processEvent(const uint16_t e) {
  switch(e) {
    case PADA_RELEASE:
      Serial.print("A");
      break;
    case PADA_PRESS:
      Serial.print("a");
      break;
    case PADB_RELEASE:
      Serial.print("B");
      break;
    case PADB_PRESS:
      Serial.print("b");
      break;    
  }
}

// #define DEBUGEVENT
void processNextEvent() {
  // If there any events on the FIFO queue that were pushed by the ISR, process them here in the main non-interrupt loop.
  uint16_t event;
  if (eventFifo.get(&event)) {
#ifdef DEBUGEVENT
    char buf[80];
    sprintf(buf, "Got an event 0x%02x in processNextEvent", event);
    Serial.println(buf);
#endif
    processEvent(event);
  }
}


// INTERRUPT CONTROL -----------------------------------------------------------------------------------------------------------

inline uint16_t readPins() {
    return PIND & 0xFC;
}

// input change detection, called in loop() for test harness, or in ISR
volatile uint16_t oldPins;
volatile uint16_t newPins;
volatile uint16_t initialPins;

void tobin(char *buf, int x) {
  buf[0] = ((x & 0x80) == 0x80) ? '1' : '0';
  buf[1] = ((x & 0x40) == 0x40) ? '1' : '0';
  buf[2] = ((x & 0x20) == 0x20) ? '1' : '0';
  buf[3] = ((x & 0x10) == 0x10) ? '1' : '0';
  buf[4] = ((x & 0x08) == 0x08) ? '1' : '0';
  buf[5] = ((x & 0x04) == 0x04) ? '1' : '0';
  buf[6] = ((x & 0x02) == 0x02) ? '1' : '0';
  buf[7] = ((x & 0x01) == 0x01) ? '1' : '0';
  buf[8] = '\0';
}

// #define DEBUGPINS 
int ledState = LOW;
void toggleLED() {
  digitalWrite(ledOut, ledState);
  ledState = !ledState;
}

void interruptHandler(void) {
  // Process any pin state transitions...
  newPins = readPins();
#ifdef DEBUGPINS
  if (newPins != oldPins) {
    
    char out[10];
    tobin(out, newPins);
    Serial.println(out);
  }
#endif
  // newPin high? That's a release since there are pull-up resistors.
  uint16_t newPin = newPins & padAInBit;
  if (newPin != (oldPins & padAInBit)) {
    eventOccurred(newPin == padAInBit ? PADA_RELEASE : PADA_PRESS);
  }
  newPin = newPins & padBInBit;
  if (newPin != (oldPins & padBInBit)) {
    eventOccurred(newPin == padBInBit ? PADB_RELEASE : PADB_PRESS);
  }
  
  oldPins = newPins;
}

// Initialise all hardware, interrupt handler.
void setup() {
  Serial.begin(115200); // TODO higher? is it possible?
  Serial.println("# DigiMorse Arduino Keyer");
  Serial.println("# (C) 2020 Matt Gumbley M0CUV");
  // The buttons...
  for (int p=0; p<8; p++) {
    pinMode(p, INPUT_PULLUP);
  }
  pinMode(padAIn, INPUT_PULLUP);
  pinMode(padBIn, INPUT_PULLUP);
  
  pinMode(ledOut, OUTPUT);
  digitalWrite(13, LOW);
  
  initialPins = oldPins = newPins = readPins();

  // Interrupt handler
  Timer1.initialize(20000); // Every 1/200th of a second (interrupt every 5 milliseconds).
  Timer1.attachInterrupt(interruptHandler);
}

void loop() {
  // Put your main code here, to run repeatedly.
  processNextEvent();
}


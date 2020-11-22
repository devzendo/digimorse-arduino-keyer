// 
// digimorse-arduino-keyer, an Arduino Nano-based Morse key/paddle <-> USB Serial interface and simple keyer
// for use in the digimorse project.
//
// (C) 2020 Matt Gumbley M0CUV
//

#include <Arduino.h> 
#include <TimerOne.h>
#include "SCoop.h"

// INPUTS ON PINS --------------------------------------------------------------------------------------------------------------
                           //      76543210
const int padBIn = 6;      // PIND  x    --
const int padBInBit = 0x40;
const int padAIn = 7;      // PIND x     --
const int padAInBit = 0x80;

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

const int ledCs = 10; // a.k.a. "LOAD" or "SS"
const int ledDin = 11; // "MISO"
// probably avoid 12 MOSI
const int ledClk = 13; // "SCK"
// TODO will need an analogue output via an RC filter to a small amp and speaker, for sidetone generation.

inline uint16_t readPins() {
    return (PIND & 0xC0);
}

// input change detection, called in loop() for test harness, or in ISR
volatile uint16_t oldPins;
volatile uint16_t newPins;
volatile uint16_t initialPins;

// EVENT MANAGEMENT ------------------------------------------------------------------------------------------------------------

// We have several events that we can react to....
enum Event {
  NONE,
  PADA_RELEASE, PADA_PRESS,
  PADB_RELEASE, PADB_PRESS
};

// Events detected by the interrupt handler are enqueued on this FIFO queue, 
// which is read by processNextEvent (in non-interrupt time).
defineFifo(eventFifo, Event, 100)

// Enqueue an event on the FIFO queue.
static char zut[20];
inline void eventOccurred(Event eventCode) {
#ifdef DEBUGEVENT
    sprintf(zut, ">ev:0x%04X", eventCode);
    Serial.println(zut);
#endif    
    if (!eventFifo.put(&eventCode)) {
        Serial.println("FIFO overrun");
    }
}

void processEvent(const Event e) {
  switch(e) {
    case NONE:
      break;
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

void processNextEvent() {
  // If there any events on the FIFO queue that were pushed by the ISR, process them here in the main non-interrupt loop.
  Event event;
  if (eventFifo.get(&event)) {
    //char buf[80];
    //sprintf(buf, "Got an event 0x%04x in processNextEvent", event);
    //Serial.println(buf);
    processEvent(event);
  }
}


// INTERRUPT CONTROL -----------------------------------------------------------------------------------------------------------

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
  uint16_t newPin = newPins & padAInBit;
  if (newPin != oldPins & padAInBit) {
    eventOccurred(newPin == 0 ? PADA_RELEASE : PADA_PRESS);
  }
  newPin = newPins & padBInBit;
  if (newPin != oldPins & padBInBit) {
    eventOccurred(newPin == 0 ? PADB_RELEASE : PADB_PRESS);
  }
  
  oldPins = newPins;
}

// Initialise all hardware, interrupt handler.
void initialise() {
  // The buttons...
  pinMode(padAIn, INPUT_PULLUP);
  pinMode(padBIn, INPUT_PULLUP);
  
  initialPins = oldPins = newPins = readPins();

  // Interrupt handler
  Timer1.initialize(20000); // Every 1/200th of a second (interrupt every 5 milliseconds).
  Timer1.attachInterrupt(interruptHandler);
}


void setup() {
  Serial.begin(115200);
}


void loop() {
  // Put your main code here, to run repeatedly.
  processNextEvent();
}

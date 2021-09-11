//
// digimorse-arduino-keyer, an Arduino Nano-based Morse key/paddle <-> USB Serial interface and simple keyer
// for use in the digimorse project.
//
// Libraries required:
// TimerOne
//
// (C) 2020-2021 Matt Gumbley M0CUV
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
//             PADA  PADB

// OUTPUTS ON PINS -------------------------------------------------------------------------------------------------------------

const int ledOut = 13;

// TODO will need an analogue output via an RC filter to a small amp and speaker, for sidetone generation.
const int sidetoneOut = 5;  // PWM, with RC low-pass filter network to convert
// to analogue. On OCR3A, PWM phase correct.



// EVENT MANAGEMENT ------------------------------------------------------------------------------------------------------------

// We have several events that we can react to....
const uint32_t PADA_RELEASE       = 0x0000;
const uint32_t PADA_PRESS         = 0x1000;
const uint32_t PADB_RELEASE       = 0x2000;
const uint32_t PADB_PRESS         = 0x3000;
const uint32_t COMMAND_TO_PROCESS = 0x4000;
const uint32_t START_OF_KEYING    = 0x5000;
const uint32_t END_OF_KEYING      = 0x6000;

// Events are encoded in the 32 bits:
// 3322222 22222111 11111110 000000000
// 1098765 43210987 65432109 876543210
// CODE    UNUSED   MSB-DUR  LSB-DUR
// CODE is from the above list (0-F); DUR is a 16-bit duration in ms.

// Events detected by the interrupt handler are enqueued on this FIFO queue,
// which is read by processNextEvent (in non-interrupt time).
defineFifo(eventFifo, uint32_t, 100)

// Enqueue an event on the FIFO queue.
static char zut[20];
void eventOccurred(const uint32_t eventCode) {
#ifdef DEBUGEVENT
  sprintf(zut, ">ev:0x%04X", eventCode);
  Serial.println(zut);
#endif
  if (!eventFifo.putInt(eventCode)) {
    Serial.println("# FIFO overrun");
  }
}

//#define DEBUGEVENT

void sendByte(const uint8_t b) {
#ifdef DEBUGEVENT
  char buf[20];
  sprintf(buf, "Writing byte 0x%02x %c", b, (b >= 32 && b <= 126) ? b : '.');
  Serial.println(buf);
#else
  Serial.write(b);
#endif  
}

void processEvent(const uint32_t e) {
  switch (e & 0xf000) {
    case START_OF_KEYING:
      sendByte('S');
      break;
    case PADA_RELEASE:
      sendByte('-');
      sendByte((uint8_t) ((e >> 8) & 0x0f));
      sendByte((uint8_t)  (e       & 0x0f));
      break;
    case PADA_PRESS:
      sendByte('+');
      sendByte((uint8_t) ((e >> 8) & 0x0f));
      sendByte((uint8_t)  (e       & 0x0f));
      break;
    // TODO: PADB release and press will eventually not be exposed over serial, they'll be consumed by the keyer code and transformed into + / - sequences.
    case PADB_RELEASE:
      sendByte('|');
      sendByte((uint8_t) ((e >> 8) & 0x0f));
      sendByte((uint8_t)  (e       & 0x0f));
      break;
    case PADB_PRESS:
      sendByte('*');
      sendByte((uint8_t) ((e >> 8) & 0x0f));
      sendByte((uint8_t)  (e       & 0x0f));
      break;
    case COMMAND_TO_PROCESS:
      processCommand();
      resetCommandBuilder();
      break;
    case END_OF_KEYING:
      sendByte('E');
      break;
  }
}

void processNextEvent() {
  // If there any events on the FIFO queue that were pushed by the ISR, process them here in the main non-interrupt loop.
  uint32_t event;
  if (eventFifo.get(&event)) {
#ifdef DEBUGEVENT
    char buf[80];
    sprintf(buf, "Got an event 0x%04x in processNextEvent", event);
    Serial.println(buf);
#endif
    processEvent(event);
  }
}

// INTERRUPT CONTROL -----------------------------------------------------------------------------------------------------------

const uint16_t interruptPeriodMs = 1;
volatile uint16_t interruptCount = 0;
volatile bool keyingInProgress = false;
const uint16_t keyingTimeoutMs = 1000; // TODO make variable, stored in NVRAM

// DEBOUNCE CONTROL ------------------------------------------------------------------------------------------------------------
#define DEBOUNCE

#ifdef DEBOUNCE

// Debounce logic based on code by Jack Ganssle.
const uint8_t checkMsec = interruptPeriodMs;     // Read hardware every so many milliseconds
const uint8_t pressMsec = 10;     // Stable time before registering pressed
const uint8_t releaseMsec = 20;   // Stable time before registering released

class Debouncer {
  public:
    Debouncer() {
      debouncedKeyPress = true; // If using internal pullups, the initial state is true.
    }
    // called every checkMsec.
    // The key state is +5v=released, 0v=pressed; there are pullup resistors.
    void debounce(bool rawPinState) {
      keyChanged = false;
      keyReleased = debouncedKeyPress;
      if (rawPinState == debouncedKeyPress) {
        // Set the timer which allows a change from current state
        resetTimer();
      } else {
        if (--count == 0) {
          // key has changed - wait for new state to become stable
          debouncedKeyPress = rawPinState;
          keyChanged = true;
          keyReleased = debouncedKeyPress;
          // And reset the timer
          resetTimer();
        }
      }
    }

    // Signals the key has changed from open to closed, or the reverse.
    bool keyChanged;
    // The current debounced state of the key.
    bool keyReleased;

  private:
    void resetTimer() {
      if (debouncedKeyPress) {
        count = releaseMsec / checkMsec;
      } else {
        count = pressMsec / checkMsec;
      }
    }

    uint8_t count = releaseMsec / checkMsec;
    // This holds the debounced state of the key.
    bool debouncedKeyPress = false;
};

Debouncer padADebounce;
Debouncer padBDebounce;
#endif // DEBOUNCE


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

// Incoming serial commands are read on interrupt and built up here until CR received,
// then an event is queued to cause the command to be processed. Only
// build up when an event is not being processed (wait until the commandBusy
// flag is false - it's set true when a command event has been queued.)
// #define DEBUGCOMMAND
const int MAX_COMMAND_LEN = 80;
volatile int commandLen = 0;
/* volatile (locks up compiler!) */ char commandBuffer[MAX_COMMAND_LEN];
volatile bool commandBusy = false;

void resetCommandBuilder() {
  commandLen = 0;
  commandBusy = false;
}

void enqueueCommand() {
  commandBusy = true;
  eventOccurred(COMMAND_TO_PROCESS);
}

void resetToDefaults() {
}

char out[40];

// A command from the user has been received in the command buffer.
void processCommand() {
#ifdef DEBUGCOMMAND
  Serial.println("# Processing command");
  Serial.println(commandBuffer);
#endif
  if (commandLen == 0) {
    return;
  }
  out[0] = '>';
  out[1] = ' ';
  out[2] = 'O';
  out[3] = 'K';
  out[4] = '\0';
  // To be continued...
  switch (commandBuffer[0]) {
    case '?':
      Serial.println("> V: Display version info");
      Serial.println("> K: MODE = keyer mode");
      Serial.println("> S: MODE = straight key mode *");
      Serial.println("> Q: Display settings");
      Serial.println("> W[5-40]: Set keyer speed between 5 and 40 WPM (*12)");
      Serial.println("> D[30-3000]: Set keyer semi-break-in timeout in ms");
      Serial.println("> R: POLARITY = reverse paddle polarity");
      Serial.println("> N: POLARITY = normal paddle polarity *");
      Serial.println("> !RESET!: Reset to all defaults");
      Serial.println("> (* indicates defaults)");
      break;
    case 'V':
      strcpy(out + 2, "v0.0");
      break;
    case 'K':
      break;
    case 'S':
      break;
      
    case 'Q':
      break;
    case 'W': // collect speed
      break;
    case 'R':
      break;
    case 'N':
      break;
    default:
      if (strcmp(commandBuffer, "!RESET!") == 0) {
        resetToDefaults();
      } else {
        out[2] = '?';
        out[3] = '\0';
      }
      break;
  }
  Serial.println(out);
}

void interruptHandler(void) {
  bool startOfKeyingSent = false;
  
  interruptCount++;

  // Detect end of keying timeout...
  if (interruptCount > keyingTimeoutMs) {
    if (keyingInProgress) {
      eventOccurred(END_OF_KEYING);
    }
    keyingInProgress = false;
    interruptCount = 0;
  }

  // Process any pin state transitions...
  newPins = readPins();
#ifdef DEBUGPINS
  if (newPins != oldPins) {

    tobin(out, newPins);
    Serial.println(out);
  }
#endif
  // newPin high? That's a release since there are pull-up resistors.

#ifdef DEBOUNCE
  padADebounce.debounce(newPins & padAInBit);
  if (padADebounce.keyChanged) {
    bool padA = padADebounce.keyReleased;
    if (!padA) {
      if (!keyingInProgress) {
        eventOccurred(START_OF_KEYING);
        keyingInProgress = true;
        startOfKeyingSent = true;
      }
    }
    if (!startOfKeyingSent) {
      eventOccurred((padA ? PADA_RELEASE : PADA_PRESS) | interruptCount);
    }
    interruptCount = 0;
    digitalWrite(ledOut, !padA);
  }

  padBDebounce.debounce(newPins & padBInBit);
  if (padBDebounce.keyChanged) {
    bool padB = padBDebounce.keyReleased;
    if (!padB) {
      if (!keyingInProgress) {
        eventOccurred(START_OF_KEYING);
        keyingInProgress = true;
        startOfKeyingSent = true;
      }
    }
    if (!startOfKeyingSent) {
      eventOccurred((padB ? PADB_RELEASE : PADB_PRESS) | interruptCount);
    }
    interruptCount = 0;
  }

#else // DEBOUNCE

  uint16_t newPin = newPins & padAInBit;
  if (newPin != (oldPins & padAInBit)) {
    bool padA = newPin == padAInBit;
    if (padA) {
      if (!keyingInProgress) {
        eventOccurred(START_OF_KEYING);
        keyingInProgress = true;
        startOfKeyingSent = true;
      }
    }
    if (!startOfKeyingSent) {
      eventOccurred((newPin == padAInBit ? PADA_RELEASE : PADA_PRESS) | interruptCount);
    }
    interruptCount = 0;
    digitalWrite(ledOut, !padA);
  }
  newPin = newPins & padBInBit;
  if (newPin != (oldPins & padBInBit)) {
    bool padB = newPin == padBInBit;
    if (padB) {
      if (!keyingInProgress) {
        eventOccurred(START_OF_KEYING);
        keyingInProgress = true;
        startOfKeyingSent = true;
      } 
    }
    if (!startOfKeyingSent) {
      eventOccurred((padB ? PADB_RELEASE : PADB_PRESS) | interruptCount);
    }
    interruptCount = 0;
  }

  oldPins = newPins;

#endif // DEBOUNCE

  // Process any incoming command data...
  if (commandBusy) {
    return;
  }

  int inByte = Serial.read();
  if (inByte == -1) {
    // No data...
    return;
  }
  if (commandLen == MAX_COMMAND_LEN - 1) {
    Serial.println("# Command buffer full");
    // Ditch the 'command', this shouldn't happen, it's presumably bogus.
    commandLen = 0;
    return;
  }
#ifdef DEBUGCOMMAND
  sprintf(out, "%02x", inByte & 0xff);
  Serial.println(out);
#endif
  if (inByte == '\n') {
    commandBuffer[commandLen] = '\0';
    enqueueCommand(); // commandLen will be the number of bytes of the command text, without \n.
  } else {
    commandBuffer[commandLen++] = (char) inByte;
  }
}

// Initialise all hardware, interrupt handler.
void setup() {
  Serial.begin(115200); // TODO higher? is it possible?
  Serial.println("# DigiMorse Arduino Keyer");
  Serial.println("# (C) 2020-2021 Matt Gumbley M0CUV");

  // The buttons... let's not have anything on PIND floating.
  for (int p = 0; p < 8; p++) {
    pinMode(p, INPUT_PULLUP);
  }
  // The paddle inputs, specifically..
  pinMode(padAIn, INPUT_PULLUP);
  pinMode(padBIn, INPUT_PULLUP);

  pinMode(ledOut, OUTPUT);
  digitalWrite(13, LOW);

  initialPins = oldPins = newPins = readPins();

  resetCommandBuilder();

  // Interrupt handler
  Timer1.initialize(interruptPeriodMs * 1000); // argument is microseconds
  Timer1.attachInterrupt(interruptHandler);
}

void loop() {
  // Put your main code here, to run repeatedly.
  processNextEvent();
}

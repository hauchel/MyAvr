// Ultraschall mit 8 LED und Senden auf Kommando
#include <LedCon5.h>
#include <avr/sleep.h>
#include <VirtualWire.h>

/*
  pin 2
  Using 8 7 segs
  D9  DataIn  white
  D8  LOAD    grey
  D7  CLK     pink

  
  D6  Trigg   grey
  D5  Echo    white



const int ledPin = 13;
const int transmitPin = 12;
const int receivePin = 11;
*/

const byte anzR = 1;
//             dataPin, clkPin, csPin, numDevices
LedCon5 lc = LedCon5(9, 7, 8, anzR);
const int trigPin = 6;
const int echoPin = 5;


int menuPos = 0; // Menu-Position


byte intens = 8;
int count = 0;

unsigned long nextMs, currMs; // for times
unsigned long delaytime = 500;

int an1;
int an2;

long zeit = 0;


void msg(const char txt[], int n) {
  Serial.print(txt);
  Serial.print(" ");
  Serial.println(n);
}


void testdigit() {
  for (int i = 0; i < 8; i++) {
    lc.setDigit(lc.lcRow, i, i, false);
  }
}

void setIntens(char what) {
  if (what == '+') {
    intens++;
    if (intens > 15) intens = 0;
  }
  else  {
    intens--;
    if (intens > 15) intens = 15;
  }
  for (int r = 0; r < 5; r++) {
    lc.setIntensity(r, intens);
  }
}

void show() {
  lc.cls();
  lc.show4Dig(zeit);
}

/*
void miss() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(3);
  noInterrupts();
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(50);
  digitalWrite(trigPin, LOW);
  zeit = pulseIn(echoPin, HIGH);
  interrupts();
  Serial.println(zeit);
}


void receive() {
  uint8_t buf[VW_MAX_MESSAGE_LEN];
  uint8_t buflen = VW_MAX_MESSAGE_LEN;

  if (vw_get_message(buf, &buflen)) // Non-blocking
  {
    int i;

    digitalWrite(ledPin, HIGH); // Flash a light to show received good message
    // Message with a good checksum received, dump it.
    Serial.print("Got: ");

    for (i = 0; i < buflen; i++)
    {
      lc.putch(buf[i]);
      Serial.print(buf[i], HEX);
      Serial.print(' ');
    }
    Serial.println();
    digitalWrite(ledPin, LOW);
  }
}

void wachauf() {
}

void schlaf() {
  lc.shutdown(0, true);
  attachInterrupt(0, wachauf, LOW);
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();
  sleep_mode();
  sleep_disable();
  detachInterrupt(0);
  Serial.print("aufgewacht");
  lc.shutdown(0, false);
}

*/

void doCmd(byte tmp) {
  Serial.println();
  switch (tmp) {
    case '0':   //
      nextMs = 0;
      break;
    case '1':   //
      delaytime = 1000;
      break;
    case '2':   //
      delaytime = 200;
      break;
    case '5':   //
      delaytime = 500;
      break;
    case 'd':   //
      testdigit();
      break;
    case 'f':   //
     //schlaf();
      break;
    case 'h':   //
      lc.setIntensity(0, 15);
      break;
    case 'i':   //
      analogReference(INTERNAL);
      Serial.print("Internal");
      break;
    case 'j':   //
      analogReference(DEFAULT);
      Serial.print("Default");
      break;
    case 'l':   //
      lc.setIntensity(0, 1);
      break;
    case 'p':   //
      Serial.print("Pullp");
      pinMode(A1, INPUT_PULLUP);
      pinMode(A2, INPUT_PULLUP);
      break;

    case 'n':   //
      Serial.print("Input");
      pinMode(A1, INPUT);
      pinMode(A2, INPUT);
      break;
    case 'r':   //
      reset();
      break;
    case 's':
      lc.shutdown(0, true);
      break;
    case 't':
      lc.shutdown(0, false);
      break;
    case 'u':   //
      break;
    case 'y':   //
      break;

    case '+':
      setIntens('+');
      break;
    case '-':
      setIntens('-');
      break;
    default:
      Serial.print(tmp);
      lc.putch(tmp);
  } //case
}

void reset() {
  int devices = lc.getDeviceCount();
  //we have to init all devices in a loop
  msg("Reset for ", devices);
  for (int address = 0; address < devices; address++) {
    /*The MAX72XX is in power-saving mode on startup*/
    lc.shutdown(address, false);
    /* Set the brightness to a medium values */
    lc.setIntensity(address, 5); //8
    /* and clear the display */
    lc.clearDisplay(address);
  }
}

void setup() {
  const char info[] = "UsMax2719  "__DATE__ " " __TIME__;
  Serial.begin(38400);
  Serial.println(info);
  pinMode(2, INPUT_PULLUP);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  reset();
}

void loop() {
  if (Serial.available() > 0) {
    doCmd(Serial.read());
  } // avail
  
 
  currMs = millis();
  if (nextMs <= currMs) {
    nextMs = currMs + delaytime;
    count++;
    //miss();
    show();
  } // tim
}














































































































































#include <LedCon5.h>
#include <dht11.h>
#include <avr/sleep.h>

/*
  pin 2
  Using 8 7 segs
  pin 12  DataIn pink
  pin 11  CLK    grey
  pin 10  LOAD   white
  D13  SCK     orange
  D12  MISO    yell
  D11  MOSI    green
  D10  SS      blue      ssPin
*/

const byte anzR = 1;
//             dataPin, clkPin, csPin, numDevices
LedCon5 lc = LedCon5(11, 13, 10, anzR);

dht11 DHT11;
#define DHT11PIN 9

int menuPos = 0; // Menu-Position
const byte keyCen = 0;
const byte keyUp = 1;
const byte keyDwn = 2;
const byte keyLft = 3;
const byte keyRgt = 4;
const byte keyNop = 9;
byte kbdOld = 0;
byte kbdCnt = 0;

byte intens = 8;
int count = 0;

unsigned long nextMs, currMs; // for times
unsigned long delaytime = 5000;

int an1;
int an2;

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
  lc.show4Dig(DHT11.temperature);
  lc.show4Dig(DHT11.humidity);
}


void lies() {
  an1 = analogRead(A1);
  an2 = analogRead(A2);
  int chk = DHT11.read(DHT11PIN);
  Serial.print(DHT11.temperature);
  Serial.print("  ");
  Serial.println(DHT11.humidity);
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

void doCmd(byte tmp) {
  Serial.println();
  switch (tmp) {
    case '0':   //
      nextMs = 0;
      break;
    case '1':   //
      delaytime = 1000;
      break;
    case '5':   //
      delaytime = 500;
      break;
    case 'd':   //
      testdigit();
      break;
    case 'f':   //
      schlaf();
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
  pinMode(A1, INPUT_PULLUP);
  pinMode(A2, INPUT_PULLUP);
}

void setup() {
  const char info[] = "TempMax2719  "__DATE__ " " __TIME__;
  Serial.begin(38400);
  Serial.println(info);
  pinMode(2, INPUT_PULLUP);
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
    lies();
    show();
  } // tim
}


















































































































































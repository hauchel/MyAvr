/* Analog Voltage and Conversion Display
  Display
  D8   CS/Load   grey
  D9   DataIn    white
  D10  CLK       pink
  A0             green
  A1             yell



*/
#include "rc5_tim2.h"
const byte rc5Pin = 2;   // TSOP weiss

#include <LedCon5.h>
const byte anzR = 1; // number of LED Disps
//             dataPin, clkPin, csPin, numDevices
LedCon5 lc = LedCon5(9, 10, 8, anzR);
byte intens = 2;

const byte ledPin = 13;  // indicators
byte runmode = 2;

unsigned long currTim;
unsigned long prevTim = 0;
unsigned long tickTim = 1000;
byte srcL = 0;
byte srcR = 0;
volatile unsigned int ana0, ana1;
const byte tabAnz = 10;
unsigned int tabIn[tabAnz], tabOut[tabAnz];
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
  if (what == '-') {
    intens--;
    if (intens > 15) intens = 15;
  }
  for (int r = 0; r < anzR; r++) {
    lc.setIntensity(r, intens);
  }
  msg("Intens ", intens);
}

void initTab() {
  byte i = 0;
  tabIn[i] = 100; tabOut[i] = 19; i++;
  tabIn[i] = 200; tabOut[i] = 50; i++;
}
int findTab(unsigned int n) {
  byte i = 0;
  while (i < tabAnz) {
    if (n < tabIn[i]) {
      return i;
    }
    i++;
  }
  return 0;
}

void setSrc(int n) {
  srcL = 14 + n;
  msg("SrcL ", srcL);
}

void help() {
  Serial.println(" r ticktim");
  Serial.println(" r,g,b ARef Def, Int,Ext");
  Serial.println(" n,m   switch LED on/off");
  Serial.println(" 0 to 5 src");
}

void doRC5() {
  int sel;
  msg("RC5 ", rc5_command);
  digitalWrite(ledPin, HIGH);
  if ((rc5_command >= 0) && (rc5_command <= 9)) {
    doCmd(rc5_command);
  } else {
    switch (rc5_command) {
      case 11:   // Store
        break;
      case 13:   // Mute
        break;
      case 43:   // >
        break;
      case 25:   // R
        doCmd('r');
        break;
      case 23:   // G
        doCmd('g');
        break;
      case 27:   // Y
        doCmd('y');
        break;
      case 36:   // B
        break;
      case 57:   // Ch+
        setIntens('+');
        break;
      case 56:   // Ch+
        setIntens('-');
        break;
      default:
        msg("Which RC5?", rc5_command);
    } // case
  } // else
}


void doCmd(byte tmp) {
  long zwi;
  switch (tmp) {
    case '-':
    case '+':   //
      setIntens(tmp);
      break;
    case 'a':   //
      tickTim = 1000;
      msg ("Tick ", tickTim);
      break;
    case 'b':   //
      tickTim = 500;
      msg ("Tick ", tickTim);
      break;
    case 'r':   //
      analogReference(DEFAULT);
      msg ("Aref Def", DEFAULT);
      break;
    case 'g':   //
      analogReference(INTERNAL);
      msg ("Aref Int", INTERNAL);
      break;
    case 'y':   //
      analogReference(EXTERNAL);
      msg ("Aref Ext", EXTERNAL);
      break;
    case 'h':   //
      break;
    case 'i':   //
      break;
    case 'j':   //
      break;
    case 'x':   //
      runmode = 2;
      msg("runmode", runmode);
      break;

    case 'n':
      lc.shutdown(0, true);
      break;
    case 'm':
      lc.shutdown(0, false);
      break;

    default:
      Serial.print(tmp);
      lc.putch(tmp);
  } //case
}


void lcReset() {
  int devices = lc.getDeviceCount();
  //we have to init all devices in a loop
  msg("lcReset for ", devices);
  for (int address = 0; address < devices; address++) {
    /*The MAX72XX is in power-saving mode on startup*/
    lc.shutdown(address, false);
    lc.setIntensity(address, intens);
    lc.clearDisplay(address);
  }
}

void readAna() {
  lc.shutdown(0, true);
  ana0 = analogRead(A0);
  ana1 = analogRead(A1);
  lc.shutdown(0, false);
}
void showAna()  {
  lc.home();
  lc.show4Dig(ana0);
  lc.show4Dig(ana1);
}

void setup() {
  const char info[] = "AnaMax2719  " __DATE__  " " __TIME__;
  Serial.begin(38400);
  Serial.println(info);
  RC5_init();
  pinMode(ledPin, OUTPUT);
  pinMode(A0, INPUT);
  pinMode(A1, INPUT);
  runmode = 2;
  lcReset();
}

void loop() {
  if (rc5_ok) {
    RC5_again();
    doRC5();
  }
  if (Serial.available() > 0) {
    doCmd(Serial.read());
  } // avail

  currTim = millis();
  if (currTim - prevTim >= tickTim) {
    digitalWrite(ledPin, LOW);
    prevTim = currTim;
    readAna();
    showAna();
    switch (runmode) {
      case 2:   //
        break;
      default:
        break;
    } //case
  } //tick
}


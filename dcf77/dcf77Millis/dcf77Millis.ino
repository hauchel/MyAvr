/*  dcf77 using millis
  use int0 on D2, store timer values in ripu then to data

  D2   int0
  Timer 1
  D5   T1        brown
  D6   AIN0      green
  D7   AIN1      blau
  D8   ICP1
  D9   OC1A
  D10  OC1B
*/

#define DEBUG 1
bool sample = true;     //
unsigned long currMs, nextTick;
unsigned long tickTime = 1000;

#include <dcf77.h>


dcf77 dcf = dcf77();
void int0() {
  dcf.dcfRead();
}

void help() {
  Serial.println(" d,e      show,eval Data");
  Serial.println(" a        Testdigits");
  Serial.println(" i show Info");
  Serial.println(" +,-  LCD brightness");
  Serial.println(" r run, z zero data");
  Serial.println(" , v verbose");
}

void zeit() {
  char str[100];
  sprintf(str, "hh %3u, mm %3u, Timst %7lu ", dcf.hour, dcf.minute, millis() - dcf.timestamp);
  Serial.println(str);
}

void doCmd(byte tmp) {
  switch (tmp) {
    case '0':
    case '1':
    case '2':
    case '3':
      break;
    case '+':   //
    case '-':   //
      break;
    case 'a':   //
      break;

#ifdef DEBUG
    case 'd':   //
      dcf.showData();
      break;
#endif
    case 'e':   //
      break;
    case 'f':   //
      break;
    case 'h':   //
      break;
    case 'i':   //
      dcf.info();
      break;
    case 'j':   //
      break;
    case 'm':   //
      break;
    case 'r':   //
      dcf.showRipu();
      break;
    case 's':   //
      sample = !sample;
      dcf.msgF(F("sample "), sample);
      break;
    case 'v':   //
      dcf.verb += 1;
      if (dcf.verb > 2) {
        dcf.verb = 0;
      }
      dcf.msgF(F("verbose "), dcf.verb);
      break;
    case 'x':   //
      break;
    case 'y':   //
      break;
    case 'q':
      break;
    case 'z':
      zeit();
      break;

    default:
      dcf.msgF(F("??"), tmp);
      help();
  } //case
}

void setup() {
  const char info[] = "dcf77Millis  " __DATE__  " " __TIME__;
  Serial.begin(38400);
  Serial.println(info);
  // Enable external interrupt (INT0)
  attachInterrupt(0, int0, CHANGE);
}

void loop() {
  if (Serial.available() > 0) {
    doCmd(Serial.read());
  } // avail

  currMs = millis();
  // every second
  if (nextTick <= currMs) {
    nextTick = currMs + tickTime;
    if (sample) {
      dcf.act();
    }
  } //tick
}

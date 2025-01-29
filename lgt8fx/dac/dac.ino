/*
    Parameter:      Reference:
    DEFAULT         VCC
    EXTERNAL        External Reference (REF)
    INTERNAL1V024   Internal 1.024 volts
    INTERNAL2V048   Internal 2.048 volts
    INTERNAL4V096   Internal 4.096 volts
*/

#include "helper.h"

void setRef(byte n) {
  switch (n) {
    case 0:
      analogReference(DEFAULT);
      msgF(F("Ref Def"), n);
      break;
    case 1:
      analogReference(EXTERNAL);
      msgF(F("Ref EXTERNAL"), n);
      break;
    case 2:
      analogReference(INTERNAL1V024);
      msgF(F("Ref INTERNAL1V024"), n);
      break;
    case 3:
      analogReference(INTERNAL2V048);
      msgF(F("Ref INTERNAL2V048"), n);
      break;
    case 4:
      analogReference(INTERNAL4V096);
      msgF(F("Ref INTERNAL4V096"), n);
      break;
    default:
      msgF(F("Ref ?"), n);
  }  // case
}

void doCmd(char x) {
  Serial.print(char(x));
  if (doNum(x)) {
    return;
  }
  Serial.println();
  switch (x) {
    case 13:
      vt100Clrscr();
      break;
    case 'a':
      analogWrite(DAC0, inp);
      msgF(F("DAC"), inp);
      break;

    case 'r':
      setRef(inp);
      break;
    default:
      Serial.print('?');
      Serial.println(int(x));
  }  // case
}

void setup() {
  const char info[] = "DAC " __DATE__ " " __TIME__;
  Serial.begin(38400);
  Serial.println(info);
  analogReference(DEFAULT);
  pinMode(DAC0, ANALOG);
  analogWrite(DAC0, 125);  // 0...255
}

void loop() {
  if (Serial.available() > 0) {
    doCmd(Serial.read());
  }  // serial
  currMs = millis();
  if (tickMs > 0) {
    if ((currMs - prevMs >= tickMs)) {
      //doCmd('x');
      prevMs = currMs;
    }
  }
}
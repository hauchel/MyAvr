#include <cluCom.h>

void setup() {
  // put your setup code here, to run once:
  const char ich[] = "vt100 " __DATE__  " "  __TIME__;
  Serial.begin(38400);
  Serial.println(ich);
  pinMode(ledPin, OUTPUT);
  ledOn(1);
}



/*
  cursorup(n) CUU       Move cursor up n lines                 ^[[<n>A
  cursordn(n) CUD       Move cursor down n lines               ^[[<n>B
  cursorrt(n) CUF       Move cursor right n lines              ^[[<n>C
  cursorlf(n) CUB       Move cursor left n lines               ^[[<n>D
  cursorhome            Move cursor to upper left corner       ^[[H
*/
void vt100Esc(char c, byte n) {
  char str[10];
  sprintf(str, "\x1B[%d%c", n, c);
  Serial.print(str);
}

void vt100Home() {
  Serial.print("\x1B[H");
}

void vt100(char c, byte n) {
  char str[10];
  sprintf(str, ">[%d%c<", n, c);
  Serial.print(str);
}

void doCmd(char tmp) {
  bool weg = false;
  if ( tmp == 8) { //backspace removes last digit
    weg = true;
    inp = inp / 10;
  }
  if ((tmp >= '0') && (tmp <= '9')) {
    weg = true;
    if (inpAkt) {
      inp = inp * 10 + (tmp - '0');
    } else {
      inpAkt = true;
      inp = tmp - '0';
    }
  }
  if (weg) {
    Serial.print(inp);
    return;
  }
  inpAkt = false;
  switch (tmp) {
    case 'a':
    case 'b':
    case 'c':
    case 'd':
    case 'j':
    case 'k':
      vt100Esc(toupper(tmp), inp);
      break;
    case 'A':
    case 'B':
      vt100(tmp, inp);
      break;
    case 'f':   //
      Serial.print("\x1B[f");
      break;
    case 'h':   //
      vt100Home;
      break;
    case 'H':   //
      Serial.print("\x1B[H");
      break;
    default:
      msgF(F(" ?? "), tmp);
  } // case
} 

void loop() {
  char tmp;
  if (Serial.available() > 0) {
    tmp = Serial.read();
    doCmd(tmp);
  } // avail


  currTim = millis();
  if (nexTim < currTim) {
    nexTim = currTim + tick;
    if (ledCnt == 0) {
      ledOff();
    } else {
      ledCnt--;
    }
  } // tick

}

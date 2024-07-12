/* jtag test fot Mega328PB (8Mhz)
  using MiniCore as its PB, might have problems if compiling with other cores (e.g. printf)
        JTAG(1)
  J100-1  TCK   yell  out
  J100-2  GND   GND
  J100-3  TDO   grn   in (!) we are the debugger
  J100-4  VCC   VCC
  J100-5  TMS   ws    out
  J100-6  nSRST   CLK
  J100-7  VCC   VCC
  J100-8  -
  J100-9  TDI pink    out
  J100-10 GND   GND
*/

const byte pTCK = 10; //PB2
const byte pTDO = 11; //PB3
const byte pTMS = 12; //PB4
const byte pTDI = 13; //PB5
const byte pSet = 9;  //PB1 0 recording start 1 recording stop

//could be changed to direct port access
#define TCKLo digitalWrite(pTCK, LOW)
#define TCKHi digitalWrite(pTCK, HIGH)
#define TMSLo digitalWrite(pTMS, LOW)
#define TMSHi digitalWrite(pTMS, HIGH)
#define TDILo digitalWrite(pTDI, LOW)
#define TDIHi digitalWrite(pTDI, HIGH)

byte verb = 1;
uint16_t rea;   // read from
byte numbit;    // number of bits to shift

byte runMode = 0; //
const byte befM = SPM_PAGESIZE;        // size bef
char  bef[befM];
int befP;


void prnt(PGM_P p) {
  // output char * from flash,
  while (1) {
    char c = pgm_read_byte(p++);
    if (c == 0) break;
    Serial.write(c);
  }
  Serial.write(" ");
}

void msgF(const __FlashStringHelper *ifsh, uint16_t n) {
  if (verb > 0) {
    PGM_P p = reinterpret_cast<PGM_P>(ifsh);
    prnt(p);
    Serial.println(n);
  }
}


void errF(const __FlashStringHelper *ifsh, int n) {
  runMode = 0;
  Serial.print(F("ErrF: "));
  PGM_P p = reinterpret_cast<PGM_P>(ifsh);
  prnt(p);
  Serial.println(n);
}

void prn3u(uint16_t a) {
  char str[10];
  sprintf(str, "%3u ", a);
  Serial.print(str);
}

#include "mybefs.h"


bool pgm2Bef(byte num) {
  if (num >= pgmbefM) {
    errF(F("pgm2Bef "), num);
    return false;
  }
  strcpy_P(bef, (char *)pgm_read_word(&(pgmbef[num])));
  return true;
}

void showBef() {
  char bf;
  if (befP < 0) {
    bf = '?';
  } else {
    bf = bef[befP];
  }
  Serial.print("'");
  Serial.print(bf);
  Serial.print("' '");
  Serial.print(bef);
  Serial.println("'");
}

byte seriBef() {
  // blocking read Bef, returns 0 if terminated by CR, 1 by "
  byte i;
  char b;
  Serial.print(F("Bef :"));
  for (i = 0; i < 50; i++) {
    while (Serial.available() == 0) {
    }
    b = Serial.read() ;
    Serial.print(b);
    if (b == 13) {
      bef[i] = 0;
      return 0;
    }
    if (b == '"') {
      bef[i] = 0;
      return 1;
    }
    bef[i] = b;
  } // loop
  errF (F("Seri "), i);
  bef[i] = 0;
  return 9;
}
void exec(byte num) {
  msgF(F("ExeFl"), num);
  if (!pgm2Bef(num)) {
    return;         // err already thrown
  }
  befP = 0;
  runMode = 3;
}


void jto(byte b) {
  // outputs one bit 0 or 1
  // updates rea
  if (b == 0) {
    TMSLo;
  } else {
    TMSHi;
  }
  TCKHi; // 13 us
  rea = rea << 1;
  if (digitalRead(pTDO)) rea = rea | 1 ;
  TCKLo;  //46
}

void jReset() {
  // to Run-Test
  for (int i = 0; i < 5; i++) {
    jto(1);
  }
  jto(0);
}

void jData4(byte b) {
  // from shift-IR add 4
  rea = 0;
  for (int i = 0; i < 4; i++) {
    if ((b & 1) == 0) TDILo; else TDIHi;
    b = b >> 1;
    if (i == 3) TMSHi;
    TCKHi;
    delayMicroseconds(1);
    TCKLo;
  }
  // now in exit1-IR
}
void toShiftDR() {
  // set to ShiftDR
  msgF(F("to ShiftDR"), 0);
  jto(1);
  jto(0);
  jto(0); // TMS low for the Data
}

void doShiftDR(uint16_t data, byte anz) {
  // in ShiftDR with TMS lo enter anz bit
  msgF(F("doShiftDR"), anz);
  rea = 0;
  for (int i = 1; i <= anz; i++) {
    if ((data & 1) == 0) TDILo; else TDIHi;
    data = data >> 1;
    if (i == anz) TMSHi;
    TCKHi;
    delayMicroseconds(1);
    TCKLo;
  }  // in exit1
  jto(1); // in Update
  jto(0); // in Runtest
}

void setInstruct(byte b) {
  // set instruction Register
  b = b & 0x0F;
  msgF(F("Instruct "), b);
  jto(1);
  jto(1);
  jto(0);
  jto(0); // TMS low for the Data
  jData4(b);  // in exit1-IR
  jto(1);     // in update-IR
  jto(0);     // home
}

void setActive () {
  pinMode(pTCK, OUTPUT);
  TCKLo;
  pinMode(pTDI, OUTPUT);
  pinMode(pTMS, OUTPUT);
  pinMode(pTDO, INPUT_PULLUP);
  pinMode(pSet, OUTPUT);
}

void setPassive () {
  pinMode(pTCK, INPUT);
  pinMode(pTDI, INPUT);
  pinMode(pTMS, INPUT);
  pinMode(pTDO, INPUT);
  pinMode(pSet, INPUT);
}

void monitor() {
  byte old = 0;
  byte cur;
  setPassive();
  Serial.println(F("Monitor, Reset to exit"));
  while (1) {
    cur = PINB & 0x3F;
    if (old != cur) {
      old = cur;
      Serial.printf(F("%02X "), old);
    }
  }
}

void doCmd(unsigned char c) {
  static int  inp;                   // numeric input
  static bool inpAkt = false;        // true if last input was a number
  bool weg = false;
  // handle numbers
  if ( c == 8) { //backspace removes last digit
    weg = true;
    inp = inp / 10;
  } else  if (c == '#') {
    inp++;
    weg = true;
  }
  if ((c >= '0') && (c <= '9')) {
    weg = true;
    if (inpAkt) {
      inp = inp * 10 + (c - '0');
    } else {
      inpAkt = true;
      inp = c - '0';
    }
  }
  if (weg) {
    // Serial.print("\b\b\b\b");
    // Serial.print(inp);
    return;
  }
  inpAkt = false;
  //Serial.println();
  switch (c) {
    case ' ':
      break;
    case 'a':
      setActive();
      break;
    case 'A':
      TCKHi;
      msgF(F("TCK"), 1);
      break;
    case 'B':
      TCKLo;
      msgF(F("TCK"), 0);
      break;
    case 'C':
      TMSHi;
      msgF(F("TMS"), 1);
      break;
    case 'd':
      toShiftDR();
      break;
    case 'D':
      TMSLo;
      msgF(F("TMS"), 0);
      break;
    case 'e':
      doShiftDR(inp, numbit);
      break;
    case 'E':
      TDIHi;
      msgF(F("TDI"), 1);
      break;
    case 'F':
      TDILo;
      msgF(F("TDI"), 0);
      break;
    case 'H':
      digitalWrite(pSet, HIGH);
      msgF(F("Set"), 1);
      break;
    case 'L':
      digitalWrite(pSet, LOW);
      msgF(F("Set"), 0);
      break;
    case 'i':
      Serial.println();
      Serial.printf(F("PORTB: %02X \n"), PORTB & 0x3F);
      Serial.printf(F("PINB : %02X \n"), PINB & 0x3F);
      Serial.printf(F("DDRB : %02X \n"), DDRB & 0x3F);
      break;
    case 'j':
      setInstruct(inp);
      break;
    case 'm':
      monitor();
      break;
    case 'n':
      numbit = inp;
      msgF(F("numbit"), numbit);
      break;
    case 'p':
      setPassive();
      break;
    case 't':
      msgF(F("to Runtest"), 0);
      jReset();
      break;
    case 'r':
      msgF(F("Rea was"), rea);
      rea = 0;
      break;
    case 'v':
      if (verb == 0) {
        verb = 1;
        Serial.println(F("Verbo"));
      } else {
        verb = 0;
        Serial.println(F("Silent"));
      }
      break;
    case 'x':  // load and run
      exec(inp);
      break;
    case 'y':  // run current
      befP = 0;
      runMode = 3;
      break;
   case 'z':  // run current
      showTxtAll();
      break;      
    case '"':   // to bef
      seriBef();
      showBef();
      break;
    case 13:
      showBef();
      break;

    case 223:   //ß 223 ä 228 ö 246 ü 252
      break;
    case 246:   //ß 223 ä 228 ö 246 ü 252
      Serial.print("0");
      jto(0);
      break;
    case 228:   //ß 223 ä 228 ö 246 ü 252
      jto(1);
      Serial.print("1");
      break;
    default:
      Serial.print ("?");
      Serial.println (byte(c));
      c = '?';
  } //switch
}

void setup() {
  const char ich[] PROGMEM = "jtag1" __DATE__  " " __TIME__;
  Serial.begin(38400);
  Serial.println(ich);
  setActive();
}

void loop() {
  unsigned char c;

  if (Serial.available() > 0) {
    c = Serial.read();
    Serial.print(char(c));
    doCmd(c);
    Serial.print(F(":"));
  }

  if (runMode > 0) { //executing
    c = bef[befP];
    if (c == 0) { //eof
      msgF(F("bef done "), 0);
      runMode = 0;
    }  else { // cmd
      doCmd(c);
      befP++;
    }
  } // runmode

} // loop

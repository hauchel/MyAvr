/* jtag test fot Mega328PB (8Mhz)
  using MiniCore as its PB, might have issues if compiling with other cores (e.g. printf)
        JTAG(1)
  J100-1  TCK   yell  out
  J100-2  GND         GND
  J100-3  TDO   grn   in (!) we are the debugger
  J100-4  VCC         VCC
  J100-5  TMS   ws    out
  J100-6  nSRST
  J100-7  VCC         VCC
  J100-8  -
  J100-9  TDI pink    out
  J100-10 GND         GND
*/

const byte pTCK = 10; //PB2
const byte pTDO = 11; //PB3
const byte pTMS = 12; //PB4
const byte pTDI = 13; //PB5
const byte pSet = 9;  //PB1 0 use H or L to set
const byte pExe = 8;  //PB0 0 Idle  1 exing
//could be changed to direct port access
#define TCKLo digitalWrite(pTCK, LOW)
#define TCKHi digitalWrite(pTCK, HIGH)
#define TMSLo digitalWrite(pTMS, LOW)
#define TMSHi digitalWrite(pTMS, HIGH)
#define TDILo digitalWrite(pTDI, LOW)
#define TDIHi digitalWrite(pTDI, HIGH)

byte verb = 1;    // 1 2
//  4: show Prog
//  8: show low lev JTAG
// 16: show bef exec
uint32_t rea;     // up to 32 bit read from TDO
byte reaCnt;      // # of bits read
byte numbit;      // # of bits to shift out

byte runMode = 0; // 3: take cmd from bef
const byte befM = 100;  // size bef
char  bef[befM];

const byte inpSM = 20;        // Stack for inps
int inpStck[inpSM];           //
byte  inpSP = 0;              //

/*
  Helpers for Serial
*/
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
  Serial.print(F(" "));
  PGM_P p = reinterpret_cast<PGM_P>(ifsh);
  prnt(p);
  Serial.println(n);
}

void errF(const __FlashStringHelper *ifsh, int n) {
  runMode = 0;
  Serial.print(F("ErrF: "));
  PGM_P p = reinterpret_cast<PGM_P>(ifsh);
  prnt(p);
  Serial.println(n);
}

#include "myJtags.h"
#include "myBefs.h"
#include "myBuf.h"

/*
  Execution related
*/
void inpPush(uint16_t was) {
  if (verb & 16) msgF(F("inpPush"), was);
  if (inpSP >= inpSM) {
    errF(F("inp Overflow"), inpSP);
  } else {
    inpStck[inpSP] = was;
    inpSP++;
  }
}

int inpPop() {
  if (verb & 16) msgF(F("inpPop"), inpSP);
  if (inpSP > 0) {
    inpSP --;
    return inpStck[inpSP];
  } else {
    errF(F("inpstack Underflow"), 0);
    return 0;
  }
}

void runne(byte num) {
  if (verb & 16) msgF(F("runne"), num); else Serial.println();
  if (runMode > 0) {    // already running
    prgPush();          // store  current
    befP = -1;          // is inced to 0 in docmd
  } else {  // first call
    befP = 0;           // not inced (!)
    runMode = 3;
  }
  if (num < 255) {    // dont load when immediate exec with y
    if (!pgm2Bef(num)) {
      runMode = 0;
      return;
    }
  }
  digitalWrite(pExe, HIGH); // no harm if already high
}

/*
  JTAG Basics
*/
void jto(byte b) {
  // outputs one bit 0 or 1 on TMS
  if (b == 0) {
    TMSLo;
  } else {
    TMSHi;
  }
  TCKHi; // 13 us
  delayMicroseconds(1); //Scherz
  TCKLo;  //46
}

void jReset() {
  // to Run-Test
  for (int i = 0; i < 5; i++) {
    jto(1);
  }
  jto(0);
}

void toShiftDR() {
  // from RuTe to ShiftDR
  if (verb & 8) msgF(F("to ShiftDR"), 0);
  jto(1);
  jto(0);
  jto(0); // TMS low for the Data
}

void doShift(uint16_t data, byte anz) {
  // in ShiftDR or IR with TMS lo enter anz bit then back to RuTe
  if (verb & 8) msgF(F("doShift"), anz);
  rea = 0;  reaCnt = 0;
  for (int i = 1; i <= anz; i++) {
    if ((data & 1) == 0) TDILo; else TDIHi;
    data = data >> 1;
    if (i == anz) TMSHi;
    TCKHi;
    rea = rea >> 1;
    reaCnt++;                     //12345678
    if (digitalRead(pTDO)) rea |= 0x80000000L ;
    TCKLo;
  }  // in exit1
  jto(1); // in Update
  jto(0); // in Runtest
}

void setInstruct(byte b) {
  // set instruction Register from RuTe
  b = b & 0x0F;
  if (verb & 8) msgF(F("Instruct "), b);
  jto(1);
  jto(1);
  jto(0);
  jto(0); // TMS low for the Data
  doShift(b, 4); // shifts IR, then in RuTe
}

void prog (byte num) {
  // enter 15 bit instruction #num
  // from RuTe, in RuTe when done
  // must be in Program Mode
  uint16_t w;
  byte in;
  char t;
  if (num >= ftxM) {
    errF(F("prog Ftx"), num);
    return;
  }
  w = fuff[num];
  t = pgm_read_byte( (char *)pgm_read_word(&(ftx[num]))); // 1. char Beschreibung
  switch (t) {
    case ' ':
      break;
    case 'K':
      w += idx[cnf].basK;
      break;
    case 'H':
      w += idx[cnf].basH;
      break;
    case 'L':
      w += idx[cnf].basL;
      break;
    case 'D':
      in = inpPop(); //byte now
      w += in;
      break;
    default:
      errF(F("prog Typ"), byte(t));
      return;
  } // case
  if (verb & 4) Serial.printf(F("prog  %2u %c %4X \n"), num, t, w);
  toShiftDR();
  doShift(w, 15); // in RuTe
}

void progf (byte num, uint16_t plu) {
  // enter 15 bit instruction #num with w+plu
  // from RuTe, in RuTe when done
  // must be in Program Mode
  uint16_t w;
  if (num >= ftxM) {
    errF(F("prog Ftx"), num);
    return;
  }
  w = fuff[num] + plu;
  if (verb & 4) Serial.printf(F("progf %2u %4X \n"), num, w);
  toShiftDR();
  doShift(w, 15); // in RuTe
}


byte lowRea() {
  // returns low byte of adjusted Rea
  uint32_t rex;
  rex = rea >> (32 - reaCnt);
  return rex & 0xFF;
}

void readData(byte anz) {
  // reads and stores anz words to buffer
  // from RuTe, in RuTe when done
  // must be in Program Mode
  // start low 0 or 128, hi not inced

  uint8_t lo, hi, ex, dl, dh, pt;
  lo = idx[cnf].basL;
  hi = idx[cnf].basH;
  ex = idx[cnf].basK;

  if (anz > bufM / 2) {
    errF(F("prog bufm?"), anz);
    return;
  }

  pt = 0;
  prog(0);
  progf(1, ex);
  progf(2, hi);

  for (int i = 1; i <= anz; i++) {
    progf(3, lo);
    prog(5);
    prog(6);  //low
    dl = lowRea();
    ramBuff[pt++] = dl;
    prog(7);  //Hi
    dh = lowRea();
    ramBuff[pt++] = dh;
    lo += 2;
    // hi inc?
  }
}

void writeData(byte anz) {
  // writes data from buffer to flash
  // from RuTe, in RuTe when done
  // must be in Program Mode

  uint8_t lo, hi, ex, dl, dh, pt;
  lo = idx[cnf].basL;
  hi = idx[cnf].basH;
  ex = idx[cnf].basK;

  if (anz > bufM / 2) {
    errF(F("Write bufm?"), anz);
    return;
  }

  pt = 0;
  prog(4);
  progf(1, ex);
  progf(2, hi);

  for (int i = 1; i <= anz; i++) {
    if (verb & 4) Serial.printf(F("write %2u at %2x %2x \n"), i, lo, hi);
    progf(3, lo);
    dl = ramBuff[pt++];
    progf(8, dl);
    dh = ramBuff[pt++];
    progf(9, dh);
    // latch data now or later?
    lo += 2;
    // no hi inc
  }
  // latch
  prog(20);
  prog(21);
  prog(22);
  // write
  prog(23);
  prog(24);
  prog(25);
  prog(26);
}

/*
  Misc
*/
void setActive () {
  pinMode(pTCK, OUTPUT);
  TCKLo;
  pinMode(pTDI, OUTPUT);
  pinMode(pTMS, OUTPUT);
  pinMode(pTDO, INPUT_PULLUP);
  pinMode(pSet, OUTPUT);
  digitalWrite(pSet, LOW);
  pinMode(pExe, OUTPUT);
  digitalWrite(pExe, LOW);
}

void setPassive () {
  pinMode(pTCK, INPUT);
  pinMode(pTDI, INPUT);
  pinMode(pTMS, INPUT);
  pinMode(pTDO, INPUT);
  pinMode(pSet, OUTPUT);
  pinMode(pExe, OUTPUT);
}

void showOut() {
  // have to adjust
  //Serial.printf(F("\bRea befor hex %lX  cnt %2u\n"), rea, reaCnt);
  rea = rea >> (32 - reaCnt);
  Serial.printf(F("\bOut (%2u) %lX \n"), reaCnt, rea);
  rea = 0; // as changed anyway
  reaCnt = 0;
}

void showRegs() {
  Serial.println();
  Serial.printf(F("PORTB: %02X \n"), PORTB & 0x3F);
  Serial.printf(F("PINB : %02X \n"), PINB & 0x3F);
  Serial.printf(F("DDRB : %02X \n"), DDRB & 0x3F);
}

void monitor() {
  byte old = 0;
  byte cur;
  setPassive();
  showRegs();
  Serial.println(F("Monitor, Reset to exit"));
  while (1) {
    cur = PINB & 0x3F;
    if (old != cur) {
      old = cur;
      Serial.printf(F("%02X "), old);
    }
  }
}

bool doCmd(unsigned char c) {
  static uint16_t  inp;              // numeric input
  static bool inpAkt = false;       // true if last input was a number (is returned)
  static byte cmdMode = 0;          // 1 config
  bool weg = false;

  if (cmdMode == 1) {
    doBufCmd(c, inp);
    cmdMode = 0;
    return false;
  }

  // handle numbers
  if ( c == 8) { //backspace removes last digit
    weg = true;
    inp = inp / 10;
    return inpAkt;
  }
  if ((c >= '0') && (c <= '9')) {
    weg = true;
    if (inpAkt) {
      inp = inp * 10 + (c - '0');
    } else {
      inpAkt = true;
      inp = c - '0';
    }
    return inpAkt;
  }
  inpAkt = false;
  switch (c) {
    case ' ':
      break;
    case 'a':
      setActive();
      break;
    case 'b': //
      cmdMode = 1;
      Serial.print("\b>");
      break;
    case 'c': //
      cnf = inp;
      msgF(F("config now"), cnf);
      showConfig(cnf);
      break;
    case 'd':
      toShiftDR();
      break;
    case 'e':
      doShift(inp, numbit);
      break;
    case 'g':
      pgm2Bef(inp);
      showBef();
      break;
    case 'i':
      showRegs();
      break;
    case 'h':
      idx[cnf].basH = inp;
      msgF(F("basH"), inp);
      break;
    case 'j':
      setInstruct(inp);
      break;
    case 'k':
      idx[cnf].basK = inp;
      msgF(F("basK"), inp);
      break;
    case 'l':
      idx[cnf].basL = inp;
      msgF(F("basL"), inp);
      break;
    case 'm':
      monitor();
      break;
    case 'n':
      numbit = inp;
      //msgF(F("numbit"), numbit);
      break;
    case 'o':
      showOut();
      break;

    case 'p':
      prog(inp);
      break;
    case 'r':
      readData(64);
      showBuff();
      break;
    case 'R':
      readData(inp);
      showBuff();
      break;

    case 't':
      if (verb & 8) msgF(F("to Runtest"), 0);
      jReset();
      break;

    case 'v':
      verb = inp;
      msgF(F("Verbo"), verb);

      break;
    case 'w':
      writeData(64);
      break;
    case 'W':
      writeData(inp);
      break;
    case 'x':  // load and run
      runne(inp);
      break;
    case 'y':  // run current
      runne(255);
      break;
    case 'z':
      showTxtAll();
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
    case 'D':
      TMSLo;
      msgF(F("TMS"), 0);
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
    case 'M':
      digitalWrite(pExe, HIGH);
      msgF(F("Exe"), 1);
      break;
    case 'N':
      digitalWrite(pExe, LOW);
      msgF(F("Exe"), 0);
      break;

    case '"':   // to bef
      seriBef();
      showBef();
      Serial.println();
      break;
    case 13:
      showBef();
      Serial.println(inp);
      showConfig(cnf);
      break;
    case ',':   //
      inpPush(inp);
      break;
    case '.':   //
      inp = inpPop();
      break;
    case '#':   //
      idx[cnf].basL += 2;
      msgF(F("basL"), idx[cnf].basL);
      break;
    case '+':   //
      idx[cnf].basL += inp;
      msgF(F("basL"), idx[cnf].basL);
      break;

    case 223:   //ß 223 ä 228 ö 246 ü 252
      showStack();
      break;
    case 252:   // ü 252
      showFtxAll();
      break;
    case 246:   // ö 246
      Serial.print("0");
      jto(0);
      break;
    case 228:   //ä 228
      jto(1);
      Serial.print("1");
      break;
    default:
      Serial.print ("?");
      Serial.println (byte(c));
      c = '?';
  } //switch
  return inpAkt;
}

void setup() {
  const char ich[] PROGMEM = "jtag1 " __DATE__  " " __TIME__;
  Serial.begin(38400);
  Serial.println(ich);
  setActive();
  getConfigs();
}

void loop() {
  unsigned char c;

  if (Serial.available() > 0) {
    c = Serial.read();
    Serial.print(char(c));
    if (!doCmd(c)) {
      Serial.print(F(":\b"));
    }
  }

  if (runMode > 0) { //executing
    c = bef[befP];
    if (c == 0) { //eobef
      if (stackP > 0) { // return to caller
        prgPop();
      } else { //end of execution
        digitalWrite(pExe, LOW);
        runMode = 0;
      }
    }  else { // cmd
      doCmd(c);
      befP++;
    }
  } // runmode
} // loop

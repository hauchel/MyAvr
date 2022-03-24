// Bahnsteuerung mit MiniControl
// SCL   A5 yell    1
// SDA   A4 grn     2
// Servo  9 ws
// inmap A0
#include <Servo.h>
#include <EEPROM.h>
#include "texte.h"
#include "helper.h"

const byte servPin = 9;
const byte anzIn = 5;
const byte inMap[anzIn] = {8,    A3,   A2,   A1,   A0};
const bool inInv[anzIn] = {true, true, true, true, false};
bool inVal[anzIn];

bool verbo = true;
bool trace = true;
bool raute = false;
byte traceLin;    // avoid many trace outputs during wait

uint16_t serpos;
byte posp = 0;
struct epromData {
  byte pos[10];
  int16_t wert[10];
};
epromData mypos; //not more than 64 as start at
const uint16_t epromAdr = 960;

const byte progLen = 32;
byte prog[progLen];
byte progp = 0;
byte beflen; // sick, set by decodeprog
unsigned long stepTim[progLen]; // set on entry

const byte anzStack = 16;
byte var1[anzStack];
byte var2[anzStack];
byte progpS[anzStack];
byte prognumS[anzStack];
byte prognum;
byte stackp;

const byte minp = 5;
const byte maxp = 180;

bool mess = false;
const uint16_t messSiz = 50;
unsigned long messTim[messSiz];
uint16_t messAnz = 50;
uint16_t messCnt;
uint16_t messPtr;

Servo myservo;

void showPos() {
  char str[50];
  Serial.println();
  for (byte i = 0; i < 10; i++) {
    sprintf(str, "%2u   %4u   %6d", i, mypos.pos[i], mypos.wert[i]);
    Serial.println(str);
  }
}

void decodProg(char tx[40], byte p) {
  byte lo, hi;
  lo = prog[p] & 0x0F;
  hi = prog[p] >> 4;
  beflen = 1;
  switch (hi) {
    case 0:   //
    case 1:   //
      strcpy_P(tx, txReserved);
      return;
    case 2:   //
      if (lo < 10) {
        sprintf(tx, "%s %2u (%3u) ", "Set Pos", lo, mypos.pos[lo]);
        return;
      }
      switch (lo) {
        case 0xE:
          strcpy_P(tx, txAttach);
          break;
        case 0xF:
          strcpy_P(tx, txDetach);
          break;
        default:
          sprintf(tx, "%s %2u ", "Set Pos invalid", lo);
      }
      return;
    case 3:   //
      sprintf(tx, "%s %2u ", "Wait True", lo);
      return;
    case 4:   //
      sprintf(tx, "%s %2u ", "Wait False", lo);
      return;
    case 5:   //
      sprintf(tx, "%s %2u ", "Skip if True", lo);
      return;
    case 6:   //
      sprintf(tx, "%s %2u ", "Skip if False", lo);
      return;
    case 7:   //
      if (lo < 8)    sprintf(tx, "%s %2u ", "DJNZ 1 to",  lo);
      else sprintf(tx, "%s %2u ", "DJNZ 2 to",  lo - 8);
      return;
    case 8:   //
      sprintf(tx, "%s %4u ", "Delay", 100 * lo);
      return;
    case 0xA:   //
      if (lo < 8)    sprintf(tx, "%s %2u ", "Label",  lo);
      else sprintf(tx, "%s %2u ", "Jump to",  lo - 8);
      return;
    case 0xB:   //
      sprintf(tx, "%s %2u ", "Call Prog", lo);
      return;
    case 0xE:   //
      beflen = 2;
      switch (lo) {
        case 1:
          sprintf(tx, "%s %3u ", "SetVar1 ", prog[p + 1]);
          break;
        case 2:
          sprintf(tx, "%s %3u ", "SetVar2 ", prog[p + 1]);
          break;
        default:
          sprintf(tx, "%s %2u ", "Invalid Ex",  lo);
      }
      return;
    case 0xF:   //
      switch (lo) {
        case 0:
          strcpy_P(tx, txReturn);
          return;
        case 2:
          strcpy_P(tx, txTracOn);
          return;
        case 3:
          strcpy_P(tx, txTracOff);
          return;
        case 0xF:
          strcpy_P(tx, txEndprog);
          return;
        default:
          sprintf(tx, "%s %2u ", "F Special", lo);
      }
      return;
    default:
      sprintf(tx, "%s %2u ", "Define Hi", hi);
      return;
  }
}

void insProg(uint16_t was) {
  if (progp >= progLen) {
    msgF(F("ProgP"), progp);
    return;
  }
  for (byte i = progLen - 1; i > progp; i--) {
    prog[i] = prog[i - 1];
  }
  prog[progp] = byte(was);
  progp++;
}

void delatProg() {
  for (byte i = progp; i < progLen; i++) {
    prog[i] = prog[i + 1];
  }
  prog[progLen - 1] = 255;
}

void showProg() {
  char str[50];
  char txt[40];
  char ind[3];
  unsigned long mess0;
  uint16_t mw;
  Serial.println();
  if (progp >= progLen) progp = progLen - 1; //limit
  mess0 = stepTim[0];
  for (byte i = 0; i < progLen; i++) {
    decodProg(txt, i);
    if (i == progp) {
      strcpy(ind, "->");
    } else {
      strcpy(ind, "  ");
    }
    if (mess) {
      if (stepTim[i] == 0) {
        mw = 0;
      } else {
        mw = stepTim[i] - mess0;
      }
      sprintf(str, "%2u  %02X %3u %3s  %-20s %6u", i, prog[i], prog[i], ind, txt, mw);
    }  else {
      sprintf(str, "%2u  %02X %3u %3s  %-20s", i, prog[i], prog[i], ind, txt);
    }
    Serial.println(str);
    if (prog[i] == 255 ) return;
    if (beflen == 2) i++;
  }
}

void showProgX() {
  char str[10];
  Serial.println();
  for (byte i = 0; i < progLen; i++) {
    sprintf(str, "%u,", prog[i]);
    Serial.print(str);
    if (prog[i] == 255 ) break;
  }
  Serial.println();
}

void readProg(uint16_t  p) {
  prognum = p;
  p = p * 32;
  EEPROM.get(p, prog);
  msgF(F(" Prog read Adr="), p);
}

void writeProg(uint16_t  p) {
  p = p * 32;
  if (p > 0) {
    EEPROM.put(p, prog);
    msgF(F(" Prog write Adr="), p);
  } else {
    msgF(F("Do not Prog write null"), p);
  }
}

void showStack(byte dep) {
  char str[50];
  char ind[3];
  if (dep == 0) dep = anzStack;
  if (dep > anzStack) dep = anzStack;
  Serial.println();
  for (byte i = 0; i < dep; i++) {
    if (i == stackp) {
      strcpy(ind, "->");
    } else {
      strcpy(ind, "  ");
    }
    sprintf(str, "%2u  %3s  %2u %2u  %3u %3u", i, ind,  prognumS[i], progpS[i], var1[i], var2[i]);
    Serial.println(str);
  }
}

byte exepJumto(byte lbl) {
  // lbl A0 to A7
  for (byte i = 0; i < progLen; i++) {
    if (prog[i] == 0xFF) break;
    if (lbl == prog[i]) {
      progp = i + 1;
      return 0;
    }
  }
  msgF(F("Label not found"), lbl);
  return 20;
}

byte exepJump(byte lo) {
  if (lo < 8) return 0; // ignore labels
  byte lbl = lo + 0xA0 - 8;
  return exepJumto(lbl);
}

byte exepDjnz(byte lo) {
  byte lbl;
  if (lo < 8) {
    if (var1[stackp] > 0) var1[stackp]--;
    if (var1[stackp] > 0) {
      lbl = lo + 0xA0;
      return exepJumto(lbl);
    }
  } else {
    if (var2[stackp] > 0) var2[stackp]--;
    if (var2[stackp] > 0) {
      lbl = lo + 0xA0 - 8;
      return exepJumto(lbl);
    }
  }
  return 0;
}

byte exepWait(byte lo, bool tf) {
  if (lo >= anzIn) return 12;
  if (inVal[lo] != tf)   progp -= 1; // keep position
  return 0;
}

byte exepSkip(byte lo, bool tf) {
  if (lo >= anzIn) return 12;
  if (inVal[lo] == tf)   progp += 1;
  return 0;
}


byte exepSpecial(byte lo) {
  switch (lo) {
    case 0:   //
    case 4:   //
      showStack(0);
      return 0;
    case 0xD:   //
      msgF(F("EOP"), progp);
      return 9;
    case 0xE:   //
      msgF(F("EOP"), progp);
      return 9;
    case 0xF:   //
      msgF(F("EOP"), progp);
      return 9;
    default:
      msgF(F("Special not implemented "), lo);
  }
  return 1;
}

byte exep2byte(byte lo) {
  switch (lo) {
    case 1:   //
      var1[stackp] = prog[progp];
      progp++;
      return 0;
    case 2:   //
      var2[stackp] = prog[progp];
      progp++;
      return 0;
    default:
      msgF(F("twobyte not implemented "), lo);
  }
  return 1;
}

byte exepSetPos(byte lo) {
  if (lo < 10) {
    posp = lo; //verwirrt?
    serpos = mypos.pos[lo];
    myservo.write(serpos);
    return 0;
  }
  switch (lo) {
    case 0xF:   //
      myservo.detach();
      break;
    case 0xE:   //
      myservo.attach(servPin);
      break;
    default:
      msgF(F("Setpos not implemented "), lo);
      return 1;
  }
  return 0;
}

byte execOne() {
  byte lo;
  byte hi;
  if (progp >= progLen) {
    msgF(F(" ExecProg progp? "), progp);
    return 10;
  }
  lo = prog[progp];
  progp++;
  hi = lo >> 4;
  lo = lo & 0x0F;
  switch (hi) {
    case 0:   //
    case 1:   //
      msgF(F("Reserved "), hi);
      return 1;
    case 2:   //
      return exepSetPos(lo);
    case 3:
      return exepWait(lo, true);
    case 4:
      return exepWait(lo, false);
    case 5:
      return exepSkip(lo, true);
    case 6:
      return exepSkip(lo, false);
    case 7:   //
      return exepDjnz(lo);
    case 8:   //
      delay(100 * lo);
      return 0;
    case 0xA:   //
      return exepJump(lo);
    case 0xB:   //
      prognum = lo;
      return 6;
    case 0xE:   //
      return exep2byte(lo);
    case 0xF:   //
      return exepSpecial(lo);
    default:
      msgF(F("Not implemented "), hi);
      return 1;
  } //case
}

byte execProg() {
  byte err = 0;
  char txt[40];
  char str[50];
  progp = 0;
  stackp = 0;
  prognumS[stackp] = prognum;
  traceLin = 255;
  for (byte i = 0; i < progLen; i++) {
    stepTim[i] = 0;
  }
  Serial.println();
  while (err == 0) {
    if (Serial.available() > 0) return 5;
    einles();

    if (progp != traceLin) {
      traceLin = progp;
      if (mess) {
        stepTim[progp] = millis();
      }
      if (trace) {
        decodProg(txt, progp);
        sprintf(str, "%2u  %02X  %-10s", progp, prog[progp], txt);
        Serial.println(str);
      }
    }
    err = execOne();
    if (err == 6) { // push (prognum set by execOne)
      progpS[stackp] = progp;
      progp = 0;
      stackp++;
      if (stackp >= anzStack) return 25;
      var1[stackp] = var1[stackp - 1];
      var2[stackp] = var2[stackp - 1];
      prognumS[stackp] = prognum;
      readProg(prognum);
      err = 0;
    }
    if (err == 9) { //pop
      if (stackp == 0) return 9;
      stackp--;
      prognum = prognumS[stackp] ;
      progp = progpS[stackp] ;
      readProg(prognum);
      err = 0;
    }
  } // while
  return err;
}

void showMess() {
  char str[60];
  for (byte i = 0; i < messAnz; i++) {
    sprintf(str, "%2u   %10lu", i, messTim[i] );
    Serial.println(str);
  }
}

void showTims() {
  for (byte i = 0; i < messAnz; i++) {
    Serial.print(messTim[i]);
    if (i % 8 == 7) Serial.println();
  }
  Serial.println();
}

void info(byte sta) {
  Serial.println();
  for (byte i = 1; i < anzIn; i++) {  // ignore 0
    if (inVal[i]) {
      Serial.print("T ");
    } else {
      Serial.print("F ");
    }
  }
  Serial.println();
  showStack(sta);
  msgF(F("serpos"), serpos);
  showPos();

}

void einles() {
  for (byte i = 0; i < anzIn; i++) {
    if (inInv[i]) inVal[i] = !digitalRead(inMap[i]);
    else inVal[i] = digitalRead(inMap[i]);
  }
}

void startMess() {
  messCnt = messAnz;
  messPtr = 0;
  mess = true;
}

void prompt() {
  if (raute) {
    Serial.print("#");
    return;
  }
  Serial.print(posp);
  Serial.print(">");
}

void redraw() {
  vt100Clrscr();
  msgF(F("Prognum"), prognum);
  showProg();
}

// Docmd helper
void setServo(byte p) {
  if (p < 10) {
    posp = p;
    msgF(F("Posi"), mypos.pos[p]);
    serpos = mypos.pos[posp];
    myservo.write(serpos);
  } else msgF(F("Invalid Position "), p);
}

bool doRaute(byte tmp) {
  if (raute) {
    if ((tmp >= '0') && (tmp <= '9')) {
      setServo(tmp - '0');
    }  else {
      raute = false;
      if (tmp=='#') return true;
    }
  }
  return raute;
}

void doCmd(byte tmp) {
  byte zwi;
  if (doRaute(tmp)) return;
  if (doNum(tmp)) return;
  tmp = doVT100(tmp);
  if (tmp == 0) return;
  switch (tmp) {
    case 'a':   //
      prlnF(F("attached"));
      myservo.attach(servPin);
      break;
    case 'b':   //
      showProgX();
      break;
    case 13:
      redraw();
      break;
    case 'c':   //
      progp = inp;
      msgF(F("Progp"), progp);
      break;
    case 'd':   //
      prlnF(F("detached"));
      myservo.detach();
      break;
    case 'f':   //
      prlnF(F("Fetch"));
      EEPROM.get(epromAdr, mypos);
      showPos();
      break;
    case 'g':   //
      zwi = execProg();
      msgF(F("execProg is "), zwi);
      break;
    case 'h':   //
      prlnF(F("write Pos"));
      EEPROM.put(epromAdr, mypos);
      break;
    case 'i':   //
      info(inp);
      break;
    case 'l':   //
      break;
    case 'm':   //
      mess = !mess;
      if (mess) {
        prlnF(F("Mess an"));
      } else {
        prlnF(F("Mess aus"));
      }
      break;
    case 'n':   //
      messAnz = inp;
      if (messAnz > messSiz) messAnz = messSiz;
      msgF(F("MessAnz"), messAnz);
      break;
    case 'o':   //
      break;
    case 'p':   //
      serpos = inp;
      msgF(F("Servo"), serpos);
      myservo.write(serpos);
      break;
    case 'r':   //
      progp = 0;
      readProg(inp);
      showProg();
      break;
    case 's':   //
      mypos.pos[posp] = serpos;
      showPos();
      break;
    case 't':   //
      trace = !trace;
      if (trace) {
        prlnF(F("Trace an"));
      } else {
        prlnF(F("Trace aus"));
      }
      break;
    case 'T':   //
      tickMs = inp;
      msgF(F("messMs"), inp);
      showTims();
      break;
    case 'v':   //
      verbo = !verbo;
      if (verbo) {
        prlnF(F("Verbose an"));
      } else {
        prlnF(F("Verbose aus"));
      }
      break;
    case 'w':   //
      writeProg(prognum);
      break;
    case 'W':   //
      writeProg(inp);
      break;
    case 'x':   //
      zwi = execOne();
      if (zwi == 0) redraw();
      else msgF(F("execOne is "), zwi);
      break;
    case 'y':   //
      delatProg();
      redraw();
      break;
    case '+':   //
      serpos += 5;
      msgF(F("Servo"), serpos);
      myservo.write(serpos);
      break;
    case '#':   //
      raute = true;
      setServo(inp);
      break;
    case ',':   //
      insProg(inp);
      Serial.print(",");
      return; //avoid prompt
    case '.':   //
      prog[progp] = byte(inp);
      redraw();
      break;
    case '-':   //
      serpos -= 5;
      msgF(F("Servo"), serpos);
      myservo.write(serpos);
      break;
    case 193:   //up
      progp -= 1;
      redraw();
      break;
    case 194:   //dwn
      progp += 1;
      redraw();
      break;
    default:
      if (verbo) {
        Serial.println();
        Serial.println(tmp);
      } else {
        Serial.print(tmp);
        Serial.println ("?  0..5, +, -, show, verbose");
      }
  } //case
  prompt();
}

void setup() {
  const char info[] = "Bahn " __DATE__  " " __TIME__;
  Serial.begin(38400);
  Serial.println(info);
  for (byte i = 0; i < anzIn; i++) {
    pinMode(inMap[i], INPUT_PULLUP);
  }
  EEPROM.get(epromAdr, mypos);
  prompt();
}


void loop() {
  if (Serial.available() > 0) {
    doCmd( Serial.read());
  } // serial

  currMs = millis();

  if (currMs - prevMs >= tickMs) {
    prevMs = currMs;
    einles();
  } // timer
}

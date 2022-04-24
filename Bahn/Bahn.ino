// Bahnsteuerung mit MiniControl
// mit PCA9685
#define BIG
//ovres:#include <Servo.h>
#include <Adafruit_PCA9685.h>
#include <EEPROM.h>
#include "helper.h"
#include "timer2.h"
// very careful, do not map same pin to different areas.
// Also see the 3 ste Pins  7,8,9 in timer2.h
//  for TWI //  A5 SCL yell   A4 SDA grn
const byte anzServ = 10; // Serv 0 is always stepper!
//ovres: const byte servMap[anzServ] = {0, 5, 6};
const byte anzIn = 5;   // Inputs
const byte inMap[anzIn] = {2,    A3,   A2,   A1,   A0};
const bool inInv[anzIn] = {true, true, true, true, true};
bool inVal[anzIn];      // current value read
const byte anzOut = 3;  // outputs
const byte outMap[anzOut] = {13, 12, 11};
byte delt = 4;

bool verbo = true;
bool raute = false;
uint16_t serpos[anzServ]; // current position
bool atdet[anzServ];      // remember attached/detached
byte sersel = 0;          // selected Servo
byte posp = 0;            // position
struct epromData {
  uint16_t pos[anzServ][10];
};
epromData mypos;
const uint16_t epromAdr = 700;// depends on 1024-len(epromData)


const uint16_t messSiz = 20;
unsigned long messTim[messSiz];
uint16_t messAnz = 20;
uint16_t messCnt;
uint16_t messPtr;

bool movOn;
bool movSet;        // if true next # is slow
uint16_t movCurr;
uint16_t movEnd;
int16_t movDelt;

//ovres: Servo myservo[anzServ];
//use default address 0x40:
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

void writeServo(byte senum, uint16_t wert) {
  if (wert > mypos.pos[senum][9]) {
    wert = mypos.pos[senum][9];
  }
  if (wert < mypos.pos[senum][0]) {
    wert = mypos.pos[senum][0];
  }
  serpos[senum] = wert;
  if (senum == 0) {
    stePosition(wert);
  } else {
    if (atdet[senum]) {
      pwm.setPWM(senum, 0, wert);
      //ovres: myservo[senum].write(wert);
      if (verbo) msgF(F("writeServo"), wert);
    } else {
      if (verbo) msgF(F("Servo detached"), wert);
    }

  }
}

uint16_t readServo(byte senum) {
  if (senum == 0) {
    return stePos;
  } else {
    //ovres: return myservo[senum].read();
    return serpos[senum];
  }
}
void detachServo(byte senum) {
  if (senum == 0) {
    digitalWrite(steEna, HIGH);
  } else {
    //ovres: myservo[senum].detach();
    pwm.setPWM(senum, 0, 0);
  }
  atdet[senum] = false;
}

void attachServo(byte senum) {
  if (senum == 0) {
    digitalWrite(steEna, LOW);
  } else {
    //ovres: myservo[senum].attach(servMap[senum]);
    pwm.setPWM(senum, 0, serpos[senum]);
  }
  atdet[senum] = true;
}

void timServo(byte wert) {
  OCR2A = wert;
}


#include "texte.h"
void showPos() {
  char str[50];
  Serial.println();
  sprintf(str, "akt  %5u ", readServo(sersel));
  Serial.println(str);
  for (byte i = 0; i < 10; i++) {
    sprintf(str, "%2u   %5u ", i, mypos.pos[sersel][i]);
    Serial.println(str);
  }
}

void showStack(byte dep) {
  char str[50];
  char ind[3];
  //if (dep == 0) dep = anzStack;
  if (dep > anzStack) dep = anzStack;
  Serial.println();
  for (byte i = 0; i < dep; i++) {
    if (i == stackp) {
      strcpy(ind, "->");
    } else {
      strcpy(ind, "  ");
    }
    sprintf(str, "%2u  %3s  %2u %2u  %3u %3u  %1u  %1u", i, ind,  prognumS[i], progpS[i], var1[i], var2[i], actIn5[i], actIn6[i]);
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

byte exepActin(byte lo) {
  if (lo < 8) {
    if (lo >= anzIn) return 12;
    actIn5[stackp] = lo;
  } else {
    lo = lo - 8;
    if (lo >= anzIn) return 12;
    actIn6[stackp] = lo;
  }
  return 0;
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
    case 0xA:   //
      msgF(F("Mess A"), progp);
      return 30;
    case 0xB:   //
      msgF(F("Mess B"), progp);
      return 31;
    case 0xC:   //
      msgF(F("Mess C"), progp);
      return 32;
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
    case 0:   //
      delay(10 * prog[progp] - 1);
      break;
    case 1:   //
      var1[stackp] = prog[progp];
      break;
    case 2:   //
      var2[stackp] = prog[progp];
      break;
    case 5:   // stepper
      stePos = prog[progp];
      break;
    default:
      msgF(F("twobyte not implemented "), lo);
      return 1;
  }
  progp++;
  return 0;
}

byte exepSelServo(byte lo) {
  if (lo >= anzServ) {
    return 13;
  }
  sersel = lo;
  return 0;
}

byte exepSetPos(byte lo) {
  if (lo < 10) {
    posp = lo; //verwirrt?
    if (movSet) {
      startMov(lo);
    } else {
      writeServo(sersel, mypos.pos[sersel][lo]);
    }
    return 0;
  }
  switch (lo) {
    case 0xA:   //
      movSet = true;
      break;
    case 0xB:   // Stepp to 0
      stePos = 0;
      break;
    case 0xD:   // wait count 0
      if ((tim2Count > 0) or (movOn))  progp -= 1;
      break;
    case 0xE:   //
      attachServo(sersel);
      break;
    case 0xF:   //
      detachServo(sersel);
      break;
    default:
      msgF(F("Setpos not implemented "), lo);
      return 1;
  }
  return 0;
}

byte execOne() {
  byte lo, hi;
  if (progp >= progLen) {
    msgF(F(" ExecProg progp? "), progp);
    return 10;
  }
  if (actIn5[stackp] > 0) {
    if (inVal[actIn5[stackp]]) {
      actIn5[stackp] = 0;
      return exepJumto(0xA5);
    }
  }
  if (actIn6[stackp] > 0) {
    if (inVal[actIn6[stackp]]) {
      actIn6[stackp] = 0;
      return exepJumto(0xA6);
    }
  }

  lo = prog[progp];
  progp++;
  hi = lo >> 4;
  lo = lo & 0x0F;

  switch (hi) {
    case 0:   //
      msgF(F("Reserved "), hi);
      return 1;
    case 1:   //
      return exepSelServo(lo);
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
    case 8:   //obsolete
      return exepActin(lo);
    case 0xA:   //
      return exepJump(lo);
    case 0xB:   // call
      if (dirty) return 26;
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

    currMs = millis();
    if (currMs - prevMs >= tickMs) {
      if (movOn) handleMov();
      prevMs = currMs;
    }

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
      stackp++;
      if (stackp >= anzStack) return 25;
      var1[stackp] = var1[stackp - 1];
      var2[stackp] = var2[stackp - 1];
      actIn5[stackp] = 0;
      actIn6[stackp] = 0;
      prognumS[stackp] = prognum;
      readProg(prognum, true); // dirty already checked by execone
      err = 0;
    }
    if (err == 9) { //pop
      if (stackp == 0) return 9;
      stackp--;
      prognum = prognumS[stackp] ;
      readProg(prognum, false); // egal
      progp = progpS[stackp] ;
      err = 0;
    }
  } // while
  return err;
}

void handleMov() {
  // called change current servo
  if (movDelt > 0) {
    if (movCurr < movEnd) {
      movCurr += movDelt;
      writeServo(sersel, movCurr);
    } else {
      movOn = false;
      writeServo(sersel, movEnd);
    }

  } else {
    if (movCurr > movEnd) {
      movCurr += movDelt;
      writeServo(sersel, movCurr);
    } else {
      movOn = false;
      writeServo(sersel, movEnd);
    }
  }
}

void startMov(byte z) {
  char str[60];
  movSet = false;
  movCurr = serpos[sersel];
  movEnd = mypos.pos[sersel][z];
  if (movEnd > movCurr) {
    movDelt = delt;
  } else {
    movDelt = -delt;
  }
  sprintf(str, "Mov from %4u to %4u Delt %2d", movCurr, movEnd, movDelt);
  Serial.println(str);
  movOn = true;
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
  for (byte i = 0; i < anzIn; i++) {
    if (inVal[i]) {
      Serial.print("T ");
    } else {
      Serial.print("F ");
    }
  }
  Serial.println();
  showStack(sta);

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
  char str[50];
  if (raute) {
    Serial.print("#");
    return;
  }
  if (teach) {
    sprintf(str, "(%2u) %2u/%1u>", progp, sersel, posp);
  } else {
    sprintf(str, "%2u/%1u>", sersel, posp);
  }
  Serial.print(str);
}



void selectServo(byte b) {
  if (b >= anzServ) b = anzServ - 1;
  sersel = b;
}

void setServo(byte p) {
  if (p < 10) {
    posp = p;
    if (mypos.pos[sersel][p] <= mypos.pos[sersel][9]) {
      msgF(F("Posi"), mypos.pos[sersel][p]);
      if (movSet) {
        startMov(p);
      } else {
        writeServo(sersel, mypos.pos[sersel][posp]);
      }
    } else {
      msgF(F("Not servod "), p);
    }
  } else msgF(F("Invalid Position "), p);
}

bool doRaute(byte tmp) {
  if (raute) {
    if ((tmp >= '0') && (tmp <= '9')) {
      setServo(tmp - '0');
    }  else {
      raute = false;
      if (tmp == '#') return true;
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
      attachServo(sersel);
      progge (0x2E);
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
      detachServo(sersel);
      progge (0x2F);
      break;
    case 'e':   //
      selectServo(inp);
      msgF(F("Servo is "), sersel);
      progge (0x10 + sersel);
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
    case 'j':   //
      timServo(inp);
      msgF(F("OCR2A is "), OCR2A);
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
      movSet = true;
      prlnF(F("movSet on"));
      progge(0x2A);
      setServo(inp);
      progge(0x20 + inp);
      break;
    case 'p':   //
      writeServo(sersel, inp);
      msgF(F("Servo"), serpos[sersel]);
      break;
    case 'q':   //
      mypos.pos[sersel][0] = inp;
      msgF(F("Min set"), inp);
      break;
    case 'Q':   //
      mypos.pos[sersel][9] = inp;
      msgF(F("Max set"), inp);
      break;
    case 'r':   //
      readProg(inp, true);
      showProg();
      break;
    case 'R':   //
      readProg(inp, false);
      showProg();
      break;
    case 's':   //
      mypos.pos[sersel][posp] = serpos[sersel];
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
    case 'V':   //
      steDirPlus = !steDirPlus;
      if (steDirPlus) {
        prlnF(F("steDirPlus true"));
      } else {
        prlnF(F("steDirPlus falses"));
      }
      break;
    case 'w':   //
      writeProg(prognum);
      break;
    case 'W':   //
      writeProg(inp);
      prognum = inp;
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
    case 'z':   //
      progp = 0;
      readProg(inp, true);
      zwi = execProg();
      msgF(F("execProg is "), zwi);
      break;
    case '+':   //
      writeServo(sersel, serpos[sersel] + delt);
      msgF(F("Servo"), serpos[sersel]);
      break;
    case '#':   //
      setServo(inp);
      progge(0x20 + inp);
      break;
    case 39:   // shift #
      raute = true;
      setServo(inp);
      break;
    case ',':   //
      insProg(inp);
      Serial.print(",");
      return; //avoid prompt
    case '.':   //
      prog[progp] = byte(inp);
      dirty = true;
      redraw();
      break;
    case '-':   //
      writeServo(sersel, serpos[sersel] - delt);
      msgF(F("Servo"), serpos[sersel]);
      break;
    case '_':   //
      delt = inp;
      msgF(F("Delt is "), delt);
      break;
    case 193:   //up
      progp -= 1;
      redraw();
      break;
    case 194:   //dwn
      progp += 1;
      redraw();
      break;
    case 228:   // ä
      teach = !teach;
      if (teach) {
        prlnF(F("Teach an"));
      } else {
        prlnF(F("Teach aus"));
        redraw();
      }
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
  for (byte i = 0; i < anzOut; i++) {
    pinMode(outMap[i], OUTPUT);
  }
  EEPROM.get(epromAdr, mypos);
  msgF(F("Progs to "), 16 * progLen);
  steSetup();
  prompt();
  timServo(150);
  pwm.begin();
  pwm.setPWMFreq(50);  //
}


void loop() {
  if (Serial.available() > 0) {
    doCmd( Serial.read());
  } // serial

  currMs = millis();

  if (currMs - prevMs >= tickMs) {
    if (movOn) handleMov();
    prevMs = currMs;
    einles();
  } // timer
}

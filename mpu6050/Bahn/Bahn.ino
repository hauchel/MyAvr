// Bahnsteuerung mit MiniControl
// SCL   A5 yell    1
// SDA   A4 grn     2
// Servo  9 ws
// inmap A0
#include <Servo.h>
#include <EEPROM.h>

const byte servPin = 9;
const byte anzIn = 5;
const byte inMap[anzIn] = {8, A3, A2, A1, A0};
bool inVal[anzIn];


uint16_t inp;
uint16_t serpos;
bool inpAkt;
unsigned long currMs, prevMs = 0;
bool verbo = true;

byte posp = 0;
struct epromData {
  byte pos[10];
  int16_t wert[10];
};
epromData mypos; //not more than 32 as progs start at

const byte progLen = 32;
byte prog[progLen];
byte progp = 0;

const byte minp = 5;
const byte maxp = 180;

bool mess = false;
const uint16_t messSiz = 100;
int16_t messX[messSiz], messY[messSiz], messZ[messSiz];
uint8_t tims[messSiz];
uint16_t messAnz = 50;
uint16_t messCnt;
uint16_t messPtr;
unsigned long messMs = 10;

Servo myservo;

char result[7];
char* toStr(int16_t character) { // converts int16 to string and formatting
  sprintf(result, "%6d", character);
  return result;
}

// Memory saving helpers
void prnt(PGM_P p) {
  // flash to serial until \0
  while (1) {
    char c = pgm_read_byte(p++);
    if (c == 0) break;
    Serial.write(c);
  }
  Serial.write(' ');
}

void msgF(const __FlashStringHelper *ifsh, int16_t n) {
  PGM_P p = reinterpret_cast<PGM_P>(ifsh);
  prnt(p);
  Serial.println(n);
}

void prnF(const __FlashStringHelper *ifsh) {
  PGM_P p = reinterpret_cast<PGM_P>(ifsh);
  prnt(p);
}

void prlnF(const __FlashStringHelper *ifsh) {
  prnF(ifsh);
  Serial.println();
}

void showPos() {
  char str[50];
  Serial.println();
  for (byte i = 0; i < 10; i++) {
    sprintf(str, "%2u   %4u   %6d", i, mypos.pos[i], mypos.wert[i]);
    Serial.println(str);
  }
}

void decodProg(char  tx[20], byte b) {
  strcpy(tx, "abc");
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
  char txt[20];
  char ind[3];
  Serial.println();
  for (byte i = 0; i < progLen; i++) {
    decodProg(txt, prog[i]);
    if (i == progp) {
      strcpy(ind, "->");
    } else {
      strcpy(ind, "  ");
    }
    sprintf(str, "%2u  %02X %1s  %-10s", i, prog[i], ind, txt);
    Serial.println(str);
    if (prog[i] == 255 ) return;
  }
}

void fetchProg(uint16_t  p) {
  p = p * 32;
  EEPROM.get(p, prog);
  msgF(F(" Prog fetch Adr="), p);
  showProg();
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

void execProg() {
  byte lo;
  byte hi;
  if (progp >= progLen) {
    msgF(F(" ExecProg progp? "), progp);
    return;
  }
  lo = prog[progp];
  hi = lo >> 4;
  lo = lo & 0x0F;
  switch (hi) {
    case 0:   //
    case 1:   //
      msgF(F("Reserved "), hi);
      break;
    case 2:   // set Pos
      msgF(F("Set Pos "), lo);
      break;
    default:
      msgF(F("Not implemented "), hi);
      break;
  } //case
}


void showMess() {
  char str[60];
  for (byte i = 0; i < messAnz; i++) {
    sprintf(str, "%2u   %6d  %6d  %6d", i, messX[i], messY[i], messZ[i]);
    Serial.println(str);
  }
}

void showTims() {
  for (byte i = 0; i < messAnz; i++) {
    Serial.print(toStr(tims[i]));
    if (i % 8 == 7) Serial.println();
  }
  Serial.println();
}

void info() {
  msgF(F("messMs"), messMs);
  for (byte i = 0; i < anzIn; i++) {
    if (inVal[i]) {
      Serial.print("T ");
    } else {
      Serial.print("F ");
    }
  }
  Serial.println();
}

void einles() {
  for (byte i = 0; i < anzIn; i++) {
    inVal[i] = !digitalRead(inMap[i]);
  }
}

void startMess() {
  messCnt = messAnz;
  messPtr = 0;
  mess = true;
}

void prompt() {
  Serial.print(posp);
  Serial.print(">");
}

void doCmd(byte tmp) {
  if ((tmp >= '0') && (tmp <= '9')) {
    if (inpAkt) {
      inp = inp * 10 + (tmp - '0');
    } else {
      inpAkt = true;
      inp = tmp - '0';
    }
    Serial.print("\b\b\b\b");
    Serial.print(inp);
    return;
  }
  inpAkt = false;

  switch (tmp) {
    case 'a':   //
      prlnF(F("attached"));
      myservo.attach(servPin);
      break;
    case 'b':   //
      showProg();
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
      progp = 0;
      fetchProg(inp);
      break;
    case 'g':   //
      msgF(F("Go"), progp);
      break;
    case 'h':   //
      writeProg(inp);
      break;
    case 'i':   //
      info();
      break;
    case 'l':   //
      break;
    case 'm':   //
      msgF(F("Mess"), mess);
      startMess();
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
      prlnF(F("read"));
      EEPROM.get(0, mypos);
      showPos();
      break;
    case 's':   //
      mypos.pos[posp] = serpos;
      showPos();
      break;
    case 't':   //
      messMs = inp;
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
      prlnF(F("write"));
      EEPROM.put(0, mypos);
      break;
    case 'x':   //
      execProg();
      break;
    case 'y':   //
      delatProg();
      showProg();
      break;
    case '+':   //
      serpos += 5;
      msgF(F("Servo"), serpos);
      myservo.write(serpos);
      break;
    case '#':   //
      if (inp < 10) {
        posp = inp;
        msgF(F("Posi"), mypos.pos[posp]);
        serpos = mypos.pos[posp];
        myservo.write(serpos);
      } else msgF(F("?inp "), inp);
      break;
    case ',':   //
      insProg(inp);
      break;
    case '-':   //
      serpos -= 5;
      msgF(F("Servo"), serpos);
      myservo.write(serpos);
      break;

    default:
      Serial.print(tmp);
      Serial.println ("?  0..5, +, -, show, verbose");
  } //case
  prompt();
}

void setup() {
  const char info[] = "QMC5883 " __DATE__  " " __TIME__;
  Serial.begin(38400);
  Serial.println(info);
  for (byte i = 0; i < anzIn; i++) {
    pinMode(inMap[i], INPUT_PULLUP);
  }
  prompt();
}


void loop() {
  if (Serial.available() > 0) {
    doCmd( Serial.read());
  } // serial

  currMs = millis();

  if (currMs - prevMs >= messMs) {
    prevMs = currMs;
    einles();
  } // timer
}

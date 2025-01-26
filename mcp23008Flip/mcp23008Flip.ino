// play Flipdot

// SCL   A5 yell    1
// SDA   A4 grn     2
// funny: works w/o pullups?

// Leistungsteile 7 V
// Spalte Bits 0 to 6  -> L293D (Beschriftungsseite Brose) 0=Schwarz, 1=Gelb, Enable pulst
// Zeile 0  7 -> 74HCT138 680 Ohm ->  PhotoMos das VCC schaltet (orange), wenn enabled werden alle 0 = schwarz gesetzt
//       8 15 -> 74HCT238         ->  ULN2803 das GND schaltet (grau), werden alle 1 = gelb gesetzt
//
#include "helper.h"
#include <Wire.h>

const byte pEnaSpa = 9;  // Enable L293 with Hi
const byte pEnaZei = 8;  // Enable HC138
uint16_t dauer = 15;     // pulse Spalte in about 10th of ms
bool verbo = false;
byte runprog = 0;
uint16_t progwait = 2000;  // wait between steps in 10th of ms
byte zeile = 0;
byte mcpDev = 0x27;  // TWI-ADDR fr test
byte mcpZei = 0x20;
byte mcpSpa = 0x27;

const byte zMax = 5;
// 15 bit values for nums    0         2     3         5
const uint16_t ziff[10] = { 31599, 1, 29671, 31207, 4, 31183 };
byte wert[zMax];

#define ENAZei digitalWrite(pEnaZei, LOW);
#define DISZei digitalWrite(pEnaZei, HIGH);
#define ENASpa digitalWrite(pEnaSpa, HIGH);
#define DISSpa digitalWrite(pEnaSpa, LOW);
#define BIT4Null zeile & 0xF7
#define BIT4Eins zeile | 0x08

void checkEot() {
  byte t = Wire.endTransmission();
  //if (verbo) msgF(F("End Tr"), t);
  if (t != 0) msgF(F("End Tr Error"), t);
}

void setMcp(byte reg, byte val) {
  Wire.beginTransmission(mcpDev);
  Wire.write(reg);
  Wire.write(val);
  checkEot();
}

void setSpalte(byte val) {
  Wire.beginTransmission(mcpSpa);
  Wire.write(9);
  Wire.write(val);
  checkEot();
}

void setZeile(byte val) {
  Wire.beginTransmission(mcpZei);
  Wire.write(9);
  Wire.write(val);
  checkEot();
  if (verbo) msgF(F("Zei"), val);
}

byte getMcp(byte reg) {
  Wire.beginTransmission(mcpDev);
  Wire.write(reg);
  checkEot();
  Wire.requestFrom((uint8_t)mcpDev, (uint8_t)1);
  return Wire.read();
}

void setupMcp(byte dev) {
  // as output
  mcpDev = dev;
  setMcp(0, 0);  // IODIR output
  setMcp(9, 0);  // GPIO value
}

void mydelay(uint16_t mst) {
  // about 1/10th of a ms
  while (mst > 0) {
    _delay_us(100);
    mst--;
  }
}

void pulse() {
  // für Spalte
  ENASpa;
  mydelay(dauer);
  DISSpa;
}

void strobe() {
  setZeile(BIT4Null);
  pulse();
  setZeile(BIT4Eins);
  pulse();
  // disable Photomos
}

bool delayOrKey(uint16_t mst) {
  while (mst > 0) {
    if (Serial.available() > 0) return true;
    _delay_us(100);
    mst--;
  }
  return false;
}

void scanne() {
  int nmcps = 0;
  Serial.println(F("Scanning..."));
  for (byte address = 1; address < 127; ++address) {
    Wire.beginTransmission(address);
    byte error = Wire.endTransmission();
    if (error == 0) {
      Serial.print(F("I2C found at address 0x"));
      if (address < 16) {
        Serial.print("0");
      }
      Serial.println(address, HEX);
      ++nmcps;
    } else if (error == 4) {
      Serial.print(F("Unknown error at address 0x"));
      if (address < 16) {
        Serial.print("0");
      }
      Serial.println(address, HEX);
    }
  }
  if (nmcps == 0) {
    Serial.println("No I2C found\n");
  } else {
    Serial.println("done\n");
  }
}


uint8_t prog1() {
  static uint8_t w;
  w = w << 1;
  if (w == 0) w = 1;
  setSpalte(w);
  strobe();
  if (verbo) msgF(F("Prog1"), w);
  if (delayOrKey(progwait)) runprog = 0;
  return w;
}

uint8_t prog2() {
  static uint8_t w;
  w = w << 1;
  if (w == 0) w = 1;
  setSpalte(w);
  if (w == 128) {  // clear
    setZeile(BIT4Null);
  } else {
    setZeile(BIT4Eins);
  }
  pulse();
  if (verbo) msgF(F("Prog2"), w);
  if (delayOrKey(progwait)) runprog = 0;
  return w;
}

uint8_t prog3() {
  static uint8_t w;
  w = w << 1;
  if (w == 0) {
    setSpalte(w);
    setZeile(BIT4Null);
    pulse();
    w = 1;
  } else {
    setZeile(BIT4Eins);
    pulse();
  }
  if (verbo) msgF(F("Prog3"), w);
  if (delayOrKey(progwait)) runprog = 0;
  return w;
}

uint8_t prog4() {
  static uint8_t myz;
  byte zs = zeile;
  zeile = myz;
  if (prog2() == 128) {
    myz++;
    if (myz >= zMax) myz = 0;
  }
  zeile = zs;
  return myz;
}

uint16_t aggr() {
  uint16_t f = 1;
  uint16_t su = 0;
  for (byte i = 0; i < zMax; i++) {
    su += f * (wert[i] & 0x7);
    f = f * 8;
  }
  return su;
}

void agg2wert(uint16_t su) {
  for (byte i = 0; i < zMax; i++) {
    wert[i] = su & 0x7;
    su = su >> 3;
  }
}

void shiftL() {
  for (byte i = 0; i < zMax; i++) {
    wert[i] = (wert[i] >> 1);
  }
}

void shiftR() {
  for (byte i = 0; i < zMax; i++) {
    wert[i] = (wert[i] << 1);
  }
}

void trans() {
  byte zs = zeile;
  for (zeile = 0; zeile < zMax; zeile++) {
    setSpalte(wert[zeile]);
    strobe();
  }
  zeile = zs;
}

void prompt() {
  char str[50];
  sprintf(str, "Z%2u y%3u", zeile, wert[zeile]);
  if (digitalRead(pEnaZei) == 0) strcat(str, " ZEI ");
  if (digitalRead(pEnaSpa) == 1) strcat(str, " SPA ");
  strcat(str, "> ");
  Serial.print(str);
}

void doCmd(byte cmd) {
  if (doNum(cmd)) {
    Serial.print(char(cmd));
    return;
  }
  switch (cmd) {
    case 'a':
      agg2wert(inp);
      trans();
      prnF(F("\n"));
      break;
    case 'b':
      agg2wert(ziff[inp]);
      trans();
      prnF(F("\n"));
      break;
    case 'd':
      DISZei;
      prnF(F("Zei Dis\n"));
      break;
    case 'e':
      ENAZei;
      prnF(F("Zei Ena\n"));
      break;
    case 'E':
      DISZei;  // avoid shortcut
      ENASpa;
      prnF(F("Spa Ena!!\n"));
      break;
    case 'f':
      dauer = inp;
      msgF(F("Dauer"), dauer);
      break;
    case 'g':  //
      msgF(F("Get"), getMcp(inp));
      break;
    case 'n':
      setSpalte(inp);
      msgF(F("Spalte "), inp);
      break;
    case 'o':  //
      setSpalte(inp);
      strobe();
      prnF(F("Stro\n"));
      break;
    case 'p':  //
      pulse();
      msgF(F("Pulse"), dauer);
      break;
    case 'R':
      setup();
      break;
    case 'S':  //
      scanne();
      break;
    case 't':
      trans();
      prnF(F("\n"));
      break;
    case 'u':
      mcpDev = inp;
      msgF(F("mcpDev now"), mcpDev);
      break;
    case 'v':  //
      verbo = !verbo;
      if (verbo) {
        prnF(F("Verbose an\n"));
      } else {
        prnF(F("Verbose aus\n"));
      }
      break;
    case 'w':
      progwait = inp;
      msgF(F("Progwait"), progwait);
      break;
    case 'x':
      runprog = inp;
      msgF(F("Prog"), runprog);
      break;
    case 'y':
      wert[zeile] = inp;
      msgF(F("Agg"), aggr());
      break;
    case 'z':
      if (inp < zMax) zeile = inp;
      setZeile(zeile);
      msgF(F("Zeile"), zeile);
      break;
    case 13:
      vt100Clrscr();
      break;
    case '+':
      zeile += 1;
      if (zeile >= zMax) zeile = 0;
      prnF(F("\n"));
      break;
    case '-':
      if (zeile == 0) zeile = zMax;
      zeile--;
      prnF(F("\n"));
      break;
    case '>':
      shiftR();
      trans();
      prnF(F("\n"));
      break;
    case '<':
      shiftL();
      trans();
      prnF(F("\n"));
      break;
    case '#':
      inp = 255;
      msgF(F("inp"), inp);
      break;
    default:
      Serial.print(cmd);
      prnF(F("?  scan, direction, port, olat, nread, verbose"));
  }  //case
  prompt();
}

void setup() {
  const char info[] = "MCP23008Flip " __DATE__ " " __TIME__;
  pinMode(pEnaSpa, OUTPUT);  // disable  Spalte
  DISSpa;
  pinMode(pEnaZei, OUTPUT);  // disable
  DISZei;
  Serial.begin(38400);
  Serial.println(info);
  Wire.begin();
  setupMcp(mcpZei);
  setupMcp(mcpSpa);
  prompt();
}

void loop() {
  if (Serial.available() > 0) {
    doCmd(Serial.read());
  }  // serial

  if (runprog > 0) {
    switch (runprog) {
      case 1:
        prog1();
        break;
      case 2:
        prog2();
        break;
      case 3:
        prog3();
        break;
      case 4:
        prog4();
        break;
      default:
        msgF(F("invalid prog"), runprog);
        runprog = 0;
    }  // case
  }    // if

  currMs = millis();
  if (currMs - prevMs > 1000) {
    prevMs = currMs;
  }  // timer
}

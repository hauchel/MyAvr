// many io's for L293D

// SCL   A5 yell    1
// SDA   A4 grn     2
// Enable active
// funny: works w/o pullups?
// Leistungsteile 7 V
// Spalte Bits 0 to 6  -> L293D (Beschriftungsseite Brose) 0=Schwarz, 1=Gelb, Enable pulst
// Zeile -> 74HC138 680 Ohm ->  1 PhotoMos das VCC schaltet (orange), wenn enabled werden alle 0 = schwarz gesetzt
//                              2 PhotoMos das GND schaltet (orange), werden alle 1 = gelb gesetzt

#include "helper.h"
#include <Wire.h>

const byte pEna = 9;
uint16_t dauer = 10;  // pulse Spalte in 10th of ms
bool verbo = false;
byte runprog = 0;
uint16_t progwait = 5000; // wait between steps in 10th of ms
byte zeile = 0;
byte mcpDev = 0x27; // TWI-ADDR for test
byte mcpZei = 0x20;
byte mcpSpa = 0x27;

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

void setSpalte( byte val) {
  Wire.beginTransmission(mcpSpa);
  Wire.write(9);
  Wire.write(val);
  checkEot();
}

void setZeile( byte val) {
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
  setMcp (0, 0); // IODIR output
  setMcp (9, 0); // GPIO value
}

void mydelay(uint16_t mst ) {
  // about 1/10th of a ms
  while (mst > 0) {
    _delay_us(100);
    mst--;
  }
}

void pulse() {
  // für Spalte
  digitalWrite(pEna, HIGH);
  mydelay(dauer);
  digitalWrite(pEna, LOW);
}

void strobe() {
  setZeile(zeile & 0xFE);
  pulse();
  setZeile(zeile | 0x01);
  pulse();
  setZeile(0);  // disable Photomos
}

bool delayOrKey (uint16_t mst) {
  while (mst > 0) {
    if ( Serial.available() > 0) return true;
    _delay_us(100);
    mst--;
  }
  return false;
}

void setDisa() {
  msgF(F("Disabled"), 0);

}

void setEna() {
  msgF(F("Enabled"), 0);
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
      Serial.print(address, HEX);
      Serial.println("  !");
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
  static uint8_t  w;
  w = w << 1;
  if (w == 0) w = 1;
  setSpalte(w);
  strobe();
  if (verbo) msgF(F("Prog1"), w);
  if (delayOrKey(progwait)) runprog=0;
  return w;
}

void doCmd(byte cmd) {
  if (doNum(cmd)) {
    Serial.print(char(cmd));
    return;
  }
  switch (cmd) {

    case 'i':   // bit set -> is input
      setMcp(0, inp);
      msgF(F("0 Direction"), inp);
      break;
    case 'd':
      setDisa();
      break;
    case 'e':
      setEna();
      break;
    case 'f':
      dauer = inp;
      msgF(F("Dauer"), dauer);
      break;
    case 'g':   //
      msgF(F("Get"),  getMcp(inp));
      break;
    case 'n':
      setSpalte(inp);
      msgF(F("Spalte "), inp);
      break;
    case 'o':   //
      setSpalte(inp);
      strobe();
      if (verbo) msgF(F("Strobe done"), inp);
      break;
    case 'p':   //
      pulse();
      msgF(F("Pulse"), dauer);
      break;
    case 'r':
      break;
    case 's':   //
      scanne();
      break;
    case 'u':
      mcpDev = inp;
      msgF(F("mcpDev now"), mcpDev);
      break;
    case 'v':   //
      verbo = !verbo;
      if (verbo) {
        prnF(F("Verbose an"));
      } else {
        prnF(F("Verbose aus"));
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
      break;
    case 'z':
      zeile = inp;
      setZeile(zeile);
      msgF(F("Zeile"), zeile);
      break;
    case '#':
      inp = 255;
      msgF(F("inp"), inp);
      break;
    default:
      Serial.print(cmd);
      prnF(F("?  scan, direction, port, olat, nread, verbose"));
  } //case
}

void setup() {
  const char info[] = "MCP23008Flip " __DATE__  " " __TIME__;
  pinMode(pEna, OUTPUT);  // disable  Spalte
  digitalWrite(pEna, LOW);
  Serial.begin(38400);
  Serial.println(info);
  Wire.begin();
  setupMcp ( mcpZei);
  setupMcp ( mcpSpa);
}

void loop() {
  if (Serial.available() > 0) {
    doCmd( Serial.read());
  } // serial

  if (runprog > 0) {
    switch (runprog) {
      case 1:
        prog1();
        break;
      default:
        runprog = 0;

    }  // case
  } // if

  currMs = millis();
  if (currMs - prevMs > 1000) {
    prevMs = currMs;
  } // timer
}

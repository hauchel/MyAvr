// many io's

// SCL   A5 yell    1
// SDA   A4 grn     2
// funny: works w/o pullups?
#include "helper.h"
#include <Wire.h>
unsigned long currMs, prevMs = 0;
bool verbo = true;
bool plumi = true;
const byte mcpDev = 0x27; // TWI-ADDR
byte mcpVal = 255; // Enable and Dir High


byte steSel = 0;
const byte nStp = 4;

void msg(const char txt[], int n) {
  Serial.print(txt);
  Serial.print(" ");
  Serial.println(n);
}

byte setMcp( byte reg, byte val) {
  Wire.beginTransmission(mcpDev);
  Wire.write(reg);
  Wire.write(val);
  return Wire.endTransmission();     // stop transmitting
}

byte getMcp(byte reg) {
  // delayMicroseconds(2000);
  Wire.beginTransmission(mcpDev);
  Wire.write(reg);
  Wire.endTransmission();
  Wire.requestFrom((uint8_t)mcpDev, (uint8_t)1);
  return Wire.read();
}

/* Test of
  Using TWI mcp23008 8 bit for Enable and Dir:
  32103210
  Ena Dir
*/

void setLi() {
  msg("Links High", steSel);
  bitSet(mcpVal, steSel);
  setMcp(9, mcpVal);

}

void setRe() {
  msg("Rechts Low", steSel);
  bitClear(mcpVal, steSel);
  setMcp(9, mcpVal);
}


void setDisa() {
  msg("Disabled", steSel);
  bitSet(mcpVal, steSel + 4);
  setMcp(9, mcpVal);
}

void setEna() {
  msg("Enabled", steSel);
  bitClear(mcpVal, steSel + 4);
  setMcp(9, mcpVal);
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

void doCmd(byte cmd) {
   if (doNum(cmd)) {
    Serial.print(char(cmd));
    return;
  }
  switch (cmd) {
    case 's':   //
      scanne();
      break;
    case 'i':   // bit set -> is input
      setMcp(0, inp);
      msg("0 Direction", inp);
      break;
    case 'd':
      setDisa();
      break;
    case 'e':
      setEna();
      break;
    case 'g':   //
      msg("Get",  getMcp(inp));
      break;
    case 'l':
      setLi();
      break;
    case 'o':   //
      setMcp(9, inp);
      msg("10 OLAT", inp);
      break;
    case 'p':   //
      setMcp(9, inp);
      msg("9 Port", inp);
      break;
    case 'r':
      setRe();
      break;
    case 'v':   //
      verbo = !verbo;
      if (verbo) {
        Serial.println("Verbose an");
      } else {
        Serial.println("Verbose aus");
      }
      break;
    case 'y':
      steSel = inp;
      if (steSel >= nStp) {
        steSel = 0;
      }
      msg("SteSel", steSel);
      break;
    default:
      Serial.print(tmp);
      Serial.println ("?  scan, direction, port, olat, nread, verbose");
  } //case
  Serial.println(mcpVal, HEX);
}

void setup() {
  const char info[] = "MCP23008 " __DATE__  " " __TIME__;
  Serial.begin(38400);
  Serial.println(info);
  Wire.begin();
}


void loop() {
  if (Serial.available() > 0) {
    doCmd( Serial.read());
  } // serial

  currMs = millis();
  if (currMs - prevMs > 1000) {
    prevMs = currMs;
  } // timer
}

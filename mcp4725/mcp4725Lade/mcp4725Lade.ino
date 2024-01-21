// MCP4725  D/A Konverter
// sets out voltage 
// INA219   Spannungs/Strom Messer
// MAX2719  8 * 7 seg Led
/*
  D9  DataIn    white
  D8  CS/Load   grey
  D7  CLK       pink
  A4  SDA       green
  A5  SCL       blue
*/

#include <EEPROM.h>
#include <Wire.h>
#include <Adafruit_INA219.h>
Adafruit_INA219 ina219;
#include <LedCon5.h>
const byte anzR = 1; // number of LED Disps
//             dataPin, clkPin, csPin, numDevices
LedCon5 lc = LedCon5(9, 7, 8, anzR);

byte intens = 3;

#define MCP4725_CMD_WRITE_FAST_MODE   0x00
#define MCP4725_CMD_WRITEDAC          0x40
#define MCP4725_CMD_WRITEDAC_EEPROM   0x60
uint8_t mcpAdr = 0x60;

uint16_t inp;
bool inpAkt = false;
bool inParam = false;
int eadr = 0;             // EEprom Addr
bool verbo = false;
// analog
uint16_t av1, oav1;
int outV;         // DAC-Value to set (0..4095)
bool avChange;

unsigned long currTim;
unsigned long prevTim = 0;
unsigned long tick = 500;    //

bool regelOn = false;       // if true call regel every tick
bool autoLad = false;       // if true determine current depending on leerlauf-voltage
int soll_mA = 0;
int busvolt_mV;
int current_mA;
int waitms = 30;          // delay leerlauf
int lelaCnt;
int lelaMax = 20; // after Cnts check leerlauf

typedef struct { // params
  byte magic = 42;
  int max_mA;
  int max_mV;
  int llzOn_mV;
  int llzOff_mV;
  int lade_mA;
  int keep_mA;
} lad_t;
lad_t lad;

uint16_t epromAdr = 0;

// common

void vt100Home() {
  Serial.print("\x1B[H");
}

void vt100ClrEol() {
  Serial.print("\x1B[K");
}

void vt100ClrEos() {
  Serial.print("\x1B[J");
}

void prntF(PGM_P p) {
  while (1) {
    char c = pgm_read_byte(p++);
    if (c == 0) break;
    Serial.write(c);
  }
  Serial.write(" ");
}

void msgF(const __FlashStringHelper *ifsh, uint16_t n) {
  // text verbraucht nur flash
  PGM_P p = reinterpret_cast<PGM_P>(ifsh);
  prntF(p);
  Serial.println(n);
}


void defaultPara() {
  lad.magic=42;
  lad.max_mA = 600;
  lad.max_mV = 1750;
  lad.llzOn_mV = 1390;  // return to load if <
  lad.llzOff_mV = 1400; // switch to keep if >
  lad.lade_mA = 500;
  lad.keep_mA = 10;
}

void showPara() {
  char str[50];
  Serial.println();
  sprintf(str, "pa  Max   %5d mA", lad.max_mA);
  Serial.println(str);
  sprintf(str, "pb  Max   %5d mV", lad.max_mV);
  Serial.println(str);
  sprintf(str, "pc  ZlOff %5d mV", lad.llzOff_mV);
  Serial.println(str);
  sprintf(str, "pd  ZlOn  %5d mV", lad.llzOn_mV);
  Serial.println(str);
  sprintf(str, "pe  Lade  %5d mA", lad.lade_mA);
  Serial.println(str);
  sprintf(str, "pf  Keep  %5d mA", lad.keep_mA);
  Serial.println(str);
  Serial.println(F("ps  standard"));
  Serial.println(F("pr  read"));
  Serial.println(F("pw  write"));
  Serial.println(F("pz  zeig"));
}

bool doPara(char tmp) {
  char str[50];
  if (!inParam) return false;
  inParam = false;
  if ((tmp < 'a') || (tmp > 'z')) return false;
  switch (tmp) {
    case 'a':   //
      lad.max_mA = inp;
      sprintf(str, "pa  Max   %5d mA", lad.max_mA);
      break;
    case 'b':   //
      lad.max_mV = inp;
      sprintf(str, "pb  Max   %5d mV", lad.max_mV);
      break;
    case 'c':
      lad.llzOff_mV = inp;
      sprintf(str, "pc  ZlOff %5d mV", lad.llzOff_mV);
      break;
    case 'd':
      lad.llzOn_mV = inp;
      sprintf(str, "pd  ZlOn  %5d mV", lad.llzOn_mV);
      break;
    case 'e':
      lad.lade_mA = inp;
      sprintf(str, "pe  Lade  %5d mA", lad.lade_mA);
      break;
    case 'f':
      lad.keep_mA = inp;
      sprintf(str, "pf  Keep  %5d mA", lad.keep_mA);
      break;
    case 'r':
      EEPROM.get(epromAdr, lad);
      showPara();
      strcpy(str, "Read");
      break;
    case 's':
      defaultPara();
      showPara();
      strcpy(str, "Set");
      break;
    case 'w':
      EEPROM.put(epromAdr, lad);
      strcpy(str, "Written");
      break;
    case 'z':   //
      showPara();
      strcpy(str, "gucksdu");
      break;
    default:
      return false;
  }
  Serial.println(str);
  return true;
}

//
void testdigit() {
  for (int i = 0; i < 8; i++) {
    lc.setDigit(lc.lcRow, i, i, false);
  }
}

void setIntens(char what) {
  if (what == '+') {
    intens++;
    if (intens > 15) intens = 0;
  }
  if (what == '-') {
    intens--;
    if (intens > 15) intens = 15;
  }
  for (int r = 0; r < anzR; r++) {
    lc.setIntensity(r, intens);
  }
  msgF(F("Intens "), intens);
}


void lcReset() {
  int devices = lc.getDeviceCount();
  //we have to init all devices in a loop
  msgF(F("Reset for "), devices);
  for (int address = 0; address < devices; address++) {
    /*The MAX72XX is in power-saving mode on startup*/
    lc.shutdown(address, false);
    /* Set the brightness to a medium values */
    lc.setIntensity(address, intens); //8
    /* and clear the display */
    lc.clearDisplay(address);
  }
}

// MCP4725

void setVoltageFast(uint16_t val) {
  byte lowVal, highVal;
  lowVal = lowByte(val);
  highVal = highByte(val);
  Wire.beginTransmission(mcpAdr);
  Wire.write(MCP4725_CMD_WRITE_FAST_MODE | highVal );
  Wire.write(lowVal);
  Wire.endTransmission();
}

void setVoltage( uint16_t output, bool writeEEPROM ) {
  Wire.beginTransmission(mcpAdr);
  if (writeEEPROM)  {
    Wire.write(MCP4725_CMD_WRITEDAC_EEPROM);
  }  else   {
    Wire.write(MCP4725_CMD_WRITEDAC);
  }
  Wire.write(output / 16);                   // Upper data bits          (D11.D10.D9.D8.D7.D6.D5.D4)
  Wire.write((output % 16) << 4);            // Lower data bits          (D3.D2.D1.D0.x.x.x.x)
  Wire.endTransmission();
}

void setWireFreq(byte b) {
  unsigned long freq;
  switch (b) {
    case 0:   //
      freq = 100000L;
      break;
    case 1:   //
      freq = 400000L;
      break;
    case 2:   //
      freq = 600000L;
      break;
    case 3:   //
      freq = 33333L;
      break;
    case 9:   //
      freq = 1000000L; // geht nicht
      break;
    default:
      msgF(F("setFreq Unknown?? "), b);
      return;
  }
  msgF(F("Freq to "), b);
  Wire.setClock(freq);
  msgF(F("TWBR:"), TWBR);
  msgF(F("TWSR:"), TWSR & 3);
  /*
    TWSR 0..3 = 1,4,16,64
    SCL Frequency = CPU Clock Frequency / (16 + (2 * TWBR))
    note: TWBR should be 10 or higher for master mode
    It is 72 for a 16mhz  with 100kHz TWI */
}

void chng(int chg) {
  outV = outV + chg;
  if (outV > 4095) {
    outV = 4095;
  } else if (outV < 0) {
    outV = 0;
  }
  if (verbo) msgF (F(" Change to "), outV);
  setVoltageFast(outV);
  /*
    av1 = analogRead(A0);
    if (verbo) msg("av1 ", av1);
  */
}

void readAn() {
  int di;
  avChange = false;
  av1 = analogRead(A1);
  di = abs(av1 - oav1);
  if (di > 3) {
    oav1 = av1;
    msgF(F("av1 "), av1);
    avChange = true;
  }
}

void readInaFloat() {
  float shuntvoltage = 0;
  float busvoltage = 0;
  float current_mA = 0;
  float loadvoltage = 0;
  float power_mW = 0;

  shuntvoltage = ina219.getShuntVoltage_mV();
  busvoltage = ina219.getBusVoltage_V();
  current_mA = ina219.getCurrent_mA();
  power_mW = ina219.getPower_mW();
  loadvoltage = busvoltage + (shuntvoltage / 1000);

  Serial.print("Bus Voltage:   "); Serial.print(busvoltage); Serial.println(" V");
  Serial.print("Shunt Voltage: "); Serial.print(shuntvoltage); Serial.println(" mV");
  Serial.print("Load Voltage:  "); Serial.print(loadvoltage); Serial.println(" V");
  Serial.print("Current:       "); Serial.print(current_mA); Serial.println(" mA");
  Serial.print("Power:         "); Serial.print(power_mW); Serial.println(" mW");
  Serial.println("");
}

void readIna() {
  // reads and shows
  char str[80];
  busvolt_mV = ina219.getBusVoltage_raw();
  current_mA = ina219.getCurrent_raw() / 10;
  sprintf(str, "%4d Out %5d mA %5d mV", outV, current_mA, busvolt_mV);
  Serial.print(str);
  vt100ClrEol();
  Serial.println();
  lc.home();
  lc.show4DigS(busvolt_mV);
  lc.show4DigS(current_mA);
}

void kennlinie(int anz) {
  // Change DAC from current outV by delt and read Ina up to og
  int delt = 10;
  for (byte i = 0; i < anz; i++ ) {
    chng(0);
    readIna();
    outV += delt;
  }
  chng(-outV);
  readIna();
}

int leerlauf() {
  //
  int outS = outV;
  chng(-outV);
  delay(waitms);
  readIna();
  chng(outS);
  return  busvolt_mV;
}

void regel() {
  // tries to set current to soll_mA;
  // does not exceed vmax or imax
  char str[80];
  int dlt = 0;
  char ov = ' ';
  vt100Home();
  readIna();
  int reg = 0; // delta of outV
  // do not exceed limits
  if (busvolt_mV > lad.max_mV) {
    reg -= 30;
    ov = 'V';
  }
  if (current_mA > lad.max_mA) {
    reg -= 30;
    ov = 'A';
  }
  if (reg == 0) {
    dlt = soll_mA - current_mA ;
    if (dlt > 50) {
      reg = 40;
    } else  if (dlt > 10) {
      reg = 10;
    } else  if (dlt > 5) {
      reg = 5;
    } else  if (dlt > 2) {
      reg = 2;
    } else if (dlt < -50) {
      reg = -40;
    } else if (dlt < -10) {
      reg = -10;
    } else if (dlt < -5) {
      reg = -5;
    } else if (dlt < -2) {
      reg = -2;
    }
  } //
  sprintf(str, "     Soll %4d mA  delt %+4d  reg %4d %c", soll_mA, dlt, reg, ov);
  Serial.println(str);
  chng(reg);
}

void i2c_scan() {
  byte error, address;
  int nDevices;
  Serial.println("Scanning...");
  nDevices = 0;
  for (address = 1; address < 127; address++ )
  {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    if (error == 0)
    {
      Serial.print("I2C device found at address 0x");
      if (address < 16)
        Serial.print("0");
      Serial.print(address, HEX);
      Serial.println(".");
      nDevices++;
    }
    else if (error == 4)
    {
      Serial.print("Unknown error at address 0x");
      if (address < 16)
        Serial.print("0");
      Serial.println(address, HEX);
    }
  }
  if (nDevices == 0)
    Serial.println("No I2C devices found\n");
  else
    Serial.println("done\n");
}

void help () {
  Serial.println (F("Main commands:"));
  Serial.println (F("r toggle regel, x toggle Autoload"));
  Serial.println (F("Test commands:"));
  Serial.println (F("a lcReset  b off  c on  n m Intens"));
  Serial.println (F("i ina read   j ina read float   _k Kennlinie"));
  Serial.println (F("_o Out Voltage  + - 10   _r Regel  s Single a Auto"));
  Serial.println (F("_t tick"));
  Serial.println (F("d detect TWI  _f set TWI freq  v verbose  cr clrscr"));
}

void doCmd( char tmp) {
  Serial.print (tmp);
  if ( tmp == 8) { //backspace removes last digit
    inp = inp / 10;
    return;
  }
  if ((tmp >= '0') && (tmp <= '9')) {
    if (inpAkt) {
      inp = inp * 10 + (tmp - '0');
    } else {
      inpAkt = true;
      inp = tmp - '0';
    }
    return;
  }

  if (doPara(tmp)) return;

  inpAkt = false;
  Serial.print("\b\b\b\b");
  vt100ClrEol();
  switch (tmp) {
    case 'a':   //
      lcReset();
      msgF(F("av1 "), av1);
      break;
    case 'b':
      lc.shutdown(0, true);
      break;
    case 'c':
      lc.shutdown(0, false);
      break;
    case 'd':   //
      i2c_scan();
      break;
    case 'e':
      testdigit();
      break;
    case 'f':   //
      setWireFreq(inp);
      break;
    case 'i':   //
      readIna();
      break;
    case 'j':   //
      readInaFloat();
      break;
    case 'k':   //
      kennlinie(inp);
      break;
    case 'l':   //
      leerlauf();
      break;
    case 'n':   //
      setIntens('-');
      break;
    case 'm':   //
      setIntens('+');
      break;
    case 'o':   //
      outV = inp;
      chng(0);
      readIna();
      break;
    case 'p':   //
      inParam = true;
      Serial.print("Para:");
      break;
    case 'r':   //
      if (regelOn) {
        regelOn = false;
        chng(-outV);
        delay(waitms);
        readIna();
        msgF(F("Regel off "), 0);
      } else {
        soll_mA = inp;
        regelOn = true;
        lelaCnt = lelaMax;
        msgF(F("Regel on "), inp);
      }
      break;
    case 's':   //
      regel();
      break;
    case 't':   //
      tick = inp;
      msgF(F("Tick "), inp);
      break;
    case 'v':   //
      verbo = !verbo;
      msgF(F("Verbo"), verbo);
      break;
    case 'w':   //
      break;
    case 'x':   //
      autoLad = !autoLad;
      msgF(F("autoLad"), autoLad);
      break;
    case '+':   //
      chng(+10);
      readIna();
      break;
    case '-':   //
      chng(-10);
      readIna();
      break;
    case 13:
      vt100Home();
      vt100ClrEos();
      lc.cls();
      break;
    default:
      help();
  } // case

}

void setup() {
  const char ich[] = "mcp4725Lade " __DATE__  " "  __TIME__;
  Serial.begin(38400);
  Serial.println(ich);
  lcReset();
  pinMode(A0, INPUT);
  Wire.begin(mcpAdr);
  if (! ina219.begin()) {
    Serial.println("Failed to find INA219 chip");
  }
  EEPROM.get(epromAdr, lad);
  if (lad.magic != 42)  defaultPara();
  showPara();
  regelOn = true;
  autoLad = true;
}

void loop() {
  char tmp;
  int ll_mV;

  if (Serial.available() > 0) {
    tmp = Serial.read();
    doCmd(tmp);
  } // avail


  currTim = millis();
  if (currTim - prevTim >= tick) {
    prevTim = currTim;
    if (regelOn) {
      lelaCnt--;
      if (lelaCnt <= 0) {
        lelaCnt = lelaMax;
        ll_mV = leerlauf();
        if (autoLad) {
          if (ll_mV > lad.llzOff_mV) {
            soll_mA = lad.keep_mA;
          }
          if (ll_mV < lad.llzOn_mV) {
            soll_mA = lad.lade_mA;
          }
        } // auto
      } else {
        regel();
      }
    } //regel
  }  //tick
}

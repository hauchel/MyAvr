/*
  lcd12864 16*4 char or 128*64 px
  + TSOP + MCP4720 + BH1750
  SDA         A4  pink
  SCL         A5  grau
  Analog In   A0 A1
  TSOP RC5     2  weiss
  LED         13
*/
const byte rc5Pin = 2;
const byte ledPin = 13;

#include "LCD12864RSPI.h"
//#include <Wire.h>
#include <Adafruit_MCP4725.h>
Adafruit_MCP4725 dac;

#include <BH1750.h>
BH1750 lightMeter;

// mit RC5 auf timer2
#include "rc5_tim2.h"

char lastRC = ' ';
uint16_t outV;

bool verb = false;
uint16_t data [128];  // y-values to plot for x
uint16_t vert[64];    // graph vertical stripe (8*)
uint16_t xMin, xMax, xDelt, xAnz;
int mX = 6;  // x-pos for update
uint16_t  fixfac = 20; //convert measure to display

byte ledCnt = 3;
bool gridM = true;      // ahow grid
byte missSrc = 10;      // Input
const byte updMax = 5;  // display miss update after ticks
byte updCnt = updMax;
bool lcdGramo;          // true if no text otput to LCD should occur

unsigned long currTim;
unsigned long prevTim = 0;
unsigned long tick = 100; // ms per tick

void msg(const char txt[], int n) {
  Serial.print(txt);
  Serial.println(n);
}


void i2c_scan() {
  byte error, address;
  int nDevices;

  Serial.println("Scanning...");

  nDevices = 0;
  for (address = 1; address < 127; address++ ) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0)  {
      Serial.print("I2C device found at address 0x");
      if (address < 16)
        Serial.print("0");
      Serial.print(address, HEX);
      Serial.println("  !");

      nDevices++;
    }
    else if (error == 4)  {
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


void zeroVert() {
  for (int y = 0; y < 64; y++) {
    vert[y] = 0;
  }
}

void putVert(int x) {
  byte bL, bH;
  for (int y = 0; y < 64; y++) {
    if (vert[y] != 0) {
      //msg("y ", y);
      setGDAdr(x, 63 - y);
      bH = vert[y] >> 8;
      bL = vert[y] & 0xFF;
      //msg("L ", bL);
      //msg("H ", bH);
      LCDA.writeData(bH);
      LCDA.writeData(bL);
    }
  }
}

void grid() {
  if (gridM) {
    vert[0]  = 0x8000;
    vert[10] = 0x8000;
    vert[20] = 0x8000;
    vert[30] = 0x8000;
    vert[40] = 0x8000;
    vert[50] = 0x8000;
    vert[60] = 0x8000;
  }
}


uint16_t miss() {
  switch (missSrc) {
    case 0:   //
      return uint16_t(analogRead(A0));
      break;
    case 1:   //
      return uint16_t(analogRead(A1));
      break;
    case 10:
      return uint16_t(lightMeter.readLightLevel());
      break;
    default:
      msg ("Unknown missSrc", missSrc);
  } //case
}

void nextMiss() {
  missSrc += 1;
  if (missSrc == 2) {
    missSrc = 10;
  }
  if (missSrc > 10) {
    missSrc = 0;
  }
  msg ("MissSrc ", missSrc);
}

void lcdMiss() {
  char str[10];
  sprintf(str, "%4u", miss());
  LCDA.displayString(mX, 0, str, 4);
}

void lcdOutV() {
  char str[10];
  if (!lcdGramo) {
    sprintf(str, "%4u", outV);
    LCDA.displayString(0, 0, str, 4);
  } else {
    msg("in gramo ", outV);
  }
}

void lcdMima() {
  char str[20];
  sprintf(str, "%4u %4u %4u   ", xMin, xMax, xDelt);
  LCDA.displayString(0, 1, str, 15);
}

void lcdInfo() {
  lcdMima();
}

void normalData() {
  int bx;
  uint16_t  mi, ma, fac;
  mi = 4096;
  ma = 0;
  for (bx = 0; bx <= xAnz; bx++) {
    if (data[bx] < mi) {
      mi = data[bx];
    }
    if (data[bx] > ma) {
      ma = data[bx];
    }
  }
  msg("mi ", mi);
  msg("ma ", ma);
  fac = 1 + ma / 64;
  msg("fac ", fac);
  if (fixfac != 0) {
    fac = fixfac;
  }
  for (bx = 0; bx <= xAnz; bx++) {
    data[bx] = data[bx] / fac;
  }
}

void datagen1() {
  int bx;
  xMin = 0;
  xMax = 64;
  xDelt = 1;
  for (bx = 0; bx < 64; bx++) {
    data[bx] = bx;
  }
  for (bx = 64; bx < 128; bx++) {
    data[bx] = 128 - bx;
  }
}

void datagen2() {
  uint16_t xOut, av;
  int bx = 0;

  xMin = 200;
  xMax = 2000;
  xDelt = 64;
  xOut = xMin;
  while (xOut <= xMax) {
    dac.setVoltageFast(xOut);
    xOut += xDelt;
    delay(200); // for BH750
    av = miss();
    data[bx] = av;
    bx += 1;
  }
  setOutV();  //previous value
  xAnz = bx - 1;
  msg("bx ", bx);
  showData();
  normalData();
}

void clearData() {
  for (int i = 0; i < 128; i++) {
    data[i] = 0;
  }
}

void showData() {
  char str[100];
  //Serial.println();
  for (int i = 0; i < 128; i += 8) {
    sprintf(str, "%3d   %4u %4u %4u %4u  %4u %4u %4u %4u ", i, data[i], data[i + 1], data[i + 2], data[i + 3], data[i + 4], data[i + 5], data[i + 6], data[i + 7]);
    Serial.println(str);
  }
}
void zeich() {
  int x,  p, bx;
  uint16_t y, akt;
  p = 0;
  for (bx = 0; bx < 8; bx++) {
    zeroVert();
    grid();
    akt = 1 << 15;
    for (x = 0; x < 16; x++) {
      y = data[p];
      if (y > 63) {
        y = 63;
      }
      p++;
      //msg("dat", y);
      vert[y] = vert[y] | akt;
      akt = akt >> 1;
    }
    putVert(bx);
  }
}

void setGDAdr(int x, int y) {
  // x is 0..15, y 0..63
  if (y > 31) {
    y = y - 32;
    x = x + 8;
  }
  LCDA.writeCommand(y + 0x80);
  LCDA.writeCommand(x + 0x80);
}

void gcls() {
  long zeit;
  zeit = millis();
  for (int y = 0; y < 32; y++) {
    setGDAdr(0, y);
    for (int i = 0; i < 32; i++) {
      LCDA.writeData(0);
    }
  }
  setGDAdr(0, 0);
  zeit = millis() - zeit;
  msg ("Elapsed ms ", int(zeit));
}


void testbild() {
  LCDA.clear();
  LCDA.moveCursor(0, 0);
  for (int i = 0; i < 63; i++) {
    LCDA.writeData(i + 65);
  }
}

void setOutV() {
  if (outV < 0) {
    outV = 0;
  } else {
    if (outV > 4095) {
      outV = 4095;
    }
  }
  if (verb) {
    msg (" Change to ", outV);
  }
  dac.setVoltageFast(outV);
}

void chng(int delt) {
  outV += delt;
  setOutV();
  lcdOutV();
}

void regelung () {
  int iw;
  lightMeter.readLightLevel();

}

unsigned int readVcc() {
  // Read 1.1V reference against AVcc
  // set the reference to Vcc and the measurement to the internal 1.1V reference
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Start conversion
  while (bit_is_set(ADCSRA, ADSC)); // measuring

  uint8_t low  = ADCL; // must read ADCL first - it then locks ADCH
  uint8_t high = ADCH; // unlocks both

  unsigned int result = (high << 8) | low;
  char str[10];
  sprintf(str, "%4u", result);
  LCDA.displayString(mX, 1, str, 4);
  // result = 1125300L / result; // Calculate Vcc (in mV); 1125300 = 1.1*1023*1000
  return result; // Vcc in millivolts
}

void doRC5() {
  msg("RC5 ", rc5_command);
  digitalWrite(ledPin, HIGH);
  ledCnt = 3;
  if ((rc5_command >= 0) && (rc5_command <= 9)) {
    doCmd(rc5_command + '0');
  } else {
    switch (rc5_command) {
      case 11:   // Store
        doCmd(lastRC);
        break;
      case 13:   // Mute
        doCmd('z');
        break;
      case 25:   // R
        doCmd('t');
        break;
      case 23:   // G
        doCmd('g');
        break;
      case 27:   // G
        doCmd('z');
        break;
      case 36:   // B
        doCmd('r');
        break;
      case 43:   // B
        doCmd('i');
        break;
      case 44:   // B
        doCmd('s');
        break;
      case 45:   // B
        msg("not used ", rc5_command);
        break;
      case 57:   // Ch+
        doCmd('l');
        break;
      case 56:   // Ch+
        doCmd('h');
        break;
      default:
        msg("Which RC5? ", rc5_command);
        msg("Address is ", rc5_address);
    } // case
  } // else
}

void help() {
  Serial.println(" 0 - 9    set out Voltage to *512");
  Serial.println(" h,j,k,l  set out Voltage -100,-10,+10,+100");
  Serial.println(" b LCD Basic mode");
  Serial.println(" c LCD Clear Screen");
  Serial.println(" d show Data");
  Serial.println(" e equalizer  nnn");
  Serial.println(" g,t LCD Graphmode,Textmode");
  Serial.println(" i show Info");
  Serial.println(" t LCD Text mode");
  Serial.println(" m next source (10=Lux)");
  Serial.println(" r run, z zero data");
  Serial.println(" u update, v verbose");

}
void doCmd( char tmp) {
  if ((tmp >= '0') && (tmp <= '9')) {
    outV = (tmp - '0') * 512;
    setOutV();
    lcdOutV();
  } else {
    switch (tmp) {
      case '+':   //
        mX += 1;
        msg("mx", mX);
        break;
      case '-':   //
        mX -= 1;
        msg("mx", mX);
        break;

      case 'a':   //
        break;
      case 'b':   //
        LCDA.writeCommand(0x30);
        msg ("Basic No G", 0);
        break;
      case 'c':   //
        LCDA.clear();
        msg("Clear", 0);
        break;
      case 'd':   //
        showData();
        break;
      case 'e':   //
        LCDA.displayString(0, 0, "a123456789012345", 16);
        break;
      case 'E':
        LCDA.displayString(0, 0, "a12345678901234a", 16);
        LCDA.displayString(0, 1, "b12345678901234b", 16);
        LCDA.displayString(0, 2, "c12345678901234c", 16);
        LCDA.displayString(0, 3, "d1234567890123", 14);  //buggy
        break;
      case 'f':   //
        fixfac = fixfac + 5;
        msg ("FixFac", fixfac);
        break;
      case 'F':   //
        fixfac = 0;
        msg ("FixFac", fixfac);
        break;
      case 'g':   // graphmode
        updCnt = 0;
        LCDA.clear();
        LCDA.writeCommand(0x36);
        LCDA.writeCommand(0x36);
        lcdGramo = true;
        break;
      case 'h':   //
        chng(-100);
        break;
      case 'i':   //
        lcdInfo();
        break;

      case 'I':   //
        i2c_scan();
        break;
      case 'j':   //
        chng(-10);
        break;
      case 'k':   //
        chng(+10);
        break;
      case 'l':   //
        chng(+100);
        break;
      case 'm':   //
        nextMiss();
        break;
      case 'r':   //
        datagen2();
        zeich();
        break;
      case 's':   //
        gridM = !gridM;
        datagen1();
        zeich();
        break;
      case 'S':   //
        setup();
        break;
      case 't':   // textmode
        LCDA.writeCommand(0x34);
        LCDA.clear();
        lcdGramo = false;
        updCnt = 1;
        lcdOutV();
        break;
      case 'u':   //
        if (updCnt == 0) {
          updCnt = 1;
        } else {
          updCnt = 0;
        }
        msg ("Update ", updCnt);
        break;
      case 'v':   //
        verb = !verb;
        msg("Verbose ", verb);
        break;
      case 'x':   //
        LCDA.writeCommand(0x34);
        msg("Ext no G", 0);
        break;
      case 'y':   //
        LCDA.writeCommand(0x36);
        msg("Ext Graf", 0);
        break;
      case 'z':   //
        gcls();
        clearData();
        break;
      default:
        msg("?? ", tmp);
        help();
    } //case
  }
}

void setup() {
  const char info[] = "lcd12864  " __DATE__  " "  __TIME__;
  Serial.begin(38400);
  Serial.println(info);
  pinMode(A0, INPUT);
  pinMode(ledPin, OUTPUT);
  // Serial.println("Init LCDA:");
  LCDA.initialise();
  lcdGramo = false;
  testbild();
  Serial.println("Init DAC 62:");
  dac.begin(0x62);
  Serial.println("Init LightMeter:");
  lightMeter.begin();
  RC5_init();
  Serial.println("Ready.");
  LCDA.clear();
}

void loop() {
  char tmp;
  if (rc5_ok) {
    RC5_again();
    doRC5();
  }
  if (Serial.available() > 0) {
    tmp = Serial.read();
    doCmd(tmp);
  } // avail
  currTim = millis();
  if (currTim - prevTim >= tick) {
    prevTim = currTim;
    if (ledCnt == 0) {
      digitalWrite(ledPin, LOW);
    } else {
      ledCnt--;
    }
    if (updCnt > 0) {
      updCnt--;
      if (updCnt == 0) {
        updCnt = updMax;
        lcdMiss();
        readVcc();
      }
    } // updCnt
  } // tick
}

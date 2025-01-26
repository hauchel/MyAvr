/* einstellbarer Widerstand mit LCD-shield
    an UNO mit Keypad Shield
*/
#include <SPI.h>
#include <LiquidCrystal.h>

/* Uno mit LCD shield
   Pins on Jumper
    D0 RX
    D1 TX
    D2 avail
    D3 SS     blue   1   ssPin
    D4 to D9 lcd
    D10 light LOW is off
    pinout voltage
      3.3  5 Gnd Gnd Raw
   Gnd          brown  4
   VCC          red    8
   D13  SCK     orange 2
   D12  MISO    yell   7
   D11  MOSI    green  3

   mcp4132
   MEMORY MAP
   00h Volatile Wiper 0
   01h Volatile Wiper 1
   04h Volatile TCON Register RAM
   05h Status Register RAM
  8 bit
  AAAACCDD
  16 bit
  AAAACCDD DDDDDDDD

  Command             Wiper 0
  00 = Write Data     0000 00nn nnnn nnnn
  01 = INCR           0000 0100
  10 = DECR           0000 1000
  11 = Read Data     wiper 12 tcon 76  status 92
*/

LiquidCrystal lcd(8, 9, 4, 5, 6, 7);
const int ssPin = 3;

byte rein;
unsigned long nextMs = 0, currMs;

int messCnt = 10;
int messFrq = 5;

float uOfs = -0.0246;
float uStep = 0.00720;
float rFac = 345.833;
float uMax = 4.20;
float iMax = 150.0;
float uMaxSel[2] = {4.20, 5.50};
int uMaxP = 0;
int rSet = 0;

int menP = 0;
const int menAnz = 5;          // 0         1          2     3         4
const char menT [menAnz] [7] = {"Run   ", "U Max ", "I Max ", "Load  ", "Hold  "};
const char menT0[] = "Run   ";
const char menT1[] = "U Max ";
const char menT2[] = "I Max ";
const char menT3[] = "Load  ";
const char menT4[] = "Hold  ";

// General
void msg(const char txt[], int n) {
  Serial.print(txt);
  Serial.print(" ");
  Serial.println(n);
}


// For Keypad
int ana0, ana1, ana2;
float volt1, volt2, strom;

const byte keyCen = 0;
const byte keyUp = 1;
const byte keyDwn = 2;
const byte keyLft = 3;
const byte keyRgt = 4;
const byte keyNop = 9;
byte kbdOld = 0;
byte kbdCnt = 0;

byte kbdPress() {
  ana0 = analogRead(A0);
  //msg("An0 ", ana0);
  if (ana0 < 50) {
    return keyRgt;
  }
  if (ana0 < 250) {
    return keyUp;
  }
  if (ana0 < 450) {
    return keyDwn;
  }
  if (ana0 < 600) {
    return keyLft;
  }
  if (ana0 < 800) {
    return keyCen;
  }
  return keyNop;
}

byte kbdRead() {
  byte ths = kbdPress();
  if (ths != kbdOld) {
    kbdOld = ths;
    kbdCnt = 5; // after first
    return ths;
  }
  kbdCnt--;
  if (kbdCnt > 0) {
    return keyNop;
  }
  kbdCnt = 3; // repeat
  return ths;
}


byte fromTwo(char cH, char cL) {
  // converts two nibbles to byte
  byte tmp;
  tmp = byte(cH) - 48;
  tmp = tmp * 16;
  tmp = tmp + byte(cL) - 48;
  return tmp;
}

void printHexSht (byte b) {
  // want to see leading 0, then space
  if (b < 16) {
    Serial.print('0');
  }
  Serial.print(b, HEX);
}

void printHex (byte b) {
  printHexSht(b);
  Serial.print(' ');
}

void lc3dig(int val) {
  if (val > 99) {
    lcd.print(val);
    return;
  }
  lcd.print(" ");
  if (val > 9) {
    lcd.print(val);
    return;
  }
  lcd.print(" ");
  lcd.print(val);
}

void miss() {
  ana1 = analogRead(A1);
  volt1 = float(ana1) * uStep - uOfs;
  ana2 = analogRead(A2);
  volt2 = float(ana2) * uStep - uOfs;
  strom = (volt2 - volt1) * rFac;
}

void setR(byte bL) {
  digitalWrite(ssPin, LOW);
  delay(10);
  rein = SPI.transfer(0);
  rein = SPI.transfer(bL);
  digitalWrite(ssPin, HIGH);
}

void readR(byte bH) {
  digitalWrite(ssPin, LOW);
  delay(10);
  rein = SPI.transfer(bH);
  rein = SPI.transfer(0);
  rSet = rein;
  digitalWrite(ssPin, HIGH);
}

void setAndRead(byte bL) {
  setR(bL);
  readR(12);
}

void send8(byte b) {
  digitalWrite(ssPin, LOW);
  delay(10);
  rein = SPI.transfer(b);
  digitalWrite(ssPin, HIGH);
}

void calcGerade(float x1, float x2, float y1, float y2) {
  uStep = (y2 - y1) / (x2 - x1);
  uOfs = y2 - uStep * x2;
}

void zeigGerade() {
  char buff[10];
  dtostrf(uStep, 9, 5, buff);
  Serial.print ("m=");
  Serial.print (buff);
  dtostrf(uOfs, 9, 5, buff);
  Serial.print (" b=");
  Serial.print (buff);
}

void zeigWerte() {
  char buff[10];
  dtostrf(uMax, 9, 5, buff);
  Serial.print ("uMax ");
  Serial.print (buff);
  dtostrf(iMax, 9, 5, buff);
  Serial.print (" iMax ");
  Serial.print (buff);
}

void incR(int delt) {
  rSet = rSet + delt;
  if (rSet < 0) {
    rSet = 0;
  }
  if (rSet > 127) {
    rSet = 127;
  }
  setR(rSet);
}

void load() {
  float uDiff, iDiff;
  uDiff = uMax - volt1;
  iDiff = iMax - strom;
  if (iDiff < 0.0) {
    incR(-1);
    return;
  }
  if (uDiff < -0.1) {
    incR(-10);
    return;
  }
  if (uDiff < 0.0) {
    incR(-1);
    return;
  }
  if (uDiff < 0.01) {
    return;
  }
  if (uDiff > 1.0) {
    incR(+10);
    return;
  }
  if (uDiff > 0.5) {
    incR(+5);
    return;
  }
  if (uDiff > 0.2) {
    incR(+1);
    return;
  }

  if (iDiff > 5.0) {
    incR(+1);
    return;
  }

}

void hold() {
  // Strom leicht positiv
  float diff;
  diff = uMax - volt1;
  if (diff < -0.09) {
    incR(-1);
    return;
  }
  if (strom < 0.0) {
    incR(+1);
    return;
  }
  if (strom > 10.0) {
    incR(-1);
    return;
  }
}


void show() {
  char buff[10];
  lcd.setCursor(0, 0) ;
  dtostrf(volt1, 5, 2, buff);
  lcd.print (buff);
  dtostrf(volt2, 5, 2, buff);
  lcd.print (buff);
  dtostrf(strom, 6, 1, buff);
  lcd.print (buff);
  lcd.setCursor(0, 1);
  if (menP == 0) {
    lcd.print ("  ");
    lcd.print (ana1);
    lcd.print ("  ");
    lcd.print (ana2);
    lcd.print (" ");
    lc3dig (rSet);
    return;
  }
  lcd.print(menT[menP]);
  if (menP == 1) {
    dtostrf(uMax, 5, 2, buff);
    lcd.print (buff);
    return;
  }
  if (menP == 2) {
    dtostrf(iMax, 5, 1, buff);
    lcd.print (buff);
    return;
  }
  if (menP == 3) {
    lc3dig (rSet);
    return;
  }
  if (menP == 4) {
    lc3dig (rSet);
    return;
  }
}

void setMen(int deltP) {
  menP = menP + deltP;
  if (menP < 0) {
    menP = 4;
  }
  if (menP > 4) {
    menP = 0;
  }
  msg("setMen ", menP);
  lcd.clear();
}

void setUMax() {
  uMaxP += 1;
  if (uMaxP > 1) {
    uMaxP = 0;
  }
  uMax = uMaxSel[uMaxP];
}

void doMenu(byte tast) {
  msg("doMenu ", menP);
  msg("T= ", tast);
  switch (tast) {
    case keyLft:
      setMen(-1);
      return;
    case keyRgt:
      setMen(+1);
      return;
  } //

  switch (menP) {
    case 0:          // Run
      switch (tast) {
        case keyUp:
          doCmd('+');
          break;
        case keyDwn:
          doCmd('-');
          break;
        case keyCen:
          setAndRead(60);
          break;
      } // tast for 0
      break;
    case 1:       // UMax
      switch (tast) {
        case keyUp:
          uMax = uMax + 0.1;
          break;
        case keyDwn:
          uMax = uMax - 0.1;
          break;
        case keyCen:
          setUMax();
          break;

      } // tast for 1
      break;
    case 2:     // IMax
      switch (tast) {
        case keyUp:
          iMax = iMax + 10;
          zeigWerte();
          break;
        case keyDwn:
          iMax = iMax - 10;
          zeigWerte();
          break;
      } // tast for 2
      break;
    case 3:     // Load
      switch (tast) {
        case keyUp:
          doCmd('+');
          break;
        case keyDwn:
          doCmd('-');
          break;
      } // tast for 3
    case 4:     // Hold
      switch (tast) {
        case keyUp:
          doCmd('+');
          break;
        case keyDwn:
          doCmd('-');
          break;
      } // tast for 3

      break;
    default:
      Serial.print("menP? ");
      Serial.print(menP);
      setMen(0);
  } //case
}



void setup() {
  const char info[] = "mcp4132 " __TIME__" "__DATE__ ;
  Serial.begin(38400);
  Serial.println(info);
  doCmd('g');

  pinMode (ssPin, OUTPUT);
  digitalWrite(ssPin, HIGH);
  SPI.begin();
  SPI.setBitOrder(MSBFIRST);
  SPI.setDataMode(SPI_MODE0);
  SPI.setClockDivider(SPI_CLOCK_DIV128);
  setAndRead(60);

  lcd.begin(16, 2);
  lcd.print(info);
  pinMode (10, OUTPUT);
  digitalWrite(10, HIGH);

}

void doCmd(byte adr) {
  switch (adr) {
    case '0':
      setAndRead(0);
      break;
    case '1':
      setAndRead(10);
      break;
    case '2':
      setAndRead(20);
      break;
    case '3':
      setAndRead(30);
      break;
    case '4':
      setAndRead(40);
      break;
    case '5':
      setAndRead(50);
      break;
    case '6':
      setAndRead(60);
      break;
    case '7':
      setAndRead(70);
      break;
    case '9':  //
      setAndRead(128);
      break;

    case 'a':  //
      break;

    case 'b':  //
      digitalWrite(10, HIGH);
      break;
    case 'c':  //
      digitalWrite(10, LOW);
      break;

    case 'i':  //
      zeigWerte();
      break;

    case 'g':  //
      calcGerade(181.0, 686.0, 1.28, 4.92);
      zeigGerade();
      break;

    case 'h':  //
      digitalWrite(ssPin, HIGH);
      Serial.print("High");
      break;

    case 'l':   //
      digitalWrite(ssPin, LOW);
      Serial.print("Low");
      break;

    case 'm':
      messCnt = 5;
      messFrq = messCnt;
      break;
    case 'n':
      messCnt = 1;
      messFrq = messCnt;
      break;
    case 'o':
      messCnt = 0;
      break;

    case 'r':   //
      readR(12);
      break;
    case 't':   //
      readR(76);
      break;
    case 's':   //
      readR(92);
      break;

    case 'R':   //
      digitalWrite(ssPin, LOW);
      delay(100);
      digitalWrite(ssPin, HIGH);
      Serial.print("Reset");
      break;

    case '+':  //
      send8(4);
      Serial.print("Inc");
      readR(12);
      break;

    case '-':  //
      send8(8);
      Serial.print("Dec");
      readR(12);
      break;

    case '=':   //
      SPI.setDataMode(SPI_MODE0);
      Serial.print("DM0");
      break;
    case '!':   //
      SPI.setDataMode(SPI_MODE1);
      Serial.print("DM1");
      break;
    case '"':   //
      SPI.setDataMode(SPI_MODE2);
      Serial.print("DM2");
      break;
    case '$':   //
      SPI.setDataMode(SPI_MODE3);
      Serial.print("DM3");
      break;

    default:
      Serial.print(char(adr));

  } //case
  Serial.println();
}

void loop() {
  char c;
  byte adr;
  byte tt = 0;
  if (Serial.available() > 0) {
    adr = Serial.read();
    doCmd(adr);
  } // serial
  currMs = millis();
  if (nextMs <= currMs) {
    nextMs = currMs + 100;
    tt = kbdRead();
    if (tt != keyNop) {
      msg ("->", ana0);
      doMenu(tt);
    } //tast
    if (messCnt > 0) {
      messCnt--;
      if (messCnt == 0) {
        messCnt = messFrq;
        miss();
        if (menP == 3) {
          load();
        }
        if (menP == 4) {
          hold();
        }
      } // cnt
      show();
    }
  } // tim

}




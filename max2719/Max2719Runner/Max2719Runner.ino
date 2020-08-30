// Ghost Car Runner

#include <LedCon5.h>
#include <EEPROM.h>
/*
  KY-40 Drehgeber
  D2   SW        blue
  D3   Clock     yellow
  D4   Data      green

  D6   Out Motor (Timer 0)

  Display
  D7   CLK       pink
  D8   CS/Load   grey
  D9   DataIn    white

  Inputs
  D10
  D11
  D12
  D13
  D14 A0   orange
  D15 A1   yell
  D16 A2   green
  D17 A3   blue

  A6  Poti orange
  A7  free yellow
*/

const byte menuMax = 10;
const char* men0Txt[menuMax] = {"0TO ", "1TI ", "2TI ", "3TI ", "4TI ", "5ET ", "6LP ", "LP  ", "8ET ", "9ET "};
const byte mLps = 0; //   Loops per second
//1 to 4 for times
const byte mMot = 5; //   Car control
const byte mRun = 6; //
const byte mDig = 7; //   show Digital
const byte mAna = 8; //  show Analog
const byte mEdi = 9; //  edit-mode
byte runmode;
byte editSen = 0;
byte editWhat = 0;
byte actn; // use if only once after encoder change

const byte anzR = 1; // number of LED Disps
//             dataPin, clkPin, csPin, numDevices
LedCon5 lc = LedCon5(9, 7, 8, anzR);

const byte kyBtPin = 2;
const byte kyClPin = 3;
const byte kyDtPin = 4;
const byte motPin = 6;

const byte sensMax = 7;     // sens 0 is not used, max is really connected+1 else steu fails
// value is true if car is over
bool sens[sensMax];    // 0  1    2  3    4
byte sensMap[sensMax] = {99, 14, 15, 16, 17, 10, 11};
bool sensInv[sensMax] = {true, false, false, false, false, false, false};  // to invert,currently not used
unsigned long sensMs[sensMax];  // last time stamp
unsigned int sensZeit[sensMax];  // time used to reach this
unsigned int sensZeitTop[sensMax] = {55555, 55555, 55555, 55555, 55555, 55555, 55555}; // best time
// steuer table
const byte steuMax = 20;
byte steuPos[steuMax];
unsigned int steuZeit[steuMax];
unsigned int steuDauer[steuMax];
byte steuGas[steuMax]; // 0-9   3   4    5    6    7    8   9
byte gasW[10] = {0, 100, 140, 160, 180, 200, 210, 220, 240, 255};
//Hi Nibble then after dauer lo Nibble

byte strePos; // 0 unknown, 1 expecting Sens1...
byte strePrev; // previous strePos
byte steuPtr; // current entry in steu Tab
byte testPos ; // if this pos reached...
unsigned int testInc ; // inc Dauer by this

byte poti; //last read analog
bool limit = false; //limit gas to poti

// ky-40 encoder
int kyLast;
// uptime
int second = 0;
int minute = 0;
byte intens = 8;
byte waitCnt = 0;
int stP;
int eadr = 0;       // EEprom Addr
unsigned long lopCnt = 0;

unsigned long currMs, nextMs, nextTick, nextKy, switchMs;

unsigned long delayTime = 100;
unsigned long tickTime = 1000;  // each second
unsigned long kyTime = 100;    // ky40 debounce


void msg(const char txt[], int n) {
  Serial.print(txt);
  Serial.print(" ");
  Serial.println(n);
}

void printByte(byte b)   {
  char str[10];
  sprintf(str, "%3d ", b);
  Serial.print(str);
}

void printMess(byte se, unsigned int z)   {
  char str[50];
  sprintf(str, "* %1d %5u ", se, z);
  Serial.println(str);
}
void printInc(byte se, byte l, unsigned int z)   {
  char str[50];
  sprintf(str, "+ %1d  %2d %5d ", se,l, z);
  Serial.println(str);
}

void steuSet(byte stPos, unsigned int stZeit, byte stGas, unsigned int stDauer) {
  steuPos[stP] = stPos;
  steuZeit[stP] = stZeit;
  steuGas[stP] = stGas;
  steuDauer[stP] = stDauer;
  stP++;
}

void steuInit() {
  /* 1 vor Kurve
      2 vor Gerade her
      3 vor Kurve
      4 vor Gerade weg
  */

  stP = 0;
  steuSet(0, 0, 0, 0);
  steuSet(1, 1000,  0x42, 50);
  steuSet(1, 55555, 0x42, 50);
  // Test
  testPos = 0;
  testInc = 10;
  steuSet(2, 400,  0x42, 100);
  steuSet(2, 500,  0x42, 100);
  steuSet(2, 600,  0x42, 100);
  steuSet(2, 55555, 0x43, 100);
  // Ergebnis
  steuSet(3, 300,   0x42, 50);
  steuSet(3, 400,   0x42, 50);
  steuSet(3, 500,   0x42, 50);
  steuSet(3, 1500,  0x42, 50);
  steuSet(3, 2000,  0x42, 50);
  steuSet(3, 55555, 0x42, 50);

  steuSet(4, 500,   0x43, 50);
  steuSet(4, 55555, 0x43, 50);
  steuSet(5, 500,   0x43, 50);
  steuSet(5, 55555, 0x43, 50);
  steuSet(6, 500,   0x43, 50);
  steuSet(6, 55555, 0x43, 50);

  steuSet(0, 0, 0, 0);
  msg("steuInit ", stP);
}

void toEprom(byte b) {
  EEPROM.update(eadr, b) ;
  printByte (b);
  eadr++;
}

void toEpromUns(unsigned int v) {
  byte b;
  b = lowByte(v);
  toEprom( b) ;
  b = highByte(v);
  toEprom( b) ;
}

byte fromEprom() {
  byte b;
  b = EEPROM.read(eadr) ;
  eadr++;
  printByte (b);
  return b;
}

unsigned int fromEpromUns() {
  byte bl, bh;
  unsigned int tmp;
  bl = fromEprom();
  bh = fromEprom() ;
  tmp = 256 * bh + bl;
  return tmp;
}

void toEpromAll() {
  eadr = 0;
  Serial.print("to  Eprom: ");
  for (int i = 0; i < sensMax; i++) {
    toEprom(sensMap[i]);
    toEprom(sensInv[i]);
  }
  Serial.println("\r\n Steu: ");
  for (int i = 0; i < steuMax; i++) {
    toEprom(steuPos[i]);
    toEpromUns(steuZeit[i]);
    toEprom(steuGas[i]);
    toEpromUns(steuDauer[i]);
    Serial.println();
  }
  msg("EADR", eadr);
}

void fromEpromAll() {
  eadr = 0;
  Serial.print("fromEprom: ");
  for (int i = 0; i < sensMax; i++) {
    sensMap[i] = fromEprom();
    sensInv[i] = fromEprom();
  }
  for (int i = 0; i < steuMax; i++) {
    steuPos[i] = fromEprom();
    steuZeit[i] = fromEpromUns();
    steuGas[i] = fromEprom();
    steuDauer[i] = fromEpromUns();
  }
  msg("EADR", eadr);
}


void testdigit() {
  for (int r = 0; r < anzR; r++) {
    for  (int i = 0; i < 8; i++) {
      lc.setDigit(r, i, i, false);
    }
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
  msg("Intens ", intens);
}

void showTime() {
  lc.home();
  lc.show4DigS(minute);
  lc.show4DigS(second);
}

void getPoti() {
  poti = analogRead(A6) / 4;
}

void showAnalog() {
  lc.home();
  getPoti();
  lc.show4DigS(poti);
}

void showZiff() {
  lc.home();
  for  (int i = 1; i < 7; i++) {
    if (sens[i])  {
      lc.home();
      lc.putch(i + '0');
      lc.putch(i + '0');
    }
  }
}

void showDigital() {
  lc.home();
  for  (int i = 1; i < sensMax; i++) {
    lc.showBol(sens[i]);
  }
  //  lc.showBol(digitalRead(kyBtPin));
}

void showMap() {
  lc.home();
  for  (int i = 1; i < sensMax; i++) {
    lc.show2Dig(sensMap[i]);
  }
}

void readSens() {
  for  (int i = 1; i < sensMax; i++) {
    sens[i] = !digitalRead(sensMap[i]);
    if (sensInv[i]) {
      sens[i] = !sens[i];
    }
  }
}

void showZeit(int sne) {
  lc.home();
  lc.putch(sne + '0');
  lc.putch(' ');
  lc.putch(' ');
  lc.show4DigS(sensZeit[sne]);
}

void showZeitTop(int sne) {
  lc.home();
  lc.putch(sne + '0');
  lc.putch('W');
  lc.putch(' ');
  lc.show4DigS(sensZeitTop[sne]);
}

byte findSteuTab(byte pos, unsigned int zeit) {
  // returns the position in the steutab
  // usually next from steuPtr:
  for (int i = steuPtr; i < steuMax; i++) {
    if (steuPos[i] == pos) {
      if (steuZeit[i] > zeit) {
        return i;
      } // zeit
    } // pos
  } // next

  for (int i = 1; i < steuMax; i++) {
    if (steuPos[i] == pos) {
      if (steuZeit[i] > zeit) {
        return i;
      } // zeit
    } // pos
  } // next

  msg("findSteuTab kaputt, pos ", pos);
  return 1;
}

byte findStep(byte wrt) {
  // returns the step for given 255 value
  for (int i = 0; i < 10; i++) {
    if (wrt <= gasW[i]) {
      return i;
    }
  } // next
  msg("findStep kaputt, wrt ", wrt);
  return 0;
}

bool detectPos() {
  //returns hit sens
  for (int i = 1; i < sensMax; i++) {
    if (sens[i]) {
      strePos = i - 1;
      incStrePos();
      return true;
    }
  }
  return false;
}

void incStrePos() {
  strePos = strePos + 1;
  if (strePos >= sensMax) {
    strePos = 1;
  }
  strePrev = strePos - 1;
  if (strePrev < 1) {
    strePrev = sensMax - 1;
  }
}

void runit() {
  byte gas;
  byte gasX;
  unsigned int zeit;
  if (strePos == 0) {  // wait for first sens
    analogWrite(motPin, gasW[2]); // moderate speed
    if (!detectPos()) {
      return;
    }
  }

  if (sens[strePos]) {
    sensMs[strePos] = currMs;
    zeit = currMs - sensMs[strePrev];
    steuPtr = findSteuTab(strePos, zeit);
    gas = steuGas[steuPtr] >> 4;  // take higher Nibb
    gasX = gasW[gas];
    if (limit) {
      if (gasX > poti) {
        gasX = poti;
      }
    }
    analogWrite(motPin, gasX );
    // lots of time from here:
    printMess(strePos, zeit);
    if (zeit < sensZeitTop[strePos]) {
      sensZeitTop[strePos] = zeit;
    }
    sensZeit[strePos] = zeit;
    switchMs = currMs + steuDauer[steuPtr];
    lc.home();
    lc.putch(strePos + '0');
    lc.putch(' ');
    lc.show4DigS(zeit);
    lc.putch(' ');
    lc.putch(gas + '0');

    if (strePos == testPos) {
      steuDauer[steuPtr] += testInc;
      printInc(steuPtr,strePos,steuDauer[steuPtr]);
    }
    // done with this
    incStrePos();
  } else { //no Sens, umschalten?
    if (switchMs <= currMs) {
      gas = steuGas[steuPtr] & 0xF;
      gasX = gasW[gas];
      /*      if (gasX > poti) {
              gasX = poti;
            }
      */
      analogWrite(motPin, gasX );
      lc.lcCol = 0;
      lc.putch(gas + '0');
    }
  }
}

void doCmd(byte tmp) {
  if ((tmp > 47) && (tmp < 58)) {
    // switch off motpin
    //digitalWrite(motPin, LOW);
    runmode = tmp - '0';
    lc.cls();
    for (int n = 0; n < 4; n++) {
      lc.putch(men0Txt[runmode][n]);
    }
    msg("Runmode", runmode);
    waitCnt = 4;
    strePos = 0; // force re-detection
    actn = 0;
    return;
  }
  Serial.println();
  switch (tmp) {
    case '+':   //
      setIntens('+');
      break;
    case '-':   //
      setIntens('-');
      break;
    case 'a':   //
      showAnalog();
      break;
    case 'd':   //
      testdigit();
      break;
    case 'e':   //
      fromEpromAll();
      break;
    case 'E':   //
      toEpromAll();
      break;
    case 'i':   //
      for (int i = 0; i < sensMax; i++) {
        sensInv[i] = !sensInv[i];
      }
      msg("Inv now", sensInv[0]);
      break;
    case 'j':   //
      break;
    case 'l':   //
      limit = !limit;
      msg("limit ", limit);
      break;

    case 'm':   //
      Serial.print("Mot Lo");
      digitalWrite(motPin, LOW);
      break;
    case 'M':   //
      Serial.print("Mot Hi");
      digitalWrite(motPin, HIGH);
      break;
    case 'n':   //
      Serial.print("A Input");
      pinMode(A6, INPUT);
      break;
    case 'p':   //
      Serial.print("A Pullp");
      pinMode(A6, INPUT_PULLUP);
      break;
    case 'q':   //
      break;
    case 's':   //
      break;
    case 't':   //
      testPos++;
      if (testPos >= sensMax) {
        testPos = 0;
      }
      msg("testPos", testPos);
      break;
    case 'u':   //
      kyTime = kyTime + 50;
      msg("kyTime", kyTime);
      break;

    case 'x':
      for (int r = 0; r < anzR; r++) {
        lc.shutdown(r, true);
      }
      break;
    case 'y':
      for (int r = 0; r < anzR; r++) {
        lc.shutdown(r, false);
      }
      break;

    case 'z':   //
      kyTime = kyTime - 50;
      msg("kyTime", kyTime);
      break;

    default:
      Serial.print(tmp);
      lc.putch(tmp);
  } //case
}

int kyChange() {
  int chg = 0;
  bool aVal;
  aVal = digitalRead(kyClPin);
  if (nextKy <= currMs) {
    if (aVal != kyLast) { // Means the knob is rotating
      if (digitalRead(kyDtPin) != aVal) {  // Means pin A Changed first
        chg = 1;
      } else {// Otherwise B changed first
        chg = -1;
      }
      nextKy = currMs + kyTime;
      //msg("Encoder: ", encoderPosCount);
    }
  }
  kyLast = aVal;
  return chg;
}

void lcReset() {
  int devices = lc.getDeviceCount();
  msg("Reset for ", devices);
  for (int address = 0; address < devices; address++) {
    /*The MAX72XX is in power-saving mode on startup*/
    lc.shutdown(address, false);
    lc.setIntensity(address, 2);
    lc.clearDisplay(address);
  }
}

void showSteu() {
  char str[50];
  sprintf(str, "%3d %3d  %5u %02X %3d", stP, steuPos[stP], steuZeit[stP], steuGas[stP], steuDauer[stP]);
  Serial.println(str);
}

void edit(char c) {
  // edit Sensor
  byte cn;
  if (c == 8) { // BS emergency exit
    msg("back to 0", 0);
    runmode = 0;
    editSen = 0;
    return;
  }
  if (editSen == 0) {
    if ((c >= '0') && (c <= '9')) {
      editSen = c - '0';
      stP = findSteuTab(editSen, 0);
      editWhat = 1;
    } else {
      msg("Select Sensor ", c);
      return;
    }
  }
  /*
    // if not numeric:
    if (editWhat < 10) {
    if ((c < '0') || (c > '9')) {
      msg("Number!, editWhat=", editWhat);
      return;
    }
    }
  */
  cn = c - '0';
  switch (editWhat) {
    case  0: // after completing
      if (c == '+') {
        stP += 1;
        showSteu();
        editWhat=1;
        return;
      }
      if (c == '-') {
        stP -= 1;
        showSteu();
        editWhat=1;
        return;
      }
      break;
    case  1: // show entry elect
      showSteu();
      Serial.print("+- 0..9       >");
      editWhat = 5;
      break;
    case  5: // enter first or +-
      if (c == '+') {
        stP += 1;
        Serial.println(c);
        showSteu();
        return;
      }
      if (c == '-') {
        stP -= 1;
        Serial.println(c);
        showSteu();
        return;
      }
      cn = cn & 0x0F; // brute
      c = cn + '0';
      Serial.print(c);
      cn = cn << 4;
      steuGas[stP] = cn | (steuGas[stP] & 0x0F);
      editWhat = 6;
      break;
    case  6: // enter second
      cn = cn & 0x0F;
      c = cn + '0';
      Serial.print(c);
      steuGas[stP] = cn | (steuGas[stP] & 0xF0);
      editWhat = 10;
      break;
    case  10: // dauer
      if (c == '+') {
        steuDauer[stP] += 10;
        Serial.print(c);
        showSteu();
        return;
      }
      if (c == '-') {
        steuDauer[stP] -= 10;
        Serial.print(c);
        showSteu();
        return;
      }
      if (c == 13) {
        Serial.println();
        showSteu();
        msg("Done", stP);
        editWhat = 0;
        return;
      }
      msg("Satz mit x...", c);
      editWhat = 1;
      break;

    default:
      msg("EditPanic", editWhat);
      editSen = 0;
      runmode = 0;
  } //case
}

void setup() {
  const char info[] = "Max2719Runner " __DATE__ " " __TIME__;
  Serial.begin(38400);
  Serial.println(info);
  //TODO: depends on Eprom!
  for  (int i = 1; i < sensMax; i++) {
    pinMode(sensMap[i], INPUT_PULLUP);
  }
  pinMode(kyBtPin, INPUT_PULLUP);
  pinMode(kyClPin, INPUT_PULLUP);
  pinMode(kyDtPin, INPUT_PULLUP);
  pinMode(motPin,  OUTPUT);
  lcReset();
  lc.lcMs = true; // show DP for zeit
  kyLast = digitalRead(kyClPin);
  steuInit();
  doCmd(mDig + '0');
}

void loop() {
  int kyChg;
  int val;
  char c;
  if (Serial.available() > 0) {
    c = Serial.read();
    if (runmode == mEdi) {
      edit(c);
    } else {
      doCmd(c);
    }
  } // avail

  kyChg = kyChange();
  if (kyChg != 0) {
    doCmd(runmode + kyChg + '0');
  } // ky change

  readSens();
  currMs = millis();
  // Emergency
  if (!digitalRead(kyBtPin)) {
    waitCnt = 0;
    if ( runmode != mMot) {
      msg("Emergency", runmode);
      runmode = mMot;
    }
  }
  if (runmode == mRun) {
    runit();
  } else {
    lopCnt = lopCnt + 1;

    // every 100ms
    if (nextMs <= currMs) {
      nextMs = currMs + delayTime;
      if (waitCnt > 0) {
        waitCnt--;
      }
      if (waitCnt == 0) {
        if ((runmode > 0) && (runmode < 5)) {
          if (actn != runmode) {
            actn = runmode;
            showZeitTop(runmode);
          }

        } else {
          switch (runmode) {
            case  mDig:
              showDigital();
              break;
            case  mAna:
              lc.cls();
              showZiff();
              break;
            case  mMot:
              showAnalog();
              analogWrite(motPin, poti);
              val = findStep(poti);
              lc.putch(' ');
              lc.show2Dig(val);
              break;
          } //case
        } // else
      }// waitCnt
    } // nextMs

    // every second
    if (nextTick <= currMs) {
      nextTick = currMs + tickTime;
      second = second + 1;
      if (second > 59) {
        second = 0;
        minute = minute + 1;
        if (minute > 99) {
          minute = 0;
        }
      }
      if (runmode == mLps) {
        lc.lcRow = 0;
        lc.lcCol = 4;
        lc.show5Dig(lopCnt);
        msg("lopCnt", lopCnt);
        lopCnt = 0;
      }
    } // nextTick
  } // else runmode
}
























































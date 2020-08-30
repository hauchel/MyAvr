//  Stopuhr mit 5*8 * LED
#include <LedCon5.h>
#include <EEPROM.h>

/*
  KY-40 Drehgeber
  D2   SW        blue
  D3   Clock     yellow
  D4   Data      green
  D5   Beleu     pink
  
  Display
  D9   DataIn    white
  D8   CS/Load   grey
  D7   CLK       pink

  Inputs
  A0   orange
  A1   yell
  A2   green
  A3   blue

*/

const byte menuMax = 10;
const char* men0Txt[menuMax] = {"0TO ", "1UP ", "2AL ", "3LU ", "4PP ", "5ET ", "6LP ", "LP  ", "8ET ", "9ET "};
const byte mNix = 0; //
const byte mUpt = 1; //  Uptime
const byte mAna = 2; //  show Analog
const byte mDig = 3; //  show Digital
const byte mUsr = 4; //   User
const byte mRun = 5; //
const byte mTop = 6; //   Top Scores
const byte mLps = 7; //   Loops per second
const byte mSto = 8; //   Store
const byte mMap = 9; //  show Port Mapping

byte runmode;
byte actn; // only once after encoder change

const byte anzR = 5; // number of LED Disps
//             dataPin, clkPin, csPin, numDevices
LedCon5 lc = LedCon5(9, 7, 8, anzR);

const byte kyBtPin = 2;
const byte kyClPin = 3;
const byte kyDtPin = 4;
const byte beleuPin = 5;
const byte anaPin[4] = {3, 2, 1, 0};

// Strecke aus max 8 Sensoren:
const byte streMax = 8;
byte stre[streMax] = {4, 3, 2, 1, 0, 0, 0, 0};
const byte sensMax = 5;     // sens 0 is not used
// value is true if car is over
bool sens[sensMax];    // 0  1    2  3    4
byte sensMap[sensMax] = {99, 14, 15, 16, 17}; //, 16,  17, 10, 11, 12,  13};
bool sensInv[sensMax] = {true, false, false, false, false}; //, true, true, false, false, true, true}; // to invert
// top speeds
const byte topsMax = 5;
unsigned int tops[topsMax];
byte topUsr[topsMax];
const byte usrMax = 5;     // 0    1     2     3
char* usrTxt[usrMax] = {"ET", "HH", "SH", "CH", "  "};
byte usrOn = 0;
// ky-40 encoder
int encoderPosCount = 0;
int kyLast;
// uptime
int second = 0;
int minute = 0;
byte intens = 8;
// State each runner
byte runner;
byte state[3];
bool anal = true;
int eadr = 0;       // EEprom Addr

unsigned long currMs, nextMs, nextTick, nextKy;
unsigned long timAR[3], timCR[3];
unsigned long lopCnt = 0;
unsigned long delayTime = 100;
unsigned long tickTime = 1000;  // each second
unsigned long kyTime = 100;    // ky40 debounce
unsigned long kareTime = 1000;

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
/*
  Daten im Eprom gespeichert:
  top  List
  user Name
  top 5
  Strecke1 bis zu 8 Messstellen , 0 = Ende nicht belegt
  Strecke2 bis zu 8 Messstellen
  SensMax
  für jeden sens (ab 1!)zwei byte
   sens Map
   sensInv
*/

void toEprom(byte b) {
  EEPROM.update(eadr, b) ;
  eadr++;
}

void toEpromInt(unsigned int v) {
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
  return b;
}

unsigned int fromEpromInt() {
  byte bl, bh;
  int tmp;
  bl = fromEprom();
  bh = fromEprom() ;
  tmp = 256 * bh + bl;
  return tmp;
}

void toEpromAll() {
  eadr = 0;
  Serial.print("to  Eprom: ");
  for (int i = 0; i < streMax; i++) {
    toEprom( stre[i]);
    printByte (stre[i]);
  }
  Serial.println("!");
}

void fromEpromAll() {
  eadr = 0;
  Serial.print("fromEprom: ");
  for (int i = 0; i < streMax; i++) {
    stre[i] = fromEprom();
    printByte (stre[i]);
  }
  Serial.println("!");
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

void showAnalog() {
  int av;
  for  (int i = 0; i < 4; i++) {
    lc.lcRow = i;
    lc.lcCol = 7;
    av = analogRead(anaPin[i]);
    lc.show4DigS(av);
    lc.putch(' ');
    if (av < 950) {
      lc.putch('L');
    } else {
      lc.putch('H');
    }

  }
}

void showDigital() {
  lc.home();
  for  (int i = 1; i < sensMax; i++) {
    lc.showBol(sens[i]);
  }
  lc.showBol(digitalRead(kyBtPin));
}

void initTops() {
  tops[0] = 11111;
  topUsr[0] = 3;
  tops[1] = 22222;
  topUsr[1] = 4;
  tops[2] = 33333;
  tops[3] = 44444;
  tops[4] = 55555;
}

void showTops() {
  lc.home();
  for  (int i = 0; i < topsMax; i++) {
    lc.putch(usrTxt[topUsr[i]][0]);
    lc.putch(usrTxt[topUsr[i]][1]);
    lc.putch(' ');
    lc.show5Dig(tops[i]);
  }
}

void showUsr() {
  lc.cls();
  for  (int i = 0; i < usrMax; i++) {
    lc.lcCol = 7;
    lc.lcRow = i;
    for (int n = 0; n < 2; n++) {
      lc.putch(usrTxt[i][n]);
    }
    lc.putch(' ');
    if (i == usrOn) {
      lc.putch('A');
    } else {
      lc.putch(' ');
    }
  }
}

int checkTops(unsigned int zeit) {
  // faulsort, returns row  if inserted
  unsigned int tmp;
  byte tmpU;
  byte tmpUon;
  int row = -1;
  for  (int i = 0; i < topsMax; i++) {
    if (tops[i] > zeit) {
      if (row < 0) {
        row = i;
        tmpUon = usrOn;
      }
      tmp = tops[i];
      tops[i] = zeit;
      zeit = tmp;
      tmpU = topUsr[i];
      topUsr[i] = tmpUon;
      tmpUon = tmpU;
    }
  }
  return row;
}

void showMap() {
  lc.home();
  for  (int i = 1; i < sensMax; i++) {
    lc.show2Dig(sensMap[i]);
  }
}

void readSensDig() {
  for  (int i = 1; i < sensMax; i++) {
    sens[i] = !digitalRead(sensMap[i]);
    if (sensInv[i]) {
      sens[i] = !sens[i];
    }
  }
}

void readSensAna() {
  int av;
  for  (int i = 0; i < 4; i++) {
    av = analogRead(anaPin[i]);
    if (av < 950) {
      sens[i + 1] = false;
    } else {
      sens[i + 1] = true;
    }
  }
}
/* State each runner
    0  Unknown
       if Sensor, take time1, to 1
    1  Karenz
       to 2 after xx
    2  Mess
       if Sensor, take time1, show, to 1
*/
void lcpos() {
  lc.lcRow = runner;
  lc.lcCol = 7;
}

void checkRound(bool la) {
  // evals laptime for selected sens
  unsigned int zeit;
  int rotz;
  switch (state[runner]) {
    case 0:  // unknown
      if (la) {
        state[runner] = 1;
        timAR[runner] = currMs;
        timCR[runner] = currMs + kareTime;
        lcpos();
        lc.putch('A');
      }
      break;
    case 1: // hit, set wait time
      if (currMs > timCR[runner]) {
        state[runner] = 2;
        lcpos();
        lc.putch('C');
      }
      break;
    case 2: // hit after wait time 
      if (la) {
        state[runner] = 1;
        zeit = currMs - timAR[runner];
        /*        rotz = checkTops(zeit);
                if (rotz >= 0) {
                  showTops();
                  lc.lcRow = rotz;
                  lc.lcCol = 7;
                  lc.putch('C');
                  lc.putch('C');
               }
        */
        timAR[runner] = currMs;
        timCR[runner] = currMs + kareTime;
        lcpos();
        lc.putch('A');
        lc.putch(' ');
        lc.putch(' ');
        //        if  (rotz < 0) {
        lc.show5Dig(zeit);
        //      }
      }
      break;
    default:
      msg("state kaputt", state[runner]);
      state[runner] = 0;
  } // case
}



void doCmd(byte tmp) {
  if ((tmp > 47) && (tmp < 58)) {
    runmode = tmp - '0';
    lc.cls();
    for (int n = 0; n < 4; n++) {
      lc.putch(men0Txt[runmode][n]);
    }
    if (runmode==mNix) {
     digitalWrite(beleuPin, LOW);
    } else {
      digitalWrite(beleuPin, HIGH);
    }
    msg("Runmode", runmode);
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
      anal = !anal;
      msg("Anal ", anal);
      break;
    case 'b':   //
      msg("Beleu ", 0);
      digitalWrite(beleuPin, LOW);
      break;
    case 'B':   //
      msg("Beleu ", 1);
      digitalWrite(beleuPin, HIGH);
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
      analogReference(INTERNAL);
      Serial.print("Analog Internal");
      break;
    case 'j':   //
      analogReference(DEFAULT);
      Serial.print("Analog Default");
      break;
    case 'n':   //
      Serial.print("A0 Input");
      pinMode(A0, INPUT);
      break;
    case 'p':   //
      Serial.print("A0 Pullp");
      pinMode(A0, INPUT_PULLUP);
      break;

    case 't':   //
      showTops();
      break;

    case 'q':   //
      checkTops(1234);
      showTops();
      break;

    case 'u':   //
      showUsr();
      /*     kyTime = kyTime + 50;
            msg("kyTime", kyTime);
      */
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
      if (digitalRead(kyDtPin) != aVal) {  // Means pin A Changed first - We're Rotating Clockwise
        chg = -1;
        encoderPosCount --;
      } else {// Otherwise B changed first and we're moving CCW
        chg = 1;
        encoderPosCount++;
      }
      nextKy = currMs + kyTime;
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

void setup() {
  const char info[] = "Max2719X5  "__DATE__ " " __TIME__;
  Serial.begin(38400);
  Serial.println(info);
  //TODO: depends on Eprom!
  for  (int i = 1; i < sensMax; i++) {
    pinMode(sensMap[i], INPUT_PULLUP);
  }
  pinMode(kyBtPin, INPUT_PULLUP);
  pinMode(kyClPin, INPUT_PULLUP);
  pinMode(kyDtPin, INPUT_PULLUP);
  pinMode(beleuPin, OUTPUT);
  digitalWrite(beleuPin, LOW);
  lcReset();
  lc.lcMs = true; // show DP for zeit
  doCmd(mRun + '0');
  initTops();
  showTops();
  kyLast = digitalRead(kyClPin);
}

void loop() {
  int kyChg;
  if (Serial.available() > 0) {
    doCmd(Serial.read());
  } // avail

  kyChg = kyChange();
  if (kyChg != 0) {
    doCmd(runmode + kyChg + '0');
  } // ky change

  if (anal) {
    readSensAna();
  } else {
    readSensDig();
  }
  
  currMs = millis();
  if (runmode == mRun) {
    runner = 1;
    checkRound(sens[1]);
    runner = 2;
    checkRound(sens[2]);
  } else {
    lopCnt = lopCnt + 1;

    // every 100ms
    if (nextMs <= currMs) {
      nextMs = currMs + delayTime;
      switch (runmode) {
        case mAna:   //
          showAnalog();
          break;
        case  mDig:
          showDigital();
          break;
        case  mMap:
          if (actn != mMap) {
            showMap();
            actn = mMap;
          }
          break;
        case  mUsr:
          if (actn != mUsr) {
            showUsr();
            actn = mUsr;
          }
          if (!digitalRead(kyBtPin)) {
            usrOn++;
            if (usrOn >= usrMax) {
              usrOn = 0;
            }
            actn = 0;
          }
          break;
        case  mTop:
          if (actn != mTop) {
            showTops();
            actn = mTop;
          }
          if (!digitalRead(kyBtPin)) {
            initTops();
            actn = 0;
          }
          break;
      } //case
    } // nextMs

    // every second
    if (nextTick <= currMs) {
      nextTick = currMs + tickTime;
      second = second + 1;
      if (second > 59) {
        second = 0;
        minute = minute + 1;
      }
      if (runmode == mUpt) {
        showTime();
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
























































//  Stopuhr mit 8 * LED und analog comp und timer1 und timer2
#include <LedCon5.h>
#include <EEPROM.h>

/*
  KY-40
  D3   Clock gelb
  D4   Data grün
  Display
  D9   DataIn    white
  D8   CS/Load   grey
  D7  CLK       pink (!)
  Timer 1
  D5   T1        brown
  D6   AIN0      green
  D7   AIN1      blau
  D8   ICP1
  D9   OC1A
  D10  OC1B
  Inputs
  A0   orange
  A1   yell
  A2   gree
  A3   blue

  runmode
  0
  1     Uptime
  2     Zeige Analog
  3     Trigger count downto
  4     Physical Ports
  5     ZeitMess zwischen A/B und C/D
  6     runcount

  Daten byte Eprom gespeichert
  Strecke1 bis zu 8 Messstellen , 0 = Ende nicht belegt
  Strecke2 bis zu 8 Messstellen
  SensMax
  für jeden sens (ab 1!)zwei byte
   sens Map
   sensInv
  Port

*/

byte runmode;

const byte anzR = 1; // number of LED Disps
//             dataPin, clkPin, csPin, numDevices
LedCon5 lc = LedCon5(9, 7, 8, anzR);
byte intens = 8;
const byte kyClPin = 3;
const byte kyDtPin = 4;

const byte anaAPin = 0;
const byte anaBPin = 1;
const byte anaCPin = 2;
const byte anaDPin = 3;

const byte menuMax = 10;
const char* men0Txt[menuMax] = {"0TO ", "1UP ", "2AL ", "3LU ", "4PP ", "5ET ", "6LP ", "7Uu ", "8ET ", "9ET "};
// Strecke aus 8 Sensoren:
const byte streMax = 8;
byte stre[streMax] = {4, 3, 2, 1, 0, 0, 0, 0};
const byte sensMax = 9;     // sens 0 is not used
// value is true if car is over
bool sens[sensMax];    // 0  1    2  3    4
byte sensMap[sensMax] = {99, 14, 15, 16,  17, 10, 11, 12,  13};
bool sensInv[sensMax] = {true, false, false, true, true, false, false, true, true}; // to invert

int encoderPosCount = 0;
int pinALast;

int second = 0;
int minute = 0;
/* State each runner
    0  unknown
       first Sensor, take time1, to 1
    1  running
       second Sensor take time2, show, to2
    2  Karenz
       to 0 after xx
*/
byte stateR1, stateR2;

int eadr = 0;       // EEprom Addr
int anaA, anaB;

unsigned long currMs, nextMs, nextTick, nextKy;
unsigned long timAR1, timBR1, timCR1;
unsigned long lopCnt = 0;
unsigned long delayTime = 100;
unsigned long tickTime = 1000;
unsigned long kyTime = 100;

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

void toEprom(byte b) {
  EEPROM.update(eadr, b) ;
  eadr++;
}

void toEpromInt(int v) {
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

int fromEpromInt() {
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
  for  (int i = 0; i < 8; i++) {
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
  msg("Intens ", intens);
}

void showTime() {
  lc.home();
  lc.show4DigS(minute);
  lc.show4DigS(second);
}

void showAnalog() {
  lc.home();
  lc.show4DigS(anaA);
  lc.show4DigS(anaB);
}

void showDigital() {
  lc.home();
  for  (int i = 1; i < sensMax; i++) {
    lc.showBol(sens[i]);
  }
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

void checkState(bool la, bool lb) {
  int zeit;
  switch (stateR1) {
    case 0:
      if (la) {
        stateR1 = 1;
        timAR1 = currMs;
        lc.home();
        lc.putch('A');
      }
      break;
    case 1:
      if (lb) {
        stateR1 = 2;
        timBR1 = currMs;
        timCR1 = timBR1 + 1000;
        zeit = timBR1 - timAR1;
        lc.putch('C');
        lc.putch(' ');
        lc.putch(' ');
        lc.show4DigS(zeit);
      }
      break;
    case 2:
      if (currMs > timCR1) {
        stateR1 = 0;
        lc.home();
        lc.putch(' ');
        lc.putch(' ');
      }
      break;
    default:
      stateR1 = 0;
  } // case
}

void doCmd(byte tmp) {
  long zwi;
  if ((tmp > 47) && (tmp < 58)) {
    runmode = tmp - '0';
    lc.cls();
    for (int n = 0; n < 4; n++) {
      lc.putch(men0Txt[runmode][n]);
    }
    msg("Runmode", runmode);
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
      kyTime = kyTime - 50;
      msg("kyTime", kyTime);
      break;

    case 'u':   //
      kyTime = kyTime + 50;
      msg("kyTime", kyTime);
      break;

    case 'x':
      lc.shutdown(0, true);
      break;
    case 'y':
      lc.shutdown(0, false);
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
    if (aVal != pinALast) { // Means the knob is rotating
      if (digitalRead(kyDtPin) != aVal) {  // Means pin A Changed first - We're Rotating Clockwise
        chg = 1;
        encoderPosCount ++;
      } else {// Otherwise B changed first and we're moving CCW
        chg = -1;
        encoderPosCount--;
      }
      nextKy = currMs + kyTime;
      msg("Encoder: ", encoderPosCount);
    }
  }
  pinALast = aVal;
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
  const char info[] = "StopMax2719  "__DATE__ " " __TIME__;
  Serial.begin(38400);
  Serial.println(info);
  //TODO: depends on Eprom!
  for  (int i = 1; i < sensMax; i++) {
    pinMode(sensMap[i], INPUT_PULLUP);
  }
  pinMode(kyClPin, INPUT_PULLUP);
  pinMode(kyDtPin,  INPUT_PULLUP);
  pinMode(anaAPin, INPUT_PULLUP);
  pinMode(anaBPin, INPUT_PULLUP);
  pinMode(anaCPin, INPUT_PULLUP);
  pinMode(anaDPin, INPUT_PULLUP);
  lcReset();
  runmode = 5;
  doCmd('5');
}

void loop() {
  int kyChg;
  if (Serial.available() > 0) {
    doCmd(Serial.read());
  } // avail
  kyChg = kyChange();
  if (kyChg != 0) {
    doCmd(runmode + kyChg + '0');
  }
  readSens();
  currMs = millis();
  if (runmode == 5) {
    checkState(sens[3], sens[4]);
  } else {
    lopCnt = lopCnt + 1;


    // every 100ms
    if (nextMs <= currMs) {
      nextMs = currMs + delayTime;

      if (runmode == 2) {
        anaA = analogRead(anaAPin);
        anaB = analogRead(anaBPin);
        showAnalog();
      }
      if (runmode == 3) {
        showDigital();
      }
      if (runmode == 4) {
        showMap();
      }
    } // nextMs

    // every second
    if (nextTick <= currMs) {
      nextTick = currMs + tickTime;
      second = second + 1;
      if (second > 59) {
        second = 0;
        minute = minute + 1;
      }
      if (runmode == 1) {
        showTime();
      }
      if (runmode == 6) {
        lc.lcCol = 4;
        lc.show5Dig(lopCnt);
        msg("lopCnt", lopCnt);
        lopCnt = 0;
      }
    } // nextTick
  } // else
}
























































//  Eieruhr mit 8 * LED
#include <LedCon5.h>
#include <avr/sleep.h>
/*
  D9  DataIn    white
  D8  CS/Load   grey
  D7  CLK       pink

  D4  Pieps

  Spannung vs volt
*/
const byte anzR = 1; // number of LED Disps
//             dataPin, clkPin, csPin, numDevices
LedCon5 lc = LedCon5(9, 7, 8, anzR);
const int piepPin = 4;

int menuPos = 0; // Menu-Position
const int menuMax = 3;
const char* men0Txt[menuMax] = {"    ", "SPA ", "SET "};


int keyAna0 = 0;

byte intens = 8;

unsigned long nextMs, currMs, nextTick;
unsigned long delayTime = 100;
unsigned long tickTime = 1000;
int uhrMode = 0;  // 0 stop, 1 run
int spannMode = 0;  // 0 stop, 1 run
int menuAn = 1;    // 0 kein Menu , 1 Menu
int minut = 5;    // initial values
int second = 1;
int piepMode = 0;  //Dauer*delaytime
int piepCnt = 0;

int anaTop = 600;  // Reference Value if no key pressed, is updated

const byte keyBoth = 0;
const byte keyDown = 1;
const byte keyRght = 2;
const byte keyNop = 9;
byte kbdOld = 0;
byte kbdCnt = 0;

void msg(const char txt[], int n) {
  Serial.print(txt);
  Serial.print(" ");
  Serial.println(n);
}


void calcAna() {
  // fro anaref

}
byte kbdPress() {
  //
  //  Volt   0    fix   lim2   lim3     anaRef
  //          both   rght    dwn     nop
  //
  keyAna0 = analogRead(A0);
  if (keyAna0 < 100) {
    return keyBoth;
  }
  if (keyAna0 < 430) {
    return keyRght;
  }
  if (keyAna0 < 580) {
    return keyDown;
  }
  return keyNop;
}

byte kbdRead() {
  byte ths = kbdPress();
  if (ths != kbdOld) {
    kbdOld = ths;
    kbdCnt = 5; // delay after first
    return ths;
  }
  kbdCnt--;
  if (kbdCnt > 0) {
    return keyNop;
  }
  kbdCnt = 3; // delay repeat
  return ths;
}

void zuMenu() {
  // on entry to menu
  lc.home();
  for (int n = 0; n < 4; n++) {
    lc.putch(men0Txt[menuPos][n]);
  }
  switch (menuPos) {
    case 0:   //  Stopuhr initial values
      showZeit();
      break;
    case 1:   // Spannung
      break;
    case 2:   // dn
      lc.putch('2');
      break;
  }
}

void doMenu(byte key) {
  Serial.print(keyAna0);
  msg ("  key ", key);
  byte tmp;
  if (key == keyRght ) {
    switch (menuPos) {
      case 0:   //  Stopuhr
        if (uhrMode != 0) {
          uhrMode = 0;
        } else {
          uhrMode = 1;
        }
        break;
      case 1:   // Spannung
        if (spannMode == 0) {
          spannMode = 1;
        } else {
          spannMode = 0;
        }
        break;
      case 2:   // Set
        minut = 5;
        second = 1;
        showZeit();
        break;
      default:
        msg ("Menu kaputt ", menuPos);
    } //case
    return;
  } //keyRght

  if (key == keyDown ) {
    menuPos++;
    if (menuPos >= menuMax) {
      menuPos = 0;
    }
    zuMenu();
  }

  switch (menuPos) {
    case 0:
      lc.putch('0');
      break;
    case 1:
      spannung();
      break;
    case 2:
      lc.putch('2');
      break;
    default:
      msg ("Menu kaputt ", menuPos);
  } //case
}

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
}

void show() {
  lc.cls();
  int ana0 = analogRead(A0);
  lc.show4Dig(ana0);
}

void spannung() {
  // without load
  lc.shutdown(0, true);
  delay(500);
  int ana0L = analogRead(A0);

  for (int i = 0; i < 8; i++) {
    lc.setDigit(lc.lcRow, i, 8, true);
  }
  lc.shutdown(0, false);
  for (int r = 0; r < anzR; r++) {
    lc.setIntensity(r, 15);
  }
  delay(500);
  int ana0H = analogRead(A0);
  lc.cls();
  setIntens(' ');
  lc.show4Dig(ana0L);
  lc.show4Dig(ana0H);
  delay(1000);
}

void showZeit() {
  lc.lcCol = 3;
  lc.show2Dig(minut);
  lc.show2Dig(second);
}

void wachauf() {
}

void schlaf() {
  lc.shutdown(0, true);
  attachInterrupt(0, wachauf, LOW);
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();
  sleep_mode();
  sleep_disable();
  detachInterrupt(0);
  Serial.print("aufgewacht");
  lc.shutdown(0, false);
}


void doCmd(byte tmp) {
  Serial.println();
  switch (tmp) {
    case '0':   //
      nextMs = 0;
      break;
    case '1':   //
      tickTime = 1000;
      break;
    case '2':   //
      tickTime = 200;
      break;
    case '5':   //
      tickTime = 50;
      break;
    case 'd':   //
      testdigit();
      break;
    case 'f':   //
      //schlaf();
      break;
    case 'h':   //
      lc.setIntensity(0, 15);
      break;
    case 'i':   //
      analogReference(INTERNAL);
      Serial.print("Analog Internal");
      break;
    case 'j':   //
      analogReference(DEFAULT);
      Serial.print("Analog Default");
      break;
    case 'l':   //
      lc.setIntensity(0, 1);
      break;
    case 'm':   //
      if (menuAn == 0) {
        menuAn = 1;
        Serial.print("Menu an");
      } else {
        menuAn = 0;
        Serial.print("Menu aus");
      }
      break;
    case 'n':   //
      Serial.print("A0 Input");
      pinMode(A0, INPUT);
      break;
    case 'p':   //
      Serial.print("A0 Pullp");
      pinMode(A0, INPUT_PULLUP);
      break;

    case 'r':   //
      reset();
      break;
    case 's':   //
      show();
      delay(1000);
      break;
    case 't':   //
      digitalWrite(piepPin, HIGH);
      break;
    case 'u':   //
      digitalWrite(piepPin, LOW);
      break;
    case 'v':
      spannung();
      break;
    case 'w':   //
      if (spannMode == 0) {
        spannMode = 1;
        Serial.print("Spann an");
      } else {
        spannMode = 0;
        Serial.print("Spann aus");
      }
      break;

    case 'x':
      lc.shutdown(0, true);
      break;
    case 'y':
      lc.shutdown(0, false);
      break;

    case '+':
      piepMode++;
      msg("PiepMode ", piepMode);
      break;
    case '-':
      piepMode--;
      msg("PiepMode ", piepMode);
      break;
    default:
      Serial.print(tmp);
      lc.putch(tmp);
  } //case
}


void reset() {
  int devices = lc.getDeviceCount();
  //we have to init all devices in a loop
  msg("Reset for ", devices);
  for (int address = 0; address < devices; address++) {
    /*The MAX72XX is in power-saving mode on startup*/
    lc.shutdown(address, false);
    /* Set the brightness to a medium values */
    lc.setIntensity(address, 5); //8
    /* and clear the display */
    lc.clearDisplay(address);
  }
}

void setup() {
  const char info[] = "EiMax2719  "__DATE__ " " __TIME__;
  pinMode(piepPin, OUTPUT);
  digitalWrite(piepPin, HIGH);
  Serial.begin(38400);
  Serial.println(info);
  pinMode(2, INPUT_PULLUP);
  digitalWrite(piepPin, LOW);
  pinMode(A0, INPUT_PULLUP);
  reset();
  spannung();
  minut = 5;
  second = 1;
  menuPos = 0;
  zuMenu();
  uhrMode=1;
}

void loop() {
  byte tast;
  if (Serial.available() > 0) {
    doCmd(Serial.read());
  } // avail

  currMs = millis();
  // every 100ms
  if (nextMs <= currMs) {
    nextMs = currMs + delayTime;
    if (piepCnt > 0) {
      piepCnt--;
      if (piepCnt <= 0) {
        digitalWrite(piepPin, LOW);
      }
    }
    /*tast = kbdRead();
    if  (menuAn == 0) {
      tast = keyNop;
    }
    if (tast != keyNop) {
      doMenu(tast);
    }*/
  } // next

  // every second
  if (nextTick <= currMs) {
    nextTick = currMs + tickTime;
    piepCnt = piepMode;
    if (piepCnt != 0) {
      digitalWrite(piepPin, HIGH);
    } else {
      digitalWrite(piepPin, LOW);  // just in case...
    }

    if (spannMode != 0) {
      lc.lcCol = 7;
      lc.show4Dig(keyAna0);
    }

    if (uhrMode != 0) {
      second --;
      if (second < 0) {
        second = 59;
        minut --;
        if (minut < 0) {
          piepCnt = 20;
          uhrMode = 0;
          piepMode = 0;
          digitalWrite(piepPin, HIGH);
        } // finished
      } // sec <0
      showZeit();
    }
  } //tick
}
























































































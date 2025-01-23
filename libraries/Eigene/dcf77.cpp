#include <dcf77.h>

void dcf77::msgF(const __FlashStringHelper *ifsh, uint16_t n) {
  PGM_P p = reinterpret_cast<PGM_P>(ifsh);
  while (1) {
    char c = pgm_read_byte(p++);
    if (c == 0) break;
    Serial.write(c);
  }
  Serial.write(" ");
  Serial.println(n);
}

void dcf77::dcfRead() {
  // on change add to ripu
  ripu[rinP] = millis();
  rinP++;
  if (rinP >= ripuL) {
    rinP = 0;
  }
}

void dcf77::showRipu() {
  char str[100];
  sprintf(str, "%5lu %5lu %5lu %5lu %5lu  %5lu %5lu %5lu %5lu %5lu ", ripu[0], ripu[1], ripu[2], ripu[3], ripu[4], ripu[5], ripu[6], ripu[7], ripu[8], ripu[9]);
  Serial.println(str);
}


void dcf77::showData() {
  char str[100];
#ifdef DEBUG
  msgF(F("DataP "), dataP);
  for (int i = 0; i < dataL; i += 8) {
    sprintf(str, "%3d   %5u %5u %5u %5u  %5u %5u %5u %5u ", i, data[i], data[i + 1], data[i + 2], data[i + 3], data[i + 4], data[i + 5], data[i + 6], data[i + 7]);
    Serial.println(str);
  }
#endif
}

void  dcf77::checkParity(byte b) {
  if ((b == 0) && !aktP) return;
  if ((b == 1) && aktP) return;
  //err(3);
  Serial.print("Parity aktP ");
  Serial.print(aktP);
  Serial.print("  b= ");
  Serial.println(b);
}

void dcf77::aktPlus(byte b) {
  if (b == 2) { //reset
    akt = 0;
    aktW = 1;
    aktP = false;
    return;
  }
  if (b == 1) {
    akt = akt + aktW;
    aktP = !aktP;
  }
  switch (aktW) {
    case 1:
      aktW = 2;
      break;
    case 2:
      aktW = 4;
      break;
    case 4:
      aktW = 8;
      break;
    case 8:
      aktW = 10;
      break;
    case 10:
      aktW = 20;
      break;
    case 20:
      aktW = 40;
      break;
    case 40:
      aktW = 80;
      break;
    case 80:
      aktW = 1;
      break;
    default:
      msgF(F("aktW invalid"), aktW);
      break;
  }
}

void dcf77::druff(byte b) {
  druP += 1;
  if (verb == 1) {
    Serial.print(b);
  }

  if (druP < 20) {
    return;
  }

  if (druP == 20) { // 1 at 20
    if (verb==1) Serial.print("/");
    if (b != 1) err(4);
    aktPlus(2);
    return;
  }
  if (druP < 28) {
    aktPlus(b);
    return;
  }
  if (druP == 28) { //parity at 28
    checkParity(b);
    if (error == 0) {
      minuteX = akt;
      msgF(F("Min"), akt);
    }
    aktPlus(2);
    return;
  }
  if (druP < 35) {
    aktPlus(b);
    return;
  }
  if (druP == 35) { //parity at 35
    checkParity(b);
    if (error == 0) {
      hourX = akt;
      msgF(F("Std"), akt);
    }
    aktPlus(2);
    return;
  }
  if (druP < 41) {
    aktPlus(b);
    return;
  }
  if (druP == 41) { // day ends 41
	aktPlus(b);
    if (error == 0) {
      msgF(F("Tag"), akt);
    }
    aktPlus(2);
    return;
  }
  if (druP < 44) {
    aktPlus(b);
    return;
  }
  if (druP == 44) { //dow ends 44
    aktPlus(b);
    if (error == 0) {
      msgF(F("WoT"), akt);
    }
    aktPlus(2);
    return;
  }
  if (druP < 49) {
    aktPlus(b);
    return;
  }
  if (druP == 49) { //month ends 49
    aktPlus(b);
    if (error == 0) {
      msgF(F("Mon"), akt);
    }
    aktPlus(2);
    return;
  }
  if (druP < 57) {
    aktPlus(b);
    return;
  }
  if (druP == 57) { //year parity 58
    if (error == 0) {
      msgF(F("Jahr"), akt);
    }
    aktPlus(2);
    return;
  }
}

void dcf77::err(byte b) {
  // 1 ,2 invalid sequence of duration
  msgF(F(" Err "), b);
  error = b;
}

byte dcf77::newState(byte news) {
  // 10: Sync received
  // sequences 1 9 or 2 8
  if (news == 10) {
    if (error == 0) {
      msgF(F("valid "), druP);
      minute = minuteX;
      hour = hourX;
      timestamp = millis();
    }
    state = 0;
    druP = 255; //byte inc to 0
    error = 0;
    aktW = 1;
    return 0;
  }
  switch (state) {
    case 0:
      break;
    case 1:
      if (news == 9) {
        druff(0);
      } else {
        err(1);
      }
      break;
    case 2:
      if (news == 8) {
        druff(1);
      } else {
        err(2);
      }
      break;
    case 8:
    case 9:
      break;
    default:
      msgF(F("news?"), news);
      break;
  }
  state = news;
  return news;
}

byte dcf77::doSample() {
  // returns  1,2,8,9
  // 10 sync
  //  0 spike
  // 101..109 error
  uint16_t zwi;
  zwi = ripu[routP] - last;
  last = ripu[routP];
  routP++;
  if (routP >= ripuL) {
    routP = 0;
  }
  if (zwi > sync ) {
#ifdef DEBUG
    dataP = 0;
    data[dataP] = zwi;
#endif
    return newState(10);
  }
#ifdef DEBUG
  dataP++;
  if (dataP >= dataL) {
    dataP = 0;
  }
  data[dataP] = zwi;
#endif
  if (zwi < 2) {
    return 0;
  }

  if (zwi < lo100) {
    msgF(F("zwi Lo"), zwi);
    return 101;
  }
  if (zwi < hi100) {
    return newState(1);
  }
  if (zwi < lo200) {
    return 102;
  }
  if (zwi < hi200) {
    return newState(2);
  }
  if (zwi < lo800) {
    return 108;
  }
  if (zwi < hi800) {
    return newState(8);
  }
  if (zwi < lo900) {
    return 109;
  }
  if (zwi < hi900) {
    return newState(9);
  }
  return 111;
}

void dcf77::info() {
  char str[100];
  sprintf(str, "\nErr %3u, druP %3u, state %3u, akt %3u, P %2u W %3u ", error, druP, state, akt, aktP, aktW);
  Serial.println(str);
}

void dcf77::act() {
  byte tmp;
  while (rinP != routP) {
    tmp = doSample();
    if (tmp > 10) {
      Serial.print("SampErr ");
      err(tmp);
      info();
    }
  }
} // sample

dcf77::dcf77() {
  pinMode(2, INPUT);
}

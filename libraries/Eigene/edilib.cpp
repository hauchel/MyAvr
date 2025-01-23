#include "edilib.h"
void Edi::esc(char t) {
  Serial.write(27);
  Serial.write(91);
  Serial.write(t);
}

void Edi::cursUp() {
  esc('A');
}

void Edi::cursDwn() {
  esc('B');
}

void Edi::cursRgt() {
  esc('C');
}

void Edi::cursRgtN(int n) {
  Serial.write(27);
  Serial.write(91);
  Serial.print(n);
  Serial.write('C');
}

void Edi::cursLft() {
  esc('D');
}

void Edi::cursPos1() {
  Serial.write(13);
}

void Edi::cursCls() {
  esc('H');
  esc('J');
}

void Edi::cursHome() {
  Serial.write(27);
  Serial.write(91);
  Serial.write(';');
  Serial.write('H');
}

void Edi::cursXY(int x , int y) {
  Serial.write(27);
  Serial.write(91);
  Serial.print(x);
  Serial.write(';');
  Serial.print(y);
  Serial.write('H');
}


void Edi::printLin() {
  int i = 0;
  cursPos1();
  while (lin[i] != 0) {
    Serial.write(lin[i]);
    i++;
  }
  xLen = i;
  esc('K');
}

void Edi::printLinRgt() {
  int i = xPos;
  esc('s');
  while (lin[i] != 0) {
    Serial.write(lin[i]);
    i++;
  }
  esc('K');
  esc('u');
}

void Edi::startEdi() {
  printLin();
  xPos = xLen;
  escSeq = 0;
}

void Edi::doLft() {
  if (xPos > 0) {
    xPos--;
    cursLft();
  }
}

void Edi::doBS(bool rea) {
  if (rea) {
    xPos--;
  }
  if (xPos > 0) {
    for (byte i = xPos; i < xLen; i++) {
      lin[i] = lin[i + 1];
    }
    xLen--;;
    if (rea) {
    cursLft();
    }
    printLinRgt();
  }
}

void Edi::doRgt() {
  if (xPos < xLen) {
    xPos++;
    cursRgt();
  }
}

void Edi::doPos1() {
  xPos = 0;
  cursPos1();
}

void Edi::doPos4() {
  doPos1();
  xPos = xLen;
  cursRgtN(xPos);
}

void Edi::doEinf() {
  xLen++;
  for (byte i = xLen; i > xPos; i--) {
    lin[i] = lin[i - 1];
  }
  printLinRgt();
}

char Edi::inp(char tmp) {
  // ESC sequence
  if (tmp == 27) {
    if (escSeq == 0) {
      escSeq = 1;
      return retPnd;
    }
    escSeq = 0;
  }
  if (escSeq == 1) {  // ESC
    switch (tmp) {
      case 91:          // ESC [
        escSeq = 2;
        return retPnd; //!
      case 'c': //
        cursCls();
        lin[0] = 0;
        break;
      case 'p': //
        startEdi();
        break;
      default:
        escSeq = 0;
        return retErr;
    }
    return retNOP;
  }
  if (escSeq == 2) {  // ESC [
    escSeq = 0;
    switch (tmp) {
      case 'D':
        doLft();
        break;
      case 'C':
        doRgt();
        break;
      case  '1':
        doPos1();
        break;
      case  '2':
        doEinf();
        break;
      case  '4':
        doPos4();
        break;
      default:
        return retErr;
    }
    return retNOP;
  }
  if (tmp == 8) {
    doBS(true);
    return retNOP;
  }
  if (tmp == 127) {
    doBS(false);
    return retNOP;
  }

  if (tmp < 27) {
    return tmp;
  }
  if (tmp > 125) {
    return tmp;
  }

  lin[xPos] = tmp;
  Serial.write(tmp);
  xPos++;
  if (xPos>xLen) {
    xLen++;
    lin[xPos+1]=0;
  }
  return retNOP;
}




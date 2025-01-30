
#include "disp832.h"

disp832::disp832() {
  // default pin assignment
  pinMode(ws2812_out, OUTPUT);
  dark();
  ledArray[0] = 25;  //"Alles im grünen Bereich"
  ledArray[1] = 0;
  ledArray[2] = 0;
}  // disp832

void disp832::begin(uint8_t cols) {
  _cols = min(cols, (uint8_t)80);
}

void disp832::setR(byte b) {
  pix[1] = b;
}
byte disp832::getR() {
  return pix[1];
}

void disp832::setG(byte b) {
  pix[0] = b;
}
byte disp832::getG() {
  return pix[0];
}

void disp832::setB(byte b) {
  pix[2] = b;
}
byte disp832::getB() {
  return pix[2];
}


void disp832::dark() {
  // all values zero
  for (int i = 0; i < ARRAYLEN; i++) {
    ledArray[i] = 0;
  }
  upd = true;
}

void disp832::fillArr() {
  byte j = 0;
  for (int i = 0; i < ARRAYLEN; i++) {
    ledArray[i] = pix[j];
    j++;
    if (j > 2) {
      j = 0;
    }
  }
  upd = true;
}

int disp832::arrPos(byte p) {
  //translate logical to phys position
  int sp = p % 16;
  switch (sp) {
    case 8: p = p + 7; break;
    case 9: p = p + 5; break;
    case 10: p = p + 3; break;
    case 11: p = p + 1; break;
    case 12: p = p - 1; break;
    case 13: p = p - 3; break;
    case 14: p = p - 5; break;
    case 15: p = p - 7; break;
  }
  //p = 255 - p; upside down
  int a = p * 3;
  if (a >= ARRAYLEN) {
    a = 0;
  }
  return a;
}

void disp832::pix2arr(byte p) {
  int a = arrPos(p);
  ledArray[a] = pix[0];
  ledArray[a + 1] = pix[1];
  ledArray[a + 2] = pix[2];
  upd = true;
}

void disp832::arr2pix(byte p) {
  int a = arrPos(p);
  pix[0] = ledArray[a];
  pix[1] = ledArray[a + 1];
  pix[2] = ledArray[a + 2];
}


void disp832::pix2color(int a) {
  if (a >= COLORM) {
    return;
  }
  a = a * 3;
  color[a] = pix[0];
  color[a + 1] = pix[1];
  color[a + 2] = pix[2];
}

void disp832::color2pix(byte a) {
  // array led a to  pix
  if (a >= COLORM) {
    return;
  }
  a = a * 3;
  pix[0] = color[a];
  pix[1] = color[a + 1];
  pix[2] = color[a + 2];
}

/* The write function is needed for derivation from the Print class. */
inline size_t disp832::write(uint8_t ch) {
  Serial.print(ch);  // assume success
  return 1;
}  // write()


bool disp832::refresh() {
  if (upd) {
    ws2812_sendarray(ledArray, ARRAYLEN);
    upd = false;
    return true;
  }
  return false;
}


void disp832::ws2812_sendarray(uint8_t *data, uint16_t datlen) {
  ws2812_sendarray_mask(data, datlen, _BV(ws2812_pin));
}

void disp832::ws2812_sendarray_mask(uint8_t *da2wd3ta, uint16_t datlen, uint8_t maskhi) {
  uint8_t curbyte, ctr, masklo;
  masklo = ~maskhi & ws2812_port;
  maskhi |= ws2812_port;

  noInterrupts();  // while sendig pixs
  while (datlen--) {
    curbyte = *da2wd3ta++;
    asm volatile(
      " ldi %0,8 \n\t"          // 0
      "loop%=:out %2, %3 \n\t"  // 1
      " lsl %1 \n\t"            // 2
      " dec %0 \n\t"            // 3

      " rjmp .+0 \n\t"  // 5

      " brcs .+2 \n\t"    // 6l / 7h
      " out %2, %4 \n\t"  // 7l / -

      " rjmp .+0 \n\t"  // 9

      " nop \n\t"         // 10
      " out %2, %4 \n\t"  // 11
      " breq end%= \n\t"  // 12 nt. 13 taken

      " rjmp .+0 \n\t"     // 14
      " rjmp .+0 \n\t"     // 16
      " rjmp .+0 \n\t"     // 18
      " rjmp loop%= \n\t"  // 20
      "end%=: \n\t"
      : "=&d"(ctr)
      : "r"(curbyte), "I"(_SFR_IO_ADDR(ws2812_port)), "r"(maskhi), "r"(masklo));

  }  // while
  interrupts();
}

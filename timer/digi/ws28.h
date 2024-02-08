/*
   Led Driver Source from
   https://github.com/cpldcpu/light_ws2812/blob/master/light_ws2812_AVR/light_ws2812.c
   Author: Tim (cpldcpu@gmail.com)
   Have to define before including:
*/

#define ARRAYLEN LEDNUM *3  // 
byte   ledArray[ARRAYLEN + 3]; // Buffer GRB plus one pixel
byte   pix[3];                // current pixel

bool   upd;                   // true refresh disp
byte led = 0;                 // last used LED
const byte colorM = 10;       // colors
//                      G R B
byte color[colorM * 3] = {0, 0, 0,  0, 100, 0,   100, 0, 0,   0, 0, 100};

void ws2812_sendarray_mask(uint8_t *, uint16_t , uint8_t);

void ws2812_sendarray(uint8_t *data, uint16_t datlen)
{
  ws2812_sendarray_mask(data, datlen, _BV(ws2812_pin));
}


void dark() {
  // all values zero
  for (int i = 0; i < ARRAYLEN; i++) {
    ledArray[i] = 0;
  }
  upd = true;
}

int arrPos( byte p) {
  int a = p * 3;
  if (a >= ARRAYLEN) {
    msgF(F("Err: arrpos Range"), p);
    a = 0;
  }
  return a;
}


void pix2arr(byte p) {
  int a = arrPos(p);
  ledArray[a] = pix[0];
  ledArray[a + 1] = pix[1];
  ledArray[a + 2] = pix[2];
  upd = true;
}

void arr2pix(byte p) {
  int a = arrPos(p);
  pix[0] = ledArray[a];
  pix[1] = ledArray[a + 1];
  pix[2] = ledArray[a + 2];
}

void pix2color(int a) {
  if (a >= colorM) {
    msgF(F("Err: pix2color Range"), a);
    return;
  }
  a = a * 3;
  color[a] = pix[0];
  color[a + 1] = pix[1];
  color[a + 2] = pix[2];
}

void color2pix(int a) {
  // array led a to  pix
  if (a >= colorM) {
    msgF(F("Err: color2pix Range"), a);
    return;
  }
  a = a * 3;
  pix[0] = color[a];
  pix[1] = color[a + 1];
  pix[2] = color[a + 2];
}

void refresh() {
  if (upd) {
    ws2812_sendarray(ledArray, ARRAYLEN);
    upd = false;
    if (verbo > 0)  Serial.print("*");
  }
}

void ws2812_sendarray_mask(uint8_t *da2wd3ta, uint16_t datlen, uint8_t maskhi)
{
  uint8_t curbyte, ctr, masklo;
  masklo   = ~maskhi & ws2812_port;
  maskhi |= ws2812_port;

  noInterrupts(); // while sendig pixs
  while (datlen--)
  {
    curbyte = *da2wd3ta++;
    asm volatile(
      " ldi %0,8 \n\t"   // 0
      "loop%=:out %2, %3 \n\t"   // 1
      " lsl %1 \n\t"   // 2
      " dec %0 \n\t"   // 3

      " rjmp .+0 \n\t"   // 5

      " brcs .+2 \n\t"   // 6l / 7h
      " out %2, %4 \n\t"   // 7l / -

      " rjmp .+0 \n\t"   // 9

      " nop \n\t"   // 10
      " out %2, %4 \n\t"   // 11
      " breq end%= \n\t"   // 12 nt. 13 taken

      " rjmp .+0 \n\t"   // 14
      " rjmp .+0 \n\t"   // 16
      " rjmp .+0 \n\t"   // 18
      " rjmp loop%= \n\t"   // 20
      "end%=: \n\t"
      :
      "=&d" (ctr)
      :
      "r" (curbyte), "I" (_SFR_IO_ADDR(ws2812_port)), "r" (maskhi), "r" (masklo)
    );

  } // while
  interrupts();
}

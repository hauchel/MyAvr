/* Show glediator formatted data:
  frame is   01 g r b g r b ...
  Read via serial, for 25 fps (= 40 ms/frame) and (24 LEDs * 3) * 10 < 20000 bit/sec , so 115200 is sufficient
  Transfer time to ws is 24*3*8*1.25 us = 0.72 ms, no ints allowed
  In jinx  use 72 channels then output fastpatch grb first channel 0 
*/

#define LEDNUM 24           // Number of LEDs in stripe
#define ws2812_port PORTB   // Data port register
#define ws2812_pin 2        // Number of the data out pin,
#define ws2812_out 10       // resulting Arduino num

#define ARRAYLEN LEDNUM *3  // 

uint8_t   ledArray[ARRAYLEN + 3]; // Buffer GRB plus one pixel
uint8_t   pix[3];             // current pixel
uint8_t   glePtr;                // Input
uint8_t   glePtrM;              // update after

void fillArr() {
  byte j = 0;
  pix[0] = 5;
  pix[1] = 0;
  pix[2] = 0;
  for (int i = 0; i < ARRAYLEN; i++) {
    ledArray[i] = pix[j];
    j++;
    if (j > 2) {
      j = 0;
    }
  }
}

void setup() {
  const char ich[] = "ws2812Gledi " __DATE__  " " __TIME__;
  Serial.begin(115200);
  Serial.println(ich);
  pinMode(ws2812_out, OUTPUT);
  fillArr();
  ws2812_sendarray(ledArray, ARRAYLEN);
  glePtrM = ARRAYLEN;
  glePtr=glePtrM+1;
}

void loop() {
  char b;
  // wait for 01, then fill
  // currTim = millis();

  if (Serial.available() > 0) {
    b = Serial.read();
    if (glePtr >= glePtrM) {
      if (b == 1) {
        glePtr = 0;
      }
    } else {
      ledArray[glePtr] = b;
      glePtr++;
      if (glePtr >= glePtrM) {
        ws2812_sendarray(ledArray, ARRAYLEN);
      }
    }
  } else { // nothing to do
    
  }

}

/*****************************************************************************************************************

   Led Driver Source from
   https://github.com/cpldcpu/light_ws2812/blob/master/light_ws2812_AVR/light_ws2812.c
   Author: Tim (cpldcpu@gmail.com)

 *****************************************************************************************************************/
void ws2812_sendarray_mask(uint8_t *, uint16_t , uint8_t);

void ws2812_sendarray(uint8_t *data, uint16_t datlen)
{
  ws2812_sendarray_mask(data, datlen, _BV(ws2812_pin));
}

void ws2812_sendarray_mask(uint8_t *da2wd3ta, uint16_t datlen, uint8_t maskhi)
{
  uint8_t curbyte, ctr, masklo;
  masklo   = ~maskhi & ws2812_port;
  maskhi |= ws2812_port;

  noInterrupts(); // while sendig pixels
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

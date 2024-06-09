
//calling: inp contains function requested, stack params
void doFuncs(byte was) {
  inp = inpPop();
  switch (was) {
    case 1:   // inp to cycle # leds up
      inp += 1;
      if (inp >= LEDNUM) {
        inp = 0;
      }
      break;
    case 2:   // inp to cycle # leds down
      inp -= 1;
      if (inp < 0) {
        inp = LEDNUM - 1;
      }
      break;
    case 3:   // inp to cycle # leds right
      inp -= 1;
      if (inp < 0) {
        inp = LEDNUM - 1;
      }
      break;
    case 4:   // inp to cycle # leds left
      inp -= 1;
      if (inp < 0) {
        inp = LEDNUM - 1;
      }
      break;
    default:
      errF(F("doFuncs"), inp);
  } //switch
}

void doRC5() {
  msgF(F("RC5 "), rc5_command);
  if (rc5_command <= 9) {
    doCmd(rc5_command + 48);
  } else {
    switch (rc5_command) {
      case 11:   // Store
        break;
      case 13:   // Mute
        break;
      case 43:   // >
        break;
      case 25:   // R
        doCmd('r');
        break;
      case 23:   // G
        doCmd('b');
        break;
      case 27:   // Y
        doCmd('g');
        break;
      case 36:   // B
        doCmd('b');
        break;
      case 44:   // Punkt
        doCmd('z');
        break;
      case 57:   // Ch+
        break;
      case 56:   // Ch+
        break;
      default:
        msgF(F("Which RC5?"), rc5_command);
    } // case
  } // else
}
/*
   Led Driver Source from
   https://github.com/cpldcpu/light_ws2812/blob/master/light_ws2812_AVR/light_ws2812.c
   Author: Tim (cpldcpu@gmail.com)
*/
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


byte basisR = 27;
byte yR =  13;
byte basisB = 203;
byte yB = 15;

void doFuncs(byte was) {  // called via eventque
  msgF(F("dofuncs"), was);
  switch (was) {
    case 1:   // Rot shot
      color2pix(1);
      inp = basisR;
      exec(yR, 1);
      break;
    case 2:   // Blau shot
      color2pix(2);
      inp = basisB;
      exec(yB, 1);
      break;
    case 3:   // free
      break;
    case 4:   // up
      break;
    case 5:   // draw
      break;
    case 6:   // down
      break;
    case 7:   // left
      break;
    case 8:   // right
      break;
    case 11:   // set led Red
      basisR = inpPop();
      break;
    case 12:   // set led Blue
      basisB = inpPop();
      break;
    case 13:   // set col
      yR = inpPop();
      break;
    case 14:   // set col
      yB = inpPop();
      break;

    case 31:   // key for left
      evqPut(1);
      break;
    case 47:   // key for down
      evqPut(6);
      break;
    case 55:   // key for right
      evqPut(2);
      break;
    case 59:   // key for rotL
      evqPut(1);
      break;
    case 61:   // key for rotR
      evqPut(2);
      break;
    case 62:   // key for up
      evqPut(4);
      break;
    case 63:   // all keys up
      break;
    default:
      errF(F("doFuncs"), was);
      Serial.print(F("Red  "));
      prnln33(basisR, yR);
      Serial.print(F("Blue "));
      prnln33(basisB, yB);
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

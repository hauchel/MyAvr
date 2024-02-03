const byte pgmbefM = 40;
const char s_00[] PROGMEM = "31x32x33x";   //start sequence in 0x
const char s_01[] PROGMEM = "";
const char s_02[] PROGMEM = "";
const char s_03[] PROGMEM = "";
const char s_04[] PROGMEM = "z";
const char s_05[] PROGMEM = "";
const char s_06[] PROGMEM = "";
const char s_07[] PROGMEM = "";
const char s_08[] PROGMEM = "8,10,5,1a13,11,5,2ai"; //assign agents
const char s_09[] PROGMEM = "3,1t4,2t5,3to";   
// subsequent use memo to set 2 colors, current posi is 1st on stack:
const char s_10[] PROGMEM = "0<c0+l,1<c0+,1d,l";  // 1 led in m[0/1] running forward
const char s_11[] PROGMEM = "2<c0+l,3<c0+,1d,l";  // 1 led in m[2/3] running forward
const char s_12[] PROGMEM = "4<c0+l,5<c0+,1d,l";  // 1 led in m[4/5] running forward
const char s_13[] PROGMEM = "0<c0+l,1<c0+,2d,l";   //1 led in m[0/1] running backward
const char s_14[] PROGMEM =  "2<c0+,1-,2d,l";   //1 led in col 3 running backward
const char s_15[] PROGMEM = "";
const char s_16[] PROGMEM = "";
const char s_17[] PROGMEM = "";
const char s_18[] PROGMEM = "";
const char s_19[] PROGMEM = "";
const char s_20[] PROGMEM = "";
const char s_21[] PROGMEM = "0+D#D#D#D";   //Balk_Push
const char s_22[] PROGMEM = "0+l#l#l#l";   //Balk
const char s_23[] PROGMEM = "0~4,23,0U~c,1+~,22yu";   //Balk6
const char s_24[] PROGMEM = "0+,~0+L9C4,23,4,~+UL,4-lu,9c4-l";   //Ring
const char s_25[] PROGMEM = "1,3,0TU,24yut0j";   //Balk_Rot
const char s_26[] PROGMEM = "";
const char s_27[] PROGMEM = "";
const char s_28[] PROGMEM = "";
const char s_29[] PROGMEM = "";
const char s_30[] PROGMEM = "0+TU,L0i0+lut";   //Var0
const char s_31[] PROGMEM = "0gb32r1C32g2C32b8C0rb32g4C32b6C0rg5C32gb0r3C3,9>";   //col_def
const char s_32[] PROGMEM = "1,9,0Uclu";   //col_show
const char s_33[] PROGMEM = "4,19,10U~30,5,4,0,4-,1,~,~3+,~U>uu";   //memo_set
const char s_34[] PROGMEM = "";   //Fade_up
const char s_35[] PROGMEM = "";   //Fade_up
const char s_36[] PROGMEM = "";   //Fade_up
const char s_37[] PROGMEM = "";   //Fade_up
const char s_38[] PROGMEM = "";   //Fade_up
const char s_39[] PROGMEM = "last!";   //Fade_up

const char *const pgmbefehle[pgmbefM] PROGMEM = {s_00,s_01,s_02,s_03,s_04,s_05,s_06,s_07,s_08,s_09,s_10,s_11,s_12,s_13,s_14,s_15,s_16,s_17,s_18,s_19,s_20,s_21,s_22,s_23,s_24,s_25,s_26,s_27,s_28,s_29,s_30,s_31,s_32,s_33,s_34,s_35,s_36,s_37,s_38,s_39};


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

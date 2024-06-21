const byte pgmbefM = 40;
const char s_00[] PROGMEM = "31x32x33x34x";   //start sequence in 0x
const char s_01[] PROGMEM = "8y,5y,12y,13y,21y,20y"; // Text1 pos,1x
const char s_02[] PROGMEM = "1c8y,2c5y,3c12y,4c13y,5c21y,6c20y";
const char s_03[] PROGMEM = "";
const char s_04[] PROGMEM = "40x41x42x43x";//call flash
const char s_05[] PROGMEM = "50x51x52x53x";
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
const char s_20[] PROGMEM = "z44,0y60,1y100,2y140,3y";//draw 0
const char s_21[] PROGMEM = "z44,4y68,5y100,6y116,7y";
const char s_22[] PROGMEM = "z44,8y68,9y100,10y140,11y";
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
const char s_34[] PROGMEM = "0,15-,10>15,11>0,15-,14>15,15>0,15-,18>15,19>";   //Fade params
const char s_35[] PROGMEM = "";   //Fade_up
const char s_36[] PROGMEM = "";   //Fade_up
const char s_37[] PROGMEM = "";   //Fade_up
const char s_38[] PROGMEM = "";   //Fade_up
const char s_39[] PROGMEM = "last!";   //Fade_up

const char *const pgmbefehle[pgmbefM] PROGMEM = {s_00, s_01, s_02, s_03, s_04, s_05, s_06, s_07, s_08, s_09, s_10, s_11, s_12, s_13, s_14, s_15, s_16, s_17, s_18, s_19, s_20, s_21, s_22, s_23, s_24, s_25, s_26, s_27, s_28, s_29, s_30, s_31, s_32, s_33, s_34, s_35, s_36, s_37, s_38, s_39};
#ifndef TETRIS
const byte charbefM = 40; // position inp to left bottom  then call.Thereafter  inp will be next position for letter
const char c_00[] PROGMEM = "";   //start sequence in 0x
const char c_01[] PROGMEM = "l#l#l#l#l,6+l##l,4+l#l#l#l#l,12+"; //A
const char c_02[] PROGMEM = "l#l#l#l#l8+l8+l";
const char c_03[] PROGMEM = "";
const char c_04[] PROGMEM = "z";
const char c_05[] PROGMEM = "l#l#l#l#l,4+l##l##l,4+l##l##l,12+"; //E
const char c_06[] PROGMEM = "";
const char c_07[] PROGMEM = "";
const char c_08[] PROGMEM = "l#l#l#l#l,6+l,8+l#l#l,5-#l#l,15+"; //H
const char c_09[] PROGMEM = "3,1t4,2t5,3to";
const char c_10[] PROGMEM = "0<c0+l,1<c0+,1d,l";  // 1 led in m[0/1] running forward
const char c_11[] PROGMEM = "2<c0+l,3<c0+,1d,l";  // 1 led in m[2/3] running forward
const char c_12[] PROGMEM = "l#l#l#l#l,4+l,8+l,16+";  // L
const char c_13[] PROGMEM = "l#l#l#l#l,7+l,5+l#l#l#l#l,12+";   //M
const char c_14[] PROGMEM =  "2<c0+,1-,2d,l";   //1 led in col 3 running backward
const char c_15[] PROGMEM = "";
const char c_16[] PROGMEM = "";
const char c_17[] PROGMEM = "";
const char c_18[] PROGMEM = "";
const char c_19[] PROGMEM = "";
const char c_20[] PROGMEM = ",8+l#l#l#l#l,8-l,16+l,12+"; //T
const char c_21[] PROGMEM = "l#l#l#l#l,4+l,8+l#l#l#l#l,12+";   //U
const char c_22[] PROGMEM = "0+l#l#l#l";   //Balk
const char c_23[] PROGMEM = "0~4,23,0U~c,1+~,22yu";   //Balk6
const char c_24[] PROGMEM = "0+,~0+L9C4,23,4,~+UL,4-lu,9c4-l";   //Ring
const char c_25[] PROGMEM = "1,3,0TU,24yut0j";   //Balk_Rot
const char c_26[] PROGMEM = "";
const char c_27[] PROGMEM = "";
const char c_28[] PROGMEM = "";
const char c_29[] PROGMEM = "";
const char c_30[] PROGMEM = "0+TU,L0i0+lut";   //Var0
const char c_31[] PROGMEM = "0gb32r1C32g2C32b8C0rb32g4C32b6C0rg5C32gb0r3C3,9>";   //col_def
const char c_32[] PROGMEM = "1,9,0Uclu";   //col_show
const char c_33[] PROGMEM = "4,19,10U~30,5,4,0,4-,1,~,~3+,~U>uu";   //memo_set
const char c_34[] PROGMEM = "";   //Fade_up
const char c_35[] PROGMEM = "";   //Fade_up
const char c_36[] PROGMEM = "";   //Fade_up
const char c_37[] PROGMEM = "";   //Fade_up
const char c_38[] PROGMEM = "";   //Fade_up
const char c_39[] PROGMEM = "last!";   //Fade_up

const char *const charbefehle[charbefM] PROGMEM = {c_00, c_01, c_02, c_03, c_04, c_05, c_06, c_07, c_08, c_09, c_10, c_11, c_12, c_13, c_14, c_15, c_16, c_17, c_18, c_19, c_20, c_21, c_22, c_23, c_24, c_25, c_26, c_27, c_28, c_29, c_30, c_31, c_32, c_33, c_34, c_35, c_36, c_37, c_38, c_39};
#endif
/*
   Led Driver Source from
   https://github.com/cpldcpu/light_ws2812/blob/master/light_ws2812_AVR/light_ws2812.c
   Author: Tim (cpldcpu@gmail.com)
   // Timing in ns       16 MHz(62.5 ns pro Takt)
   w_zeropulse   350     5.6    3
   w_onepulse    900    14.4    7
   w_totalperiod 1250     20   10

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

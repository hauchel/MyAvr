/* Sketch  to play with WS2812 LEDs
   by me, based on
   driver discussed and made by Tim in http://www.mikrocontroller.net/topic/292775
   addapted for Arduino Uno by chris 30.7.2013

  Commands read via serial, from and to EEprom:
   tick Command


*/
#include <edilib.h>
Edi edi;
bool ediMode = false;
#include <EEPROM.h>

#define LEDNUM 24           // Number of LEDs in stripe
#define ws2812_port PORTB   // Data port register, adapt setup() if changed 
#define ws2812_pin 2        // Number of the data out pin,
#define ws2812_out 10       // resulting Arduino num

#define ARRAYLEN LEDNUM *3  // 

uint8_t   ledArray[ARRAYLEN + 3]; // Buffer GRB plus one pixel
byte colM = 6; // 0 to 5
byte rowM = 4;
byte befP = 4;
byte   pix[3];             // current pixel
byte   ptmp[3];             // temp for shifts
uint8_t   intens[3];          // (max) intensity
uint8_t   curr = 0;           // current color selected (0,1,2)
uint8_t   progCnt;            // step in program counts down to 0
char      progMode;           // mode in program
bool      debug;              // true if
bool      eprom;              // true if reading from eprom
uint8_t   runMode;            // 0 normal 1 single step
char    bef[100];
bool      upd;                // true refresh disp
uint16_t inp;                 // numeric input
bool inpAkt = false;          // true if last input was a number
bool inpExt = false;          // true if last input was an extended
bool verb = false;            // verbose
byte led = 0;
unsigned long currTim;
unsigned long nexTim = 0;
unsigned long tick = 100;     //
int tickW = 0;               // wait ticks
const char *befehle[] = { "40rgb",
                          "20rg0b",
                          "0l4l8l12l16l20l",
                          "0u+u+u+u+u+u5t0j",
                          "This is string 5",
                          "This is string 6"
                        };
const byte befM = 6;
int befNum;
bool befAktiv = false;
const byte stackM = 6;
byte stackP = 0;
int stackBefP[stackM];
int stackBefNum[stackM];
int au, ao, ug, og;          // temps for shift
int eadr;

void msg(const char txt[], int n) {
  if (verb) {
    Serial.print(txt);
    Serial.print(" ");
    Serial.println(n);
  }
}


void readEprom(byte slot) {
  byte i, b;
  eadr = slot * 50;
  msg ("read Eprom", eadr);
  for (i = 0; i < 50; i++) {
    b = EEPROM.read(eadr) ;
    bef[i] = b;
    eadr++;
    if (b == 0) {
      return;
    }
  }
  msg ("read E ", i);
  bef[i] = 0;
}

void writeEprom(byte slot) {
  eadr = slot * 50;
  msg ("To Eprom", eadr);
  for (byte i = 0; i < 50; i++) {
    EEPROM.update(eadr, bef[i]) ;
    eadr++;
    if (bef[i] == 0) {
      return;
    }
  }
}

void prog1_init() {
  // when resetting prog1
  ledArray[0] = 25; //"Alles im grünen Bereich"
  ledArray[1] = 0;
  ledArray[2] = 0;
  intens[0] = 32;
  intens[1] = 32;
  intens[2] = 32;
  progCnt = 0;
  progMode = 'z';
  runMode = 255;
}

void doFlick() {
  // varies RGB by random +-2
  int z;
  uint8_t c;
  c = 0;
  for (int i = 0; i < ARRAYLEN; i++) {
    z = random(-2, +3) + int(ledArray[i]);
    if (z < 1) {
      z = 1;
    }
    if (z > intens[c]) {
      z = intens[c];
    }
    ledArray[i] = z;
    c++;
    if (c > 2) {
      c = 0;
    }
  }
}


void prog1_step() {
  // one step of prog1
  if (runMode == 0) {
    return;
  }
  if (runMode == 1) {
    runMode = 0;
  }
  if (progCnt > 0) {
    progCnt--;
  }
  // move up
  if ((progMode == 'c') or (progMode == 'f') or (progMode == 'z')) {
    // save top
    ledArray[ARRAYLEN]   = ledArray[ARRAYLEN - 3];
    ledArray[ARRAYLEN + 1] = ledArray[ARRAYLEN - 2];
    ledArray[ARRAYLEN + 2] = ledArray[ARRAYLEN - 1];
    for (int i = ARRAYLEN - 1; i > 2; i--) {
      ledArray[i] = ledArray[i - 3];
    }
  }

  // LED 0 depending on Mode
  switch (progMode) {
    case 'c':
      ledArray[0] = ledArray[ARRAYLEN - 3];
      ledArray[1] = ledArray[ARRAYLEN - 2];
      ledArray[2] = ledArray[ARRAYLEN - 1];
      break;
    case 'f':
      ledArray[0] = pix[0];
      ledArray[1] = pix[1];
      ledArray[2] = pix[2];
      break;
    case 'j':
      doFlick();
      break;
    case 'l':
      break;
    case 'z':
      ledArray[0] = random (intens[0]);
      ledArray[1] = random (intens[1]);
      ledArray[2] = random (intens[2]);
      break;
    default:;
  }
  ws2812_sendarray(ledArray, ARRAYLEN);
}

void dark() {
  // all values zero
  for (int i = 0; i < ARRAYLEN; i++) {
    ledArray[i] = 0;
  }
  ws2812_sendarray(ledArray, ARRAYLEN);
}

void setPix() {
  for (byte i = 0; i < 3; i++) {
    pix[i] = intens[i];
  }
  upd = true;
}

void pix2arr(int a) {
  // array led a to  pix
  a = a * 3;
  if (a >= ARRAYLEN) {
    msg("pix2arr Range", a);
    a = 0;
  }
  ledArray[a] = pix[0];
  ledArray[a + 1] = pix[1];
  ledArray[a + 2] = pix[2];
  upd = true;
}

void arr2tmp(int a) {
  // array led a to  tmp
  a = a * 3;
  if (a >= ARRAYLEN) {
    msg("arr2tmp Range", a);
    a = 0;
  }
  ptmp[0] = ledArray[a];
  ptmp[1] = ledArray[a + 1];
  ptmp[2] = ledArray[a + 2];
}

void tmp2arr(int a) {
  // array tmp to led a
  a = a * 3;
  if (a >= ARRAYLEN) {
    msg("tmp2arr2 Range", a);
    a = 0;
  }
  ledArray[a] = ptmp[0];
  ledArray[a + 1] = ptmp[1];
  ledArray[a + 2] = ptmp[2];
}

byte ledUp(byte in) {
  int tmp;
  tmp = in;
}

void calcau(byte col) {
  if (col >= colM) {
    msg("shift Range", col);
    col = 0;
  }
  au = col * rowM;
  ao = au + rowM - 1;
  og = (au + rowM) * 3 - 1;
  ug = (au + 1) * 3;
  /*
    char str[100];
    sprintf(str, "calcau au %4u ao %4u ug %4u og %4u ", au, ao, ug, og);
    Serial.println(str);
  */
}

void shiftUp(byte col) {
  calcau(col);
  arr2tmp(ao);
  for (byte i = og; i >= ug; i--) {
    ledArray[i] = ledArray[i - 3];
  }
  tmp2arr(au);
  upd = true;
}

void shiftDown(byte col) {
  calcau(col);
  arr2tmp(au);
  og = (au + rowM) * 3 - 1;
  ug = (au + 1) * 3;
  for (byte i = ug; i <= og; i++) {
    ledArray[i - 3] = ledArray[i];
  }
  tmp2arr(ao);
  upd = true;
}

void fillArr() {
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

void setIntens(uint8_t val) {
  intens[curr] = val;
  pix[curr] = val;
}

void setProgMode(char x) {
  progMode = x;
  if (runMode == 0) { //show result by stepping 1
    runMode = 1;
  }
}

void showEprom() {
  char adr;
  Serial.print(":");
  for (int i = 0; i < 20; i++) {
    adr = char(EEPROM.read(i));
    Serial.print(adr);
  }
  Serial.println();
}

void ser2Eprom() {
  char adr;
  int i = 0;
  Serial.print("Eprom:");
  adr = '?';
  while (byte(adr) != 13) {
    if (Serial.available() > 0) {
      adr = Serial.read();
      EEPROM.write(i, adr);
      Serial.print(adr);
      i++;
    }
  }
  EEPROM.write(i, adr); //terminating #13
  Serial.println();
  showEprom();
}

void evalEprom() {
  // interprets Eprom
  char adr;
  eprom = false;
  runMode = 10;
  for (int i = 0; i < 100; i++) { // Eprom Size?
    while (progCnt > 0) {
      Serial.print ("Wait\t");
      Serial.println(progCnt);
      prog1_step();
    }
    if (Serial.available() > 0) { //Keyboard
      return;
    }
    adr = EEPROM.read(i);
    Serial.print(i);
    Serial.print("\t");
    Serial.println(adr);
    adr = doCmd(adr);
    if (adr == '?') { // not understood
      return  ;
    }
  }
}


void help() {
  Serial.println(" nnn Number bs,+,-");
  Serial.println(" r g b set nnn to pix");
  Serial.println(" l select led nnn and set to pix");
  Serial.println(" f set all pix");
  Serial.println(" u,d  rotate up and down");
  Serial.println(" x bef nnn, y exec");
  Serial.println(" - previous  + next   v verbose  d dauer");
}

void info() {
  char str[100];
  sprintf(str, "PM %4u Led %3u pix R%3u G%3u B%3u  int R%3u G%3u B%3u", progMode, led, pix[1], pix[0], pix[2], intens[1], intens[0], intens[2]);
  Serial.println(str);
}


void copyBef(int num) {
  strcpy(bef, befehle[befNum]);
  Serial.println(bef);
}
void los(int num) {
  if (befAktiv) {
    push();
  }
  befNum = num;
  befAktiv = true;
  befP = 0;
}

void push() {
  stackBefP[stackP] = befP;
  stackBefNum[stackP] = befNum;
  stackP++;
  msg("Push ", stackP);
  if (stackP >= stackM) {
    befAktiv = false;
    msg("stack Overflow", 0);
  }
}

void pop() {
}

char doCmd(char tmp) {
  bool weg = false;

  if (ediMode) {
    tmp = edi.inp(tmp);
    switch (tmp) {
      case 13: //
        ediMode = false;
        strcpy (bef, edi.lin);
        msg ("Edi done:", 0);
        break;
      case 28:  // expect more
        break;
      case 29:
        msg(" edi retErr", 0);
        break;
    }
    return tmp;
  }

  // handle numbers
  if ( tmp == 8) { //backspace removes last digit
    weg = true;
    inp = inp / 10;
  } else  if (tmp == '+') {
    inp++;
    weg = true;
  } else  if (tmp == '-') {
    inp--;
    weg = true;
  }
  if ((tmp >= '0') && (tmp <= '9')) {
    weg = true;
    if (inpAkt) {
      inp = inp * 10 + (tmp - '0');
    } else {
      inpAkt = true;
      inp = tmp - '0';
    }
  }
  if (weg) {
    Serial.print("\b\b\b\b");
    Serial.print(inp);
    return tmp;
  }
  inpAkt = false;
  // handle combined copy shift ...

  switch (tmp) {
    case 'a':   // avanti!
      runMode = 255;
      break;
    case 'b':   // set blue (2)
      curr = 2;
      setIntens(inp);
      setPix();
      msg(" b ", inp);
      break;
    case 'c':   // cycle
      setProgMode(tmp);
      break;
    case 'd':   // set all 0
      shiftDown(inp);
      msg(" dwn ", inp);
      break;
    case 'D':   //
      debug = not debug;
      break;
    case 'e':   //
      strcpy (edi.lin, befehle[befNum]);
      edi.startEdi();
      ediMode = true;
      break;
    case 'E':   //
      ser2Eprom();
      break;
    case 'f':   // fill
      fillArr();
      break;
    case 'g':   // set green (0)
      curr = 0;
      setIntens(inp);
      setPix();
      msg(" g ", inp);
      break;
    case 'h':   // history
      break;
    case 'i':  // reset
      prog1_init();
      break;
    case 'j':   //
      if (befAktiv) {
        befP = inp;
        msg ("jump ", inp);
      }
      break;
    case 'l':   //
      led = inp;
      pix2arr(led);
      msg(" Led ", led);
      break;
    case 'm':   //
      readEprom(inp);
      break;
    case 'n':   //
      copyreadEprom(inp);
      break;
    case 'r':   // set red (1)
      curr = 1;
      setIntens(inp);
      setPix();
      msg(" r ", inp);
      break;
    case 's':   // step
      if (runMode > 0) {
        runMode = 0;
      }
      else {
        runMode = 1;
      }
      break;
    case 't':
      tickW = inp;
      break;
    case 'u':  //
      shiftUp(inp);
      msg(" up ", inp);
      break;
    case 'U':  //
      upd = true;
      msg(" upd ", upd);
      break;
    case 'v':  //
      if (verb) {
        msg(" silent ", verb);
        verb = false;
      } else {
        verb = true;
        msg(" verbose ", verb);
      }
      break;
    case 'w':  //
      writeEprom(inp);
      break;
    case 'x':  //
      los(inp);
      break;
    case 'y':  // stop
      befAktiv = false;
      befP = 0;
      break;
    case 'z':  //
      dark();
      msg(" zero ", 0);
      break;
    case ' ':
      break;
    default:
      info();
      return '?';
  } //switch
  return tmp ;
}

void setup() {
  const char ich[] = "ws2812Top " __DATE__  " " __TIME__;
  Serial.begin(38400);
  Serial.println(ich);
  pinMode(ws2812_out, OUTPUT);
  debug = false;
  eprom = false;
  ediMode = false;
  prog1_init();
}

void loop() {
  char adr;
  if (eprom) {
    evalEprom();
  }

  if (Serial.available() > 0) {
    adr = Serial.read();
    doCmd(adr);
  }

  if (befAktiv) {
    if (tickW == 0) {
      adr = bef[befP];
      if (adr == 0) {
        msg ("bef done ", befP);
        befAktiv = false;
      }  else {
        doCmd(adr);
        befP++;
      }
    } //tickW
  }

  currTim = millis();
  if (nexTim < currTim) {
    nexTim = currTim + tick;
    if (tickW > 0) {
      tickW--;
      msg("tickW ", tickW);
    }
    if (tickW == 0) {
      if (upd) {
        ws2812_sendarray(ledArray, ARRAYLEN);
        upd = false;
      }
    }
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

/*
    This routine writes an array of bytes with RGB values to the Dataout pin
  using the fast 800kHz clockless WS2811/2812 protocol.
  The order of the color-data is GRB 8:8:8. Serial data transmission begins
  with the most significant bit in each byte.
  The total length of each bit is 1.25µs (20 cycles @ 16Mhz)
   At 0µs the dataline is pulled high.
   To send a zero the dataline is pulled low after 0.375µs (6 cycles).
   To send a one the dataline is pulled low after 0.625µs (10 cycles).
  After the entire bitstream has been written, the dataout pin has to remain low
  for at least 50µs (reset condition).
  Due to the loop overhead there is a slight timing error: The loop will execute
  in 21 cycles for the last bit write. This does not cause any issues though,
  as only the timing between the rising and the falling edge seems to be critical.
  Some quick experiments have shown that the bitstream has to be delayed by
  more than 3µs until it cannot be continued (3µs=48 cyles).

*/
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

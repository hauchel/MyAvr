/* Sketch  to play with WS2812 LEDs
   based on driver discussed and made by Tim in http://www.mikrocontroller.net/topic/292775
   addapted for Arduino Uno by chris 30.7.2013

  Commands read via serial, from and to EEprom:
   tick Command


*/
#include <edilib.h>
Edi edi;
bool ediMode = false;
#include <EEPROM.h>
uint16_t eadr;
uint16_t eadrM = 1024;
const byte ekeyU = 255;
const byte ekeyE = 254;

#define LEDNUM 24           // Number of LEDs in stripe
#define ws2812_port PORTB   // Data port register
#define ws2812_pin 2        // Number of the data out pin,
#define ws2812_out 10       // resulting Arduino num

#define ARRAYLEN LEDNUM *3  // 

uint8_t   ledArray[ARRAYLEN + 3]; // Buffer GRB plus one pixel
byte colM = 6; // 0 to 5
byte rowM = 4;
int befP = 0;
byte   pix[3];                // current pixel
byte   ptmp[3];               // temp for shifts
uint8_t   intens[3];          // (max) intensity
uint8_t   curr = 0;           // current color selected (0,1,2)
uint8_t   progCnt;            // step in program counts down to 0
char      progMode;           // mode in program
bool      debug;              // true if
uint8_t   runMode;            // 0 normal, 1 single step, 2 ss triggered
char    bef[100];
bool      upd;                // true refresh disp
uint16_t inp;                 // numeric input
bool inpAkt = false;          // true if last input was a number
bool inpExt = false;          // true if last input was an extended
bool verb = false;            // verbose
byte led = 0;
unsigned long currTim;
unsigned long prevTim = 0;
unsigned long tick = 100;     //
int tickW = 0;               // wait ticks
const char *befehle[] = { "1X2X3X",
                          "20rg0b",
                          "0l1t4l1t8l1t12l16l20l",
                          "0u+u+u+u+u+u5t0j",
                          "This is string 5",
                          "This is string 6"
                        };
const byte befM = 6;
int befNum;
bool befAktiv = false;
const byte stackM = 6;
byte stackP = 0; // points to next free
int stackBefP[stackM];
int stackBefNum[stackM];
int au, ao, ug, og;          // temps for shift
uint8_t   glePtr;                // gledi Input
uint8_t   glePtrM;              // update after

void msg(const char txt[], int n) {
  if (verb) {
    Serial.print(txt);
    Serial.print(" ");
    Serial.println(n);
  }
}

void initEprom() {
  EEPROM.update(0, 0xFF) ;
}

bool searchEprom(byte key) {
  byte b, ofs;
  eadr = 0; // on return true to key, else to FF
  while (eadr < eadrM) {
    b = EEPROM.read(eadr) ;
    if (b == key) return true;
    if (b == ekeyU) return false;
    eadr++;
    ofs = EEPROM.read(eadr);
    eadr = eadr + ofs;
  }
}

void listEprom() {
  byte key, ofs, b;
  char str[100];
  uint16_t ea;
  eadr = 0;
  while (eadr < eadrM) {
    key = EEPROM.read(eadr) ;
    if (key == ekeyU) return;
    eadr++;
    ofs = EEPROM.read(eadr);
    sprintf(str, "key %4u ofs %4u  eadr %4u  ", key, ofs, eadr);
    Serial.print(str);
    ea = eadr + 1;
    eadr = eadr + ofs;
    if (key < ekeyE) {
      for (byte i = 0; i < 50; i++) {
        b = EEPROM.read(ea) ;
        str[i] = b;
        ea++;
        if (b == 0) break;
      } // show
      Serial.print(str);
    }
    Serial.println("<");
  } // while
}

void delEprom(byte key) {
  byte i, b;
  if (!searchEprom(key)) {
    msg ("Del not found", key);
    return;
  }
  EEPROM.write(eadr, ekeyE) ;
  msg ("Deleted ", key);
}

bool readEprom(byte key) {
  byte i, b;
  if (!searchEprom(key)) {
    msg ("Not found", key);
    return false;
  }
  eadr = eadr + 2;
  for (i = 0; i < 50; i++) {
    b = EEPROM.read(eadr) ;
    bef[i] = b;
    eadr++;
    if (b == 0) {
      return true;
    }
  }
  msg ("read E Err ", i);
  bef[i] = 0;
  return false;
}

void writeEprom(byte key) {
  char str[100];
  int len = strlen(bef);
  byte ofs;
  uint16_t finadr;
  if (searchEprom(key)) {
    msg ("Found", eadr);
    eadr++;
    ofs = EEPROM.read(eadr) - 2;
    sprintf(str, "Exist key %3u len %3u free %4u ", key, len, ofs );
    Serial.println(str);
    if (len > ofs) {
      msg("Del me first", len);
      return;
    }
  } else {
    ofs = len + 5;
    sprintf(str, "Add  key %3u eadr %4u len %3u ofs %4u ", key, eadr, len, ofs );
    Serial.println(str);
    EEPROM.update(eadr, key) ;
    eadr++;
    EEPROM.update(eadr, ofs) ;
    finadr = eadr + ofs;
    EEPROM.update(finadr, ekeyU);
  }
  eadr++; // points to ofs
  for (byte i = 0; i <= len; i++) {
    EEPROM.update(eadr, bef[i]) ;
    eadr++;
  }
}


void gledi() {
  glePtrM = ARRAYLEN;
  glePtr = 0; // called after 01;
  char b;
  unsigned long sendTim = 0;
  unsigned long nowTim = 0;
  // wait for 01, then fill, after full send
  sendTim = millis();
  while (true) {
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
          sendTim = millis();
        }
      }
    } else { // nothing to do
      nowTim = millis();
      if (nowTim - sendTim >= 200) {
        ledArray[0] = 25; //"Alles im grünen Bereich"
        ledArray[1] = 0;
        ledArray[2] = 0;
        ws2812_sendarray(ledArray, ARRAYLEN);
        return;
      }
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
  runMode = 0;
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
  sprintf(str, "RM %1u akt %1u befN %2u befP %2d Stp %2u Inp %4u Led %3u pix R%3u G%3u B%3u >", runMode, befAktiv, befNum, befP, stackP, inp, led, pix[1], pix[0], pix[2]);
  Serial.print(str);
  Serial.println(bef);
}


void copyBef(int num) {
  strcpy(bef, befehle[num]);
  Serial.println(bef);
}

void los(int num) {
  msg("Los ", num);
  if (num >= 0) {
    push();           // store  current
    if (!readEprom(num)) {
      befAktiv = false;
      return;
    }
    befP = -1;
  } else {
    stackP = 0;
    befP = 0;
  }
  befAktiv = true;
}

void showStack() {
  char str[100];

  for (int i = 0; i < stackM; i++) {
    if (i == stackP) {
      Serial.println("----");
    }
    sprintf(str, "%2u N %2u P %2u", i, stackBefNum[i], stackBefP[i]);
    Serial.println(str);
  }
}

void push() {
  msg("Push ", stackP);
  if (stackP >= stackM) {
    befAktiv = false;
    msg("stack Overflow", stackP);
  } else {
    stackBefP[stackP] = befP; // is inc'd on pop
    stackBefNum[stackP] = befNum;
    stackP++;
  }
}

void pop() {
  msg("Pop", stackP);
  if (stackP > 0) {
    stackP--;
    befP = stackBefP[stackP] + 1;
    readEprom(stackBefNum[stackP]);
  } else {
    befAktiv = false;
    msg("stack Underflow", 0);
  }
}


void statusLine() {
  char str[100];
  edi.esc('s');
  edi.cursHome();
  int f = 0;
  sprintf(str, "M %1u X %2u of %2u esc %1u Bef %2u >", ediMode, edi.xPos, edi.xLen, edi.escSeq, befNum);
  Serial.print(str);
  edi.esc('K');
  edi.esc('u');
}

char doCmd(char tmp) {
  bool weg = false;

  if (ediMode) {
    tmp = edi.inp(tmp);
    switch (tmp) {
      case 13: //
        ediMode = false;
        strcpy (bef, edi.lin);
        Serial.println();
        msg ("Edi done:", edi.xLen);
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
    // Serial.print("\b\b\b\b");
    // Serial.print(inp);
    return tmp;
  }
  if (tmp != 's') { // trace
    inpAkt = false;
  }
  // handle combined copy shift ...

  switch (tmp) {
    case 1:
      gledi();
    case 'a':   // avanti!
      runMode = 0;
      msg("runmode", runMode);
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
    case 'e':   //
      strcpy (edi.lin, bef);
      edi.startEdi();
      ediMode = true;
      befAktiv = false;
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
      break;
    case 'I':  // reset
      initEprom();
      break;
    case 'j':   //
      befP = inp;
      msg ("jump ", inp);
      befAktiv = true;
      break;
    case 'l':   //
      led = inp;
      pix2arr(led);
      msg(" Led ", led);
      break;
    case 'm':   //
      if (readEprom(inp)) {
        Serial.print(bef);
        Serial.println("<");
      } else {
        msg("Not found", inp);
      }
      break;
    case 'M':   //
      listEprom();
      break;
    case 'n':   //
      copyBef(inp);
      break;
    case 'p':
      pop();
      break;
    case 'r':   // set red (1)
      curr = 1;
      setIntens(inp);
      setPix();
      msg(" r ", inp);
      break;
    case 's':   // step
      if (runMode == 0) {
        runMode = 1;
      }  else if (runMode == 2) {
        runMode = 1;
      }
      break;
    case 'S':  //
      showStack();
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
    case 'W':   //
      delEprom(inp);
      break;
    case 'x':  // exec Bef
      los(-1);
      break;
    case 'X':  // gosub
      los(inp);
      break;
    case 'y':  // stop
      befAktiv = false;
      befP = 0;
      msg("Stop", 0);
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
  //Serial.begin(38400);
  Serial.begin(115200); // wg. gledi
  Serial.println(ich);
  pinMode(ws2812_out, OUTPUT);
  debug = false;
  ediMode = false;
  prog1_init();
}

void loop() {
  char adr;

  currTim = millis();

  if (Serial.available() > 0) {
    doCmd(Serial.read());
  }

  if (befAktiv) {
    if (tickW == 0) {
      adr = bef[befP];
      if (runMode != 2) {
        if (adr == 0) {
          msg ("bef done ", befP);
          inpAkt = false ; // just in case...
          if (stackP > 0) {
            pop();
          } else {
            befAktiv = false;
            info();
            Serial.println ("Finished ");
          }
        }  else {
          doCmd(adr);
          befP++;
          if (runMode == 1) {
            runMode = 2;
            info();
          }
        }
      } // runM
    } //tickW
  }
  if (currTim - prevTim >= tick) {
    prevTim = currTim;
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

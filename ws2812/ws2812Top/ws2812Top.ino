/* Sketch  to play with WS2812 LEDs
   based on driver discussed and made by Tim in http://www.mikrocontroller.net/topic/292775
   Commands read via serial, from and to EEprom:
   tick Command
*/
#include <EEPROM.h>

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
byte   apix[3];               // analog pixel

byte   ptmp[3];               // temp for shifts
uint8_t   intens[3];          // (max) intensity not used
uint8_t   curr = 0;           // current color selected (0,1,2)
uint8_t   progCnt;            // step in program counts down to 0
char      progMode;           // mode in program
bool      debug;              // true if
uint8_t   runMode;            // 0 normal, 1 single step, 2 ss triggered
char    bef[100];             // commands to be executed
bool      upd;                // true refresh disp

uint16_t inp;                 // numeric input
bool inpAkt = false;          // true if last input was a number
const byte inpSM = 5;          //
uint16_t inpStck[inpSM];      // Stack for inps
byte  inpSP = 0;              // Pointer
const byte memoM = 10;        // Memory for inps
uint16_t memo[memoM];         //


bool verb = false;            // verbose
byte led = 0;
unsigned long currTim;
unsigned long prevTim = 0;
unsigned long tick = 100;     //
int tickW = 0;               // wait ticks
uint16_t eadr;
uint16_t eadrM = 1024;
const byte ekeyU = 255;
const byte ekeyE = 254;

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
const byte stackM = 6;      // Stack for prog exec
byte stackP = 0;            // points to next free
int stackBefP[stackM];
int stackBefNum[stackM];
int au, ao, ug, og;          // temps for shift

void msg(const char txt[], int n) {
  if (verb) {
    Serial.print(txt);
    Serial.print(" ");
    Serial.println(n);
  }
}

void err(const char txt[], int n) {
  // stop execution
  befAktiv = false;
  Serial.print("Err: ");
  Serial.print(txt);
  Serial.print(" ");
  Serial.println(n);
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

void showEprom() {
  byte key, ofs, b;
  char str[100];
  uint16_t ea;
  eadr = 0;
  Serial.println(" EPROM:");
  while (eadr < eadrM) {
    key = EEPROM.read(eadr) ;
    if (key == ekeyU) return;
    eadr++;
    ofs = EEPROM.read(eadr);
    sprintf(str, "eadr %4u ofs %3u key %3u  >", eadr, ofs, key);
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
    err ("Del not found", key);
    return;
  }
  EEPROM.write(eadr, ekeyE) ;
  msg ("Deleted ", key);
}

bool readEprom(byte key) {
  byte i, b;
  if (!searchEprom(key)) {
    err ("Not found", key);
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
  err ("read E ", i);
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
      err("Del me first", len);
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
  pix[curr] = inp;
  upd = true;
}

void pix2arr(int a) {
  // array led a to  pix
  a = a * 3;
  if (a >= ARRAYLEN) {
    err("pix2arr Range", a);
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


bool analo() {
  int t;
  bool chg = false;
  t = analogRead(A0) / 4;
  if (apix[0] != t) {
    apix[0] = t;
    chg = true;
  }
  t = analogRead(A1) / 4;
  if (apix[1] != t) {
    apix[1] = t;
    chg = true;
  }
  t = analogRead(A2) / 4;
  if (apix[2] != t) {
    apix[2] = t;
    chg = true;
  }
  return chg;
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
  sprintf(str, "RM %1u akt %1u befN %2u befP %2d Stp %2u Inp %4u Led %3u pix R%3u G%3u B%3u ", runMode, befAktiv, befNum, befP, stackP, inp, led, pix[1], pix[0], pix[2]);
  Serial.print(str);
  showBef();
}

void showBef() {
  Serial.print(">");
  Serial.print(bef);
  Serial.println("<");
}

void seriBef() {
  // blocking read Bef
  byte i;
  char b;
  Serial.print("Bef :");
  for (i = 0; i < 50; i++) {
    while (Serial.available() == 0) {
    }
    b = Serial.read() ;
    Serial.print(b);
    if (b == 13) {
      bef[i] = 0;
      return;
    }
    bef[i] = b;
  } // loop
  err ("Seri ", i);
  bef[i] = 0;
}

void copyBef(int num) {
  strcpy(bef, befehle[num]);
}


void los(int num) {
  msg("Los ", num);
  if (num >= 0) {
    push();           // store  current
    if (!readEprom(num)) {
      err("not in Eprom", num);
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
  Serial.println(" Progstack:");
  for (int i = 0; i < stackM; i++) {
    if (i == stackP) {
      Serial.println("^^^^");
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
    err("stack Underflow", 0);
  }
}

void inpPush() {
  msg("inpPush ", inpSP);
  if (inpSP >= inpSM) {
    err("inp Overflow", inpSP);
  } else {
    inpStck[inpSP] = inp; // is inc'd on pop
    inpSP++;
  }
}

uint16_t inpPop() {
  msg("inpPop", inpSP);
  if (inpSP > 0) {
    inpSP --;
    return inpStck[inpSP];
  } else {
    err("inpstack Underflow", 0);
    return 0;
  }
}

void inp2Memo() {
  msg("inp2Memo ", inp);
  if (inp > memoM) {
    err("memo ", inp);
  } else {
    memo[inp] = inpPop();
  }
}

void memo2Inp() {
  msg("memo2Inp ", inp);
  if (inp > memoM) {
    err("memo ", inp);
  } else {
    inp = memo[inp];
  }
}

void inpShow() {
  char str[100];
  Serial.print(" inp=");
  Serial.println(inp);
  for (byte i = 0; i < inpSM; i++) {
    if (i == inpSP) {
      Serial.println("^^^^");
    }
    sprintf(str, "%2u  %5u ", i, inpStck[i]);
    Serial.println(str);
  }
  Serial.println("Memo:");
  for (byte i = 0; i < memoM; i++) {
    sprintf(str, "%2u  %5u ", i, memo[i]);
    Serial.println(str);
  }
}



char doCmd(char tmp) {
  bool weg = false;

  // handle numbers
  if ( tmp == 8) { //backspace removes last digit
    weg = true;
    inp = inp / 10;
  } else  if (tmp == '#') {
    inp++;
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
    case '?':   //
      inpShow();
      break;
    case ',':   //
      inpPush();
      break;
    case '>':   //
      inp2Memo();
      break;
    case '<':   //
      memo2Inp();
      break;
    case '+':   //
      inp = inp + inpPop();
      break;
    case '-':   //
      inp = inpPop() - inp;
      break;
    case 'a':   // avanti!
      runMode = 0;
      msg("runmode", runMode);
      break;
    case 'b':   // set blue (2)
      curr = 2;
      setPix();
      msg(" b ", inp);
      break;
    case 'c':   // cycle
      setProgMode(tmp);
      break;
    case 'd':   //
      shiftDown(inp);
      msg(" dwn ", inp);
      break;
    case 'e':   // edit bef
      seriBef();
      showBef();
      break;
    case 'f':   // fill
      fillArr();
      break;
    case 'g':   // set green (0)
      curr = 0;
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
    case 'L':   //
      inp = led;
      break;
    case 'm':   //
      if (readEprom(inp)) {
        showBef();
      } else {
        err("Not found", inp);
      }
      break;
    case 'M':   //
      showEprom();
      break;
    case 'n':   //
      copyBef(inp);
      showBef();
      break;
    case 'o':   //
      showBef();
      break;
    case 'p':
      pop();
      break;
    case 'r':   // set red (1)
      curr = 1;
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
    case 'X':  // exec Bef
      los(-1);
      break;
    case 'x':  // gosub
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
  Serial.begin(38400);
  Serial.println(ich);
  pinMode(ws2812_out, OUTPUT);
  debug = false;
  prog1_init();
}

void loop() {
  char c;
  currTim = millis();
  if (Serial.available() > 0) {
    c = Serial.read();
    Serial.print(c);
    doCmd(c);
  }

  if (befAktiv) {
    if (tickW == 0) {
      c = bef[befP];
      if (runMode != 2) {
        if (c == 0) {
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
          doCmd(c);
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

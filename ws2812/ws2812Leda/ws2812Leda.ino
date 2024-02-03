/* Sketch  to play with WS2812 LEDs
   based on driver discussed and made by Tim in http://www.mikrocontroller.net/topic/292775
   Commands read via serial, from and to EEprom.
*/

#define LEDNUM 80           // Number of LEDs in stripe
#define ws2812_port PORTB   // Data port register
#define ws2812_pin 2        // Number of the data out pin,
#define ws2812_out 10       // resulting Arduino num
#define ARRAYLEN LEDNUM *3  // 
byte   ledArray[ARRAYLEN + 3]; // Buffer GRB plus one pixel
#include "myBefs.h"
#include "rc5_tim2.h"
const byte rc5Pin = 2;        // TSOP purple
byte   pix[3];                // current pixel
byte   apix[3];               // analog pixel
byte   ptmp[3];               // temp for shifts
uint8_t   intens[3];          // (max) intensity not used
const byte colorM = 10;       // colors
byte color[colorM * 3];
bool debug;                   // true if
uint8_t   runMode;            // 0 not run, 1 single step, 2 ss after executing, 3 run
bool   upd;                   // true refresh disp
int    inp;                   // numeric input
int    inpsw;                 // swap-register
bool inpAkt = false;          // true if last input was a number
const byte inpSM = 20;        // Stack for inps
uint16_t inpStck[inpSM];      //
byte  inpSP = 0;              //
byte verb = 2;                // verbose 0 garnix 1 (.*) 2 debug 3 alles
bool echo = true ;            // echo back chars received
byte led = 0;                 // last used LED
unsigned long currTim;        // Time
unsigned long prevTim = 0;    //
unsigned long tickTim = 40;   // Tick size in ms 25/s =40ms
unsigned long startTim;       // set when starting execution
int tickW = 0;                // remaining wait ticks
const byte befM = 80;         // size bef
char    bef[befM];            // commands to be executed
int befP = 0;                 // next to execute
int befNum;                   // last read from eprom or pgm (negative)
const byte stackM = 5;        // Stack depth for prog exec
byte stackP = 0;              // points to next free
int stackBefP[stackM];        // saved befP
int stackBefNum[stackM];      // saved befNum
const byte memoM = 22;        // Memory 9.. special usage
int memo[memoM];              //
const byte pisM = 10;         // pix-Stack 0..2 und 3 for LED
byte pis[pisM][4];
byte pisP = 0;
const byte loopM = 4;         // max loop level
byte loopLev;
int loopCurr[loopM];
int loopInc[loopM];
int loopEnd[loopM];
byte loopBefP[loopM];

void msg(const char txt[], int n) {
  char str[30];
  if (verb > 1) {
    Serial.println();
    if (runMode != 0) {
      sprintf(str, "%3d %2d ", befNum, befP);
      Serial.print(str);
    }
    Serial.print(txt);
    Serial.print(" ");
    Serial.print(n);
    Serial.print(" ");
  }
}

void errF(const __FlashStringHelper *ifsh, int n) {
  runMode = 0;
  Serial.print(F("ErrF: "));
  PGM_P p = reinterpret_cast<PGM_P>(ifsh);
  while (1) {
    char c = pgm_read_byte(p++);
    if (c == 0) break;
    Serial.write(c);
  }
  Serial.write(" ");
  Serial.println(n);
}


#include "myEproms.h"

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
    errF(F("arrpos Range"), p);
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
    errF(F("pix2color Range"), a);
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
    errF(F("color2pix Range"), a);
    return;
  }
  a = a * 3;
  pix[0] = color[a];
  pix[1] = color[a + 1];
  pix[2] = color[a + 2];
}

void arr2tmp(byte p) {
  // array led a to  tmp
  int a = arrPos(p);
  ptmp[0] = ledArray[a];
  ptmp[1] = ledArray[a + 1];
  ptmp[2] = ledArray[a + 2];
}

void tmp2arr(byte p) {
  int a = arrPos(p);
  ledArray[a] = ptmp[0];
  ledArray[a + 1] = ptmp[1];
  ledArray[a + 2] = ptmp[2];
  upd = true;
}

void arr2memo(byte p) {
  // uses ptmp!
  arr2tmp(p);
  memo[0] = ptmp[0];
  memo[1] = ptmp[1];
  memo[2] = ptmp[2];
  memo[3] = p;
}

void memo2arr() {
  // uses ptmp!
  ptmp[0] = memo[0];
  ptmp[1] = memo[1];
  ptmp[2] = memo[2];
  tmp2arr(memo[3]);
}


void showPis() {
  char str[50];
  Serial.println(" pixStack:");
  for (int i = 0; i < pisM; i++) {
    if (i == pisP) {
      Serial.println(F("^^^^"));
    }
    sprintf(str, "%2u %3u %3u %3u %3u", i, pis[i][0], pis[i][1], pis[i][2], pis[i][3]);
    Serial.println(str);
  }
}

void randvalCol(byte col) {
  int ran, rano;
  char str[50];
  if (col > 2) {
    errF(F("randval"), col);
    return;
  }
  byte brmi = 10 + col * 4;
  byte brma = brmi + 1;
  byte bvmi = brmi + 2;
  byte bvma = brmi + 3;
  rano = random(memo[brmi], memo[brma]);
  ran =  rano + pix[col];
  if (ran < memo[bvmi]) ran = memo[bvmi];
  if (ran > memo[bvma]) ran = memo[bvma];
  pix[col] = ran;
  if (verb > 1) {
    sprintf(str, "col %1u %4d %4d %4d %4d %4d -> %3u", col, memo[brmi], memo[brma], memo[bvmi], memo[bvma], rano, pix[col]);
    Serial.println(str);
  }
}

void fadeArr() {

}

void pushPis(byte p) {
  msg("PushPis", p);
  if (pisP >= pisM) {
    errF(F("pis Overflow"), pisP);
  } else {
    int a = arrPos(p);
    pis[pisP][0] = ledArray[a];
    pis[pisP][1] = ledArray[a + 1];
    pis[pisP][2] = ledArray[a + 2];
    pis[pisP][3] = p;
    pisP++;
  }
}

void popPis() {

  upd = true;
  if (pisP > 0) {
    pisP--;
    msg("PopPis", pis[pisP][3]);
    int a = arrPos(pis[pisP][3]);
    ledArray[a] = pis[pisP][0];
    ledArray[a + 1] = pis[pisP][1];
    ledArray[a + 2] = pis[pisP][2];
  } else {
    errF(F("pis Underflow"), 0);
  }
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
  pix[0] = apix[0];
  pix[1] = apix[1];
  pix[2] = apix[2];
  return chg;
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

void showBef() {
  char bf;
  if (befP < 0) {
    bf = '?';
  } else {
    bf = bef[befP];
  }
  Serial.print("'");
  Serial.print(bf);
  Serial.print("' '");
  Serial.print(bef);
  Serial.println("'");
}

void info() {
  char str[100];
  sprintf(str, "RM %1u befN%4d befP %2d Stp %2u LL %2u Inp %5d Led %2u pix R%3u G%3u B%3u ", runMode, befNum, befP, stackP, loopLev, inp, led, pix[1], pix[0], pix[2]);
  Serial.print(str);
  showBef();
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
  errF (F("Seri "), i);
  bef[i] = 0;
}

bool pgm2Bef(byte num) {
  if (num >= pgmbefM) {
    errF(F("pgmbef "), num);
    return false;
  }
  //strcpy(bef, pgmbefehle[num]);
  strcpy_P(bef, (char *)pgm_read_word(&(pgmbefehle[num])));
  befNum = -num;
  return true;
}


void exec(int num) {
  msg("Exec", num);
  if (runMode > 0) {
    push();           // store  current
  }
  if (num > 0) {
    if (!readEprom(num)) {
      return;         // err already thrown
    }
  } else {
    if (!pgm2Bef(-num)) {
      return;         // err already thrown
    }
  }
  if (runMode == 0) {
    befP = 0;
    runMode = 3;
    startTim = millis();
  } else {
    befP = -1;
  }
}


void los() {
  //pgm in bef
  msg("Los", 0);
  loopLev = 0;
  stackP = 0;
  if (runMode == 0) {
    befP = 0;
    runMode = 3;
    startTim = millis();
  } else {
    befP = -1;
  }
}

void showStack() {
  char str[100];
  Serial.println(" Progstack:");
  for (int i = 0; i < stackM; i++) {
    if (i == stackP) {
      Serial.println("^^^^");
    }
    sprintf(str, "%2u N %4d P %2u", i, stackBefNum[i], stackBefP[i]);
    Serial.println(str);
  }
}

void push() {
  msg("prgPush", stackP);
  if (stackP >= stackM) {
    errF(F("Stack Overflow"), stackP);
  } else {
    stackBefP[stackP] = befP; // is inc'd on pop
    stackBefNum[stackP] = befNum;
    stackP++;
  }
}

void pop() {
  msg("prgPop", stackP);
  if (stackP > 0) {
    stackP--;
    befP = stackBefP[stackP] + 1;
    if (stackBefNum[stackP] <= 0) {
      pgm2Bef(-stackBefNum[stackP]);
    } else {
      readEprom(stackBefNum[stackP]);
    }
  } else {
    errF(F("prg stack Underflow"), 0);
  }
}

void inpPush() {
  msg("inpPush", inp);
  if (inpSP >= inpSM) {
    errF(F("inp Overflow"), inpSP);
  } else {
    inpStck[inpSP] = inp; // is inc'd on pop
    inpSP++;
  }
}

uint16_t inpPop() {
  if (inpSP > 0) {
    inpSP --;
    msg("inpPop", inpStck[inpSP]);
    return inpStck[inpSP];
  } else {
    errF(F("inpstack Underflow"), 0);
    return 0;
  }
}

void inp2Memo() {
  msg("inp2Memo ", inp);
  if (inp > memoM) {
    errF(F("memo "), inp);
  } else {
    memo[inp] = inpPop();
  }
}

void memo2Inp() {
  msg("memo2Inp ", inp);
  if (inp > memoM) {
    errF(F("memo "), inp);
  } else {
    inp = memo[inp];
  }
}

void showInp() {
  char str[100];
  Serial.print(" inp=");
  Serial.println(inp);
  for (byte i = 0; i < inpSM; i++) {
    if (i == inpSP) {
      Serial.println("^^^^");
    }
    sprintf(str, "%2u %6d", i, inpStck[i]);
    Serial.println(str);
  }
  Serial.println("Memo:");
  for (byte i = 0; i < memoM; i++) {
    if (memo[i] != 0) {
      sprintf(str, "%2u  %5d ", i, memo[i]);
      Serial.println(str);
    }
  }
}

void showLoop() {
  char str[100];
  Serial.print("Loops: ");
  Serial.println(loopLev);
  for (int i = 1; i < loopM; i++) {
    sprintf(str, "%2u Cur %4d Inc %4d End  %4d  Bef %3u", i, loopCurr[i],  loopInc[i],  loopEnd[i], loopBefP[i]);
    Serial.println(str);
  }
}

void startLoop() {
  // end,dlt,strtO
  loopLev++;
  if (loopLev >= loopM) {
    errF (F("Loop Depth"), loopLev);
    return;
  }
  loopEnd[loopLev] = inpPop();
  loopInc[loopLev] = inpPop();
  loopCurr[loopLev] = inp;
  loopBefP[loopLev] = befP;
  msg("For with", inp);
}

void nextLoop() {
  bool agn = false;
  if (loopLev == 0) {
    errF (F("Loop Level"), loopLev);
    return;
  }
  loopCurr[loopLev] += loopInc[loopLev];
  if (loopInc[loopLev] > 0) {
    if (loopCurr[loopLev] <= loopEnd[loopLev]) agn = true;
  } else { //negativ
    if (loopCurr[loopLev] >= loopEnd[loopLev]) agn = true;
  }
  if (agn) {
    befP = loopBefP[loopLev];
    inp = loopCurr[loopLev];
    msg("Loop", inp);
  } else {
    inp = loopCurr[loopLev];
    loopLev--;
    msg("Loop End", inp);
  }
}


void refresh() {
  if (upd) {
    ws2812_sendarray(ledArray, ARRAYLEN);
    upd = false;
    if (verb > 0)  Serial.print("*");
  }
}


void doRC5() {
  msg("RC5 ", rc5_command);
  if ((rc5_command >= 0) && (rc5_command <= 9)) {
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
        msg("Which RC5?", rc5_command);
    } // case
  } // else
}

char doCmd(char tmp) {
  bool weg = false;
  int zwi;
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
  switch (tmp) {
    case '?':   //
      showInp();
      showLoop();
      showPis();
      break;
    case ',':   //
      inpPush();
      break;
    case '>':   //
      inp2Memo();
      break;
    case '~':   //
      zwi = inp;
      inp = inpsw;
      inpsw = zwi;
      msg("Swap", inp);
      break;
    case '<':   //
      memo2Inp();
      break;
    case '+':   //
      inp =  inpPop() + inp;
      break;
    case '-':   //
      inp = inpPop() - inp;
      break;
    case 'a':   //
      analo();
      break;
    case 'b':   // set blue (2)
      pix[2] = inp;
      msg("b", inp);
      break;
    case 'B':   //
      inp = pix[2];
      break;
    case 'c':   //
      color2pix(inp);
      break;
    case 'C':   //
      pix2color(inp);
      break;
    case 'D':   //
      pushPis(inp);
      break;
    case 'd':   //
      popPis();
      break;
    case 'e':   // edit bef
      seriBef();
      showBef();
      break;
    case 'f':   // fill
      fillArr();
    case 'F':   // fill
      fadeArr();
      break;
    case 'g':   // set green (0)
      pix[0] = inp;
      msg("g", inp);
      break;
    case 'G':   //
      inp = pix[0];
      break;
    case 'h':   // halt
      break;
    case 'i':  // intensity change
      randvalCol(inp);
      break;
    case 'I':  // reset
      initEprom();
      break;
    case 'j':   //
      befP = inp;
      if (runMode > 0) { //already running
        befP--; // as inced below
      } else { //
        runMode = 3;
      }
      msg ("jump ", befP);
      break;
    case 'k':   //
      runMode = 3;
      msg("runmode", runMode);
      break;
    case 'K':   //
      msg("Comb", combineEprom()) ;
      break;
    case 'l':   //
      led = inp;
      pix2arr(led);
      msg("Leds", led);
      break;
    case 'L':   //
      msg("LedL", inp);
      arr2pix(inp);
      break;
    case 'm':   //
      if (readEprom(inp)) {
        showBef();
      } else {
        errF(F("Key not found"), inp);
      }
      break;
    case 'M':   //
      showEprom();
      break;
    case 'n':   //
      pgm2Bef(inp);
      showBef();
      break;
    case 'p':  // pause
      break;
    case 'q':  // quit
      runMode = 0;
      Serial.println(F("Stop"));
      break;
    case 'Q':  // quit
      runMode = 0;
      loopLev = 0;
      befP = 0;
      stackP = 0;
      inpSP = 0;
      Serial.println(F("Reset"));
      break;
    case 'r':   // set red (1)
      pix[1] = inp;
      msg("r", inp);
      break;
    case 'R':   //
      inp = pix[1];
      break;
    case 's':   // step
      if (runMode == 0) {
        los();
      }
      runMode = 1;
      break;
    case 'S':  //
      showStack();
      break;
    case 't':    // tick sync see below
      break;
    case 'T':
      tickW = memo[9];
      refresh(); // last update before wait
      break;
    case 'u':   //
      nextLoop();
      break;
    case 'U':   //
      startLoop();
      break;
    case 'v':  //
      switch (verb) {
        case 0:
          verb = 1;
          Serial.println("eins");
          break;
        case 1:
          Serial.println("zwei");
          verb = 2;
          break;
        default:
          verb = 0;
          Serial.println("nix");
      }
      break;
    case 'V':  //
      echo = !echo;
      if (echo)  Serial.println(F("echo")); else Serial.println(F("still"));
      break;
    case 'w':  //
      writeEprom(inp);
      break;
    case 'W':   //
      delEprom(inp);
      break;
    case 'x':  // gosub
      exec(inp);
      break;
    case 'X':  // exec Bef
      los();
      break;
    case 'y':  // stop
      exec(-inp);
      break;
    case 'z':  //
      dark();
      msg(" zero ", 0);
      break;
    case ' ':
      break;
    case 13:
      info();
      break;
    default:
      Serial.print ("?");
      Serial.println (byte(tmp));
      return '?';
  } //switch
  return tmp ;
}

void setup() {
  const char ich[] = "ws2812Leda " __DATE__  " " __TIME__;
  Serial.begin(38400);
  Serial.println(ich);
  pinMode(ws2812_out, OUTPUT);
  RC5_init();
  debug = false;
  runMode = 0;
  dark();
  ledArray[0] = 25; //"Alles im grünen Bereich"
  ledArray[1] = 0;
  ledArray[2] = 0;
}

void loop() {
  char c;
  currTim = millis();
  if (Serial.available() > 0) {
    c = Serial.read();
    if (echo)  Serial.print(c);
    doCmd(c);
  }
  if (rc5_ok) {
    RC5_again();
    doRC5();
  }

  if (runMode > 0) { //executing
    c = bef[befP];
    if (runMode != 2) {
      if (c == 0) {
        msg ("bef done ", befP);
        inpAkt = false ; // just in case...
        if (stackP > 0) {
          pop();
        } else {
          runMode = 0;
          refresh();  //last upd should be included in time
          startTim = millis() - startTim;
          Serial.print (F("  Finished Exec ms="));
          Serial.println (startTim);
        }
      }  else { // valid cmd
        doCmd(c);
        if ((tickW == 0) || (c != 't')) { // wait for sync
          befP++;
        }
        if (runMode == 1) { //single step executed
          runMode = 2;
          Serial.println();
          info();
        }
      }
    } // runM
  }

  if (currTim - prevTim >= tickTim) {
    prevTim = currTim;
    if (tickW > 0) {
      tickW--;
      if (verb > 0) Serial.print(".");
    }
    if (tickW == 0) {
      refresh();
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

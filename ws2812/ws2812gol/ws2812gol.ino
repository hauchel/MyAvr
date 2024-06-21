/* Sketch  to play with Game of Life WS2812 LEDs
  based on driver discussed and made by Tim in http://www.mikrocontroller.net/topic/292775
  Commands read via serial, from and to Flash using optiboot.
*/

#define LEDNUM 256          // Number of LEDs in stripe
#define ws2812_port PORTB   // Data port register
#define ws2812_pin 1        // Number of the data out pin there ,
#define ws2812_out 9       // resulting Arduino num
#include "optiboot.h"
// allocate flash to write to, must be initialized, one more than used as 0 for EOS
#define NUMBER_OF_PAGES 10
const uint8_t flashSpace[SPM_PAGESIZE * (NUMBER_OF_PAGES + 1)] __attribute__ (( aligned(SPM_PAGESIZE) )) PROGMEM = {"\x0"};
#define ARRAYLEN LEDNUM *3  // 
byte   ledArray[ARRAYLEN]; // Buffer GRB

const byte rc5Pin = 2;        // TSOP purple
byte   pix[3];                // current pixel
const byte colorM = 10;       // colors conf
byte color[colorM * 3];
bool debug;                   // true if
byte  runMode;                // 0 not run, 1 single step, 2 ss after exe, 3 run
bool   upd;                   // true refresh disp
int    inp;                   // numeric input
int    inpsw;                 // swap-register
bool inpAkt = false;          // true if last input was a number
const byte inpSM = 7;         // Stack for inps conf
int inpStck[inpSM];           //
byte  inpSP = 0;              //
byte verb = 0;                // verbose 0 garnix 1 (.*) 2 debug 3 alles
bool echo = true ;            // echo back chars received
unsigned long currTim;        // Time
unsigned long prevTim = 0;    //
unsigned long tickTim = 20;   // in ms
unsigned long startTim;       // set when starting exe
const byte befM = SPM_PAGESIZE;        // size bef
typedef union {
  char  bef[befM];
  uint8_t ramBuffer[SPM_PAGESIZE];
} chunk_t ;
chunk_t chunk;

int befP;                   // next to execute
byte befNum;                // current bef: 0..39 and 40 ..
const byte stackM = 5;      // Stack depth for prog exec
byte stackP;                // points to next free
int stackBefP[stackM];      // saved befP
byte stackBefNum[stackM];   // saved befNum

const byte chngM = 127;      // size;
byte chng[chngM + 1];         // fields to change
byte hinP;                 // to be added from 0 up
byte wegP;                // to be cleared from top down

byte butt;             // Buttons on PortC
byte buttold = 63;
byte buttcnt = 0;

const byte evqM = 5;  // event-que
byte evq[evqM];
byte evqIn = 0;
byte evqOut = 0;

byte tpr = 5;         // ticks per run set by A
byte tic = 0;
bool simu = false;    // set by a
byte maxpix = 39;     // max sum of pix in birth;

#include "golNab.h"
#include "myBefs.h"
#include "rc5_tim2.h"

void prnt(PGM_P p) {
  // output char * from flash,
  while (1) {
    char c = pgm_read_byte(p++);
    if (c == 0) break;
    Serial.write(c);
  }
  Serial.write(" ");
}

void msgF(const __FlashStringHelper *ifsh, uint16_t n) {
  if (verb > 1) {
    PGM_P p = reinterpret_cast<PGM_P>(ifsh);
    prnt(p);
    Serial.println(n);
  }
}

void debF(const __FlashStringHelper *ifsh, int n, byte lev) {
  if (verb >= lev) {
    PGM_P p = reinterpret_cast<PGM_P>(ifsh);
    prnt(p);
    Serial.println(n);
  }
}

void errF(const __FlashStringHelper *ifsh, int n) {
  runMode = 0;
  Serial.print(F("ErrF: "));
  PGM_P p = reinterpret_cast<PGM_P>(ifsh);
  prnt(p);
  Serial.println(n);
}

void prn3u(uint16_t a) {
  char str[10];
  sprintf(str, "%3u ", a);
  Serial.print(str);
}

void prnln33(uint16_t a, uint16_t b) {
  prn3u(a);
  prn3u(b);
  Serial.println();
}

byte evqPut(byte was) {
  msgF(F("evqPut"), was);
  evq[evqIn] = was;
  evqIn++;
  if (evqIn >= evqM) evqIn = 0;
  if (evqIn == evqOut) {
    errF(F("evq Overflow"), evqIn);
    return 1;
  }
  return 0;
}

byte evqGet() {
  byte tmp;
  if (evqIn == evqOut) {
    return 0;
  }
  tmp = evq[evqOut];
  evqOut++;
  if (evqOut >= evqM) evqOut = 0;
  return tmp;
}

bool readPage(byte page) {
  page = 1 + page - pgmbefM; // page is translated -39 so eg 40 reads 1
  msgF(F("readPage"), page);
  if (page <= NUMBER_OF_PAGES) {
    optiboot_readPage(flashSpace, chunk.ramBuffer, page);
    return true;
  } else {
    errF(F("Page max "), NUMBER_OF_PAGES);
    return false;
  }
}

void writePage(byte page) {
  page = 1 + page - pgmbefM;
  msgF(F("writePage"), page);
  if (page <= NUMBER_OF_PAGES) {
    optiboot_writePage(flashSpace, chunk.ramBuffer, page);
  } else {
    errF(F("Page max "), NUMBER_OF_PAGES);
  }
}


void dark() {
  // all values zero
  for (int i = 0; i < ARRAYLEN; i++) {
    ledArray[i] = 0;
  }
  upd = true;
}

int arrPos( byte p) {
  //translate logical to phys position (G in array)
  int sp = p % 16;
  switch (sp) {
    case 8:  p += 7; break;
    case 9:  p += 5; break;
    case 10: p += 3; break;
    case 11: p += 1; break;
    case 12: p -= 1; break;
    case 13: p -= 3; break;
    case 14: p -= 5; break;
    case 15: p -= 7; break;
  }
  //p = 255 - p; upside down
  int a = p * 3;
  if (a >= ARRAYLEN) {
    errF(F("arrPos Range"), p);
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

void color2pix(byte a) {
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

bool pixcmp(byte p) {
  // returns tru if pix==led)
  int a = arrPos(p);
  if (ledArray[a] != pix[0]) return false;
  if (ledArray[a + 1] != pix[1]) return false;
  if (ledArray[a + 2] != pix[2]) return false;
  return true;
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

void info() {
  char str[50];
  sprintf(str, "RM %1u Inp %5d pix R%3u G%3u B%3u ", runMode, inp, pix[1], pix[0], pix[2]);
  Serial.print(str);
  showBef();
}

void wasPush(int was) {
  msgF(F("wasPush"), was);
  if (inpSP >= inpSM) {
    errF(F("inp Overflow"), inpSP);
  } else {
    inpStck[inpSP] = was; // is inc'd on pop
    inpSP++;
  }
}
void inpPush() {
  wasPush(inp);
}

int inpPop() {
  if (inpSP > 0) {
    inpSP --;
    msgF(F("inpPop"), inpStck[inpSP]);
    return inpStck[inpSP];
  } else {
    errF(F("inpstack Underflow"), 0);
    return 0;
  }
}

void showInp() {
  char dd[4];
  const static char dj[] PROGMEM = "-> ";
  const static char dn[] PROGMEM = "   ";
  Serial.print(F(" inp="));
  Serial.print(inp);
  Serial.print(F("inpsp"));
  Serial.println(inpSP);
  for (int i = 0; i < inpSM; i++) {
    if (i == inpSP) {
      strcpy_P(dd, dj );
    } else {
      strcpy_P(dd, dn );
    }
    Serial.print(dd);
    prnln33(i, inpStck[i]);
  }
}

void prgPush() {
  msgF(F("prgPush"), befNum);
  if (stackP >= stackM) {
    errF(F("Stack Overflow"), stackP);
  } else {
    stackBefP[stackP] = befP; // is inc'd on pop
    stackBefNum[stackP] = befNum;
    stackP++;
  }
}

void prgPop() {
  if (stackP > 0) {
    stackP--;
    befP = stackBefP[stackP] + 1;
    befNum = stackBefNum[stackP] ;
    if (befNum >= pgmbefM) {
      readPage(befNum);
    } else {
      pgm2Bef(befNum);
    }
  } else {
    errF(F("prg stack Underflow"), 0);
  }
}

void showBef() {
  char bf;
  if (befP < 0) {
    bf = '?';
  } else {
    bf = chunk.bef[befP];
  }
  Serial.print("'");
  Serial.print(bf);
  Serial.print("' '");
  Serial.print(chunk.bef);
  Serial.println("'");
}

byte seriBef() {
  // blocking read Bef, returns 0 if terminated by CR, 1 by "
  byte i;
  char b;
  Serial.print(F("Bef :"));
  for (i = 0; i < 50; i++) {
    while (Serial.available() == 0) {
    }
    b = Serial.read() ;
    Serial.print(b);
    if (b == 13) {
      chunk.bef[i] = 0;
      return 0;
    }
    if (b == '"') {
      chunk.bef[i] = 0;
      return 1;
    }
    chunk.bef[i] = b;
  } // loop
  errF (F("Seri "), i);
  chunk.bef[i] = 0;
  return 9;
}

void showTxt(byte num) {
  // reads and prints
  if (num >= chartxtM) {
    errF(F("chartxt "), num);
  } else {
    prnt((char *)pgm_read_word(&(chartxt[num])));
    Serial.println();
  }
}

void showTxtAll() {
  // reads and prints
  Serial.println(F("gols:"));
  for (int i = 0; i < chartxtM; i++) {
    prn3u(i);
    showTxt(i);
  }
}

bool char2Bef(byte num) {
  if (num >= charbefM) {
    errF(F("charbef "), num);
    return false;
  }
  strcpy_P(chunk.bef, (char *)pgm_read_word(&(charbefehle[num])));
  befNum = num + 200; //we can not call others from chars
  return true;
}

bool pgm2Bef(byte num) {
  if (num >= pgmbefM) {
    errF(F("pgm2Bef "), num);
    return false;
  }
  strcpy_P(chunk.bef, (char *)pgm_read_word(&(pgmbefehle[num])));
  befNum = num;
  return true;
}

void exec(byte num, byte was) {
  // was 0 get pgm and execute, was 1 get char
  startTim = micros();
  if (runMode > 0) {
    prgPush();           // store  current
  }

  if (was == 0) {
    msgF(F("ExeFl"), num);
    if (num >= pgmbefM) {
      if (!readPage(num)) {
        return;         // err already thrown
      }
      befNum = num;
    } else {
      if (!pgm2Bef(num)) {
        return;         // err already thrown
      }
    }
  } else {
    msgF(F("ExeChar"), num);
    if (!char2Bef(num)) {
      return;         // err already thrown
    }
  }
  if (runMode == 0) {
    befP = 0;
    runMode = 3;
  } else {
    befP = -1;
  }
}

byte bef2end() {
  // position befp just before trailing #0 to return
  byte i;
  i = befP;
  while (i < 128) {
    if (chunk.bef[i] == 0) return i - 1;
    i++;
  }
  errF(F("bef2end no End??"), befP);
  return 0;
}

#include "myFuncs.h"

void refresh() {
  if (upd) {
    ws2812_sendarray(ledArray, ARRAYLEN);
    upd = false;
    if (verb > 0)  Serial.print("*");
  }
}

void setNab(byte p) {
  // set to pix0 uint16_t nab[p][8] (debug only)
  uint16_t a;
  for (int i = 0; i < 8; i++) {
    a = pgm_read_word(&nab[p][i]);
    msgF(F("pix"), a);
    if (a < 999) ledArray[a] = pix[0];
  }
  upd = true;
}

uint16_t sumNab(byte p) {
  // return sum of G of  nab[p][8]
  uint16_t a;
  uint16_t su = 0;
  for (int i = 0; i < 8; i++) {
    a = pgm_read_word(&nab[p][i]);
    if (a < 999) su += ledArray[a];
  }
  //if (su > 0) msgF(F("Sum Nab"), su);
  return su;
}

void showChng() {
  Serial.println(F("  Hin:"));
  for (int  i = 0; i < hinP; i++) {
    prnln33(i, chng[i]);
  }
  Serial.println(F("  Weg:"));
  for (int  i = chngM; i > wegP; i--) {
    prnln33(i, chng[i]);
  }
}

void birth(byte p) {
  //uint16_t sumG=0; // must be 3
  uint16_t sumR = 0;
  uint16_t sumB = 0;
  uint16_t a;
  for (int i = 0; i < 8; i++) {
    a = pgm_read_word(&nab[p][i]);
    if (a < 999) {
      sumR += ledArray[a + 1];
      sumB += ledArray[a + 2];
    }
  }
  sumR = sumR / 3;
  sumB = sumB / 3;
  if (sumR > sumB) {
    if (sumR > maxpix) sumR = maxpix;
    sumB = maxpix - sumR;
  } else {
    if (sumB > maxpix) sumB = maxpix;
    sumR = maxpix - sumB;
  }
  if (verb > 0) {
    Serial.print(F("Birth at "));
    prn3u(p);
    prnln33(sumR, sumB);
  }
  pix[0] = 1;
  pix[1] = sumR;
  pix[2] = sumB;
  pix2arr(p);
}

void execChng() {
  pix2color(9); //save
  //if (verb > 0) Serial.println(F("  Hin:"));
  for (int  i = 0; i < hinP; i++) {
    //if (verb > 0)  prnln33(i, chng[i]);
    birth(chng[i]);
  }
  //if (verb > 0) Serial.println(F("  Weg:"));
  color2pix(0);
  for (int  i = chngM; i > wegP; i--) {
    //if (verb > 0) prnln33(i, chng[i]);
    pix2arr(chng[i]);
  }
  color2pix(9);
  upd = true;
}

void buildChng() {
  //startTim = micros();
  hinP = 0;
  wegP = chngM;
  uint16_t su;
  uint16_t a;
  for (int  i = 0; i < LEDNUM; i++) {
    a = arrPos(i);
    su = sumNab(i);
    if (su == 3) {
      if (ledArray[a] == 0) {
        chng[hinP] = i;
        hinP++;
        if (hinP == wegP) {
          errF(F("buildChng Hin"), hinP);
          return;
        }
      }
    } else if (su == 2) {

    } else {
      if (ledArray[a] != 0) {
        chng[wegP] = i;
        wegP--;
        if (hinP == wegP) {
          errF(F("buildChng Weg"), hinP);
          return;
        }

      }
    } // su
  } // next
  //startTim = micros() - startTim;
  //Serial.print (F("buildChng us="));
  //Serial.println (startTim);
}

char doCmd(unsigned char tmp) {
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

  inpAkt = false;

  switch (tmp) {
    case '?':   //
      showInp();
      break;
    case 'a':
      simu = !simu;
      if (simu) {
        msgF(F("Simu on"), tpr);
        tic = 0;  // start immediately
      } else {
        msgF(F("Simu off"), tpr);
      }
      break;
    case 'A':
      tpr = inp;
      msgF(F("ticks per Run"), tpr);
      break;
    case 'b':   // set blue (2)
      pix[2] = inp;
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
      break;
    case 'd':   //
      doFuncs(inp);
      break;
    case 'e':   //
      evqPut(inp);
      break;
    case 'f':   // fill
      fillArr();
      break;
    case 'F':   //
      break;
    case 'g':   // set green (0)
      pix[0] = inp;
      break;
    case 'G':   //
      inp = pix[0];
      break;
    case 'h':   //
      buttold = PINC;
      Serial.print(buttold);
      break;
    case 'i':   //
      showChng();
      break;
    case 'j':   // jump
      befP = inp;
      if (runMode > 0) { //already running
        befP--; // as inced below
      } else { //
        runMode = 3;
      }
      msgF(F("jump "), befP);
      break;
    case 'J':
      befP = bef2end();
      msgF(F("to End"), befP);
      break;
    case 'k':   //
      runMode = 3;
      msgF(F("runmode"), runMode);
      break;
    case 'K':   //
      break;
    case 'l':   //
      pix2arr(inp);
      break;
    case 'L':   //
      arr2pix(inp);
      break;
    case 'm':   //
      if (inp >= pgmbefM) {
        readPage(inp);
      } else {
        pgm2Bef(inp);
      }
      showBef();
      break;
    case 'n':   //
      zwi = sumNab(inp);
      msgF(F("sumNab"), zwi);
      break;
    case 'N':   //
      setNab(inp);
      break;
    case 'o':  //
      buildChng();
      showChng();
      break;
    case 'p':  //
      buildChng();
      execChng();
      break;
    case 'P':
      maxpix = inp;
      msgF(F("maxpix"), inp);
      break;
    case 'q':  // quit
      runMode = 0;
      Serial.println(F("Stop"));
      break;
    case 'r':   // set red (1)
      pix[1] = inp;
      break;
    case 'R':   //
      inp = pix[1];
      break;
    case 'T':
      tickTim = inp;
      msgF(F("tickTim"), inp);
      break;
    case 'u':  // show  all texts
      showTxtAll();
      break;

    case 'U':  // show one text
      showTxt(inp);
      break;

    case 'v':  //
      switch (verb) {
        case 0:
          verb = 1;
          Serial.println(F("eins"));
          break;
        case 1:
          Serial.println(F("zwei"));
          verb = 2;
          break;
        default:
          verb = 0;
          Serial.println(F("nix"));
      }
      break;
    case 'V':  //
      echo = !echo;
      if (echo)  Serial.println(F("echo")); else Serial.println(F("still"));
      break;
    case 'w':  //
      writePage(inp);
      break;
    case 'W':   //
      break;
    case 'x':  // gosub
      exec(inp, 0);
      break;
    case 'y':  //
      zwi = inp;
      if (inpSP > 0) {
        inp =  inpPop(); //if  something pushed???->Verwirrung grande
      } else {
        inp = inpsw;    // else take it from  swap
      }
      exec(zwi, 1);
      break;
    case 'Y':  //
      char2Bef(inp);
      break;
    case 228:  // ä same ägain
      exec(befNum, 0);
      break;
    case 'z':  //
      dark();
      msgF(F(" zero "), 0);
      break;
    case ' ':
      simu = false; //
      break;
    case 13:
      info();
      break;
    case 223:   //ß 223 ä 228 ö 246 ü 252)
      showInp();
      //showLoop();
      //showPis();
      break;
    case ',':   //
      inpPush();
      break;
    case '~':   //
      zwi = inp;
      inp = inpsw;
      inpsw = zwi;
      msgF(F("Swap"), inp);
      break;
    case '+':   //
      inp =  inpPop() + inp;
      break;
    case '-':   //
      inp = inpPop() - inp;
      break;
    case '"':   // to bef
      seriBef();
      showBef();
      break;
    default:
      Serial.print ("?");
      Serial.println (byte(tmp));
      return '?';
  } //switch
  return tmp ;
}

void setup() {
  const char ich[] PROGMEM = "ws2812GoL " __DATE__  " " __TIME__;
  Serial.begin(38400);
  Serial.println(ich);
  pinMode(ws2812_out, OUTPUT);
  pinMode(rc5Pin , INPUT_PULLUP);
  pinMode(A0, INPUT_PULLUP); // PortC 0
  pinMode(A1, INPUT_PULLUP); // PortC
  pinMode(A2, INPUT_PULLUP); // PortC
  pinMode(A3, INPUT_PULLUP); // PortC
  pinMode(A4, INPUT_PULLUP); // PortC 4 (SDA)
  pinMode(A5, INPUT_PULLUP); // PortC 5 (SCL)
  RC5_init();
  debug = false;
  runMode = 0;
  dark();
  ledArray[0] = 25; //"Alles im grünen Bereich"
  ledArray[1] = 0;
  ledArray[2] = 0;
}

void loop() {
  unsigned char c;

  if (Serial.available() > 0) {
    c = Serial.read();
    if (echo)  Serial.print(char(c));
    doCmd(c);
  }

  if (rc5_ok) {
    RC5_again();
    doRC5();
  }

  if (runMode > 0) { //executing
    c = chunk.bef[befP];
    if (runMode != 2) { //not single step
      //c = retpix(c); // return if pix = led
      if (c == 0) {
        msgF(F("bef done "), befNum);
        inpAkt = false ; // just in case...
        if (stackP > 0) {
          prgPop();
        } else { //end of execution
          runMode = 0;
          refresh();  //last upd should be included in time
          if (verb > 0) {
            startTim = micros() - startTim;
            Serial.print (F("  Fini us="));
            Serial.println (startTim);
          }
        } //eof
      }  else { // cmd
        doCmd(c);
        befP++;
        if (runMode == 1) { //single step executed
          runMode = 2;
          Serial.println();
          info();
        }
      }
    } // runM
    return;
  } // runM >0

  // from here runM 0 only

  // process event
  if (evqIn != evqOut) {
    doFuncs(evqGet());
  }


  currTim = millis();
  if (currTim - prevTim >= tickTim) {
    if (verb > 2) Serial.print('.');
    prevTim = currTim;

    if (simu) {
      if (tic > 0) {
        tic--;
      } else {
        tic = tpr;
        buildChng();
        execChng();
      }
    }

    butt = PINC & 0x3F; //A0 to A5
    if (butt != buttold) {
      buttold = butt;
      buttcnt = 0;
      evqPut(butt);
    } else {  //repeat?
      if (butt != 63) {
        buttcnt++;
        if (buttcnt > 20) {
          buttcnt = 0;
          evqPut(butt);
        }
      }
    }

    refresh();
  }
}

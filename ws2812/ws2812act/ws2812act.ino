/* Sketch  to play with WS2812 LEDs
  based on driver discussed and made by Tim in http://www.mikrocontroller.net/topic/292775
  Commands read via serial, from and to Flash using optiboot.
*/

#define LEDNUM 256          // Number of LEDs in stripe
#define ws2812_port PORTB   // Data port register
#define ws2812_pin 1        // Number of the data out pin there ,
#define ws2812_out 9       // resulting Arduino num
#include "optiboot.h"
// allocate flash to write to, must be initialized, one more than used as 0 for EOS
#define NUMBER_OF_PAGES 50
const uint8_t flashSpace[SPM_PAGESIZE * (NUMBER_OF_PAGES + 1)] __attribute__ (( aligned(SPM_PAGESIZE) )) PROGMEM = {"\x0"};
#define ARRAYLEN LEDNUM *3  // 
byte   ledArray[ARRAYLEN + 3]; // Buffer GRB plus one pixel

const byte rc5Pin = 2;        // TSOP purple
byte   pix[3];                // current pixel
byte   apix[3];               // analog pixel value (tbd)
const byte colorM = 10;       // colors conf
byte color[colorM * 3];
bool debug;                   // true if
byte  runMode;                // 0 not run, 1 single step, 2 ss after exe, 3 run
bool   upd;                   // true refresh disp
int    inp;                   // numeric input
int    inpsw;                 // swap-register
bool inpAkt = false;          // true if last input was a number
const byte inpSM = 7;        // Stack for inps conf
int inpStck[inpSM];           //
byte  inpSP = 0;              //
byte verb = 3;                // verbose 0 garnix 1 (.*) 2 debug 3 alles
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

int befP;               // next to execute
byte befNum;           // current bef: 0..39 and 40 ..
byte butt;             // Buttons on PortC
byte buttold = 63;
byte buttcnt = 0;
// event-que
const byte evqM = 5;
byte evq[evqM];
byte evqIn = 0;
byte evqOut = 0;

// agent-Specifics
byte tick;
const byte agM = 5;
bool agentsOn = false;
byte agc = agM;           // current agent, agM if none
byte agTack[agM];        // when restart
byte agDelt[agM];        // Frequency
byte agActive[agM];      // 0 no, 1 active
byte agBefNum[agM];      // which bef to start
byte agInpSp[agM];
int agInpStck[agM][inpSM];

const byte stackM = 5;      // Stack depth for prog exec
byte stackP;                // points to next free
int stackBefP[stackM];      // saved befP
byte stackBefNum[stackM];   // saved befNum
const byte memoM = 22;      // Memory 9.. special usage
int memo[memoM];            //

const byte loopM = 4;         // max loop level
byte loopLev;
int loopCurr[loopM];
int loopInc[loopM];
int loopEnd[loopM];
byte loopBefP[loopM];
#define TETRIS

#include "myBefs.h"
#include "rc5_tim2.h"

void prnt(PGM_P p) {
  while (1) {
    char c = pgm_read_byte(p++);
    if (c == 0) break;
    Serial.write(c);
  }
  Serial.write(" ");
}

void msgF(const __FlashStringHelper *ifsh, int n) {
  // text verbraucht nur flash
  if (verb > 1) {
    PGM_P p = reinterpret_cast<PGM_P>(ifsh);
    prnt(p);
    Serial.println(n);
  }
}

void debF(const __FlashStringHelper *ifsh, int n, byte lev) {
  // text verbraucht nur flash
  if (verb >= lev) {
    PGM_P p = reinterpret_cast<PGM_P>(ifsh);
    prnt(p);
    Serial.println(n);
  }
}

void errF(const __FlashStringHelper *ifsh, int n) {
  runMode = 0;
  agentsOn = false;
  Serial.print(F("ErrF: "));
  PGM_P p = reinterpret_cast<PGM_P>(ifsh);
  prnt(p);
  Serial.println(n);
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

#ifdef TETRIS
#include "tetris.h"
#endif

bool readPage(byte page) {
  page = 1 + page - pgmbefM;
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
  //translate logical to phys position
  int sp = p % 16;
  switch (sp) {
    case 8: p = p + 7; break;
    case 9: p = p + 5; break;
    case 10: p = p + 3; break;
    case 11: p = p + 1; break;
    case 12: p = p - 1; break;
    case 13: p = p - 3; break;
    case 14: p = p - 5; break;
    case 15: p = p - 7; break;
  }
  //p = 255 - p; upside down
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

void randvalCol(byte col) {
  int ran, rano;
  if (col > 2) {
    errF(F("randval"), col);
    return;
  }
  byte brmi = 10 + col * 4; // memo locations per color
  byte brma = brmi + 1; //max
  byte bvmi = brmi + 2;
  byte bvma = brmi + 3;
  rano = random(memo[brmi], memo[brma]);
  ran =  rano + pix[col];
  if (ran < memo[bvmi]) ran = memo[bvmi];
  if (ran > memo[bvma]) ran = memo[bvma];
  pix[col] = ran;
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
    bf = chunk.bef[befP];
  }
  Serial.print("'");
  Serial.print(bf);
  Serial.print("' '");
  Serial.print(chunk.bef);
  Serial.println("'");
}

void info() {
  char str[100];
  sprintf(str, "RM %1u befN%4d befP %2d Stp %2u LL %2u Inp %5d pix R%3u G%3u B%3u ", runMode, befNum, befP, stackP, loopLev, inp, pix[1], pix[0], pix[2]);
  Serial.print(str);
  showBef();
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

bool pgm2Bef(byte num) {
  if (num >= pgmbefM) {
    errF(F("pgm2Bef "), num);
    return false;
  }
  strcpy_P(chunk.bef, (char *)pgm_read_word(&(pgmbefehle[num])));
  befNum = num;
  return true;
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


void los() {
  //pgm in bef
  msgF(F("Los"), 0);
  loopLev = 0;
  stackP = 0;
  if (runMode == 0) {
    befP = 0;
    runMode = 3;
    startTim = micros();
  } else {
    befP = -1;
  }
}

void showStack() {
  char str[50];
  char dd[4];
  const static char dj[] PROGMEM = "-> ";
  const static char dn[] PROGMEM = "   ";
  Serial.println(F(" Progstack:"));
  for (int i = 0; i < stackM; i++) {
    if (i == stackP) {
      strcpy_P(dd, dj );
    } else {
      strcpy_P(dd, dn );
    }
    sprintf(str, "%s %2u N %4d P %2u", dd, i, stackBefNum[i], stackBefP[i]);
    Serial.println(str);
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

void wasPush(int was) {
  //msgF(F("wasPush"), inp);
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
    //msgF(F("inpPop"), inpStck[inpSP]);
    return inpStck[inpSP];
  } else {
    errF(F("inpstack Underflow"), 0);
    return 0;
  }
}

void inpToAg() {
  msgF(F("inpToAg"), agc);
}

void inp2Memo() {
  msgF(F("inp2Memo "), inp);
  if (inp > memoM) {
    errF(F("memo "), inp);
  } else {
    memo[inp] = inpPop();
  }
}

void memo2Inp() {
  msgF(F("memo2Inp "), inp);
  if (inp > memoM) {
    errF(F("memo "), inp);
  } else {
    inp = memo[inp];
  }
}

void showInp() {
  char str[50];
  char dd[4];
  const static char dj[] PROGMEM = "-> ";
  const static char dn[] PROGMEM = "   ";
  Serial.print(F(" inp="));
  Serial.println(inp);
  for (int i = 0; i < inpSM; i++) {
    if (i == inpSP) {
      strcpy_P(dd, dj );
    } else {
      strcpy_P(dd, dn );
    }
    Serial.print(dd);
    sprintf(str, "%2u %6d", i, inpStck[i]);
    Serial.println(str);
  }
  //return;
  Serial.println(F("Memo:"));
  for (byte i = 0; i < memoM; i++) {
    if (memo[i] != 0) {
      sprintf(str, "%2u  %5d ", i, memo[i]);
      Serial.println(str);
    }
  }
}

void showLoop() {
  char str[60];
  Serial.print(F("Loops: "));
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
  msgF(F("For with"), inp);
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
    msgF(F("Loop"), inp);
  } else {
    inp = loopCurr[loopLev];
    loopLev--;
    msgF(F("Loop End"), inp);
  }
}

void agCop2ag(byte ag) {
  // copy inps to ag
  //msgF(F("Cop2ag"), ag);
  for (byte i = 0; i < inpSM; i++) {
    agInpStck[ag][i] = inpStck[i];
  }
  agInpSp[ag] = inpSP;
}

void agCop2inp(byte ag) {
  // copy ag to inp
  //msgF(F("Cop2inp"), ag);
  for (byte i = 0; i < inpSM; i++) {
    inpStck[i] = agInpStck[ag][i];
  }
  inpSP = agInpSp[ag];
}

void agShow(byte ag) {
  char str[50];
  sprintf(str, "%2u %3d %1d %4d %4d", ag, agBefNum[ag], agActive[ag], agTack[ag], agDelt[ag]);
  Serial.println(str);
}

void agShowAll() {
  Serial.print(F("\x1B[H\x1B[J"));
  Serial.print(F("  Bef Act Tack Dlt  "));
  Serial.println(tick);
  for (byte i = 0; i < agM; i++) {
    agShow(i);
  }
}

void agStart() {
  // pop params, inp is ag to start
  if (inp >= agM) {
    errF (F("Agent Num"), inp);
    return;
  }
  byte ag = inp;
  agDelt[ag] = inpPop();
  agBefNum[ag] = inpPop();
  agActive[ag] = 1;
  agTack[ag] = tick + 1;
  agCop2ag(ag);
  msgF (F("Agent Start"), ag);
}


void agStop() {
  // pop params, inp is ag to start
  if (inp >= agM) {
    errF (F("Agent Num"), inp);
    return;
  }
  if (agActive[inp] == 0) {
    msgF (F("Agent Start"), inp);
    agActive[inp] = 1;
  } else {
    msgF (F("Agent Stop"), inp);
    agActive[inp] = 0;
  }
}

void refresh() {
  if (upd) {
    ws2812_sendarray(ledArray, ARRAYLEN);
    upd = false;
    if (verb > 0)  Serial.print("*");
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


char doCmd(unsigned char tmp) {
  bool weg = false;
  int zwi;
  byte zwib;
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
      break;

    case 'a':   //
      agStart();
      break;
    case 'A':   //
      agStop();
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
    case 'F':   // fill
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
    case 'i':  //
      agShowAll();
      break;
    case 'I':  // reset
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
    case 'M':   //
      break;
    case 'n':   //
      errF(F("No n please!"), 0);
      break;
    case 'o':  //
      agentsOn = !agentsOn;
      if (agentsOn)  Serial.println(F("Agents On")); else Serial.println(F("Agents Off"));
      break;
    case 'p':  // pix
      randvalCol(inp);
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
    case 't': // tack-offset
      zwib = inp; //agent,
      if (zwib < agM) {
        agTack[zwib] = inpPop() + tick;
      } else {
        errF(F("Tack"), inp);
      }
      break;
    case 'T':
      tickTim = inp;
      msgF(F("tickTim"), inp);
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
    case 228:  // ä same again
      exec(befNum, 0);
      break;
    case 'X':  // exec Bef
      los();
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
    case 'z':  //
      dark();
      msgF(F(" zero "), 0);
      break;
    case ' ':
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
    case '>':   //
      inp2Memo();
      break;
    case '"':   // to bef
      if (seriBef() == 0) {
        showBef();
      } else {
        los();
      }
      break;
    case '~':   //
      zwi = inp;
      inp = inpsw;
      inpsw = zwi;
      msgF(F("Swap"), inp);
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
    case '{':   //
      if (inp == inpsw)  befP = bef2end();
      break;
    case '[':   //
      if (pixcmp(inp))  befP = bef2end();
      break;
    case '}':   //
      if (inp != inpsw)  befP = bef2end();
      break;
    case ']':   //
      if (!pixcmp(inp))  befP = bef2end();
      break;

    default:
      Serial.print ("?");
      Serial.println (byte(tmp));
      return '?';
  } //switch
  return tmp ;
}

void setup() {
  const char ich[] PROGMEM = "ws2812ACT " __DATE__  " " __TIME__;
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

/*  if (rc5_ok) {
    RC5_again();
    doRC5();
  } */

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
          if (agc < agM) { // have to save agents stack
            agCop2ag(agc);
            agc = agM;
          }
          refresh();  //last upd should be included in time
          /*if (verb > 0) {
            startTim = micros() - startTim;
            Serial.print (F("  Fini us="));
            Serial.println (startTim);
          }
          */
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

  // process event (if rm 0 only)
  if (evqIn != evqOut) {
    doFuncs(evqGet());
  }

  // check for agent
  if (agentsOn) {
    for (int i = 0; i < agM; i++) {
      if (agActive[i] == 1) {
        if (tick == agTack[i]) {
          agc = i;
          msgF(F("Agent activate"), i);
          agTack[i] += agDelt[i];
          agCop2inp(agc);
          exec(agBefNum[agc], 0);
          return;
        }
      }
    }
  } // agents

  currTim = millis();
  if (currTim - prevTim >= tickTim) {
    if (verb > 2) Serial.print('.');

    prevTim = currTim;
    if (agentsOn) tick++;

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

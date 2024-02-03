/* Sketch  to play with WS2812 LEDs
   based on driver discussed and made by Tim in http://www.mikrocontroller.net/topic/292775
   Commands read via serial, from and to Flash using optiboot.
*/

#define LEDNUM 80           // Number of LEDs in stripe
#define ws2812_port PORTB   // Data port register
#define ws2812_pin 2        // Number of the data out pin,
#define ws2812_out 10       // resulting Arduino num
#include "optiboot.h"
// allocate flash to write to, must be initialized, one more than used as 0 for EOS
#define NUMBER_OF_PAGES 50
const uint8_t flashSpace[SPM_PAGESIZE * (NUMBER_OF_PAGES + 1)] __attribute__ (( aligned(SPM_PAGESIZE) )) PROGMEM = {"\x0"};
#define ARRAYLEN LEDNUM *3  // 
byte   ledArray[ARRAYLEN + 3]; // Buffer GRB plus one pixel

const byte rc5Pin = 2;        // TSOP purple
byte   pix[3];                // current pixel
byte   apix[3];               // analog pixel
byte   ptmp[3];               // temp for shifts
const byte colorM = 10;       // colors conf
byte color[colorM * 3];
bool debug;                   // true if
uint8_t   runMode;            // 0 not run, 1 single step, 2 ss after exe, 3 run
bool   upd;                   // true refresh disp
int    inp;                   // numeric input
int    inpsw;                 // swap-register
bool inpAkt = false;          // true if last input was a number
const byte inpSM = 10;        // Stack for inps conf
int inpStck[inpSM];           //
byte  inpSP = 0;              //
byte verb = 2;                // verbose 0 garnix 1 (.*) 2 debug 3 alles
bool echo = true ;            // echo back chars received
byte led = 0;                 // last used LED
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

int befP;                // next to execute
byte befNum;            // current bef: 0..39 and 40 ..

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

const byte stackM = 5;        // Stack depth for prog exec
byte stackP;              // points to next free
int stackBefP[stackM];        // saved befP
byte stackBefNum[stackM];      // saved befNum
const byte memoM = 22;        // Memory 9.. special usage
int memo[memoM];              //

const byte loopM = 4;         // max loop level
byte loopLev;
int loopCurr[loopM];
int loopInc[loopM];
int loopEnd[loopM];
byte loopBefP[loopM];

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

bool readPage(byte page) {
  page -= 39; // page is translated -39 so eg 40 reads 1
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
  page -= 39;
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
      chunk.bef[i] = 0;
      return;
    }
    chunk.bef[i] = b;
  } // loop
  errF (F("Seri "), i);
  chunk.bef[i] = 0;
}

bool pgm2Bef(byte num) {
  if (num >= pgmbefM) {
    errF(F("pgmbef "), num);
    return false;
  }
  strcpy_P(chunk.bef, (char *)pgm_read_word(&(pgmbefehle[num])));
  befNum = num;
  return true;
}


void exec(byte num) {
  // get pgm and execute
  startTim = millis();
  msgF(F("Exec"), num);
  if (runMode > 0) {
    push();           // store  current
  }
  if (num >= pgmbefM) {
    if (!readPage(num)) {
      return;         // err already thrown
    }
  } else {
    if (!pgm2Bef(num)) {
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
  msgF(F("prgPush"), stackP);
  if (stackP >= stackM) {
    errF(F("Stack Overflow"), stackP);
  } else {
    stackBefP[stackP] = befP; // is inc'd on pop
    stackBefNum[stackP] = befNum;
    stackP++;
  }
}

void pop() {
  msgF(F("prgPop"), stackP);
  if (stackP > 0) {
    stackP--;
    befP = stackBefP[stackP] + 1;
    if (stackBefNum[stackP] < pgmbefM) {
      pgm2Bef(stackBefNum[stackP]);
    } else {
      readPage(stackBefNum[stackP]);
    }
  } else {
    errF(F("prg stack Underflow"), 0);
  }
}

void inpPush() {
  msgF(F("inpPush"), inp);
  if (inpSP >= inpSM) {
    errF(F("inp Overflow"), inpSP);
  } else {
    inpStck[inpSP] = inp; // is inc'd on pop
    inpSP++;
  }
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
  return;
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
  char str[40];
  sprintf(str, " %1u %3d %1d %4d %4d ", ag, agBefNum[ag], agActive[ag], agTack[ag], agDelt[ag]);
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

#include "myFuncs.h"


char doCmd(char tmp) {
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
      seriBef();
      showBef();
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
      doFuncs();
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
    case 'h':   // halt
      break;
    case 'i':  //
      agShowAll();

      break;
    case 'I':  // reset
      break;
    case 'j':   //
      befP = inp;
      if (runMode > 0) { //already running
        befP--; // as inced below
      } else { //
        runMode = 3;
      }
      msgF(F("jump "), befP);
      break;
    case 'k':   //
      runMode = 3;
      msgF(F("runmode"), runMode);
      break;
    case 'K':   //
      break;
    case 'l':   //
      led = inp;
      pix2arr(led);
      break;
    case 'L':   //
      arr2pix(inp);
      break;
    case 'm':   //
      if (inp < pgmbefM) {
        pgm2Bef(inp);
      } else {
        readPage(inp);
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
      exec(inp);
      break;
    case 'X':  // exec Bef
      los();
      break;
    case 'y':  //
      errF(F("No y please!"), 0);
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
  RC5_init();
  debug = false;
  runMode = 0;
  dark();
  ledArray[0] = 25; //"Alles im grünen Bereich"
  ledArray[1] = 0;
  ledArray[2] = 0;
}

void doRun() {

}
void loop() {
  char c;

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
    c = chunk.bef[befP];
    if (runMode != 2) { //single step
      if (c == 0) {
        msgF(F("bef done "), befP);
        inpAkt = false ; // just in case...
        if (stackP > 0) {
          pop();
        } else { //end of execution
          runMode = 0;
          if (agc < agM) { // have to save stack
            agCop2ag(agc);
            agc = agM;
          }
          refresh();  //last upd should be included in time
          if (verb > 0) {
            startTim = millis() - startTim;
            Serial.print (F("  Fini ms="));
            Serial.println (startTim);
          }
        }
      }  else { // valid cmd
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
  } // exec

  // check for agent
  if (agentsOn) {
    for (int i = 0; i < agM; i++) {
      if (agActive[i] == 1) {
        if (tick == agTack[i]) {
          agc = i;
          msgF(F("Agent activate"), i);
          agTack[i] += agDelt[i];
          agCop2inp(agc);
          exec(agBefNum[agc]);
          return;
        }
      }
    }
  } // agents

  currTim = millis();
  if (currTim - prevTim >= tickTim) {
    prevTim = currTim;
    if (agentsOn) tick++;
    refresh();
  }
}

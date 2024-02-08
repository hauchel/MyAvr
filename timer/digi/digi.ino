// Digital Input storage on atmega 328
// Timer1 wgm 15, Compare Match B triggers read, OCR1A determines frequency
// Drehgeber
#include "helper.h"

// pins for timers
//const byte poc2B = 3;  // pd3   // reserved Tim2
const byte pcinp = 5;    // pd5   Input (also Count for CS 6,7)
const byte picp1 = 8;    // pb0   Input Capt
const byte poc1A = 9;    // pb1
const byte poc1B = 10;   // pb2
#define ws2812_port PORTB   // Data port register
#define ws2812_pin 3        // Number of the data out pin,
#define ws2812_out 11       // resulting Arduino num
// pins for debug
const byte povf = 12;    //pb4
const byte dovfS = _BV(4); // to set/reset pin fast
const byte dovfC = dovfS  ^ 0xFF;
const byte pcap = 13;    //pb5p
const byte dcapS = _BV(5);
const byte dcapC = dcapS  ^ 0xFF;
// Use A0 to A3 as input  pc

// counts for ints, just used to see if called
byte volatile cntO; // overflow
byte volatile cntA; //
byte volatile cntB; //
byte volatile cntC; // capture
byte volatile cntD; // aDc

// Timer properties        to change in cmdTim-Mode
byte tim1WGM = 0;        // w  to TCCRA and TCCRB
byte tim1CS = 2;         // c  clock select to TCCRB
byte tim1COM1A = 1;      // d  compare Match Output A 0..3 to  TCCRA
byte tim1COM1B = 1;      // e  compare Match Output B 0..3 to  TCCRA
byte tim1TIMSK1 = 0 ;    // f  interrupt enable to TIMSK1
byte tim1IC =  0;        // m  ICNC1 and ICES1 0..3 to TCCRB
float uspt = 0.06255;    // u  usec per tick depending on quartz. must start with 0.0  use u to set inp * 0.0001
const uint16_t tim1presc[6] = {0, 1, 8, 64, 256, 1024};

bool volatile oneShot = false; // o to switch
uint16_t volatile ovcnt;
bool volatile ferdisch = false; // measure complete
bool cmdTim = false; // to use letters 2way, g to switch
// ringbuffer for captured values
const byte ripuM = 20;
byte ripuLis  [ripuM];
byte volatile ripuP = 0; // pointer, also used to stop recording when >=ripuM
byte ripuOld = 0;       //
byte showCnt = 0;       // for CR/LF in verbo 2
byte ripuLisOld = 0;
byte verbo = 0; //
uint16_t anzH;
byte messP, messN;
int posi = 0;
int posiOld = 0;

#define LEDNUM 10           // Number of LEDs in stripe

void timer1Init() {
  TCCR1B = 0;  // stop timer by setting CS to 0
  TIMSK1 = 0; // no timer ints during init
  TCNT1 = 0;
  ovcnt = 0;
  //   7      6        5       4        3       2       1       0
  // COM1A1 COM1A0   CO1MB1  COM1B0     –       –      WGM11   WGM10    TCCR1A
  //    cmoA            cmoB        0       0       w       w
  TCCR1A = (tim1COM1A << 6) | (tim1COM1B << 4) | (tim1WGM & 3);
  //                                        OCIE1B  OCIE1A  TOIE1   TIMSK1
  // 0      0       0       0       0       o       o       o
  TIMSK1 = tim1TIMSK1;     // Timer interrupts
  // ICN1  ICES1    –      WGM13    WGM12   CS22    CS21    CS20    TCCR1B
  // i      i       0       w       w        c       c       c
  TCCR1B = (tim1IC << 6) | tim1CS | ((tim1WGM << 1) & 0x18);
}

ISR(TIMER1_OVF_vect) {
  PORTB |= dovfS; //debug
  cntO++;
  ovcnt++;
  PORTB &= dovfC; //debug
}

ISR(TIMER1_COMPA_vect) {
  cntA++;
}

ISR(TIMER1_COMPB_vect) {
  cntB++;
  PORTB |= dcapS; //debug
  if (ripuP >= ripuM) {
    PORTB &= dcapC; //debug
    return;
  }
  ripuLis[ripuP] = PINC & 0x0F;
  ripuP++;
  if (ripuP >= ripuM) {
    if (oneShot)  { // no more capture ints
      ferdisch = true;
      return;
    }
    ripuP = 0;
  }
  PORTB &= dcapC; //debug
}

ISR(TIMER1_CAPT_vect) {
  cntC++;
}

#include "ws28.h"
#include "epr.h"

float us2cs() {
  // returns duration of one tick depending on clock source
  if (tim1CS < 6) {
    return uspt * tim1presc[tim1CS];
  }
  return 0.0;
}
void showCnts() {
  char str[80];
  sprintf(str, "O:%4u A:%4u B:%4u C:%4u D:%4u", cntO, cntA, cntB, cntC, cntD);
  Serial.println(str);
}

void showRegs() {
  char str[120];
  sprintf(str, "TCRA: 0x%02X  TCRB: 0x%02X  TIMSK1: 0x%02X ", TCCR1A, TCCR1B, TIMSK1);
  Serial.println(str);
  msgF(F("_a OCR1A   "), OCR1A);
  msgF(F("_b OCR1B   "), OCR1B);
  msgF(F("_c CS      "), tim1CS);
  msgF(F("_d COM1A   "), tim1COM1A);
  msgF(F("_e COM1B   "), tim1COM1B);
  msgF(F("_f TIMSK1  "), tim1TIMSK1);
  msgF(F("_m ICNC1/ICES1 "), tim1IC);
  msgF(F("_w WGM     "), tim1WGM);
}

void showRipu() {
  char str[120];
  float zwi = us2cs();
  float dauer;
  vt100Clrscr();
  sprintf(str, "P %3u  auto %1u uspt ",  ripuP, oneShot);
  Serial.print(str);
  Serial.print(uspt, 6); //sprintf doesn't like floats
  Serial.print(" clo ");
  Serial.print(zwi, 6);
  dauer = zwi * OCR1A;
  dauer = 1000000 / dauer; // Freq
  Serial.print("  MessF ");
  Serial.println(dauer, 3);
  Serial.println(F(" #   An  In"));
  for (byte i = 0; i < ripuM; i++ ) {
    sprintf(str, "%2u  %2u", i, ripuLis[i]);
    Serial.println(str);
  }
}
//   15  14  12  13  15
//     ------>  ---->
void evalMess() {
  // checks  entry at ripuOld, sets posi
  char str[40];
  uint16_t anz;
  const byte pos1 = 15;
  const byte pos12 = 14;
  const byte pos2 = 12;
  const byte pos21 = 13;

  PORTB |= dovfS; //debug
  if (ripuLisOld != ripuLis[ripuOld]) { // change
    messP = messN;
    messN = ripuLis[ripuOld];
    if (messN == pos1) {
      if (messP == pos21) {
        posi += 1;
      }
      if (messP == pos12) {
        posi -= 1;
      }
    }
    if (messN == pos2) {
      if (messP == pos21) {
        posi -= 1;
      }
      if (messP == pos12) {
        posi += 1;
      }
    }
    anz = anzH;
    anzH = 0;
    ripuLisOld = ripuLis[ripuOld];
    if (verbo > 0) { //summary
      sprintf(str, " anz %3u", anz);
      Serial.println(str);
      Serial.print(ripuLisOld);
      showCnt = 0;
    }
  } //change
  anzH += 1;
  if (verbo == 2) {
    sprintf(str, "%4u", ripuLisOld);
    Serial.print(str);
    showCnt++;
    if (showCnt > 19) {
      showCnt = 0;
      Serial.println();
      Serial.print(" ");
    }
  }

  ripuOld++;
  if (ripuOld >= ripuM) ripuOld = 0;

  PORTB &= dovfC; //debug
}


void help () {
  Serial.println (F("Timer Mode: (n to toggle)"));
  Serial.println (F("_a  set OCR1A "));
  Serial.println (F("_b  set OCR1B "));
  Serial.println (F("_c  set clockselect 0, 1, 8, 64, 256, 1024, ext, ext"));
  Serial.println (F("_d  set COM1A (Pin 9) 0..3, 0 always not used"));
  Serial.println (F("_e  set COM1B (Pin 10)0..3, 0 always not used"));
  Serial.println (F("_f  set interrupts:  32 ICIE  4 OCIEB, 2 OCIEA, 1 TOIE"));
  Serial.println (F("i   init Timer (also called after sets c..f above"));
  Serial.println (F("_w  set wgm 0..15"));
  Serial.println (F("Common Mode:"));
  Serial.println (F("_b  Blue, B to get "));
  Serial.println (F("_k  read Konfig (0 to 10)"));
  Serial.println (F("_K  write Konfig (0 to 10)"));
  Serial.println (F("_l  set Led"));
  Serial.println (F("_m  set ICNC1 2 and ICES1 1 (0 to 3)"));
  Serial.println (F("o   toggle OneShot"));
  Serial.println (F("_p  ADPS Prescaler 0..7 = 2 ..1 128"));
  Serial.println (F("_r  Red, R to get"));
  Serial.println (F("_s  pointer to switch on"));
  Serial.println (F("_t  tick time in ms"));
  Serial.println (F("_u  set us per tick post - decs e.g. 1234u means 0.01234"));
  Serial.println (F("v   toggle verbose"));
  Serial.println (F("x   eXec one capture"));
  Serial.println (F("y   toggle auto"));
  Serial.println (F("z   show Ripu"));
}

bool doCmdTimer(char x) {
  // returns true if executed
  float tmpf;
  switch (x) {
    case 'a':
      OCR1A = inp;
      msgF(F(": OCR1A"), OCR1A);
      return true;
    case 'b':
      OCR1B = inp;
      msgF(F(": OCR1B"), OCR1B);
      return true;
    case 'c':
      tim1CS = inp & 7;
      timer1Init();
      msgF(F(": tim1CS"), tim1CS);
      return true;
    case 'd':
      tim1COM1A = inp & 3;
      timer1Init();
      msgF(F(": tim1COM1A"), tim1COM1A);
      return true;
    case 'e':
      tim1COM1B = inp & 3;
      timer1Init();
      msgF(F(": tim1COM1B"), tim1COM1B);
      return true;
    case 'f':
      tim1TIMSK1 = inp ;
      timer1Init();
      msgF(F(": tim1TIMSK1"), tim1TIMSK1);
      return true;
    case 'u': //
      msgF(F(" to uspt"), inp);
      uspt = 0.00001 * inp;
      Serial.println(uspt, 6);
      return true;
    case 'U': //
      msgF(F(" Dauer von"), inp);
      tmpf = uspt * inp;
      Serial.println(tmpf, 6);
      return true;
    case 'w':
      tim1WGM = inp & 15;
      timer1Init();
      msgF(F("wgm"), tim1WGM);
      return true;
    case 'z':
      showRipu();
      return true;

  }
  return false;
}

void doCmd(char x) {
  Serial.print(char(x));
  if (doNum(x))  return;
  Serial.println();
  if (cmdTim) {
    if (doCmdTimer(x)) return;
  }

  switch (x) {
    case 13:
      vt100Clrscr();
      break;
    case ' ':
      showCnts();
      break;
    case 'a':
      break;
    case 'b':   // set blue (2)
      pix[2] = inp;
      msgF(F("blue"), inp);
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
    case 'd':
      break;
    case 'e':
      break;
    case 'f':
      break;
    case 'g':   // set green (0)
      pix[0] = inp;
      msgF(F("green"), inp);
      break;
    case 'G':   //
      inp = pix[0];
      break;
    case 'h':
      help();
      break;
    case 'i':
      timer1Init();

      break;
    case 'j':
      showRegs();
      break;
    case 'k':
      readKonfig(inp);
      timer1Init();
      showRegs();
      break;
    case 'K':
      writeKonfig(inp);
      break;
    case 'l':   //
      led = inp;
      pix2arr(led);
      msgF(F("Leds"), led);
      break;
    case 'L':   //
      msgF(F("LedL"), inp);
      arr2pix(inp);
      break;
    case 'm':
      tim1IC = inp & 3;
      timer1Init();
      msgF(F(": tim1IC"), tim1IC);
      break;
    case 'n':
      cmdTim = !cmdTim;
      msgF(F("cmdTim"), cmdTim);
      break;
    case 'o':
      oneShot = !oneShot;
      msgF(F("oneShot"), oneShot);
      break;
    case 'r':   // set red (1)
      pix[1] = inp;
      msgF(F("red"), inp);
      break;
    case 'R':   //
      inp = pix[1];
      break;
    case 't':   //
      tickMs = inp;
      msgF(F("tickMs"), tickMs);
      break;
    case 'v':
      verbo++;
      if (verbo > 2) verbo = 0;
      //if (verbo > 0) verbo = 2; // no 1
      msgF(F("Verbose"), verbo);
      break;
    case 'x': //start in auto
      ovcnt = 0; // to get convenient values
      ripuP = 0; // worst case ripuP is inced now
      break;
    case 'y':
      break;
    case 'z':  //
      dark();
      break;
    default:
      Serial.print('?');
      Serial.println(int(x));
  } // case
}


void setup() {
  const char info[] = "dig " __DATE__  " "  __TIME__;
  Serial.begin(38400);
  Serial.println(info);
  pinMode(picp1, INPUT_PULLUP);
  pinMode(pcinp, INPUT_PULLUP);
  pinMode(poc1A, OUTPUT);
  pinMode(poc1B, OUTPUT);
  pinMode(povf, OUTPUT);
  pinMode(pcap, OUTPUT);
  pinMode(A0, INPUT_PULLUP);
  pinMode(A1, INPUT_PULLUP);
  pinMode(A2, INPUT_PULLUP);
  pinMode(A3, INPUT_PULLUP);
  pinMode(ws2812_out, OUTPUT);
  tickMs = 100;
  
  readKonfig(0);   // wtf odbc1a??
  timer1Init();
}

void loop() {
  if (Serial.available() > 0) {
    doCmd( Serial.read());
    if (cmdTim) Serial.print("Ti ");
  }

  if (ripuP < ripuM) {
    if (ripuP != ripuOld) {
      evalMess();
    }
  }
  if (posi != posiOld) {
    Serial.print("Po ");
    color2pix(0);
    pix2arr(posiOld);
    if (posi < 0) posi = LEDNUM - 1;
    if (posi >= LEDNUM) posi = 0;
    Serial.println(posi);

    posiOld = posi;
    color2pix(1);
    pix2arr(posiOld);
  }

  if (ferdisch) {
    ferdisch = false;
    showRipu();
  }

  currMs = millis();
  if (currMs - prevMs >= tickMs) {
    prevMs = currMs;
    refresh();
  }

}

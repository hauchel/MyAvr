// learning timer1 on atmega 328
#include "helper.h"
#include <EEPROM.h>

// pins for ints
const byte pInt0 = 2;    // pd2
const byte pInt1 = 3;    // pd3

// pins for timers
const byte pcin1 = 5;    // pd5   Input Count (for wgm 14,15)
const byte picp1 = 8;    // pb0   Input Capt
const byte poc1A = 9;    // pb1
const byte poc1B = 10;   // pb2
//const byte poc2A = 11; // pb3  // reserved Tim2


// pins for debug
const byte povf = 12;    //pb4
const byte dovfS = _BV(4); // to set/reset pin fast
const byte pcap = 13;    //pb5
const byte dcapS = _BV(5);

// counts for ints, just used to see if called
byte volatile cntO; // overflow
byte volatile cntA; //
byte volatile cntB; //
byte volatile cntC; // capture
byte volatile cnt1; // int1

// Timer properties        to change
byte tim1WGM = 0;        // w  to TCCRA and TCCRB
byte tim1CS = 2;         // c  clock select to TCCRB
byte tim1COM1A = 1;      // d  compare Match Output A 0..3 to  TCCRA
byte tim1COM1B = 2;      // e  compare Match Output B 0..3 to  TCCRA
byte tim1TIMSK1 = 33;    // f  interrupt enable to TIMSK1
byte tim1IC =  2;        // m  ICNC1 and ICES1 0..3 to TCCRB
float uspt = 0.06255;    // usec per tick depending on quartz. must start with 0.0  use u to set inp * 0.0001
const uint16_t tim1presc[6] = {0, 1, 8, 64, 256, 1024};

bool volatile autoOn = false; // y to switch
uint16_t volatile ovcnt;        // overflow count -> hi word
bool volatile ferdisch = false; // measure complete
// ringbuffer for captured values 4 bytes,
typedef union {
  uint32_t za32;
  uint16_t za16[2];
  uint16_t za08[4];
} zahl_t;
const byte ripuM = 32;
zahl_t ripu[ripuM];
byte volatile ripuP = 0; // pointer, also used to stop recording when >=ripuM



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
  PORTB &= !dovfS; //debug
}

ISR(TIMER1_COMPA_vect) {
  cntA++;
}

ISR(TIMER1_COMPB_vect) {
  cntB++;
}


ISR(TIMER1_CAPT_vect) {
  if (ripuP >= ripuM) return;
  PORTB |= dcapS; //debug
  ripu[ripuP].za16[0] = ICR1;
  if ( (TIFR1 & 1 << TOV1)) {  // if pending timer verflow
    ripu[ripuP].za16[1] = ovcnt + 1;
  } else {
    ripu[ripuP].za16[1] = ovcnt;
  }
  ripuP++;
  if (ripuP >= ripuM) {
    if (autoOn)  { // no more capture ints
      ferdisch = true;
      return;
    }
    ripuP = 0;
  }
  cntC++;
  PORTB &= !dcapS; //debug
}

void int1() {
  if (ripuP >= ripuM) return;
  PORTB |= dcapS; //debug
  ripu[ripuP].za16[0] = TCNT1;
  if ( (TIFR1 & 1 << TOV1)) {  // if pending timer verflow
    ripu[ripuP].za16[1] = ovcnt + 1;
  } else {
    ripu[ripuP].za16[1] = ovcnt;
  }
  ripuP++;
  if (ripuP >= ripuM) {
    if (autoOn)  { // no more capture ints
      ferdisch = true;
      return;
    }
    ripuP = 0;
  }
  cnt1++;
  PORTB &= !dcapS; //debug
}


void writeKonfig(byte n) {
  int ea = n * 40;
  uint16_t t;
  msgF(F("write "), n);
  EEPROM.update(ea, 42); ea++;
  EEPROM.update(ea, tim1WGM); ea++;
  EEPROM.update(ea, tim1CS); ea++;
  EEPROM.update(ea, tim1COM1A); ea++;
  EEPROM.update(ea, tim1COM1B); ea++;
  EEPROM.update(ea, tim1TIMSK1); ea++;
  EEPROM.update(ea, tim1IC); ea++;
  t = OCR1A;
  EEPROM.put(ea, t); ea += 2;
  t = OCR1B;
  EEPROM.put(ea, t); ea += 2;
  EEPROM.put(ea, uspt); ea += 4;
  EEPROM.update(ea, autoOn); ea += 1;
  EEPROM.put(ea, tickMs); ea += 4;
  msgF(F("adr "), ea);
}

void readKonfig(byte n) {
  int ea = n * 40;
  uint16_t t;
  msgF(F(":read "), n);
  t = EEPROM.read(ea); ea++;
  if (t != 42) {
    msgF(F("Invalid Konfig "), t);
    return;
  }
  tim1WGM = EEPROM.read(ea); ea++;
  tim1CS = EEPROM.read(ea); ea++;
  tim1COM1A = EEPROM.read(ea); ea++;
  tim1COM1B = EEPROM.read(ea); ea++;
  tim1TIMSK1 = EEPROM.read(ea); ea++;
  tim1IC = EEPROM.read(ea); ea++;
  EEPROM.get(ea, t); ea += 2;
  msgF(F("rea OC1A "), t);
  OCR1A = t;
  EEPROM.get(ea, t); ea += 2;
  OCR1B = t;
  EEPROM.get(ea, uspt); ea += 4;
  autoOn = EEPROM.read(ea); ea++;
  EEPROM.get(ea, tickMs); ea += 4;
  msgF(F("adr "), ea);
}

float us2cs() {
  // returns duration of one tick depending on clock source
  if (tim1CS < 6) {
    return uspt * tim1presc[tim1CS];
  }
  return 0.0;
}
void showCnts() {
  char str[80];
  sprintf(str, "O:%4u A:%4u B:%4u C:%4u 1:%4u", cntO, cntA, cntB, cntC, cnt1);
  Serial.println(str);
}

void showRegs() {
  char str[80];
  sprintf(str, "TCRA: 0x%02X  TCRB: 0x%02X  TIMSK1: 0x%02X", TCCR1A, TCCR1B, TIMSK1);
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

void explainRegs() {
  char str[80];
  byte t, wgm, comA, comB, cs, ica;
  t = TCCR1A;
  wgm = t & 3;
  comA = t  >> 6;
  comB = (t >> 4) & 3;
  t = TCCR1B;
  ica = t >> 6;
  cs = t & 7;
  t = t >> 1;
  wgm |= t & 12;
  sprintf(str, "WGM %3u, CS %3u, COM1A %2u, COM1B %2u, ICNC1/ICES1 %2u", wgm, cs, comA, comB, ica);
  Serial.println(str);
}

void showRipu() {
  char str[120];
  float zwi = us2cs();
  float dauer;
  long delt;

  sprintf(str, "  P %2u  auto %1u   uspt ",  ripuP, autoOn);
  Serial.print(str);
  Serial.print(uspt, 6); //sprintf doesn't like floats
  Serial.print("  clo ");
  Serial.println(zwi, 6);

  Serial.println(F(" #   Hi     Lo        long       ticks       dur (ms) "));

  for (byte i = 0; i < ripuM; i++ ) {
    if (i == 0) {
      delt = 0;
    } else {
      delt =  ripu[i].za32 - ripu[i - 1].za32;
    }
    sprintf(str, "%2u %4u %6u  %10lu  %10ld  ", i, ripu[i].za16[1], ripu[i].za16[0], ripu[i].za32, delt);
    Serial.print(str);
    if (i == 0) {
      Serial.println();
    } else {
      dauer = zwi * delt / 1000;
      dtostrf(dauer, 10, 3, str);
      Serial.println(str);
    }
  } // next
}
void help () {
  Serial.println (F("_a  set OCR1A "));
  Serial.println (F("_b  set OCR1B "));
  Serial.println (F("_c  set clockselect 0, 1, 8, 64, 256, 1024, ext, ext"));
  Serial.println (F("_d  set COM1A (Pin 9) 0..3, 0 always not used"));
  Serial.println (F("_e  set COM1B (Pin 10)0..3, 0 always not used"));
  Serial.println (F("    Non-PWM (0,4,12): 1 toggle, 2 clear, 3 set"));
  Serial.println (F("    Fast PWM (14,15): 1 toggle OC1A OC1B normal, 2 clear match set BOT, 3 inv"));
  Serial.println (F("    Fast PWM (5,6,7): 1 toggle, 2 clear match set BOT, 3 inv"));
  Serial.println (F("    PWM (9,11):       1 toggle, 2 clear up, set down, 3 inv"));
  Serial.println (F("    PWM(1,2,3,8,10):  1 toggle, 2 clear up, set down, 3 inv"));
  Serial.println (F("_f  set interrupts:  32 ICIE  4 OCIEB, 2 OCIEA, 1 TOIE"));
  Serial.println (F("i   init Timer (also called after sets c..f above"));
  Serial.println (F("_k  read Konfig (0 to 10)"));
  Serial.println (F("_K  write Konfig (0 to 10)"));
  Serial.println (F("_m  set ICNC1 2 and ICES1 1 (0 to 3)"));
  Serial.println (F("o   force Output compare 128+64 to TCCR1C"));
  Serial.println (F("p   Pinmode outputs"));
  Serial.println (F("r   set low"));
  Serial.println (F("s   set high"));
  Serial.println (F("_t  tick time in ms (0 to switch off)"));
  Serial.println (F("_u  set us per tick post-decs (e.g. 1234u means 0.01234"));
  Serial.println (F("_w  set wgm 0..15"));
  Serial.println (F("x   eXec one capture"));
  Serial.println (F("y   toggle auto-capture"));
  Serial.println (F("z   show Ripu"));
}

void doCmd(char x) {
  Serial.print(char(x));
  if (doNum(x)) {
    return;
  }
  Serial.println();
  switch (x) {
    case 13:
      vt100Clrscr();
      break;
    case ' ':
      showCnts();
      break;
    case 'a':
      OCR1A = inp;
      msgF(F(":OCR1A"), OCR1A);
      break;
    case 'b':
      OCR1B = inp;
      msgF(F(":OCR1B"), OCR1B);
      break;
    case 'c':
      tim1CS = inp & 7;
      timer1Init();
      msgF(F(":tim1CS"), tim1CS);
      break;
    case 'd':
      tim1COM1A = inp & 3;
      timer1Init();
      msgF(F(":tim1COM1A"), tim1COM1A);
      break;
    case 'e':
      tim1COM1B = inp & 3;
      timer1Init();
      msgF(F(":tim1COM1B"), tim1COM1B);
      break;
    case 'f':
      tim1TIMSK1 = inp ;
      timer1Init();
      msgF(F(":tim1TIMSK1"), tim1TIMSK1);
      break;
    case 'h':
      help();
      break;
    case 'i':
      timer1Init();
      explainRegs();
      msgF(F("TCCR1B"), TCCR1B);
      msgF(F("TIMSK1"), TIMSK1);
      break;
    case 'j':
      //explainRegs();
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
    case 'm':
      tim1IC = inp & 3;
      timer1Init();
      msgF(F(":tim1IC"), tim1IC);
      break;
    case 'o':
      TCCR1C = 128 + 64;
      msgF(F("FOC"), TCCR1C);
      break;
    case 'p':
      pinMode(poc1A, OUTPUT);
      pinMode(poc1B, OUTPUT);
      msgF(F("Pinmodes"), 0);
      break;
    case 'r':
      digitalWrite(poc1A, LOW);
      digitalWrite(poc1B, LOW);
      msgF(F("Set to "), 0);
      break;
    case 's':
      digitalWrite(poc1A, HIGH);
      digitalWrite(poc1B, HIGH);
      msgF(F("Set to"), 1);
      break;
    case 't':   //
      tickMs = inp;
      msgF(F("tickMs"), inp);
      break;
    case 'u': //
      msgF(F(" to uspt"), inp);
      uspt = 0.00001 * inp;
      Serial.println(uspt, 5);
      break;
    case 'v':
      break;
    case 'w':
      tim1WGM = inp & 15;
      timer1Init();
      msgF(F("wgm"), tim1WGM);
      break;
    case 'x': //start in auto
      ovcnt = 0; // to get convenient values
      ripuP = 0;
      break;
    case 'y':
      autoOn = !autoOn;
      msgF(F("Auto"), autoOn);
      break;
    case 'z':
      showRipu();
      break;
    default:
      Serial.print('?');
      Serial.println(int(x));
  } // case
}


void setup() {
  const char info[] = "pinchng " __DATE__  " "  __TIME__;
  Serial.begin(38400);
  Serial.println(info);
  pinMode(pInt0, INPUT_PULLUP);
  pinMode(pInt1, INPUT_PULLUP);
  pinMode(picp1, INPUT_PULLUP);
  pinMode(pcin1, INPUT_PULLUP);

  pinMode(poc1A, OUTPUT);
  pinMode(poc1B, OUTPUT);
  pinMode(povf, OUTPUT);
  pinMode(pcap, OUTPUT);

  OCR1A = 100;
  OCR1B = 60;
  tickMs = 1000;
  readKonfig(0);
  timer1Init();
  attachInterrupt(1, int1, RISING);
}

void loop() {
  if (Serial.available() > 0) {
    doCmd( Serial.read());
  }

  if (ferdisch) {
    ferdisch = false;
    showRipu();
  }
  currMs = millis();
  if (tickMs > 0) {
    if ((currMs - prevMs >= tickMs) ) {
      doCmd('x');
      prevMs = currMs;
    }
  }
}

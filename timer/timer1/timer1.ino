// learning timer1 on atmega 328
// and DAC-Out on LGT8

#define LGT8

#include "helper.h"
#include <EEPROM.h>

// DAC and LCD

// pins for timers   A-Tännschen on LGT8 SSOP20: 5 is RX, 8/9 and 10/11 are same Pin
//const byte poc2B = 3;  // pd3   // reserved Tim2
const byte pcin1 = 5;   // pd5   Input Count (for wgm 14,15)
const byte picp1 = 8;   // pb0   Input Capt
const byte poc1A = 9;   // pb1   Out A (set to toggle with 1d)
const byte poc1B = 10;  // pb2   Out B (set to toggle with 1e)
//const byte poc2A = 11; // pb3  // reserved Tim2

// pins for debug
const byte povf = 12;       //pb4
const byte dovfS = _BV(4);  // to set/reset pin fast
const byte pcap = 13;       //pb5
const byte dcapS = _BV(5);

// counts for ints, just used to see if called
byte volatile cntO;  // overflow
byte volatile cntA;  //
byte volatile cntB;  //
byte volatile cntC;  // capture

// Timer properties        to change
byte tim1WGM = 0;      // w  to TCCRA and TCCRB
byte tim1CS = 2;       // c  clock select to TCCRB
byte tim1COM1A = 1;    // d  compare Match Output A 0..3 to  TCCRA
byte tim1COM1B = 2;    // e  compare Match Output B 0..3 to  TCCRA
byte tim1TIMSK1 = 33;  // f  interrupt enable to TIMSK1
byte tim1IC = 2;       // m  ICNC1 and ICES1 0..3 to TCCRB
#ifdef LGT8
float uspt = 0.03118;  // usec per tick= 1/quartz[MHz]. must start with 0.0  use u to set inp * 0.0001
#else
float uspt = 0.06255;
#endif
uint16_t ufak = 8;  // frequency dep on Kurvenform
const uint16_t tim1presc[6] = { 0, 1, 8, 64, 256, 1024 };
const byte notMax = 12;
//                          c 0               d 2               e 4      f 5               g 7               a 9               h
float tonFreq[notMax] = { 523.251, 554.365, 587.330, 622.254, 659.255, 698.456, 739.989, 783.991, 830.609, 880.000, 932.328, 987.767 };
uint16_t oca[notMax];

bool volatile autoOn = false;  // y to switch
uint16_t volatile ovcnt;
bool volatile ferdisch = false;  // measure complete
// ringbuffer for captured values
typedef union {
  uint32_t za32;
  uint16_t za16[2];
} zahl_t;
const byte ripuM = 32;
zahl_t ripu[ripuM];
byte volatile ripuP = 0;  // pointer, also used to stop recording when >=ripuM

#ifdef LGT8
#include <Wire.h>

byte table[256];  // DAC values indexed by byte

void setAnaRef(byte n) {
  switch (n) {
    case 0:
      analogReference(DEFAULT);
      msgF(F("Ref Def"), n);
      break;
    case 1:
      analogReference(EXTERNAL);
      msgF(F("Ref EXTERNAL"), n);
      break;
    case 2:
      analogReference(INTERNAL1V024);
      msgF(F("Ref INTERNAL1V024"), n);
      break;
    case 3:
      analogReference(INTERNAL2V048);
      msgF(F("Ref INTERNAL2V048"), n);
      break;
    case 4:
      analogReference(INTERNAL4V096);
      msgF(F("Ref INTERNAL4V096"), n);
      break;
    default:
      msgF(F("Ref ?"), n);
  }  // case
}

void kurven(byte n) {
  switch (n) {
    case 0:
      for (int i = 0; i < 256; i++) {
        table[i] = (1 + sin((float)i * TWO_PI / 256)) / 2 * 255;
      }
      ufak = 128;
      msgF(F("Sinus 256"), n);
      break;
    case 1:
      for (int i = 0; i < 128; i++) {
        table[i] = (1 + sin((float)i * TWO_PI / 128)) / 2 * 255;
        table[i + 128] = table[i];
      }
      ufak = 64;
      msgF(F("Sinus 128"), n);
      break;
    case 2:
      for (int i = 0; i < 256; i++) {
        table[i] = i;
      }
      ufak = 128;
      msgF(F("Ramp 256"), n);
      break;
    case 3:
      for (int i = 0; i < 128; i++) {
        table[i] = i * 2;
        table[i + 128] = table[i];
      }
      ufak = 64;
      msgF(F("Ramp 128"), n);
      break;

    default:
      msgF(F("Kurven ?"), n);
  }  // case
}
#endif



void timer1Init() {
  TCCR1B = 0;  // stop timer by setting CS to 0
  TIMSK1 = 0;  // no timer ints during init
  TCNT1 = 0;
  ovcnt = 0;
  //   7      6        5       4        3       2       1       0
  // COM1A1 COM1A0   CO1MB1  COM1B0     –       –      WGM11   WGM10    TCCR1A
  //    cmoA            cmoB        0       0       w       w
  TCCR1A = (tim1COM1A << 6) | (tim1COM1B << 4) | (tim1WGM & 3);
  //                                        OCIE1B  OCIE1A  TOIE1   TIMSK1
  // 0      0       0       0       0       o       o       o
  TIMSK1 = tim1TIMSK1;  // Timer interrupts
  // ICN1  ICES1    –      WGM13    WGM12   CS22    CS21    CS20    TCCR1B
  // i      i       0       w       w        c       c       c
  TCCR1B = (tim1IC << 6) | tim1CS | ((tim1WGM << 1) & 0x18);
}

ISR(TIMER1_OVF_vect) {
  PORTB |= dovfS;  //debug
  cntO++;
  ovcnt++;
  PORTB &= !dovfS;  //debug
}

ISR(TIMER1_COMPA_vect) {
  cntA++;
}

ISR(TIMER1_COMPB_vect) {
#ifdef LGT8
  analogWrite(DAC0, table[cntB]);
#endif
  cntB++;
}

ISR(TIMER1_CAPT_vect) {
  if (ripuP >= ripuM) return;
  PORTB |= dcapS;  //debug
  ripu[ripuP].za16[0] = ICR1;
  if ((TIFR1 & 1 << TOV1)) {  // if pending timer verflow
    ripu[ripuP].za16[1] = ovcnt + 1;
  } else {
    ripu[ripuP].za16[1] = ovcnt;
  }
  ripuP++;
  if (ripuP >= ripuM) {
    if (autoOn) {  // no more capture ints
      ferdisch = true;
      return;
    }
    ripuP = 0;
  }
  cntC++;
  PORTB &= !dcapS;  //debug
}

void writeKonfig(byte n) {
  int ea = n * 40;
  uint16_t t;
  msgF(F("write "), n);
  EEPROM.update(ea, 42);
  ea++;
  EEPROM.update(ea, tim1WGM);
  ea++;
  EEPROM.update(ea, tim1CS);
  ea++;
  EEPROM.update(ea, tim1COM1A);
  ea++;
  EEPROM.update(ea, tim1COM1B);
  ea++;
  EEPROM.update(ea, tim1TIMSK1);
  ea++;
  EEPROM.update(ea, tim1IC);
  ea++;
  t = OCR1A;
  EEPROM.put(ea, t);
  ea += 2;
  t = OCR1B;
  EEPROM.put(ea, t);
  ea += 2;
  EEPROM.put(ea, uspt);
  ea += 4;
  EEPROM.update(ea, autoOn);
  ea += 1;
  EEPROM.put(ea, tickMs);
  ea += 4;
  msgF(F("adr "), ea);
}

void readKonfig(byte n) {
  int ea = n * 40;
  uint16_t t;
  msgF(F(":read "), n);
  t = EEPROM.read(ea);
  ea++;
  if (t != 42) {
    msgF(F("Invalid Konfig "), t);
    return;
  }
  tim1WGM = EEPROM.read(ea);
  ea++;
  tim1CS = EEPROM.read(ea);
  ea++;
  tim1COM1A = EEPROM.read(ea);
  ea++;
  tim1COM1B = EEPROM.read(ea);
  ea++;
  tim1TIMSK1 = EEPROM.read(ea);
  ea++;
  tim1IC = EEPROM.read(ea);
  ea++;
  EEPROM.get(ea, t);
  ea += 2;
  msgF(F("rea OC1A "), t);
  OCR1A = t;
  EEPROM.get(ea, t);
  ea += 2;
  OCR1B = t;
  EEPROM.get(ea, uspt);
  ea += 4;
  autoOn = EEPROM.read(ea);
  ea++;
  EEPROM.get(ea, tickMs);
  ea += 4;
  msgF(F("adr "), ea);
}

float us2cs() {
  // returns duration of one tick depending on clock source
  if (tim1CS < 6) {
    return uspt * tim1presc[tim1CS];
  }
  return 0.0;
}

float ctcFreq() {
  // CTC Mode Frequency
  float dau = 2 * us2cs() * (OCR1A + 1);
  return 1000000.0 / dau;
}

float calcNote(byte n) {
  // calc OCRA depending on settings for note
  if (n < notMax) {
    float a = 2 * us2cs() * ufak * tonFreq[n];
    if (a > 0) return 1000000.0 / a;
  }
  return -1.0;
}

void playNote(byte n) {
  if (n < notMax) {
    OCR1A = oca[n];
  } else {
    msgF(F("Invalid Note"), n);
  }
}

void buildoca() {
  if (us2cs() != 0) {
    for (byte i = 0; i < notMax; i++) {
      oca[i] = round(calcNote(i));
    }
  } else {
    msgF(F("No build, us2cs is"), 0);
  }
}

void showCnts() {
  char str[80];
  sprintf(str, "O:%4u A:%4u B:%4u C:%4u ", cntO, cntA, cntB, cntC);
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
  msgF(F("_u ufak    "), ufak);
  Serial.print(uspt, 6);
  Serial.print(F(" = "));
  Serial.println(ctcFreq());
  buildoca();
}

void explainRegs() {
  char str[80];
  byte t, wgm, comA, comB, cs, ica;
  t = TCCR1A;
  wgm = t & 3;
  comA = t >> 6;
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
  char strd[20];
  char strf[20];
  long delt, avg;
  long dif;
  float zwi = us2cs();
  float dauer;

  // average is
  avg = ripu[ripuM - 1].za32 - ripu[2].za32;
  avg = avg / (ripuM - 3);
  sprintf(str, "  P %2u  auto %1u  avg %10ld   uspt ", ripuP, autoOn, avg);
  Serial.print(str);
  Serial.print(uspt, 6);  //sprintf doesn't like floats
  Serial.print("  clo ");
  Serial.print(zwi, 6);
  dauer = zwi * avg;
  dauer = 1000000.0 / dauer;  // freq
  Serial.print("    ");
  Serial.println(dauer, 3);

  Serial.println(F(" #   Hi     Lo        long       ticks      abw     dur (us)       freq"));

  for (byte i = 0; i < ripuM; i++) {
    if (i == 0) {
      delt = 0;
      dif = 0;
    } else {
      delt = ripu[i].za32 - ripu[i - 1].za32;
      dif = delt - avg;
    }  //
    sprintf(str, "%2u %4u %6u  %10lu  %10ld  %+7ld   ", i, ripu[i].za16[1], ripu[i].za16[0], ripu[i].za32, delt, dif);
    if (i == 0) {
      Serial.println(str);
    } else {
      Serial.print(str);
      dauer = zwi * delt;
      dtostrf(dauer, 9, 3, strd);
      dauer = 1000000.0 / dauer;  // freq
      dtostrf(dauer, 9, 3, strf);
      sprintf(str, " %s  %s", strd, strf);
      Serial.println(str);
    }
  }
}

void coKos() {
  Serial.println(F("Common Konfigs "));
  Serial.println(F("Frequency Generator CTC on 9 (pOC1A) or 10 (pOC1B) "));
  Serial.println(F("   0f 1de 4w 1c "));
  Serial.println(F("   at 32MHz:  9a=200KHz, 99a=20KHz 20000a=800Hz"));
  Serial.println(F("_d  set COM1A (Pin 9) 0..3, 0 always not used"));
}

void help() {
  Serial.println(F("_a  set OCR1A "));
  Serial.println(F("_b  set OCR1B "));
  Serial.println(F("_c  set clockselect 0, 1, 8, 64, 256, 1024, ext, ext"));
  Serial.println(F("_d  set COM1A (Pin 9) 0..3, 0 always not used"));
  Serial.println(F("_e  set COM1B (Pin 10)0..3, 0 always not used"));
  Serial.println(F("    Non-PWM (0,4,12): 1 toggle, 2 clear, 3 set"));
  Serial.println(F("    Fast PWM (14,15): 1 toggle OC1A OC1B normal, 2 clear match set BOT, 3 inv"));
  Serial.println(F("    Fast PWM (5,6,7): 1 toggle, 2 clear match set BOT, 3 inv"));
  Serial.println(F("    PWM (9,11):       1 toggle, 2 clear up, set down, 3 inv"));
  Serial.println(F("    PWM(1,2,3,8,10):  1 toggle, 2 clear up, set down, 3 inv"));
  Serial.println(F("_f  set interrupts:  32 ICIE  4 OCIEB, 2 OCIEA, 1 TOIE"));
  Serial.println(F("i   init Timer (also called after sets c..f above"));
  Serial.println(F("_k  read Konfig (0 to 10)"));
  Serial.println(F("_K  write Konfig (0 to 10)"));
  Serial.println(F("_m  set ICNC1 2 and ICES1 1 (0 to 3)"));
  Serial.println(F("o   force Output compare 128+64 to TCCR1C"));
  Serial.println(F("p   Pinmode outputs"));
  Serial.println(F("r   set low"));
  Serial.println(F("s   set high"));
  Serial.println(F("_t  tick time in ms"));
  Serial.println(F("_U  set us per tick post-decs (e.g. 1234U means 0.01234"));
  Serial.println(F("_w  set wgm 0..15"));
  Serial.println(F("x   eXec one capture"));
  Serial.println(F("y   toggle auto-capture"));
  Serial.println(F("z   show Ripu"));
  Serial.println(F("_n  play note (set OCR1A"));
  Serial.println(F("_N  calc note"));
  Serial.println(F("_q  out DAC"));
  Serial.println(F("_l  AnaRef 0..4"));
  Serial.println(F("_g  Kurvenform"));
  Serial.println(F("_u  ufak"));
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
      tim1TIMSK1 = inp;
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
      msgF(F("Pinmodes OUTPUT"), 0);
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
    case 't':  //
      tickMs = inp;
      msgF(F("tickMs"), inp);
      break;
    case 'u':  //
      msgF(F("uFak"), inp);
      uspt = 0.00001 * inp;
      Serial.println(uspt, 5);
      break;
    case 'U':  //
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
    case 'x':     //start in auto
      ovcnt = 0;  // to get convenient values
      ripuP = 0;
      break;
    case 'y':
      autoOn = !autoOn;
      msgF(F("Auto"), autoOn);
      break;
    case 'z':
      showRipu();
      break;
    case '+':
      OCR1A += inp;
      msgF(F("OCRA"), OCR1A);
      break;
    case '-':
      OCR1A -= inp;
      msgF(F("OCRA"), OCR1A);
      break;
#ifdef LGT8
    case 'g':
      kurven(inp);
      break;
    case 'n':
      playNote(inp);
      break;
    case 'N':
      Serial.print("Note");
      Serial.println(calcNote(inp));
      break;
    case 'q':
      analogWrite(DAC0, inp);
      msgF(F("DAC"), inp);
      break;
    case 'l':
      setAnaRef(inp);
      break;
#endif
    default:
      Serial.print('?');
      Serial.println(int(x));
  }  // case
}


void setup() {
  const char info[] = "timer1 " __DATE__ " " __TIME__;
  Serial.begin(38400);
  Serial.println(info);

  pinMode(picp1, INPUT_PULLUP);
  pinMode(pcin1, INPUT_PULLUP);
  pinMode(poc1A, OUTPUT);
  pinMode(poc1B, OUTPUT);
  pinMode(povf, OUTPUT);
  pinMode(pcap, OUTPUT);

  OCR1A = 100;
  OCR1B = 60;
  tickMs = 0;
  tim1COM1B = 0;

  readKonfig(0);
  timer1Init();
#ifdef LGT8
  Serial.println(F("ONLY on LGT8FX!"));
  setAnaRef(3);
  pinMode(DAC0, ANALOG);
  analogWrite(DAC0, 125);
  kurven(0);
#endif
}

void loop() {
  if (Serial.available() > 0) {
    doCmd(Serial.read());
  }

  if (ferdisch) {
    ferdisch = false;
    showRipu();
  }
  currMs = millis();
  if (tickMs > 0) {
    if ((currMs - prevMs >= tickMs)) {
      doCmd('x');
      prevMs = currMs;
    }
  }
}

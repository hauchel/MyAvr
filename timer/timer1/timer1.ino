// learning timer1
// capture

#include "helper.h"

const byte poc1A = 9; // pb1
const byte poc1B = 10;  // pb2
const byte picp1 = 8; // pb0  //Input
//const byte poc2A = 11; // pb3
//const byte poc2B = 3;  // pd3

byte volatile cntO; // counts for ints
byte volatile cntA; //
byte volatile cntB; //
byte volatile cntC; // capture

byte tim1cs = 3;    // c  clock select, 6 = /256 to TCCRB
byte tim1cmoA = 3;  // d  compare Match Output A 0..3 to  TCCRA
byte tim1cmoB = 3;  // e  compare Match Output B 0..3 to  TCCRA
byte tim1ocie = 0;  // f  interrupt enable to TIMSK1
byte tim1wgm = 15;  // w  to TCCRA and TCCRB
byte tim1ica =  2;  // m  ICNC1 and ICES1 0..3 to TCCRB

bool autoOn = false; //

void timer1Init() {
  byte wgmh = (tim1wgm << 1) & 0x18;
  // need to set globals before calling!
  // Timer for fast pwm mode 7: counts to OCR2A, int at OCR2B and OCR2A
  // 7      6       5       4       3       2       1       0
  // COMA1 COMA0   COMB1  COMB0    –       –       WGM1   WG20    TCCR1A
  //    cmoA            cmoB        0       0       w       w
  TCCR1A =  (tim1cmoA << 6) | (tim1cmoB << 4) | (tim1wgm & 3);
  // ICN1  ICES1    –      WGM13    WGM12   CS22    CS21    CS20    TCCR1B
  // i      i       0       w       w        c       c       c
  TCCR1B = (tim1ica << 6) | tim1cs | wgmh;
  //                                        OCIE2B  OCIE2A  TOIE2   TIMSK1
  // 0      0       0       0       0       o       o       o
  TIMSK1 = tim1ocie;                                    // enable Timer interrupts
  sei();  //set global interrupt flag to enable interrupts
}

ISR(TIMER1_OVF_vect) {
  cntO++;
}

ISR(TIMER1_COMPA_vect) {
  cntA++;
}

ISR(TIMER1_COMPB_vect) {
  cntB++;
}

ISR(TIMER1_CAPT_vect) {
  cntC++;
}

void showCnts() {
  char str[80];
  sprintf(str, "O:%4u A:%4u B:%4u C:%4u ", cntO, cntA, cntB, cntC);
  Serial.println(str);
}

void showRegs() {
  char str[80];
  sprintf(str, "OCR1A:%8u OCR1B:%8u TCRA:%4u TCRB: %4u TIMSK1: %4u", OCR1A, OCR1B, TCCR1A, TCCR1B, TIMSK1);
  Serial.println(str);
  //  sprintf(str, "OCR1AL:%4u OCR1AH:%4u OCR1BL:%4u OCR1BH:%4u TCNT1L: %4u TCNT1H: %4u", OCR1AL, OCR1AH, OCR1BL, OCR1BH, TCNT1L, TCNT1H);
  //  Serial.println(str);
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
  sprintf(str, "wgm %3u, cs %3u, comA %2u, comB %2u, ica %2u", wgm, cs, comA, comB, ica);
  Serial.println(str);
}

void help () {
  Serial.println (F("Running in Fast PWM wgm=15"));
  Serial.println (F("_a  set OCR1A to change freq"));
  Serial.println (F("_b  set OCR1B (<OCR1A) to change "));
  Serial.println (F("_c  set clockselect 0 to 7"));
  Serial.println (F("_d  set COM1A (Pin 9) to 0 not, 1 Toggle, 2 Clear Match, set BOT, 3 invert"));
  Serial.println (F("_e  set COM1B (Pin 10)  "));
  Serial.println (F("    Norm or CTC:   0 not, 1 toggle, 2 clear, 3 set"));
  Serial.println (F("    Fast PWM:      0 not, 1 toggle, 2 clear match set BOT, 3 inv"));
  Serial.println (F("_f  set interrupts:  32 ICIE  4 OCIEB, 2 OCIEA, 1 TOIE"));
  Serial.println (F("i   init Timer (also called after sets c..f above"));
  Serial.println (F("_m  set ICNC1 2 and ICES1 1 (0 to 3)"));
  Serial.println (F("o   force Output compare 128+64 to TCCR1C"));
  Serial.println (F("p   Pinmode outputs"));
  Serial.println (F("r   set low"));
  Serial.println (F("s   set high"));
  Serial.println (F("_t  tick time in ms"));
  Serial.println (F("_w  set wgm 0..15"));
  Serial.println (F("x   eXec one capture"));
  Serial.println (F("y   toggle auto-capture"));
}

void doCmd(char x) {
  uint16_t tmpu;
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
      showRegs();
      break;
    case 'a':
      OCR1A = inp;
      tmpu = OCR1A;
      Serial.println(tmpu);
      msgF(F(":OCR1A"), OCR1A);
      break;
    case 'b':
      OCR1B = inp;
      msgF(F(":OCR1B"), OCR1B);
      break;
    case 'c':
      tim1cs = inp & 7;
      timer1Init();
      msgF(F(":tim1cs"), tim1cs);
      break;
    case 'd':
      tim1cmoA = inp & 3;
      timer1Init();
      msgF(F(":tim1cmoA"), tim1cmoA);
      break;
    case 'e':
      tim1cmoB = inp & 3;
      timer1Init();
      msgF(F(":tim1cmoB"), tim1cmoB);
      break;
    case 'f':
      tim1ocie = inp ;
      timer1Init();
      msgF(F(":tim1ocie"), tim1ocie);
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
    case 'u': //Test
      OCR1A = 0xAAAA;
      break;
    case 'v': //Test
      OCR1A = 1024;
      break;
    case 'w':
      tim1wgm = inp & 15;
      timer1Init();
      msgF(F("wgm"), tim1wgm);
      break;
    case 'x': //ICP
      break;
    case 'y':
      autoOn = !autoOn;
      msgF(F("Auto"), autoOn);
      break;
    default:
      Serial.print('?');
      Serial.println(int(x));
  } // case
}


void setup() {
  const char info[] = "timer1 " __DATE__  " "  __TIME__;
  Serial.begin(38400);
  Serial.println(info);
  pinMode(poc1A, OUTPUT);
  pinMode(poc1B, OUTPUT);
  pinMode(picp1, INPUT_PULLUP);
  OCR1A = 100;
  OCR1B = 60;
  timer1Init();
  tickMs = 1000;
}

void loop() {
  if (Serial.available() > 0) {
    doCmd( Serial.read());
  }
  currMs = millis();
  if (currMs - prevMs >= tickMs) {
    prevMs = currMs;
    if (autoOn)  {};
  } // tick
}

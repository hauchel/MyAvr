// learning adc on atmega 328
// Timer1 wgm 15, Compare Match B rising triggers ADC, OCR1A determines frequency
// Output pstart is switched
#include "helper.h"

// pins for timers
//const byte poc2B = 3;  // pd3   // reserved Tim2
const byte pcinp = 5;    // pd5   Input (also Count for CS 6,7)
const byte pstart = 7;   // pd5   goes High
const byte picp1 = 8;    // pb0   Input Capt
const byte poc1A = 9;    // pb1
const byte poc1B = 10;   // pb2
//const byte poc2A = 11; // pb3  // reserved Tim2

// pins for debug
const byte povf = 12;    //pb4
const byte dovfS = _BV(4); // to set/reset pin fast
const byte dovfC = dovfS  ^ 0xFF;
const byte pcap = 13;    //pb5
const byte dcapS = _BV(5);
const byte dcapC = dcapS  ^ 0xFF;

// counts for ints, just used to see if called
byte volatile cntO; // overflow
byte volatile cntA; //
byte volatile cntB; //
byte volatile cntC; // capture
byte volatile cntD; // aDc

// Timer properties        to change
byte tim1WGM = 0;        // w  to TCCRA and TCCRB
byte tim1CS = 2;         // c  clock select to TCCRB
byte tim1COM1A = 1;      // d  compare Match Output A 0..3 to  TCCRA
byte tim1COM1B = 1;      // e  compare Match Output B 0..3 to  TCCRA
byte tim1TIMSK1 = 0 ;    // f  interrupt enable to TIMSK1
byte tim1IC =  0;        // m  ICNC1 and ICES1 0..3 to TCCRB
float uspt = 0.06255;    // u  usec per tick depending on quartz. must start with 0.0  use u to set inp * 0.0001
const uint16_t tim1presc[6] = {0, 1, 8, 64, 256, 1024};

bool volatile oneShot = false; // o to switch
bool volatile fire = false; // y to switch
uint16_t volatile ovcnt;
byte volatile anOnP, anOffP;  // pointer to switch off
byte anOnSav;                 // as anon ist cleared
bool volatile ferdisch = false; // measure complete

// ringbuffer for captured values
const byte ripuM = 20;
uint16_t ripuAdc[ripuM];
bool ripuLis  [ripuM];
byte volatile ripuP = 0; // pointer, also used to stop recording when >=ripuM
byte ripuOld = 0;       //
byte showCnt = 0;       // for CR/LF in verbo 2
bool ripuLisOld = 0;
byte verbo = 0; //

uint16_t anzH, anzL;
uint16_t limH = 5;
uint32_t sumH, sumL;
byte durchl;
bool durchOdd;

byte anMUX = 1;   // admux 0..7,8,14,15
byte anPS = 3;    // adps  0..7
byte anTS = 5;    // adts Trigger source Timer1 MatchB

void adcInit() {
  //   7      6        5       4        3       2       1       0
  // REFS1   REFS0    ADLAR     -                ADMUX
  ADMUX = (1 << REFS0) | (0 << ADLAR) | anMUX; // AVcc , right adj,
  // ADEN    ADSC     ADATE   ADIF      ADIE   ADPS2   ADPS1   ADPS0
  ADCSRA = (1 << ADEN) |  (1 << ADIF) |  (1 << ADATE) |  (1 << ADIE) | anPS; //
  //                                                   ADTS
  ADCSRB = anTS;
  DIDR0 = _BV(anMUX); // disable Digital input
}

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
}

ISR(TIMER1_CAPT_vect) {
  cntC++;
}

ISR(ADC_vect) {
  uint16_t tmp, mess;
  PORTB |= dcapS; //debug
  if (ripuP >= ripuM) {
    PORTB &= dcapC; //debug
    return;
  }
  cntD++;

  // ADCL must be read first, then ADCH.
  mess = ADCL;
  tmp = ADCH;
  ripuLis[ripuP] = !digitalRead(pcinp); // Lichtschranke
  mess |= (tmp << 8);
  if (anOffP == ripuP) {
    digitalWrite(pstart, LOW);
  }
  if (anOnP == ripuP) {
    digitalWrite(pstart, HIGH);
    anOnP = 0xFF;
  }

  ripuAdc[ripuP] = mess;
  ripuP++;
  if (ripuP >= ripuM) {
    if (oneShot)  { // no more capture ints
      digitalWrite(pstart, LOW);  // zur Sicherheit
      ferdisch = true;
      return;
    }
    ripuP = 0;
  }
  PORTB &= dcapC; //debug
}

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
  sprintf(str, "ADMUX: 0x%02X  ADCSRA: 0x%02X  ADCSRB: 0x%02X  DIDR0: 0x%02X", ADMUX, ADCSRA, ADCSRB, DIDR0);
  Serial.println(str);

  msgF(F("_a OCR1A   "), OCR1A);
  msgF(F("_b OCR1B   "), OCR1B);
  msgF(F("_c CS      "), tim1CS);
  msgF(F("_d COM1A   "), tim1COM1A);
  msgF(F("_e COM1B   "), tim1COM1B);
  msgF(F("_f TIMSK1  "), tim1TIMSK1);
  msgF(F("_m ICNC1/ICES1 "), tim1IC);
  msgF(F("_w WGM     "), tim1WGM);
  msgF(F("_l ADMUX   "), anMUX);
  msgF(F("_p ADPS    "), anPS);
}

void showRipu() {
  char str[120];
  float zwi = us2cs();
  float dauer;
  vt100Clrscr();
  sprintf(str, "P %3u  auto %1u  set %2u (%3u)  reset %2u uspt ",  ripuP, oneShot, anOnSav, anOnP, anOffP);
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
    sprintf(str, "%2u %4u  %2u", i, ripuAdc[i], ripuLis[i]);
    Serial.println(str);
  }
}

void doFire() {
  // set
  digitalWrite(pstart, HIGH);
  Serial.print("*");
  anOffP = ripuP + 2;
  if (anOffP > ripuM) anOffP -= ripuM;
}

void evalMess() {
  // shows entry at ripuOld
  char str[40];
  uint16_t anz, avg;
  uint32_t sum;
  uint16_t mess = ripuAdc[ripuOld];
  PORTB |= dovfS; //debug
  if (ripuLisOld != ripuLis[ripuOld]) { // Lischra change
    if (ripuLisOld == 1) { //switching to 0
      durchl += 1;
      anzL = 0;
      anz = anzH;
      sumL = 0;
      sum = sumH;
      avg = sum / anz;
      // eval previos 1
      if (anz > limH) { // only fire if not too fast
        if ( (durchl & 0x01) == 0) {
          if (!durchOdd) {
            doFire();
          }
        } else {
          if (durchOdd) {
            doFire();
          }
        } 
      }  // anz>limH
    } else { // switch to 1
      anzH = 0;
      anz = anzL;
      sumH = 0;
      sum = sumL;
      avg = sum / anz;
    }
    ripuLisOld = ripuLis[ripuOld];

    if (verbo > 0) { //summary
      sprintf(str, "du %3u an %3u av %4u", durchl, anz, avg);
      Serial.println(str);
      Serial.print(ripuLisOld);
      showCnt = 0;
    }
  } //change
  if (ripuLisOld == 1) { //old is current
    anzH += 1;
    sumH += mess;
  } else {
    anzL += 1;
    sumL += mess;
  }
  if (verbo == 2) {
    sprintf(str, "%5u", mess);
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
  Serial.println (F("_a  set OCR1A "));
  Serial.println (F("_b  set OCR1B "));
  Serial.println (F("_c  set clockselect 0, 1, 8, 64, 256, 1024, ext, ext"));
  Serial.println (F("_d  set COM1A (Pin 9) 0..3, 0 always not used"));
  Serial.println (F("_e  set COM1B (Pin 10)0..3, 0 always not used"));
  Serial.println (F("    Fast PWM (14,15): 1 toggle OC1A OC1B normal, 2 clear match set BOT, 3 inv"));
  Serial.println (F("    Fast PWM (5,6,7): 1 toggle, 2 clear match set BOT, 3 inv"));
  Serial.println (F("_f  set interrupts:  32 ICIE  4 OCIEB, 2 OCIEA, 1 TOIE"));
  Serial.println (F("i   init Timer (also called after sets c..f above"));
  Serial.println (F("_k  read Konfig (0 to 10)"));
  Serial.println (F("_K  write Konfig (0 to 10)"));
  Serial.println (F("_l  ADMUX  0..7 8, 14,15)"));
  Serial.println (F("_m  set ICNC1 2 and ICES1 1 (0 to 3)"));
  Serial.println (F("n   onefire"));
  Serial.println (F("o   toggle OneShot"));
  Serial.println (F("_p  ADPS Prescaler 0..7 = 2 ..1 128"));
  Serial.println (F("_r  pointer to switch off"));
  Serial.println (F("_s  pointer to switch on"));
  Serial.println (F("_t  tick time in ms"));
  Serial.println (F("_u  set us per tick post - decs e.g. 1234u means 0.01234"));
  Serial.println (F("v   toggle verbose"));
  Serial.println (F("_w  set wgm 0..15"));
  Serial.println (F("x   eXec one capture"));
  Serial.println (F("y   toggle auto"));
  Serial.println (F("z   show Ripu"));
}

void doCmd(char x) {
  float tmpf;
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
      digitalWrite(pstart, LOW);
      showCnts();
      break;
    case '+':
      limH += 1;
      msgF(F("limH"), limH);
      break;
    case '-':
      limH -= 1;
      msgF(F("limH"), limH);
      break;

    case 'a':
      OCR1A = inp;
      msgF(F(": OCR1A"), OCR1A);
      break;
    case 'b':
      OCR1B = inp;
      msgF(F(": OCR1B"), OCR1B);
      break;
    case 'c':
      tim1CS = inp & 7;
      timer1Init();
      msgF(F(": tim1CS"), tim1CS);
      break;
    case 'd':
      tim1COM1A = inp & 3;
      timer1Init();
      msgF(F(": tim1COM1A"), tim1COM1A);
      break;
    case 'e':
      tim1COM1B = inp & 3;
      timer1Init();
      msgF(F(": tim1COM1B"), tim1COM1B);
      break;
    case 'f':
      tim1TIMSK1 = inp ;
      timer1Init();
      msgF(F(": tim1TIMSK1"), tim1TIMSK1);
      break;
    case 'g':
      durchOdd = !durchOdd;
      msgF(F("durchOdd"), durchOdd);
      break;
    case 'h':
      help();
      break;
    case 'i':
      timer1Init();
      adcInit();
      break;
    case 'j':
      showRegs();
      break;
    case 'k':
      readKonfig(inp);
      timer1Init();
      adcInit();
      showRegs();
      break;
    case 'K':
      writeKonfig(inp);
      break;
    case 'l':
      anMUX = inp & 15;
      adcInit();
      msgF(F("anMUX"), anMUX);
      break;
    case 'm':
      tim1IC = inp & 3;
      timer1Init();
      msgF(F(": tim1IC"), tim1IC);
      break;
    case 'n':
      doFire();
      break;
    case 'o':
      oneShot = !oneShot;
      msgF(F("oneShot"), oneShot);
      break;
    case 'p':
      anPS = inp & 7;
      adcInit();
      msgF(F(": anPS"), anPS);
      break;
    case 'r':
      anOffP = inp;
      msgF(F("anOffP"), anOffP);
      break;
    case 's':
      anOnSav = inp;
      msgF(F("anOnP"), anOnSav);
      break;
    case 't':   //
      tickMs = inp;
      msgF(F("tickMs"), inp);
      break;
    case 'u': //
      msgF(F(" to uspt"), inp);
      uspt = 0.00001 * inp;
      Serial.println(uspt, 6);
      break;
    case 'U': //
      msgF(F(" Dauer von"), inp);
      tmpf = uspt * inp;
      Serial.println(tmpf, 6);
      break;
    case 'v':
      verbo++;
      if (verbo > 2) verbo = 0;
      //if (verbo > 0) verbo = 2; // no 1
      msgF(F("Verbose"), verbo);
      break;
    case 'w':
      tim1WGM = inp & 15;
      timer1Init();
      msgF(F("wgm"), tim1WGM);
      break;
    case 'x': //start in auto
      ovcnt = 0; // to get convenient values
      ripuP = 0; // worst case ripuP is inced now
      anOnP = anOnSav;
      durchl = 0;
      break;
    case 'y':
      fire = !fire;
      msgF(F("fire"), fire);
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
  const char info[] = "adc " __DATE__  " "  __TIME__;
  Serial.begin(38400);
  Serial.println(info);
  pinMode(picp1, INPUT_PULLUP);
  pinMode(pcinp, INPUT_PULLUP);
  pinMode(poc1A, OUTPUT);
  pinMode(poc1B, OUTPUT);
  pinMode(povf, OUTPUT);
  pinMode(pcap, OUTPUT);
  pinMode(pstart, OUTPUT);
  digitalWrite(pstart, LOW);
  OCR1A = 100;
  OCR1B = 60;
  readKonfig(0);
  timer1Init();
  readKonfig(0);  // wtf odbc1a??
  limH=5;// raus!
  timer1Init();
  
  adcInit();
  tickMs = 1000;
}

void loop() {
  if (Serial.available() > 0) {
    doCmd( Serial.read());
  }

  if (ripuP < ripuM) {
    if (ripuP != ripuOld) {
      evalMess();
    }
  }

  if (ferdisch) {
    ferdisch = false;
    if (verbo < 2)   showRipu();
  }

}

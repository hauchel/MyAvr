// learning timer2

#include "helper.h"
//#include <EEPROM.h>

const byte poc2A = 11; // pb3
const byte poc2B = 3;  // pd3
byte volatile cntO; // counts for ints
byte volatile cntA; //
byte volatile cntB; //

byte tim2cs = 6;    // value of TCCR2B i.e. clock select, 6 = /256
byte tim2cmoA = 1;  // compare Match Output A 0..3 to  TCCR2A
byte tim2cmoB = 2;  // compare Match Output B 0..3 to  TCCR2A
byte tim2ocie = 0;  // interrupt enable to TIMSK2
byte tim2wgm = 7;     // to TCCR2A and TCCR2B

typedef struct { // params
  byte magic = 42;
  byte cs = 7;
  byte cmoA = 1;
  byte cmoB = 2;
  byte ocie = 0;
  byte wgm = 7;
  byte oc2a = 77;
  byte oc2b = 60;
} lad_t;
lad_t lad;

bool autoOn = false; //

void timer2Init() {
  // need to set globals before calling!
  // Timer for fast pwm mode 7: counts to OCR2A, int at OCR2B and OCR2A
  // 7      6       5       4       3       2       1       0
  // COM2A1 COM2A0  COM2B1  COM2B0  –       –       WGM21   WGM20   TCCR2A
  //    cmoA            cmoB        0       0       w       w
  TCCR2A =  (tim2cmoA << 6) | (tim2cmoB << 4) | (tim2wgm & 3);
  // FOC2A  FOC2B   –       –       WGM22   CS22    CS21    CS20    TCCR2B
  // 0      0       0       0       w       c       c       c
  TCCR2B = tim2cs | ((tim2wgm << 1) & 8);
  //                                        OCIE2B  OCIE2A  TOIE2   TIMSK2
  // 0      0       0       0       0       o       o       o
  TIMSK2 = tim2ocie;                                    // enable Timer interrupts
  sei();  //set global interrupt flag to enable interrupts
}

ISR(TIMER2_OVF_vect) {
  cntO++;
}

ISR(TIMER2_COMPA_vect) {
  cntA++;
}

ISR(TIMER2_COMPB_vect) {
  cntB++;
}

void showCnts() {
  char str[50];
  byte t = TCNT2;
  sprintf(str, "O:%4u A:%4u B:%4u TCNT:%4u ", cntO, cntA, cntB, t);
  Serial.println(str);
}

void showRegs() {
  char str[50];
  sprintf(str, "OCRA:%4u OCRB:%4u TCRA:%4u TCRB: %4u", OCR2A, OCR2B, TCCR2A, TCCR2B);
  Serial.println(str);
}


void explainRegs() {
  char str[80];
  byte t, wgm, comA, comB, cs;
  t = TCCR2A;
  wgm = t & 3;
  comA = t  >> 6;
  comB = (t >> 4) & 3;
  t = TCCR2B;
  cs = t & 7;
  wgm |= ((t >> 1) & 4);
  sprintf(str, "wgm %3u, cs %3u, comA %2u, comB %2u", wgm, cs, comA, comB);
  Serial.println(str);
}

void help () {
  Serial.println (F("Running in Fast PWM wgm=7"));
  Serial.println (F("_a  set OCR2A to change freq"));
  Serial.println (F("_b  set OCR2B (<OCR1A) to change "));
  Serial.println (F("_c  set clockselect 0 to 7"));
  Serial.println (F("_d  set COM2A (Pin 11) to 0 not, 1 Toggle, 2 Clear Match, set BOT, 3 invert"));
  Serial.println (F("_e  set COM2B (Pin 3)  to 0 not, 1 res, 2 non-inv, 3 inv"));
  Serial.println (F("_f  set interrupts: 4 OCIE2B, 2 OCIE2A, 1 TOIE2"));
  Serial.println (F("i   init Timer (also called after sets c..f above"));
  Serial.println (F("o   force Output compare 128+64+tim2cs to TCCR2B"));
  Serial.println (F("p   Pinmode outputs"));
  Serial.println (F("r   set low"));
  Serial.println (F("s   set high"));
  Serial.println (F("_t  tick time in ms"));
  Serial.println (F("_w  set wgm 0..7"));
  Serial.println (F("x   eXec one capture"));
  Serial.println (F("y   toggle auto-capture"));
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
      showRegs();
      break;
    case 'a':
      OCR2A = inp;
      msgF(F(":OCR2A"), OCR2A);
      break;
    case 'b':
      OCR2B = inp;
      msgF(F(":OCR2B"), OCR2B);
      break;
    case 'c':
      tim2cs = inp & 7;
      timer2Init();
      msgF(F(":tim2cs"), tim2cs);
      break;
    case 'd':
      tim2cmoA = inp & 3;
      timer2Init();
      msgF(F(":tim2cmoA"), tim2cmoA);
      break;
    case 'e':
      tim2cmoB = inp & 3;
      timer2Init();
      msgF(F(";tim2cmoB"), tim2cmoB);
      break;
    case 'f':
      tim2ocie = inp & 7;
      timer2Init();
      msgF(F(":tim2ocie"), tim2ocie);
      break;
    case 'h':
      help();
      break;
    case 'i':
      timer2Init();
      explainRegs();
      msgF(F("TCCR2A"), TCCR2A);
      msgF(F("TCCR2B"), TCCR2B);
      break;
    case 'o':
      TCCR2B = 128 | 64 | ((tim2wgm << 1) & 8) | tim2cs;
      msgF(F("FOC"), TCCR2B);
      break;
    case 'p':
      pinMode(poc2A, OUTPUT);
      pinMode(poc2B, OUTPUT);
      msgF(F("Pinmodes"), 0);
      break;
    case 'r':
      digitalWrite(poc2A, LOW);
      digitalWrite(poc2B, LOW);
      msgF(F("Set to "), 0);
      break;
    case 's':
      digitalWrite(poc2A, HIGH);
      digitalWrite(poc2B, HIGH);
      msgF(F("Set to"), 1);
      break;
    case 't':   //
      tickMs = inp;
      msgF(F("tickMs"), inp);
      break;
    case 'w':
      tim2wgm = inp & 7;
      timer2Init();
      msgF(F("wgm"), tim2wgm);
      break;
    case 'x': //ICP
      InputCapture_out();
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
  const char info[] = "timer2 " __DATE__  " "  __TIME__;
  Serial.begin(38400);
  Serial.println(info);
  pinMode(poc2A, OUTPUT);
  pinMode(poc2B, OUTPUT);
  OCR2A = 77;
  OCR2B = 60;
  timer2Init();
  tickMs = 1000;
  //ICP
#define ICP_INPUT 8
  pinMode(ICP_INPUT, INPUT_PULLUP);
  init_InputCapture();
  // to test connect pin 8 to pin3 so timer2 OC2B is used as input

}

void loop() {
  if (Serial.available() > 0) {
    doCmd( Serial.read());
  }
  currMs = millis();
  if (currMs - prevMs >= tickMs) {
    prevMs = currMs;
    if (autoOn)  InputCapture_out();
  } // tick
}

// below from https://www.mikrocontroller.net/topic/563902

volatile unsigned long ICP_val = 0, T1OVF_Ctr = 0;

/******************************************************************************
  Function:     void InputCapture()
  Description:
  Parameters:   None
  Return Value: None
******************************************************************************/
void InputCapture_out()
{
  double Freq = 0.0;

  if (ICP_val > 0)
    Freq = 16000000.0 / (double) ICP_val;

  Serial.print("ICP: ");
  Serial.print(ICP_val);
  Serial.print(" OVF: ");
  Serial.print(T1OVF_Ctr);
  Serial.print(" F: ");
  Serial.println(Freq, 4);
}



/******************************************************************************
  Function:     void init_ICP()
  Description:  setup for ICP
  Parameters:   None
  Return Value: None
******************************************************************************/
void init_InputCapture()
{
  //HH added output compares to ease debugging
  // OC1B PB2 Pin10
  // TCCR1A =0
  TCCR1A = (1 << COM1B0) ;
  pinMode(10, OUTPUT);
  OCR1B = 0;
  //HH End
  TCCR1B = 0;
  TIMSK1 = 0;
  // ICNC1 ICES1 â€“ WGM13 WGM12 CS12 CS11 CS10
  TCCR1B = (1 << ICNC1) | (1 << CS10); // 16MHZ Clk, ICU Filter EN, ICU Pin falling
  TIFR1 =   1 << ICF1;                // Clear ICF1. Clear pending interrupts
  TIMSK1 = (1 << TOIE1) | (1 << ICIE1); // Enable Timer OVF & CAPT Interrupts
}



/******************************************************************************
  Function:     ISR(TIMER1_OVF_vect)
  Description:  Keep track of overflows from T1 counter
  Parameters:   None
  Return Value: None
******************************************************************************/
ISR(TIMER1_OVF_vect)
{
  T1OVF_Ctr++;
}



/******************************************************************************
  Function:     ISR(TIMER1_CAPT_vect)
  Description:  respond to ICP transitions and compute result
  Parameters:   Return global: Freq, ICP_val
  Return Value: None
******************************************************************************/
ISR(TIMER1_CAPT_vect)
{
  static uint8_t toggle = 0;
  static uint16_t T1;
  uint16_t T2;
  //HH unused unsigned long T;

  if (!toggle)
  {
    T1 = ICR1;
    toggle = 1;
    T1OVF_Ctr = 0;
  }
  else
  {
    T2 = ICR1;
    ICP_val = T2 + (65536 * T1OVF_Ctr) -  T1;
    T1OVF_Ctr = 0;
    toggle = 0;
  }
}

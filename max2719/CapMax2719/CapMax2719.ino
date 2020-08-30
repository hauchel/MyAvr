/* Counter with LED-Display for NE555 Cap
   pulses counted in Timer1 (ext trigger on D5) while Timer2 counts 1 or 1/10 sec
   Timer2 values taken from eprom to compensate bad resonator

  Display
  D9   DataIn    white
  D8   CS/Load   grey
  D10  CLK       pink 
  Timer 1
  D5   T1        brown
  D6   AIN0      green
  D7   AIN1      blau
  D8   ICP1
  D9   OC1A      used
  D10  OC1B      used

*/

#include <LedCon5.h>
const byte anzR = 1; // number of LED Disps
//             dataPin, clkPin, csPin, numDevices
LedCon5 lc = LedCon5(9, 10, 8, anzR);
byte intens;

byte runmode = 2;
const byte in0Pin = 6;
const byte in1Pin = 7;
const byte t1Pin = 5;

unsigned long currTim;
unsigned long prevTim = 0;
unsigned long tickTim = 1000;

volatile boolean tim2trig = false;
volatile unsigned int t1cnt;
volatile unsigned int t1over;
volatile unsigned int overs;
volatile byte t2ovf;               // current
volatile byte t2ovfset = 244;      // # of runs for one second
volatile byte t2ocaset  = 54;      // on last to trigger

void msg(const char txt[], int n) {
  Serial.print(txt);
  Serial.print(" ");
  Serial.println(n);
}

// tim1 begin
byte cs1x, comab, wgm10, wgm11, wgm12, wgm13;

void timer1CrAB() {
  TCCR1A =  comab |
            (wgm11 <<  WGM11) |
            (wgm10 <<  WGM10) ;

  TCCR1B = (0 <<  ICNC1) |
           (0 <<  ICES1) |
           (wgm13 <<  WGM13) |
           (wgm12 <<  WGM12) |
           cs1x;

  Serial.print("TCCR1A:");
  Serial.print(TCCR1A, BIN);
  Serial.print("   B: ");
  Serial.println(TCCR1B, BIN);
}

/* timer1
  define
  comab   output OC1A/OC1B
  wgm     waveform
  clock   speed
  timsk   ints

*/
void timer1CTC() {
  // count ctc, no outpus
  wgm13 = 0;
  wgm12 = 1;
  wgm11 = 0;
  wgm10 = 0;
  comab =   (0 << COM1A1) |
            (0 << COM1A0) |
            (0 << COM1B1) |
            (0 << COM1B0) ;
}


void timer1Setup() {
  /*  CTC Mode
       cs1x
      0 0 0 No clock source (Timer/Counter stopped).
      0 0 1 clkI/O/1 (No prescaling)
      0 1 0 clkI/O/8 (From prescaler)
      0 1 1 clkI/O/64 (From prescaler)
      1 0 0 clkI/O/256 (From prescaler)
      1 0 1 clkI/O/1024 (From prescaler)
      1 1 0 External clock source on T1 pin. Clock on falling edge.
      1 1 1 External clock source on T1 pin. Clock on rising edge.
  */
  timer1CTC();
  cs1x = 6;
  timer1CrAB();
  TCNT1  = 0;
  // set compare match register
  OCR1A = 9999;  //count til 9999 then inc t1over
  OCR1B =  8000; // egal
  // enable timer compare interrupt
  TIMSK1 |= (1 << OCIE1A);
  TIMSK1 |= (1 << OCIE1B);
  //Serial.println(TIMSK1, BIN);
}

ISR(TIMER1_COMPA_vect) {
  t1over = t1over + 1;
  // digitalWrite(intAPin, HIGH);
}

ISR(TIMER1_COMPB_vect) {
  // digitalWrite(intAPin, LOW);
}

// tim1 end

void timer2Setup() {
  /*
    normal mode (count 255), int in ocra so it counts 256*t2ovfset + t2ocaset
    2A contains com2a,com2b (output modes) and wgm21,20
    2B contains FOCs, wgm22 and cs2x
  */
  TCCR2A =  0;
  TCCR2B =  6;   // clock / 256
  TCNT2  = 0;
  // set compare match register
  OCR2A = 255; // is set at start
  OCR2B = 255; // dont care
  // enable timer compare interrupt
  TIMSK2 = (1 << OCIE2A);
}

void timer2Start() {
  // setting the PSRASY bit in GTCCR resets prescaler
  GTCCR = (1 << PSRASY);
  OCR2A = t2ocaset;
  t2ovf = t2ovfset ;
  TCNT2 = 0;
  TCNT1 = 0;
  t1over = 0;
}

ISR (TIMER2_COMPA_vect)
{
  if (t2ovf == 0) {
    t1cnt = TCNT1;
    overs = t1over;
    tim2trig = true;
  } else {
    t2ovf--;
  }
}

void testdigit() {
  for (int i = 0; i < 8; i++) {
    lc.setDigit(lc.lcRow, i, i, false);
  }
}

void setIntens(char what) {
  if (what == '+') {
    intens++;
    if (intens > 15) intens = 0;
  }
  if (what == '-') {
    intens--;
    if (intens > 15) intens = 15;
  }
  for (int r = 0; r < anzR; r++) {
    lc.setIntensity(r, intens);
  }
}


void help() {
  Serial.println(" qwas  Tune T2");
  Serial.println(" n,m   switch LED on/off");
  Serial.println(" b LCD Basic mode");
  Serial.println(" c LCD Clear Screen");
  Serial.println(" d show Data");
  Serial.println(" e equalizer  nnn");
  Serial.println(" g,t LCD Graphmode,Textmode");
  Serial.println(" i show Info");
  Serial.println(" t LCD Text mode");
  Serial.println(" m next source (10=Lux)");
  Serial.println(" r run, z zero data");
  Serial.println(" u update, v verbose");

}

void doCmd(byte tmp) {
  long zwi;
  switch (tmp) {

    case 'b':   //
      break;
    case 'c':   //
      break;
    case 'g':   //
      break;
    case 'r':   //
      break;

    case 'x':   //
      runmode = 2;
      msg("runmode", runmode);
      setupcom();
      break;
    case 'y':   //
      runmode = 3;
      msg("runmode", runmode);
      setupcom();
      break;
    case 'z':   //
      runmode = 4;
      msg("runmode", runmode);
      setupcom();
      break;

    case 'd':   //
      testdigit();
      break;

    // Tuning t2
    case 'q':
      t2ocaset = t2ocaset - 1;
      msg("t2oca", t2ocaset);
      break;
    case 'w':   //
      t2ocaset = t2ocaset + 1;
      msg("t2oca", t2ocaset);
      break;
    case 'a':   //
      t2ovfset = t2ovfset - 1;
      msg("t2ovf", t2ovfset);
      break;
    case 's':   //
      t2ovfset = t2ovfset + 1;
      msg("t2ovf", t2ovfset);
      break;
      
    case 'h':   //
      break;
    case 'i':   //
      break;
    case 'j':   //
      break;

    case 't':   //
      t1cnt = TCNT1;
      TCNT1 = 0;
      overs = t1over;
      t1over = 0;
      tim2trig = true;
      break;

    case 'n':
      lc.shutdown(0, true);
      break;
    case 'm':
      lc.shutdown(0, false);
      break;

    default:
      Serial.print(tmp);
      lc.putch(tmp);
  } //case
}


void lcReset() {
  int devices = lc.getDeviceCount();
  //we have to init all devices in a loop
  msg("lcReset for ", devices);
  for (int address = 0; address < devices; address++) {
    /*The MAX72XX is in power-saving mode on startup*/
    lc.shutdown(address, false);
    lc.setIntensity(address, 2);
    lc.clearDisplay(address);
  }
}


void setupcom() {
  timer1Setup();
  timer2Setup();
  switch (runmode) {
    case 2:   //
      t2ovfset = 244;      // # of runs one sec
      t2ocaset  = 54;      // on last to trigger
      break;
    case 4:
      t2ovfset = 24;      // # of runs 1/10 sec
      t2ocaset  = 108;    // on last to trigger
      break;
    default:
      msg ("setupcom invalid runmode", runmode);
      break;
  } //case
  timer2Start();
}

void setup() {
  const char info[] = "CapMax2719  " __DATE__  " " __TIME__;
  Serial.begin(38400);
  Serial.println(info);
  pinMode(in0Pin, INPUT_PULLUP);
  pinMode(in1Pin, INPUT_PULLUP);
  pinMode(t1Pin, INPUT_PULLUP);
  runmode = 2;
  setupcom();
  lcReset();
}

void loop() {

  if (Serial.available() > 0) {
    doCmd(Serial.read());
  } // avail


  if (tim2trig)  {
    lc.home();
    lc.show2Dig(overs);
    lc.show4Dig(t1cnt);
    lc.putch(' ');
    timer2Start();
    tim2trig = false;

  }

  currTim = millis();
  if (currTim - prevTim >= tickTim) {
    prevTim = currTim;
    switch (runmode) {
      case 2:   //
        break;
      default:
        break;
    } //case
  } //tick
}


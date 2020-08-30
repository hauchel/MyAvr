/* Timer1 Experiments ICP with LED-Display and TCS3200

*/
#include <LedCon5.h>
/*
  LED Display
  D11  DataIn    white
  D12  CS/Load   grey
  D13  CLK       pink
  Timer 1
  D5   T1        brown
  D6   AIN0      green
  D7   AIN1      blau
  D8   ICP1
  D9   OC1A
  D10  OC1B
  Outputs
  A0 (14)        white  s0  -green
  A1 (15)        grey   s1  -red
  A2 (16)        pink   s2  -yell
  A3 (17)        blue   s3  -blue

                       GND  -blue
                       VCC  -yell
                       OUT  -red
                       LED  -green
  für tcs3200 scaling 3 cs1x=1 counts 1 cyc (10 kHz=1600 2kHz=8000)
       0    1    2    2+RF   2+GF
  R  1250  300  180    300    740
  G  3500  690  400   1700   1030
  B  3800  900  520   1450   1800
  C   900  180  105    250    370
*/

const byte anzR = 1; // number of LED Disps
//             dataPin, clkPin, csPin, numDevices
LedCon5 lc = LedCon5(11, 13, 12, anzR);
const byte t1Pin = 5;
const byte in0Pin = 6;
const byte in1Pin = 7;
const byte icpPin = 8;

const byte s0Pin = A0;
const byte s1Pin = A1;
const byte s2Pin = A2;
const byte s3Pin = A3;

byte myCol;
long val[4];            // current frequencies
uint16_t valP[3];       // percentage RGB
byte intens = 8;
bool fmode = true;      // show frequencies/data
bool verb = false;      // tell me more
bool sample = true;     //
bool updLed = true;     // reduce LED updates during samples
bool perc = true;       // calc 100% else use C
const byte dataL = 16;  // samples to take
uint16_t data [dataL + 2];
volatile byte dataP = 0;

unsigned long fosc = 16000000L;
unsigned long currMs, nextTick;
unsigned long tickTime = 1000;

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
  cs1x    speed
  timsk   ints
*/

void timer1CTC() {
  // count ctc, no outpus
  wgm13 = 0;
  wgm12 = 1;
  wgm12 = 0;  // normal, no ctc
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
  timer1CrAB();
  TCNT1  = 0;
  // set compare match register
  OCR1A = 9999;  //count til 9999 then inc t1over
  OCR1B =  8000; // egal
  // enable interrupts
  //TIMSK1 |= (1 << OCIE1A);
  //TIMSK1 |= (1 << OCIE1B);
  TIMSK1 |= (1 << ICIE1);
  //TIMSK1 |= (1 << TOIE1);
  //Serial.println(TIMSK1, BIN);
}

ISR(TIMER1_COMPA_vect) {
  // digitalWrite(intAPin, HIGH);
}

ISR(TIMER1_COMPB_vect) {
}

ISR (TIMER1_CAPT_vect)
{
  //When interrupt into TIMER1_CAPT_vect
  if (dataP < dataL) {
    data[dataP] = ICR1;
    dataP++;
  }
}

ISR (TIMER1_OVF_vect)
{
  // digitalWrite(intAPin, LOW);
}
// tim1 end


void miss() {
  // after selecting color start
  for (myCol = 0; myCol < 4; myCol++) {
    setColor(myCol);
    dataP = 0;
    while (dataP < dataL) {
      delay(5);
    }
    // take this one
    val[myCol] = fosc / (data[9] - data[8]);
  }
  calcValP();
  show();
}

void calcValP() {
  unsigned long zw, ref;
  if (perc) {
    ref = val[0] + val[1] + val[2];
  } else {
    ref = val[3];
  }
  for (int i = 0; i < 3; i++) {
    zw = 100L * val[i];
    valP[i] = zw / ref;
  }
}

void show() {
  char str[100];
  sprintf(str, "R%3u G%3u B%3u    ", valP[0], valP[1], valP[2]);
  Serial.println(str);
  if (verb) {
    sprintf(str, "R %6lu  G %6lu  B %6lu  C %6lu", val[0], val[1], val[2], val[3]);
    Serial.println(str);
  }
}

void showData() {
  char str[100];
  msg("DataP ", dataP);
  for (int i = 0; i < dataL; i += 8) {
    sprintf(str, "%3d   %5u %5u %5u %5u  %5u %5u %5u %5u ", i, data[i], data[i + 1], data[i + 2], data[i + 3], data[i + 4], data[i + 5], data[i + 6], data[i + 7]);
    Serial.println(str);
  }
}

void evalData() {
  char str[100];
  uint16_t dif[8];
  unsigned long fre[8];
  int i, j;
  for (i = 0; i < dataL; i += 8) {
    for (j = 0; j < 8; j ++) {
      dif[j] = data[i + j + 1] - data[i + j];
      fre[j] = fosc / dif[j];
    }
    if (!fmode) {
      sprintf(str, "%3d  D  %5u %5u %5u %5u  %5u %5u %5u %5u ", i, dif[0], dif[1], dif[2], dif[3], dif[4], dif[5], dif[6], dif[7]);
    } else {
      sprintf(str, "%3d  F  %5lu %5lu %5lu %5lu  %5lu %5lu %5lu %5lu ", i, fre[0], fre[1], fre[2], fre[3], fre[4], fre[5], fre[6], fre[7]);
    }
    Serial.println(str);
  }
}

void setScaling(byte b) {
  /*
    s0 s1  Frequency Scaling
    L  L Power down
    L  H   2%
    H  L  20%
    H  H 100%
  */
  msg("Scaling", b);
  switch (b) {
    case 0:
      digitalWrite(s0Pin, 0);
      digitalWrite(s1Pin, 0);
      break;
    case 1:
      digitalWrite(s0Pin, 0);
      digitalWrite(s1Pin, 1);
      break;
    case 2:
      digitalWrite(s0Pin, 1);
      digitalWrite(s1Pin, 0);
      break;
    case 3:
      digitalWrite(s0Pin, 1);
      digitalWrite(s1Pin, 1);
      break;
    default:
      msg ("invalid Scaling", b);
      break;
  }
}

void setColor(byte b) {
  /* Datasheet:
    s2 s3  Diode Type
    0 L  L   Red
    1 H  H   Green
    2 L  H   Blue    Vertauscht mit Clear??
    3 H  L   Clear (no filter)
  */
  if (verb) {
    msg("Color", b);
  }
  switch (b) {
    case 0: //R
      digitalWrite(s2Pin, 0);
      digitalWrite(s3Pin, 0);
      break;
    case 1: //G
      digitalWrite(s2Pin, 1);
      digitalWrite(s3Pin, 1);
      break;
    case 2: //B
      digitalWrite(s2Pin, 1);
      digitalWrite(s3Pin, 0);
      break;
    case 3: //C
      digitalWrite(s2Pin, 0);
      digitalWrite(s3Pin, 1);
      break;
    default:
      msg ("invalid setColor ", b);
      break;
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
  msg("Intens ", intens);
}

void help() {
  Serial.println(" 0 - 3    set Scaling");
  Serial.println(" r,g,b,c  set Filter");
  Serial.println(" + -      set Intens");
  Serial.println(" h,j      set OC1A");
  Serial.println(" k,l      set cs1x");
  Serial.println(" d,e      show,eval Data");
  Serial.println(" f        toggle Freq/Cnt");
  Serial.println(" a        Testdigits");
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
    case '0':
    case '1':
    case '2':
    case '3':
      setScaling(tmp - '0');
      dataP = 0;
      break;
    case '+':   //
    case '-':   //
      setIntens(tmp);
      break;
    case 'a':   //
      testdigit();
      break;
    case 'b':   //
      dataP = 0;
      setColor(2);
      break;
    case 'c':   //
      dataP = 0;
      setColor(3);
      break;
    case 'd':   //
      showData();
      break;
    case 'e':   //
      evalData();
      break;
    case 'f':   //
      fmode = !fmode;
      msg("Fmode ", fmode);
      evalData();
      break;
    case 'g':   //
      dataP = 0;
      setColor(1);
      break;
    case 'h':   //
      evalData();
      break;
    case 'i':   //
      break;
    case 'j':   //
      evalData();
      break;
    case 'l':   //
      cs1x += 1;
      if (cs1x > 7) {
        cs1x = 0;
      }
      msg ("cs1x ", cs1x);
      timer1Setup();
      dataP = 0;
      break;
    case 'k':   //
      cs1x -= 1;
      if (cs1x > 7) {
        cs1x = 7;
      }
      msg ("cs1x ", cs1x);
      timer1Setup();
      dataP = 0;
      break;
    case 'm':   //
      miss();
      break;
    case 'r':   //
      dataP = 0;
      setColor(0);
      break;
    case 's':   //
      perc = !perc;
      msg("100 % ", perc);
      break;
    case 't':   //
      TCNT1 = 0;
      dataP = 0;
      break;
    case 'v':   //
      verb = !verb;
      msg("verbose ", verb);
      break;
    case 'x':   //
      sample = true;
      setColor(myCol);
      dataP = 0;
      msg("sample on ", myCol);
      break;
    case 'y':   //
      sample = false;
      msg("sample off ", myCol);
      break;
    case 'z':   //
      break;

    case 'q':
      lc.shutdown(0, true);
      break;
    case 'w':
      lc.shutdown(0, false);
      break;

    default:
      msg("?? ", tmp);
      help();
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

void setup() {
  const char info[] = "Tim1Max2719  " __DATE__  " " __TIME__;
  Serial.begin(38400);
  Serial.println(info);
  pinMode(in0Pin, INPUT_PULLUP);
  pinMode(in1Pin, INPUT_PULLUP);
  pinMode(t1Pin, INPUT_PULLUP);
  pinMode(icpPin, INPUT_PULLUP);
  pinMode(s0Pin, OUTPUT);
  pinMode(s1Pin, OUTPUT);
  pinMode(s2Pin, OUTPUT);
  pinMode(s3Pin, OUTPUT);
  setScaling(3);
  myCol = 0;
  setColor(myCol);
  cs1x = 1;
  timer1Setup();

  lcReset();
}

void loop() {

  if (Serial.available() > 0) {
    doCmd(Serial.read());
  } // avail

  if (sample) {
    if (dataP >= dataL) {
      // take this one
      val[myCol] = fosc / (data[9] - data[8]);
      myCol++;
      if (myCol > 3) {
        myCol = 0;
        calcValP();
        if (updLed) {
          updLed = false;
          lc.home();
          lc.show2Dig(valP[0]);
          lc.putch(' ');
          lc.show2Dig(valP[1]);
          lc.putch(' ');
          lc.show2Dig(valP[2]);
        }
        //show();
      }
      setColor(myCol);
      dataP = 0;
    }
  } // sample

  currMs = millis();
  // every second
  if (nextTick <= currMs) {
    nextTick = currMs + tickTime;
    updLed = true;
    if (sample) {
    } else {
      lc.home();
      lc.show5Dig(TCNT1);
      lc.putch(' ');
    }
  } //tick
}


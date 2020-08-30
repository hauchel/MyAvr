/* Timer1 Experiments with LED-Display and dcf77
    use int0 on D2, store timer values in ripu then to data

*/
#include <LedCon5.h>
/*
  LED Display
  D11  DataIn    white
  D12  CS/Load   grey
  D13  CLK       pink
  Ints
  D2   int0
  Timer 1
  D5   T1        brown
  D6   AIN0      green
  D7   AIN1      blau
  D8   ICP1
  D9   OC1A
  D10  OC1B
*/

const byte anzR = 1; // number of LED Disps
//             dataPin, clkPin, csPin, numDevices
LedCon5 lc = LedCon5(11, 13, 12, anzR);
const byte t1Pin = 5;
const byte in0Pin = 6;
const byte in1Pin = 7;
const byte icpPin = 8;

byte myCol;
byte intens = 8;
byte verb = 1;        // tell me more
bool sample = true;     //
const byte dataL = 128;  // timer protocol
const byte ripuL = 10;  // samples to take
const byte druL = 10;  // samples to take
uint16_t data [dataL + 2];
uint16_t ripu [ripuL];
uint16_t last;
volatile byte rinP = 0;
volatile byte routP = 0;
volatile byte dataP = 0;
volatile byte t1ovf = 0;
volatile byte state = 0;

unsigned long currMs, nextTick;
unsigned long tickTime = 1000;
byte hour = 99;
byte minute = 99;
byte hourX = 99;  // temp during eval
byte minuteX = 99;
byte sec = 99;
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
      0 0 1 clkI/O/1 (No prescaling)       0.0625us
      0 1 0 clkI/O/8 (From prescaler)
      0 1 1 clkI/O/64 (From prescaler)
      1 0 0 clkI/O/256 (From prescaler)
      1 0 1 clkI/O/1024 (From prescaler)  64us   4 sec
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
  //TIMSK1 |= (1 << ICIE1);
  TIMSK1 |= (1 << TOIE1);
  //Serial.println(TIMSK1, BIN);
}

ISR(TIMER1_COMPA_vect) {
  // digitalWrite(intAPin, HIGH);
}

ISR(TIMER1_COMPB_vect) {
}

ISR (TIMER1_CAPT_vect) {
}

ISR (TIMER1_OVF_vect) {
  // digitalWrite(intAPin, LOW);
  t1ovf++;
}
// tim1 end

void dcfRead() {
  // on change add to ripu
  ripu[rinP] = TCNT1;
  rinP++;
  t1ovf = 0;
  if (rinP >= ripuL) {
    rinP = 0;
  }
}

void showRipu() {
  char str[100];
  sprintf(str, "%5u %5u %5u %5u %5u  %5u %5u %5u %5u %5u ", ripu[0], ripu[1], ripu[2], ripu[3], ripu[4], ripu[5], ripu[6], ripu[7], ripu[8], ripu[9]);
  Serial.println(str);
}

void showData() {
  char str[100];
  msg("DataP ", dataP);
  for (int i = 0; i < dataL; i += 8) {
    sprintf(str, "%3d   %5u %5u %5u %5u  %5u %5u %5u %5u ", i, data[i], data[i + 1], data[i + 2], data[i + 3], data[i + 4], data[i + 5], data[i + 6], data[i + 7]);
    Serial.println(str);
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
  Serial.println(" d,e      show,eval Data");
  Serial.println(" a        Testdigits");
  Serial.println(" i show Info");
  Serial.println(" +,-  LCD brightness");
  Serial.println(" r run, z zero data");
  Serial.println(" , v verbose");
}

void ledhm() {
  lc.home();
  lc.show2Dig(hour);
  lc.putch(' ');
  lc.show2Dig(minute);
  lc.putch(' ');
}

void doCmd(byte tmp) {
  long zwi;
  switch (tmp) {
    case '0':
    case '1':
    case '2':
    case '3':
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

      break;
    case 'c':   //
      dataP = 0;

      break;
    case 'd':   //
      showData();
      break;
    case 'e':   //
      break;
    case 'f':   //
      break;
    case 'g':   //
      dataP = 0;
      break;
    case 'h':   //
      break;
    case 'i':   //
      info();
      break;
    case 'j':   //
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
      break;
    case 'r':   //
      showRipu();
      break;
    case 's':   //
      sample = !sample;
      msg("sample ", sample);
      break;
    case 't':   //
      TCNT1 = 0;
      dataP = 0;
      break;
    case 'v':   //
      verb += 1;
      if (verb > 2) {
        verb = 0;
      }
      msg("verbose ", verb);
      break;
    case 'x':   //
      break;
    case 'y':   //
      break;
    case 'z':   //
      for (int i = 0; i < dataL; i++) {
        data[i] = 0;
      }
      dataP = 0;
      msg("Zero", 0);
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


byte error = 99;
byte druP = 0;
byte akt;   //
byte aktW;  //wert
byte aktP;  //parity

void aktPlus(byte b) {
  if (b == 2) {
    akt = 0;
    aktW = 1;
    aktP = 0;
    return;
  }
  if (b == 1) {
    akt = akt + aktW;
    aktP += 1;
  }
  switch (aktW) {
    case 1:
      aktW = 2;
      break;
    case 2:
      aktW = 4;
      break;
    case 4:
      aktW = 8;
      break;
    case 8:
      aktW = 10;
      break;
    case 10:
      aktW = 20;
      break;
    case 20:
      aktW = 40;
      break;
    case 40:
      aktW = 0;
      break;
    default:
      msg("aktW invalid ", aktW);
      break;
  }
}

void druff(byte b) {
  druP += 1;
  if (verb == 1) {
    Serial.print(b);
  }
  lc.lcCol = 1;
  lc.show2Dig(druP);

  if (druP < 19) {
    return;
  }
  if (druP == 21) { // 1!
    Serial.print("/");
    aktPlus(2);
    return;
  }
  if (druP < 29) {
    aktPlus(b);
    return;
  }
  if (druP == 29) {
    if (error == 0) {
      minuteX=akt;
      msg("Min ", akt);
    }
    aktPlus(2);
    return;
  }
  if (druP < 36) {
    aktPlus(b);
    return;
  }
  if (druP == 36) {
    if (error == 0) {
      hourX=akt;
      msg("Std ", akt);
    }
    aktPlus(2);
    return;
  }
  if (druP < 42) {
    aktPlus(b);
    return;
  }
  if (druP == 42) {
    if (error == 0) {
      msg("Tag ", akt);
    }
    aktPlus(2);
    return;
  }
  if (druP < 45) {
    aktPlus(b);
    return;
  }
  if (druP == 45) {
    aktPlus(b);
    if (error == 0) {
      msg("WoT ", akt);
    }
    aktPlus(2);
    return;
  }
  if (druP < 51) {
    aktPlus(b);
    return;
  }
  if (druP == 51) {
    aktPlus(b);
    if (error == 0) {
      msg("Mon ", akt);
    }
    aktPlus(2);
    return;
  }
}

void err(byte b) {
  msg(" Err ", b);
  error = b;
}

byte newState(byte news) {
  // 10: Sync received
  // sequences 1 9 or 2 8
  if (news == 10) {
    if (error == 0) {
      msg ("valid ", druP);
      minute=minuteX;
      hour=hourX;
      ledhm();
      
    }
    state = 0;
    druP = 0;
    error = 0;
    return 0;
  }
  switch (state) {
    case 0:
      break;
    case 1:
      if (news == 9) {
        druff(0);
      } else {
        err(1);
      }
      break;
    case 2:
      if (news == 8) {
        druff(1);
      } else {
        err(2);
      }
      break;
    case 8:
    case 9:
      break;
    default:
      msg("news? ", news);
      break;
  }
  state = news;
  return news;
}

uint16_t lo100 =  1550;
uint16_t hi100 =  1750;
uint16_t lo200 =  3200;
uint16_t hi200 =  3400;
uint16_t lo800 = 12300;
uint16_t hi800 = 12500;
uint16_t lo900 = 13900;
uint16_t hi900 = 14100;
uint16_t sync  = 25000;


byte doSample() {
  // returns  1,2,8,9
  // 10 sync
  //  0 spike
  // 101..109 error
  uint16_t zwi;
  zwi = ripu[routP] - last;
  last = ripu[routP];
  routP++;
  if (routP >= ripuL) {
    routP = 0;
  }
  if (zwi > sync ) {
    dataP = 0;
    data[dataP] = zwi;
    return newState(10);
  }

  dataP++;
  if (dataP >= dataL) {
    dataP = 0;
  }
  data[dataP] = zwi;

  if (zwi < 2) {
    return 0;
  }

  if (zwi < lo100) {
    msg("zwi", zwi);
    return 101;
  }
  if (zwi < hi100) {
    return newState(1);
  }
  if (zwi < lo200) {
    return 102;
  }
  if (zwi < hi200) {
    return newState(2);
  }
  if (zwi < lo800) {
    return 108;
  }
  if (zwi < hi800) {
    return newState(8);
  }
  if (zwi < lo900) {
    return 109;
  }
  if (zwi < hi900) {
    return newState(9);
  }
  return 111;
}

void info() {
  char str[100];
  sprintf(str, "\nErr %3u, druP %3u, state %3u, akt %3u, P %2u W %3u ", error, druP, state, akt, aktP, aktW);
  Serial.println(str);
}

void setup() {
  const char info[] = "Tim1Max2719  " __DATE__  " " __TIME__;
  Serial.begin(38400);
  Serial.println(info);
  pinMode(in0Pin, INPUT_PULLUP);
  pinMode(in1Pin, INPUT_PULLUP);
  pinMode(t1Pin, INPUT_PULLUP);
  pinMode(icpPin, INPUT_PULLUP);
  pinMode(2, INPUT);
  cs1x = 5;
  timer1Setup();
  lcReset();
  attachInterrupt(0, dcfRead, CHANGE);          // Enable external interrupt (INT0)
}

void loop() {
  byte tmp;
  if (Serial.available() > 0) {
    doCmd(Serial.read());
  } // avail

  if (sample) {
    while (rinP != routP) {
      tmp = doSample();
      if (tmp > 10) {
        Serial.print("  ");
        Serial.println(tmp);
        info();
      } else {
        if (verb == 2) {
          Serial.print(tmp);
        }
      }
    }
  } // sample

  currMs = millis();
  // every second
  if (nextTick <= currMs) {
    nextTick = currMs + tickTime;
    if (sample) {

    } else {
      lc.home();
      lc.show5Dig(TCNT1);
      lc.putch(' ');
    }
  } //tick
}


// Sensor
//  A4   grau    SDA
//  A5   weiss   SCL
//  VCC  rot
//  GND  schwarz
// LED 13
//  Aufsteckteil
// D2 TSOP
// D4 Transmit
// D9 MotFet   gelb

//HH
#include "Wire.h"
#include "I2Cdev.h"
#include "MPU6050.h"
#include <VirtualWire.h>
#include "rc5_tim2.h"


const byte ledPin = 13;
const byte transmitPin = 4;
const byte receivePin = 5;
const byte motPin = 9;

MPU6050 accelgyro;

const byte mwMax = 30;
const byte mspMax = 3;
int mw[mspMax][mwMax];
byte mwP;
int mwPof;
int ax, ay, az;
int gx, gy, gz;

boolean show = false;
byte ledOn;
byte ledCnt = 10;
byte ledMax = 5;
byte sel;
byte motStat;
bool oneOnly = true; // stop motor after sample top
unsigned long currMs, nextMs, mpuMs;
unsigned long lopCnt = 0;
unsigned long tickTime = 100;
unsigned long mpuTime = 10;
byte waitCnt = 0;
byte aliCnt = 0; // count for alive signal, 0=send now
byte runmode = 0;
// 1= sample
//

byte senP;
const byte senMin = 2;
const byte senMax = VW_MAX_MESSAGE_LEN;
//               0=seq,   1=type
char sen[senMax] = {'X', 'B', 'Z', '1', '2', '3', '4', '5', '6', '7', '8', '9'};

const byte aliMax = 2;
char ali[aliMax] = {'X', 'A'};

char seq = '0';

void msg(const char txt[], int n) {
  Serial.print(txt);
  Serial.print(" ");
  Serial.println(n);
}


bool sende() {
  if (vw_tx_active ( )) {
    msg("sende busy", 0);
    return false;
  }
  sen[0] = incSeq();
  digitalWrite(ledPin, HIGH); // Flash a light to show transmitting, reset by ledon
  vw_send((uint8_t *)sen, senP);
  msg("sende los ", 0);
  return true;
}

void sendeAlive() {
  if (vw_tx_active ( )) {
    return;
  }
  ali[0] = incSeq();
  ali[1] = motStat;
  digitalWrite(ledPin, HIGH); // Flash a light to show transmitting
  vw_send((uint8_t *)ali, aliMax);
}

void sendeQuit(byte b) {
  char qui[3];
  qui[0] = incSeq();
  qui[1] = 'Q';
  qui[2] = b;
  vw_wait_tx();
  vw_send((uint8_t *)qui, 3);
  vw_wait_tx();
}

void sendeWait() {
  int zeit;
  sen[0] = incSeq();
  currMs = millis();
  digitalWrite(ledPin, HIGH); // Flash a light to show transmitting
  vw_send((uint8_t *)sen, senP);
  vw_wait_tx(); // Wait until the whole message is gone
  digitalWrite(ledPin, LOW);
  zeit = millis() - currMs;
  //msg("sent time ", zeit);
  aliCnt = 10;
}

void putch(char c) {
  sen[senP] = c;
  senP += 1;
  if (senP >= senMax) {
    sendeWait();
    senP = senMin;
  }
}

char incSeq() {
  seq += 1;
  if (seq > '9') {
    seq = '0';
  }
  return seq;
}

void putInt(int m) {
  putch(lowByte(m));
  putch(highByte(m));
}

void putMw(int r) {
  // row number+data
  putch(lowByte(r));
  for  (int j = 0; j < mspMax; j++) {
    putInt(mw[j][r]);
  }
  msg("putmw", r);
}

void trans(int beg, int anz) {
  for  (int j = beg; j < beg + anz; j++) {
    senP = senMin;
    putMw(j);
    sendeWait();
  }
}

void showRaw() {
  Serial.print("a/g:\t");
  Serial.print(ax);
  Serial.print("\t");
  Serial.print(ay);
  Serial.print("\t");
  Serial.print(az);
  Serial.print("\t");
  Serial.print(gx);
  Serial.print("\t");
  Serial.print(gy);
  Serial.print("\t");
  Serial.println(gz);
}

void sample() {
  accelgyro.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
  mw[0][mwP] = ax;
  mw[1][mwP] = ay;
  mw[2][mwP] = az;
  mwP++;
  if (mwP >= mwMax) {
    mwP = 0;
    mwPof++;
    if (oneOnly) {
      motoOff();
      runmode = 0;
    }
  }
}

int freeRam ()
{
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}

void showSample() {
  char str[80];
  int d0 = 0;
  int d1 = 0;
  int d2 = 0;
  int v = freeRam();

  sprintf(str, "mwP  %3d   mwPof %6d    tim %4d free %4d", mwP, mwPof, mpuTime, v);
  Serial.println(str);
  for  (int i = 0; i < mwMax; i++) {
    if (i > 0) {
      d0 = mw[0][i] - mw[0][i - 1];
      d1 = mw[1][i] - mw[1][i - 1];
      d2 = mw[2][i] - mw[2][i - 1];
    }
    if (i == mwP) {
      Serial.println();
    }
    sprintf(str, "%2d   %6d %6d    %6d %6d    %6d %6d", i, mw[0][i], d0, mw[1][i], d1, mw[2][i], d2);
    Serial.println(str);
  }
}

void newSpeed(int sp) {
  vw_setup(sp);
  msg("speed", sp);
}

void motoOn() {
  digitalWrite(motPin, HIGH);
  motStat = 'C';
}

void motoOff() {
  digitalWrite(motPin, LOW);
  motStat = 'A';
}

void doCmd(byte tmp) {
  int v;
  Serial.println();
  switch (tmp) {
    case 13:   //
      sendeWait();
      senP = senMin;
      break;
    case 8:   //
      senP = senMin;
      msg("senP", senP);
      break;
    case '+':   //
      ledOn++;
      if (ledOn > ledMax) {
        ledOn = ledMax;
      }
      msg("ledon", ledOn);
      break;
    case '-':   //
      ledOn--;
      if (ledOn > ledMax) { // it's a byte
        ledOn = 0;
      }
      msg("ledon", ledOn);
      break;
    // remote top row
    case '1':   //  sample data cont
      runmode = 1;
      mwP = 0;
      mwPof = 0;
      oneOnly = false;
      break;
    case '2':   //  sample data one shot
      aliCnt = 20;
      runmode = 1;
      mwP = 0;
      mwPof = 0;
      oneOnly = true;
      motoOn();
      break;
    // remote 2nd row
    case '4':   // transfer data 0..10
      trans(0, 15);
      break;
    case '5':   //
      trans(15, 15);
      break;
    case '6':   //
      trans(30, 10);
      break;
    // remote 3rd row
    case '8':   //
      motoOn();
      aliCnt = 0; // to show immediately
      break;
    case '9':   //
      motoOff();
      aliCnt = 0;
      break;
    // remote 4th row
    case 48:   //
      newSpeed(2000);
      aliCnt = 0;
      break;
    case 58:   //
      newSpeed(3000);
      aliCnt = 0;
      break;
    case 88:   //
      newSpeed(4000);
      aliCnt = 0;
      break;

    case 'a':   //
      mpuTime = 100;
      msg("mpuTime", mpuTime);
      break;
    case 'b':   //
      mpuTime = 50;
      msg("mpuTime", mpuTime);
      break;
    case 'c':   //
      mpuTime = 10;
      msg("mpuTime", mpuTime);
      break;
    case 'f':   //
      v = freeRam();
      msg("Free", v);
      break;
    case 'g':
      newSpeed(2000);
      break;
    case 'h':
      newSpeed(3000);
      break;
    case 'i':
      newSpeed(4000);
      break;
    case 'l':   //
      putMw(2);
      break;
    case 's':   // show data
      showSample();
      runmode = 0;
      msg("runmode ", runmode);
      break;
    case 'v':   //
      show = !show;
      msg("Show", show);
      break;
    default:
      msg("Was ? ", tmp);

  } //case
}
void setup() {
  const char info[] = "MPU6050_test " __DATE__ " " __TIME__;
  Serial.begin(38400);
  Serial.println(info);
  pinMode(motPin, OUTPUT);
  motoOff();
  Wire.begin();
  Serial.println("Initializing I2C devices...");
  accelgyro.initialize();
  Serial.println("Testing device connections...");
  if (accelgyro.testConnection()) {
    Serial.println("MPU6050 connection successful");
  } else {
    Serial.println ("MPU6050 connection failed");
  }

  vw_set_tx_pin(transmitPin);
  vw_setup(2000);       // Bits per sec
  pinMode(ledPin, OUTPUT);

  RC5_init();
}

void loop() {
  char text[10];
  if (Serial.available() > 0) {
    doCmd(Serial.read());
  } // avail
  if (rc5_ok) {
    RC5_again();
    /*    Serial.print(rc5_toggle_bit);                       // Display toggle bit
        sprintf(text, "%02X", rc5_address);
        Serial.print(text);                             // Display address in hex format
        sprintf(text, "%02X", rc5_command);
        Serial.println(text);                             // Display command in hex format
    */
    sendeQuit(rc5_command + '0');
    doCmd(rc5_command + '0');
  }
  currMs = millis();
  lopCnt++;

  // mpu related
  if (mpuMs <= currMs) {
    mpuMs = currMs + mpuTime;
    switch (runmode) {
      case 0:   //
        break;
      case 1:   //
        sample();
        break;
      default:
        break;
    } //case
  } // mpu


  // every xxms
  if (nextMs <= currMs) {
    nextMs = currMs + tickTime;
    // LED
    ledCnt--;
    if (ledCnt == 0) {
      digitalWrite(ledPin, LOW);
      ledCnt = ledMax;
    }
    if (ledCnt == ledOn) {
      digitalWrite(ledPin, HIGH);
    }

    if (aliCnt > 0) {
      aliCnt--;
    } else {
      sendeAlive();
      aliCnt = 20;
    }
    // wait
    if (waitCnt > 0) {
      waitCnt--;
    }
    //
    if (waitCnt == 0) {
      switch (runmode) {
        case 0:   //
          break;
        case 1:   //
          break;
        case 2:   //
          //HH
          accelgyro.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
          break;
        default:
          msg("Unexpected runmode", runmode);
          runmode = 0;
      } //case
    } // wait
    if (show) {
      showRaw();
    }
  } // tick
}



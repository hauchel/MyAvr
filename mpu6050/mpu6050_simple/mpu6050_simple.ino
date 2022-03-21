// MPU6050
//  A4   grau    SDA
//  A5   weiss   SCL
//  VCC  rot (check if 5V or 3V3)
//  GND  schwarz
// LED 13

//https://github.com/jrowberg/i2cdevlib
#include "Wire.h"
#include "I2Cdev.h"
#include "MPU6050.h"
#include <EEPROM.h>

const byte ledPin = 13;
byte ledOn;
byte ledCnt = 10;
byte ledMax = 5;
byte runMode;
byte runModeP; // previous
bool show;
byte num;     // general number entered

MPU6050 accelgyro;
const byte mwMax = 30;
const byte mspMax = 3;
int mw[mspMax][mwMax];
byte mwP;
int mwPof;
int ax, ay, az;
int gx, gy, gz;
const byte ofsMax = 6;
int offset[ofsMax];  //ax_  ay_  az_  gx_   gy_   gz_
int avg[ofsMax]; // average

unsigned long tickTime = 100;
unsigned long mpuTime = 10;
unsigned long currMs, nextMs, mpuMs;
bool oneOnly = true; // stop motor after sample top

int eadr = 0;

// General
void msg(const char txt[], int n) {
  Serial.print(txt);
  Serial.print(" ");
  Serial.println(n);
}

void printByte(byte b)   {
  char str[10];
  sprintf(str, "%3d ", b);
  Serial.print(str);
}

//Eprom related

union dub2Un {
  int i;
  byte b[2];
  unsigned int u;
};
dub2Un dub;

union dub4Un {
  long l;
  unsigned int u[2];
  byte b[4];
};
dub4Un dub4;

void toEprom(byte b) {
  EEPROM.update(eadr, b) ;
  printByte (b);
  eadr++;
}

void toEpromInt( int v) {
  dub.i = v;
  toEprom(dub.b[0]) ;
  toEprom(dub.b[1]) ;
}

byte fromEprom() {
  byte b;
  b = EEPROM.read(eadr) ;
  eadr++;
  printByte (b);
  return b;
}

int fromEpromInt() {
  dub.b[0] = fromEprom();
  dub.b[1] = fromEprom();
  return dub.i;
}

void toEpromAll() {
  eadr = 0;
  Serial.print("to  Eprom: ");
  for (int i = 0; i < ofsMax; i++) {
    toEpromInt(offset[i]);
  }
  msg("EADR", eadr);
}

void fromEpromAll() {
  eadr = 0;
  Serial.print("fromEprom: ");
  for (int i = 0; i < ofsMax; i++) {
    offset[i] = fromEpromInt();
  }
  msg("EADR", eadr);
}


// calc average for col n
int calcAvg(int n) {
  long lo = 0L;
  for (int i = 0; i < mwMax; i++) {
    lo += mw[n][i];
  }
  lo = lo / mwMax;
  return int(lo);
}

// calc sdev for col n
int calcSDev(int n, int avg) {
  long lo = 0L;
  long zwi;
  for (int i = 0; i < mwMax; i++) {
    zwi = mw[n][i] - avg;
    zwi = zwi * zwi;
    lo += zwi;
  }
  lo = lo / mwMax;
  return int(lo);
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


void sampleAcc() {
  accelgyro.getAcceleration(&ax, &ay, &az);
  mw[0][mwP] = ax;
  mw[1][mwP] = ay;
  mw[2][mwP] = az;
  mwP++;
  if (mwP >= mwMax) {
    if (oneOnly) {
      runMode = 0;
      showSample();
    }
    mwP = 0;
    mwPof++;
  }
}

void sampleRot() {
  accelgyro.getRotation(&gx, &gy, &gz);
  mw[0][mwP] = gx;
  mw[1][mwP] = gy;
  mw[2][mwP] = gz;
  mwP++;
  if (mwP >= mwMax) {
    if (oneOnly) {
      runMode = 0;
      showSample();
    }
    mwP = 0;
    mwPof++;
  }
}

void readFifo() {
  int ffcB, ffcA;
  // sample via fifo for n ms
  accelgyro.setFIFOEnabled(true);
  accelgyro.setRate(7); // 1 kHz
  accelgyro.resetFIFO();
  ffcB = accelgyro.getFIFOCount();
  accelgyro.setAccelFIFOEnabled(true);
  delay(mwMax+3);  // +3 for 
  accelgyro.setAccelFIFOEnabled(false);
  ffcA = accelgyro.getFIFOCount();
  msg("Before ", ffcB);
  msg("After  ", ffcA);
  mwP = 0;
  for  (int i = 0; i < mwMax; i++) {
    for  (int j = 0; j < 3; j++) {
      dub.b[1] = accelgyro.getFIFOByte();
      dub.b[0] = accelgyro.getFIFOByte();
      mw[j][i] = dub.i;
    } // next j
  } // next i
  showSample();
}

int freeRam () {
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}

void showMotion() {
  char str[80];
  sprintf(str, "XN %1d XP %1d",
          accelgyro.getXNegMotionDetected(),
          accelgyro.getXPosMotionDetected());
  /*bool getYNegMotionDetected();
    bool getYPosMotionDetected();
    bool getZNegMotionDetected();
    bool getZPosMotionDetected();
    bool getZeroMotionDetected(); */
  Serial.println(str);
}
void info() {
  msg("Temp        ",  accelgyro.getTemperature());
  msg("AccelRange  ",  accelgyro.getFullScaleAccelRange());
  msg("GyrosRange  ",  accelgyro.getFullScaleGyroRange());
  msg("SampleRate  ",  accelgyro.getRate());
  msg("FIFO count  ",  accelgyro.getFIFOCount());
  msg("FIFO byte   ",  accelgyro.getFIFOByte());
  msg("FIFO enabl  ",  accelgyro.getFIFOEnabled());
  msg("DLPF mode   ",  accelgyro.getDLPFMode());
}

void setOfs() {
  char str[80];
  sprintf(str, "old X %3d  Y %3d Z %3d",
          accelgyro.getXAccelOffset(),
          accelgyro.getYAccelOffset(),
          accelgyro.getZAccelOffset());
  msg(str, 0);
  accelgyro.setXAccelOffset(-688);
  accelgyro.setYAccelOffset(-787);
  accelgyro.setZAccelOffset(792);
  msg("setOfs done", 0);
}

void showSample() {
  char str[80];
  int d[mspMax];
  int j;
  sprintf(str, "mwP  %3d   mwPof %6d    tim %4d free %4d", mwP, mwPof, mpuTime, freeRam());
  Serial.println(str);
  for  (int j = 0; j < mspMax; j++) {
    d[j] = 0;
  }
  for  (int i = 0; i < mwMax; i++) {
    if (i > 0) {
      for  (int j = 0; j < mspMax; j++) {
        d[j] = mw[j][i] - mw[j][i - 1];
      }
      if (i == mwP) {
        Serial.println();
      }
    }
    sprintf(str, "%2d   %6d %6d    %6d %6d    %6d %6d", i, mw[0][i], d[0], mw[1][i], d[1], mw[2][i], d[2]);
    Serial.println(str);
  }
  sprintf(str, "avg  %6d           %6d           %6d", calcAvg(0), calcAvg(1), calcAvg(2));
  Serial.println(str);
}

void startRun(byte num) {
  runMode = num;
  runModeP = num;
  switch (runMode) {
    case 0:   //  nix
      break;
    case 1:   //  sample acc
      mwP = 0;
      mwPof = 0;
      break;
    case 2:   //  sample rot
      mwP = 0;
      mwPof = 0;
      break;
    default:
      msg("runMode? ", runMode);
  } //case
}

void doCmd(byte tmp) {
  Serial.println();
  if ((tmp >= '0') && (tmp <= '9')) {
    num = tmp - '0';
    msg("Num ", num);
    return;
  }
  switch (tmp) {
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

    case 'a':   //
      mpuTime = 100;
      msg("mpuTime", mpuTime);
      break;
    case 'A':   //
      accelgyro.setFullScaleAccelRange(num);
      msg("FullScaleAccelRange", num);
      info();
      break;
    case 'b':   //
      mpuTime = 50;
      msg("mpuTime", mpuTime);
      break;
    case 'c':   //
      mpuTime = 10;
      msg("mpuTime", mpuTime);
      break;
    case 'd':   //
      mpuTime = 1;
      msg("mpuTime", mpuTime);
      break;
    case 'e':   //
      fromEpromAll();
      break;
    case 'E':   //
      toEpromAll();
      break;

    case 'F':   //
      accelgyro.setFIFOEnabled(num);
      msg("FIFOEnabled", num);
      info();
      break;
    case 'g':   //
      startRun(runModeP);
      break;

    case 'G':   //
      accelgyro.setFullScaleGyroRange(num);
      msg("FullScaleGyrosRange", num);
      info();
      break;
    case 'i':   //
      info();
      break;
    case 'k':   //
      msg("freeRam", freeRam());
      break;
    case 'm':   //
      showMotion();
      break;
    case 'o':   //
      setOfs();
      break;

    case 'q':   //
      startRun(num);
      break;

    case 's':   // show data
      showSample();
      runMode = 0;
      msg("runMode ", runMode);
      break;
    case 'r':   //
      accelgyro.setRate(num);
      msg("rate ", num);
      break;
    case 't':   // show data
      break;
    case 'v':   //
      show = !show;
      msg("Show", show);
      break;
    case 'w':   //
      readFifo();
      break;

    default:
      msg("Was ? ", tmp);

  } //case
}

void setup() {
  const char info[] = "MPU6050_simple " __DATE__ " " __TIME__;
  Serial.begin(38400);
  Serial.println(info);
  Serial.println("Initializing I2C devices...");
  accelgyro.initialize();
  Serial.println("Testing device connections...");
  if (accelgyro.testConnection()) {
    Serial.println("MPU6050 connection successful");
  } else {
    Serial.println ("MPU6050 connection failed");
  }

  pinMode(ledPin, OUTPUT);
}

void loop() {
  char text[10];
  if (Serial.available() > 0) {
    doCmd(Serial.read());
  } // avail

  currMs = millis();

  // mpu related
  if (mpuMs <= currMs) {
    mpuMs = currMs + mpuTime;
    switch (runMode) {
      case 0:   //
        break;
      case 1:   //
        sampleAcc();
        break;
      case 2:   //
        sampleRot();
        break;
      default:
        break;
    } //case
  } // mpu


  // every tick
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

    switch (runMode) {
      case 0:   //
        break;
      case 1:   //
        break;
      case 2:   //
        break;
      default:
        msg("Unexpected runMode", runMode);
        runMode = 0;
    } //case
    if (show) {
      showRaw();
    }
  } // tick
}



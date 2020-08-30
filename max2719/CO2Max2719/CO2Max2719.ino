/* Display values sent by co2 sensor via MAX2719
   Check supply voltage
   TSOP controlled rc5

  TODO:
  Umrahmung Ziffern
  Lauflicht

  MH-Z19B:                              Top View:
  GND         black (3)              *         VCC red
  VCC         red   (4)              RX blue   GND black
  D10  RX  <- TX green (6)           TX green  *
  D11  TX  -> RX blue  (5)           *         *
  .                                  *
  TSOP4838:
  D2    out (1) purple
  GND       (2) black
  VCC       (3) red

  Display:
  D8   CS/Load   grey
  D9   DataIn    white
  D7   CLK       purple

  Analog:
  A3             yell
  A2             green
*/

#include "rc5_tim2.h"
const byte rc5Pin = 2;   // TSOP purple

#include <LedCon5.h>
const byte anzR = 1; // number of LED Disps
//             dataPin, clkPin, csPin, numDevices
LedCon5 lc = LedCon5(9, 7, 8, anzR);
byte intens = 2;

#include <SoftwareSerial.h>
SoftwareSerial mySerial(10, 11); // RX, TX
byte cmd2000[9] = {0xFF, 0x01, 0x99, 0x00, 0x00, 0x00, 0x07, 0xD0, 0x8F};
byte cmdRead[9] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};
byte cmdZero[9] = {0xFF, 0x01, 0x87, 0x00, 0x00, 0x00, 0x00, 0x00, 0x87};

byte recP = 0;
byte recv[9];

const byte anzVolt = 5;
unsigned int volt [anzVolt];
byte voltP = 0;

const byte ledPin = 13;  // indicators
byte runmode = 2;

unsigned long currTim;
unsigned int currMinut;
unsigned long prevTim = 0;
unsigned long prevVoltTim = 0;

unsigned long tickTim = 1000;
unsigned long voltTim = 1000;



volatile unsigned int anaVolt, co2Val = 8888;
bool verbose = false;
void msg(const char txt[], int n) {
  Serial.print(txt);
  Serial.print(" ");
  Serial.println(n);
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
  Serial.println(" r ticktim");
  Serial.println(" r,g,b ARef Def, Int,Ext");
  Serial.println(" n,m   switch LED on/off");
  Serial.println(" 0 to 5 src");
}

/*   communication
  0x86 Read CO2 concentration
    Response FF 86 hi lo
  0x99 Range setting
    Response FF 99 10 00
*/

byte getCheckSum(char *packet) {
  // return checksum pointed to
  char i, checksum;
  for ( i = 1; i < 8; i++) {
    checksum += packet[i];
  }
  checksum = 0xff - checksum;
  checksum += 1;
  return checksum;
}

byte myserEval() {
  // received 8, check contents
  byte cs;
  cs = getCheckSum((char *) recv);
  if (verbose) {
    Serial.print(cs, HEX);
  }
  if (recv[8] != cs) {
    co2Val = 9991;
    return 1;   // checksum err
  }
  if (recv[0] != 0xFF) {
    co2Val = 9992;
    return 2;  // not FF
  }
  if (recv[1] == 0x99) {
    return 0;
  }
  if (recv[1] != 0x86) {
    co2Val = 9993;
    return 3;  // unknown
  }
  co2Val = recv[2];
  co2Val = co2Val * 256 + recv[3];
  return 0;
}


void myserCheck() {
  // read incoming
  byte c, eva;
  while (mySerial.available()) {
    c = mySerial.read();
    recv[recP] = c;
    if (verbose) {
      Serial.print(c, HEX);
      Serial.print(" ");
    }
    recP++;
    if (recP > 8) {
      recP = 0;
      Serial.print("<");
      eva = myserEval();
      msg("Eval ", co2Val);
    }
  }
}

void myserRead() {
  recP = 0;
  for (int i = 0; i < 9; i++) {
    mySerial.write(cmdRead[i]);
  }
}

void myserRange2000() {
  recP = 0;
  for (int i = 0; i < 9; i++) {
    mySerial.write(cmd2000[i]);
  }
}

void myserZero() {
  recP = 0;
  for (int i = 0; i < 9; i++) {
    mySerial.write(cmdZero[i]);
  }
}

void myserRange10000() {
  recP = 0;
  //FF 01 99 00 00 00 27 10 2F
  mySerial.write((byte)0xFF);
  mySerial.write((byte)0x01);
  mySerial.write((byte)0x99);
  mySerial.write((byte)0x00);
  mySerial.write((byte)0x00);
  mySerial.write((byte)0x00);
  mySerial.write((byte)0x27);
  mySerial.write((byte)0x10);
  mySerial.write((byte)0x2F);
}

void setRunmode(byte b) {
  runmode = b;
  msg("runmode", runmode);
  prevTim = 0 ; //show immediately
}


void readAna() {
  anaVolt = analogRead(A3);
  const byte anzVolt = 5;
  volt [voltP] = anaVolt;
  voltP++;
  if (voltP >= anzVolt) {
    voltP = 0;
  }
}

unsigned int calcVolt() {
  unsigned int sum = 0;
  for (int i = 0; i < anzVolt; i++) {
    sum += volt[i];
    if (verbose) {
      msg("Volt", volt[i]);
    }
  }
  sum = sum / anzVolt;
  if (verbose) {
    msg("Sum ", sum);
  }
  return sum;
}

void clearVolt() {
  unsigned int sum = 0;
  for (int i = 0; i < anzVolt; i++) {
    volt[i] = 0;
  }
  voltP = 0;
  msg("Volt Zero", voltP);
}


void lcInfo(char *packet) {
  // print 4 of string
  lc.home();
  for (byte  i = 0; i < 4; i++) {
    lc.putch(packet[i]);
  }
}

void doRC5() {
  int sel;
  msg("RC5 ", rc5_command);
  digitalWrite(ledPin, HIGH);
  if ((rc5_command >= 0) && (rc5_command <= 9)) {
    doCmd(rc5_command + 48);
  } else {
    switch (rc5_command) {
      case 11:   // Store
        break;
      case 13:   // Mute
        break;
      case 43:   // >
        break;
      case 25:   // R
        doCmd('a');
        break;
      case 23:   // G
        doCmd('b');
        break;
      case 27:   // Y
        doCmd('c');
        break;
      case 36:   // B
        break;
      case 44:   // Punkt
        doCmd('z');
        break;
      case 57:   // Ch+
        setIntens('+');
        break;
      case 56:   // Ch+
        setIntens('-');
        break;
      default:
        msg("Which RC5?", rc5_command);
    } // case
  } // else
}


void doCmd(byte tmp) {
  switch (tmp) {
    case '-':
    case '+':   //
      setIntens(tmp);
      break;
    case '1':   //
      tickTim = 1000;
      msg ("Tick ", tickTim);
      break;
    case '2':   //
      tickTim = 5000;
      msg ("Tick ", tickTim);
      break;
    case '3':   //
      tickTim = 10000;
      msg ("Tick ", tickTim);
      break;
    case 'a':   //
      setRunmode(1);
      break;
    case 'b':   //
      setRunmode(2);
      break;
    case 'c':   //
      setRunmode(3);
      break;

    case 'd':   //
      analogReference(DEFAULT);
      msg ("Aref Def", DEFAULT);
      break;
    case 'g':   //
      analogReference(INTERNAL);
      msg ("Aref Int", INTERNAL);
      break;
    case 'i':   //
      myserRange10000();
      msg ("Range", 10000);
      break;
    case 'j':   //
      myserRange2000();
      msg ("Range", 2000);
      break;
    case 'n':
      lc.shutdown(0, true);
      break;
    case 'm':
      lc.shutdown(0, false);
      break;
    case 'r':
      myserRead();
      msg ("Read", 0);
      break;
    case 't':   //
      testdigit();
      break;
    case 'v':   //
      verbose = !verbose;
      msg("verbose", verbose);
      break;
    case 'x':   //
      clearVolt();
      break;
    case 'z':   //
      myserZero();
      msg ("Zero", 0);
      break;
    default:
      Serial.print(tmp);
      lc.putch(tmp);
  } //case
}


void lcReset() {
  int devices = lc.getDeviceCount();
  // init all devices in a loop
  msg("lcReset for ", devices);
  for (int address = 0; address < devices; address++) {
    /*The MAX72XX is in power-saving mode on startup*/
    lc.clearDisplay(address);
    lc.setIntensity(address, intens);
    lc.shutdown(address, false);
  }
}


void setup() {
  const char info[] = "CO2Max2719  " __DATE__  " " __TIME__;
  Serial.begin(38400);
  Serial.println(info);
  RC5_init();
  mySerial.begin(9600);
  pinMode(ledPin, OUTPUT);
  pinMode(A3, INPUT);
  pinMode(A1, INPUT);
  analogReference(INTERNAL);
  runmode = 1;
  lcReset();
}

void loop() {
  if (rc5_ok) {
    RC5_again();
    doRC5();
  }
  if (Serial.available() > 0) {
    doCmd(Serial.read());
  } // avail

  myserCheck();
  currTim = millis();
  if (currTim - prevTim >= tickTim) {
    digitalWrite(ledPin, LOW);
    prevTim = currTim;
    currMinut = currTim / 60000L;
    switch (runmode) {
      case 1:   //  CO2
        myserRead();
        lc.home();
        lc.putch('C');
        lc.putch('0');
        lc.putch('2');
        lc.putch(' ');
        lc.show4DigS(co2Val);
        break;
      case 2:   // Time and Volt
        lc.home();
        readAna();
        lc.show4DigS(calcVolt());
        lc.show4DigS(anaVolt);
        break;
      case 3:   // History
        lc.home();
        lc.putch('H');
        lc.putch('I');
        lc.putch('S');
        lc.putch(' ');
        lc.show4Dig(currMinut);
        break;
      default:
        break;
    } //case
  } //tick
}


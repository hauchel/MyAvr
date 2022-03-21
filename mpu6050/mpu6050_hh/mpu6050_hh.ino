// Lagebestimmung incl Servo

// SCL   A5 yell    1
// SDA   A4 grn     2
// Servo  9 ws
#include <Wire.h>
#include <Servo.h>
#include <EEPROM.h>

const byte serv = 9;
uint16_t inp;
uint16_t serpos;
bool inpAkt;
unsigned long currMs, prevMs = 0;
bool verbo = true;

byte posp = 0;
struct eprom {
  byte pos[10];
  int16_t wert[10];
};
eprom mypos;
const byte minp = 5;
const byte maxp = 180;

bool mess = false;
const uint16_t messSiz = 200;
int16_t messW[messSiz], wrtx, wrty;
uint8_t tims[messSiz];
uint16_t messAnz = 100;
uint16_t messCnt;
uint16_t messPtr;
unsigned long messMs = 10;
byte obsReg = 0x3B ;
int16_t obsWrt;

Servo myservo;
byte device = 0x68;


int16_t accX, accY, accZ, gyroX, gyroY, gyroZ, tRaw; // Raw register values (accelaration, gyroscope, temperature)
char result[7]; // temporary variable used in convert function

char* toStr(int16_t character) { // converts int16 to string and formatting
  sprintf(result, "%6d", character);
  return result;
}
// Memory saving helpers
void prnt(PGM_P p) {
  // flash to serial until \0
  while (1) {
    char c = pgm_read_byte(p++);
    if (c == 0) break;
    Serial.write(c);
  }
  Serial.write(' ');
}

void msgF(const __FlashStringHelper *ifsh, int16_t n) {
  PGM_P p = reinterpret_cast<PGM_P>(ifsh);
  prnt(p);
  Serial.println(n);
}

void prnF(const __FlashStringHelper *ifsh) {
  PGM_P p = reinterpret_cast<PGM_P>(ifsh);
  prnt(p);
}

void prlnF(const __FlashStringHelper *ifsh) {
  prnF(ifsh);
  Serial.println();
}

byte setReg( byte reg, byte val) {
  Wire.beginTransmission(device);
  Wire.write(reg);
  Wire.write(val);
  return Wire.endTransmission();     // stop transmitting
}

byte getReg(byte reg) {
  // delayMicroseconds(2000);
  Wire.beginTransmission(device);
  Wire.write(reg);
  Wire.endTransmission();
  Wire.requestFrom((uint8_t)device, (uint8_t)1);
  return Wire.read();
}

uint16_t getOne() {
  Wire.beginTransmission(device);
  Wire.write(obsReg); // starting with register 0x3B (ACCEL_XOUT_H)
  Wire.endTransmission(false);
  Wire.requestFrom(uint8_t(device), (uint8_t) 2, (uint8_t)true);
  return Wire.read() << 8 | Wire.read();
}

void getall() {
  Wire.beginTransmission(device);
  Wire.write(0x3B); // starting with register 0x3B (ACCEL_XOUT_H)
  Wire.endTransmission(false); // the parameter indicates that the Arduino will send a restart.
  // As a result, the connection is kept active.
  Wire.requestFrom(uint8_t(device), (uint8_t) 14, (uint8_t)true); // request a total of 7*2=14 registers

  // "Wire.read()<<8 | Wire.read();" means two registers are read and stored in the same int16_t variable
  accX = Wire.read() << 8 | Wire.read(); // reading registers: 0x3B (ACCEL_XOUT_H) and 0x3C (ACCEL_XOUT_L)
  accY = Wire.read() << 8 | Wire.read(); // reading registers: 0x3D (ACCEL_YOUT_H) and 0x3E (ACCEL_YOUT_L)
  accZ = Wire.read() << 8 | Wire.read(); // reading registers: 0x3F (ACCEL_ZOUT_H) and 0x40 (ACCEL_ZOUT_L)
  tRaw = Wire.read() << 8 | Wire.read(); // reading registers: 0x41 (TEMP_OUT_H) and 0x42 (TEMP_OUT_L)
  gyroX = Wire.read() << 8 | Wire.read(); // reading registers: 0x43 (GYRO_XOUT_H) and 0x44 (GYRO_XOUT_L)
  gyroY = Wire.read() << 8 | Wire.read(); // reading registers: 0x45 (GYRO_YOUT_H) and 0x46 (GYRO_YOUT_L)
  gyroZ = Wire.read() << 8 | Wire.read(); // reading registers: 0x47 (GYRO_ZOUT_H) and 0x48 (GYRO_ZOUT_L)

  Serial.print("59AX = "); Serial.print(toStr(accX));
  Serial.print(" | 61AY = "); Serial.print(toStr(accY));
  Serial.print(" | 63AZ = "); Serial.print(toStr(accZ));
  Serial.print(" | tmp = "); Serial.print((tRaw + 12412.0) / 340.0);
  Serial.print(" | 67GX = "); Serial.print(toStr(gyroX));
  Serial.print(" | 69GY = "); Serial.print(toStr(gyroY));
  Serial.print(" | 71GZ = "); Serial.print(toStr(gyroZ));
  Serial.println();
}

void scanne() {
  int nDevices = 0;
  Serial.println("Scanning...");
  for (byte address = 1; address < 127; ++address) {
    Wire.beginTransmission(address);
    byte error = Wire.endTransmission();
    if (error == 0) {
      Serial.print("I2C device found at address 0x");
      if (address < 16) {
        Serial.print("0");
      }
      Serial.print(address, HEX);
      Serial.println("  !");
      ++nDevices;
    } else if (error == 4) {
      Serial.print("Unknown error at address 0x");
      if (address < 16) {
        Serial.print("0");
      }
      Serial.println(address, HEX);
    }
  }
  if (nDevices == 0) {
    Serial.println("No I2C devices found\n");
  } else {
    Serial.println("done\n");
  }
}

void showPos() {
  char str[50];
  Serial.println();
  for (byte i = 0; i < 10; i++) {
    sprintf(str, "%2u   %4u   %6d", i, mypos.pos[i], mypos.wert[i]);
    Serial.println(str);
  }
}

void showMess() {
  for (byte i = 0; i < messAnz; i++) {
    Serial.print(toStr(messW[i]));
    if (i % 8 == 7) Serial.println();
  }
  Serial.println();
}

void showTims() {
  for (byte i = 0; i < messAnz; i++) {
    Serial.print(toStr(tims[i]));
    if (i % 8 == 7) Serial.println();
  }
  Serial.println();
}

void info() {
  msgF(F("messMs"), messMs);
  msgF(F("ObsReg "), obsReg);
  showMess();
  prlnF(F("obs x 59, y 61, z 63, t 65, gx 67, gy 69, gz 71"));
}

void startMess() {
  messCnt = messAnz;
  messPtr = 0;
  mess = true;
}

void doKomma(uint16_t was) {
  msgF(F("Komma"), was);
  myservo.write(wrtx);
  myservo.attach(serv);
  delay(1000);
  myservo.detach();
  startMess();
  serpos = wrty;
  myservo.write(serpos);
}

void prompt() {
  Serial.print(posp);
  Serial.print(">");
}

void doCmd(byte tmp) {
  if ((tmp >= '0') && (tmp <= '9')) {
    if (inpAkt) {
      inp = inp * 10 + (tmp - '0');
    } else {
      inpAkt = true;
      inp = tmp - '0';
    }
    Serial.print("\b\b\b\b");
    Serial.print(inp);
    return;
  }
  inpAkt = false;

  switch (tmp) {
    case 'a':   //
      prlnF(F("attached"));
      myservo.attach(serv);
      break;
    case 'd':   //
      prlnF(F("detached"));
      myservo.detach();
      break;
    case 'i':   //
      info();
      break;
    case 'l':   //
      getall();
      break;
    case 'm':   //
      msgF(F("Mess"), mess);
      startMess();
      break;
    case 'n':   //
      messAnz = inp;
      if (messAnz > messSiz) messAnz = messSiz;
      msgF(F("MessAnz"), messAnz);
      break;
    case 'o':   //
      obsReg = inp;
      msgF(F("Observe"), obsReg);
      break;
    case 'p':   //
      serpos = inp;
      msgF(F("Servo"), serpos);
      myservo.write(serpos);
      break;
    case 'r':   //
      prlnF(F("read"));
      EEPROM.get(0, mypos);
      showPos();
      break;
    case 'S':   //
      scanne();
      break;
    case 's':   //
      mypos.pos[posp] = serpos;
      showPos();
      break;
    case 't':   //
      messMs = inp;
      msgF(F("messMs"), inp);
      showTims();
      break;
    case 'v':   //
      verbo = !verbo;
      if (verbo) {
        prlnF(F("Verbose an"));
      } else {
        prlnF(F("Verbose aus"));
      }
      break;
    case 'w':   //
      prlnF(F("write"));
      EEPROM.put(0, mypos);
      break;
    case 'x':   //
      wrtx = inp;
      msgF(F("wrtx"), inp);
      break;
    case 'y':   //
      wrty = inp;
      msgF(F("wrty"), inp);
      break;
    case '+':   //
      serpos += 5;
      msgF(F("Servo"), serpos);
      myservo.write(serpos);
      break;
    case '#':   //
      if (inp < 10) {
        posp = inp;
        msgF(F("Posi"), mypos.pos[posp]);
        myservo.write(mypos.pos[posp]);
      } else msgF(F("?inp "), inp);
      break;
    case ',':   //
      doKomma(inp);
      break;
    case '-':   //
      serpos -= 5;
      msgF(F("Servo"), serpos);
      myservo.write(serpos);
      break;

    default:
      Serial.print(tmp);
      Serial.println ("?  0..5, +, -, show, verbose");
  } //case
  prompt();
}

void setup() {
  const char info[] = "MPU6050_hh " __DATE__  " " __TIME__;
  Serial.begin(38400);
  Serial.println(info);
  Wire.begin();
  Wire.beginTransmission(device);
  Wire.write(0x6B); // PWR_MGMT_1 register
  Wire.write(0); // wake up!
  Wire.endTransmission(true); //If true, endTransmission() sends a stop message
  prompt();
}


void loop() {
  if (Serial.available() > 0) {
    doCmd( Serial.read());
  } // serial

  currMs = millis();

  if (currMs - prevMs >= messMs) {
    prevMs = currMs;
    if (mess) {
      messW[messPtr] = getOne();
      tims[messPtr++] = currMs & 0xFF;
      if (messPtr == 3) myservo.attach(serv);
      messCnt--;
      if (messCnt == 0) {
        myservo.detach();
        mess = false;
        info();
      }
    }
  } // timer
}

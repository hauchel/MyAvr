// Magnetfeldmessung
// SCL   A5 yell    1
// SDA   A4 grn     2
// Servo  9 ws
#include <Wire.h>
#include <QMC5883L.h>
#include <Servo.h>
#include <EEPROM.h>

QMC5883L comp;

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
const uint16_t messSiz = 100;
int16_t messX[messSiz], messY[messSiz], messZ[messSiz];
int16_t wrtx = 20, wrty = 120;
uint8_t tims[messSiz];
uint16_t messAnz = 50;
uint16_t messCnt;
uint16_t messPtr;
unsigned long messMs = 10;

Servo myservo;
byte device = 0x68;


int16_t accX, accY, accZ; // Raw register values (accelaration,)
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

void getall() {
  comp.readRaw(&accX, &accY, &accZ);
  Serial.print("AX = "); Serial.print(toStr(accX));
  Serial.print(" | AY = "); Serial.print(toStr(accY));
  Serial.print(" | AZ = "); Serial.print(toStr(accZ));
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
  char str[60];
  for (byte i = 0; i < messAnz; i++) {
    sprintf(str, "%2u   %6d  %6d  %6d", i, messX[i], messY[i], messZ[i]);
    Serial.println(str);
  }
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
  showMess();
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
    case 'h':   //
      msgF(F("Hz 10, 50 100 200"), inp);
      comp.setSamplingRate(inp);
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
      msgF(F("overs 512, 256, 128,64"), inp);
      comp.setOversampling(inp);
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
        serpos=mypos.pos[posp];
        myservo.write(serpos);
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
  const char info[] = "QMC5883 " __DATE__  " " __TIME__;
  Serial.begin(38400);
  Serial.println(info);
  Wire.begin();
  comp.init();
  comp.setSamplingRate(3);

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
      comp.readRaw(&accX, &accY, &accZ);
      messX[messPtr] = accX;
      messY[messPtr] = accY;
      messZ[messPtr] = accZ;
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

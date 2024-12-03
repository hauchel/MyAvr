/* Spender RFID chips mit 2 Servos
   Typical pin layout used:
   -----------------------------------
               MFRC522      Arduino
               Reader/PCD   Uno
   Signal      Pin          Pin
   -----------------------------------
   VCC
   RST/Reset   RST          9     white
   GND
   IRQ
   SPI MISO    MISO         12    green
   SPI MOSI    MOSI         11    blue
   SPI SCK     SCK          13    yell
   SPI SS      SDA(SS)      10    lila 
   SOUND_PIN
   SERVO1_PIN
   SERVO2_PIN
   HALT_PIN
   GO_PIN
*/


#include <SPI.h>
#include <MFRC522.h>
#include <EEPROM.h>
#include <SoftwareSerial.h>
#include "helper.h"

#define SS_PIN 10     //slave select
#define RST_PIN 9     //reset 
#define SOUND_PIN 8   // Beeper, high for sound
#define HALT_PIN 5      // stop execution
#define GO_PIN 4        // start, if pressed on boot run prog 1
#define SWS_RX_PIN 3    // from bahn, orange
#define SWS_TX_PIN 2    // to bahn, yellow

MFRC522 mfrc522(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;    //create a MIFARE_Key struct
SoftwareSerial mySerial =  SoftwareSerial(SWS_RX_PIN, SWS_TX_PIN);

int writeme = 0;          // if 1: write card
bool verbo = false;
bool check = false;       // auto-detect card in loop
byte cardNo = 0;          // holds current cardno (read and to write)

const byte anzServ = 2; //
//const byte progLen = 48;
//byte prog[progLen];    // not used
//byte progp = 0;        // not used
const byte anzRum = 40;   // from Bahn
char rum[anzRum];
byte rumP = 0;
bool bahnPending = false; // waiting for Exec complete
//#include "namen_spend.h"

struct epromData {
  uint16_t pos[anzServ][10];
};
epromData mypos;
const uint16_t epromAdr = 700;// depends on 1024-len(epromData)

#include "rfids.h"
const byte anzAbl = 6;      // Ablagen+1 (1..5)
byte ablag[anzAbl];          //
byte state;
byte ablRaus = 1;           // next cardno to output
byte ablZiel = 1;           // where to put

void handleRum(char c) {
  if (c == 10) return;
  if (c != 13) {
    rum[rumP] = c;
    rumP++;
    rum[rumP] = 0;
    if (rumP < (anzRum - 2)) return;
  }
  Serial.print(">>");
  Serial.println(rum);
  if (strncmp(rum, "OK", 2) == 0) {
    bahnPending = false;
    Serial.println(F("weiter"));
  }
  rumP = 0;
  rum[rumP] = 0;
}

void doWait(bool lies) {
  // if true tries to read card and sets cardNo
  while (bahnPending) {
    if (digitalRead(HALT_PIN) == LOW) {
      state = 115;
      return;
    }
    if (lies) {
      if (mfrc522.PICC_IsNewCardPresent()) {
        cardNo = newcard();
      }
    }
    Serial.write('.');
    delay(100);
    while (mySerial.available() > 0) {
      handleRum(mySerial.read());
    }
    if (digitalRead(GO_PIN) == LOW) {
      bahnPending = false;
      return;
    }
  }
}

void waitRum() {
  doWait(false);
}

bool waitGreen() {
  while (digitalRead(GO_PIN) == HIGH) {
    Serial.println(F("Press Go"));
    beep(10);
    for (byte i = 0; i < 20; i++) {
      if (digitalRead(HALT_PIN) == LOW) {
        state = 115;
        return false;
      }
      if (digitalRead(GO_PIN) == LOW) {
        return true;
      }
      delay(10);
    }
  }
  return true;
}

byte callBahn(byte lo) {
  // sends prog# to exec
  if (verbo) msgF(F("callBahn"), lo);
  mySerial.print(lo);
  mySerial.print('z');
  bahnPending = true;
  waitRum();
  return 0;
}

byte posStepBahn(byte lo) {
  // sends Stepper command
  if (verbo) msgF(F("stepBahn"), lo);
  mySerial.print("0ea");
  mySerial.print(lo);
  mySerial.print("#?");
  bahnPending = true;
  waitRum();
  return 0;
}

void cleanAblag() {
  for (byte i = 1; i < anzAbl; i++) {
    ablag[i] = 0;
  }
  ablRaus = 1;
}

void showAblag() {
  for (byte i = 1; i < anzAbl; i++) {
    Serial.print(ablag[i]);
    Serial.print(" ");
  }
  Serial.println();
}

byte freeAblag() {
  // returns number to put, 0 no avail
  for (byte i = 1; i < anzAbl; i++) {
    if (ablag[i] == 0) return i;
  }
  return 0;
}

byte maxAblag() {
  // returns position of highest card in Ablage, 0 no avail
  byte maxi = 0;
  byte maxp = 0;
  for (byte i = 1; i < anzAbl; i++) {
    if (ablag[i] > maxi ) {
      maxi = ablag[i];
      maxp = i;
    }
  }
  if (verbo) msgF(F("maxAbl"), maxi);
  return maxp;
}

byte inAblag(byte n) {
  // returns position of card n in Ablage, 0 no avail
  for (byte i = 1; i < anzAbl; i++) {
    if (ablag[i] == n ) {
      return i;
    }
  }
  return 0;
}

byte handle3() {
  // Neue Card bekannt,  Möglichkeiten: direkt raus, ablegen, direkt loopen, anderen loopen
  if (ablRaus == cardNo) {
    return  20;
  }
  ablZiel = freeAblag();
  if (ablZiel == 0) {
    return 10;
  }
  return 4;
}

void switchState(byte to) {
  msgF(F("State to"), to);
  if (to == 3) {
    to = handle3();
    msgF(F("handle3 "), to);
  }
  switch (to) {
    case 0:
      mySerial.print("V10_"); // klapp and delt
      callBahn(1);
      break;
    case 1:      // Position Stepper
      callBahn(2);
      break;
    case 2:   // read card
      doFeed(2);
      if (cardNo == 0) {
        state = 40;
        return;
      }
      break;
    case 4:   // card  in tray
      callBahn(12);
      callBahn(3);
      break;
    case 5:     // ablegen
      if (ablZiel == 0) {
        state = 102;
        return;
      }
      ablag[ablZiel] = cardNo;
      posStepBahn(ablZiel + 1); // #2..#6
      callBahn(4);
      break;
    case 6:     // aufnehmen
      callBahn(6);
      break;
    case 10:     // grössten von Abl
      callBahn(10);
      ablZiel = maxAblag();
      if (ablZiel == 0) {
        state = 103;
        return;
      }
      break;
    case 11:     // positioniere ablZiel
      if (ablZiel == 0) {
        state = 103;
        return;
      }
      ablag[ablZiel] = 0;
      posStepBahn(ablZiel + 1);
      callBahn(6);
      posStepBahn(1);
      if (!waitGreen()) return;
      callBahn(2); // feed position, cardNo in feeder
      delay(300);
      break;
    case 20:   // card  in tray und raus
      callBahn(12);
      callBahn(3);
      callBahn(7);
      callBahn(8);
      callBahn(9);
      ablRaus += 1;
      break;
    case 21:   // check ob Ablraus in Ablage
      ablZiel = inAblag(ablRaus);
      if (ablZiel == 0) {
        msgF(F("nicht in Ablag"), ablRaus);
        state = 22;
        return;
      }
      ablag[ablZiel] = 0;
      posStepBahn(ablZiel + 1);
      callBahn(6);
      callBahn(7);
      callBahn(8);
      callBahn(9);
      ablRaus += 1;
      break;
    case 40:   // read err (von 2)
      msgF(F("rfidWakeup"), rfidWakeup());
      cardNo = newcard();
      if (cardNo > 0) {
        state = 3;
        return;
      }
      break;
    default:
      msgF(F("State invalid"), to);
      state = 113;
      return;
  }
  state = to;
}

void stater(byte stasta) {
  state = stasta - 1; //
  msgF(F("stater"), state);
  while (state < 100) {
    if (digitalRead(HALT_PIN) == LOW) {
      state = 115;
      return;
    }
    waitRum();
    msgF(F("Current State "), state);
    switch (state) {
      case 3:
        handle3();  // either 3,20
        switchState(state);
        break;
      case 5:
        switchState(1); //loop
        break;
      case 11:
        switchState(3);
        break;
      case 21:
        switchState(21);
        break;
      case 22:
        switchState(1);
        break;
      case 40:
        switchState(40);
        break;
      default:
        switchState(state + 1);
    }
  }
}

void doFeed(byte was) {
  // sets cardNo # read else 0
  // was 1 lesen+raus,   2 lesen only
  cardNo = 0;
  mySerial.print("11z");
  bahnPending = true;
  doWait(true);
  if (was == 2) return;
  callBahn(12);
}

void prompt() {
  char str[50];
  sprintf(str, "S%3u C%2u A%2u P%2u>", state, cardNo, ablZiel, bahnPending);
  Serial.print(str);
}

void docmd(byte tmp) {
  if (doNum(tmp)) {
    Serial.print(char(tmp));
    return;
  }
  tmp = doVT100(tmp);
  if (tmp == 0) return;
  switch (tmp) {
    case 'a':   //
      break;
    case 'A':   //
      ablZiel = inp;
      msgF(F("ablZiel"), ablZiel);
      break;
    case 'B':   //
      ablag[ablZiel] = inp;
      Serial.println();
      showAblag();
      break;
    case 'c':   //
      check = !check;
      if (check)  prlnF(F("Check an"));
      else        prlnF(F("Check aus"));
      break;
    case 'C':   //
      cardNo = inp;
      msgF(F("cardNo"), cardNo);
      break;
    case 'd':   //
      break;
    case 'e':   //
      break;
    case 'f':   //
      break;
    case 'g':   //
      break;
    case 'h':   //
      break;
    case 'i':   //
      showAblag();
      break;
    case 'j':   //
      switchState(inp);
      break;
    case 'J':   //
      bahnPending = false;
      stater(inp);
      break;
    case 'k':   //
      prlnF(F("Write off"));
      writeme = 0;
      break;
    case 'K':   //
      prlnF(F("Write on"));
      writeme = 1;
      break;
    case 'n':   //
      newcard();
      break;
    case 'p':   //
      break;
    case 'q':   //
      break;
    case 'Q':   //
      break;
    case 'r':   //
      //readProg(inp, true);
      //showProg();
      break;
    case 'R':   //
      //readProg(inp, false);
      //showProg();
      break;
    case 's':   //
      break;
    case 't':   //
      break;
    case 'u':   //
      msgF(F("rfidWakeup"), rfidWakeup());
      cardNo = newcard();
      break;
    case 'U':   //
      msgF(F("rfidReqA"), rfidReqA());
      newcard();
      break;
    case 'v':   //
      verbo = !verbo;
      if (verbo)  prlnF(F("Verbose an"));
      else        prlnF(F("Verbose aus"));
      break;
    case 'w':   //
      //writeProg(prognum);
      break;
    case 'W':   //
      //writeProg(inp);
      //prognum = inp;
      break;
    case 'x':   //
      break;
    case 'y':   //
      break;
    case 'z':   //
      break;

    case 13:
      break;
    case '+':   //
      cardNo++;
      cardNoChange();
      break;
    case '-':   //
      cardNo--;
      cardNoChange();
      break;
    case '?':   //
      mySerial.write('?');
      break;
    case 228:   // ä
      break;
    case 246:   // ö
      posStepBahn(inp);
      break;
    case 252:   // ü
      callBahn(inp);
      break;
    default:
      if (verbo) {
        Serial.println();
        Serial.println(tmp);
      } else {
        Serial.print(tmp);
        Serial.println ("?  0..5, +, -, show, verbose");
      }
  } //case
  prompt();
}

void setup() {
  Serial.begin(38400);
  Serial.println(F( "rfid_spend " __DATE__ " " __TIME__));
  SPI.begin();               // Init SPI bus
  mfrc522.PCD_Init();        // Init MFRC522 card

  pinMode(SWS_RX_PIN, INPUT_PULLUP);
  pinMode(SWS_TX_PIN, OUTPUT);
  mySerial.begin(38400);

  pinMode(SOUND_PIN, OUTPUT);
  digitalWrite(SOUND_PIN, 0);
  beep(20);
  // Prepare the security key for the read and write functions - all six key bytes are set to 0xFF at chip delivery from the factory.
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;//keyByte is defined in the "MIFARE_Key" 'struct' definition in the .h file of the library
  }
  EEPROM.get(epromAdr, mypos);
  pinMode(HALT_PIN, INPUT_PULLUP);
  pinMode(GO_PIN, INPUT_PULLUP);
  prompt();
}

void loop() {
  if (check) {
    if (mfrc522.PICC_IsNewCardPresent()) {
      newcard();
    }
  }

  while (mySerial.available() > 0) {
    handleRum(mySerial.read());
  }

  if (Serial.available() > 0) {
    docmd(Serial.read());
  }

  if (digitalRead(GO_PIN) == LOW) {
    if (digitalRead(HALT_PIN) == LOW) {
      doFeed(1);
    } else {
      prlnF(F("Press Red also"));
    }
  }

  currMs = millis();
  if (currMs - prevMs >= tickMs) {
    prevMs = currMs;
  } // timer
} // loop

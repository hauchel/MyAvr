/* Spender RFID chips mit 2 Servos
   Typical pin layout used:
   -----------------------------------
               MFRC522      Arduino
               Reader/PCD   Uno
   Signal      Pin          Pin
   -----------------------------------
   RST/Reset   RST          9     white
   SPI SS      SDA(SS)      10    lila
   SPI MOSI    MOSI         11    blue
   SPI MISO    MISO         12    green
   SPI SCK     SCK          13    yell
   SOUND_PIN
   SERVO1_PIN
   SERVO2_PIN
   HALT_PIN
   GO_PIN
*/


#include <SPI.h>
#include <MFRC522.h>
#include <EEPROM.h>
#include <Servo.h>
#include <SoftwareSerial.h>
#include "helper.h"

#define SS_PIN 10     //slave select
#define RST_PIN 9     //reset 
#define SOUND_PIN 8   // Beeper, high for sound
#define SERVO1_PIN 7
#define SERVO2_PIN 6
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
byte cardNo = 0;          // holds current cardno (for read and write)

const byte anzServ = 2; //
const byte servMap[anzServ] = {SERVO1_PIN, SERVO2_PIN};
Servo myservo[anzServ];
uint16_t serpos[anzServ]; // current position
byte sersel = 0;          // selected Servo
byte posp[anzServ];       // position
const byte progLen = 48;
const byte anzRum = 40;   // from Bahn
char rum[anzRum];
byte rumP = 0;
bool bahnPending = false; // waiting for Exec complete
#include "namen_spend.h"

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


#include "../../Bahn/texte.h"
// Mini exec from bahn


byte callBahn(byte lo) {
  // sends prog# to exec
  if (verbo) msgF(F("callBahn"), lo);
  mySerial.print(lo);
  mySerial.print('z');
  bahnPending = true;
  return 0;
}

byte posStepBahn(byte lo) {
  // sends Stepper command
  if (verbo) msgF(F("stepBahn"), lo);
  mySerial.print("0ea");
  mySerial.print(lo);
  mySerial.print('#');
  bahnPending = true;
  return 0;
}

void cleanAbl() {
  for (byte i = 1; i < anzAbl; i++) {
    ablag[i] = 0;
  }
  ablRaus = 1;
}

void showAbl() {
  for (byte i = 1; i < anzAbl; i++) {
    Serial.print(ablag[i]);
    Serial.print(" ");
  }
  Serial.println();
}

byte freeAbl() {
  // returns number to put, 0 not avail
  for (byte i = 1; i < anzAbl; i++) {
    if (ablag[i] == 0) return i;
  }
  return 0;
}

void switchState(byte to) {
  msgF(F("State to"), to);
  switch (to) {
    case 0:
      cleanAbl();
      mySerial.print("V10_"); // klapp and delt
      callBahn(1);
      state = 0;
      break;
    case 1:      // Position Stepper
      callBahn(2);
      state = 1;
      break;
    case 2:   // read card
      cardNo = doFeed(2);
      if (cardNo == 0) {
        state = 101;
        return;
      }
      // drei Möglichkeiten: ablegen, direkt loopen, anderen loopen
      ablZiel = freeAbl();
      if (ablZiel == 0) {
        state = 102;
        return;
      }
      state=2;
      break;
    case 3:   // card  in tray
      readProg(3, true); //schieb raus
      execProg();
      callBahn(3);
      state = 3;
      break;
    case 4:     // ablegen 
      ablag[ablZiel] = cardNo;
      posStepBahn(ablZiel + 1); // #2..#6
      callBahn(4);
      state = 4;
      break;
    case 6:     // aufnehmen
      callBahn(6);
      state = 6;
      break;
    default:
      msgF(F("State invalid"), to);
      state = 113;
      return;
  }
}

void stater() {
  while (state < 100) {
    if (digitalRead(HALT_PIN) == LOW) {
      state = 115;
      return;
    }
    while (bahnPending) {
      if (digitalRead(HALT_PIN) == LOW) {
        state = 115;
        return;
      }
      Serial.write('.');
      delay(100);
      while (mySerial.available() > 0) {
        handleRum(mySerial.read());
      }
    }


    msgF(F("Current State "), state);

    switchState(state + 1);
  }
}

void selectServo(byte b) {
  char str[50];
  char nam[20];
  if (b >= anzServ) b = anzServ - 1;
  sersel = b;
  strcpy_P(nam, (char *)pgm_read_word(&(servNam[sersel])));
  Serial.println();
  sprintf(str, "Servo %2u %s at %2u %4u", sersel, nam, posp[sersel], serpos[sersel]);
  Serial.println(str);
}

void writeServo(byte senum, uint16_t wert) {
  if (wert > mypos.pos[senum][9]) wert = mypos.pos[senum][9];
  if (wert < mypos.pos[senum][0]) wert = mypos.pos[senum][0];
  serpos[senum] = wert;
  myservo[senum].write(wert);
  if (verbo) msgF(F("writeServo"), wert);
}

void setServo(byte p) {
  if (p < 10) {
    posp[sersel] = p;
    if (mypos.pos[sersel][p] <= mypos.pos[sersel][9]) {
      msgF(F("Posi"), mypos.pos[sersel][p]);
      writeServo(sersel, mypos.pos[sersel][p]);
    } else {
      msgF(F("Not servod "), p);
    }
  } else msgF(F("Invalid Position "), p);
}

byte exepSelServo(byte lo) {
  if (lo >= anzServ) {
    return 13;
  }
  sersel = lo;
  return 0;
}

byte exepSetPos(byte lo) {
  if (lo < 10) {
    posp[sersel] = lo; //verwirrt?
    writeServo(sersel, mypos.pos[sersel][lo]);
    return 0;
  }
  switch (lo) {
    case 0xE:   //
      myservo[sersel].attach(servMap[sersel]);
      break;
    case 0xF:   //
      myservo[sersel].detach();
      break;
    default:
      msgF(F("Setpos not implemented "), lo);
      return 1;
  }
  return 0;
}

byte exepSpecial(byte lo) {
  switch (lo) {
    case 0xA:   //
      msgF(F("Mess A"), progp);
      return 30;
    case 0xB:   //
      msgF(F("Mess B"), progp);
      return 31;
    case 0xC:   //
      msgF(F("Mess C"), progp);
      return 32;
    case 0xD:   //
      // msgF(F("EOP"), progp);
      return 9;
    case 0xE:   //
      // msgF(F("EOP"), progp);
      return 9;
    case 0xF:   //
      // msgF(F("EOP"), progp);
      return 9;
    default:
      msgF(F("Special not implemented "), lo);
  }
  return 1;
}

byte exep2byte(byte lo) {
  switch (lo) {
    case 0:   //
      delay(10 * prog[progp] - 1);
      break;
    default:
      msgF(F("twobyte not implemented "), lo);
      return 1;
  }
  progp++;
  return 0;
}

byte execOne() {
  byte lo, hi;
  if (progp >= progLen) {
    msgF(F(" ExecProg progp? "), progp);
    return 10;
  }
  lo = prog[progp];
  progp++;
  hi = lo >> 4;
  lo = lo & 0x0F;

  switch (hi) {
    case 1:   //
      return exepSelServo(lo);
    case 2:   //
      return exepSetPos(lo);
    case 0xB:   // Bahn!
      return callBahn(lo);
      break;
    case 0xC:   // Bahn!
      return posStepBahn(lo);
      break;
    case 0xE:   //
      return exep2byte(lo);
    case 0xF:   //
      return exepSpecial(lo);
    default:
      msgF(F("Not implemented "), hi);
      return 1;
  } //case
}

byte execProg() {
  byte err = 0;
  char txt[40];
  char str[50];
  progp = 0;
  traceLin = 255;
  Serial.println();

  while (err == 0) {
    if (Serial.available() > 0) return 5;
    if (digitalRead(HALT_PIN) == LOW) return 4;
    if (progp != traceLin) {
      traceLin = progp;
      if (trace) {
        decodProg(txt, progp);
        sprintf(str, "%2u  %02X  %-10s", progp, prog[progp], txt);
        Serial.println(str);
      }
    }
    err = execOne();
  } // while
  return err;
}

void showPos() {
  char str[50];
  char nam[20];
  strcpy_P(nam, (char *)pgm_read_word(&(servNam[sersel])));
  Serial.println();
  sprintf(str, "akt  %5u %s",  myservo[sersel].read(), nam);
  Serial.println(str);
  for (byte i = 0; i < 10; i++) {
    sprintf(str, "%2u   %5u ", i, mypos.pos[sersel][i]);
    Serial.println(str);
  }
}

byte doFeed(byte was) {
  // return card # read else 0
  // was 1 lesen+raus,   2 lesen only
  byte zwi;
  byte card = 0;
  readProg(2, true);
  zwi = execProg();
  msgF(F("exec2 is "), zwi);
  if (zwi != 9) return 0;
  // read?
  if (mfrc522.PICC_IsNewCardPresent()) {
    card = newcard();
  } else {
    msgF(F("doit no newcard"), 0);
  }
  if (was == 2) return card;
  readProg(3, true);
  zwi = execProg();
  msgF(F("exec3 is "), zwi);
  return card;
}

void prompt() {
  char str[50];
  sprintf(str, "C%2u S%2u P%2u>", cardNo, state, bahnPending);
  Serial.print(str);
}

void docmd(byte tmp) {
  byte zwi;
  if (doNum(tmp)) {
    Serial.print(char(tmp));
    return;
  }
  tmp = doVT100(tmp);
  if (tmp == 0) return;
  switch (tmp) {
    case 'a':   //
      prlnF(F("attached"));
      myservo[sersel].attach(servMap[sersel]);
      progge (0x2E);
      break;
    case 'b':   //
      beep(inp);
      break;
    case 'B':   //
      beepErr(inp);
      break;
    case 'c':   //
      check = !check;
      if (check)  prlnF(F("Check an"));
      else        prlnF(F("Check aus"));
      break;
    case 'd':   //
      prlnF(F("detached"));
      myservo[sersel].detach();
      progge (0x2F);
      break;
    case 'e':   //
      selectServo(inp);
      progge (0x10 + sersel);
      break;
    case 'f':   //
      prlnF(F("Fetch"));
      EEPROM.get(epromAdr, mypos);
      showPos();
      break;
    case 'g':   //
      zwi = execProg();
      msgF(F("execProg is "), zwi);
      break;
    case 'h':   //
      prlnF(F("write Pos"));
      EEPROM.put(epromAdr, mypos);
      break;
    case 'i':   //
      showPos();
      showAbl();
      break;
    case 'j':   //
      switchState(inp);
      break;
    case 'J':   //
      stater();
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
      writeServo(sersel, inp);
      msgF(F("Servo"), serpos[sersel]);
      break;
    case 'q':   //
      mypos.pos[sersel][0] = inp;
      msgF(F("Min set"), inp);
      break;
    case 'Q':   //
      mypos.pos[sersel][9] = inp;
      msgF(F("Max set"), inp);
      break;
    case 'r':   //
      readProg(inp, true);
      showProg();
      break;
    case 'R':   //
      readProg(inp, false);
      showProg();
      break;
    case 's':   //
      mypos.pos[sersel][posp[sersel]] = serpos[sersel];
      showPos();
      break;
    case 't':   //
      trace = !trace;
      if (trace)   prlnF(F("Trace an"));
      else         prlnF(F("Trace aus"));
      break;
    case 'u':   //
      msgF(F("rfidWakeup"), rfidWakeup());
      newcard();
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
      writeProg(prognum);
      break;
    case 'W':   //
      writeProg(inp);
      prognum = inp;
      break;
    case 'x':   //
      zwi = execOne();
      if (zwi == 0) redraw();
      else msgF(F("execOne is "), zwi);
      break;
    case 'y':   //
      delatProg();
      redraw();
      break;
    case 'z':   //
      if (readProg(inp, true)) {
        zwi = execProg();
        msgF(F("execProg is"), zwi);
      }
      break;

    case 13:
      redraw();
      break;
    case '+':   //
      cardNo++;
      cardNoChange();
      break;
    case '-':   //
      cardNo--;
      cardNoChange();
      break;
    case ',':   //
      insProg(inp);
      Serial.print(",");
      return; //avoid prompt
    case '.':   //
      prog[progp] = byte(inp);
      dirty = true;
      redraw();
      break;
    case '#':   //
      setServo(inp);
      progge(0x20 + inp);
      break;
    case 193:   //up
      progp -= 1;
      redraw();
      break;
    case 194:   //dwn
      progp += 1;
      redraw();
      break;
    case 228:   // ä
      teach = !teach;
      if (teach) {
        prlnF(F("Teach an"));
      } else {
        prlnF(F("Teach aus"));
        redraw();
      }
      break;
    case 246:   // ö
      callBahn(inp);
      break;
    case 252:   // ü
      posStepBahn(inp);
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

void handleRum(char c) {
  if (verbo) Serial.println(uint8_t(c));
  if (c == 10) return;
  if (c != 13) {
    rum[rumP] = c;
    rumP++;
    rum[rumP] = 0;
    if (rumP < (anzRum - 2)) return;
  }
  Serial.print(">>");
  Serial.println(rum);
  rumP = 0;
  if (strncmp(rum, "OK", 2) == 0) {
    msgF(F(" ok erkannt"), 0);
  }

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

  if (Serial.available() > 0) {
    Serial.println();
    docmd( Serial.read());
  }

  while (mySerial.available() > 0) {
    handleRum(mySerial.read());
  }

  if (digitalRead(GO_PIN) == LOW) {
    doFeed(3);
  }

  currMs = millis();
  if (currMs - prevMs >= tickMs) {
    prevMs = currMs;
  } // timer
} // loop

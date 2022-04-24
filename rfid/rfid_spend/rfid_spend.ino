/* Spender RFID chips mit 2 Servos
   Typical pin layout used:
   -----------------------------------
               MFRC522      Arduino
               Reader/PCD   Uno/101
   Signal      Pin          Pin
   -----------------------------------
   RST/Reset   RST          9             white
   SPI SS      SDA(SS)      10            lila
   SPI MOSI    MOSI         11 / ICSP-4   blue
   SPI MISO    MISO         12 / ICSP-1   green
   SPI SCK     SCK          13 / ICSP-3   yell
   SOUND_PIN
   SERVO1_PIN
   SERVO2_PIN
  Idea: store number in sector (1) and blockAddr (4)

*/


#include <SPI.h>
#include <MFRC522.h>
#include <EEPROM.h>
#include <Servo.h>
#include "helper.h"

#define SS_PIN 10  //slave select pin
#define RST_PIN 9  //reset pin
#define SOUND_PIN 8 // Beeper, high for sound
#define SERVO1_PIN 7
#define SERVO2_PIN 6

MFRC522 mfrc522(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;//create a MIFARE_Key struct named 'key', which will hold the card information

byte sector         = 1;
byte blockAddr      = 4;
byte dataBlock[]    = {
  0x01, 0x02, 0x03, 0x04, //  1,  2,   3,  4,
  0x05, 0x06, 0x07, 0x08, //  5,  6,   7,  8,
  0x09, 0x0a, 0xff, 0x0b, //  9, 10, 255, 11,
  0x0c, 0x0d, 0x0e, 0x0f  // 12, 13, 14, 15
};

int writeme = 0;     // if 1: write card
bool verbo = false;
bool check = false;
byte readbackblock[18];//This array is used for reading out a block. The MIFARE_Read method requires a buffer that is at least 18 bytes to hold the 16 bytes of a block.
byte cardNo = 1;

const byte anzServ = 2; //
const byte servMap[anzServ] = {SERVO1_PIN, SERVO2_PIN};
Servo myservo[anzServ];
uint16_t serpos[anzServ]; // current position
byte sersel = 0;          // selected Servo
byte posp = 0;            // position

struct epromData {
  uint16_t pos[anzServ][10];
};
epromData mypos;
const uint16_t epromAdr = 700;// depends on 1024-len(epromData)

void selectServo(byte b) {
  if (b >= anzServ) b = anzServ - 1;
  sersel = b;
}


#include "../../Bahn/texte.h"
// Mini exec from bahn


byte exepSelServo(byte lo) {
  if (lo >= anzServ) {
    return 13;
  }
  sersel = lo;
  return 0;
}

byte exepSetPos(byte lo) {
  if (lo < 10) {
    posp = lo; //verwirrt?
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
      msgF(F("EOP"), progp);
      return 9;
    case 0xE:   //
      msgF(F("EOP"), progp);
      return 9;
    case 0xF:   //
      msgF(F("EOP"), progp);
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

void writeServo(byte senum, uint16_t wert) {
  if (wert > mypos.pos[senum][9]) {
    wert = mypos.pos[senum][9];
  }
  if (wert < mypos.pos[senum][0]) {
    wert = mypos.pos[senum][0];
  }
  serpos[senum] = wert;
  myservo[senum].write(wert);
  if (verbo) msgF(F("writeServo"), wert);
}

void setServo(byte p) {
  if (p < 10) {
    posp = p;
    if (mypos.pos[sersel][p] <= mypos.pos[sersel][9]) {
      msgF(F("Posi"), mypos.pos[sersel][p]);
      writeServo(sersel, mypos.pos[sersel][posp]);
    } else {
      msgF(F("Not servod "), p);
    }
  } else msgF(F("Invalid Position "), p);
}

void showPos() {
  char str[50];
  Serial.println();
  sprintf(str, "akt  %5u ",  myservo[sersel].read());
  Serial.println(str);
  for (byte i = 0; i < 10; i++) {
    sprintf(str, "%2u   %5u ", i, mypos.pos[sersel][i]);
    Serial.println(str);
  }
}
int tryread() {
  // returns true if success
  int tmp = mfrc522.PICC_ReadCardSerial();
  if (verbo) Serial.print("tryread: ReadCardSerial ");
  if (tmp) {
    if (verbo) Serial.println("OK");
  }
  else {
    msgF(F("Tryread Err"), tmp);
  }
  return (tmp);
}

void dump_byte_array(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
  }
}
void rfidDisconn () {
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
}

void rfidRead(byte blocknum) {
  byte buffer[18];
  byte size = sizeof(buffer);
  MFRC522::StatusCode status;
  status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(blocknum, buffer, &size);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("rfid Read failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }

  Serial.print(F("Data in block ")); Serial.print(blocknum); Serial.println(F(":"));
  dump_byte_array(buffer, 16); Serial.println();
  Serial.println();
}

void rfidWakeup() {
  byte atqa_answer[2];
  byte atqa_size = 2;
  mfrc522.PICC_WakeupA(atqa_answer, &atqa_size);
}

void newcard() {
  byte trailerBlock   = 7;
  MFRC522::StatusCode status;
  byte buffer[18];
  byte size = sizeof(buffer);
  if ( ! tryread()) {
    Serial.println("newcard: aborting");
    return;
  }
  // Authenticate
  if (verbo) Serial.println(F("Authenticating using key A..."));
  status = (MFRC522::StatusCode) mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("PCD_Authenticate() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }
  // Show the whole sector as it currently is
  if (verbo) {
    Serial.println(F("Current data in sector:"));
    mfrc522.PICC_DumpMifareClassicSectorToSerial(&(mfrc522.uid), &key, sector);
    Serial.println();
  }
  status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(blockAddr, buffer, &size);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("newcard Read failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
  }

  Serial.print(F("Data in block ")); Serial.print(blockAddr); Serial.println(F(":"));
  dump_byte_array(buffer, 16); Serial.println();
  Serial.println();

  if (writeme != 0) {
    Serial.print(F("Writing data into block ")); Serial.print(blockAddr);
    Serial.println(F(" ..."));
    dump_byte_array(dataBlock, 16); Serial.println();
    status = (MFRC522::StatusCode) mfrc522.MIFARE_Write(blockAddr, dataBlock, 16);
    if (status != MFRC522::STATUS_OK) {
      Serial.print(F("MIFARE_Write() failed: "));
      Serial.println(mfrc522.GetStatusCodeName(status));
    }
    Serial.println();
  }
  if (check) {
    rfidDisconn ();
  }
}

void beep(int d) {
  digitalWrite(SOUND_PIN, 1);
  delay(d);
  digitalWrite(SOUND_PIN, 0);
}

void cardNoChange() {
  msgF (F("Card is"), cardNo);
  dataBlock[4] = cardNo;
  dataBlock[5] = 255 - cardNo;
}

void docmd(byte tmp) {
  byte zwi;
  if (doNum(tmp)) return;
  tmp = doVT100(tmp);
  if (tmp == 0) return;
  switch (tmp) {
    case 'a':   //
      prlnF(F("attached"));
      myservo[sersel].attach(servMap[sersel]);
      progge (0x2E);
      break;
    case 'b':   //
      beep(20);
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
      msgF(F("Servo is "), sersel);
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
      mypos.pos[sersel][posp] = serpos[sersel];
      showPos();
      break;
    case 't':   //
      trace = !trace;
      if (trace)   prlnF(F("Trace an"));
      else         prlnF(F("Trace aus"));
      break;
    case 'u':   //  wakeUp
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

    case 13:
      redraw();
      break;
    case 'z':   //
      progp = inp;
      msgF(F("Progp"), progp);
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
    default:
      if (verbo) {
        Serial.println();
        Serial.println(tmp);
      } else {
        Serial.print(tmp);
        Serial.println ("?  0..5, +, -, show, verbose");
      }
  } //case

}


void setup() {
  const char info[] = "brain " __DATE__ " " __TIME__;
  Serial.begin(38400);
  Serial.println(info);

  SPI.begin();               // Init SPI bus
  mfrc522.PCD_Init();        // Init MFRC522 card

  pinMode(SOUND_PIN, OUTPUT);
  digitalWrite(SOUND_PIN, 0);
  beep(20);
  // Prepare the security key for the read and write functions - all six key bytes are set to 0xFF at chip delivery from the factory.
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;//keyByte is defined in the "MIFARE_Key" 'struct' definition in the .h file of the library
  }
  EEPROM.get(epromAdr, mypos);
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

  currMs = millis();
  if (currMs - prevMs >= tickMs) {
    prevMs = currMs;
  } // timer
} // loop

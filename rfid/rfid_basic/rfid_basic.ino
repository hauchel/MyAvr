/*
   read/write MIFARE classic cards, based on Example sketch
   requires functions.ino
  Signal       Reader          Pin
   -----------------------------------
   VCC                            ornge (3.3!)
   RST/Reset   RST          9     white
   GND
   IRQ
   SPI MISO    MISO         12    green
   SPI MOSI    MOSI         11    blue
   SPI SCK     SCK          13    yell
   SPI SS      SDA(SS)      10    lila

   We support many readers at the same time, so SS is different for each
*/
#include "helper.h"
#include <SPI.h>
#include <MFRC522.h>
#define NR_OF_READERS   3
byte ssPins[] = {10, 8, 7};
MFRC522 mfrc522[NR_OF_READERS];       

#define RST_PIN 9  //reset pin

MFRC522::MIFARE_Key key;//create a MIFARE_Key struct named 'key', which will hold the card information
byte readbackblock[18];   //This array is used for reading out a block. The MIFARE_Read method requires a buffer that is at least 18 bytes to hold the 16 bytes of a block.

int writeme = 0;          // if 1: write card
bool verbo = false;
bool check = false;       // auto-detect card in loop
byte cardNo = 0;          // holds current cardno (read and to write)
int blocknum = 2; //this is the block number we will write into and then read. Do not write into 'sector trailer' block, since this can make the block unusable.

int writeblock = 1;  // with one of blocks defined below
byte block_0 [16] = {
  "CardNo xxx     "
};
unsigned long time, nexttime = 0;
int state = 2;
byte rdr=0;   //active reader
/*                     to
  0  Disabled           ->n by serial '0' to '3'
  1  PICC present       ->2 by disconnect
  2  unknown            ->1 by newcardPresent ->3 by 2nd read
  3  no PICC present    ->1 by newcardPresent
*/

MFRC522::StatusCode mfrstat;
#include "functions.h"

void dump_byte_array(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
  }
}

void setstate(int newstate) {
  if (state == 0) {
    return;
  }
  Serial.print ("State from ");
  Serial.print (state);
  Serial.print (" to ");
  Serial.println (newstate);
  state = newstate;
}

void cardNoChange() {
  msgF (F("Card is"), cardNo);
  block_0[7] = cardNo + 48;
}

void blocknumChange(int dlt) {
  blocknum += dlt;
  if (blocknum < 2) {
    blocknum = 2;
  }
  msgF (F("blocknum is"), blocknum);
}
bool tryread() {
  // returns true if success
  bool tmp = mfrc522[rdr].PICC_ReadCardSerial(); //if PICC_ReadCardSerial returns 1, the "uid" struct  contains the ID of the read card.
  Serial.print("tryread: ReadCardSerial ");
  if (tmp) {
    Serial.println("OK");
  }
  else {
    Serial.println("Err");
    return tmp;
  }
  Serial.print(F("Card UID:"));
  dump_byte_array(mfrc522[rdr].uid.uidByte, mfrc522[rdr].uid.size);
  Serial.println();
  Serial.print(F("PICC type: "));
  MFRC522::PICC_Type piccType = mfrc522[rdr].PICC_GetType(mfrc522[rdr].uid.sak);
  Serial.println(mfrc522[rdr].PICC_GetTypeName(piccType));

  return (tmp);
}

int wakeup() {
  // returns true if success
  byte bufferATQA[2];
  byte bufferSize = sizeof(bufferATQA);
  mfrstat = mfrc522[rdr].PICC_WakeupA(bufferATQA, &bufferSize);
  Serial.print("wakeup returned ");
  Serial.print(mfrc522[rdr].GetStatusCodeName(mfrstat));
  return 0;
}

void disconn () {
  mfrstat = mfrc522[rdr].PICC_HaltA();
  Serial.print("HaltA: ");
  Serial.println(mfrc522[rdr].GetStatusCodeName(mfrstat));

  mfrc522[rdr].PCD_StopCrypto1();
}

void mynewcard() {
  if ( ! tryread()) {
    Serial.println("newcard: aborting");
    return;
  }
  int tmp;

  if (writeme) {
    tmp = writeBlock(blocknum, block_0);
  } // writeme

  tmp = readBlock(blocknum, readbackblock);
  Serial.print("Read: ");
  Serial.print(tmp);
  for (int j = 0 ; j < 15 ; j++) //print the block contents
  {
    Serial.write (readbackblock[j]);
  }
  Serial.println("< Done");
  if (state == 0) { // without state check
    mfrc522[rdr].PICC_HaltA();
    mfrc522[rdr].PCD_StopCrypto1();
  }
}

void initReaders() {
  Serial.println(F("Checking Readers ..."));
  for (byte reader = 0; reader < NR_OF_READERS; reader++) {
    delay(1);
    mfrc522[reader].PCD_Init(ssPins[reader], RST_PIN);        // Init each MFRC522 reader
    //byte verbyte = mfrc522[reader].PCD_ReadRegister(mfrc522[reader].VersionReg);
    Serial.print(F("Reader "));
    Serial.print(reader);
    Serial.print(F(": "));
    mfrc522[reader].PCD_DumpVersionToSerial();
    mfrc522[reader].PICC_HaltA();
    mfrc522[reader].PCD_StopCrypto1();
  }
}

void docmd(char cmd) {
   if (doNum(cmd)) {
    Serial.print(char(cmd));
    return;
  }
  //tmp = doVT100(tmp);
  //if (tmp == 0) return;
  switch (cmd) {

    case '0':   //
      state = 0;
      msgF (F("State to "), state);
      break;
    case '1':   //
      state = 1;
       msgF (F("State to "), state);
      break;
    case '2':   //
      state = 2;
       msgF (F("State to "), state);
      break;
    case '3':   //
      state = 3;
       msgF (F("State to "), state);
      break;

    case 'a':   //
      blocknumChange(+1);
      break;
    case 'b':   //
      blocknumChange(-1);
      break;

    case 'd':   // Disconnect
      disconn();
      break;
    case 'm':   //  handle
      mynewcard();
      break;
    case 'r':   //  Read mode
      writeme = 0;
      msgF (F("Read Mode"), writeme);
      break;

    case 't':   //  Try to read serial
      tryread();
      break;

    case 'u':   //  wakeUp
      wakeup();
      tryread();
      break;
    case 'v':   //
      verbo = !verbo;
      if (verbo)  Serial.println(F("Verbose an"));
      else        Serial.println(F("Verbose aus"));
      break;
    case 'w':   // Write mode
      writeme = 1;
      msgF(F("Write Mode"), writeme);
      break;

    case '+':   // select Block to write
      cardNo++;
      cardNoChange();
      break;

    case '-':   //
      cardNo--;
      cardNoChange();
      break;

    default:
      Serial.print("?? 0,1,2,3, b,c, d,r,t,u,w, +,-");
  } //case

}


void setup() {
  const char info[] = "rfid_basic " __DATE__ " " __TIME__;
  Serial.begin(38400);
  Serial.println(info);

  SPI.begin();               // Init SPI bus
 
  // Prepare the security key for the read and write functions - all six key bytes are set to 0xFF at chip delivery from the factory.
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;//keyByte is defined in the "MIFARE_Key" 'struct' definition in the .h file of the library
  }

  initReaders();
}


void loop() {
  if (mfrc522[rdr].PICC_IsNewCardPresent()) {
    mynewcard();
    setstate(1);
  }

  if (Serial.available() > 0) {
    Serial.println();
    docmd( Serial.read());
  }

  // check state
  time = millis();
  if (time > nexttime) {
    nexttime = time + 100; // every...
    switch (state) {
      case 0:
        break;
      case 1:   // PICC was present
        disconn();
        setstate(2);
        break;
      case 2:   // unknown
        setstate(3);
        break;
      case 3:  // no PICC
        break;
      default:
        Serial.print("Maschin kaputt, state is ");
        Serial.println(state);
        state = 0;
    } // case
  } // time

} // loop

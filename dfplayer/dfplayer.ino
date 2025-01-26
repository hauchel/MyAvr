/***************************************************
  DFPlayer - A Mini MP3 Player For Arduino
  <https://www.dfrobot.com/product-1121.html>

  An Player:                        MiniPro

   1 VCC             rot
   2 RX              gelb              9  (TX)
   3 TX              grün              8  (RX)
   7 GND             schwarz
  16 Busy            blau              7
 ****************************************************/

#include "Arduino.h"
#include "SoftwareSerial.h"
#include "DFRobotDFPlayerMini.h"

#include <dcf77.h>
dcf77 dcf = dcf77();
void int0() {
  dcf.dcfRead();
}

SoftwareSerial mySoftwareSerial(8, 9); // RX, TX
DFRobotDFPlayerMini dfp;
const byte busyPin = 7;
//#include "rc5_tim2.h"
//const byte rc5Pin = 2;   // TSOP weiss
const byte ledPin = 13;



bool verbo = false;
bool dauer = true;
int inp;
int folder;
bool inpAkt;   // true if last input was a number

unsigned long currTim;
unsigned long nexTim = 0;
unsigned long tick = 100;     //
unsigned long tick10 = 10000U;     //
unsigned long nexTim10 = 0;

const byte seqAnz = 10;
int  seqFold[seqAnz]; // count down
int  seqNum[seqAnz];
byte seqP = 0;
byte tickAnf = 5; // ticks after start to ignore busy
byte tickCnt = 0; // counts ticks after start

byte hh, mm, ss;

void msg(const char txt[], int n) {
  Serial.print(txt);
  Serial.print(" ");
  Serial.println(n);
}

void printDetail(uint8_t type, int value) {
  switch (type) {
    case TimeOut:
      Serial.println(F("Time Out!"));
      break;
    case WrongStack:
      Serial.println(F("Stack Wrong!"));
      break;
    case DFPlayerCardInserted:
      Serial.println(F("Card Inserted!"));
      break;
    case DFPlayerCardRemoved:
      Serial.println(F("Card Removed!"));
      break;
    case DFPlayerCardOnline:
      Serial.println(F("Card Online!"));
      break;
    case DFPlayerUSBInserted:
      Serial.println(F("USB Inserted!"));
      break;
    case DFPlayerUSBRemoved:
      Serial.println(F("USB Removed!"));
      break;
    case DFPlayerPlayFinished:
      Serial.print(F("Number:"));
      Serial.print(value);
      Serial.println(F(" Play Finished!"));
      break;
    case DFPlayerError:
      Serial.print(F("DFPlayerError:"));
      switch (value) {
        case Busy:
          Serial.println(F("Card not found"));
          break;
        case Sleeping:
          Serial.println(F("Sleeping"));
          break;
        case SerialWrongStack:
          Serial.println(F("Get Wrong Stack"));
          break;
        case CheckSumNotMatch:
          Serial.println(F("Check Sum Not Match"));
          break;
        case FileIndexOut:
          Serial.println(F("File Index Out of Bound"));
          break;
        case FileMismatch:
          Serial.println(F("Cannot Find File"));
          break;
        case Advertise:
          Serial.println(F("In Advertise"));
          break;
        default:
          msg("UNKNOWN value", value);
          break;
      }
      break;
    default:
      msg("UNKNOWN type", type);
      break;
  }
}

void info2() {
  char str[100];
  int sta = dfp.readState();
  delay(20);
  int vol = dfp.readVolume();
  delay(20);
  int fin = dfp.readCurrentFileNumber();
  sprintf(str, "Sta %4d   Vol %2d  Cur %4d", sta , vol, fin );
  Serial.println(str);
}

void help() {
  Serial.println(F(" nnn Number bs"));
  Serial.println(F(" a Advertise nnn"));
  Serial.println(F(" b in MP3 play  nnn"));
  Serial.println(F(" p  play  nnn"));
  Serial.println(" e equalizer  nnn");
  Serial.println(" f select folder  nnn");
  Serial.println(" f select folder  nnn");
  Serial.println(" l volume  nnn");
  Serial.println(" - previous  + next   v verbose  d dauer");

}
void info() {
  //----Read imformation----
  msg("EQ       ", dfp.readEQ());
  msg("Folders  ", dfp.readFolderCounts());
  msg("FilCnt   ", dfp.readFileCounts());
  msg("inFold   ", dfp.readFileCountsInFolder(folder));
}

bool isBusy() {
  return (!digitalRead(busyPin));
}

void sequDown() {
  byte b;
  seqP = 6;
  b = seqP - 1;
  seqFold[b] = 2;
  seqNum[b--] = 5;
  seqFold[b] = 2;
  seqNum[b--] = 4;
  seqFold[b] = 2;
  seqNum[b--] = 3;
  seqFold[b] = 2;
  seqNum[b--] = 2;
  seqFold[b] = 2;
  seqNum[b--] = 1;
  seqFold[b] = 2;
  seqNum[b--] = 10;
}

void sequUhr() {
  //
  byte b;
  seqP = 4;
  b = seqP - 1;
  seqFold[b] = 3; // 3 beim
  seqNum[b--] = 25;
  seqFold[b] = 3; // 2 Uhr
  seqNum[b--] = hh;
  seqFold[b] = 4; // 1 Minu
  seqNum[b--] = mm;
  seqFold[b] = 5; // 0 Secu
  seqNum[b--] = ss;
}

void nextUhr() {
  char str[50];
  ss++;
  if (ss == 6) {
    mm++;
    if (mm == 60) {
      hh++;
      if (hh > 24) {
        hh = 1;
      }
    }
    if (mm > 60) {
      mm = 1;
    }
  }
  if (ss > 6) {
    ss = 1;
  }
  sprintf(str, "%02u:%02u:%02u ", hh, mm, ss * 10);
  Serial.println(str);
}

void zeit() {
  char str[100];
  sprintf(str, "hh %3u, mm %3u, Timst %7lu ", dcf.hour, dcf.minute, millis() - dcf.timestamp);
  Serial.println(str);
  if (dcf.hour < 24) {
    hh = dcf.hour;
    mm = dcf.minute;
    ss=1;
  }
}

/*
  void doRC5() {
  msg("RC5 ", rc5_command);
  digitalWrite(ledPin, HIGH);
  if (rc5_command <= 9) {
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
        doCmd('r');
        break;
      case 23:   // G
        doCmd('g');
        break;
      case 27:   // Y
        doCmd('y');
        break;
      case 36:   // B
        break;
      case 57:   // Ch+

        break;
      case 56:   // Ch+

        break;
      default:
        msg("Which RC5?", rc5_command);
    } // case
  } // else
  }
*/
void doCmd( char tmp) {
  bool weg = false;
  if ( tmp == 8) { //backspace removes last digit
    weg = true;
    inp = inp / 10;
  }
  if ((tmp >= '0') && (tmp <= '9')) {
    weg = true;
    if (inpAkt) {
      inp = inp * 10 + (tmp - '0');
    } else {
      inpAkt = true;
      inp = tmp - '0';
    }
  }
  if (weg) {
    Serial.print("\b\b\b\b");
    Serial.print(inp);
    return;
  }
  inpAkt = false;

  switch (tmp) {
    case '+':   //
      dfp.volumeUp();
      break;
    case '-':
      dfp.volumeDown();
      break;
    case 'a':   //
      msg ("Advert ", inp);
      dfp.advertise(inp);
      break;
    case 'b':   //
      msg ("in MP3 play ", inp);
      dfp.playMp3Folder(inp);
      break;
    case 'c':   //
      msg ("in folder 2 play ", 2);
      dfp.playLargeFolder(2, 2);
      break;
    case 'C':   //
      msg ("in folder 55 play ", 2);
      dfp.playLargeFolder(55, 2);
      break;
    case 'd':   //
      dauer = !dauer;
      msg("Dauer", dauer);
      break;
    case 'e':   //
      msg ("Equalizer ", inp);
      dfp.EQ(inp);
      break;
    case 'f':   //
      folder = inp;
      msg ("Select folder ", folder);
      msg("File count ", dfp.readFileCountsInFolder(folder));
      break;
    case 'g':   //
      sequUhr();
      break;
    case 'G':   //
      sequDown();
      break;
    case 'h':   //
      hh = inp;
      break;
    case 'i':   //
      info();
      break;
    case 'm':   //
      mm = inp;
      break;
    case 'n':   //
      nextUhr();
      sequUhr();
      break;
    case 'p':   //
      msg ("Play ", inp);
      dfp.play(inp);
      break;
    case 'o':   //
      msg ("Folder Play ", inp);
      dfp.playFolder(folder, inp);
      break;
    case 'r':   //
      msg ("Reset ", inp);
      dfp.reset();
      break;
    case 's':   //
      msg ("Stop ", inp);
      dfp.stop();
      break;
    case 't':   //
      msg ("Tick ", inp);
      tick = inp;
      break;
    case 'T':   //
      msg ("TickAnf ", inp);
      tickAnf = inp;
      break;
    case 'v':   //
      msg ("Volume ", inp);
      dfp.volume(inp);
      break;
    case '>':   //
      dfp.next();
      break;
    case '<':   //
      dfp.previous();
      break;

    case 'x':   //
      verbo = !verbo;
      msg("Verbo", verbo);
      break;
    case 'y':   //
      dcf.verb += 1;
      if (dcf.verb > 2) {
        dcf.verb = 0;
      }
      dcf.msgF(F("verbose "), dcf.verb);
      break;
    case 'z':
      zeit();
      break;

    default:
      msg("?? ", tmp);
      help();
  } // case

  if (verbo) info2();
}


void setup() {
  const char ich[] = "DFPLAYER  " __DATE__  " "  __TIME__;
  Serial.begin(38400);
  Serial.println(ich);
  mySoftwareSerial.begin(9600);
  //RC5_init();
  pinMode(ledPin, OUTPUT);
  pinMode(busyPin, INPUT);
  Serial.println(F("Initializing DFPlayer ... (May take 3~5 seconds)"));
  if (!dfp.begin(mySoftwareSerial)) {  //Use softwareSerial to communicate with mp3.
    Serial.println(F("Unable to begin:"));
    Serial.println(F("1.Please recheck the connection!"));
    Serial.println(F("2.Please insert the SD card or USB drive!"));
  } else {
    Serial.println(F("DFPlayer Mini online."));
  }
  dfp.setTimeOut(500);
  dfp.volume(10);
  attachInterrupt(0, int0, CHANGE);
}

void loop() {
  char tmp;
  bool avail;
  /*
    if (rc5_ok) {
    RC5_again();
    doRC5();
    }
  */
  if (Serial.available() > 0) {
    tmp = Serial.read();
    doCmd(tmp);
  } // avail

  dcf.act();
  currTim = millis();

  if (nexTim10 < currTim) {
    if (dauer) dfp.playFolder(5, 7); //piep
    tickCnt = tickAnf;
    nexTim10 = currTim + tick10;
    nextUhr();
    sequUhr();
  }

  if (nexTim < currTim) {
    digitalWrite(ledPin, LOW);
    nexTim = currTim + tick;
    if (dauer) {
      if (seqP > 0) {
        if (tickCnt == 0) {
          if (!isBusy()) { //launch next
            seqP--;
            tickCnt = tickAnf;
            dfp.playFolder(seqFold[seqP], seqNum[seqP]);
            if (verbo) msg("SeqP ", seqP);
          }
        } else {
          tickCnt--;
        }
      }
      if (verbo) {
        avail = dfp.available();
        if (avail) {
          printDetail(dfp.readType(), dfp.read()); //Print the detail message from DFPlayer to handle different errors and states.
        }
      } // verbo
    } // dauer
  } // tick
}

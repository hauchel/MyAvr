/***************************************************
  DFPlayer - A Mini MP3 Player For Arduino
  <https://www.dfrobot.com/product-1121.html>


  An Player:                        MiniPro

  1 VCC             rot
  2 RX              gelb             11  (TX)
  3 TX              grün             10  (RX)

  7 GND             schwarz
 ****************************************************/

#include "Arduino.h"
#include "SoftwareSerial.h"
#include "DFRobotDFPlayerMini.h"

SoftwareSerial mySoftwareSerial(10, 11); // RX, TX
DFRobotDFPlayerMini myDFPlayer;
#include "rc5_tim2.h"
const byte rc5Pin = 2;   // TSOP weiss


bool verbo = false;
bool dauer = false;
int inp;
bool inpAkt;   // true if last input was a number

unsigned long currTim;
unsigned long nexTim = 0;
unsigned long tick = 1000;     //

const byte ledPin = 13; 

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
      Serial.println("USB Inserted!");
      break;
    case DFPlayerUSBRemoved:
      Serial.println("USB Removed!");
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
  sprintf(str, "Sta %4d   Vol %2d  Cur %4d", myDFPlayer.readState(), myDFPlayer.readVolume(), myDFPlayer.readCurrentFileNumber());
  Serial.println(str);
}

void help() {
  Serial.println(" nnn Number bs");
  Serial.println(" a Advertise nnn");
  Serial.println(" b in MP3 play  nnn");
  Serial.println(" p  play  nnn");
  Serial.println(" e equalizer  nnn");
  Serial.println(" f select folder  nnn");
  Serial.println(" f select folder  nnn");
  Serial.println(" l volume  nnn");
  Serial.println(" - previous  + next   v verbose  d dauer");

}
void info() {
  //----Read imformation----
  msg("EQ       ", myDFPlayer.readEQ());
  msg("Folders  ", myDFPlayer.readFolderCounts());
  msg("FilCnt   ", myDFPlayer.readFileCounts());
  msg("inFold   ", myDFPlayer.readFileCountsInFolder(3));
}

void doRC5() {
  msg("RC5 ", rc5_command);
  digitalWrite(ledPin, HIGH);
  if ((rc5_command >= 0) && (rc5_command <= 9)) {
    doCmd(rc5_command+48);
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
      myDFPlayer.volumeUp();
      break;
    case '-':
      myDFPlayer.volumeDown();
      break;
    case 'a':   //
      msg ("Advert ", inp);
      myDFPlayer.advertise(inp);
      break;
    case 'b':   //
      msg ("in MP3 play ", inp);
      myDFPlayer.playMp3Folder(inp);
      break;
    case 'c':   //
      msg ("in folder 2 play ", 2);
      myDFPlayer.playLargeFolder(2, 2);
      break;
    case 'C':   //
      msg ("in folder 55 play ", 2);
      myDFPlayer.playLargeFolder(55, 2);
      break;
    case 'd':   //
      dauer = !dauer;
      msg("Dauer", dauer);
      break;
    case 'e':   //
      msg ("Equalizer ", inp);
      myDFPlayer.EQ(inp);
      break;
    case 'f':   //
      msg ("Loop folder ", 15);
      myDFPlayer.loopFolder(15);
      break;
    case 'i':   //
      info();
      break;
    case 'v':   //
      msg ("Volume ", inp);
      myDFPlayer.volume(inp);
      break;
    case '>':   //
      myDFPlayer.next();
      break;
    case '<':   //
      myDFPlayer.previous();
      break;
    case 'p':   //
      msg ("Play ", inp);
      myDFPlayer.play(inp);
      break;
    case 'x':   //
      verbo = !verbo;
      msg("Verbo", verbo);
      break;
    default:
      msg("?? ", tmp);
      help();
  } // case
  
  info2();
}


void setup() {
  const char ich[] = "DFPLAYER  " __DATE__  " "  __TIME__;
  Serial.begin(38400);
  Serial.println(ich);
  mySoftwareSerial.begin(9600);
  RC5_init();
  pinMode(ledPin, OUTPUT);
  Serial.println();
  Serial.println(F("DFRobot DFPlayer Mini Demo"));
  Serial.println(F("Initializing DFPlayer ... (May take 3~5 seconds)"));

  if (!myDFPlayer.begin(mySoftwareSerial)) {  //Use softwareSerial to communicate with mp3.
    Serial.println(F("Unable to begin:"));
    Serial.println(F("1.Please recheck the connection!"));
    Serial.println(F("2.Please insert the SD card or USB drive!"));
  } else {
    Serial.println(F("DFPlayer Mini online."));
  }
  myDFPlayer.setTimeOut(500);
}

void loop() {
  char tmp;
  bool avail;
  if (rc5_ok) {
    RC5_again();
    doRC5();
  }
  if (Serial.available() > 0) {
    tmp = Serial.read();
    doCmd(tmp);
  } // avail

  currTim = millis();
  if (nexTim < currTim) {
     digitalWrite(ledPin, LOW);
    nexTim = currTim + tick;
    if (dauer) {
      avail = myDFPlayer.available();
      if (avail) {
        printDetail(myDFPlayer.readType(), myDFPlayer.read()); //Print the detail message from DFPlayer to handle different errors and states.
      } else {
        if (verbo) {
          Serial.print (".");
        }
      }
    } // dauer
  } // tick
}

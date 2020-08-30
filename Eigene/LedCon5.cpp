/*
      LedCon5.cpp - A library for controling Leds with a MAX7219/MAX7221
      Copyright (c) 2007 Eberhard Fahle

*/


#include "LedCon5.h"

//the opcodes for the MAX7221 and MAX7219
#define OP_NOOP   0
#define OP_DIGIT0 1
#define OP_DIGIT1 2
#define OP_DIGIT2 3
#define OP_DIGIT3 4
#define OP_DIGIT4 5
#define OP_DIGIT5 6
#define OP_DIGIT6 7
#define OP_DIGIT7 8
#define OP_DECODEMODE  9
#define OP_INTENSITY   10
#define OP_SCANLIMIT   11
#define OP_SHUTDOWN    12
#define OP_DISPLAYTEST 15

LedCon5::LedCon5(int dataPin, int clkPin, int csPin, int numDevices) {
  SPI_MOSI = dataPin;
  SPI_CLK = clkPin;
  SPI_CS = csPin;
  if (numDevices <= 0 || numDevices > 8 )
    numDevices = 8;
  maxDevices = numDevices;
  pinMode(SPI_MOSI, OUTPUT);
  pinMode(SPI_CLK, OUTPUT);
  pinMode(SPI_CS, OUTPUT);
  digitalWrite(SPI_CS, HIGH);
  SPI_MOSI = dataPin;
  for (int i = 0; i < 64; i++)
    status[i] = 0x00;
  for (int i = 0; i < maxDevices; i++) {
    spiTransfer(i, OP_DISPLAYTEST, 0);
    //scanlimit is set to max on startup
    setScanLimit(i, 7);
    //decode is done in source
    spiTransfer(i, OP_DECODEMODE, 0);
    clearDisplay(i);
    //we go into shutdown-mode on startup
    shutdown(i, true);
  }
  lcRow = 0;
  lcCol = 7;
  lcDp=false;
  lcMs=false;
}

int LedCon5::getDeviceCount() {
  return maxDevices;
}

void LedCon5::shutdown(int addr, bool b) {
  if (addr < 0 || addr >= maxDevices)
    return;
  if (b)
    spiTransfer(addr, OP_SHUTDOWN, 0);
  else
    spiTransfer(addr, OP_SHUTDOWN, 1);
}

void LedCon5::setScanLimit(int addr, int limit) {
  if (addr < 0 || addr >= maxDevices)
    return;
  if (limit >= 0 && limit < 8)
    spiTransfer(addr, OP_SCANLIMIT, limit);
}

void LedCon5::setIntensity(int addr, int intensity) {
  if (addr < 0 || addr >= maxDevices)
    return;
  if (intensity >= 0 && intensity < 16)
    spiTransfer(addr, OP_INTENSITY, intensity);
}

void LedCon5::clearDisplay(int addr) {
  int offset;
  if (addr < 0 || addr >= maxDevices)
    return;
  offset = addr * 8;
  for (int i = 0; i < 8; i++) {
    status[offset + i] = 0;
    spiTransfer(addr, i + 1, status[offset + i]);
  }
}

void LedCon5::setDigit(int addr, int digit, byte value, boolean dp) {
  int offset;
  byte v;

  if (addr < 0 || addr >= maxDevices)
    return;
  if (digit < 0 || digit > 7 || value > 15)
    return;
  offset = addr * 8;
  v = pgm_read_byte_near(charTable + value);
  if (dp)
    v |= B10000000;
  status[offset + digit] = v;
  spiTransfer(addr, digit + 1, v);
}

void LedCon5::setChar(int addr, int digit, char value, boolean dp) {
  int offset;
  byte index, v;

  if (addr < 0 || addr >= maxDevices)
    return;
  if (digit < 0 || digit > 7)
    return;
  offset = addr * 8;
  index = (byte)value;
  if (index > 127) {
    //no defined beyond index 127, so we use the space char
    index = 32;
  }
  v = pgm_read_byte_near(charTable + index);
  if (dp)
    v |= B10000000;
  status[offset + digit] = v;
  spiTransfer(addr, digit + 1, v);
}


void  LedCon5::putch(byte c) {
  //
  if (c == 13) {
    lcCol = 88;
  }  else {
    setChar(lcRow, lcCol, c, lcDp);
	lcDp=false;
    lcCol--;
  }
  if (lcCol > 7) { //its a byte,negative are >7!
    lcCol = 7;
    lcRow++;
    if (lcRow >= maxDevices) {
      lcRow = 0;
    }
  }
}

void LedCon5::home() {
  lcRow = 0;
  lcCol = 7;
}

void LedCon5::cls() {
  lcRow = 0;
  lcCol = 7;
  for (int i = 0; i < maxDevices; i++) {
    clearDisplay(i);
  }
}

void LedCon5::showBol(bool b) {
  if (b) {
    putch('H');
  } else {
    putch('L');
  }
}

void LedCon5::showDig(byte b) {
  // show as -00, 255 -1 ...
  char sig = ' ';
  if (b > 127) {
    b = 0 - b;
    sig = '-';
  }
  if (b < 10) {
    putch(' ');
    putch(sig);
    putch(b + 48);
    return;
  }
  if (b < 100) {
    putch(sig);
    byte r = b / 10;
    putch(r + 48);
    b = b - r * 10;
    putch(b + 48);
    return;
  }
  putch('-');
  putch(sig);
  putch('-');
}

void LedCon5::show2Dig(int b) {
  // show as    0, 99
  int r;
  char sig='0';
  if (b<0 ) {
	  putch('-');
  	  putch('-');
	  return;
  }
  if (b > 99) {
	 putch('E');
     putch('E');
	 return;
  } 
  if (b > 9) {
    r = b / 10;
    putch(r + 48);
    b = b - r * 10;
  } else {
    putch(sig);
  }
  putch(b + 48);
}

void LedCon5::show4Dig(int b) {
	show4(b,'0');
}	
void LedCon5::show4DigS(int b) {
	show4(b,' ');
}
	
void LedCon5::show4(int b,char sig) {
  // show as  0 .. 9999
  int r;
  if (b<0 ) {
	  putch(' ');
	  putch(' ');
 	  putch(' ');
  	  putch('-');
	  return;
  }
  if (b > 999) {
    r = b / 1000;
    putch(r + 48);
    b = b - r * 1000;
	sig='0';
  } else {
    putch(sig);
  }
  if (b > 99) {
    r = b / 100;
    putch(r + 48);
    b = b - r * 100;
	sig='0';
  } else {
    putch(sig);
  }
  if (b > 9) {
    r = b / 10;
    putch(r + 48);
    b = b - r * 10;
  } else {
    putch(sig);
  }
  putch(b + 48);
}

void LedCon5::show5Dig(unsigned int b) {
  // show as  0 .. 99999
  unsigned int r;
  char sig = ' ';
  if (b > 9999) {
    r = b / 10000;
    putch(r + 48);
    b = b - r * 10000;
    sig='0';
  } else {
    putch(sig);
  }
  if (lcMs) {
   lcDp=true;
  }
  if (b > 999) {
    r = b / 1000;
    putch(r + 48);
    b = b - r * 1000;
    sig='0';
  } else {
    putch(sig);
  }
 
  if (b > 99) {
    r = b / 100;
    putch(r + 48);
    b = b - r * 100;
    sig='0';
  } else {
    putch(sig);
  }
  if (b > 9) {
    r = b / 10;
    putch(r + 48);
    b = b - r * 10;
    sig='0';
  } else {
    putch(sig);
  }
  putch(b + 48);
}

void LedCon5::spiTransfer(int addr, volatile byte opcode, volatile byte data) {
  //Create an array with the data to shift out
  int offset = addr * 2;
  int maxbytes = maxDevices * 2;

  for (int i = 0; i < maxbytes; i++)
    spidata[i] = (byte)0;
  //put our device data into the array
  spidata[offset + 1] = opcode;
  spidata[offset] = data;
  //enable the line
  digitalWrite(SPI_CS, LOW);
  //Now shift out the data
  for (int i = maxbytes; i > 0; i--)
    shiftOut(SPI_MOSI, SPI_CLK, MSBFIRST, spidata[i - 1]);
  //latch the data onto the display
  digitalWrite(SPI_CS, HIGH);
}





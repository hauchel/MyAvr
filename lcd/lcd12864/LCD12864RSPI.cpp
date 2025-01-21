/*
	Class to operate the LCD12864 screen using SPI interface.
	Equiptment wiki: http://www.dfrobot.com/wiki/index.php?title=SPI_LCD_Module_(SKU:DFR0091)
	The classes are modified version from the library downloaded in the above wiki page.
	
	Author: Yi Wei.
	Date: 2012-04-29
  modified HH: removed a lot not used by me
*/

#include "arduino.h"
#include "LCD12864RSPI.h"

extern "C" 
{
	#include <inttypes.h>
	//#include <stdio.h>  //not needed yet
	//#include <string.h> //needed for strlen()
	#include <avr/pgmspace.h>
}


LCD12864RSPI::LCD12864RSPI() 
{
	this->DEFAULTTIME = 80; // 80 ms default time
	this->delayTime = DEFAULTTIME;
} 

/* Delay dalayTime amount of microseconds. */
void LCD12864RSPI::delayns(void)
{   
	delayMicroseconds(delayTime);
}

/* Write byte data. */
void LCD12864RSPI::writeByte(int data)
{
	digitalWrite(latchPin, HIGH);
	delayns();
	shiftOut(dataPin, clockPin, MSBFIRST, data);
	digitalWrite(latchPin, LOW);
}

/* Write command cmd into the screen. */
void LCD12864RSPI::writeCommand(int cmd)
{
	int H_data,L_data;
	H_data = cmd;
	H_data &= 0xf0;           //Mask lower 4 bit data
	L_data = cmd;             //Format: xxxx0000
	L_data &= 0x0f;           //Mask higher 4 bit data
	L_data <<= 4;             //Format: xxxx0000
	writeByte(0xf8);          //RS=0, an instruction is to be written.
	writeByte(H_data);
	writeByte(L_data);
}

/* Write data data into the screen. */
void LCD12864RSPI::writeData(int data)
{
	int H_data,L_data;
	H_data = data;
	H_data &= 0xf0;           //Mask lower 4 bit data
	L_data = data;            //Format: xxxx0000
	L_data &= 0x0f;           //Mask higher 4 bit data
	L_data <<= 4;             //Format: xxxx0000
	writeByte(0xfa);          //RS=1, data is to be written.
	writeByte(H_data);
	writeByte(L_data);
}

/* Initialize the screen. */
void LCD12864RSPI::initialise()
{
    pinMode(latchPin, OUTPUT);     
    pinMode(clockPin, OUTPUT);    
    pinMode(dataPin, OUTPUT);
    digitalWrite(latchPin, LOW);
    delayns();

    writeCommand(0x30);        //Function related command.
    writeCommand(0x0c);        //Display related command. 
    writeCommand(0x01);        //Clear related command.
    writeCommand(0x06);        //Preset point instruction command.
}

/* Clear the screen. */
void LCD12864RSPI::clear(void)
{  
    writeCommand(0x30);
    writeCommand(0x01); // Clear screen command.
}

/* Move cursor to 0-based column X and 0-based row Y as the starting point of the next display. */
void LCD12864RSPI::moveCursor(int X, int Y)
{
  switch(Y)
   {
     case 0:  X|=0x80;break;
     case 1:  X|=0x90;break;
     case 2:  X|=0x88;break;
     case 3:  X|=0x98;break;
     default: break;
   }
  writeCommand(X);
}

/* Display string ptr (with length len) at 0-based column X and 0-based row Y. */
void LCD12864RSPI::displayString(int X,int Y, const char *ptr,int len)
{
	int i;
	int y2 = Y;
	moveCursor(X, Y);    
	for(i=0; i<len; i++)
	{ 
	    writeData(ptr[i]);
		if(i>0 && i%15==0) {
			y2++;
			moveCursor(0, y2);
		}
	}
}


LCD12864RSPI LCDA = LCD12864RSPI();

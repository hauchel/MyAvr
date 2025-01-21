#ifndef LCD12864RSPI_h
#define LCD12864RSPI_h
#include <avr/pgmspace.h>
#include <inttypes.h>

/*
	Class to operate the LCD12864 screen using SPI interface.
	Equiptment wiki: http://www.dfrobot.com/wiki/index.php?title=SPI_LCD_Module_(SKU:DFR0091)
	The classes are modified version from the library downloaded in the above wiki page.
	
	Author: Yi Wei.
	Date: 2012-04-29
  modified HH: removed a lot not used by me
  bugs: moveCursor takes x*2, displayStr blanks if more than 
  useful commands:
  
  0x30   Basic, no G
  0x34
  0x36  
*/
class LCD12864RSPI {
	typedef unsigned char uchar;

public:
	LCD12864RSPI();
	
	void initialise(void);         /* Initialize the screen. */
	void delayns(void);            /* Delay dalayTime amount of microseconds. */
	
	void writeByte(int data);      /* Write byte data. */
	void writeCommand(int cmd);    /* Write command cmd into the screen. */
	void writeData(int data);      /* Write data data into the screen. */
	void moveCursor(int X, int Y); /* Move cursor to 0-based column X (*2!)and 0-based row Y as the starting point of the next display. */

	void clear(void);			                    /* Clear the screen. */
	void displayString(int X,int Y,const char *ptr,int dat); /* Display string ptr (with length len) at 0-based column X and 0-based row Y. */
	
	int delayTime;    /* Delay time in microseconds. */
	int DEFAULTTIME;  /* Default delay time. */

	static const int clockPin = 7;  //orange  
	static const int latchPin = 8;  // yell
	static const int dataPin  = 9;  //green 
};
extern LCD12864RSPI LCDA;    
#endif

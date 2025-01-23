/*    Based on
 *    LedControl.h - A library for controling Leds with a MAX7219/MAX7221
 *    Copyright (c) 2007 Eberhard Fahle
 *
 */

#ifndef LedCon5_h
#define LedCon5_h

#include <avr/pgmspace.h>

#if (ARDUINO >= 100)
#include <Arduino.h>
#else
#include <WProgram.h>
#endif

/*
 * Segments to be switched on for characters and digits on
 *      6 
 *    1    5
 *      0
 *    2   4
 *      3    7
 */
const static byte charTable [] PROGMEM  = {
    B01111110,B00110000,B01101101,B01111001,B00110011,B01011011,B01011111,B01110000,
    B01111111,B01111011,B01110111,B00011111,B00001101,B00111101,B01001111,B01000111,
    // 16
    B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,
    B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,
    // 32
    B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,
    B00000000,B00000000,B00000000,B00000000,B10000000,B00000001,B10000000,B00000000,
    //48  0       1         2         3         4         5         
    B01111110,B00110000,B01101101,B01111001,B00110011,B01011011,B01011111,B01110000,
    B01111111,B01111011,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,
    //64          A                                                          G
    B00000000,B01110111,B00011111,B01001110,B00111101,B01001111,B01000111,B01100010,
    //    H       I                             L     M        N            O
    B00110111,B00000110,B00000000,B00000111,B00001110,B00100111,B00000000,B01111110,
    // 80  P      Q                  S        T         U           V         W
    B01100111,B01111000,B00000000,B01011011,B01000110,B00111110,B00011100,B00010011,
    B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00001000,
    // 96        a
    B00000000,B01110111,B00011111,B00001101,B00111101,B01001111,B01000111,B00000000,
    B00110111,B00000000,B00000000,B00000000,B00001110,B00000000,B00010101,B00011101,
    //
    B01100111,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,
    B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000
};


class LedCon5 {
private :
    /* The array for shifting the data to the devices */
    byte spidata[16];
    /* Send out a single command to the device */
    void spiTransfer(int addr, byte opcode, byte data);
    /* We keep track of the led-status for all 8 devices in this array */
    byte status[64];
    /* Data is shifted out of this pin*/
    int SPI_MOSI;
    /* The clock is signaled on this pin */
    int SPI_CLK;
    /* This one is driven LOW for chip selectzion */
    int SPI_CS;
    /* The maximum number of devices we use */
    int maxDevices;

public:
    byte lcRow;     // 0 for one 
    byte lcCol;     // 7 downto 0
	bool lcDp;		// if next putch uses decimal point
	bool lcMs;		// use DP for show5
    /* 
     * Create a new controler 
     * Params :
     * dataPin		pin on the Arduino where data gets shifted out
     * clockPin		pin for the clock
     * csPin		pin for selecting the device 
     * numDevices	maximum number of devices that can be controled
     */
    LedCon5(int dataPin, int clkPin, int csPin, int numDevices=1);

    /*
         * Gets the number of devices attached to this LedControl.
     * Returns :
     * int	the number of devices on this LedControl
     */
    int getDeviceCount();

    /* 
     * Set the shutdown (power saving) mode for the device
     * Params :
     * addr	The address of the display to control
     * status	If true the device goes into power-down mode. Set to false
     *		for normal operation.
     */
    void shutdown(int addr, bool status);

    /* 
     * Set the number of digits (or rows) to be displayed.
     * See datasheet for sideeffects of the scanlimit on the brightness
     * of the display.
     * Params :
     * addr	address of the display to control
     * limit	number of digits to be displayed (1..8)
     */
    void setScanLimit(int addr, int limit);

    /* 
     * Set the brightness of the display.
     * Params:
     * addr		the address of the display to control
     * intensity	the brightness of the display. (0..15)
     */
    void setIntensity(int addr, int intensity);

    /* 
     * Switch all Leds on the display off. 
     * Params:
     * addr	address of the display to control
     */
    void clearDisplay(int addr);

    /* 
     * Display a hexadecimal digit on a 7-Segment Display
     * Params:
     * addr	address of the display
     * digit	the position of the digit on the display (0..7)
     * value	the value to be displayed. (0x00..0x0F)
     * dp	sets the decimal point.
     */
    void setDigit(int addr, int digit, byte value, boolean dp);

    /* 
     * Display a character on a 7-Segment display.
     * Params:
     * addr	address of the display
     * digit	the position of the character on the display (0..7)
     * value	the character to be displayed. 
     * dp	sets the decimal point.
     */
    void setChar(int addr, int digit, char value, boolean dp);

    void putch(byte c);
    void home();
    void cls();
	void showBol(bool b);
    void showDig(byte b);    // show as -00, 255 -1 ...
    void show2Dig(int b);    // show 2 as    0, 99
	void show4(int b,char sig);
	void show4Dig(int b);    // show 4 with zero
	void show4DigS(int b);    // show 4 with space
	void show5Dig(unsigned int b);
};

#endif	//LedCon5.h





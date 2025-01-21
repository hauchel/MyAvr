// Lcdwiki GUI library with init code from Rossum
// MIT license
// stripped everything except st7796 from https://github.com/synapticloopltd/LCDWIKI_SPI

#ifndef _LCDWIKI_SPI_H_
#define _LCDWIKI_SPI_H_

#if ARDUINO >= 100
	#include "Arduino.h"
#else
	#include "WProgram.h"
#endif

#ifdef __AVR__
	#include <avr/pgmspace.h>
#elif defined(ESP8266)
	#include <pgmspace.h>
#else
	#define pgm_read_byte(addr) (*(const unsigned char *)(addr))
	#define pgm_read_word(addr) (*(const unsigned short *)(addr))
#endif

#include "LCDWIKI_GUI.h"


#define ROTATION_0    0
#define ROTATION_90   1
#define ROTATION_180  2
#define ROTATION_270  3

#define ID_7796     12
#define ID_UNKNOWN  0xFF
#define ST7796S     14

typedef struct _lcd_info {
	uint16_t lcd_id;
	int16_t lcd_wid;
	int16_t lcd_heg;
} lcd_info;

class LCDWIKI_SPI:public LCDWIKI_GUI {
	public:
		LCDWIKI_SPI(uint16_t model,int8_t cs, int8_t cd, int8_t reset,int8_t led);
		void Init_LCD(void);
		void reset(void);
		void start(uint16_t ID);
		void Draw_Pixe(int16_t x, int16_t y, uint16_t color);
		void Spi_Write(uint8_t data);
		uint8_t Spi_Read(void);
		void Write_Cmd(uint16_t cmd);
		void Write_Data(uint16_t data);
		void Write_Cmd_Data(uint16_t cmd, uint16_t data);
		void init_table8(const void *table, int16_t size);
		void init_table16(const void *table, int16_t size);
		void Push_Command(uint8_t cmd, uint8_t *block, int8_t N);
		uint16_t Color_To_565(uint8_t r, uint8_t g, uint8_t b);
		uint16_t Read_ID(void);
		void Fill_Rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
		void Set_Rotation(uint8_t r); 
		uint8_t Get_Rotation(void) const;
		void Invert_Display(boolean i);
		void SH1106_Display(void);
		void SH1106_Draw_Bitmap(uint8_t x,uint8_t y,uint8_t width, uint8_t height, uint8_t *BMP, uint8_t mode);
		uint16_t Read_Reg(uint16_t reg, int8_t index);
		int16_t Read_GRAM(int16_t x, int16_t y, uint16_t *block, int16_t w, int16_t h);
		void Set_Addr_Window(int16_t x1, int16_t y1, int16_t x2, int16_t y2);

		void Push_Any_Color(uint16_t *block, int16_t n, bool first, uint8_t flags);
		void Push_Any_Color(uint8_t * block, int16_t n, bool first, uint8_t flags);

		void Push_Same_Color(uint16_t color, uint16_t n, bool first);

		void Push_Compressed_Image(int16_t x, int16_t y, uint16_t *block, uint8_t flags);
		void Push_Indexed_Image(int16_t x, int16_t y, uint8_t *block, uint8_t flags);

		void Vert_Scroll(int16_t top, int16_t scrollines, int16_t offset);
		int16_t Get_Height(void) const;
		int16_t Get_Width(void) const;
		void Set_LR(void);
		void Led_control(boolean i);

	protected:
		uint8_t xoffset;
		uint8_t yoffset;

		uint16_t WIDTH;
		uint16_t HEIGHT;
		uint16_t width;
		uint16_t height;
		uint16_t rotation;
		uint16_t lcd_driver;
		uint16_t lcd_model;

	private:
		uint16_t XC;
		uint16_t YC;
		uint16_t CC;
		uint16_t RC;
		uint16_t SC1;
		uint16_t SC2;
		uint16_t MD;
		uint16_t VL;
		uint16_t R24BIT;
		uint16_t MODEL;
 
		volatile uint8_t *spicsPort;
		volatile uint8_t *spicdPort;
		volatile uint8_t *spimisoPort;
		volatile uint8_t *spimosiPort;
		volatile uint8_t *spiclkPort;

		uint8_t spicsPinSet;
		uint8_t spicdPinSet;
		uint8_t spimisoPinSet;
		uint8_t spimosiPinSet;
		uint8_t spiclkPinSet;
		uint8_t spicsPinUnset;
		uint8_t spicdPinUnset;
		uint8_t spimisoPinUnset;
		uint8_t spimosiPinUnset;
		uint8_t spiclkPinUnset;

		int8_t _cs;
		int8_t _cd;
		int8_t _miso;
		int8_t _mosi;
		int8_t _clk;
		int8_t _reset;
		int8_t _led;
};
#endif

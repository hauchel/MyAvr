// IMPORTANT: LIBRARY MUST BE SPECIFICALLY CONFIGURED FOR EITHER TFT SHIELD
// OR BREAKOUT BOARD USAGE.

// Lcdwiki GUI library with init code from Rossum
// MIT license

#if defined(ARDUINO_ARCH_ESP8266)
#define USE_HWSPI_ONLY
#endif

#include <SPI.h>
#include "pins_arduino.h"
#include "wiring_private.h"
#include "LCDWIKI_SPI.h"
#include "lcd_spi_registers.h"
#include "mcu_spi_magic.h"

#define TFTLCD_DELAY16  0xFFFF
#define TFTLCD_DELAY8   0x7F
#define MAX_REG_NUM     24

static uint8_t SH1106_buffer[1024] = {0};

//The mode,width and heigth of supported LCD modules
lcd_info current_lcd_info[] = {
  0x9325, 240, 320,
  0x9328, 240, 320,
  0x9341, 240, 320,
  0x9090, 320, 480,
  0x7575, 240, 320,
  0x9595, 240, 320,
  0x9486, 320, 480,
  0x7735, 128, 160,
  0x1283, 130, 130,
  0x1106, 128, 64,
  0x7735, 128, 128,
  0x9488, 320, 480,
  0x9488, 320, 480,
  0x9225, 176, 220,
  0x7796, 320, 480,
};


/*!
   @brief Constructor for a hardware SPI which will instantaite a new object
     based on the model number.  The model number must be one of the values
     that is defined in the LCDWIKI_SPI.h file.

   @param model The width of the display in pixels

   @param cs The Arduino pin that will be attached to the chip select line on the TFT board
   @param cd The Arduino pin that will be attached to the command or data line on the TFT board
   @param reset The Arduino pin that will be attached to the reset line on the TFT board
   @param led The Arduino pin that will be attached to the LED line on the TFT board
*/
LCDWIKI_SPI::LCDWIKI_SPI(uint16_t model, int8_t cs, int8_t cd, int8_t reset, int8_t led) {
  _cs = cs;
  _cd = cd;
  _miso = -1;
  _mosi = -1;
  _clk = -1;
  _reset = reset;
  _led = led;
  MODEL = model;
#if defined(__AVR__)
  spicsPort = portOutputRegister(digitalPinToPort(_cs));
  spicsPinSet = digitalPinToBitMask(_cs);
  spicsPinUnset = ~spicsPinSet;
  if (cd < 0)
  {
    spicdPort = 0;
    spicdPinSet = 0;
    spicdPinUnset = 0;
  }
  else
  {
    spicdPort = portOutputRegister(digitalPinToPort(_cd));
    spicdPinSet = digitalPinToBitMask(_cd);
    spicdPinUnset = ~spicdPinSet;
  }
  spimisoPort = 0;
  spimisoPinSet = 0;
  spimisoPinUnset = 0;
  spimosiPort = 0;
  spimosiPinSet = 0;
  spimosiPinUnset = 0;
  spiclkPort = 0;
  spiclkPinSet = 0;
  spiclkPinUnset = 0;

  *spicsPort     |=  spicsPinSet; // Set all control bits to HIGH (idle)
  *spicdPort     |=  spicdPinSet; // Signals are ACTIVE LOW
#elif defined(ARDUINO_ARCH_ESP8266)
  digitalWrite(_cs, HIGH);
  digitalWrite(_cd, HIGH);
#endif
  pinMode(cs, OUTPUT);	  // Enable outputs
  pinMode(cd, OUTPUT);

  if (reset >= 0) {
    digitalWrite(reset, HIGH);
    pinMode(reset, OUTPUT);
  }

  if (led >= 0) {
    pinMode(led, OUTPUT);
  }

  SPI.begin();
  SPI.setClockDivider(SPI_CLOCK_DIV4); // 4 MHz (half speed)
  SPI.setBitOrder(MSBFIRST);
  SPI.setDataMode(SPI_MODE0);

  xoffset = 0;
  yoffset = 0;
  rotation = 0;
  lcd_model = current_lcd_info[model].lcd_id;

  WIDTH = current_lcd_info[model].lcd_wid;
  HEIGHT = current_lcd_info[model].lcd_heg;

  width = WIDTH;
  height = HEIGHT;

  setWriteDir();
}

/*!
   Initialise the LCD - which __MUST__ be called before anything is done -
   this resets the controller, turns on the LCD and then calls start()
*/
void LCDWIKI_SPI::Init_LCD(void) {
  reset();
  Led_control(true);

  if (lcd_model == 0xFFFF) {
    lcd_model = Read_ID();
  }

  start(lcd_model);
}

/*!
   @brief Reset the LCD py pulling the reset pin low, waiting 2, then setting
     it high.  This is common to both shield and breakout configurations

   @param void
*/
void LCDWIKI_SPI::reset(void) {
  CS_IDLE;
  RD_IDLE;
  WR_IDLE;

  if (_reset >= 0) {
    digitalWrite(_reset, LOW);
    delay(2);
    digitalWrite(_reset, HIGH);
  }

  CS_ACTIVE;
  CD_COMMAND;

  write8(0x00);

  for (uint8_t i = 0; i < 3; i++) {
    WR_STROBE; // Three extra 0x00s
  }

  CS_IDLE;
}

/*!
   @brief Control whether the LED display is turned on

   @param turnOn true to turn on, false to turn off
*/
void LCDWIKI_SPI::Led_control(boolean turnOn) {
  if (_led >= 0) {
    if (turnOn) {
      digitalWrite(_led, HIGH);
    } else {
      digitalWrite(_led, LOW);
    }
  }
}

//spi write for hardware 
void LCDWIKI_SPI::Spi_Write(uint8_t data) {
    SPI.transfer(data);
}

//spi read for hardware
uint8_t LCDWIKI_SPI::Spi_Read(void) {
    return SPI.transfer(0xFF);
}

void LCDWIKI_SPI::Write_Cmd(uint16_t cmd) {
  CS_ACTIVE;
  writeCmd16(cmd);
  CS_IDLE;
}

void LCDWIKI_SPI::Write_Data(uint16_t data) {
  CS_ACTIVE;
  writeData16(data);
  CS_IDLE;
}

void LCDWIKI_SPI::Write_Cmd_Data(uint16_t cmd, uint16_t data) {
  CS_ACTIVE;
  writeCmdData16(cmd, data);
  CS_IDLE;
}

//Write a command and N datas
void LCDWIKI_SPI::Push_Command(uint8_t cmd, uint8_t *block, int8_t N) {
  CS_ACTIVE;
  writeCmd16(cmd);
  while (N-- > 0)  {
    uint8_t u8 = *block++;
    writeData8(u8);

  }

  CS_IDLE;
}


void LCDWIKI_SPI::Set_Addr_Window(int16_t x1, int16_t y1, int16_t x2, int16_t y2) {
  CS_ACTIVE;
  uint8_t x_buf[] = { x1 >> 8, x1 & 0xFF, x2 >> 8, x2 & 0xFF };
  uint8_t y_buf[] = { y1 >> 8, y1 & 0xFF, y2 >> 8, y2 & 0xFF };
  Push_Command(XC, x_buf, 4);
  Push_Command(YC, y_buf, 4);
  CS_IDLE;
}

// Unlike the 932X drivers that set the address window to the full screen
// by default (using the address counter for drawPixel operations), the
// 7575 needs the address window set on all graphics operations.  In order
// to save a few register writes on each pixel drawn, the lower-right
// corner of the address window is reset after most fill operations, so
// that drawPixel only needs to change the upper left each time.
void LCDWIKI_SPI::Set_LR(void) {
  CS_ACTIVE;
  writeCmdData8(HX8347G_COLADDREND_HI, (width - 1) >> 8);
  writeCmdData8(HX8347G_COLADDREND_LO, width - 1);
  writeCmdData8(HX8347G_ROWADDREND_HI, (height - 1) >> 8);
  writeCmdData8(HX8347G_ROWADDREND_LO, height - 1);
  CS_IDLE;
}

/*!
   @brief Push the compressed image format directly to the screen memory.  This
     will set the address window to x, y, width - 1, height -1.  The width and
     height come from the compressed image headers.

   @param x The x or top co-ordinate to start drawing at (top-left)
   @param y The y or left co-ordinate to start drawing at (top-left)
   @param block The pointer to the block of data
   @param flags If set to 1 - it will read from PROGMEM address, else it will
     just read from memory

   @warning There is no bounds checking on this function - if you have a
     compressed image that will overflow the screen, then the behaviour is
     undefined
*/
void LCDWIKI_SPI::Push_Compressed_Image(int16_t x, int16_t y, uint16_t *block, uint8_t flags) {
  uint16_t color;
  uint16_t numberToDraw;

  int16_t width;
  int16_t height;

  // whether to read from PROGMEM, or memory
  bool isconst = flags & 1;

  // width and height are the first two words in the file
  if (isconst) {
    width = pgm_read_word(block++);
    height = pgm_read_word(block++);
  } else {
    width = (*block++);
    height = (*block++);
  }

  int16_t numPixels = width * height;

  Set_Addr_Window(x, y, x + width - 1, y + height - 1);

  CS_ACTIVE;
  writeCmd8(CC);

  while (numPixels-- > 0) {
    if (isconst) {
      numberToDraw = pgm_read_word(block++);
    } else {
      numberToDraw = (*block++);
    }

    if ((numberToDraw & 0x8000) == 0x8000) {
      // we have compression
      numberToDraw -= 0x8000;
      numPixels -= numberToDraw;

      if (isconst) {
        color = pgm_read_word(block++);
      } else {
        color = (*block++);
      }

      while (numberToDraw-- > 0) {
   
          writeData16(color);
        
      }
    } else {
      // draw the raw colors
      numPixels -= numberToDraw;
      while (numberToDraw-- > 0) {
        if (isconst) {
          color = pgm_read_word(block++);
        } else {
          color = (*block++);
        }

        
          writeData16(color);
        
      }
    }
  }
  CS_IDLE;
}

/*!
   @brief Push the indexed image format directly to the screen memory.  This
     will set the address window to x, y, width - 1, height -1.  The width and
     height come from the compressed image headers.

   @param x The x or top co-ordinate to start drawing at (top-left)
   @param y The y or left co-ordinate to start drawing at (top-left)
   @param block The pointer to the block of data
   @param flags If set to 1 - it will read from PROGMEM address, else it will
     just read from memory

   @warning There is no bounds checking on this function - if you have a
     compressed image that will overflow the screen, then the behaviour is
     undefined
*/
void LCDWIKI_SPI::Push_Indexed_Image(int16_t x, int16_t y, uint8_t *block, uint8_t flags) {
  uint16_t color; // the current colour that we are drawing
  uint16_t width; // the width of the image
  uint16_t height; // the height of the image
  long numPixels; // the number of pixels we have - which is width * height

  uint8_t numberToDraw; // the number of compressed colours to draw
  uint8_t h; // reading the high byte for 2 bytes of width
  uint8_t l; // reading the low byte for 2 bytes of height

  uint8_t colorIndex; // the index of the colour that maps to the colour index in the header
  uint8_t numEntries; // the number of colour map entries in the header
  uint8_t mapPointer; // the pointer to the start of the colour map

  bool isconst = flags & 1; // whether to read from PROGMEM, or memory
  bool is8Bit = false; // Whether to use 8 or 16 bit widths

  uint8_t *mapAddress = block; //

  if (isconst) {
    is8Bit = pgm_read_byte(block++);
    mapAddress += 1;

    if (is8Bit) {
      width = pgm_read_byte(block++);
      height = pgm_read_byte(block++);
      mapAddress += 2;
    } else {
      width = pgm_read_word(block++);
      height = pgm_read_word(block++);
      mapAddress += 4;
    }

    // here we have the height and width - the next byte is the number
    // of entries in the map

    numEntries = pgm_read_byte(block++);
    mapAddress += 1;
  } else {
    is8Bit = (*block++);

    if (is8Bit) {
      width = (*block++);
      height = (*block++);
    } else {
      h = (*block++);
      l = (*block++);
      width = (h << 8 | l);

      h = (*block++);
      l = (*block++);
      height = (h << 8 | l);
    }

    // here we have the height and width - the next byte is the number
    // of entries in the map

    numEntries = (*block++);
    mapAddress += 1;
  }

  numPixels = width * height;
  // now we need to skip ahead the number of entries
  block += (numEntries * 2);

  // At this point we are at the start of the data

  // TODO - should we do bounds checking??
  Set_Addr_Window(x, y, x + width - 1, y + height - 1);

  CS_ACTIVE;
  writeCmd8(CC);

  while (numPixels > 0) {
    if (isconst) {
      numberToDraw = pgm_read_byte(block++);
    } else {
      numberToDraw = (*block++);
    }

    if ((numberToDraw & 0x80) == 0x80) {
      // we have compression
      numberToDraw -= 0x80;
      numPixels -= numberToDraw;

      if (isconst) {
        colorIndex = pgm_read_byte(block++);
        color = ((pgm_read_byte(mapAddress + (colorIndex * 2))) << 8) + (pgm_read_byte(mapAddress + (colorIndex * 2) + 1));
      } else {
        colorIndex = (*block++);
        color = (*(mapAddress + (colorIndex * 2)) << 8) + *(mapAddress + (colorIndex * 2) + 1);
      }

      while (numberToDraw-- > 0) {
      
          writeData16(color);
        
      }
    } else {
      // draw the raw colors
      numPixels -= numberToDraw;

      while (numberToDraw-- > 0) {

        if (isconst) {
          colorIndex = pgm_read_byte(block++);
          color = ((pgm_read_byte(mapAddress + (colorIndex * 2))) << 8) + (pgm_read_byte(mapAddress + (colorIndex * 2) + 1));
        } else {
          colorIndex = (*block++);
          color = (*(mapAddress + (colorIndex * 2)) << 8) + *(mapAddress + (colorIndex * 2) + 1);
        }

    
          writeData16(color);
        
      }
    }
  }
  CS_IDLE;
}

//push color table for 16bits
void LCDWIKI_SPI::Push_Any_Color(uint16_t * block, int16_t n, bool first, uint8_t flags) {
  uint16_t color;
  uint8_t h, l;
  bool isconst = flags & 1;

  CS_ACTIVE;
  if (first) {
    writeCmd8(CC);
  }

  while (n-- > 0) {
    if (isconst) {
      color = pgm_read_word(block++);
    } else  {
      color = (*block++);
    }

    writeData16(color);

  }
  CS_IDLE;
}


/*!
   @brief Push colours directly to the display memory (which will then update
     the display).  It will read two bytes before pushing the data.

   @param block The pointer to the block of data for the colours
   @param n The number of bytes in the block
   @param first Whether this is the first write to the display - in effect this
     will send a command to the chip to indicate that data is going to be
     written.  Set this to 1 if it is the first write
   @param flags There are two flags that can be set from least significant bit
       00000001 - then this is going to be read from PROGMEM, else RAM
       00000010 - This is a big-endian, rather than little endian
     The flags are not mutually exclusive and these are the options:
       passing in a
         1 - PROGMEM read
         2 - Big-endian
         3 - PROGMEM read and big-endian
*/
void LCDWIKI_SPI::Push_Any_Color(uint8_t * block, int16_t n, bool first, uint8_t flags) {
  uint16_t color;
  uint8_t h, l;
  bool isconst = flags & 1;
  bool isbigend = (flags & 2) != 0;
  CS_ACTIVE;

  if (first) {
    writeCmd8(CC);
  }

  while (n-- > 0) {
    if (isconst) {
      h = pgm_read_byte(block++);
      l = pgm_read_byte(block++);
    } else {
      h = (*block++);
      l = (*block++);
    }

    color = (isbigend) ? (h << 8 | l) :  (l << 8 | h);

    writeData16(color);

  }
  CS_IDLE;
}

/*!
   @brief Push colours directly to the display memory (which will then update
     the display) at the address window that is already set.

   @param color The colour to push int rgb565 format
   @param n The number of bytes in the block
   @param first Whether this is the first write to the display - in effect this
     will send a command to the chip to indicate that data is going to be
     written.  Set this to 1 if it is the first write
   @param first If true, will push a CC command to start drawing

   @warning you will need to set the the address window first using
     Set_Addr_Window()
*/
void LCDWIKI_SPI::Push_Same_Color(uint16_t color, uint16_t n, bool first) {

  CS_ACTIVE;
  if (first) {

    writeCmd8(CC);
  }

  while (n-- > 0) {

    writeData16(color);
  }
  CS_IDLE;
}

/*!
   @brief convert a 24 bit colour (8 bits each for red, green, blue) to an
     rgb565 (5 bits red, 6 bits green, 5 bits blue) 16 bit unsigned integer

   @param r The red 8 bit value
   @param g The green 8 bit value
   @param b The blue 8 bit value

   @return The rgb565 encoded unsigned int colour
*/
uint16_t LCDWIKI_SPI::Color_To_565(uint8_t r, uint8_t g, uint8_t b) {
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | ((b & 0xF8) >> 3);
}

//read value from lcd register
uint16_t LCDWIKI_SPI::Read_Reg(uint16_t reg, int8_t index) {
  uint16_t ret;
  uint16_t high;

  uint8_t low;

  CS_ACTIVE;
  writeCmd16(reg);
  setReadDir();
  delay(1);

  do {
    read16(ret);
  } while (--index >= 0);

  CS_IDLE;
  setWriteDir();
  return ret;
}

//read graph RAM data

/*!
   @brief Read from the graphics RAM and store the values into the passed in
     array

   @param x The x co-ordinate to start reading from
   @param y The y co-ordinate to start reading from

   @param block The array to write the values to

   @param w The width of memory area to read from
   @param h The height of the memory area to read from

   @return 0 Surprisingly always returns 0 :)
*/
int16_t LCDWIKI_SPI::Read_GRAM(int16_t x, int16_t y, uint16_t *block, int16_t w, int16_t h) {
  uint16_t ret, dummy;
  int16_t n = w * h;
  uint8_t r, g, b, tmp;

  Set_Addr_Window(x, y, x + w - 1, y + h - 1);

  while (n > 0) {
    CS_ACTIVE;
    writeCmd16(RC);
    setReadDir();

    read8(r);
    while (n) {
      if (R24BIT == 1) {
        read8(r);
        read8(g);
        read8(b);
        ret = Color_To_565(r, g, b);
      } else if (R24BIT == 0) {
        read16(ret);
      }

      *block++ = ret;
      n--;
    }


    // set it back to that we are writing to the display
    writeCmd16(CC);
    CS_IDLE;
    setWriteDir();
  }

  return 0;
}

//read LCD controller chip ID
uint16_t LCDWIKI_SPI::Read_ID(void) {
  uint16_t ret;

  if ((Read_Reg(0x04, 0) == 0x00) && (Read_Reg(0x04, 1) == 0x8000)) {
    uint8_t buf[] = {0xFF, 0x83, 0x57};
    Push_Command(HX8357D_SETC, buf, sizeof(buf));
    ret = (Read_Reg(0xD0, 0) << 16) | Read_Reg(0xD0, 1);
    if ((ret == 0x990000) || (ret == 0x900000)) {
      return 0x9090;
    }
  }

  ret = Read_Reg(0xD3, 1);

  if (ret == 0x9341) {
    return 0x9341;
  } else if (ret == 0x9486) {
    return 0x9486;
  } else if (ret == 0x9488) {
    return 0x9488;
  } else {
    return Read_Reg(0, 0);
  }
}

//set x,y  coordinate and color to draw a pixel point

/*!
   @brief Draw a pixel to a co-ordinate of the screen (x, y), this will do
     nothing if the provided co-ordinates are off the screen.

   @param x The x co-ordinate of the display
   @param y The y co-ordinate of the display

   @param color The rgb565 packed colour to write

   @warning This is a slow operation and will call Set_Addr_Window(x, y, x, y).
     If you need to draw more than one pixel in a row, the Push_Any_Color()
     function is a much faster way of doing this
*/
void LCDWIKI_SPI::Draw_Pixe(int16_t x, int16_t y, uint16_t color) {
  if ((x < 0) || (y < 0) || (x > Get_Width()) || (y > Get_Height())) {
    return;
  }

  Set_Addr_Window(x, y, x, y);

  CS_ACTIVE;

  writeCmdData16(CC, color);

  CS_IDLE;
}

/*!
   @brief Fill a rectangle starting from x, y with width w and height h.  If
     the rectangle will write out of bounds of the display, then it will be
     cropped to the edge of the display.
     i.e. fill area from x to x+w, y to y+h

   @param x The x co-ordinate of the display
   @param y The y co-ordinate of the display

   @param w The width of the rectangle
   @param h The height of the rectangle

   @param color The rgb565 colour to fill the rectangle with
*/
void LCDWIKI_SPI::Fill_Rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
  int16_t end;

  if (w < 0) {
    w = -w;
    x -= w;
  }                           //+ve w
  end = x + w;

  if (x < 0) {
    x = 0;
  }

  if (end > Get_Width()) {
    end = Get_Width();
  }
  w = end - x;

  if (h < 0) {
    h = -h;
    y -= h;
  }                           //+ve h
  end = y + h;

  if (y < 0) {
    y = 0;
  }

  if (end > Get_Height()) {
    end = Get_Height();
  }
  h = end - y;

  Set_Addr_Window(x, y, x + w - 1, y + h - 1);

  CS_ACTIVE;

  writeCmd8(CC);

  if (h > w) {
    end = h;
    h = w;
    w = end;
  }

  while (h-- > 0)  {
    end = w;
    do {

      writeData16(color);

    } while (--end != 0);
  }

  CS_IDLE;
}

/*!
    @brief Vertically scroll a portion of the screen

    @param top
    @param scrollines
    @param offset
*/
void LCDWIKI_SPI::Vert_Scroll(int16_t top, int16_t scrollines, int16_t offset) {
  int16_t bfa;
  int16_t vsp;
  int16_t sea = top;


  bfa = HEIGHT - top - scrollines;

  if (offset <= -scrollines || offset >= scrollines) {
    offset = 0; //valid scroll
  }

  vsp = top + offset; // vertical start position

  if (offset < 0) {
    vsp += scrollines;          //keep in unsigned range
  }
  sea = top + scrollines - 1;


  uint8_t d[6];           // for multi-byte parameters
  d[0] = top >> 8;        //TFA
  d[1] = top;
  d[2] = scrollines >> 8; //VSA
  d[3] = scrollines;
  d[4] = bfa >> 8;        //BFA
  d[5] = bfa;
  Push_Command(SC1, d, 6);
  d[0] = vsp >> 8;        //VSP
  d[1] = vsp;
  Push_Command(SC2, d, 2);

  if (offset == 0)  {
    Push_Command(0x13, NULL, 0);
  }
}

//get lcd width
int16_t LCDWIKI_SPI::Get_Width(void) const {
  return width;
}

//get lcd height
int16_t LCDWIKI_SPI::Get_Height(void) const {
  return height;
}

//set clockwise rotation
void LCDWIKI_SPI::Set_Rotation(uint8_t r) {
  rotation = r & 3; // just perform the operation ourselves on the protected variables
  width = (rotation & 1) ? HEIGHT : WIDTH;
  height = (rotation & 1) ? WIDTH : HEIGHT;

  CS_ACTIVE;


  uint8_t val;
  switch (rotation)  {
    case 0:
      val = ILI9341_MADCTL_MX | ILI9341_MADCTL_BGR; //0 degree
      break;
    case 1:
      val = ILI9341_MADCTL_MV | ILI9341_MADCTL_BGR; //90 degree
      break;
    case 2:
      val = ILI9341_MADCTL_MY | ILI9341_MADCTL_ML | ILI9341_MADCTL_BGR; //180 degree
      break;
    case 3:
      val = ILI9341_MADCTL_MX | ILI9341_MADCTL_MY | ILI9341_MADCTL_ML | ILI9341_MADCTL_MV | ILI9341_MADCTL_BGR; //270 degree
      break;
  }
  writeCmdData8(MD, val);

  Set_Addr_Window(0, 0, width - 1, height - 1);
  Vert_Scroll(0, HEIGHT, 0);
  CS_IDLE;
}

//get current rotation
//0  :  0 degree
//1  :  90 degree
//2  :  180 degree
//3  :  270 degree
uint8_t LCDWIKI_SPI::Get_Rotation(void) const {
  return rotation;
}

//Anti color display
void LCDWIKI_SPI::Invert_Display(boolean i) {
  CS_ACTIVE;

  uint8_t val = VL ^ i;


  writeCmd8(val ? 0x21 : 0x20);

  CS_IDLE;
}

/*!
   @brief Draw a bitmap to the screen at position x, y with width and height

   @param x The x position to start drawing the bitmap
   @param y The y position to start drawing the bitmap
   @param width The width of the image
   @param height The height of the image
   @param BMP The pointer to the bitmap data (in the correct format for this display)
   @param mode The mode - at the moment if it is 1 - it will read from PROGMEM,
     otherwise, it will read from memory

   @warning This is quite a slow way to draw a bit map as it sets draws
     individual pixels by calling Draw_Pixe()
*/
void LCDWIKI_SPI::SH1106_Draw_Bitmap(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t *BMP, uint8_t mode) {
  uint8_t i,
          j,
          k,
          tmp;

  for (i = 0; i < (height + 7) / 8; i++) {
    for (j = 0; j < width; j++) {
      if (mode) {
        tmp = pgm_read_byte(&BMP[i * width + j]);
      } else {
        tmp = ~(pgm_read_byte(&BMP[i * width + j]));
      }

      for (k = 0; k < 8; k++) {
        if (tmp & 0x01) {
          Draw_Pixe(x + j, y + i * 8 + k, 1);
        } else {
          Draw_Pixe(x + j, y + i * 8 + k, 0);
        }
        tmp >>= 1;
      }
    }
  }
}

void LCDWIKI_SPI::SH1106_Display(void) {
  u8 i, n;
  CS_ACTIVE;
  for (i = 0; i < 8; i++)
  {
    writeCmd8(YC + i);
    writeCmd8(0x02);
    writeCmd8(XC);
    for (n = 0; n < WIDTH; n++)
    {
      writeData8(SH1106_buffer[i * WIDTH + n]);
    }
  }
  CS_IDLE;
}

void LCDWIKI_SPI::init_table8(const void *table, int16_t size) {
  uint8_t i;
  uint8_t *p = (uint8_t *) table, dat[MAX_REG_NUM];            //R61526 has GAMMA[22]
  while (size > 0) {
    uint8_t cmd = pgm_read_byte(p++);
    uint8_t len = pgm_read_byte(p++);
    if (cmd == TFTLCD_DELAY8) {
      delay(len);
      len = 0;
    } else {
      for (i = 0; i < len; i++) {
        dat[i] = pgm_read_byte(p++);
      }

      Push_Command(cmd, dat, len);
    }
    size -= len + 2;
  }
}

void LCDWIKI_SPI:: init_table16(const void *table, int16_t size)
{
  uint16_t *p = (uint16_t *) table;
  while (size > 0)
  {
    uint16_t cmd = pgm_read_word(p++);
    uint16_t d = pgm_read_word(p++);
    if (cmd == TFTLCD_DELAY16)
    {
      delay(d);
    }
    else
    {
      Write_Cmd_Data(cmd, d);                      //static function
    }
    size -= 2 * sizeof(int16_t);
  }
}

void LCDWIKI_SPI::start(uint16_t ID) {
  reset();
  delay(200);

  switch (ID) {

    case 0x7796:
      lcd_driver = ID_7796;
      XC = ILI9341_COLADDRSET, // 0x2A
      YC = ILI9341_PAGEADDRSET, // 0x2B
      CC = ILI9341_MEMORYWRITE, // 0x2C
      RC = HX8357_RAMRD, // 0x2E
      SC1 = 0x33, // Vertical scrolling definition
      SC2 = 0x37, // Vertical scrolling start address of RAM
      MD = ILI9341_MADCTL, // 0x36 - Memory Data Access Contro
      VL = 0, // This is to do with the display inversion control
      R24BIT = 1; // this is about reading the colour from the display in 24 bit or 16 bit

      static const uint8_t ST7796S_regValues[] PROGMEM = {
        0xF0, 1, 0xC3,
        0xF0, 1, 0x96,
        0x36, 1, 0x68,
        0x3A, 1, 0x05,
        0xB0, 1, 0x80,
        0xB6, 2, 0x00, 0x02,
        0xB5, 4, 0x02, 0x03, 0x00, 0x04,
        0xB1, 2, 0x80, 0x10,
        0xB4, 1, 0x00,
        0xB7, 1, 0xC6,
        0xC5, 1, 0x24,
        0xE4, 1, 0x31,
        0xE8, 8, 0x40, 0x8A, 0x00, 0x00, 0x29, 0x19, 0xA5, 0x33,
        0xC2, 0,
        0xA7, 0,
        0xE0, 14, 0xF0, 0x09, 0x13, 0x12, 0x12, 0x2B, 0x3C, 0x44, 0x4B, 0x1B, 0x18, 0x17, 0x1D, 0x21,
        0xE1, 14, 0xF0, 0x09, 0x13, 0x0C, 0x0D, 0x27, 0x3B, 0x44, 0x4D, 0x0B, 0x17 , 0x17, 0x1D, 0x21,
        0x36, 1, 0x48,
        0xF0, 1, 0xC3,
        0xF0, 1, 0x69,
        0x13, 0,
        0x11, 0,
        0x29, 0,
      };
      init_table8(ST7796S_regValues, sizeof(ST7796S_regValues));
      break;
    default:
      lcd_driver = ID_UNKNOWN;
      break;
  }

  Set_Rotation(rotation);
  Invert_Display(false);
}

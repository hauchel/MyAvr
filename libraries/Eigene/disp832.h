#ifndef disp832_h
#define disp832_h

#include "Arduino.h"
#include "Print.h"
#include <stddef.h>
#include <stdint.h>


#define LEDNUM 256         // Number of LEDs in stripe
#define ws2812_port PORTB  // Data port register
#define ws2812_pin 1       // Number of the data out pin there ,
#define ws2812_out 9       // resulting Arduino num
#define ARRAYLEN LEDNUM * 3
#define COLORM 10
class disp832 : public Print {
public:

  disp832();

  void begin(uint8_t cols);

  void dark();
  int arrPos(byte p);
  void pix2arr(byte p);
  void arr2pix(byte p);
  void pix2color(int a);
  void color2pix(byte a);
  bool refresh();
  void setR(byte b);
  byte getR();
  void setG(byte b);
  byte getG();
  void setB(byte b);
  byte getB();
  void fillArr();

  // support of Print class
  virtual size_t write(uint8_t ch);

private:
  bool upd;                // true refresh disp
  byte pix[3];             // current pixel
  byte color[COLORM* 3];
  uint8_t ledArray[ARRAYLEN];  // Buffer GRB
  uint8_t _cols;               ///< number of cols of the display
  void ws2812_sendarray_mask(uint8_t *, uint16_t, uint8_t);
  void ws2812_sendarray(uint8_t *data, uint16_t datlen);
};
#endif

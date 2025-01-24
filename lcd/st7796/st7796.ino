// test 320*480
//https://synapticloop.medium.com/getting-started-with-the-4-tft-spi-with-st7796s-driver-2c88a017510
//https://github.com/synapticloopltd/LCDWIKI_GUI
#include "LCDWIKI_GUI.h" // Core graphics library
#include "LCDWIKI_SPI.h" // Hardware-specific library
#include "helper.h"

/*
  GND
  VCC
  SCL  13
  SDA  11 MOSI
  RST  8
  DC   9
  CS   10
  BL   7
*/
//                      CS, CD, RST, LED
LCDWIKI_SPI mylcd(ST7796S, 10, 9, 8, 7);
#define  BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF

bool textmode;
bool ledmode;
uint16_t  myx, myy;
void doCmd(char x) {
  Serial.print(char(x));
  if (doNum(x)) {
    return;
  }
  Serial.println();
  switch (x) {
    case 13:
      vt100Clrscr();
      break;
    case 'b':
      mylcd.Set_Text_Back_Color(inp);
      msgF(F("Back"), mylcd.Get_Text_Back_Color());
      break;
    case 'c':
      mylcd.Set_Text_Color(inp);
      msgF(F("Color"), mylcd.Get_Text_Color());
      break;
    case 'f':
      mylcd.Fill_Screen(inp);
      break;
    case 'l':
      ledmode = !ledmode;
      mylcd.Led_control(ledmode);
      msgF(F("ledmode"), ledmode);
      break;
    case 'p':
      mylcd.Print_String("Hello World!", myx, myy);
      break;
    case 'q':
      mylcd.Draw_Rectangle(20, inp, 50, inp + 20);
      msgF(F("Rect"), inp);
      break;
    case 'r':
      mylcd.Set_Rotation(inp);
      msgF(F("Rot"), mylcd.Get_Rotation());
      break;
    case 't':
      textmode = !textmode;
      mylcd.Set_Text_Mode(textmode);
      msgF(F("textmode"), mylcd.Get_Text_Mode());
      break;
    case 's':
      mylcd.Set_Text_Size(inp);
      msgF(F("textsize"), mylcd.Get_Text_Size());
      break;
    case 'u':
      msgF(F("wrifill"), inp);
      while (inp < 100) {
        mylcd.Write(char(inp));
        inp++;
      }
      break;
    case 'w':
      mylcd.Write(char(inp));
      msgF(F("write"), inp);
      break;
    case 'x':
      myx = inp;
      mylcd.Set_Text_Cursor(myx, myy);
      msgF(F("Tx"), mylcd.Get_Text_X_Cursor() );
      break;
    case 'y':
      myy = inp;
      mylcd.Set_Text_Cursor(myx, myy);
      msgF(F("Ty"), mylcd.Get_Text_Y_Cursor() );
      break;
    default:
      Serial.print('?');
      Serial.println(int(x));
  } // case
}

void setup() {
  const char info[] = "st7796 " __DATE__  " "  __TIME__;
  Serial.begin(38400);
  Serial.println(info);
  mylcd.Init_LCD();
  mylcd.Set_Rotation(1);
  mylcd.Set_Text_Size(2);
  mylcd.Set_Text_Color(0xFFFF); // White
  mylcd.Set_Text_Back_Color(0x0000); // Black
  mylcd.Set_Draw_color(0xFFFF); // White
}

void loop() {
  if (Serial.available() > 0) {
    doCmd( Serial.read());
  } // serial
  currMs = millis();
  if (tickMs > 0) {
    if ((currMs - prevMs >= tickMs) ) {
      //doCmd('x');
      prevMs = currMs;
    }
  }
}

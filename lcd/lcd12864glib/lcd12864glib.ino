#include "U8glib.h"
U8GLIB_ST7920_128X64_4X u8g(10);

void setup(void) {
}

void loop(void) {
  u8g.firstPage();
  do {
    u8g.setFont(u8g_font_unifont);
    u8g.drawStr( 0, 10, "Hallo Welt");
  } while ( u8g.nextPage() );

  delay(1000);
}

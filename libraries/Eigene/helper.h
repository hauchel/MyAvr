
byte vt100 = 0; // ESC (1) 91 (2) x+128
uint16_t inp;
bool inpAkt;
unsigned long currMs, prevMs = 0;
unsigned long tickMs=50;

// Memory saving helpers
void prnt(PGM_P p) {
  // flash to serial until \0
  while (1) {
    char c = pgm_read_byte(p++);
    if (c == 0) break;
    Serial.write(c);
  }
  Serial.write(' ');
}

void msgF(const __FlashStringHelper *ifsh, int16_t n) {
  PGM_P p = reinterpret_cast<PGM_P>(ifsh);
  prnt(p);
  Serial.println(n);
}

void prnF(const __FlashStringHelper *ifsh) {
  PGM_P p = reinterpret_cast<PGM_P>(ifsh);
  prnt(p);
}

void prlnF(const __FlashStringHelper *ifsh) {
  prnF(ifsh);
  Serial.println();
}

// Docmd helper
bool doNum(byte tmp) {
  if ((tmp >= '0') && (tmp <= '9')) {
    if (inpAkt) {
      inp = inp * 10 + (tmp - '0');
    } else {
      inpAkt = true;
      inp = tmp - '0';
    }
 //   Serial.print("\b\b\b\b");
//    Serial.print(inp);
    return true;
  } //digit
  inpAkt = false;
  return false;
}

byte doVT100(byte tmp) {
  // 27 91 x  -> up 193, down 194, right 195, left 196
  switch (vt100) {
    case 0:
      if (tmp == 27) {
        vt100 = 1;
        return 0;
      }
      return tmp;
    case 1:
      if (tmp == 91) {
        vt100 = 2;
        return 0;
      }
      vt100 = 0;
      return tmp;
    case 2:
      vt100 = 0;
      return tmp + 128;
  }
  return tmp;
}

/*
  cursorup(n) CUU       Move cursor up n lines                 ^[[<n>A  use vt100Esc('A',2)
  cursordn(n) CUD       Move cursor down n lines               ^[[<n>B
  cursorrt(n) CUF       Move cursor right n lines              ^[[<n>C
  cursorlf(n) CUB       Move cursor left n lines               ^[[<n>D
  cursorhome            Move cursor to upper left corner       ^[[H
*/
void vt100Esc(char c, byte n) {
  char str[10];
  sprintf(str, "\x1B[%d%c", n, c);
  Serial.print(str);
}

void vt100Clrscr() {
  Serial.print("\x1B[H");
  Serial.print("\x1B[J");
}

void vt100Home() {
  Serial.print("\x1B[H");
}
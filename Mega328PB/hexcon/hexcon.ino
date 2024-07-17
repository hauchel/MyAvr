
const uint8_t befM = 100;  // size bef
char  bef[befM], befSav[befM];
uint8_t verb = 0;
/*
  Helpers for Serial
*/
void prnt(PGM_P p) {
  // output char * from flash,
  while (1) {
    char c = pgm_read_byte(p++);
    if (c == 0) break;
    Serial.write(c);
  }
  Serial.write(" ");
}

void msgF(const __FlashStringHelper *ifsh, uint16_t n) {
  Serial.print(F(" "));
  PGM_P p = reinterpret_cast<PGM_P>(ifsh);
  prnt(p);
  Serial.println(n);
}

void errF(const __FlashStringHelper *ifsh, int n) {
  Serial.print(F("ErrF: "));
  PGM_P p = reinterpret_cast<PGM_P>(ifsh);
  prnt(p);
  Serial.println(n);
}


uint8_t seriBef() {
  // blocking read Bef, returns 0 if terminated by CR, 1 by "
  uint8_t i;
  char b;
  //Serial.print(F("\bBef: "));
  for (i = 0; i < befM - 3; i++) { // max bef len
    while (Serial.available() == 0) {
    }
    b = Serial.read() ;
    Serial.print(b);
    if (b == 13) {
      bef[i] = 0;
      return 0;
    }
    if (b == '"') {
      bef[i] = 0;
      return 1;
    }
    bef[i] = b;
  } // loop
  errF (F("Seri "), i);
  bef[i] = 0;
  return 9;
}


void wait() {
  Serial.print("...");
  while (Serial.available() == 0) { };
  Serial.read();
  Serial.println();
}


bool fetch2(uint8_t* xp, uint8_t* xw) {
  // scans bef and returns true if ok,  w and new p
  uint8_t p;
  uint8_t myw;
  char c;
  p = *xp;

  char d[5];
  if (verb & 2) {
    Serial.printf(F("Enter p = %3u\n "), p);
  }
  if (verb & 8) {
    Serial.printf(F(" >%s< \n"), bef);
  }
  if (verb & 4) wait();

  if (bef[p] == 0) {
    errF(F("unexpected null at "), p);
    return false;
  }
  d[0] = bef[p++];
  if (verb & 1) {
    c = d[0];
    Serial.print(c);
  }
  if (bef[p] == 0) {
    errF(F("unexpected null at "), p);
    return false;
  }
  d[1] = bef[p++];
  if (verb & 1) {
    c = d[1];
    Serial.print(c);
  }
  d[2] = 0;
  if (verb & 16) {
    *xp = p;
    return true;
  }

  myw = (uint8_t)strtol(d, NULL, 16);
  if (verb & 2) {
    Serial.printf(F("Exit p = %3u w = %02X \n "), p, myw);
    if (verb & 4) wait();
  }
  *xp = p;
  *xw = myw;
  return true;
}

void hex2buf() {
  uint8_t p = 0;
  uint8_t anz, adrH, adrL, typ;
  uint8_t w;
  bool r;
  byte cs;

  r = fetch2(&p, &anz);
  if (verb & 4) wait();
  if (verb & 1) Serial.printf("at %3u anz %3u \n", p, anz);
  if (verb & 4) wait();
  if (!r) return;

  r = fetch2(&p, &adrH);
  if (verb & 1) Serial.printf(F("at %2u adrh %02X \n"), p, adrH);
  if (verb & 4) wait();
  if (!r) return;

  if (!fetch2(&p, &adrL)) return;
  if (verb & 1) Serial.printf(F("at %2u adrL %02X \n"), p, adrL);
  if (!fetch2(&p, &typ)) return;
  if (verb & 1) Serial.printf(F("at %2u typ %02X \n"), p, typ);
  for (int i = 0; i < anz; i++)  {
    if (!fetch2(&p, &w)) return;
    Serial.printf(F("at %2u wrt %02X \n"), p, w);
  }
}

bool doCmd(unsigned char c) {
  static uint16_t  inp;              // numeric input
  static bool inpAkt = false;        // true if last input was a number (is returned)
  static uint8_t cmdMode = 0;        // 1=buffer, 2,3 hex input
  static char hexs[3];

  if (cmdMode == 2) {    // 1st digit of hex
    hexs[0] = c;
    cmdMode = 3;
    return true;
  }
  if (cmdMode == 3) {    // 2nd digit of hex
    hexs[1] = c;
    hexs[2] = 0;
    inp = (uint16_t)strtol(hexs, NULL, 16);
    Serial.printf("\b\b\b\b%u", inp);
    cmdMode = 0;
    return false;
  }

  // handle numbers
  if ( c == 8) { //backspace removes last digit
    inp = inp / 10;
    return inpAkt;
  }
  if ((c >= '0') && (c <= '9')) {
    if (inpAkt) {
      inp = inp * 10 + (c - '0');
    } else {
      inpAkt = true;
      inp = c - '0';
    }
    return inpAkt;
  }
  inpAkt = false;
  switch (c) {
    case ' ':
      break;
    case 'a':
      strncpy(bef, "200E00000001020304E2", 40);
      hex2buf();
      break;
    case 'b':
      strncpy(bef, "200E0000000102030405060708090A0B0C0D0E0F101112131415161718191A1B1C1D1E1FE2", befM);
      hex2buf();
      break;
    case 'c':
      strncpy(bef, "200E1000202122232425262728292A2B2C2D2E2F303132333435363738393A3B3C3D3E3FD2", befM);
      hex2buf();
      break;
    case 'd':
      strncpy(bef, "200E2000404142434445464748494A4B4C4D4E4F505152535455565758595A5B5C5D5E5FC2", befM);
      hex2buf();
      break;
    case ':':   // to hex
      seriBef();
      hex2buf();
      Serial.println();
      break;
    case 228:   //ä 228
      cmdMode = 2;
      Serial.print("\b0x");
      break;
    case 'v':
      verb = inp;
      msgF(F("Verb"), verb);
      break;

    default:
      Serial.print ("?");
      Serial.println (uint8_t(c));
      c = '?';
  } //switch
  return inpAkt;
}

void setup() {
  const char ich[] PROGMEM = "hexcon " __DATE__  " " __TIME__;
  Serial.begin(38400);
  Serial.println(ich);
}

void loop() {
  unsigned char c;
  if (Serial.available() > 0) {
    c = Serial.read();
    Serial.print(char(c));
    if (!doCmd(c)) {
      Serial.print(F("_\b")); //else num input
    }
  }
}

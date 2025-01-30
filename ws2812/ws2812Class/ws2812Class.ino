
#include "helper.h"
#include "disp832.h"

disp832 dsp;


void errF(const __FlashStringHelper *ifsh, int n) {
  Serial.print(F("ErrF: "));
  PGM_P p = reinterpret_cast<PGM_P>(ifsh);
  prnt(p);
  Serial.println(n);
}

uint16_t inpsw;           // swap-register
const byte inpSM = 7;     // Stack for inps conf
uint16_t inpStck[inpSM];  //
byte inpSP = 0;           //

void inpPush() {
  if (inpSP >= inpSM) {
    errF(F("inp Overflow"), inpSP);
  } else {
    inpStck[inpSP] = inp;  // is inc'd on pop
    inpSP++;
  }
}

int inpPop() {
  if (inpSP > 0) {
    inpSP--;
    //msgF(F("inpPop"), inpStck[inpSP]);
    return inpStck[inpSP];
  } else {
    errF(F("inpstack Underflow"), 0);
    return 0;
  }
}

char doCmd(unsigned char tmp) {
  // handle numbers
  if (doNum(tmp)) {
    return tmp;
  }
  switch (tmp) {
    case 'b':  // set blue (2)
      dsp.setB(inp);
      break;
    case 'B':  //
      inp = dsp.getB();
      break;
    case 'c':  //
      dsp.color2pix(inp);
      break;
    case 'C':  //
      dsp.pix2color(inp);
      break;
    case 'd':
      dsp.dark();
      break;
    case 'f':  // fill
      dsp.fillArr();
      break;
    case 'g':  // set green (0)
      dsp.setG(inp);
      break;
    case 'G':  //
      inp = dsp.getG();
      break;
    case 'l':  //
      dsp.pix2arr(inp);
      break;
    case 'L':  //
      dsp.arr2pix(inp);
      break;
    case 'r':  // set red (1)
      dsp.setR(inp);
      break;
    case 'R':  //
      inp = dsp.getR();
      break;
    case 'T':
      tickMs = inp;
      msgF(F("tick "), inp);
      break;
    case ',':  //
      inpPush();
      break;
    case '+':  //
      inp = inpPop() + inp;
      break;
    case '-':  //
      inp = inpPop() - inp;
      break;
    case ' ':  //
      Serial.print(F(" inp="));
      Serial.println(inp);
      break;
    default:
      Serial.print("?");
      Serial.println(byte(tmp));
      return '?';
  }  //switch
  return tmp;
}

void setup() {
  const char ich[] PROGMEM = "ws2812Class" __DATE__ " " __TIME__;
  Serial.begin(38400);
  Serial.println(ich);
}

void loop() {
  unsigned char c;

  if (Serial.available() > 0) {
    c = Serial.read();
    Serial.print(char(c));
    doCmd(c);
  }
  currMs = millis();
  if ((currMs - prevMs >= tickMs)) {
    prevMs = currMs;
    if (dsp.refresh()) Serial.print("*");
  }
}

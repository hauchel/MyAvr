
#define DEBUG 1

#ifndef dcf77_h
#define dcf77_h

#include <Arduino.h>

class dcf77 {
    byte hourX = 99;  // temp during eval
    byte minuteX = 99;
    byte sec = 99;
#ifdef DEBUG
#define  dataL  128  // timer protocol
    uint16_t data [dataL + 2];
#endif
#define ripuL 10  // samples to take (usually 5 secs)
    unsigned long ripu [ripuL];
    unsigned long last;
    volatile byte rinP = 0;
    volatile byte routP = 0;
    volatile byte state = 0;
    volatile byte dataP = 0;
    byte error = 99;
    byte druP = 0;
    byte akt;   //
    byte aktW;  //wert
    bool aktP;  //parity
    // adapt according to frequency:
    const uint16_t lo100 =  80;
    const uint16_t hi100 =  120;
    const uint16_t lo200 =  200;
    const uint16_t hi200 =  250;
    const uint16_t lo800 =  700;
    const uint16_t hi800 =  800;
    const uint16_t lo900 =  850;
    const uint16_t hi900 = 900;
    const uint16_t sync  = 1200;
    void checkParity(byte b);
    void aktPlus(byte b);
    void druff(byte b);
    void err(byte b);
    byte newState(byte news);
    byte doSample();

  public:
    byte hour = 99;
    byte minute = 99;
    unsigned long timestamp = 0;
    byte verb = 1;        // tell me more
    dcf77();
    void dcfRead();
    void showRipu();
    void showData();
    void info();
    void act();
    void msgF(const __FlashStringHelper *ifsh, uint16_t n);
};

#endif	//dcf77_h
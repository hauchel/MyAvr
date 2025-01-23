#ifndef edilib_h
#define edilib_h
#include <avr/pgmspace.h>
#include <Arduino.h>

class Edi {
  private :

  public:
    const char retNOP =  27;
    const char retPnd =  28;
    const char retErr =  29;
    char lin[100];
    byte escSeq = 0;
    int  xPos, xLen;

    void startEdi();
    void esc(char t);
    void cursUp();
    void cursDwn();
    void cursRgt();
    void cursRgtN(int n);
    void cursLft();
    void cursPos1();
    void cursCls();
    void cursHome();
    void cursXY(int x , int y);
    void printLin();
    void printLinRgt();
    void doLft();
    void doBS(bool rea);
    void doRgt();
    void doPos1();
    void doPos4();
    void doEinf();
    char inp(char tmp);
};
#endif


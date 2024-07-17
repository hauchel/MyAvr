const byte pgmbefM = 28;
//                             Id        Reset      Prog En      Reg          Prog  read
const char s_00[] PROGMEM = "T1jd32n0eo 12jd1n3e  4jd16n41840e 11jd5n13ed16n0e  5j 10p17p18po";
const char t_00[] PROGMEM = "To Progmode and Read locks (PM)";
const char s_01[] PROGMEM = "T1j d 32n0eo";
const char t_01[] PROGMEM = "Get IDCODE (0x1)";
const char s_02[] PROGMEM = "15j d 16n1234eo";
const char t_02[] PROGMEM = "Bypass (0xF)";
const char s_03[] PROGMEM = "T4j d 16n41840e";
const char t_03[] PROGMEM = "Prog Enable (0x4) ok";
const char s_04[] PROGMEM = "T4j d 16n0e";
const char t_04[] PROGMEM = "Prog Enable (0x4) zero";
const char s_05[] PROGMEM = "T12j d 1n3e";
const char t_05[] PROGMEM = "Reset 1 (0xC)";
const char s_06[] PROGMEM = "t12j d 1n0e";
const char t_06[] PROGMEM = "Reset 0 (0xC)";
const char s_07[] PROGMEM = "0p1p2p3p5p6po7po";
const char t_07[] PROGMEM = "Read Flash";
const char s_08[] PROGMEM = "4j d 16n 0e 12j d 1n0e";
const char t_08[] PROGMEM = "Leave Prog komplett";

const char s_09[] PROGMEM = "10p11p12po";
const char t_09[] PROGMEM = "read extended";
const char s_10[] PROGMEM = "10p13p14po";
const char t_10[] PROGMEM = "read high";
const char s_11[] PROGMEM = "10p15p16po";
const char t_11[] PROGMEM = "read low";
const char s_12[] PROGMEM = "10p17p18po";
const char t_12[] PROGMEM = "read lock";

const char s_13[] PROGMEM = "4p1p2p14x"; // never change
const char t_13[] PROGMEM = "Enter first write (2,3,13x)";
const char s_14[] PROGMEM = "3p8p9p#";  // never change
const char t_14[] PROGMEM = "Enter next L (4,5,15x) and inc";
const char s_15[] PROGMEM = "20p21p22p";// never change
const char t_15[] PROGMEM = "Latch";
const char s_16[] PROGMEM = "23p24p25p26p27po"; // never change
const char t_16[] PROGMEM = "Write Flash Page";
const char s_17[] PROGMEM = "13x 15x 16x";
const char t_17[] PROGMEM = "Wri 1 mal a,b,17x";
const char s_18[] PROGMEM = "13x 15x 14x 15x 16x";
const char t_18[] PROGMEM = "Wri 2 mal a,b,c,d 18x";
const char s_19[] PROGMEM = "13x 15x 14x 15x 14x 15x 16x";
const char t_19[] PROGMEM = "Wri 3 mal";
const char s_20[] PROGMEM = "64,65,66,67,68,69";
const char t_20[] PROGMEM = "Put 6 on stack ";
const char s_21[] PROGMEM = "4bi0bm 0bw 1bi1bm 1bw 2bi2bm 2bw 3bi3bm 3bw";
const char t_21[] PROGMEM = "Testdata in Flash 0-3";
const char s_22[] PROGMEM = "0l0brw 64l1brw 128l2brw 192l3brw";
const char t_22[] PROGMEM = "Schreibe 4 pages";
const char s_23[] PROGMEM = "0brbt 0l  rbc 1brbt 64l rbc 2brbt 128l rbc 3brbt 192l rbc";
const char t_23[] PROGMEM = "Vergleiche ";
const char s_24[] PROGMEM = "";
const char t_24[] PROGMEM = "";
const char s_25[] PROGMEM = "rbh#rbh#rbh#rbh#";
const char t_25[] PROGMEM = "Lese 4 out Hex";
const char s_26[] PROGMEM = "";
const char t_26[] PROGMEM = "";
const char s_27[] PROGMEM = "";
const char t_27[] PROGMEM = "";


const char *const pgmbef[pgmbefM] PROGMEM = {
  s_00, s_01, s_02, s_03, s_04, s_05, s_06, s_07, s_08, s_09, s_10, s_11, s_12, s_13, s_14, s_15, s_16, s_17, s_18, s_19,
  s_20, s_21, s_22, s_23, s_24, s_25, s_26, s_27,
};
//,  s_28, s_29, s_30, s_31, s_32, s_33, s_34, s_35, s_36, s_37, s_38, s_39

const char *const pgmtxt[pgmbefM] PROGMEM = {
  t_00, t_01, t_02, t_03, t_04, t_05, t_06, t_07, t_08, t_09, t_10, t_11, t_12, t_13, t_14, t_15, t_16, t_17, t_18, t_19,
  t_20, t_21, t_22, t_23, t_24, t_25, t_26, t_27,
};
//  t_28, t_29,


int befP;                   // next to execute
byte befNum;                // current bef
const byte stackM = 5;      // Stack depth for prog exec
byte stackP;                // points to next free
int stackBefP[stackM];      // saved befP
byte stackBefNum[stackM];   // saved befNum
const byte befM = 100;  // size bef
char  bef[befM], befSav[befM];

void bef2sav() {
  memcpy(befSav, bef, befM);
}
void sav2bef() {
  memcpy(bef, befSav, befM);
}

void showTxt(byte num) {
  // reads and prints
  if (num >= pgmbefM) {
    errF(F("showTxt "), num);
  } else {
    prnt((char *)pgm_read_word(&(pgmtxt[num])));
    Serial.println();
  }
}

void showTxtAll() {
  // reads and prints
  Serial.println(F("\b xe"));
  for (int i = 0; i < pgmbefM; i++) {
    Serial.printf(F("%3u "), i);
    showTxt(i);
  }
}

bool pgm2Bef(byte num) {
  // gets bef from flash or saved
  if (verb & 16) msgF(F("pgm2Bef"), num);
  if (num == 255) {
    sav2bef();
  } else {
    if (num >= pgmbefM) {
      errF(F("pgm2Bef "), num);
      return false;
    }
    strcpy_P(bef, (char *)pgm_read_word(&(pgmbef[num])));
  }
  befNum = num;
  befP=0; // possibly changed later
  return true;
}

void showBef() {
  char bf;
  if (befP < 0) {
    bf = '?';
  } else {
    bf = bef[befP];
  }
  Serial.printf(F("\r%3d  %2d : ' "), befNum, befP);
  Serial.print(bf);
  Serial.print("' '");
  Serial.print(bef);
  Serial.println("'");
}

byte seriBef() {
  // blocking read Bef, returns 0 if terminated by CR, 1 by "
  byte i;
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


void showStack() {
  Serial.printf(F("\rNum %3u  P %3u  stackP %2u \n"), befNum, befP, stackP);
  for (int i = 0; i < stackM; i++) {
    Serial.printf(F(" %3u   %3u \n"), stackBefNum[i], stackBefP[i]);
  }
}

void prgPush() {
  if (verb & 16) msgF(F("prgPush"), befNum);
  if (stackP >= stackM) {
    errF(F("Stack Overflow"), stackP);
    runMode = 0;
  } else {
    stackBefP[stackP] = befP;
    stackBefNum[stackP] = befNum;
    stackP++;
  }
}

void prgPop() {
  if (stackP > 0) {
    stackP--;
    befP = stackBefP[stackP] + 1;
    befNum = stackBefNum[stackP] ;
    if (verb & 16) msgF(F("prgPop"), befNum);
    if (!pgm2Bef(befNum)) runMode = 0;
  } else {
    errF(F("prg stack Underflow"), 0);
    runMode = 0;
  }
}

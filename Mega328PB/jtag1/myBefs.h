const byte pgmbefM = 20;
const char s_00[] PROGMEM = "31x32x33x34x";   //start sequence in 0x
const char t_00[] PROGMEM = "Block";
const char s_01[] PROGMEM = "H18n t1j d 0eL"; // Text1 pos,1x
const char t_01[] PROGMEM = "Get IDCODE (0x1)";
const char s_02[] PROGMEM = "H16n t15j d 1234eL";
const char t_02[] PROGMEM = "Bypass (0xF)";
const char s_03[] PROGMEM = "";
const char t_03[] PROGMEM = "Dummy";
const char s_04[] PROGMEM = "40x41x42x43x";//call flash
const char t_04[] PROGMEM = "Dummy";
const char s_05[] PROGMEM = "50x51x52x53x";
const char t_05[] PROGMEM = "Dummy";
const char s_06[] PROGMEM = "";
const char t_06[] PROGMEM = "Dummy";
const char s_07[] PROGMEM = "";
const char t_07[] PROGMEM = "Dummy";
const char s_08[] PROGMEM = "8,10,5,1a13,11,5,2ai"; //assign agents
const char t_08[] PROGMEM = "Dummy";
const char s_09[] PROGMEM = "3,1t4,2t5,3to";
const char t_09[] PROGMEM = "Dummy";
const char s_10[] PROGMEM = "0<c0+l,1<c0+,1d,l";  // 1 led in m[0/1] running forward
const char t_10[] PROGMEM = "Dummy";
const char s_11[] PROGMEM = "2<c0+l,3<c0+,1d,l";  // 1 led in m[2/3] running forward
const char t_11[] PROGMEM = "Dummy";
const char s_12[] PROGMEM = "4<c0+l,5<c0+,1d,l";  // 1 led in m[4/5] running forward
const char t_12[] PROGMEM = "Dummy";
const char s_13[] PROGMEM = "0<c0+l,1<c0+,2d,l";   //1 led in m[0/1] running backward
const char t_13[] PROGMEM = "Dummy";
const char s_14[] PROGMEM =  "2<c0+,1-,2d,l";   //1 led in col 3 running backward
const char t_14[] PROGMEM = "Dummy";
const char s_15[] PROGMEM = "";
const char t_15[] PROGMEM = "Dummy";
const char s_16[] PROGMEM = "";
const char t_16[] PROGMEM = "Dummy";
const char s_17[] PROGMEM = "";
const char t_17[] PROGMEM = "Dummy";
const char s_18[] PROGMEM = "";
const char t_18[] PROGMEM = "Dummy";
const char s_19[] PROGMEM = "";
const char t_19[] PROGMEM = "Dummy";

const char *const pgmbef[pgmbefM] PROGMEM = {s_00, s_01, s_02, s_03, s_04, s_05, s_06, s_07, s_08, s_09, s_10, s_11, s_12, s_13, s_14, s_15, s_16, s_17, s_18, s_19
                                            };
//, s_20, s_21, s_22, s_23, s_24, s_25, s_26, s_27, s_28, s_29, s_30, s_31, s_32, s_33, s_34, s_35, s_36, s_37, s_38, s_39

const char *const pgmtxt[pgmbefM] PROGMEM =
{ t_00, t_01, t_02, t_03, t_04, t_05, t_06, t_07, t_08, t_09, t_10, t_11, t_12, t_13, t_14, t_15, t_16, t_17, t_18, t_19
};
//  t_20, t_21, t_22, t_23, t_24, t_25, t_26, t_27, t_28, t_29, //t_10, t_11, t_12, t_13, t_14, t_15, t_16, t_17, t_18, t_19,


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
  Serial.println(F("gols:"));
  for (int i = 0; i < pgmbefM; i++) {
    prn3u(i);
    showTxt(i);
  }
}

// stuff to handle JTAG program commands
// will become class when stable


const byte ftxM = 30;
//Flash access first char of Text is type, see prog()
//                             ! 
const char ftx_00[] PROGMEM = "  3a  Enter Flash Read";
const char ftx_01[] PROGMEM = "K 3b  Load Address EHB (+K)";
const char ftx_02[] PROGMEM = "H 3c  Load Address  HB (+H)";
const char ftx_03[] PROGMEM = "L 3d  Load Address  LB (+L)";
const char ftx_04[] PROGMEM = "  2a  Enter Flash Write";
const char ftx_05[] PROGMEM = "  3e1 Read Low and High Byte";
const char ftx_06[] PROGMEM = "  3e2 Read Low and High Byte";
const char ftx_07[] PROGMEM = "  3e3 Read Low and High Byte";
const char ftx_08[] PROGMEM = "D 2e  Load Data LB (+D)";
const char ftx_09[] PROGMEM = "D 2f  Load Data HB (+D)";
// Fuse Reads
const char ftx_10[] PROGMEM = "  8a  Enter F/L Bit Read";
const char ftx_11[] PROGMEM = "  8b1 Read Extended ";
const char ftx_12[] PROGMEM = "  8b2 Read Extended ";
const char ftx_13[] PROGMEM = "  8c1 Read Fuse High";
const char ftx_14[] PROGMEM = "  8c2 Read Fuse High";
const char ftx_15[] PROGMEM = "  8d1 Read Fuse Low";
const char ftx_16[] PROGMEM = "  8d2 Read Fuse Low";
const char ftx_17[] PROGMEM = "  8e1 Read Lock";
const char ftx_18[] PROGMEM = "  8e2 Read Low";
const char ftx_19[] PROGMEM = "-";
// More
const char ftx_20[] PROGMEM = "  2g1 Latch Data";
const char ftx_21[] PROGMEM = "  2g2 Latch Data";
const char ftx_22[] PROGMEM = "  2g3 Latch Data";
const char ftx_23[] PROGMEM = "  2h1 Write Flash Page";
const char ftx_24[] PROGMEM = "  2h2 Write Flash Page";
const char ftx_25[] PROGMEM = "  2h3 Write Flash Page";
const char ftx_26[] PROGMEM = "  2h4 Write Flash Page";
const char ftx_27[] PROGMEM = "  2i  Poll for Page Write Complete";
const char ftx_28[] PROGMEM = "-";
const char ftx_29[] PROGMEM = "-";
const uint16_t fuff[ftxM] = {
  //      0 5               1 6                2 7              3 8                    4 9
  0b010001100000010, 0b000101100000000, 0b000011100000000, 0b000001100000000, 0b010001100010000,
  0b011001000000000, 0b011011000000000, 0b011011100000000, 0b001001100000000, 0b001011100000000,
  0b010001100000100, 0b011101000000000, 0b011101100000000, 0b011111000000000, 0b011111100000000,
  0b011001000000000, 0b011001100000000, 0b011011000000000, 0b011011100000000,        0,
  0b011011100000000, 0b111011100000000, 0b011011100000000, 0b011011100000000, 0b011010100000000,
  0b011011100000000, 0b011011100000000, 0b011011100000000,   0, 0
};
const char *const ftx[ftxM] PROGMEM = {ftx_00, ftx_01, ftx_02, ftx_03, ftx_04, ftx_05, ftx_06, ftx_07, ftx_08, ftx_09,
                                       ftx_10, ftx_11, ftx_12, ftx_13, ftx_14, ftx_15, ftx_16, ftx_17, ftx_18, ftx_19,
                                       ftx_20, ftx_21, ftx_22, ftx_23, ftx_24, ftx_25, ftx_26, ftx_27, ftx_28, ftx_29
                                      };


void showFtx(byte num) {
  if (num >= ftxM) {
    errF(F("showFtx "), num);
  } else {
    Serial.printf(F("%3u   %04X : "), num,  fuff[num]);
    prnt((char *)pgm_read_word(&(ftx[num])));
    Serial.println();
  }
}


void showFtxAll() {
  // reads and prints
  Serial.println(F("FTXe:"));
  for (int i = 0; i < ftxM; i++) {
    showFtx(i);
  }
}

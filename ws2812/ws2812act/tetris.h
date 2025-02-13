/* replaces myfunc.h
      Tetris has those 7 blocks:
      I,      J,   L,      O,   S,       T      Z
      x       x    x      xx    xx      xxx    xx
      x       x    x      xy     yx      y      yx
      x      xy    yx
      y

*/
const byte charbefM = 70; // set inp to hotspot then call.
// generated by tertis.py
const char c_00[] PROGMEM = "l,8-l,8-l,8-l";
const char c_30[] PROGMEM = "],8-],8-],8-]3d";
const char c_01[] PROGMEM = "l,1+l,2-l,1-l";
const char c_31[] PROGMEM = "],1+],2-],1-]3d";
const char c_02[] PROGMEM = "l,8-l,8-l,8-l";
const char c_32[] PROGMEM = "],8-],8-],8-]3d";
const char c_03[] PROGMEM = "l,1+l,2-l,1-l";
const char c_33[] PROGMEM = "],1+],2-],1-]3d";
const char c_04[] PROGMEM = "l,1-l,7-l,8-l";
const char c_34[] PROGMEM = "],1-],7-],8-]3d";
const char c_05[] PROGMEM = "l,2+l,1-l,9-l";
const char c_35[] PROGMEM = "],2+],1-],9-]3d";
const char c_06[] PROGMEM = "l,8-l,7-l,1-l";
const char c_36[] PROGMEM = "],8-],7-],1-]3d";
const char c_07[] PROGMEM = "l,9+l,8-l,2-l";
const char c_37[] PROGMEM = "],9+],8-],2-]3d";
const char c_08[] PROGMEM = "l,1+l,9-l,8-l";
const char c_38[] PROGMEM = "],1+],9-],8-]3d";
const char c_09[] PROGMEM = "l,8+l,6-l,1-l";
const char c_39[] PROGMEM = "],8+],6-],1-]3d";
const char c_10[] PROGMEM = "l,8-l,8-l,1-l";
const char c_40[] PROGMEM = "],8-],8-],1-]3d";
const char c_11[] PROGMEM = "l,1+l,2-l,6-l";
const char c_41[] PROGMEM = "],1+],2-],6-]3d";
const char c_12[] PROGMEM = "l,1-l,7-l,1-l";
const char c_42[] PROGMEM = "],1-],7-],1-]3d";
const char c_13[] PROGMEM = "l,1-l,7-l,1-l";
const char c_43[] PROGMEM = "],1-],7-],1-]3d";
const char c_14[] PROGMEM = "l,1-l,7-l,1-l";
const char c_44[] PROGMEM = "],1-],7-],1-]3d";
const char c_15[] PROGMEM = "l,1-l,7-l,1-l";
const char c_45[] PROGMEM = "],1-],7-],1-]3d";
const char c_16[] PROGMEM = "l,1+l,9-l,1-l";
const char c_46[] PROGMEM = "],1+],9-],1-]3d";
const char c_17[] PROGMEM = "l,7+l,8-l,7-l";
const char c_47[] PROGMEM = "],7+],8-],7-]3d";
const char c_18[] PROGMEM = "l,1+l,9-l,1-l";
const char c_48[] PROGMEM = "],1+],9-],1-]3d";
const char c_19[] PROGMEM = "l,7+l,8-l,7-l";
const char c_49[] PROGMEM = "],7+],8-],7-]3d";
const char c_20[] PROGMEM = "l,1-l,6-l,1-l";
const char c_50[] PROGMEM = "],1-],6-],1-]3d";
const char c_21[] PROGMEM = "l,9+l,8-l,9-l";
const char c_51[] PROGMEM = "],9+],8-],9-]3d";
const char c_22[] PROGMEM = "l,1+l,7-l,1-l";
const char c_52[] PROGMEM = "],1+],7-],1-]3d";
const char c_23[] PROGMEM = "l,9+l,8-l,9-l";
const char c_53[] PROGMEM = "],9+],8-],9-]3d";
const char c_24[] PROGMEM = "l,8+l,7-l,2-l";
const char c_54[] PROGMEM = "],8+],7-],2-]3d";
const char c_25[] PROGMEM = "l,8+l,9-l,7-l";
const char c_55[] PROGMEM = "],8+],9-],7-]3d";
const char c_26[] PROGMEM = "l,1+l,2-l,7-l";
const char c_56[] PROGMEM = "],1+],2-],7-]3d";
const char c_27[] PROGMEM = "l,8+l,7-l,9-l";
const char c_57[] PROGMEM = "],8+],7-],9-]3d";

const char c_28[] PROGMEM = "";
const char c_58[] PROGMEM = "";
const char c_29[] PROGMEM = "";
const char c_59[] PROGMEM = "";

const char c_60[] PROGMEM = "220,11d0,12d1,13d5e";   //I
const char c_61[] PROGMEM = "220,11d4,12d2,13d5e";   //J
const char c_62[] PROGMEM = "220,11d8,12d3,13d5e";   //L
const char c_63[] PROGMEM = "220,11d12,12d4,13d5e";  //O
const char c_64[] PROGMEM = "220,11d16,12d5,13d5e";  //S
const char c_65[] PROGMEM = "220,11d20,12d6,13d5e";  //T
const char c_66[] PROGMEM = "220,11d24,12d7,13d5e";  //Z
const char c_67[] PROGMEM = "";   //Fade_up
const char c_68[] PROGMEM = "";   //Fade_up
const char c_69[] PROGMEM = "last y!";   //Fade_up

const char *const charbefehle[charbefM] PROGMEM =
{ c_00, c_01, c_02, c_03, c_04, c_05, c_06, c_07, c_08, c_09, c_10, c_11, c_12, c_13, c_14, c_15, c_16, c_17, c_18, c_19,
  c_20, c_21, c_22, c_23, c_24, c_25, c_26, c_27, c_28, c_29, c_30, c_31, c_32, c_33, c_34, c_35, c_36, c_37, c_38, c_39,
  c_40, c_41, c_42, c_43, c_44, c_45, c_46, c_47, c_48, c_49, c_50, c_51, c_52, c_53, c_54, c_55, c_56, c_57, c_58, c_59,
  c_60, c_61, c_62, c_63, c_64, c_65, c_66, c_67, c_68, c_69,
};

// Function declarations

int inpPop();
void wasPush(int was);
void exec(byte num, byte was);
void color2pix(byte a);

//calling: was contains function requested, stack params

byte tetled;  // current led
byte tetobj;  // sprite
byte tetcol;  // its col
byte newled;
byte newobj;

void dodo(byte nled, byte nobj) {
  color2pix(0);
  inp = tetled;
  exec(tetobj, 1); // remove
  newled = nled;
  newobj = nobj;
  evqPut(9);   //# have it checked
  evqPut(5);   //# have it drawn
}

void dochk() {
  msgF(F("docheck"),tetobj);
  exec(tetobj + 30, 1); //run check which will call 3 if allowed
  //check
}

void doswitch() {
  msgF(F("doswitch"),newled);
  tetled = newled;
  tetobj = newobj;
}

void doFuncs(byte was) {  // call
  byte tmp;
  msgF(F("dofuncs"), was);
  switch (was) {
    case 1:   // Rot left
      tmp = tetobj % 4;
      if (tmp == 0) {
        dodo(tetled, tetobj + 3);
      } else {
        dodo(tetled, tetobj - 1);
      }
      break;
    case 2:   // Rot right
      tmp = tetobj % 4;
      if (tmp == 3) {
        dodo(tetled, tetobj - 3);
      } else {
        dodo(tetled, tetobj + 1);
      }
      break;
    case 3:   //
      doswitch();
      break;
    case 4:   // up
      dodo(tetled + 8, tetobj);
      break;
    case 5:   // draw
      color2pix(tetcol);
      inp = tetled;
      exec(tetobj, 1); //only prepares Bef but does not
      break;
    case 6:   // down
      dodo(tetled - 8, tetobj);
      break;
    case 7:   // left
      dodo(tetled - 1, tetobj);
      break;
    case 8:   // right
      dodo(tetled + 1, tetobj);
      break;
    case 9:   //
      dochk();
      break;
    case 11:   // set led
      tetled = inpPop();
      break;
    case 12:   // set obj
      tetobj = inpPop();
      break;
    case 13:   // set col
      tetcol = inpPop();
      break;
    case 31:   // key for left
      evqPut(7);
      break;
    case 47:   // key for down
      evqPut(6);
      break;
    case 55:   // key for right
      evqPut(8);
      break;
    case 59:   // key for rotL
      evqPut(1);
      break;
    case 61:   // key for rotR
      evqPut(2);
      break;
    case 62:   // key for up
      evqPut(4);
      break;
    case 63:   // all keys up
      break;
    default:
      errF(F("doFuncs"), was);
      Serial.print(F("tetled "));
      Serial.print(tetled);
      Serial.print(F(" tetobj "));
      Serial.print(tetobj);
      Serial.print(F(" tetcol "));
      Serial.println(tetcol);
  } //switch
}

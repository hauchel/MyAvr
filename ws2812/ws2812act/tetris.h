/* Tetris has those blocks:
      I,      J,   L,      O,   S,       T      Z
      x       x    x      xx    xx      xxx    xx
      x       x    x      xy     yx      y      yx
      x      xy    yx
      y

*/
#ifdef TETRIS
const byte charbefM = 40; // set inp to hotspot then call.
const char c_00[] PROGMEM = "l,8-l,8-l,8-l";
const char c_01[] PROGMEM = "l,1+l,2-l,1-l";
const char c_02[] PROGMEM = "l,8-l,8-l,8-l";
const char c_03[] PROGMEM = "l,1+l,2-l,1-l";
const char c_04[] PROGMEM = "l,1-l,7-l,8-l";
const char c_05[] PROGMEM = "l,2+l,1-l,9-l";
const char c_06[] PROGMEM = "l,8-l,7-l,1-l";
const char c_07[] PROGMEM = "l,9+l,8-l,2-l";
const char c_08[] PROGMEM = "l,1+l,9-l,8-l";
const char c_09[] PROGMEM = "l,8+l,6-l,1-l";
const char c_10[] PROGMEM = "l,8-l,8-l,1-l";
const char c_11[] PROGMEM = "l,1+l,2-l,6-l";
const char c_12[] PROGMEM = "l,1-l,7-l,1-l";
const char c_13[] PROGMEM = "l,1+l,8-l,1-l";
const char c_14[] PROGMEM = "l,1-l,7-l,1-l";
const char c_15[] PROGMEM = "l,1+l,8-l,1-l";
const char c_16[] PROGMEM = "l,1+l,9-l,1-l";
const char c_17[] PROGMEM = "l,8+l,7-l,8-l";
const char c_18[] PROGMEM = "l,1+l,9-l,1-l";
const char c_19[] PROGMEM = "l,7+l,8-l,7-l";
const char c_20[] PROGMEM = ",8+l#l#l#l#l,8-l,16+l,12+"; //T
const char c_21[] PROGMEM = "l#l#l#l#l,4+l,8+l#l#l#l#l,12+";   //U
const char c_22[] PROGMEM = "0+l#l#l#l";   //Balk
const char c_23[] PROGMEM = "0~4,23,0U~c,1+~,22yu";   //Balk6
const char c_24[] PROGMEM = "0+,~0+L9C4,23,4,~+UL,4-lu,9c4-l";   //Ring
const char c_25[] PROGMEM = "1,3,0TU,24yut0j";   //Balk_Rot
const char c_26[] PROGMEM = "";
const char c_27[] PROGMEM = "";
const char c_28[] PROGMEM = "";
const char c_29[] PROGMEM = "";
const char c_30[] PROGMEM = "220,11d0,12d1,13d5e";   //I
const char c_31[] PROGMEM = "220,11d4,12d2,13d5e";   //J
const char c_32[] PROGMEM = "220,11d8,12d3,13d5e";   //L
const char c_33[] PROGMEM = "220,11d12,12d4,13d5e";  //O
const char c_34[] PROGMEM = "220,11d16,12d5,13d5e";  //S
const char c_35[] PROGMEM = "220,11d20,12d6,13d5e";  //T
const char c_36[] PROGMEM = "220,11d24,12d7,13d5e";  //Z
const char c_37[] PROGMEM = "";   //Fade_up
const char c_38[] PROGMEM = "";   //Fade_up
const char c_39[] PROGMEM = "last!";   //Fade_up

const char *const charbefehle[charbefM] PROGMEM = {c_00, c_01, c_02, c_03, c_04, c_05, c_06, c_07, c_08, c_09, c_10, c_11, c_12, c_13, c_14, c_15, c_16, c_17, c_18, c_19, c_20, c_21, c_22, c_23, c_24, c_25, c_26, c_27, c_28, c_29, c_30, c_31, c_32, c_33, c_34, c_35, c_36, c_37, c_38, c_39};

// Function declarations

int inpPop();
void wasPush(int was);
void exec(byte num, byte was);
void color2pix(byte a);

//calling: was contains function requested, stack params

byte tetled;  // current led
byte tetobj;  // sprite
byte tetcol;  // its col

void dodo(byte newled, byte newobj) {
  color2pix(0);
  inp = tetled;
  exec(tetobj, 1);
  tetled = newled;
  tetobj = newobj;
  evqPut(5);   //# have it drawn
}

void doFuncs(byte was) {  // call
  msgF(F("dofuncs"), was);
  switch (was) {
    case 1:   // Rot left
      dodo(tetled, tetobj - 1);
      break;
    case 2:   // Rot right
      dodo(tetled, tetobj + 1);
      break;
    case 3:   // free
      break;
    case 4:   // free
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
      dodo(tetled + 1tetobj);
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

#endif

/* states
  0   init
  1   Positioned all to feed
  2   card gelesen,
  3   Möglichkeiten: ablegen, direkt loopen, anderen loopen, card is 0
  4   card in tray
  5   zu Ziel und abgelegt -> 1
  
  6

  10  höchsten von Ablage, ablZiel festlegent
  11  hole ablZiel
  12  loope   ->
  
  20   raus
  21   ist raus, what next?

  
  101   Karte nicht lesbar
  102   kann nicht ablegen
  103   alles kassiert
  113   switch state invalid
  115   Halt pin



*/
// all names max 20 else %$@
const char nixx[] PROGMEM = "";

const char s_0[] PROGMEM = "Rein";
const char s_1[] PROGMEM = "Raus";
const char *const servNam[anzServ] PROGMEM = {s_0, s_1};


//positions
const char p_00[] PROGMEM = "Min";
const char p_01[] PROGMEM = "Draus";
const char p_02[] PROGMEM = "Schieb";
const char p_03[] PROGMEM = "Justa";
const char p_04[] PROGMEM = "";
const char p_05[] PROGMEM = "";
const char p_06[] PROGMEM = "";
const char p_07[] PROGMEM = "";
const char p_08[] PROGMEM = "";
const char p_09[] PROGMEM = "Max";
//
const char p_10[] PROGMEM = "Min";
const char p_11[] PROGMEM = "Eingef";
const char p_12[] PROGMEM = "";
const char p_13[] PROGMEM = "Stat2";
const char p_14[] PROGMEM = "LadeFeed";
const char p_15[] PROGMEM = "Entlad";
const char p_16[] PROGMEM = "stat5";
const char p_17[] PROGMEM = "string 7<";
const char p_18[] PROGMEM = "Raus";
const char p_19[] PROGMEM = "Max";

//
const char *const posNam[anzServ][10] PROGMEM = {p_00, p_01, p_02, p_03, p_04, p_05, p_06, p_07, p_08, p_09,
                                                 p_10, p_11, p_12, p_13, p_14, p_15, p_16, p_17, p_18, p_19,
                                                };

const char pr_0[] PROGMEM = "";
const char pr_1[] PROGMEM = "";
const char pr_2[] PROGMEM = "Rein";
const char pr_3[] PROGMEM = "Raus";
const char pr_4[] PROGMEM = "";
const char pr_5[] PROGMEM = "";
const char pr_6[] PROGMEM = "";
const char pr_7[] PROGMEM = "";
const char pr_8[] PROGMEM = "";
const char pr_9[] PROGMEM = "";
const char *const progNam[20] PROGMEM = {pr_0, pr_1, pr_2, pr_3, pr_4, pr_5, pr_6, pr_7, pr_8, pr_9, nixx, nixx, nixx, nixx, nixx, nixx, nixx, nixx, nixx, nixx};

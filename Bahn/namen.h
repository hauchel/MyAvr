// all names max 20 else %$@
const char nixx[] PROGMEM = "";

const char s_0[] PROGMEM = "Stepper";
const char s_1[] PROGMEM = "Horiz";
const char s_2[] PROGMEM = "Vert";
const char s_3[] PROGMEM = "";
const char s_4[] PROGMEM = "AusGreif";
const char s_5[] PROGMEM = "AusDreh";
const char s_6[] PROGMEM = "";
const char s_7[] PROGMEM = "";
const char s_8[] PROGMEM = "string 8<";
const char s_9[] PROGMEM = "string 9<";
const char *const servNam[anzServ] PROGMEM = {s_0, s_1, s_2, s_3, s_4, s_5, s_6, s_7};

//positions
const char p_00[] PROGMEM = "Null";
const char p_01[] PROGMEM = "Feeder";
const char p_02[] PROGMEM = "Stat1";
const char p_03[] PROGMEM = "Stat2";
const char p_04[] PROGMEM = "Stat3";
const char p_05[] PROGMEM = "Stat4";
const char p_06[] PROGMEM = "stat5";
const char p_07[] PROGMEM = "?";
const char p_08[] PROGMEM = "Raus";
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
const char p_20[] PROGMEM = "Min";
const char p_21[] PROGMEM = "Eingef";
const char p_22[] PROGMEM = "Stat1";
const char p_23[] PROGMEM = "Stat2";
const char p_24[] PROGMEM = "LadeFeed";
const char p_25[] PROGMEM = "Entlad";
const char p_26[] PROGMEM = "stat5";
const char p_27[] PROGMEM = "string 7<";
const char p_28[] PROGMEM = "Raus";
const char p_29[] PROGMEM = "Max";

const char *const posNam[anzServ][10] PROGMEM = {p_00, p_01, p_02, p_03, p_04, p_05, p_06, p_07, p_08, p_09,
                                                 p_10, p_11, p_12, p_13, p_14, p_15, p_16, p_17, p_18, p_19,
                                                 p_20, p_21, p_22, p_23, p_24, p_25, p_26, p_27, p_28, p_29,
                                                 nixx, nixx, nixx, nixx, nixx, nixx, nixx, nixx, nixx, nixx,
                                                 nixx, nixx, nixx, nixx, nixx, nixx, nixx, nixx, nixx, nixx,
                                                 nixx, nixx, nixx, nixx, nixx, nixx, nixx, nixx, nixx, nixx,
                                                 nixx, nixx, nixx, nixx, nixx, nixx, nixx, nixx, nixx, nixx,
                                                 nixx, nixx, nixx, nixx, nixx, nixx, nixx, nixx, nixx, nixx,
                                                };

// progs
const char pr_0[] PROGMEM = " ";
const char pr_1[] PROGMEM = "Grundst";
const char pr_2[] PROGMEM = "to Feed";
const char pr_3[] PROGMEM = "Feed to Move";
const char pr_4[] PROGMEM = "Entladen";
const char pr_5[] PROGMEM = "to Loop";
const char pr_6[] PROGMEM = "Laden";
const char pr_7[] PROGMEM = "Dreh Grundst";
const char pr_8[] PROGMEM = "Raus";
const char pr_9[] PROGMEM = "Zu & Rum";
const char *const progNam[20] PROGMEM = {pr_0, pr_1, pr_2, pr_3, pr_4, pr_5, pr_6, pr_7, pr_8, pr_9, nixx, nixx, nixx, nixx, nixx, nixx, nixx, nixx, nixx, nixx};

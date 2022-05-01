/* states
  0   init
  1   Positioned all to feed
  2   card gelesen, drei Möglichkeiten: ablegen, direkt loopen, anderen loopen
  3   card in tray
  4   zu Ziel
  5   ablegen -> 1




101   Karte nicht lesbar
102   kann nicht ablegen
103
113   switch state invlid
115   Halt pin



*/
// names max 20
const char s_0[] PROGMEM = "0 Rein";
const char s_1[] PROGMEM = "1 Raus";
const char s_2[] PROGMEM = "2 Vert";
const char s_3[] PROGMEM = "1x2x3x0j";
const char s_4[] PROGMEM = "4 AusGreif";
const char s_5[] PROGMEM = "5 AusDreh";
const char s_6[] PROGMEM = "string 6<\x00";
const char s_7[] PROGMEM = "string 7<";
const char s_8[] PROGMEM = "string 8<";
const char s_9[] PROGMEM = "string 9<";
const char *const servNam[anzServ] PROGMEM = {s_0, s_1};

const char p_0[] PROGMEM = "0 Stepper";
const char p_1[] PROGMEM = "1 Horiz";
const char p_2[] PROGMEM = "2 Vert";
const char p_3[] PROGMEM = "1x2x3x0j";
const char p_4[] PROGMEM = "string 4<\x00";
const char p_5[] PROGMEM = "string 5<\x00";
const char p_6[] PROGMEM = "string 6<\x00";
const char p_7[] PROGMEM = "string 7<";
const char p_8[] PROGMEM = "string 8<";
const char p_9[] PROGMEM = "string 9<";
const char *const progNam[10] PROGMEM = {p_0, p_1, p_2, p_3, p_4,}; // p_5, p_6, p_7, p_8, p_9};

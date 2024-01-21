//
//  SDA green   A4
//  SCL blue   A5
//  Analog     A0
#include <EEPROM.h>
#include <Wire.h>
#include <Adafruit_INA219.h>
Adafruit_INA219 ina219;


uint8_t mcpAdr = 0x60;
uint16_t inp;
bool inpAkt;

#define MCP4725_I2CADD_DEFAULT 0x62 //A0-Pin ohne Beschaltung
#define MCP4725_I2CADD_A0_VDD  0x63 //A0-Pin auf VDD

#define MCP4725_CMD_WRITE_FAST_MODE   0x00
#define MCP4725_CMD_WRITEDAC          0x40
#define MCP4725_CMD_WRITEDAC_EEPROM   0x60

#define MCP4725_NORMAL_MODE     0x00
#define MCP4725_POWER_DOWN_1K   0x10
#define MCP4725_POWER_DOWN_100K 0x20
#define MCP4725_POWER_DOWN_500K 0x30

#define MCP4725_EEPROM_READY    0x80

#define MCP4725_NO_EEPROM    0
#define MCP4725_WRITE_EEPROM 1

void setVoltageFast(uint16_t val) {
  byte lowVal, highVal;
  lowVal = lowByte(val);
  highVal = highByte(val);
  Wire.beginTransmission(mcpAdr);
  Wire.write(MCP4725_CMD_WRITE_FAST_MODE | highVal );
  Wire.write(lowVal);
  Wire.endTransmission();
}
void setVoltage( uint16_t output, bool writeEEPROM ) {
  Wire.beginTransmission(mcpAdr);
  if (writeEEPROM)  {
    Wire.write(MCP4725_CMD_WRITEDAC_EEPROM);
  }  else   {
    Wire.write(MCP4725_CMD_WRITEDAC);
  }
  Wire.write(output / 16);                   // Upper data bits          (D11.D10.D9.D8.D7.D6.D5.D4)
  Wire.write((output % 16) << 4);            // Lower data bits          (D3.D2.D1.D0.x.x.x.x)
  Wire.endTransmission();
}

unsigned int readRegister() {
  uint8_t regByte[3];
  unsigned int regVal;
  byte i = 0;
  Wire.requestFrom(mcpAdr, uint8_t(3));
  while (Wire.available())
  {
    regByte[i] = Wire.read();
    i++;
  }
  regVal = (((regByte[1] << 8) + regByte[2]) >> 4) & 0xFFF;
  return regVal;
}

unsigned int readEEPROM() {
  uint8_t regByte[5];
  unsigned int eepromVal;
  byte i = 0;
  //Warte, bis letzter Schreibvorgang ins EEPROM beendet ist
  Wire.requestFrom(mcpAdr, uint8_t(5));
  while (Wire.available())
  {
    regByte[i] = Wire.read();
    i++;
  }
  eepromVal = (regByte[3] << 8) + regByte[4];
  return eepromVal;
}

bool setMode(byte mode) {
  byte _mode = mode;
  bool modeOk = false;

  if (_mode == MCP4725_NORMAL_MODE || _mode == MCP4725_POWER_DOWN_1K
      || _mode == MCP4725_POWER_DOWN_100K || _mode == MCP4725_POWER_DOWN_500K)   {
    Wire.beginTransmission(mcpAdr);
    Wire.write(_mode);
    Wire.write(0);
    Wire.endTransmission();
    modeOk = true;
  }

  if (modeOk) return true;
  else return false;
}

void setFreq(byte b) {
  unsigned long freq;
  switch (b) {
    case 0:   //
      freq = 100000L;
      break;
    case 1:   //
      freq = 400000L;
      break;
    case 2:   //
      freq = 600000L;
      break;
    case 3:   //
      freq = 33333L;
      break;
    case 9:   //
      freq = 1000000L; // geht nicht
      break;
    default:
      msg("setFreq Unknown ?? ", b);
      return;
  }
  msg("Freq to ", b);
  Wire.setClock(freq);
  msg("TWBR:", TWBR);
  msg("TWSR:", TWSR & 3);
  /*
    TWSR 0..3 = 1,4,16,64
    SCL Frequency = CPU Clock Frequency / (16 + (2 * TWBR))
    note: TWBR should be 10 or higher for master mode
    It is 72 for a 16mhz  with 100kHz TWI */
}

const PROGMEM uint16_t DACLookup_FullSine_9Bit[512] =
{
  2048, 2073, 2098, 2123, 2148, 2174, 2199, 2224,
  2249, 2274, 2299, 2324, 2349, 2373, 2398, 2423,
  2448, 2472, 2497, 2521, 2546, 2570, 2594, 2618,
  2643, 2667, 2690, 2714, 2738, 2762, 2785, 2808,
  2832, 2855, 2878, 2901, 2924, 2946, 2969, 2991,
  3013, 3036, 3057, 3079, 3101, 3122, 3144, 3165,
  3186, 3207, 3227, 3248, 3268, 3288, 3308, 3328,
  3347, 3367, 3386, 3405, 3423, 3442, 3460, 3478,
  3496, 3514, 3531, 3548, 3565, 3582, 3599, 3615,
  3631, 3647, 3663, 3678, 3693, 3708, 3722, 3737,
  3751, 3765, 3778, 3792, 3805, 3817, 3830, 3842,
  3854, 3866, 3877, 3888, 3899, 3910, 3920, 3930,
  3940, 3950, 3959, 3968, 3976, 3985, 3993, 4000,
  4008, 4015, 4022, 4028, 4035, 4041, 4046, 4052,
  4057, 4061, 4066, 4070, 4074, 4077, 4081, 4084,
  4086, 4088, 4090, 4092, 4094, 4095, 4095, 4095,
  4095, 4095, 4095, 4095, 4094, 4092, 4090, 4088,
  4086, 4084, 4081, 4077, 4074, 4070, 4066, 4061,
  4057, 4052, 4046, 4041, 4035, 4028, 4022, 4015,
  4008, 4000, 3993, 3985, 3976, 3968, 3959, 3950,
  3940, 3930, 3920, 3910, 3899, 3888, 3877, 3866,
  3854, 3842, 3830, 3817, 3805, 3792, 3778, 3765,
  3751, 3737, 3722, 3708, 3693, 3678, 3663, 3647,
  3631, 3615, 3599, 3582, 3565, 3548, 3531, 3514,
  3496, 3478, 3460, 3442, 3423, 3405, 3386, 3367,
  3347, 3328, 3308, 3288, 3268, 3248, 3227, 3207,
  3186, 3165, 3144, 3122, 3101, 3079, 3057, 3036,
  3013, 2991, 2969, 2946, 2924, 2901, 2878, 2855,
  2832, 2808, 2785, 2762, 2738, 2714, 2690, 2667,
  2643, 2618, 2594, 2570, 2546, 2521, 2497, 2472,
  2448, 2423, 2398, 2373, 2349, 2324, 2299, 2274,
  2249, 2224, 2199, 2174, 2148, 2123, 2098, 2073,
  2048, 2023, 1998, 1973, 1948, 1922, 1897, 1872,
  1847, 1822, 1797, 1772, 1747, 1723, 1698, 1673,
  1648, 1624, 1599, 1575, 1550, 1526, 1502, 1478,
  1453, 1429, 1406, 1382, 1358, 1334, 1311, 1288,
  1264, 1241, 1218, 1195, 1172, 1150, 1127, 1105,
  1083, 1060, 1039, 1017,  995,  974,  952,  931,
  910,  889,  869,  848,  828,  808,  788,  768,
  749,  729,  710,  691,  673,  654,  636,  618,
  600,  582,  565,  548,  531,  514,  497,  481,
  465,  449,  433,  418,  403,  388,  374,  359,
  345,  331,  318,  304,  291,  279,  266,  254,
  242,  230,  219,  208,  197,  186,  176,  166,
  156,  146,  137,  128,  120,  111,  103,   96,
  88,   81,   74,   68,   61,   55,   50,   44,
  39,   35,   30,   26,   22,   19,   15,   12,
  10,    8,    6,    4,    2,    1,    1,    0,
  0,    0,    1,    1,    2,    4,    6,    8,
  10,   12,   15,   19,   22,   26,   30,   35,
  39,   44,   50,   55,   61,   68,   74,   81,
  88,   96,  103,  111,  120,  128,  137,  146,
  156,  166,  176,  186,  197,  208,  219,  230,
  242,  254,  266,  279,  291,  304,  318,  331,
  345,  359,  374,  388,  403,  418,  433,  449,
  465,  481,  497,  514,  531,  548,  565,  582,
  600,  618,  636,  654,  673,  691,  710,  729,
  749,  768,  788,  808,  828,  848,  869,  889,
  910,  931,  952,  974,  995, 1017, 1039, 1060,
  1083, 1105, 1127, 1150, 1172, 1195, 1218, 1241,
  1264, 1288, 1311, 1334, 1358, 1382, 1406, 1429,
  1453, 1478, 1502, 1526, 1550, 1575, 1599, 1624,
  1648, 1673, 1698, 1723, 1747, 1772, 1797, 1822,
  1847, 1872, 1897, 1922, 1948, 1973, 1998, 2023
};

const PROGMEM uint16_t DACLookup_FullSine_8Bit[256] =
{
  2048, 2098, 2148, 2198, 2248, 2298, 2348, 2398,
  2447, 2496, 2545, 2594, 2642, 2690, 2737, 2784,
  2831, 2877, 2923, 2968, 3013, 3057, 3100, 3143,
  3185, 3226, 3267, 3307, 3346, 3385, 3423, 3459,
  3495, 3530, 3565, 3598, 3630, 3662, 3692, 3722,
  3750, 3777, 3804, 3829, 3853, 3876, 3898, 3919,
  3939, 3958, 3975, 3992, 4007, 4021, 4034, 4045,
  4056, 4065, 4073, 4080, 4085, 4089, 4093, 4094,
  4095, 4094, 4093, 4089, 4085, 4080, 4073, 4065,
  4056, 4045, 4034, 4021, 4007, 3992, 3975, 3958,
  3939, 3919, 3898, 3876, 3853, 3829, 3804, 3777,
  3750, 3722, 3692, 3662, 3630, 3598, 3565, 3530,
  3495, 3459, 3423, 3385, 3346, 3307, 3267, 3226,
  3185, 3143, 3100, 3057, 3013, 2968, 2923, 2877,
  2831, 2784, 2737, 2690, 2642, 2594, 2545, 2496,
  2447, 2398, 2348, 2298, 2248, 2198, 2148, 2098,
  2048, 1997, 1947, 1897, 1847, 1797, 1747, 1697,
  1648, 1599, 1550, 1501, 1453, 1405, 1358, 1311,
  1264, 1218, 1172, 1127, 1082, 1038,  995,  952,
  910,  869,  828,  788,  749,  710,  672,  636,
  600,  565,  530,  497,  465,  433,  403,  373,
  345,  318,  291,  266,  242,  219,  197,  176,
  156,  137,  120,  103,   88,   74,   61,   50,
  39,   30,   22,   15,   10,    6,    2,    1,
  0,    1,    2,    6,   10,   15,   22,   30,
  39,   50,   61,   74,   88,  103,  120,  137,
  156,  176,  197,  219,  242,  266,  291,  318,
  345,  373,  403,  433,  465,  497,  530,  565,
  600,  636,  672,  710,  749,  788,  828,  869,
  910,  952,  995, 1038, 1082, 1127, 1172, 1218,
  1264, 1311, 1358, 1405, 1453, 1501, 1550, 1599,
  1648, 1697, 1747, 1797, 1847, 1897, 1947, 1997
};

int eadr = 0;       // EEprom Addr
const byte ledPin = 13;  // indicators
byte ledCnt = 3;
bool verbo = false;
// analog
int av1, oav1, outV;
bool avChange;

void sin9() {
  while (Serial.available() == 0) {
    for (uint16_t i = 0; i < 512; i++)   {
      setVoltage(pgm_read_word(&(DACLookup_FullSine_9Bit[i])), false);
    }
  }
}

void sin8() {
  while (Serial.available() == 0) {
    for (uint16_t i = 0; i < 256; i++)
    {
      setVoltage(pgm_read_word(&(DACLookup_FullSine_8Bit[i])), false);
    }
  }
}


void sin9F() {
  while (Serial.available() == 0) {
    for (uint16_t i = 0; i < 512; i++)   {
      setVoltageFast(pgm_read_word(&(DACLookup_FullSine_9Bit[i])));
    }
  }
}

void sin8F() {
  while (Serial.available() == 0) {
    for (uint16_t i = 0; i < 256; i++)
    {
      setVoltageFast(pgm_read_word(&(DACLookup_FullSine_8Bit[i])));
    }
  }
}
unsigned long runStart = 0;
unsigned long currTim;
unsigned long nexTim = 0;
unsigned long meZeit = 0;
unsigned long tick = 50;     //
byte runP = 0;


void chng(int neu) {
  outV = outV + neu;
  if (outV < 0) {
    outV = 0;
  } else {
    if (outV > 4095) {
      outV = 4095;
    }
  }
  msg (" Change to ", outV);
  setVoltageFast(outV);
  av1 = analogRead(A0);
  msg("av1 ", av1);
}


void readAn() {
  int di;
  avChange = false;
  av1 = analogRead(A1);
  di = abs(av1 - oav1);
  if (di > 3) {
    oav1 = av1;
    msg("av1 ", av1);
    avChange = true;
  }
  /*
    av2 = analogRead(A2);
    di = abs(av2 - oav2);
    if (di > 3) {
      oav2 = av2;
      msg("av2 ", av2);
      avChange = true;
    }
  */
}

void readIna() {
  float shuntvoltage = 0;
  float busvoltage = 0;
  float current_mA = 0;
  float loadvoltage = 0;
  float power_mW = 0;

  shuntvoltage = ina219.getShuntVoltage_mV();
  busvoltage = ina219.getBusVoltage_V();
  current_mA = ina219.getCurrent_mA();
  power_mW = ina219.getPower_mW();
  loadvoltage = busvoltage + (shuntvoltage / 1000);

  Serial.print("Bus Voltage:   "); Serial.print(busvoltage); Serial.println(" V");
  Serial.print("Shunt Voltage: "); Serial.print(shuntvoltage); Serial.println(" mV");
  Serial.print("Load Voltage:  "); Serial.print(loadvoltage); Serial.println(" V");
  Serial.print("Current:       "); Serial.print(current_mA); Serial.println(" mA");
  Serial.print("Power:         "); Serial.print(power_mW); Serial.println(" mW");
  Serial.println("");
}


void msg(const char txt[], int n) {
  Serial.print(txt);
  Serial.print(" ");
  Serial.println(n);
}


void toEpromInt(int v) {
  byte b;
  b = lowByte(v);
  EEPROM.update(eadr, b) ;
  eadr++;
  b = highByte(v);
  EEPROM.update(eadr, b) ;
  eadr++;
}


int fromEpromInt() {
  byte bl, bh;
  int tmp;
  bl = EEPROM.read(eadr) ;
  eadr++;
  bh = EEPROM.read(eadr) ;
  eadr++;
  tmp = 256 * bh + bl;
  return tmp;
}


void i2c_scan() {
  byte error, address;
  int nDevices;
  Serial.println("Scanning...");
  nDevices = 0;
  for (address = 1; address < 127; address++ )
  {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    if (error == 0)
    {
      Serial.print("I2C device found at address 0x");
      if (address < 16)
        Serial.print("0");
      Serial.print(address, HEX);
      Serial.println(".");
      nDevices++;
    }
    else if (error == 4)
    {
      Serial.print("Unknown error at address 0x");
      if (address < 16)
        Serial.print("0");
      Serial.println(address, HEX);
    }
  }
  if (nDevices == 0)
    Serial.println("No I2C devices found\n");
  else
    Serial.println("done\n");
}


void doCmd( char tmp) {
  bool weg = false;
  Serial.print (tmp);
  if ( tmp == 8) { //backspace removes last digit
    weg = true;
    inp = inp / 10;
  }
  if ((tmp >= '0') && (tmp <= '9')) {
    weg = true;
    if (inpAkt) {
      inp = inp * 10 + (tmp - '0');
    } else {
      inpAkt = true;
      inp = tmp - '0';
    }
  }
  if (weg) {
    return;
  }
  inpAkt = false;
  Serial.print("\b");
  switch (tmp) {
    case 'a':
      msg("Read Register ", readRegister());
      break;
    case 'b':
      msg("Read Eprom ", readEEPROM());
      break;
        case 'd':   //
      i2c_scan();
      break;
    case 'f':   //
      setFreq(inp);
      break;
    case 'i':   //
      readIna();
      break;
  
    case 'k':   //
      chng(+10);
      break;
    case 'l':   //
      chng(+100);
      break;

    case 'o':   //
      outV = inp;
      chng(0);
      break;

    case 'r':   //
      av1 = analogRead(A0);
      msg("av1 ", av1);
      break;
    case 's':   //
      msg("sin 8 Fast ", av1);
      sin8F();
      break;
    case 't':   //
      msg("sin 9 Fast", av1);
      sin9F();
      break;
    case 'S':   //
      msg("sin 8 ", av1);
      sin8();
      break;
    case 'T':   //
      msg("sin 9 ", av1);
      sin9();
      break;
    case 'v':   //
      verbo = !verbo;
      msg("Verbo", verbo);
      break;
    case 'w':   //
      //    fromEprom(true);
      break;
    case 'W':   //
      //  toEprom();
      break;

    case '+':   //
      chng(+1);
      break;
    case '-':   //
      chng(-1);
      break;

    default:
      msg("?? v s S t T r +-  I j k l  f", tmp);
  } // case

}

void setup() {
  const char ich[] = "mcp4725  " __DATE__  " "  __TIME__;
  Serial.begin(38400);
  Serial.println(ich);
  pinMode(A0, INPUT);
  pinMode(A1, INPUT_PULLUP);
  pinMode(A2, INPUT_PULLUP);
  pinMode(ledPin, OUTPUT);
  Wire.begin(mcpAdr);
  if (! ina219.begin()) {
    Serial.println("Failed to find INA219 chip");
  }
  //  fromEprom(false);
}

void loop() {
  char tmp;

  if (Serial.available() > 0) {
    Serial.println();
    tmp = Serial.read();
    Serial.print(char(tmp));
    doCmd(tmp);
  } // avail


  currTim = millis();
  if (nexTim < currTim) {
    nexTim = currTim + tick;
    if (ledCnt == 0) {
      digitalWrite(ledPin, LOW);
    }
    if (ledCnt > 0) {
      ledCnt--;
    }

  }  //tick


}

#include <Wire.h>

const PROGMEM uint16_t DACLookup_FullSine_6Bit[64] =
{
  2048, 2248, 2447, 2642, 2831, 3013, 3185, 3346,
  3495, 3630, 3750, 3853, 3939, 4007, 4056, 4085,
  4095, 4085, 4056, 4007, 3939, 3853, 3750, 3630,
  3495, 3346, 3185, 3013, 2831, 2642, 2447, 2248,
  2048, 1847, 1648, 1453, 1264, 1082,  910,  749,
  600,  465,  345,  242,  156,   88,   39,   10,
  0,   10,   39,   88,  156,  242,  345,  465,
  600,  749,  910, 1082, 1264, 1453, 1648, 1847
};


const PROGMEM uint16_t DACLookup_FullSine_5Bit[32] =
{
  2048, 2447, 2831, 3185, 3495, 3750, 3939, 4056,
  4095, 4056, 3939, 3750, 3495, 3185, 2831, 2447,
  2048, 1648, 1264,  910,  600,  345,  156,   39,
  0,   39,  156,  345,  600,  910, 1264, 1648
};

#define MCP4726_CMD_WRITEDAC       (0x40)  // Writes data to the DAC
#define MCP4726_CMD_WRITEDACEEPROM (0x60)  // Writes data to the DAC and the EEPROM 


void setVoltage( uint16_t output )
{
  Wire.beginTransmission(0x62);
  Wire.write(MCP4726_CMD_WRITEDAC);
  Wire.write(output / 16);                   // Upper data bits          (D11.D10.D9.D8.D7.D6.D5.D4)
  Wire.write((output % 16) << 4);            // Lower data bits          (D3.D2.D1.D0.x.x.x.x)
  Wire.endTransmission();
}


void setVoltFast( uint16_t output )
{
  byte lowWert, highWert;
  lowWert = lowByte(output);
  highWert = highByte(output);
  Wire.beginTransmission(0x62);
  Wire.write(highWert & 0x0F);
  Wire.write(lowWert);
  Wire.endTransmission();
}

void setup() {
  const char info[] = "mcp4725  "__DATE__ " " __TIME__;
  Serial.begin(38400);
  Serial.println(info);
  Wire.begin();
}

void sine() {
  uint16_t i, j;
  for (j = 0; j < 128; j++) {
    for (i = 0; i < 64; i++)
    {
      setVoltFast(pgm_read_word(&(DACLookup_FullSine_6Bit[i])));
    }
  }
}

void tine() {
  uint16_t i, j;
  for (j = 0; j < 255; j++) {
    for (i = 0; i < 32; i++)
    {
      setVoltFast(pgm_read_word(&(DACLookup_FullSine_5Bit[i])));
    }
  }
}



void loop() {
  char tmp;

  if (Serial.available() > 0) {
    tmp = Serial.read();

    switch (tmp) {
      case '1':   //
        setVoltage(100);
        break;
      case '2':   //
        setVoltage(200);
        break;
      case '5':   //
        setVoltage(3500);
        break;
      case '9':   //
        setVoltage(4095);
        break;
      case 's':   //
        sine();
        break;
      case 't':   //
        tine();
        break;
      case 'y':   // 72:   36 Hz
        Wire.setClock(100000);
        Serial.println(TWBR);
        break;
      case 'x':   // 12:  109 Hz
        Wire.setClock(400000 );
        Serial.println(TWBR);
        break;
      case 'c':   // 2    162 Hz
        Wire.setClock(800000);
        Serial.println(TWBR);
        break;
      case 'v':   //
        TWBR=1;
        Serial.println(TWBR);
        break;
      default:
        Serial.print("?");
    } //case

    Serial.println();
  } // serial

}

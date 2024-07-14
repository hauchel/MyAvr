Just a text info file
Values to put in IR:
Tap={
     '0x0':'??'
     '0x1':'IDCODE',		// Standard 32 bit done returns 0x4978203F
     '0x2':'??'
     '0x3':'??'
     '0x4':'Prog Enable',	// Prog send '0xA370' = 41840 = 1010 0011 0111 0000
     '0x5':'Prog Command', 	// Prog 15 bit
     '0x6':'Prog Pageload', 	// Prog
     '0x7':'Prog Pageread', 	// Prog     
     '0x8':'Force Break',	// Debug	
     '0x9':'Run',			    // Debug	
     '0xA':"AVR Instr",		// Debug
     '0xB':'Reg write',		// Debug
     '0xC':'Reset'		// Prog
     '0xD':'??'			// ??
     '0xE':'??'			// ??
     '0xF':'Bypass'		// Standard 1 bit shift done
         }

Programming:
  Reset
  ProgEnable
  ProgCommand


Basic commands

EXTEST
SAMPLE/PRELOAD

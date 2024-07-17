Just a text file wirth my notes, .h to see it in IDE
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

Combination of bas are stored in

Commands see doCmd() and doBufCmd()
Config stored in EEProm, selected

  c   select config
  h   set current  H Address
  l   set current  L Address
  k   set current  extended (which alwys is 0)
  bz      showConfigs();
  bf      fillBuff(inp);
  bn
  bm
  bg      getConfigs() from EEProm
  bp      putConfigs() to   EEProm
  bi      set inc   
  bs      showBuff();
  br      readmyPage(inp);
  bw      writemyPage(inp);

Hi Level 


Jtag-Related



Memory details:

Target pagesize is 128 words = 256 bytes
at328 pagesize is 128 bytes, need 2 pages to store one target page
Example:pages   Addr        mypage
6000  607F      60 00     0    0 
                60 40    64    1
6080  60FF      60 80   128    2
                60 C0   192    3
6100  617F      61 00     0    4
                         64    5

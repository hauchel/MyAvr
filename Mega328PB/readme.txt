e
Um den Assemblercode zu erzeugen, musst du folgendes tun:
1. Aktiviere in den Arduino-Einstellungen "Ausführliche Ausgabe 
während:" "Kompilierung"
2. Übersetze deinen Sketch
3. Notiere dir das in den letzten Zeilen der Ausgabe genannte 
Arduino-Installationsverzeichnis (bei mir zum Beispiel 
C:\\Users\\Hobbyraum\\AppData\\Local\\Arduino15\\packages\\arduino\\tool 
s\\avr-gcc\\7.3.0-atmel3.6.1-arduino7/bin)  und das temporäre 
Sketchverzeichnis (bei mir zum Beispiel 
C:\\Users\\HOBBYR~1\\AppData\\Local\\Temp\\arduino_build_174782).
4. Öffne ein Windows Eingabeaufforderung
5. Wechsle in der Eingabeaufforderung mit "cd 
C:\\Users\\HOBBYR~1\\AppData\\Local\\Temp\\arduino_build_174782" (oder 
wie auch immer es bei dir gerade heißt) in das temporäre 
Sketchverzeichnis.
6. Führe in der Eingabeaufforderung "dir" aus. Eine der Dateien dort 
sollte die Endung ".elf" haben (bei mir zum Beispiel "gen1mhz.ino.elf").
7. Führe in der Eingabeaufforderung auf der .elf-Datei das Kommando 
avr-objdump mit den Optionen -d (disassemblieren) und -S (mit Sourcecode 
verschränken) aus, und leite die Ausgabe in eine neue Datei um (bei mit 
zum Beispiel 
"C:\\Users\\Hobbyraum\\AppData\\Local\\Arduino15\\packages\\arduino\\too 
ls\\avr-gcc\\7.3.0-atmel3.6.1-arduino7/bin/avr-objdump"  -dS 
gen1mhz.ino.elf >gen1mhz.ino.elf.lst)
8. Öffne das temporäre Sketchverzeichnis in einem Windows 
Dateiexplorer-Fenster
9. Öffne dort die entstandene Datei mit der Endung ".lst" mit einem 
Editor deiner Wahl. Dort findest du den erzeugten Assemblercode.

C:\Program Files (x86)\Arduino\hardware\tools\avr/bin/avrdude -CC:\Program Files (x86)\Arduino\hardware\tools\avr/etc/avrdude.conf -v -patmega328p -carduino -PCOM3 -b115200 -D -Uflash:w:C:\Users\hh\AppData\Local\Temp\arduino_build_777481/ws2812act.ino.hex:i 

avrdude: Version 6.3-20190619


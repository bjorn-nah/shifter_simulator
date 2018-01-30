@echo off

set name="shifter_simulator.nes"

set CC65_HOME=..\

cc65 -Oi shifter_simulator.c --add-source
ca65 crt0.s
ca65 shifter_simulator.s

ld65 -C nrom_128_horz.cfg -o %name% crt0.o shifter_simulator.o nes.lib

pause

del *.o
del shifter_simulator.s

C:\dev\nes\fceux\fceux.exe %name%
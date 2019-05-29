# PCF8563Sync
Arduino sketch built to set the date and time on a PCF8563 either automatically or manually

Code by Gavin Tryzbiak
Inspiration from Paul Stoffregen's DS1307RTC Library

### Important:
* The Serial Monitor should be set to "No line ending" in the dropdown menu at the bottom.
* The Serial Monitor MUST match "#define BAUD_RATE XXXX" from line 40 in the dropdown menu of the Serial Monitor! By default it is 115200, but it can be set lower if your cable has a poor connection, or higher for quicker text output.

### Note:
* This sketch will attempt to sync your PCF8563 to your computer's time. However, the time is pulled as soon as the sketch begins compiling. Compile time, sketch upload time, and I2C tramission time are not accounted for so the time on your PCF8563 may be a few seconds late.
* You may need to change your register! Do this by changing "#define I2C_ADDRESS 0x51" from line 41 to whatever register your chip uses. This sketch does not support the serparate r/w address version of the chip.
* You may think your chip has separate addresses, but use an "I2CScanner" sketch to check, as it may actually be a single-address chip.

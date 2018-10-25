Here you will find only the modified files from the original ArduCAM library.
The settings are for:
    - Arduino MKR1000
    - ADC HX 711

##Changes
    - Added: void startUsingOldTare(unsigned int t, long tareValue)
        It tares the load cell using a value stored in the memory (EEPROM)
        NOTE: It should be already implemented in the newest version of the library with a different name

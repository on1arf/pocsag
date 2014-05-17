POCSAG library + demo-application for si4432-based ISM modules
--------------------------------------------------------------


This arduino application can create and send send 512 bps
alphanumeric POCSAG-messages using si4432-based ISM transceivers.

It consists of:
- a library called "Pocsag", tne code to create an alpha-numeric
pocsag-message

- two demo-application to send pocsag-messages using si4432-based
RF-modules: one designed for use on ISM-band frequencies (433/434 +
863-870 Mhz) and a "ham" version using 430-440 Mhz.


Release-information:
Version: 0.1.1 (21040501)
Version: 0.1.2 (21040517)




The demo-application use the "RadioHead" arduino library:
http://www.airspayce.com/mikem/arduino/RadioHead/

Note that, as the RadioHead library by default does not allow
packets larger then 50 bytes, one line need to be changed in the
Readhead class before compiling the "pocsag" demo applications:

In the file libraries/RadioHead/RH_RF22.h:
#define RH_RF22_MAX_MESSAGE_LEN 50

Should be changed to:
#define RH_RF22_MAX_MESSAGE_LEN 255



Have fun!

73
Kristoff - ON1ARF

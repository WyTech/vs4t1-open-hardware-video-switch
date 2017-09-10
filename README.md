# VS4T1 video switch open hardware platform.
This is a repository of source code, firmware and hardware reference docs for the VS4T1.
VS4T1 is the switch platform used in the now discontinued products:
CamSwitcher, CamSwitcherPro, CamSwitcherII, XVideo10, XVideo26, and RSVSwitch devices.

Schematics and example code are included in this repo.
The VS4T1 video switch contains an AVR ATTiny2313 CPU, MAX232 drivers, video switching circuitry, and a dual use Serial/ISP RJ45 port. 
The processor can be easily replaced with another with increased memory.

This project can be duplicated on Arduino platforms by using the video switch portion of the circuit, which is simply some
high frequency transistors and resistors tied to output lines.

Features of the VS4T1 are:
- Serial I/O
- 4 available I/O lines
- Video rated bus with RCA connectors. 
- Video switching via high speed transistors to connect video ports to output bus.

Firmware examples are available for the following:
- RS-232 control of settings
- Interactive RS-232 control of video switching
- Serial status feedback for home control interfacing
- Relay closure detection
- Video switching
- X-10 CM11A host 
- X-10 MR26A host
- Video sequencing via timer control

Programming requires a custom cable to adapt the standard ISP connector to the RJ45 interface.
Serial communication is accomplished with a custom RJ45 to DB9 serial cable 

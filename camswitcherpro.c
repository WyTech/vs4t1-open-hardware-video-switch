// VS4T1 Video/Audio 4-to-1 switching cable controller
// CamSwitcherPro video switch firmware
// (C) 2013 Alan Capesius 
// The code released under Open Source Expat MIT License
// See license-mit-expat.txt for details

// Fuses:
//	Brown-out detection disabled BODLEVEL=1111
//	Int RC Osc 8Mhz 65ms  CKSEL=0100
//  Serial program downloading Enabled SPIEN=0
// Clock: 8Mhz
// Based on ATTiny2313 2048 bytes flash, 128 bytes RAM, 128 bytes EEPROM

#include <ctype.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
//#include <string.h>
#include <avr/wdt.h>

// this line makes EEP file used by avrdude to program eeprom
uint8_t EEMEM eepromdefaults[7]={"S14CQI\0"};

// eeprom storage variables
struct {
	unsigned char scanmode;
	unsigned char cam;
	unsigned char maxcam;
	unsigned char speed;
	unsigned char events;
	unsigned char sensors;
} ee;

unsigned char *eeptr=0x0000; 			
unsigned char allowbaudchange = 0;

 void TransmitByte( unsigned char data )
{
	while ( !(UCSRA & (1<<UDRE)) );    // Wait for empty transmit buffer 
	UDR = data; 			        // Start transmission 
}

void TransmitString( PGM_P msg)
{
	unsigned char x = 0;
	unsigned char c;

	while( (c = pgm_read_byte(msg+x)) ){
		if(c=='\n'){
			TransmitByte('\r');
		}
		TransmitByte(c);
		x++;
	}

}



void InitUART( unsigned char baud )
{

	// Set baud rate 
	UBRRH = 0;
	UBRRL = (unsigned char)baud;
	
	// Enable receiver and transmitter
	UCSRB = (1<<RXEN)|(1<<TXEN);
	
	// Set frame format: 8N1
	UCSRC = (3<<UCSZ0);

}

// switch to camera 1-4 or zero=all off
void setcam(unsigned char cam){
	if(cam>'4') cam = '1';

	if(cam=='0'){
		PORTB = 0x00;					// pullups off, video off
	} else {
		PORTB = (1<<(cam-'1'));   // shift 1 cam-'1' positions left
	}
	
	ee.cam = cam;

	if(ee.events=='E'){					// send event if set to
		TransmitByte(ee.cam);
	}
}

unsigned char getcam(unsigned char bits)
{
	unsigned char cam = '0';

	// convert raw bits to camera number
	if(bits & 0x02){			
		cam = '2';
	} else {
		if(bits & 0x04){		
			cam = '3';
		} else {
			if(bits & 0x08){	
				cam = '4';
			}
		}
	} 

	return cam;
}

SIGNAL(SIG_OVERFLOW1)
{
	if(ee.cam>=ee.maxcam){
		ee.cam = '0';
	}
	
	setcam(++ee.cam);
	TCNT1 = 26473;				// 5 sec

}


void setspeed(unsigned char speed)
{
	unsigned char speeds[4] = {207,103,51,25};
	// set serial speed in bps based on 8Mhz clock
	// A	207	2400
	// B	103	4800
	// C	51	9600
	// D	25	19200
	InitUART(speeds[speed-'A']); 	
	ee.speed = speed;
}

void setscanmode(inchar)
{
	ee.scanmode = inchar;

	if(inchar=='M'){				// manual select camera
		TIMSK = 0;					// disable timer1 overflow
	} else {
		// autoscan cameras
		TCNT1 = 26473;				// 5 sec
		TIMSK |= _BV(TOIE1);		// enable timer1 overflow
	}
}

void applysettings()
{
	// baud rate, cam #, and scan mode must be applied to the hardware to control 
	// the ports and timer the other settings are used to control program flow
	setspeed(ee.speed);
	setscanmode(ee.scanmode);
	setcam(ee.cam);
}

// load defaults from eeprom
void LoadDefaults()
{
	eeprom_read_block(&ee, eeptr, sizeof(ee));
	applysettings();
}

// Main - a simple test program
int main( void )
{
	// clear any watchdog flags and disable watchdog resets
	// watchdog is used to reset via software and must be disabled here to prevent continuous resets
	// watchdog is not used for watchdog resets, only for software reset and is disabled by fuses at startup
	// this code captures and resets the watchdog timer after a software reset	MCUSR = 0;
	wdt_disable();

	// factory defaults for ee structure
	PGM_P defaults = 	PSTR("S14CQI");

	// long strings stored in program space for messaging
	PGM_P banner = 	PSTR("\nCamSwitcherPro Public Domain Rev B\n");
	PGM_P help = 	PSTR("Line: 0=Off,1*,2,3,4\nEvts: E=Evts Q=Quiet*\nMode: S=Scan* M=Manual\n Max: X=2 Y=3 Z=4*\n Cfg: W=Write F=Factory\n Get: G=Line H=Sensors\nSens: I=Off* J=On\n BPS: A=2400 B=4800 C=9600* D=19200 (8N1)\nMore: R=Reset ?=help !=Unlock BPS\n*=default\n");

	unsigned char sensorcam = '0';			// camera selected by sensor
	unsigned char oldsensorcam = '0';		// previous sensorcam value
	unsigned char savecam = '0';			// camera selected before sensor tripped
	unsigned char debounce = 0;

	DDRB = 0x0F;    // set portb PB0-3 to outputs, PB4-7 as inputs, (PB4 line not used)
	PORTB = 0x00;	// set top 4 bits low to disable pullups, low 4 bits low to turn off video
	//MCUCR = 0x00;	// pullups off
	DDRD = 0x00;	// serial I/O on port D

	TCCR1B = (1<<CS10) | (1<<CS12);		// prescale /1024, normal mode 8 sec

	LoadDefaults();

	TransmitString(banner);

	sei();

  	while (1) {

		if( (UCSRA & (1<<RXC)) ){ ; 	// Check for incoming data 
			cli();

			unsigned char inchar = toupper(UDR);		

			if(inchar>='0' && inchar <='4'){
				// switch camera input
				setcam(inchar);
			}
			
			if(inchar>='A' && inchar <='D'){
				if(allowbaudchange){
					setspeed(inchar);
					allowbaudchange = 0;
				}
			}
			
			if(inchar=='E' || inchar =='Q'){
				// events are sent to serial port on/off
				ee.events = inchar;
			}
			
			if(inchar=='F'){
				// factory defaults
				memcpy_P(&ee,defaults,sizeof(ee));
				applysettings();
			}

			if(inchar=='G'){
				// query current camera setting
				TransmitByte(ee.cam);
			}

			if(inchar=='H'){
				// query current sensor setting
				TransmitByte('0' + ((PINB & 0x40)>>6) );  // RJ45 pin 6, PB7, 100
				TransmitByte('0' + ((PINB & 0x80)>>7) );  // RJ45 pin 7, PB5, 010
				TransmitByte('0' + ((PINB & 0x20)>>5) );  // RJ45 pin 8, PB6, 001
			}

			if(inchar=='I' || inchar=='J'){
				ee.sensors = inchar;
			}			

			if(inchar=='W'){
				// save settings to eeprom  $$$
				eeprom_write_block(&ee, eeptr, sizeof(ee));
			}

			if(inchar=='M' || inchar=='S'){
				setscanmode(inchar);
			}

			if(inchar=='R'){
				// reset device
				wdt_enable(WDTO_15MS);
   				for(;;);
			}


			if(inchar>='X' && inchar<='Z'){
				// set max cam to 2,3 or 4
				ee.maxcam = '4' + inchar - 'Z';  // '4' + 'Y' - 'Z' = '3' so character is converted to camera number
			}

			if(inchar=='!'){
				allowbaudchange = 1;
			}

			if(inchar=='?'){
				// show banner
				TransmitString(banner);
				TransmitString(help);
			}

			sei();
		} else {

			if(ee.sensors=='J'){
				// get top 4 bits shifted to low byte and strip off low bit
				sensorcam = getcam(PINB>>4 & 0x0E);

				if(sensorcam!=oldsensorcam){
					debounce = 0;
					oldsensorcam = sensorcam;
				} else {
					// same pin config
					if(debounce>250){
						// same config stable for x tests, go ahead and process it
						cli();

						if(sensorcam=='0'){
							// no sensors closed, so revert back to saved camera setting
							if(savecam>'0'){
								setcam(savecam);
								savecam = '0';
							}
						} else { 

							// a sensor is closed
							// if we haven't already saved the original camera, do so now
							if(savecam=='0'){
								savecam = ee.cam;
							}

							if(ee.cam!=sensorcam){
								// a sensor is closed for a different camera than the selected sensor
								// switch to selected camera
								setcam(sensorcam);
							}
						}
						sei();
					} else {
						// count to 250 to debounce any noise
						debounce++;
					}
				}

			}
		}
	}
}



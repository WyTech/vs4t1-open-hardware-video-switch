// VS4T1 Video/Audio 4-to-1 switching cable controller
// CamSwitcher video switch firmware
// (C) 2013 Alan Capesius 
// The code released under Open Source Expat MIT License
// See license-mit-expat.txt for details

// Fuses:
//	Brown-out detection disabled BODLEVEL=1111
//	Int RC Osc 8Mhz 65ms  CKSEL=0100
//  Serial program downloading Enabled SPIEN=0
// Clock: 8Mhz
// Based on ATTiny2313 2048 bytes flash, 128 bytes RAM, 128 bytes EEPROM

// Change Log: 
// 2007/04/23 Fixed scan interval error. No units shipped with error. RevA designation still used, no menu changes.

#include <ctype.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
//#include <string.h>
#include <avr/wdt.h>

// this line makes EEP file used by avrdude to program eeprom
uint8_t EEMEM eepromdefaults[3]={"140"};

// eeprom storage variables
struct {
	unsigned char cam;
	unsigned char maxcam;
	unsigned char multmax;
} ee;

unsigned char *eeptr=0x0000; 			
unsigned char mult;

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
		PORTB = 0x00;					// pullups on, video off
	} else {
		PORTB = 0x00 + (1<<(cam-'1'));   // pullups on + video setting  36 bytes
	}
	
	ee.cam = cam;

}


SIGNAL(SIG_OVERFLOW1)
{
	if(mult < ee.multmax){
		mult++;
	} else {
		if(ee.cam>=ee.maxcam){
			ee.cam = '0';
		}
		setcam(++ee.cam);
		mult = '0';
	}
	
	TCNT1 = 26473;				// 5 sec
}


void showconfig()
{
	PGM_P config1 = PSTR("Configured for ");
	PGM_P config2 = PSTR(" cameras, ");
	PGM_P config3 = PSTR(" second interval.\n");

	TransmitString(config1);
	TransmitByte(ee.maxcam);
	TransmitString(config2);

	switch(ee.multmax){
		case '0':
		default:
			TransmitByte('5');
			break;
		case '1':
			TransmitByte('1');
			TransmitByte('0');
			break;
		case '2':
			TransmitByte('1');
			TransmitByte('5');
			break;
		case '3':
			TransmitByte('2');
			TransmitByte('0');
			break;
		case '4':
			TransmitByte('2');
			TransmitByte('5');
			break;
		case '5':
			TransmitByte('3');
			TransmitByte('0');
			break;
	}
			
	TransmitString(config3);
}

void applysettings()
{
	// baud rate, cam #, and scan mode must be applied to the hardware to control 
	// the ports and timer the other settings are used to control program flow
	InitUART(51); 	// 9600bps
	
	// set scan mode on
	TCNT1 = 26473;				// 5 sec
	TIMSK |= _BV(TOIE1);		// enable timer1 overflow
}

// load defaults from eeprom
void LoadDefaults()
{
	eeprom_read_block(&ee, eeptr, sizeof(ee));
	setcam('1');
}

// Main - a simple test program
int main( void )
{
	// clear any watchdog flags and disable watchdog resets
	// watchdog is used to reset via software and must be disabled here to prevent continuous resets
	// watchdog is not used for watchdog resets, only for software reset and is disabled by fuses at startup
	// this code captures and resets the watchdog timer after a software reset
	MCUSR = 0;
	wdt_disable();
	
	// factory defaults for ee structure
	PGM_P defaults = 	PSTR("140");

	// long strings stored in program space for messaging
	PGM_P settings = 	PSTR("Settings written\n");
	PGM_P banner = 	PSTR("\nCamSwitcher Rev B - Public Domain\n");
	PGM_P help = 	PSTR("2: Configure for 2 cameras\n3: Configure for 3 cameras\n4: Configure for 4 cameras*\nI: Change cycle interval (5-30 seconds)\nW: Write settings to memory\nF: Factory Settings\nR: Reset\n?: Help\n*=default\n");

	DDRB = 0xFF;    // set all ports as outputs
	PORTB = 0x00;	// all ports off (video off)
	DDRD = 0x00;	// serial I/O on port D

	TCCR1B = (1<<CS10) | (1<<CS12);		// prescale /1024, normal mode 8 sec

	LoadDefaults();
	applysettings();
	mult = '0';

	TransmitString(banner);
	showconfig();

	sei();

  	while (1) {

		if( (UCSRA & (1<<RXC)) ){ ; 	// Check for incoming data 
			cli();

			unsigned char inchar = toupper(UDR);		

			if(inchar=='F'){
				// factory defaults
				memcpy_P(&ee,defaults,sizeof(ee));
				applysettings();
				showconfig();
			}

			if(inchar=='W'){
				// save settings to eeprom 
				eeprom_write_block(&ee, eeptr, sizeof(ee));
				TransmitString(settings);
			}

			if(inchar=='R'){
				// reset device
				wdt_enable(WDTO_15MS);
   				for(;;);
			}


			if(inchar>='2' && inchar<='4'){
				// set max cam to 2,3 or 4
				ee.maxcam = inchar;
				showconfig();
			}

			if(inchar=='I'){
				// set scan interval 5-30 seconds
				ee.multmax++;
				if(ee.multmax>='6') ee.multmax='0';
				mult = '0';
				showconfig();
			}

			if(inchar=='?'){
				// show banner
				TransmitString(banner);
				TransmitString(help);
				showconfig();
			}

			sei();
		}
	}
}



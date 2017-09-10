// VS4T1 Firmware Released to Public Domain
// FCC EMI Testing program
// (C) 2013 Alan Capesius 
// The code released under Open Source Expat MIT License
// See license-mit-expat.txt for details

// exercises all output of the device for emission testing lab on continuous loop
// Video/Audio 4-to-1 switching cable controller
// Fuses:
//	Brown-out detection disabled BODLEVEL=1111
//	Int RC Osc 8Mhz 65ms  CKSEL=0100
//  Serial program downloading Enabled SPIEN=0
// Clock: 8Mhz

// Includes 
#include <avr\io.h>
#include <ctype.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
//#include <avr/eeprom.h>
#include <avr/wdt.h>

//int approvemode = 0;
//unsigned char lastchar ='\0';
unsigned char switchcam = 1;
unsigned char shadow = 0;
unsigned char maxcam = 4;
unsigned char T1start = 1;  // 8.4sec delay

// Prototypes 
void InitUART( unsigned int baudrate );

// This function basically wastes time
void delay_ms(long int ms) {
  unsigned char time;

  while (ms != 0) {
    // this number is dependant on the clock frequency
    for (time=0; time <= 100 ; time++);
    ms--;
  }
}

void InitUART( unsigned int baud )
{
	// Set baud rate 
	UBRRH = (unsigned char)(baud>>8);
	UBRRL = (unsigned char)baud;
	
	// Enable receiver and transmitter
	UCSRB = (1<<RXEN)|(1<<TXEN);
	
	// Set frame format: 8data, 2stop bits
	// UCSRC = (1<<USBS)|(3<<UCSZ0);
	
	// Set frame format: 8N1
	UCSRC = (3<<UCSZ0);
}

void TransmitByte( unsigned char data )
{
	while ( !(UCSRA & (1<<UDRE)) );    // Wait for empty transmit buffer 
	UDR = data; 			        // Start transmittion 
}

void TransmitString( PGM_P msg, unsigned char crlf)
{
	int x;
	unsigned char c;

	x = 0;

	while( (c = pgm_read_byte(msg+x)) ){
		TransmitByte(c);
		x++;
	}

	if(crlf){
		TransmitByte('\r');
		TransmitByte('\n');
	} else {
		TransmitByte(' ');
	}
}

// switch to camera 1-4 or zero=all off
void setcam(unsigned char cam){
	shadow &= 0xF0;

	if(cam==0){
		PORTB = shadow;
	} else {
		PORTB = shadow + _BV(cam-1);   // pullups on + video setting
	}
}

unsigned char getcam(unsigned char bits)
{
	unsigned char cam = 0;

	// convert raw bits to camera number
	if(bits & 0x08){
		cam = 4;
	} else {
		if(bits & 0x04){
			cam = 3;
		} else {
			if(bits & 0x02){
				cam = 2;
			} else {
				if(bits & 0x01){
					cam = 1;
				}
			}
		}
	} 

	return cam;
}

// Main - a simple test program
int main( void )
{
	// clear any watchdog flags and disable watchdog resets
	// watchdog is used to reset via software and must be disabled here to prevent continuous resets
	MCUSR = 0;
	wdt_disable();

	// long strings stored in program space
	PGM_P banner = 	PSTR("\r\nVS4T1 TestPgm Firmware Public Domain"); 
	PGM_P helpprompt = 	PSTR("Type ? for help");
	PGM_P help = 		PSTR("Video Line: 0=None* 1,2,3,4\r\nOutput Line: N=None, A,B,C\r\n\r\n");

	unsigned char echo = 1;

	DDRB = 0xFF;       // set portb 0-3 to outputs, 4-7 as inputs, (4 line not used)
	PORTB = 0x00;	// set top 4 bits high to enable pullups, low 4 bits low to turn off video
	MCUCR = 0x00;	// pullups on 
	DDRD = 0x00;	// serial I/O on port D

	// set initial port states - output: 0=low 1=high 
	// for video switch thru NPN switch, 0=video on 1= video off

	InitUART( 51 ); // Set the baudrate to 9600 bps using a 8MHz clock 

	TransmitString(banner,1);
	TransmitString(helpprompt,1);

  	while (1) {

		if( (UCSRA & (1<<RXC)) ){ ; 	// Check for incoming data 

			unsigned char inchar = toupper(UDR);		

			if(echo){					// echo if set to
				TransmitByte(inchar);
			}

			unsigned char cam = 0;
				
			switch(inchar){		 
				case '0':
				case '1':
				case '2':
				case '3':
				case '4':
					cam = inchar - '0';
					setcam(cam);
					break;
				case 'A':
					shadow &= 0x0F;
					shadow ^= _BV(7);
					PORTB = shadow;
					break;
				case 'B':
					shadow &= 0x0F;
					shadow ^= _BV(6);
					PORTB = shadow;
					break;
				case 'C':
					shadow &= 0x0F;
					shadow ^= _BV(5);
					PORTB = shadow;
					break;
				case 'N':
					shadow &= 0x0F;
					PORTB = shadow;
					break;
				case 'Z':
					PORTB = 0x00;
					shadow = 0;
					// query current camera setting
					TransmitString(banner, 1);
					break;
				case '?':
					TransmitString(banner, 1);
					TransmitString(help,1);
					break;
				default:
					break;

			}
		}
	}
}



// VS4T1 Video/Audio 4-to-1 switching cable controller
// Simple video switching program

// Fuses:
//	Brown-out detection disabled BODLEVEL=1111
//	Int RC Osc 8Mhz 65ms  CKSEL=0100
//  Serial program downloading Enabled SPIEN=0
// Clock: 8Mhz

// compiles to 926 bytes

// Includes 
#include <avr\io.h>
#include <ctype.h>
#include <avr/pgmspace.h>

// Prototypes 
void InitUART( unsigned int baudrate );

// This function basically wastes time
void delay_ms(long int ms) {
  unsigned long int timer;

  while (ms != 0) {
    // this number is dependant on the clock frequency
    for (timer=0; timer <= 100 ; timer++);
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
	
	// Set frame format: 8data, 2stop bits (default)
	// UCSRC = (1<<USBS)|(3<<UCSZ0);
	
	// Set frame format: 8N1
	UCSRC = (3<<UCSZ0);
}

unsigned char ReceiveByte( void )
{
	// Wait for data to be received
	while ( !(UCSRA & (1<<RXC)) ) ;

	// Get and return received data from buffer
	return toupper(UDR);
}


void TransmitByte( unsigned char data )
{
	while ( !(UCSRA & (1<<UDRE)) );    // Wait for empty transmit buffer 
	UDR = data; 			                 // Start transmission 
}

void TransmitCRLF()
{
	TransmitByte('\r');
	TransmitByte('\n');
}

void TransmitString( PGM_P msg, int crlf)
{
	int x;
	int c;

	x = 0;

	while( (c = pgm_read_byte(msg+x)) ){
		TransmitByte(c);
		x++;
	}

	if(crlf){
		TransmitCRLF();
	} else {
		TransmitByte(' ');
	}
}

int main( void )
{
  // store strings in program memory
	PGM_P copyright = 	PSTR("(c) 2012 Open Source MIT License");
	PGM_P company = 	PSTR("Tech World, Inc");
	PGM_P product = 	PSTR("VS4T1 Online");
	PGM_P helpprompt = 	PSTR("Press 1,2,3,4 to switch cameras or ? for help");
	PGM_P help = 		PSTR("\r\n\r\n1-4 - Select input 1-4\r\n");

	// set initial port states - output: 0=low 1=high 
	// for video switch thru NPN switch, 0=video on 1=video off
  // only one should be on at a time for video switching
  // we start with everything off
	DDRB = 0xFF;  // set all 8 pins on port B to outputs
	PORTB = 0x00;	// turn off all Sources

	DDRD = 0x00;	  // serial I/O on port D
	InitUART( 51 ); // Set the baudrate to 9600 bps using a 8MHz clock 

	TransmitCRLF();
	TransmitString(company,0);
	TransmitString(product,0);
	TransmitString(copyright,1);
	TransmitString(helpprompt,1);

  // loop forever
  while (1) {

    // Check for incoming data 
    if( (UCSRA & (1<<RXC)) ){ ; 	
      unsigned char inchar = ReceiveByte();
    
      // echo the type character
      TransmitByte(inchar);
    
      // this code selects the input source
      switch(inchar){		 
        case '0':
          PORTB = 0x00;	// turn off all Sources
          break;
        case '1':
          PORTB = 0x01;	// turn on Source 1
          break;
        case '2':
          PORTB = 0x02;	// turn on Source 2
          break;
        case '3':
          PORTB = 0x04;	// turn on Source 3
          break;
        case '4':
          PORTB = 0x08;	// turn on Source 4
          break;
        case '?':
          TransmitString(help,1);
          break;
        default:
          break;

      }
    }
  }
}

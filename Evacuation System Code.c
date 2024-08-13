/*-----------------------------------------------------------------------------
File Name:	Evacuation System Code  
Author:	Vaibhav Sinha
Date:		11/06/2024
Modified:	None
© Fanshawe College, 2024
 
Description:	This program controls an evacuation circuit using the PIC18F45K22 
microcontroller. It manages door locking and unlocking based on 
intruder detection, remote maintenance (Remox) mode, fire alarms, 
and password breaches. The system initializes the necessary 
configurations, continuously monitors sensor inputs, and adjusts 
motor controls for door operations. The program also includes an 
interrupt service routine (ISR) for handling serial communication, 
triggering specific actions based on received commands.
 
-----------------------------------------------------------------------------*/
// Use of AI / Cognitive Assistance software is not allowed in any exercise.
 
/* Preprocessor ---------------------------------------------------------------
   Hardware Configuration Bits ----------------------------------------------*/
#pragma config FOSC	= INTIO67
#pragma config PLLCFG	= OFF
#pragma config PRICLKEN = ON
#pragma config FCMEN	= OFF
#pragma config IESO	= OFF
#pragma config PWRTEN	= OFF 
#pragma config BOREN	= ON
#pragma config BORV	= 285 
#pragma config WDTEN	= OFF
#pragma config PBADEN	= OFF
#pragma config LVP		= OFF
#pragma config MCLRE	= EXTMCLR
 
// Libraries ------------------------------------------------------------------
#include <p18f45k22.h>
#include <stdio.h>
#include <stdlib.h>
#include <delays.h>
#include <string.h>
 
// Constants  -----------------------------------------------------------------
#define TRUE		1	
#define FALSE		0
#define BYTESIZE 		8
#define RC2FLAG 		PIR3bits.RC2IF
#define SHIFT2 		2
#define INPUT 		1
#define OUTPUT 		0
#define BYTESIZE		8
 
 
#define INTRUDER PORTBbits.RB0
#define LMTUP PORTDbits.RD2
#define LMTDWN PORTDbits.RD3
 
#define M1FWD LATCbits.LATC0
#define M1REV LATCbits.LATC3
#define M2FWD LATCbits.LATC1
#define M2REV LATCbits.LATC2
 
#define REMOX PORTDbits.RD5
//
#define LEDFWD LATBbits.LATB2
#define LEDREV LATBbits.LATB3
 
#define TALARM PORTCbits.RC7
#define PASSBREACH PORTDbits.RD4
#define DLOCK LATDbits.LATD0
 
#define BUFSIZE 30
#define RC1FLAG PIR1bits.RC1IF
#define INTGON 0xC0
#define TOKENSIZE 35
 
// Global Variables  ----------------------------------------------------------
char serviceMode = FALSE;
char buf1[] = {"$ALM\r"};
 
typedef char flag_t;
flag_t sentenceRdy = FALSE;
char receivingbuf[BUFSIZE];
char *tokens[TOKENSIZE];
char insert = 0;
char hold = 0;
// Prototypes

void ISR();
 
// Interrupt Vector 
 
#pragma code interrupt_vector = 0x08
 
void interrupt_vector(void)
{
_asm
GOTO ISR
_endasm
}
 
#pragma code 
 
// Functions  -----------------------------------------------------------------
 
/*>>> configOSC4MHz: ----------------------------------------------------------- 
Author:	Vaibhav Sinha
Date:		11/06/2024
Modified:	None
Desc:	    	Sets the oscillation frequency of microcontroller PIC18F45K22 to
4 MHz and waits for the system to be stable.
Input: 		None
Returns:	None
----------------------------------------------------------------------------*/
void configOSC4MHz(void)
{
OSCCON = 0X52;
while(!OSCCONbits.HFIOFS); // waits for frequency to be stable
 
} // eo configOSC4MHz::
 
/*>>> configPort: ---------------------------------------------------------------------
Author:	Vaibhav Sinha
Date:		11/06/2024
Modified:	None
Desc:	    	This function will Configure the I/O ports of microcontroller 
PIC18F45K22 as per the operation of our system.
Input: 		None
Returns:	None
----------------------------------------------------------------------------*/
void configPort(void)
{
ANSELA 	= 0x00; //Sets the PORTA for digital operation
LATA	    	= 0x00; //Sets the PORTA for no output
TRISA		= 0xFF; //Sets the PORTA for input operation
ANSELB	= 0x00; //Sets the first 2 bits of PORTB for analog operation
LATB		= 0x00; //Sets the PORTB for no output
TRISB		= 0xF3; //Sets the 3rd and 4th bits of PORTB for output operation
ANSELC 	= 0X00; //Sets all the pins of PORTC for digital operation
LATC 		= 0X00; //Sets all the pins of PORTC for no outpout
TRISC	= 0XFF; //Sets all the pins of PORTC as inputs
ANSELD 	= 0X00; //Sets all the pins of PORTD for digital operation
LATD 		= 0X00; //Sets all the pins of PORTD for no outpout
TRISD	= 0XFE; //Sets all the pins of PORTD as inputs
} // eo configPort::
 
/*>>>configUSART2::===============================================
Author:	Vaibhav Sinha
Date:		11/06/2024
Modified:	None
Desc:		This function sets the USART2 for  19.2k baud rate, SP1 on, TX & RX 
enabled, 8 bit, 1 stop bit, non-inverted.
Input: 		None
Returns:	None
=================================================================*/
void configUSART2(void)
{
BAUDCON1 		= 0X40;
TXSTA1		= 0X26;
RCSTA1 		= 0X90;
SPBRG1 		= 12;
SPBRGH		= 0;
 
}// eo configUSART2::
 
/*>>> configINTS: -----------------------------------------------------------
Author:	Vaibhav Sinha
Date:		11/06/2024
Modified:	None
Desc:		Initializes interrupts for Timer0 and Receiver #2 to enable their 
operation.
Input: 		None
Returns:	None
----------------------------------------------------------------------------*/
void configINTS(void)
{
// Configure Receiver #1 interrupt
IPR3bits.RC2IP 	= FALSE;    // Receiver #1 interrupt priority set to low
PIR3bits.RC2IF 	= FALSE;    // Clear Receiver #1 interrupt flag
PIE3bits.RC2IE 	= TRUE;     // Enable Receiver #1 interrupt

RCONbits.IPEN 	= FALSE;     // Global interrupt priority disabled
INTCON 		|= INTGON; // Enable interrupts globally
} // eo configINTS ::
 
/*>>> systemInitialization: ------------------------------------------------- 
Author:	Vaibhav Sinha
Date:		10/06/2024
Modified:	None
Desc:	    	This function will initialize the system when called at the top of
main function. All the configuration functions written above, have 
been called in this function. 
Input: 		None
Returns:	None
----------------------------------------------------------------------------*/
void systemInitialization(void)
{
configOSC4MHz();
configPort();
configUSART2();
configINTS();

} // eo systemInitialization::
 
#pragma interrupt ISR //pragma for interrupt service routine
 
/*>>> ISR: ===========================================================
Author:	Vaibhav Sinha
Date:		11/06/2024
Modified:	None
Desc:		This function will interupt the main function on the event of 
recieving data from the evacuation system and will store it into
an array for further processing.		
Input: 		None
Returns:	None
============================================================================*/
 
void ISR(void)
{
char i=0;
if(RC2FLAG == TRUE)
{
hold = RCREG2;
if(hold == '$')
{
for(i=0;i<30; i++)
{
receivingbuf[i]=0;
}
insert = 0;	
}
if(hold == '\r')
{
sentenceRdy = TRUE;
}
receivingbuf[insert] = hold;
insert++;
}
INTCON |= 0xC0;
} // eo ISR ::
 
/*>>> receivedSen: ===========================================================
Author:	Vaibhav Sinha
Date:		11/06/2024
Modified:	None
Desc:		This function will use the serial communication to receive a string.
Input: 		None
Returns:	None
============================================================================*/
void receivedSen(void)
{
puts2USART(receivingbuf);
}
 
 
/*--- MAIN: FUNCTION ----------------------------------------------------------
----------------------------------------------------------------------------*/
void main( void )
{
char downFlag = FALSE; //flag for downward movement of the tray
char upFlag = FALSE; //flag for upward movement of the tray
int fireAlarm = TRUE; //variable that stores the results of fire detection data validation
systemInitialization(); //configures the system as per operation requirements 
 
while(1)
{
fireAlarm = strcmp(receivingbuf, buf1); //compairing the recieved string from fire detection system 
if(sentenceRdy)
{
sentenceRdy = FALSE;
receivedSen(); //calling the function
}
 
if(INTRUDER||serviceMode)
{
DLOCK = TRUE;
}

while((!INTRUDER && !REMOX) || !fireAlarm || TALARM || PASSBREACH || downFlag)
{
downFlag = TRUE;
DLOCK = FALSE; //locking the door(Normally in unlocked state) 
if (!LMTUP && LMTDWN)//artifact is exposed
{
LEDFWD = TRUE;
M1FWD = TRUE;
M2REV = TRUE;
}
if (!LMTDWN && LMTUP)//artifact is secured
{
LEDFWD = FALSE;
M1FWD = FALSE;
M2REV = FALSE;
downFlag = FALSE;			
}
} // eo while intruder

` 		while(REMOX||upFlag)
{
DLOCK = TRUE; //unlocking the door
upFlag=TRUE;
if (!LMTDWN) //artifact is secured
{
LEDREV = TRUE;
M1REV = TRUE;
M2FWD = TRUE;
}
if (!LMTUP)//artifact is exposed
{
LEDREV = FALSE;
M1REV = FALSE;
M2FWD = FALSE;
upFlag = FALSE;
}
}// eo while remote access
 
}//eo indefinite loop
 
} // eo main::

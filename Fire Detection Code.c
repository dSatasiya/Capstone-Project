/*=============================================================================
File Name:	Fire Detection Code
Author:	Vaibhav Sinha
Date:		11/06/2024
Modified:	Shubham on 19/07/2024
© Fanshawe College, 2024
 
Description:	This program is designed to detect fire and smoke using sensors 
connected to a PIC18F45K22 microcontroller. It monitors the 
output from smoke and flame sensors, processes the data, and 
activates an alarm and motor (likely to open a door or activate 
a ventilation system) if smoke or fire is detected. The program 
also transmits an alert message when an alarm condition is met. 
The system operates on a 4MHz internal oscillator and uses 
various hardware configurations to manage inputs and outputs. 
The configuration settings, including ADC, USART, and timers, 
are set up to facilitate accurate sensor readings and timely 
responses to hazardous conditions. The code runs continuously, 
checking sensor data at regular intervals and triggering safety 
mechanisms when thresholds are exceeded.
=============================================================================*/
 
/* Preprocessor ===============================================================
   Hardware Configuration Bits ==============================================*/
#pragma config FOSC		= INTIO67
#pragma config PLLCFG	= OFF
#pragma config PRICLKEN = ON
#pragma config FCMEN	= OFF
#pragma config IESO		= OFF
#pragma config PWRTEN	= OFF 
#pragma config BOREN	= ON
#pragma config BORV		= 285 
#pragma config WDTEN	= OFF
#pragma config PBADEN	= OFF
#pragma config LVP		= OFF
#pragma config MCLRE	= EXTMCLR
 
// Libraries ==================================================================
#include <p18f45k22.h>
#include <stdio.h>
#include <stdlib.h>
#include <usart.h>
 
// Constants  =================================================================
#define TRUE			1	
#define FALSE			0
#define HUNDREDMILSEC 	0x3CB0
#define T0FLAG 			INTCONbits.TMR0IF
#define BYTESIZE 		8
#define SAMPLESIZE 		5
#define SENCOUNT 		2
#define ONESEC 			10	
#define ADCRES 			0.0048828125
#define SMOKE 			0
#define FLAME 			1
#define SMOKEM 			300
#define SMOKEB 			1940
#define FLAMEM			6
#define SMOKEL 			2000 // ppm
#define FLAMEL 			2  //meters
#define ALARM			LATCbits.LATC3
#define MOTOR			LATCbits.LATC2
#define SMOKE_FLAG		LATDbits.LATD6
#define FLAME_FLAG      LATDbits.LATD7
#define BUFSIZE			20
 
// Global Variables  ==========================================================
 
typedef int sensor_t; //data type for raw data from sensors
 
typedef struct 
{
sensor_t sample[SAMPLESIZE];
sensor_t avg;
sensor_t duckLimit;
char insert;
char avgReady;
char smokeFlag;
char flameFlag;
} sensorCh_t; 
 
sensorCh_t sensors[SENCOUNT];
 
char receivingBuf[BUFSIZE] = {0};
 
// Functions  =================================================================
 
/*>>> SetOSC4MHz: ===========================================================
Author:	Vaibhav Sinha
Date:		11/06/2024
Modified:	None
Desc:		Sets the oscillation frequency of microcontroller PIC18F45K22 to
4 MHz and waits for the system to be stable.  
Input: 		None
Returns:	None
============================================================================*/
void setOSC4MHz(void)
{
OSCCON = 0X52;
while(!OSCCONbits.HFIOFS); //waiting for the system to become stable
} // eo setOSC4MHz::
 
/*>>> configPorts: ===========================================================
Author:	Vaibhav Sinha
Date:		11/06/2024
Modified:	None
Desc:		This function will Configure the I/O ports of microcontroller 
PIC18F45K22 as per the operation of our system. 
Input: 		None or dataType variableName, why this input is needed / what is 
its use.  LIST EACH INPUT ARGUMENT ON ITS OWN LINE.
Returns:	None or datatype, what is the data / what is its meaning
============================================================================*/
void configPorts(void)
{
ANSELA = 0X0E;
LATA 	= 0X00;
TRISA	= 0XFF;
ANSELB = 0X00;
LATB 	= 0X00;
TRISB	= 0X00;
ANSELC = 0X00;
LATC 	= 0X00;
TRISC	= 0XE0;
ANSELD = 0x00;
LATD = 0x00;
TRISD = 0x00;
 
}// eo configPorts::
 
/*>>> configADC: ===========================================================
Author:	Vaibhav Sinha
Date:		11/06/2024
Modified:	None
Desc:		This function sets the ADC Module of microcontreoller PIC18F45K22
for 12 Tad, Fosc/8, Channel 0, standard voltage references, ADC 
module on.
Input: 		None
Returns:	None
============================================================================*/
void configADC(void)
{
ADCON0 	= 0X01;
ADCON1 	= 0X00;
ADCON2	= 0XA9;
 
}// eo configADC::
 
/*>>> configUSART: ===========================================================
Author:	Vaibhav Sinha
Date:		11/06/2024
Modified:	None
Desc:		This function sets the USART1 for  9600 baud rate, SP1 on, TX & RX 
enabled, 8 bit, 1 stop bit, non-inverted.
Input: 		None
Returns:	None
============================================================================*/
void configUSART(void)
{
BAUDCON1 	= 0X40;
TXSTA1	= 0X26;
RCSTA1 	= 0X90;
SPBRG1 	= 12;
SPBRGH	= 0;
 
}// eo configUSART::
 
/*>>> resetTMR0: ===========================================================
Author:	Vaibhav Sinha
Date:		11/06/2024
Modified:	None
Desc:		This function resets the timer0 after the rollover time is passed.
Input: 		int psc, the prescale value according to the desired rollover time.
Returns:	None
============================================================================*/
void resetTMR0(int psc)
{
T0FLAG 	= FALSE;
TMR0H 	= psc >> BYTESIZE;
TMR0L 	= psc;
 
}// eo resetTMR0::
 
/*>>> configTMR0: ===========================================================
Author:	Vaibhav Sinha
Date:		11/06/2024
Modified:	None
Desc:		This function sets the Timer0 module of the microcontroller 
PIC18F45K22 for100ms rollover, Timer0 on, prescaler as needed.
Input: 		int psc, the prescale value according to the desired rollover time.
Returns:	None
============================================================================*/
void configTMR0(int psc)
{
resetTMR0(psc);
T0CON = 0X90;
 
}// eo configTMR0::
 
/*>>> initSensorCh: ===========================================================
Author:	Vaibhav Sinha
Date:		11/06/2024
Modified:	Shubham on 19/07/2024
Desc:		This function will initialize all the elements of the data structure
sensorCh_t
Input: 		sensorCh_t *sen, pointer that will be used to address all elements 
of the data structure sensorCh_t.
Returns:	None
============================================================================*/
void initSensorCh(sensorCh_t *sen)
{
int index = 0;
for (index = 0; index < SAMPLESIZE; index++)
{
sen -> sample[index] = FALSE;
}
sen -> avg = FALSE;
sensors[SMOKE].duckLimit = SMOKEL;
sensors[FLAME].duckLimit = FLAMEL;
sen -> insert = FALSE;
sen -> avgReady = FALSE;
sen-> smokeFlag = FALSE;
sen-> flameFlag = FALSE;
}// eo initSensorCh::
 
/*>>> getADCSamle: ===========================================================
Author:Vaibhav Sinha
Date:		11/06/2024
Modified:	None
Desc:		This function takes the raw data from the input ports, converts 
them binary readable values and stores them into a registor. 
Input: 		char chID, this input specifies the port from which samples are to
be taken.
Returns:	None
============================================================================*/
sensor_t getADCSample(char chID)
{
ADCON0bits.CHS = chID;
ADCON0bits.GO = TRUE;
while(ADCON0bits.GO);
return ADRES;
}// eo getADCSample::
 
/*>>> transmitSen: ===========================================================
Author:	Vaibhav Sinha
Date:		11/06/2024
Modified:	None
Desc:		This function will use the serial communication to transmit a string.
Input: 		None
Returns:	None
============================================================================*/
void transmitSen()
{
char receivingBuf[] = {"$ALM\r"};
puts1USART(receivingBuf);	
}// eo getADCSample::
 
/*>>> systemInit: ===========================================================
Author:	Vaibhav Sinha
Date:		12/05/2024
Modified:	None
Desc:		This function will initialize the system when called at the top of
main function. All the configuration functions written above, have 
been called in this function. 
Input: 		None
Returns:	None
============================================================================*/
void systemInit(void)
{
setOSC4MHz(); //setting the oscillation frequency
configPorts(); //configuring the I/O ports	
configADC(); //setting the ADC Module
configUSART(); //setting the serial communication
configTMR0(HUNDREDMILSEC); //setting the Timer Module for 100ms
}// eo systemInit::
 
 
/*=== MAIN: FUNCTION ==========================================================
============================================================================*/
 
void main( void )
{
char secCount = 0;
char count = 0;
long sum = 0;
char index = 0;
float volts = 0;
 
systemInit(); //initializing the system
 
for (count = 0; count < SENCOUNT; count++)
{
initSensorCh(&sensors[count]);
}
 
while(1)
{
if (T0FLAG == TRUE)//100mS Rollover
{
resetTMR0(HUNDREDMILSEC);//reseting after rollover
secCount++;
if (secCount >= ONESEC)//timer rolled over 10 times (1 second)
{
char chID = 0;
secCount = 0;
//collecting samples 2 different channels
//and storing them into sample arrays.
for (chID = 0; chID < SENCOUNT; chID++)
{
sensors[chID].sample[sensors[chID].insert] = getADCSample(chID);
sensors[chID].insert++;
if (sensors[chID].insert > SAMPLESIZE)
{
sensors[chID].insert = FALSE;
sensors[chID].avgReady = TRUE;
} 
//collecting 10 samples for each channel and taking their average
if(sensors[chID].avgReady)
{
long sum = 0;
for(index = 0; index < SAMPLESIZE; index++)
{
            	sum += sensors[chID].sample[index];
}
sensors[chID].avg = sum / SAMPLESIZE;
volts = sensors[chID].avg * ADCRES;
//calculating the physical value using the linear eqations of each sensor
switch (chID)
{
case SMOKE:
//smoke sensor linear equation C = 1940*V+300( for range 300-10000 ppm)
volts = volts*SMOKEB; 
sensors[chID].avg = (float)volts+SMOKEM;
break;

case FLAME:
//D=6*V(for range 0-30 meter)
sensors[chID].avg = (float)volts*FLAMEM; 
break;

default:
break;
}
//taking action based on differnt scenarios
if (sensors[SMOKE].avg > sensors[SMOKE].duckLimit)
{
ALARM = TRUE;
MOTOR = TRUE;
SMOKE_FLAG = TRUE;
transmitSen();		
}
else
{
SMOKE_FLAG = FALSE;
}
if (sensors[FLAME].avg > sensors[FLAME].duckLimit)
{
ALARM = TRUE;
MOTOR = TRUE;
FLAME_FLAG = TRUE;
transmitSen();	
}
else
{
FLAME_FLAG = FALSE;
}
if(sensors[SMOKE].avg < sensors[SMOKE].duckLimit  && sensors[FLAME].avg < sensors[FLAME].duckLimit)
{
ALARM = FALSE;
MOTOR = FALSE;
SMOKE_FLAG = FALSE;	
FLAME_FLAG = FALSE;		
}
}//if average ready
}//for channel switching			
}//eo if seccount reaches 10								
}//eo if T0FLAG
}//eo while loop::
} // eo main::

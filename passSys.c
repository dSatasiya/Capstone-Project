/*-----------------------------------------------------------------------------
	File Name: PASS TEST code
	Author:	   Shubham and Druv Satasiya
	Date:	   06/07/2024 
	Modified:  Dhruv Satasiya on 25/07/2024,Shubham on 30/07/2024
	© Fanshawe College, 2023

	Description: This program integrates the codes for motion sensor, LCD, Temperature sensor,,
		     Keypad, and test the alert system, message display on LCd, Temperature Display, and 
		     Password Validation.
	
-----------------------------------------------------------------------------*/

/* Preprocessor ---------------------------------------------------------------
   Hardware Configuration Bits ----------------------------------------------*/
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

// Libraries ------------------------------------------------------------------
#include <p18f45k22.h>
#include <stdio.h>
#include <stdlib.h>
#include <delays.h>
#include "xlcd.h"
#include <string.h>


// Constants  -----------------------------------------------------------------
#define TRUE 1
#define FALSE 0
#define INPUT 1
#define OUTPUT 0
#define PBMASK 0x01
#define DELAYCOUNT 20			// delay to introduce a debounce for the pushbuttons.
#define DELAYCOUNT_1 60   		// for 15ms of delay
#define DELAYCOUNT_2 20   		// for 5ms of delay
#define DEBOUNCE_DELAY 20
#define T0FLAG INTCONbits.TMR0IF	// Timer Flag
#define BYTESIZE 8			
#define PSC_VALUE 0x0BDC//MS		// pre scaler value
#define PWMPERIOD 0xF9			// PWM period
#define SHIFT2 2			
#define RELAY LATAbits.LATA0		// Relay pin 
#define LOCK LATAbits.LATA4		// Lock pin
#define DCVAL 100 			//dc value for 10% duty cycle
//#define LED LATEbits.LATE1
#define SYSON LATCbits.LATC0	
#define SAMPSIZE 10			// Total number of samples
#define ONESEC 10
#define ADCRES 0.00488281 		// ADC resolutions
#define TEMPM 0.01

// LCD Display Orientation Commands Constants ::::::::::::::::::::::::::::::::::::::
#define LINE1_LCD		0x00	// Start of line 1
#define LINE2_LCD		0x40	// Start of line 2

// KEYBOARD MATRIX  Constants ::::::::::::::::::::::::::::::::::::::::::::::::::::::
#define SAMPSIZE 10			// Total number of samples
#define ONESEC 10
#define ADCRES 0.00488281 		// ADC resolutions
#define TEMPM 0.01
#define TEMP_INDICATION LATCbits.LATC6	//TEMP ALM
#define Y1 LATBbits.LATB4		/* Y1, Y2, Y3, Y4 and X1, X2, X3, X4 Keypad Pins*/
#define Y2 LATBbits.LATB5
#define Y3 LATCbits.LATC4
#define Y4 LATCbits.LATC5				
#define X1 PORTBbits.RB0
#define X2 PORTBbits.RB1
#define X3 PORTBbits.RB2
#define X4 PORTBbits.RB3
#define SIZE 10
#define TOTAL_TRIALS 3			// predefined trials for security system
#define MASTER_PIN PORTCbits.RC3
#define MASTER LATAbits.LATA1		// master lock pin
#define SECONDARY LATAbits.LATA0	// secondary lock pin

// Calcuation Constants ::::::::::::::::::::::::::::::::::::::::::::::::::::::
#define MOTIONSEN PORTAbits.RA5
#define	ECHO PORTCbits.RC1

// Global Variables  ----------------------------------------------------------
char keyValue = FALSE;			
char password[] ="456B#";		// default(correct) password
char passFlag = FALSE;			// password flag
int samplesArr[SAMPSIZE];		// array to store samples
char trialCount = TOTAL_TRIALS;		// trial counter
char trials[16] = {0};			// trial's array
char trialsIndex = 0;			// trial index
typedef int sensor_t;

// Function Prototpyes ::::::::::::::::::::::::::::::::::::::::::::::::::::::::

/*Structure for sensor's samples*/
typedef struct
{
	sensor_t samples[SAMPSIZE];
	sensor_t avgtime;
	sensor_t full;
	sensor_t empty;
	char insert;
	char avgRdy;
}sensorCh_t;

sensorCh_t sensors;			// sensor's object

/*>>> setOsc: ===========================================================
Author:		Shubham
Date:		06/07/2024
Modified:	None 
Desc:		It Configures the PIC For 16 MHZ
Input: 		None
Returns:	None
============================================================================*/
void setOsc()
{
	OSCCON = 0x72; 	     
	while(!OSCCONbits.HFIOFS);
} // eo setOsc::

/*>>> set4Osc: ===========================================================
Author:		Shubham
Date:		06/07/2024
Modified:	None 
Desc:		It Configures the PIC For 4 MHZ
Input: 		None
Returns:	None
============================================================================*/
void set4Osc()
{
	OSCCON = 0x52; 	     		// 0111 0010 and used as 4MHz osc. 
	while(!OSCCONbits.HFIOFS);
} // eo set4Osc::

/*>>> DelayFor18TCY: ===========================================================
Author:		Dhruv Satasiya
Date:		06/07/2024
Modified:	None 
Desc:		It Configures for 18TCY delay
Input: 		None
Returns:	None
============================================================================*/

void DelayFor18TCY(void)
{
	Nop();  			// It creates a single cycle delay / NOP. 
	Nop();
	Nop();
	Nop();
	Nop();
	Nop();
	Nop();
	Nop();
	Nop();
	Nop();
	Nop();
	Nop();
	Nop();
	Nop();
	Nop();
	Nop();
	Nop();
	Nop();
}//DelayFor18TCY::

/*>>> DelayPORXLCD: ===========================================================
Author:		Dhruv Satasiya
Date:		06/07/2024
Modified:	None 
Desc:		It Configures for delay of 15ms by 16MHz Oscillator
Input: 		None
Returns:	None
============================================================================*/

void DelayPORXLCD (void)
{
 	Delay1KTCYx(DELAYCOUNT_1); 	// Delay of 15ms by 16MHz Oscillator's frequecny. 
	return;
}//DelayPORXLCD::

/*>>> DelayXLCD: ===========================================================
Author:		Dhruv Satasiya
Date:		06/07/2024
Modified:	None 
Desc:		It Configures for delay of 5ms by 16MHz Oscillator
Input: 		None
Returns:	None
============================================================================*/

void DelayXLCD (void)
{
 	Delay1KTCYx(DELAYCOUNT_2); 	// Delay of 5ms by 16MHz Oscillator Frequecny. 
 	return;
}//DelayXLCD::

/*>>> resetTMR0: ===========================================================
Author:		Dhruv Satasiya
Date:		06/07/2024
Modified:	NoNe 
Desc:		It Configures the Timer registors for the FAlse start and reset Timer interrupt flag
Input: 		None
Returns:	None
============================================================================*/
void resetTMR0(int psc)
{
	T0FLAG = FALSE;			// resetting the timer flag
	TMR0H = psc<<BYTESIZE;
	TMR0L = psc;
}//resetTMR0::

/*>>> configTMR0: ===========================================================
Author:		Dhruv Satasiya
Date:		06/07/2024
Modified:	None 
Desc:		It Configures the Timer for 1 Sec at 16 MHz freq and calls reset timer function.
Input: 		None
Returns:	None
============================================================================*/
void configTMR0(int psc)
{
	resetTMR0(psc);
	T0CON = 0x91;   		// prescaler assigned as 1:2 as Fosc is 16MHz//Timer ON.
}//configTMR0::

/*>>> userMode: ===========================================================
Author:		Shubham
Date:		06/07/2024
Modified:	None 
Desc:		This function displays how to enter password mode on LCD
Input:		char *ptr1 to message for first line char *ptr2 to message for second line of display
Returns:	None
============================================================================*/
void userMode(char *ptr1,char *ptr2)
{
	SetDDRamAddr(0x80);		// This function is used to keep or set the 
					// cursor at particular location.
	Delay10KTCYx(40);
	while(*ptr1 != '\0')
	{
		WriteDataXLCD(*ptr1);	// in-built XLCD function to write the data on LCD
		ptr1++;
		Delay10KTCYx(50);
	}
	Delay10KTCYx(20);
	SetDDRamAddr(0x40);		// This function is used to keep or set the 									// cursor at particular location.
	Delay10KTCYx(50);
	while(*ptr2 != '\0')
	{
		WriteDataXLCD(*ptr2);
		ptr2++;
		Delay10KTCYx(50);
	}
	while(BusyXLCD());
	WriteCmdXLCD(0x01);             // Clear display in-built function
	Delay10KTCYx(20);
}//userMode::

/*>>> introMessage: ===========================================================
Author:		Shubham
Date:		24/07/2024
Modified:	None 
Desc:		This function displays the Introduction message for the user on LCD.
Input:		char *ptr1 to the Intro message in first line and char *ptr2 to message in second line.
Returns:	None
============================================================================*/
void introMessage(char *ptr1,char *ptr2)
{
	SetDDRamAddr(0x81);		// This function is used to keep or set the 
					// cursor at particular location.
	Delay10KTCYx(40);
	while(*ptr1 != '\0')
	{
		WriteDataXLCD(*ptr1);	// in-built XLCD function to write the data on LCD
		ptr1++;
		Delay10KTCYx(50);
	}
	Delay10KTCYx(20);
	SetDDRamAddr(0x42);		// This function is used to keep or set the 									// cursor at particular location.
	Delay10KTCYx(50);
	while(*ptr2 != '\0')
	{
		WriteDataXLCD(*ptr2);
		ptr2++;
		Delay10KTCYx(50);
	}
	while(BusyXLCD());
	WriteCmdXLCD(0x01);             // Clear display
	Delay10KTCYx(20);
}//introMessage::

/*>>> fillPass: ===========================================================
Author:		Shubham
Date:		06/07/2024
Modified:	None 
Desc:		This function displays the Password Entering process on the LCD Screen in first line
Input: 		char *ptr to the array having fill password message for the User.
Returns:	None
============================================================================*/
void fillPass(char *ptr)
{
	SetDDRamAddr(0x45);		// This function is used to keep Cursor at 2nd LINE at 5 position								// cursor at particular location.	
	Delay1KTCYx(TRUE);
	while(*ptr != '\0')
	{
		WriteDataXLCD(*ptr);
		ptr++;
		Delay10KTCYx(20);
	}
	while(BusyXLCD());	
}//fillPass::

/*>>> masterLock: ===========================================================
Author:		Dhruv Satasiya
Date:		25/07/2024
Modified:	Shubham on 30/07/2024 
Desc:		This function displays the Master lock message on Display. Also it wait for the 
		remote access to reopen the door.
Input: 		None
Returns:	None
============================================================================*/
void masterLock()
{
	char masterLock[] = {"Master Locked"};
	char masterCount = 0;

	while(BusyXLCD());
	WriteCmdXLCD(0x01);             // Clear display
	Delay10KTCYx(20);

	SetDDRamAddr(0x81);	
	Delay10KTCYx(20);
	Delay10KTCYx(20);

	while(masterLock[masterCount] != '\0')			// while loop to print the master 
								//lock message on LCD
	{
		WriteDataXLCD(masterLock[masterCount]);
		masterCount++;
		Delay10KTCYx(50);
	}
	Delay10KTCYx(20);

	while(1)			// Indefinite loop to keep the door locked untill remote
					// access is given to unlock the safe
	{
		MASTER = FALSE;
		if(MASTER_PIN)    	// waiting to get high input to break the loop..
		{

			trialCount = TOTAL_TRIALS;	// resetting the trial counts to its defualt value
			MASTER = TRUE;
			break;
		}
	}
} // eo masterLock::

/*>>> tempAlert: ===========================================================
Author:		Shubham
Date:		24/07/2024
Modified:	None 
Desc:		This function displays the Temperature alert on the LCD.
Input: 		None
Returns:	None
============================================================================*/
void tempAlert()
{
	char alertMessage[] = {"!!TEMP ALERT!! "};	// Temperature alert message
	char userMessage[] = {"!!SAFE LOCKED!! "};	// Safe Unlocked message
	char count = 0;
	Delay10KTCYx(20);
	WriteCmdXLCD(0x01);             // Clear display
	Delay10KTCYx(20);
	SetDDRamAddr(0x81);		// This function is used to keep or set the 
					// cursor at particular location.
	Delay10KTCYx(20);
	while(alertMessage[count] != '\0')
	{
		WriteDataXLCD(alertMessage[count]);
		count++;
		Delay10KTCYx(50);
	}
	while(BusyXLCD());
	Delay10KTCYx(20);
	count =0;
	SetDDRamAddr(0x40);		// This function is used to keep or set the 									// cursor at particular location.
	Delay10KTCYx(50);
	while(userMessage[count] != '\0')
	{
		WriteDataXLCD(userMessage[count]);
		count++;
		Delay10KTCYx(50);
	}
	while(BusyXLCD());
	
}//tempAlert::

/*>>> getADCSample: ===========================================================
Author:		Shubham
Date:		13/05/2024
Modified:	None
Desc:		This function gets ADC value from the input channel.
Input: 		char Chan is the channel number
Returns:	int ADRES is the converted ADC
 ============================================================================*/
int getADCSample(char chan)
{
	ADCON0bits.CHS = chan;
	ADCON0bits.GO = TRUE;
	while(ADCON0bits.GO);		// waiting for the samples to be calculated
	return ADRES;
}

/*>>> setADC: ===========================================================*/
/*Author:	Dhruv Satasiya
Date:		06/07/2024
Modified:	None
Desc:		sets the ADC registers for 12 TAD, Fosc/8, Channel 0,1,2 and 
		standard voltage references. 
Input: 		None
Returns:	None
============================================================================*/
void setADC()
{
	ADCON0 = 0x01;
	ADCON1 = 0x00;
	ADCON2 = 0xA9;
	
} // eo setADC::

/*>>> tempControl: ===========================================================
Author:		Shubham
Date:		06/07/2024
Modified:	None
Desc:		This function calculates the temperature by taking samples and converting it into volatge value.
Input: 		none
Returns:	None
 ============================================================================*/
void tempControl(void)
{
	/*All the local variables to calculate the temperature*/
	float volts = FALSE;
	int average = FALSE;
	float value = FALSE;
	char sampCount = FALSE;
	long sum = FALSE;
	char flag = FALSE;
	char index = FALSE;

	while(TRUE)
	{
		volts =	getADCSample(3);	// Getting samples from chan 3(Temp sensor)
		value = (float)volts*ADCRES;	// converting the result into voltage
		samplesArr[sampCount] = ((float)value/ TEMPM)-2; 
		sampCount++;
		if(sampCount>=SAMPSIZE)		// resetting the sampCount
		{
			sampCount = FALSE;
			flag = TRUE;		
		}
		if(flag)			// getting an average
		{
			for(index = FALSE;index<SAMPSIZE;index++)
			{
				sum += samplesArr[index];
			}
			average = sum/SAMPSIZE;
			sum = FALSE;
			if(average > 21)
			{
				TEMP_INDICATION = TRUE;
				tempAlert();
			}
			if(average<=21)
			{
				TEMP_INDICATION = FALSE;
				break;			
			}
			
		}
	}		
}//tempControl::

/*>>> keyPad: ===========================================================
Author:		Shubham
Date:		06/07/2024
Modified:	NoNe 
Desc:		This function displays the Right or Wrong password on the LCD Screen in first line
Input:		None
Returns:	None
============================================================================*/
void keyPad()
{	
	char count = 0;
	char correct[]={"Correct Password"};	// Correct password message
	char wrong[]={" Wrong Password"};	// Wrong password message
	char unlock[]={"Safe Unlocked "};	// Safe unlocking message
	SetDDRamAddr(0x40);			// This function is used to keep or set the 
	Delay1KTCYx(10);

	if(passFlag)
	{	
		while(wrong[count] != '\0')	
		{
			WriteDataXLCD(wrong[count]);
			count++;
			Delay10KTCYx(50);
		}
		Delay10KTCYx(20);
	
		while(BusyXLCD());
		WriteCmdXLCD(0x01);             // Clear display
		Delay10KTCYx(20);

		sprintf(trials, "Trials Left: %d", trialCount);		// Continuously updating the trials

		SetDDRamAddr(0x81);		// This function is used to keep or set the 
						// cursor at particular location.
		Delay10KTCYx(20);
		Delay10KTCYx(20);
		while(trials[trialsIndex] != '\0')
		{
			WriteDataXLCD(trials[trialsIndex]);
			trialsIndex++;
			Delay10KTCYx(50);
		}
		trialsIndex = 0;
		Delay10KTCYx(200);
		
		LOCK=FALSE;
		SECONDARY=FALSE;

		if(trialCount == 0)
		{
			masterLock();
		}		
	}
	else
	{
		trialCount = TOTAL_TRIALS;
		while(correct[count] != '\0')
		{
			WriteDataXLCD(correct[count]);
			count++;
			Delay10KTCYx(50);
			Delay10KTCYx(20);
		}
		while(BusyXLCD());	
		WriteCmdXLCD(0x01);             // Clear display
		Delay10KTCYx(250);
		SetDDRamAddr(0x82);		// This function is used to keep or set the 									// cursor at particular location.
		Delay10KTCYx(50);
		count = 0;
		while(unlock[count] != '\0')
		{
			WriteDataXLCD(unlock[count]);
			count++;
			Delay10KTCYx(100);
			LOCK=TRUE;		
		}	
		while(BusyXLCD());
		if(!passFlag)
		{
			SECONDARY = TRUE;
		}
				           	 // safe unlocked...
	}	
	WriteCmdXLCD(0x01);             	// Clear display
	Delay10KTCYx(20);
	
}//keyPad::

/*>>> getKey: ===========================================================
Author:		Dhruv Satasiya
Date:		06/07/2024
Modified:	11/07/2024 by Shubham 
Desc:		It continuously detects the keys to print the numbers from the keypad
Input: 		None
Returns:	None
============================================================================*/
char getKey()
{
	/*================COLUMN - 1 ===============*/
		Y1 = FALSE; Y2 = TRUE; Y3 = TRUE; Y4 = TRUE;
		if(X1 == 0)
		{
			return keyValue = '1';	
		}
		if(X2 == 0)
		{
			return keyValue = '4';	
		}	
		if(X3 == 0)
		{
			return keyValue = '7';
		}	
		if(X4 == 0)
		{
			return keyValue = '*';
		}
		
		/*================COLUMN - 2 ===============*/
		Y1 = TRUE; Y2 = FALSE; Y3 = TRUE; Y4 = TRUE;
		if(X1 == 0)
		{
			return keyValue = '2';
		}
		if(X2 == 0)
		{
			return keyValue = '5';
		}	
		if(X3 == 0)
		{
			return keyValue = '8';
		}	
		if(X4 == 0)
		{
			return keyValue = '0';
		}
		
		/*================COLUMN - 3 ===============*/
		Y1 = TRUE; Y2 = TRUE; Y3 = FALSE; Y4 = TRUE;
		if(X1 == 0)
		{
			return keyValue = '3';
		}
		if(X2 == 0)
		{
			return keyValue = '6';
		}	
		if(X3 == 0)
		{
			return keyValue = '9';
		}	
		if(X4 == 0)
		{
			return keyValue = '#';
		}
		
		/*================COLUMN - 4 ===============*/				
		Y1 = TRUE; Y2 = TRUE; Y3 = TRUE; Y4 = FALSE;
		if(X1 == 0)
		{
			return keyValue = 'A';
		}
		if(X2 == 0)
		{
			return keyValue = 'B';
		}	
		if(X3 == 0)
		{
			return keyValue = 'C';
		}	
		if(X4 == 0)
		{
			return keyValue = 'D';
		}		
		else
		{
			return keyValue = 'Z';
		}						
} // eo getKey ::


/*>>> userLogIn: ===========================================================
Author:		Shubham
Date:		13/05/2024
Modified:	None
Desc:		This function is to enter password mode and password storing in an array and do validation
Input: 		none
Returns:	None
 ============================================================================*/
void userLogIn(void)
{	
	/*All the user login*/
	
	char userPass[SIZE];			// array to hold the user enterd password 
	char encryption[SIZE];			// password encryption array
	char index = FALSE;			
	char currState = FALSE;			// current state and last state for the Keypad system
	char lastState = 0;	
	char element = FALSE;			
	char keypad = FALSE;			// variable to hold the key value form the keypad
	element = FALSE;
	
	for(index = 0; index<SIZE; index++)	// for loop to initalize array
	{
		userPass[index]= FALSE;
	}	
	for(index = 0; index<SIZE; index++)	// for loop to initalize array
	{
		encryption[index]= FALSE;
	}		
	if(getKey() == '*')			// password  mode  
	{	
		char message[]={"Enter Password"};
		//char trialCount = 2;
		char i= 0;			
		SetDDRamAddr(0x81);		// This function is used to keep or set the 
						// cursor at particular location.
		Delay10KTCYx(20);
		Delay10KTCYx(20);
		while(message[i] != '\0')
		{
			WriteDataXLCD(message[i]);
			i++;
			Delay10KTCYx(50);
		}
		Delay10KTCYx(20);
		
		/*.....Main Algorithm to get the user password 
			Let the user enter the password....*/	
		
		while(userPass[element]!='#')
		{
			currState = getKey();
			if(currState!=lastState&& currState !=('Z')&&currState !=('*'))
			{	
				keypad = currState;
				userPass[element]=keypad;
				encryption[element]='*';
				if(userPass[element]!='#')
				{
					element++;
				}
				
				fillPass(encryption);
				lastState = currState;
			}
			
		}
		trialCount--;		
		passFlag = strcmp(password,userPass);
		keyPad();	

		
	}
}//userLogIn::

/*>>> systemInit: ===========================================================
Author:		Shubham
Date:		13/05/2024
Modified:	None
Desc:		This function configures the ports ,calls initializing functions and initialize sample array.
Input: 		none
Returns:	None
 ============================================================================*/
void systemInit()
{
	char index = FALSE;
	ANSELA = 0x0B;			//Configuring RA Pins for input operation
	LATA = 0x00;
	TRISA=0xEC;

	//TRISCbits.TRISC3 = FALSE;
	ADCON0=0x01;
	ADCON1=0x00;
	ADCON2=0xA9;

	ANSELB = 0x00;
	TRISB =0xCF;

	ANSELC =0x00;
	LATC=0X00;
	TRISC =0x8E;

	ANSELEbits.ANSE2 = FALSE; 
	LATEbits.LATE2 = FALSE;
	TRISEbits.TRISE2 = TRUE;


//	configUSART1();
	configTMR0(PSC_VALUE);

	configTMR0(PSC_VALUE);
	setOsc();
	setADC();

	for(index = 0; index<SAMPSIZE; index++)		// for loop to initalize array
	{
		samplesArr[index]= FALSE;
	} 
}//eo systemInit

/*--- MAIN: FUNCTION ----------------------------------------------------------
 ----------------------------------------------------------------------------*/
void main( void )
{
	/*-------All the variables to--------------*/
	
	char display[]={"Greetings From"};
	char shiftMessage[]={"!!Tartarus!!"};			
	char insert = FALSE;
	char line1[]={"Long Press * to"};
	char line2[]={"Enter Pass Mode "};
	int dcValue = 0;
	char pbState = 0;
	char shift = 0;
	char currState = FALSE;
	char lastState = FALSE;
	char count = FALSE;
	char pbMask = 0x10;
	char index = FALSE;	
	SECONDARY = FALSE;
	TEMP_INDICATION = FALSE;
	
	systemInit();
	resetTMR0(PSC_VALUE);
	while(TRUE)
	{
		MASTER = TRUE;
		
		if(T0FLAG)				// modified part for the motion sensor
		{ 
			resetTMR0(PSC_VALUE);
			if(TRUE)
			{
				if(!MOTIONSEN)		//A5 pbState == 0x20
				{
					SYSON = TRUE;
					count = 0;		
				}
				else
				{
					count++;
					if(count>=150)
					{
						SYSON = FALSE;
						count = FALSE;
					}
				}
			}
		}

		tempControl();
		if(SYSON)
		{
			OpenXLCD(EIGHT_BIT & LINES_5X7);
			SetDDRamAddr(0x83);
			Delay10KTCYx(20);
			introMessage(display,shiftMessage);
			Delay10KTCYx(5);
			userLogIn();
			userMode(line1,line2);
			Delay10KTCYx(5);
			userLogIn();
		}			
	}
} // eo main::

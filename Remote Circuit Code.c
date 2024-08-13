/*-----------------------------------------------------------------------------
	File Name: Remote Circuit Code
	Author	       Shubham 
	Date:	        06/07/2024 
	Modified:  Shubham on 24/07/2024
	© Fanshawe College, 2024

	Description:This program receives alerts (flame, smoke, temperature alarms) 
			and signals from other circuits, displaying various messages on 
			the LCD. Additionally, the program allows for setting the system 
			into maintenance mode, resetting alarms, unlocking the master lock 
			and door lock, and displaying the status of sensors using push-button
			controls. LEDs are toggled according to the status of different settings.
-----------------------------------------------------------------------------*/

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
#pragma config LVP	= OFF
#pragma config MCLRE	= EXTMCLR

// Required Header 
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
#define DELAYCOUNT 20		// delay to introduce a debounce for the pushbuttons.
#define DELAYCOUNT_1 60   // for 15ms of delay
#define DELAYCOUNT_2 20   // for 5ms of delay
#define DEBOUNCE_DELAY 20
#define TMRON T0CONbits.TMR0ON
#define T0FLAG INTCONbits.TMR0IF
#define BYTESIZE 8
#define PSC_VALUE 0x0BDC
#define SHIFT2 2
#define RELAY LATAbits.LATA0
#define LOCK LATAbits.LATA1	
#define CONTROLLED LATBbits.LATB3
#define DCVAL 100 //dc value for 10% duty cycle
//#define LED LATEbits.LATE1
#define BUZZER LATCbits.LATC0	
#define SAMPSIZE 10					// Total number of samples
#define ONESEC 10
// LCD Display Orientation Commands
#define LINE1_LCD		0x00		// Start of line 1
#define LINE2_LCD		0x40		// Start of line 2
//KEYBOARD MATRIX


#define SIZE 10
#define STATUSPB PORTAbits.RA7
#define ALARM_RST_PB PORTAbits.RA6
#define UNLOCKPB PORTCbits.RC0
#define FLAMEALARM PORTBbits.RB5
#define SMOKEALARM PORTBbits.RB4
#define ALARMLED LATAbits.LATA1
#define ALARMBUZZER LATAbits.LATA0
#define SYSTEMOK LATAbits.LATA5 //System is OK
#define UNLOCKOUT LATCbits.LATC4 // To unlock DOOR and SAFE master lock
#define REMOXIN PORTCbits.RC1 //To read the remote access pushbutton
#define	REMOXOUT LATCbits.LATC5 //To write the remote access to EvacSystem
#define DISPLAYON PORTCbits.RC6
#define DISPLAYONLED LATAbits.LATA4
#define DOORLOCKLED  LATCbits.LATC2
#define MASTERON_OFF LATCbits.LATC3
#define MASTERLOCK_IN PORTCbits.RC7
#define MAINTENANCEMODE_LED LATAbits.LATA3
// Global Variables  ----------------------------------------------------------

	char passFlag = FALSE;
	char smokeFlag = FALSE;
	char flameFlag = FALSE;
	char tempFlag = FALSE;
	char alarmRST = FALSE;
/*>>> setOsc: ===========================================================
Author:	Shubham
Date:		06/07/2024
Modified:	NoNe 
Desc:		It Configures the PIC For 16 MHZ
Input: 		None
Returns:	None
============================================================================*/
void setOsc()
{
	OSCCON = 0x72; 	    //configures 16MHz operation 
	while(!OSCCONbits.HFIOFS);
} // eo setOsc::
/*>>> DelayFor18TCY: ===========================================================
Author:	Shubham
Date:		06/07/2024
Modified:	NoNe 
Desc:		It Configures for 18TCY delay
Input: 		None
Returns:	None
============================================================================*/

void DelayFor18TCY(void)
{
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
	Nop();
}//DelayFor18TCY::
/*>>> DelayPORXLCD: ===========================================================
Author:	Shubham
Date:		06/07/2024
Modified:	NoNe 
Desc:		It Configures for delay of 15ms 
Input: 		None
Returns:	None
============================================================================*/

void DelayPORXLCD (void)
{
 Delay1KTCYx(DELAYCOUNT_1);  
 return;
}//DelayPORXLCD::
/*>>> DelayXLCD: ===========================================================
Author:		Shubham
Date:		06/07/2024
Modified:	NoNe 
Desc:		It Configures for delay of 5ms 
Input: 		None
Returns:	None
============================================================================*/

void DelayXLCD (void)
{
 Delay1KTCYx(DELAYCOUNT_2); 
 return;
}//DelayXLCD::

/*>>> userMode: ===========================================================
Author:	Shubham
Date:		06/07/2024
Modified:	NoNe 
Desc:		This function displays the prompt meant for user
Input:		char *ptr1 and char *ptr2 to the arrays having user mode message
Returns:	None
============================================================================*/
void userMode(char *ptr1,char *ptr2)//DISPLAY INTRUDER ALERT
{
	SetDDRamAddr(0x82);			// Sets the cursor in first line 
	Delay10KTCYx(40);
	while(*ptr1 != '\0')// while loop to print the data
	{
		WriteDataXLCD(*ptr1);
		ptr1++;
		Delay10KTCYx(50);
	}//eo while
	Delay10KTCYx(20);
	SetDDRamAddr(0x41);			// This sets the cursor in second line									
	Delay10KTCYx(50);
	while(*ptr2 != '\0')
	{
		WriteDataXLCD(*ptr2);
		ptr2++;
		Delay10KTCYx(50);
	}//eo while
	while(BusyXLCD());//continue until the LCD is busy
	WriteCmdXLCD(0x01);             // Clear display
	Delay10KTCYx(20);
	CONTROLLED = TRUE;//turning on the control On led
}//userMode::
/*>>> introMessage: ===========================================================
Author:	Shubham
Date:		24/07/2024
Modified:	NoNe 
Desc:		This function displays the the introduction message in first and second line
Input:		char *ptr1 and char *ptr2 to the arrays having Introduction message
Returns:	None
============================================================================*/
void introMessage(char *ptr1,char *ptr2)
{
	SetDDRamAddr(0x81);			// This function is used to keep or set the 
									// cursor at particular location.
	Delay10KTCYx(4);
//	Delay10KTCYx(20);
	while(*ptr1 != '\0')
	{
		WriteDataXLCD(*ptr1);
		ptr1++;
		Delay10KTCYx(50);
		flameFlag = FLAMEALARM;
		smokeFlag = SMOKEALARM;
	}
	Delay10KTCYx(5);
	SetDDRamAddr(0x42);			// This function is used to keep or set the 									// cursor at particular location.
	Delay10KTCYx(5);
	while(*ptr2 != '\0')
	{
		WriteDataXLCD(*ptr2);
		ptr2++;
		Delay10KTCYx(50);
		flameFlag = FLAMEALARM;
		smokeFlag = SMOKEALARM;
	}
	while(BusyXLCD());
	WriteCmdXLCD(0x01);             // Clear display
	Delay10KTCYx(2);
}//introMessage::

/*>>> fillPass: ===========================================================
Author:	Shubham
Date:		06/07/2024
Modified:	NoNe 
Desc:		This function displays the Password Entering process on the LCD 
			Screen in first line
Input:		char *ptr to the arrays having user message
Returns:	None
============================================================================*/
void fillPass(char *ptr)// Display PASSWORD ENTERING PROCESS
{
	SetDDRamAddr(0x45);// This function is used to keep Cursor at 2nd LINE at 5 position
	Delay1KTCYx(TRUE);
	while(*ptr != '\0')
	{
		WriteDataXLCD(*ptr);
		ptr++;
		Delay10KTCYx(20);
	}//eo while
	while(BusyXLCD());
	
}//fillPass::
/*>>> almRST: ===========================================================
Author:	Shubham
Date:		07/08/2024
Modified:	NoNe 
Desc:		This function turn on/off buzzer
Input: 		None
Returns:	None
============================================================================*/

void almRST (void)
{
 	//ALARM RESET(BUZZER OFF)		
		if(!ALARM_RST_PB&&!alarmRST)//turn off the buzzer
		{
			alarmRST = TRUE;
		}//eo if
		else if(!ALARM_RST_PB&&alarmRST)// turn on the buzzer
		{
			alarmRST= FALSE;	
		}//eo else if
		
}//almRST::
/*>>> tempAlarm: ===========================================================
Author:	Shubham
Date:		25/07/2024
Modified:	NoNe 
Desc:		This function displays temperature warning on LCD
Returns:	None
============================================================================*/
void tempAlarm()
{
	char count = 0;
	char tempAlarm[]={"!Temp Alarm!"};
	SetDDRamAddr(0x80);	// setting cursor in first line
	Delay1KTCYx(10);
	if(tempFlag)
	{
		SYSTEMOK=FALSE;//turning off system LED
		DISPLAYONLED= FALSE;//Turning off display LED
		while(tempAlarm[count] != '\0')
		{
			WriteDataXLCD(tempAlarm[count]);
			count++;
			Delay10KTCYx(50);
			flameFlag = FLAMEALARM;
			smokeFlag = SMOKEALARM;
			almRST();//checking buzzer status
		}//eo while
		while(BusyXLCD());
// toggling the Alarm LED and Buzzer
		ALARMLED = TRUE;
		almRST();
		if(!alarmRST)
		{
			ALARMBUZZER = TRUE;
		}		
		Delay10KTCYx(40);
		ALARMLED = FALSE;
		ALARMBUZZER = FALSE;
		Delay10KTCYx(100);
		almRST();
		ALARMLED = TRUE;
		if(!alarmRST)
		{
			ALARMBUZZER = TRUE;
		}		
		Delay10KTCYx(40);
		ALARMLED = FALSE;
		ALARMBUZZER = FALSE;
		almRST();
		ALARMLED = TRUE;
		if(!alarmRST)
		{
			ALARMBUZZER = TRUE;
		}		
		Delay10KTCYx(100);
		ALARMLED = FALSE;
		ALARMBUZZER = FALSE;
	}
	Delay10KTCYx(20);	
	WriteCmdXLCD(0x01);             // Clear display
	Delay10KTCYx(2);
}//tempAlarm::
/*>>> flameAlarm: ===========================================================
Author:		Shubham
Date:		25/07/2024
Modified:	NoNe 
Desc:		This function displays the flame alarm warning
Input:		None
Returns:	None
============================================================================*/
void flameAlarm()
{
	char count = 0;
	char FlameAlarm[]={" !!Flame Alarm!!"};
	
	SetDDRamAddr(0x80);//sets the cursor in first line
	Delay1KTCYx(10);
	
	if(flameFlag)
	{
		SYSTEMOK=FALSE;
		DISPLAYONLED= FALSE;
		while(FlameAlarm[count] != '\0')
		{
			WriteDataXLCD(FlameAlarm[count]);
			count++;
			Delay10KTCYx(50);
			flameFlag = FLAMEALARM;//checking other alarm status
			smokeFlag = SMOKEALARM;
			almRST();
		}
		while(BusyXLCD());
		almRST();
//toggling alarm and Buzzer
		ALARMLED = TRUE;
		if(!alarmRST)
		{
			ALARMBUZZER = TRUE;
		}		
		Delay10KTCYx(100);
		ALARMLED = FALSE;
		ALARMBUZZER = FALSE;
		Delay10KTCYx(40);
		almRST();
		ALARMLED = TRUE;
		if(!alarmRST)
		{
			ALARMBUZZER = TRUE;
		}		
		Delay10KTCYx(100);
		ALARMLED = FALSE;
		ALARMBUZZER = FALSE;
		almRST();
		ALARMLED = TRUE;
		if(!alarmRST)
		{
			ALARMBUZZER = TRUE;
		}//eo if		
		Delay10KTCYx(100);
		ALARMLED = FALSE;
		ALARMBUZZER = FALSE;
		count = 0;
	}//eo if
	Delay10KTCYx(20);	
	WriteCmdXLCD(0x01);             // Clear display
	Delay10KTCYx(2);
}//flameAlarm::
/*>>> smokeAlarm: ===========================================================
Author:		Shubham
Date:		25/07/2024
Modified:	NoNe 
Desc:		This function displays Smoke warning
Input:		None
Returns:	None
============================================================================*/
void smokeAlarm()// Display PASSWORD ENTERING PROCESS
{
	char count = 0;
	char smokeAlarm[]={" !!Smoke Alarm!!"};
	
	SetDDRamAddr(0x80);	
	Delay1KTCYx(10);
	
	if(smokeFlag)
	{
		SYSTEMOK=FALSE;
		DISPLAYONLED= FALSE;
		while(smokeAlarm[count] != '\0')
		{
			WriteDataXLCD(smokeAlarm[count]);
			count++;
			Delay10KTCYx(50);
			almRST();
		}
		while(BusyXLCD());
		almRST();
		ALARMLED = TRUE;
		if(!alarmRST)
		{
			ALARMBUZZER = TRUE;
		}		
		Delay10KTCYx(100);
		ALARMLED = FALSE;
		ALARMBUZZER = FALSE;
		Delay10KTCYx(40);
		almRST();
		ALARMLED = TRUE;
		if(!alarmRST)
		{
			ALARMBUZZER = TRUE;
		}		
		Delay10KTCYx(100);
		ALARMLED = FALSE;
		ALARMBUZZER = FALSE;
		Delay10KTCYx(40);
		almRST();
		ALARMLED = TRUE;
		if(!alarmRST)
		{
			ALARMBUZZER = TRUE;
		}		
		Delay10KTCYx(100);
		ALARMLED = FALSE;
		ALARMBUZZER = FALSE;
		count = 0;	
	}		
	Delay10KTCYx(20);	
	WriteCmdXLCD(0x01);             // Clear display
	Delay10KTCYx(2);
}//smokeAlarm::
/*>>> serviceMode: ===========================================================
Author:		Shubham
Date:		25/07/2024
Modified:	NoNe 
Desc:		This function displays the Service mode message
Input:		None
Returns:	None
============================================================================*/
void serviceMode()
{
	char count = 0;
	char serviceMode[]={" !!SERVICE--MODE!!"};
	
	SetDDRamAddr(0x80);		
	Delay1KTCYx(10);
	
	if(smokeFlag)
	{
		SYSTEMOK=FALSE;
		while(serviceMode[count] != '\0')
		{
			WriteDataXLCD(serviceMode[count]);
			count++;
			Delay10KTCYx(50);
		}
		while(BusyXLCD());
	}		
	Delay10KTCYx(20);	
	WriteCmdXLCD(0x01);             // Clear display
	Delay10KTCYx(2);
}//serviceMode::
/*>>> lockMessage: ===========================================================
Author:		Shubham
Date:		25/07/2024
Modified:	NoNe 
Desc:		This function displays the Unlocking message
Returns:	None
============================================================================*/
void lockMessage()
{
	char count = 0;
	char message1[]={" SAFE UNLOCKED "};
	char message2[]={" DOOR UNLOCKED "};
	
	SetDDRamAddr(0x80);
	Delay1KTCYx(10);	
	while(message1[count] != '\0')
	{
		WriteDataXLCD(message1[count]);
		count++;
		Delay10KTCYx(50);
	}	
	Delay1KTCYx(100);
	SetDDRamAddr(0x80);
	count = FALSE;	
	while(message2[count] != '\0')
	{
		WriteDataXLCD(message2[count]);
		count++;
		Delay10KTCYx(50);
	}
	while(BusyXLCD());				
	Delay10KTCYx(20);	
	WriteCmdXLCD(0x01);             // Clear display
	Delay10KTCYx(2);
}//lockMessage::
/*>>> systemStatus: ===========================================================
Author:		Shubham
Date:		06/07/2024
Modified:	NoNe 
Desc:		This function displays status of different sensor status
Returns:	None
============================================================================*/
void systemStatus()
{
	char count = 0;
	char sensors[]={" !Sensor Status!"};
	char tempOk[]={"Temp:OK"};
	char FlameOk[]={"Flame:OK "};
	char smokeOk[]={"    Smoke:OK    "};
	char tempF[]={"Temp:FL"};
	char FlameF[]={"Flame:FL"};
	char smokeF[]={"    Smoke:FL   "};
	SetDDRamAddr(0x80);		
	Delay1KTCYx(10);
	
	while(sensors[count] != '\0')
	{
		WriteDataXLCD(sensors[count]);
		count++;
		Delay10KTCYx(50);
	}
	while(BusyXLCD());	
	Delay10KTCYx(250);
	SetDDRamAddr(0x40);	
	Delay10KTCYx(50);
	count = 0;
	if(tempFlag)
	{
		while(tempF[count] != '\0')
		{
			WriteDataXLCD(tempF[count]);
			count++;
			Delay10KTCYx(100);
		}
	}
	else
	{
		while(tempOk[count] != '\0')
		{
			WriteDataXLCD(tempOk[count]);
			count++;
			Delay10KTCYx(100);
		}
	}	
	while(BusyXLCD());
	Delay10KTCYx(250);
	SetDDRamAddr(0x48);	
	Delay10KTCYx(50);
	count = 0;
	if(flameFlag)
	{
		while(FlameF[count] != '\0')
		{
			WriteDataXLCD(FlameF[count]);
			count++;
			Delay10KTCYx(100);
		}
	}
	else
	{
		while(FlameOk[count] != '\0')
		{
			WriteDataXLCD(FlameOk[count]);
			count++;
			Delay10KTCYx(100);
		}
	}
	
	while(BusyXLCD());
	Delay10KTCYx(250);
	SetDDRamAddr(0x40);	
	Delay10KTCYx(50);
	count = 0;
	if(smokeFlag)
	{
		while(smokeF[count] != '\0')
		{
			WriteDataXLCD(smokeF[count]);
			count++;
			Delay10KTCYx(100);
		}
	}
	else
	{
		while(smokeOk[count] != '\0')
		{
			WriteDataXLCD(smokeOk[count]);
			count++;
			Delay10KTCYx(100);
		}	
	}
	
	while(BusyXLCD());
	Delay10KTCYx(250);	
	WriteCmdXLCD(0x01);             // Clear display
	Delay10KTCYx(20);
	
}//systemStatus::
/*>>> status: ===========================================================
Author:		Shubham
Date:		06/07/2024
Modified:	NoNe 
Desc:		This function call teh status function on pressing status PB
Input: 		None
Returns:	None
============================================================================*/

void status(void)
{
	char count = FALSE;
	if(!STATUSPB)
	{			
		while(!count)
		{
			count = 1;
			systemStatus();
			if(!STATUSPB)
			{
				break;
			}
		}
					
	}
}//status::

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
	ANSELA = 0x00;	//Configuring RA Pins for input operation
	LATA = 0x00;
	TRISA=0xC0;

	ADCON0=0x01;
	ADCON1=0x00;
	ADCON2=0xA9;
	ANSELB = 0x00;
	TRISB =0xF7;
	ANSELC =0x00;
	LATC=0X00;
	TRISC =0xC3;
	setOsc();
}//systemInit::

/*--- MAIN: FUNCTION ----------------------------------------------------------
 ----------------------------------------------------------------------------*/
void main( void )
{
	char display[]={"Greetings From"};
	char shiftMessage[]={"!!Tartarus!! "};			
	char insert = FALSE;
	char line1[]={"**Controls**"};
	char line2[]={"**Turning On** "};
	int dcValue = 0;
	char pbState = 0;
	char shift = 0;
	char currState = FALSE;
	char lastState = FALSE;
	char count = FALSE;
	char pbMask = 0x10;
	char rFlag = FALSE;
	char mFlag = FALSE;
	MAINTENANCEMODE_LED = FALSE;
	CONTROLLED= FALSE;
	systemInit();
	OpenXLCD(EIGHT_BIT & LINES_5X7);
	SetDDRamAddr(0x83);
	Delay10KTCYx(20);
	introMessage(display,shiftMessage);	
	userMode(line1,line2);
	while(TRUE)
	{
		almRST();
		//checking alarms
		flameFlag = FLAMEALARM;
		smokeFlag = SMOKEALARM;
		//UNLOACKING SAFE AND DOOR
		if(!UNLOCKPB&& mFlag)// if loop to unlock when unlock PB is pressed
		{
			count =1;
			UNLOCKOUT = TRUE;			
			MASTERON_OFF= FALSE;
			lockMessage();
		}//eo if
		else if(UNLOCKPB)
		{
			UNLOCKOUT = FALSE;//resetting the output
			
		}//eo else if
		
		if(MASTERLOCK_IN&&!mFlag)//if loop to check the master lock signal from other PIC
		{
			MASTERON_OFF =TRUE;
			mFlag = TRUE;
		}
		else if(!MASTERLOCK_IN)
		{
			mFlag = FALSE;
		}
		//SYSTEM OK INDICATOR
		//if loop to display greeting message if there is no alarm and turn on status ok LED
		if(!tempFlag&&!smokeFlag&&!flameFlag&&!DISPLAYON)
		{
			SYSTEMOK=TRUE;
			introMessage(display,shiftMessage);
			DISPLAYONLED = TRUE;		
		}
		status();
		//DOOR SYSTEM
		if(!smokeFlag&&!flameFlag)
		{
			DOORLOCKLED = FALSE;
		}
		else if(tempFlag||DISPLAYON)
		{
			DOORLOCKLED= TRUE;
		}

		status();
		//If loop to display alarm if any alarm signal is detected
		if((tempFlag||smokeFlag||flameFlag)&&!REMOXOUT)
		{
			almRST();
			tempAlarm();
			flameAlarm();
			smokeAlarm();
		}
		//if loop to go into MAINTENANCE MODE 
		if (!REMOXIN&&!rFlag)
		{
			rFlag = TRUE;	
			MAINTENANCEMODE_LED = TRUE;
			REMOXOUT = TRUE;
			serviceMode();

		}//eo if
		else if(rFlag&&!REMOXIN)
		{
			REMOXOUT = FALSE;
			MAINTENANCEMODE_LED = FALSE;
			rFlag= FALSE;
		}//eo else if	
	}
} // eo main::


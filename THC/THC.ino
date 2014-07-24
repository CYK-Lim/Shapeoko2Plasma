/*
 * Sketch1.ino
 *
 * Timer 3 counts the 1Mhz reference from the THCAD (Port E bit 6)
 * Timer 4 counts the signal from the THCAD (Port H bit 7)
 * Timer 5 controls the output to the stepper driver (pins 45-46)
 * Int4 is the input from the parallel port (pin 2) counting pulses
 * Int5 is the input from the parallel port (pin 3) reading direction
 * Int7 is the input from the parallel port (Port E bit 7) THC ON
 *
 * Created: 5/30/2014 2:10:21 PM
 * Author: owner
 */

int timer4Val = 0; 
int interruptCntr = 0;
boolean porte4s = false;
boolean porte5s = false;
float voltageACT = 0.0;
float voltageSP = 70.0;
float deadBand = 1.0;
long thcCorrectInc = 1;
int FreqZero = 1212;
int incomingByte = 0;
long currPosACT = 0;
long currPosIN = 0;
long currPosSP = 0;
boolean timer5ON = false;
int temp = 0;
boolean zDirIn = false;
boolean thcOn = false;
float thcKP = 0.10;
float voltageERR = 0.0;
boolean voltageUpdated = 0;
unsigned int junkCounter = 0;


int rampMin = 230;
int rampMax = 1830;
int rampVel = 1830;
int rampOCR = 1830;
int rampDeltaErr = 0;
int rampErrDir = 0;
int rampMotorDir = 0;


//Only called upon startup
void setup()
{
  
        DDRB |= _BV(DDB7); // set direction for the LED
        
        
	DDRL |= _BV(DDL4) | _BV(DDL3); //set them as outputs  
	
	//Serial.begin(57600);
	//Serial.println("Serial Started");
		
//////// TIMER 3 ///////// TIMER 3 ////////
	  
	// External clock, Clear timer on Compare Match
	TCCR3A = 0x0;
	TCCR3B = _BV(WGM32)|_BV(CS32) | _BV(CS31) | _BV(CS30);
		
	//Set the top and enable the interrupt
	OCR3A = 0xA17; 
	TIMSK3 = _BV(OCIE3A);
		
	//Set the pin as an input Port E bit 6
	DDRE &= ~_BV(DDE6);

//////// TIMER 4 ///////// TIMER 4 ////////		
			
	//Normal counter, select external Clock rising edge 
	TCCR4A = 0x0;
	TCCR4B = _BV(CS42) | _BV(CS41) | _BV(CS40);
		
	// Set Port H Bit 7 as an input
	DDRH &= ~_BV(DDH7);
	
//////// TIMER 5 ///////// TIMER 5 ////////
	
	//Set it to toggle the bit on COM5B on match (phase and freq correct	
	TCCR5A = _BV(COM5B1) | _BV(WGM50);
	TCCR5B = _BV(WGM53); //leave the clock off for now
	
	//Set the top and compare value
	OCR5A = 0xE6; //E6=230//398=920//730=1840 is 115uS with no prescaling (top)
	OCR5B = 0x73; //73=115//1CC=460//398=920 is 57.5uS with no prescaling (toggle bit)
	
	TIMSK5 = _BV(TOIE5); //enable bottom interrupt	
	
///////////// EXTERNAL INTERRUPTS /////////////////////////

	//Set the external interrupt edge triggers	
	EICRB =  _BV(ISC70)| _BV(ISC50) | _BV(ISC41) | _BV(ISC40);
	
	//Enable the external interrupt
	EIMSK = _BV(INT7) | _BV(INT5) | _BV(INT4);
	
	//Set port E bit 4 as an input and enable pull up resistor
	DDRE &= ~_BV(DDE4);
	PORTE |= _BV(PORTE4);
	
	//Set port E bit 5 as an input and enable pull up resistor
	DDRE &= ~_BV(DDE5);
	PORTE |= _BV(PORTE5);
	
	//Set port E bit 7 as an input and enable pull up resistor
	DDRE &= ~_BV(DDE7);
	PORTE |= _BV(PORTE7);
	

	
	// Clear all timers
	TCNT3 = 0;
	TCNT4 = 0;
	TCNT5 = 0;
	
	currPosACT = 0;
	currPosIN = 0;
	currPosSP = 0;
	timer5ON = 0;
	
//NEED TO INITIALIZE THE PINS THAT ARE INTERRUPT DRIVEN
	temp = PINE & _BV(PINE7);
	
	if (temp != 0)
		thcOn = 1;
	else
		thcOn = 0;
		
	temp = PINE & _BV(PINE5);
			
	if (temp != 0)
		zDirIn = 1;
	else
		zDirIn = 0;
	
}

//STEP counter from the PC
ISR(INT4_vect) 
{
	//porte4s = ~porte4s;

	if (zDirIn != 0)
		currPosIN--;
	else
		currPosIN++;
		
	if(!thcOn)
		currPosSP = currPosIN;
}

//Direction monitor from the PC
ISR(INT5_vect) 
{
	//porte5s = ~porte5s;
	
	temp = PINE & _BV(PINE5);
	
	if (temp != 0)
		zDirIn = 1;
	else
		zDirIn = 0;	
}

//Monitors the THC ON/OFF command from the PC
ISR(INT7_vect)
{
	//This is where the THC Enable is monitored
	temp = PINE & _BV(PINE7);
		
	if (temp != 0)
        {
		thcOn = 1;

                //sets the start speed for the ramp
                OCR5A = 0x726; //E6=230//398=920//730=1840 is 115uS with no prescaling (top)
	        OCR5B = 0x393;
                rampOCR = 1830;
                
        }
	else
	{
		thcOn = 0;
		currPosSP = currPosIN;
		currPosACT = currPosIN;

                //bypasses the ramp to pass pulses through
                OCR5A = 0xE6; //E6=230//398=920//730=1840 is 115uS with no prescaling (top)
	        OCR5B = 0x73;
	}
}

//Collects the counts from the V/F converter
ISR(TIMER3_COMPA_vect)
{
	timer4Val = TCNT4;
	TCNT4 = 0;
	voltageUpdated = 1;
}

//Updates the output position, issues new step, or stops stepping
ISR(TIMER5_OVF_vect ) 
{
	//Sample the pin to see which direction it moved
	temp = PINL & _BV(PINL3);
	
	if (temp != 0)
		currPosACT--;
	else
		currPosACT++;
	
	//see if there are more pulses to be delivered
        if (thcOn)
        {
                rampVel = rampMax - rampOCR;
                
                if( rampVel == 0 )
                {
                        if (currPosACT > currPosSP)
                	{
                		PORTL |= _BV(PORTL3);  //set direction for motor
                	}
                  	else if ( currPosACT < currPosSP)
                  	{
                  		PORTL &= ~_BV(PORTL3);	//set direction for motor
                  	}
                        else
        	        {
        		    //no pulses are needed, stop the timer	
        		    TCCR5B &= 0xF8;
        		    timer5ON = 0;	
        	        } 
                }                
                else if( temp == 0 )
                {
                   if( currPosSP > ( currPosAct + rampVel ))
                   {
                      rampOCR--; //Speed up
                      if (rampOCR < rampMin)
                        rampOCR = rampMin;
                        
                      OCR5A = rampOCR;
                      OCR5B = rampOCR/2;
                   }
                   else if( currPosSP <= ( currPosAct + rampVel ))
                   {
                      rampOCR++; //Slow down
                      if (rampOCR < rampMin)
                        rampOCR = rampMin;
                        
                      OCR5A = rampOCR;
                      OCR5B = rampOCR/2;
                   }
                }
                else if( temp != 0 )
                {
                   if( currPosSP < ( currPosAct - rampVel ))
                   {
                      rampOCR--; //Speed up
                      if (rampOCR < rampMin)
                        rampOCR = rampMin;
                        
                      OCR5A = rampOCR;
                      OCR5B = rampOCR/2;
                   }
                   else if( currPosSP >= ( currPosAct + rampVel ))
                   {
                      rampOCR++; //Slow down
                      if (rampOCR < rampMin)
                        rampOCR = rampMin;
                        
                      OCR5A = rampOCR;
                      OCR5B = rampOCR/2;
                   }
                }                         
        }
        else
        {                
        	if (currPosACT > currPosSP)
        	{
        		PORTL |= _BV(PORTL3);  //set direction for motor
        	}
        	else if ( currPosACT < currPosSP)
        	{
        		PORTL &= ~_BV(PORTL3);	//set direction for motor
        	}
        	else
        	{
        		//no pulses are needed, stop the timer	
        		TCCR5B &= 0xF8;
        		timer5ON = 0;	
        	}
        }
}

//Calculates the voltage feedback
void calcVolts()
{
	/*	
	Voltage =  voltageFullScale * (timer4Val - FreqZero) / (freqFullScale - FreqZero)
	

	Where FreqOut is the output frequency of the THCAD, VFS is the unipolar full scale
	range 10V or 300V or custom value determined by external resistors. FFS is the full scale
	output frequency on the calibration sticker, and FZERO is the 0V input output frequency
	on the calibration sticker
	*/
	
	float temp = timer4Val*4-1212;
	
	if(temp <= 0)
	{
		voltageACT = 0;
	}else
	{
		voltageACT =  300 * temp / 8399;
	}	
}

//Main loop
void loop()
{
  
      //calcVolts();//moved out of loop below just for debugging
	
	// if THC is on and there is a new data point available
	if (thcOn && voltageUpdated)
	{
		voltageUpdated = 0;
		calcVolts();
		
		voltageERR = voltageSP - voltageACT;
		
		if (voltageACT < (voltageSP-deadBand))
		{
			currPosSP += thcCorrectInc*voltageERR*thcKP;
		}
		else if (voltageACT > (voltageSP+deadBand))
		{
			currPosSP += thcCorrectInc*voltageERR*thcKP;
		} 

	}
	
	
	//Start the output pulsing if it is not already
	if (timer5ON == 0 )
	{			
		if (currPosACT != currPosSP)
		{
			if (currPosACT > currPosSP)
				PORTL |= _BV(PORTL3);//set the direction	
			else 
				PORTL &= ~_BV(PORTL3);//set the direction
				
			
			//start the timer
			TCCR5B |= _BV(CS50); // | _BV(CS50);
			timer5ON = 1;
		}
	}
	
        //LED for troubleshooting
        if( thcOn )
          PORTB |= _BV(7);
        else
          PORTB &= ~_BV(7);
          
	//debugJunk();		
}

//Just for debugging
void debugJunk()
{
	junkCounter++;
/*	
	if (Serial.available() )//> 0 && timer5ON == 0)
	{
		incomingByte = Serial.read();
			
		if(incomingByte == '0')
		{
			thcOn = 0;
		}
		else if (incomingByte == '1')
		{
			thcOn = 1;
		}
	}
	
	
*/	
	
		//Serial.print(temp2,BIN);
	
	if (!(junkCounter % 2000))
	{
		Serial.print(thcOn,BIN);
		Serial.print(" ");
		Serial.print(voltageSP);
		Serial.print(" ");
		Serial.print(voltageACT);
		Serial.print(" ");
		Serial.print(currPosIN);
		Serial.print(" ");
		Serial.println(zDirIn);
	}
	

	//delay(50);
	


	/*
	
	
	temp2 = PINL & _BV(PINL4);
	
	if (temp2 > 0)
	{
		Serial.print("UP");
	}
	else
	{
		Serial.print("DN");
	}
	
	

	//Serial.print(temp2,BIN);
	

		Serial.print(" ");
	Serial.print(timer5ON);
	Serial.print(" ");
	Serial.println(currPos);
	
	delay(10);
	
	*/
	//digitalWrite(13, porte5s);
	



	
	/* 	
	Serial.print(voltage);
	Serial.print("\t ");
	Serial.println(timer4Val);
	Serial.println();
	delay(100);


	digitalWrite(13, HIGH);
	Serial.println("High");
	delay(250);
	digitalWrite(13,LOW);
	Serial.println("Low");
	delay(250);
	*/
}

/*********************************************************************
 *               analog example for Versa1.0
 *	Analog capture on connectors K1, K2, K3 and K5. 
 *********************************************************************/

#define BOARD Versa1

#include <fruit.h>
#include <analog.h>
#include <dcmotor.h>

t_delay mainDelay;
t_ramp rampeVitesseA, rampeVitesseB;
DCMOTOR_DECLARE(A);
DCMOTOR_DECLARE(B);

t_delay secondesDelay;
unsigned char secondes, minutes, heures;
long int Seconds;

#define COURANT_MAX_MILLIAMPERES 5000L

#define COURANT_MAX ((COURANT_MAX_MILLIAMPERES * 57) / 500) // /1000 * 140mV/A / 5V * 4096

void setup(void) {	
//----------- Setup ----------------
	fruitInit();
			
	pinModeDigitalOut(LED); 	// set the LED pin mode to digital out
	digitalClear(LED);		// clear the LED
	delayStart(mainDelay, 5000); 	// init the mainDelay to 5 ms
	delayStart(secondesDelay, 1000000); 	// init the secondesDelay to 1 s

//----------- Analog setup ----------------
	analogInit();		// init analog module
	analogSelect(0,MOTA_CURRENT);	// assign MotorA current sense to analog channel 0
	analogSelect(1,MOTB_CURRENT);	// assign MotorA current sense to analog channel 0

//----------- dcmotor setup ----------------

    //DCMOTOR_INIT(A);
    //DCMOTOR_INIT(B);
    dcmotorInit(A);
    dcmotorInit(B);
    
    rampInit(&rampeVitesseA);
    rampInit(&rampeVitesseB);
    
    EEreadMain();
}

int i = 0;
unsigned char countA = 0, countB = 0;
#define COUNT_MAX 10 // * 5ms = 50ms
void sequenceCompute();

void timeReset() { heures = minutes = secondes = 0; Seconds = 0;}

void timeSet(unsigned char h, unsigned char m, unsigned char s ) 
{ 
    heures = h;
    minutes = m;
    secondes = s;
    Seconds = 3600L*h + 60L*m + s;
}

long int toSeconds(unsigned char h, unsigned char m, unsigned char s) 
{
    return 3600L*h + 60L*m + s;
}

void loop() {
// ---------- Main loop ------------
	fraiseService();	// listen to Fraise events
	analogService();	// analog management routine

	if(delayFinished(mainDelay)) // when mainDelay triggers :
	{
		delayStart(mainDelay, 5000); 	// re-init mainDelay
		analogSend();		// send analog channels that changed

		rampCompute(&rampeVitesseA);
		rampCompute(&rampeVitesseB);
		
		DCMOTOR(A).Vars.PWMConsign = rampGetPos(&rampeVitesseA);
		DCMOTOR(B).Vars.PWMConsign = rampGetPos(&rampeVitesseB);
		
		if(analogGet(0) > COURANT_MAX) { if(countA < COUNT_MAX) countA++; }
		else { if(countA > 0) countA--; }
		
		if(analogGet(1) > COURANT_MAX) { if(countB < COUNT_MAX) countB++; }
		else { if(countB > 0) countB--; }
		
		if(countA == 0) { digitalSet(MAEN); digitalSet(MAEN2); }
		else if(countA == COUNT_MAX) { digitalClear(MAEN); digitalClear(MAEN2); }

		if(countB == 0) { digitalSet(MBEN); digitalSet(MBEN2); }
		else if(countB == COUNT_MAX) { digitalClear(MBEN); digitalClear(MBEN2); }

		DCMOTOR_COMPUTE(A,SYM);
		DCMOTOR_COMPUTE(B,SYM);
		
		if(i++ == 10) {
		    i = 0;
		    printf("C P %d %d\n",DCMOTOR(A).Vars.PWMConsign, DCMOTOR(B).Vars.PWMConsign);
		}
        
	}
	
	if(delayFinished(secondesDelay)) // when secondesDelay triggers :
    {
	    delayStart(secondesDelay, 1000000); 	// init the secondesDelay to 1 s
	    Seconds++;
	    if(++secondes >= 60) {
	        secondes = 0;
            if(++minutes >= 60) {
                minutes = 0;
                heures++;
            }
        }
        sequenceCompute();
    }
}

// Receiving

void fraiseReceiveChar() // receive text
{
	unsigned char c;
	
	c=fraiseGetChar();
	if(c=='L'){		//switch LED on/off 
		c=fraiseGetChar();
		digitalWrite(LED, c!='0');		
	}
	else if(c=='E') { 	// echo text (send it back to host)
		printf("C");
		c = fraiseGetLen(); 			// get length of current packet
		while(c--) printf("%c",fraiseGetChar());// send each received byte
		putchar('\n');				// end of line
	}
	else if(c=='S') { // save EEPROM
	    EEwriteMain();
	}	
	else if(c=='R') { // save EEPROM
        timeReset();
	}	
}



void fraiseReceive() // receive raw
{
	unsigned char c;
	c=fraiseGetChar();
	
	switch(c) {
	    case 10 :  timeSet(fraiseGetChar(), fraiseGetChar(), fraiseGetChar()); break;
	    case 120 : DCMOTOR_INPUT(A) ; break;
	    case 121 : DCMOTOR_INPUT(B) ; break;
	    case 122 : rampInput(&rampeVitesseA); break;
	    case 123 : rampInput(&rampeVitesseB); break;
	}
}

void EEdeclareMain() {
    rampDeclareEE(&rampeVitesseA);
    rampDeclareEE(&rampeVitesseB);
}  

/*---------------------------------------------------------*/
t_time Time;

void sequenceCompute()
{
    printf("C T %d %d %d %ld\n",heures, minutes, secondes, Seconds);
    //rampGoto(&rampeVitesseA, vitesseA);
    //return;
    
    if(Seconds == toSeconds(0,0,10)) { rampGoto(&rampeVitesseA, 500) ;}
    else if(Seconds == toSeconds(0,0,30)) { rampGoto(&rampeVitesseB, 500) ;}
    else if(Seconds == toSeconds(0,1,0)) { rampGoto(&rampeVitesseA, -500) ;}
    else if(Seconds == toSeconds(0,1,1)) { rampGoto(&rampeVitesseB, -500) ;}
    else if(Seconds == toSeconds(0,1,40)) { rampGoto(&rampeVitesseA, 0) ;}
    else if(Seconds == toSeconds(0,1,55)) { rampGoto(&rampeVitesseB, 0) ;}
    //else if(Seconds < toSeconds(0,1,30)) { digitalSet(LAMP1) ;}
    
}  

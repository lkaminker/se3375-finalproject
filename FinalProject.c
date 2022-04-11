#include "address_map_arm.h"

volatile int start;
volatile int time;
volatile int buttons;

volatile int *hex0 = (int *)HEX3_HEX0_BASE;
volatile int *hex1 = (int *)HEX5_HEX4_BASE;
volatile int *buts = (int *)KEY_BASE;
volatile int *timer = (int *)TIMER_BASE;
// global variables
volatile ADC * const adc_ptr = ( ADC *) ADC_BASE;
volatile GPIO * const gpio_ptr = ( unsigned int *) JP1_BASE;

// for the timer (IDK but it's from lab 1)
int Exp(int x, int y) {
    volatile int result = 1;
    while (y != 0)
    {
        result *= x;
        y--;
    }
    return result;
}

// DISPLAYS TIME
void Display(int value) {
    int decVals[10];
    decVals[0] = 0x3F;
    decVals[1] = 0x6;
    decVals[2] = 0x5B;
    decVals[3] = 0x4F;
    decVals[4] = 0x66;
    decVals[5] = 0x6D;
    decVals[6] = 0x7D;
    decVals[7] = 0x7;
    decVals[8] = 0x7F;
    decVals[9] = 0x6F;

    volatile int i;
    volatile int a = 0x0;
    volatile int b = 0x0;
    for (i = 5; i >= 0; i--) {
        int digit = (value % Exp(10, i + 1)) / (Exp(10, i));

        if (i > 3) {
            b <<= 8;
            b += decVals[digit];
        }
        else {
            a <<= 8;
            a += decVals[digit];
        }
    }
    *(hex0) = a;
    *(hex1) = b;
    return;
}

// READ BUTTONS
int ReadButs(void) {
    return *(buts)&0xF;
}

struct Timer {
    volatile int *status;
    volatile int *control;
    volatile int *loadL;
    volatile int *loadH;
    volatile int *readL;
    volatile int *readH;
};
struct Timer *myTimer;
 
// structure for the GPIO
//reads input, and lights up with respect to the voltage incrementing
typedef struct _GPIO {   
	unsigned int data;   
	unsigned int control;
} GPIO;

typedef struct _ADC {     
	unsigned int ch0;     
	unsigned int ch1;     
	unsigned int ch2;     
	unsigned int ch3;     
	unsigned int ch4;     
	unsigned int ch5;     
	unsigned int ch6;     
	unsigned int ch7; 
} ADC; 

// READ VOLTAGE FROM POTENTIOMETER
int readVoltage() {     
	int voltage;            
	// just reads 0-11 bits
	voltage = adc_ptr->ch0 & 0xfff;     
	return voltage; 
} 
	
// CONVERT VOLTAGE VALUE INTO AMOUNT OF TIME TO BE ADDED
// as the dial is turned, more time is added to the count
// set it to 13 so that there are 12 possible values (increases by 2 minutes/120 secs)
int voltageToPercent(value) {     
	int time = 0;     
	int voltAsPercent;
	// 4096 = max voltage (2^12 bits)     
	voltAsPercent = (value*13)/4096; 
	
	// 12 bit number, and reference voltage     
	if (voltAsPercent == 1) {         
		time = 10;    
	}     else if (voltAsPercent == 2) {        
		time = 20;   
	}     else if (voltAsPercent == 3) {         
		time = 30;     
	}     else if (voltAsPercent == 4) {         
		time = 40;     
	}     else if (voltAsPercent == 5) {         
		time = 50;    
	}     else if (voltAsPercent == 6) {         
		time = 60;      
	}     else if (voltAsPercent == 7) {         
		time = 70;      
	}     else if (voltAsPercent == 8) {         
		time = 80;    
    }     else if (voltAsPercent == 9) {
		time = 90;  
	}     else if (voltAsPercent == 10) {         
		time = 100;     
	}     else if (voltAsPercent == 10) {         
		time = 110;     
	}     else if (voltAsPercent == 10) {         
		time = 120;     
	} 
	return time; 
} 

// (from lab 3) check flag bit 15 to check that the channel has completed measuring voltage
int checkADC(void){     
		int bitCheck;     
		bitCheck = adc_ptr->ch0 & 0x8000;     
		return bitCheck; 
}

int main(void) {   
    // timer
    myTimer->status = timer;
    myTimer->control = timer + 1;
    myTimer->loadL = timer + 2;
    myTimer->loadH = timer + 3;
    myTimer->readL = timer + 4;
    myTimer->readH = timer + 5;

    *(myTimer->loadL) = 0x4240;
    *(myTimer->loadH) = 0xF;
    *(myTimer->control) = 0b100;

    // sets output bits (1) for 0-9 GPIO pins (will be for LED bank)
	gpio_ptr->control = 0x3FF;     
	int voltReading;     
	int setTime;

    start = 0;
    time = 0;

    while (1) {
    // ----READS POTENTIOMETER----
		// writes 1 to channel 0 to update channels (start ADC conversion)
		adc_ptr->ch0 = 0x1;

	    // checking bit 15 to see if the flag is set to 1
		if (checkADC() == 0x8000) {   
				voltReading = readVoltage();                 
				setTime = voltageToPercent(voltReading);   
                time += setTime;              
				// gpio_ptr->data = ledBank;             
	    } 
    
        // ---- DISPLAY TIME ON 7-SEGMENT DISPLAY ----
        Display(time);

        // --- READS BUTTONS ----
        buttons = ReadButs();
        // start
        if (!start && buttons == 0b0001) {
            start = 1;
        }
        // stop and reset
        if (start && buttons == 0b0010) {
            time = 0;
            start = 0;
        }

        // decrement time when start is enabled
        if ((*(myTimer->status) & 0x1) == 0b1 && start) {

            // ** THIS WILL NEED TO BE CHANGED*****
            // currently increments up and wraps around at 59secs
            // we need it to wrap down from 0 sec to 59
            if (time % 10000 == 5999) {
                time += 4001; // when it reaches 59 sec
            }

            else {
                time--;

                // LED BANK STUFF, ex:
                // interval = time/100
                // if (time == (time - interval)), then light up first LED
                // if (time == 2*(time - interval)), then light up second LED ... so on...
            }
        }
        *(myTimer->status) = 0x0;
        *(myTimer->control) = 0b100;
    }  
}

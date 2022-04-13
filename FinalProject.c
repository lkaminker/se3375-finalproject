#include "address_map_arm.h"
#include "lcd_driver.h"
#include "lcd_graphic.h"

/* Our first program!  Watch the blinking light!
*/
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

volatile GPIO * const gpio_ptr = ( unsigned int *) JP1_BASE;
volatile ADC * const adc_ptr = ( ADC *) ADC_BASE;

int readSwitches(void) {    
	volatile unsigned int * switchPtr = (unsigned int *) SW_BASE;
	volatile int switchInput;     
	switchInput = *switchPtr & 0x1;     
	return switchInput;
}

// read voltage from adc
int readVoltage(num) {     
	int voltage;     
	if (num == 0){         
		// just reads 0-11 bits
		voltage = adc_ptr->ch0 & 0xfff;     
	}     else if (num == 1) {         
		voltage = adc_ptr->ch1 & 0xfff;     
	}     
	return voltage; 
} 
	
int voltageToPercent(value, normalized) {     
	int ledOn;     
	int voltAsPercent;
	// 4096 = max voltage (2^12 bits)     
	voltAsPercent = (value*10)/normalized; 
	
	// 12 bit number, and reference voltage     
	if (voltAsPercent == 0) {         
		ledOn = 0b0;     
	}     else if (voltAsPercent == 1) {        
		 ledOn = 0b1;     
	}     else if (voltAsPercent == 2) {         
		ledOn = 0b11;     
	}     else if (voltAsPercent == 3) {         
		ledOn = 0b111;     
	}     else if (voltAsPercent == 4) {         
		ledOn = 0b1111;     
	}     else if (voltAsPercent == 5) {         
		ledOn = 0b11111;     
	}     else if (voltAsPercent == 6) {         
			ledOn = 0b111111;     
	}     else if (voltAsPercent == 7) {         
		ledOn = 0b1111111;    
    }     else if (voltAsPercent == 8) {
		         ledOn = 0b11111111;     
	}     else if (voltAsPercent == 9) {         
		ledOn = 0b111111111;     
	} else {
        ledOn = 0b1111111111;
    }
	return ledOn; 
} 

int buttonsCheck(void ){
	volatile int * button = (int*)KEY_BASE;

	int buttonValue =* button;

	return buttonValue;
}

int switchCheck(void){
	volatile int * switchPointer = (int*)SW_BASE;

	int switchValue = *switchPointer;
	switchValue &= 1;
    
	return (switchValue);
}

int checkTime(void){
	volatile int* status = (int*) TIMER_BASE;
	int timCout = *status&1;

	if(timCout==1){
		*status = 0; // 00 just write to the status 
	}

	return (timCout);
}
 
void stopTimer(){
	volatile int * control  = (int*)(TIMER_BASE + 4);
	*control = 8; // 1000 
}

void startTimer(void){
	volatile int * highPeriod = (int*)(TIMER_BASE + 12);
	volatile int * lowPeriod = (int*)(TIMER_BASE + 8);
	volatile int * control = (int*)(TIMER_BASE + 4);

	*lowPeriod = 100 * 10000;

	int val = 100 * 10000;
	*highPeriod = val>>16;

	*control = 6; // 0110
}

void showTimer(int val){
	volatile int * displayOne = (int*)HEX3_HEX0_BASE;
	volatile int * displayTwo = (int*)HEX5_HEX4_BASE;
	unsigned char map[16] = {
        0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F,
        0x77, 0x7C, 0x39, 0x5E, 0x79, 0x71};
    
    int minOne = (val/60000);
    int minTwo = ((val%60000)/6000);
    int secOne = ((val%6000)/1000);
    int secTwo = ((val%1000)/100);
    int msecOne = ((val%100)/10);
    int msecTwo = (val%10);
    
    *displayOne = (map[msecTwo] + (map[msecOne]<<8) + (map[secTwo]<<16) + (map[secOne]<<24));
    *displayTwo = (map[minTwo] + (map[minOne]<<8));	
}

int checkADC(void){     
	int bitCheck;     
	bitCheck = adc_ptr->ch0 & 1<16;     
	return bitCheck; 
}

int main() {
	int prevInstruction = 0;
	int paused = 0;
	int lapTime = 0;
	int instruction = 0;
	int countedTime = 0;

    int totalTime = 0;

    int weightRead = 0;

    gpio_ptr->control = 0x3FF;
    gpio_ptr->data = 0b0000000000;

    adc_ptr->ch1 = 0x1;
    init_spim0();
    init_lcd();

    
    while (1) 
	{
		if(checkTime() == 1){
            clear_screen();

            // takes in array of characters (can't accept string to buffer)
            char text[20] = "HOUR COMPLETE \0";
            // takes in text and row number on the screen
            LCD_text(text, 3);

            if (countedTime == 0){ //stop the count organically
                int flash = 299;
                while (flash > 1) {
                    if (checkTime() == 1){
                        flash--;
                    }
                    int flashStatus = 1;
                    gpio_ptr->data = 0b1111111111;

                    if ((flash & 50) == 0) {
                        //printf("FLASH FLASH\n");
                        //showTimer(99999);
                        if (flashStatus == 1){
                            gpio_ptr->data = 0b0000000000;
                            //showTimer(1);
                            flashStatus = 0;
                        } else {
                            gpio_ptr->data = 0b1111111111;
                            //showTimer(0);
                            flashStatus = 1;
                        }
                    }
                }
                stopTimer();
                countedTime = totalTime;
                paused = 0;
            } else {
                int ledOn = voltageToPercent(countedTime, totalTime);
                gpio_ptr->data = ledOn; 
                if(checkADC()) {
                    weightRead = readVoltage(0);
                }
                if (switchCheck() == 1 || weightRead > (4096/2)){
                    countedTime--;
                }
            }
            // cals on LCD to check the buffer and update its screen
           refresh_buffer();
		}

		instruction = buttonsCheck();

		if(((instruction&1)==1) && ((prevInstruction&1)!=1) && (paused==0)){ //count down
			startTimer();
			paused = 1;   
		}
		else if(((instruction&2)==2) && ((prevInstruction&2)!=2) && (paused==1)){ //pause count down
			stopTimer();
			paused = 0;
		}
		else if(((instruction&4)==4) && ((prevInstruction&4)!=4)) { //add time to count down
			countedTime += 100;
            totalTime += 100;
		}
		else if(((instruction&8)==8) && ((prevInstruction&8)!=8)) { //reset count down
			countedTime = 0;
			lapTime = 0;

			stopTimer();
			paused = 0;
            totalTime = 0;
		}

		prevInstruction = instruction;

		showTimer(countedTime);
    }
}

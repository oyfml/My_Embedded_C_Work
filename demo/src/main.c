/*****************************************************************************
 *   A demo example using several of the peripherals on the base board
 *
 *   Copyright(C) 2011, EE2024
 *   All rights reserved.
 *
 ******************************************************************************/
#include <stdio.h>

#include "lpc17xx_pinsel.h"

#include "lpc17xx_gpio.h"
#include "lpc17xx_i2c.h"
#include "lpc17xx_ssp.h"
#include "lpc17xx_timer.h"
#include "lpc17xx_uart.h"

#include "joystick.h"
#include "pca9532.h"
#include "acc.h"
#include "oled.h"
#include "rgb.h"
#include "led7seg.h"

#include "light.h"
#include "temp.h"
#include "string.h"

#define OBSTACLE_NEAR_THRESHOLD 3000
#define TEMP_HIGH_THRESHOLD 28
#define ACC_THRESHOLD 25.6	/*0.4g x 64 = 25.6*/

volatile uint32_t msTicks = 0;
volatile uint32_t oneSecondTicks;
volatile uint32_t ModeChangeTicks;
volatile uint32_t initTicks;
volatile uint32_t lightTicks = 0;
volatile uint32_t LEDTicks = 0;

uint8_t oled_disp[100] = {};
char clear[] = {"                "};

uint32_t light_value;
int32_t temp_value;
int8_t PressSW3 = 0;
uint32_t INTcount = 0;
int it = 1;
int click = 1;
int8_t ObsWarn = 0;

typedef enum {
    MODE_STATIONARY, MODE_FORWARD, MODE_REVERSE
} system_mode_t;
volatile system_mode_t mode;

void readForwardsensors(uint32_t* temp_value, int8_t* x, int8_t* y, int8_t* z){
  *temp_value = temp_read();
  acc_read(&*x, &*y, &*z);
}

static void printForward(uint32_t temp_value, int8_t x, int8_t y, int8_t z){
  static uint8_t OLED_TEMP[15];
  static uint8_t OLED_X[15];
  static uint8_t OLED_Y[15];
  static uint8_t OLED_Z[15];

  if(it == 0){
  //Clear screen
	  oled_putString(0, 30, clear, OLED_COLOR_WHITE, OLED_COLOR_WHITE);
	  oled_putString(0, 40, clear, OLED_COLOR_WHITE, OLED_COLOR_WHITE);
	  oled_putString(0, 50, clear, OLED_COLOR_WHITE, OLED_COLOR_WHITE);
  }
  sprintf(OLED_TEMP, "Temp=%2.1f", temp_value/10.0);
  sprintf(OLED_X, "X=%1.1f", x/64.0);
  sprintf(OLED_Y, "Y=%1.1f", y/64.0);
  sprintf(OLED_Z, "Z=%1.1f", z/64.0);

  oled_putString(35, 30, (uint8_t *) OLED_TEMP, OLED_COLOR_BLACK, OLED_COLOR_WHITE);
  oled_putString(0, 30, (uint8_t *) OLED_X, OLED_COLOR_BLACK, OLED_COLOR_WHITE);
  oled_putString(0, 40, (uint8_t *) OLED_Y, OLED_COLOR_BLACK, OLED_COLOR_WHITE);
  oled_putString(0, 50, (uint8_t *) OLED_Z, OLED_COLOR_BLACK, OLED_COLOR_WHITE);
}

void incrementLED(uint32_t light){
int8_t x = 0;
uint16_t LedArr[15] = {0x01, 0x03, 0x07, 0xF, 0x1F, 0x3F, 0x7F, 0xFF, 0x1FF, 0x3FF, 0x7FF, 0xFFF, 0x1FFF, 0x3FFF, 0x7FFF};

	if(light < 200){
		x = 0;
	}
	if((light > 200) &&(light < 400)){
		x = 1;
	}
	if((light > 400) &&(light < 600)){
		x = 2;
  	}
	if((light > 600) &&(light < 800)){
		x = 3;
	}
	if((light > 800) &&(light < 1000)){
		x = 4;
	}
	if((light > 1000) &&(light < 1200)){
		x = 5;
	}
	if((light > 1200) &&(light < 1400)){
		x = 6;
	}
	if((light > 1400) &&(light < 1600)){
		x = 7;
	}
	if((light > 1600) &&(light < 1800)){
		x = 8;
	}
	if((light > 1800) &&(light < 2000)){
		x = 9;
	}
	if((light > 2000) &&(light < 2200)){
		x = 10;
	}
	if((light > 2200) &&(light < 2400)){
		x = 11;
	}
	if((light > 2400) &&(light < 2600)){
		x = 12;
	}
	if((light > 2600) &&(light < 2800)){
		x = 13;
	}
	if((light > 2800) &&(light < 3000)){
		x = 14;
	}

	 pca9532_setLeds(LedArr[x], 0xFFFF);

}

void setBlinky(int8_t Red, int8_t Blue) {

	if((Red == 0)&&(Blue == 0)){
		GPIO_ClearValue(2, (1 << 0));
		GPIO_ClearValue(0, (1 << 26));
	}
	if((Red == 1)&&(Blue == 0)){
		GPIO_ClearValue(0, (1 << 26));/*Blue off*/
		if(click == 1){
			GPIO_SetValue(2, (1 << 0));
		} else {
			GPIO_ClearValue(2, (1 << 0));
		}
	}
	if((Red == 0)&&(Blue == 1)){
		GPIO_ClearValue(2, (1 << 0));/*Red off*/
		if(click == 1){
			GPIO_SetValue(0, (1 << 26));
		}else {
			GPIO_ClearValue(0, (1 << 26));
		}
	}
	if((Red == 1)&&(Blue == 1)){
		if(click == 1){
			GPIO_SetValue(2, (1 << 0));
			GPIO_SetValue(0, (1 << 26));
		} else {
			GPIO_ClearValue(2, (1 << 0));
			GPIO_ClearValue(0, (1 << 26));
		}
	}
}

static void init_ssp(void)
{
	SSP_CFG_Type SSP_ConfigStruct;
	PINSEL_CFG_Type PinCfg;

	/*
	 * Initialize SPI pin connect
	 * P0.7 - SCK;
	 * P0.8 - MISO
	 * P0.9 - MOSI
	 * P2.2 - SSEL - used as GPIO
	 */
	PinCfg.Funcnum = 2;
	PinCfg.OpenDrain = 0;
	PinCfg.Pinmode = 0;
	PinCfg.Portnum = 0;
	PinCfg.Pinnum = 7;
	PINSEL_ConfigPin(&PinCfg);
	PinCfg.Pinnum = 8;
	PINSEL_ConfigPin(&PinCfg);
	PinCfg.Pinnum = 9;
	PINSEL_ConfigPin(&PinCfg);
	PinCfg.Funcnum = 0;
	PinCfg.Portnum = 2;
	PinCfg.Pinnum = 2;
	PINSEL_ConfigPin(&PinCfg);

	SSP_ConfigStructInit(&SSP_ConfigStruct);

	// Initialize SSP peripheral with parameter given in structure above
	SSP_Init(LPC_SSP1, &SSP_ConfigStruct);

	// Enable SSP peripheral
	SSP_Cmd(LPC_SSP1, ENABLE);

}

static void init_i2c(void)
{
	PINSEL_CFG_Type PinCfg;

	/* Initialize I2C2 pin connect */
	PinCfg.Funcnum = 2;
	PinCfg.Pinnum = 10;
	PinCfg.Portnum = 0;
	PINSEL_ConfigPin(&PinCfg);
	PinCfg.Pinnum = 11;
	PINSEL_ConfigPin(&PinCfg);

	// Initialize I2C peripheral
	I2C_Init(LPC_I2C2, 100000);

	/* Enable I2C1 operation */
	I2C_Cmd(LPC_I2C2, ENABLE);
}

static void init_GPIO(void)
{
	// Initialize button
	/* Line 261 */	PINSEL_CFG_Type PinCfg;
	/* Line 262 */
	/* Line 263 */	    PinCfg.Funcnum = 0;
	/* Line 264 */	    PinCfg.OpenDrain = 0;
	/* Line 265 */	    PinCfg.Pinmode = 0;
	/* Line 266 */	    PinCfg.Portnum = 1;
	/* Line 267 */	    PinCfg.Pinnum = 31;
	/* Line 268 */
	/* Line 269 */	PINSEL_ConfigPin(&PinCfg);//SW4

						PinCfg.Portnum = 0;
						PinCfg.Pinnum = 4;

						PINSEL_ConfigPin(&PinCfg);//SW3 P0.4


	/* Line 271 */	GPIO_SetDir(1, 1<<31, 0);

					GPIO_SetDir(0, 1<<4, 0);

}

void ready_uart(void)
{
	// PINSEL Configuration
	PINSEL_CFG_Type CPin;
	    CPin.OpenDrain = 0;
	    CPin.Pinmode = 0;
	    CPin.Funcnum = 2;
	    CPin.Pinnum = 0;
	    CPin.Portnum = 0;
	PINSEL_ConfigPin(&CPin);
	    CPin.Pinnum = 1;
	    CPin.Portnum = 0;
	PINSEL_ConfigPin(&CPin);

	// Initialise and enable the UART. Not enabling the UART will lead to a hard fault
	UART_CFG_Type UCfg;
	    UCfg.Baud_rate = 115200;
	    UCfg.Databits = UART_DATABIT_8;
	    UCfg.Parity = UART_PARITY_NONE;
	    UCfg.Stopbits = UART_STOPBIT_1;

	// supply power & setup working parameters for UART3
	UART_Init(LPC_UART3, &UCfg);

	// enable transmit for uart3
	UART_TxCmd(LPC_UART3, ENABLE);

	// FIFO configuration- For system enhancements only
	//
}

void SysTick_Handler(void)
{
	msTicks++;
}

static uint32_t getTicks(void)
{
	return msTicks;
}

//EINT3 Interrupt Handler, GPIO0 and GPIO2 interrupts share the same position in NVIC with EINT3 - pg 24 GPIO notes
void EINT3_IRQHandler(void) {

	//Switch 3 interrupt
	if ((LPC_GPIOINT->IO0IntStatF >> 4) & 0x01) {
		PressSW3++;/*Increments whenever switch 3 is pressed*/
		if(INTcount == 0){
			ModeChangeTicks = msTicks;
		}
		INTcount++;/*Resets to 0 after 1 second*/
		LPC_GPIOINT->IO0IntClr = (1 << 4);
	}
	//Light sensor interrupt
	if ((LPC_GPIOINT->IO2IntStatF >> 5) & 0x01) {
		ObsWarn = 1;
		LPC_GPIOINT->IO2IntClr = (1 << 5);
	}

}

int main (void) {

    int32_t xoff = 0;
    int32_t yoff = 0;
    int32_t zoff = 0;

    int8_t x = 0;
    int8_t y = 0;
    int8_t z = 0;

    init_i2c();
    init_ssp();
    init_GPIO();
    ready_uart();

    pca9532_init();
    joystick_init();
    acc_init();
    oled_init();

    led7seg_init();
    light_enable();
	temp_init(&getTicks);
	rgb_init();

	light_setRange(LIGHT_RANGE_4000);
	light_setLoThreshold(0);
	light_setHiThreshold(OBSTACLE_NEAR_THRESHOLD);
	light_setIrqInCycles(LIGHT_CYCLE_1);
	light_clearIrqStatus();

//	//Set Interrupt Priority
	NVIC_SetPriorityGrouping(4); //Preempt Priority Bit[7:5], Sub Priority Bit[4:3]
	NVIC_SetPriority(SysTick_IRQn,NVIC_EncodePriority(4,0,0));//Preempt 0, Sub 0
	NVIC_SetPriority(EINT3_IRQn,NVIC_EncodePriority(4,1,0));//Preempt 1, Sub 0
	//Initialize SW3 interrupt
	LPC_GPIOINT->IO0IntEnF |= 1 << 4;
	//Initialize LED interrupt
	LPC_GPIOINT->IO2IntEnF |= 1 << 5;
	NVIC_ClearPendingIRQ(EINT3_IRQn);
	NVIC_EnableIRQ(EINT3_IRQn);

  /*Assume base board in zero-g position when reading first value.*/
    acc_read(&x, &y, &z);
    xoff = 0-x;
    yoff = 0-y;
    zoff = 64-z;

    /* ---- Speaker ------> */

    GPIO_SetDir(2, 1<<0, 1);
    GPIO_SetDir(2, 1<<1, 1);

    GPIO_SetDir(0, 1<<27, 1);
    GPIO_SetDir(0, 1<<28, 1);
    GPIO_SetDir(2, 1<<13, 1);
    GPIO_SetDir(0, 1<<26, 1);

    GPIO_ClearValue(0, 1<<27); //LM4811-clk
    GPIO_ClearValue(0, 1<<28); //LM4811-up/dn
    GPIO_ClearValue(2, 1<<13); //LM4811-shutdn

    /* <---- Speaker ------ */


    oled_clearScreen(OLED_COLOR_WHITE);

  	SysTick_Config(SystemCoreClock / 1000);

    mode = MODE_STATIONARY;
    oneSecondTicks = msTicks;

    int ActiveSW3 = 0;
    uint8_t ActivateREVERSE = 0;
    int ReturnMODE = 0;

    uint8_t ArrPointer = 0;
    uint8_t SevenSEG[16] = {0x24, 0x7D, 0xE0, 0x70, 0x39, 0x32, 0x22, 0x7C, 0x20, 0x30, 0x28, 0x20, 0xA6, 0x24, 0xA2, 0xAA};
    //                        0     1     2     3     4     5     6     7     8     9     A     B     C     D     E     F

    int8_t Red = 0;
    int8_t Blue = 0;
    int8_t PrintAirBOnce = 1;
    int8_t PrintTempOnce = 1;
    int8_t PrintObsOnce = 1;

    static char* msg = NULL;
    unsigned char result[100] = "";
    int32_t UARTnum = 0;

    while (1)
    {

      if(PressSW3 == 1){
    	  if(mode == MODE_STATIONARY){ /*Active for 1 second*/
    		  ActiveSW3 = 1;
    	  }
    	  if((mode == MODE_FORWARD)||(mode == MODE_REVERSE)){
    		  ReturnMODE = 1;
    	  }
      }

      if((msTicks - ModeChangeTicks) < 1000){
    	  if ((ActiveSW3 == 1)&&(mode == MODE_STATIONARY)) { /*Switch 3 is pressed,awaiting next action*/
    		  if(PressSW3 == 1){
    			  ActivateREVERSE = 10;
    		  }
    		  if(PressSW3 > 1){
    			  ActivateREVERSE = 1;
    		  }
    	  }
      }

      else if((msTicks - ModeChangeTicks) >= 1000){ /*After 1 second, change mode*/
    	  INTcount = 0;
    	  if((ActiveSW3 == 1)&&(mode == MODE_STATIONARY)){
    		  if(ActivateREVERSE == 1){
    			  mode = MODE_REVERSE;
    			  oled_clearScreen(OLED_COLOR_WHITE);
    			  ActivateREVERSE = 0;
    			  PressSW3 = 0;
    			  ActiveSW3 = 0;
    			  it = 1;/*First iteration enabled*/
    			  PrintObsOnce = 1;
    		  }
    		  if(ActivateREVERSE == 10){
    			  mode = MODE_FORWARD;
    			  oled_clearScreen(OLED_COLOR_WHITE);
    			  ActivateREVERSE = 0;
    			  PressSW3 = 0;
    			  ActiveSW3 = 0;
    			  it = 1;/*First iteration enabled*/
    			  oneSecondTicks = msTicks;/*Start Timer*/
    			  PrintTempOnce = 1;
    			  PrintAirBOnce = 1;
    			  Red = 0;
    			  Blue = 0;
    		  }
    	  }
      }
      if((mode == MODE_REVERSE)||(mode == MODE_FORWARD)){ /*Return to Stationary mode within 1 second*/
        if(ReturnMODE == 1){
          mode = MODE_STATIONARY;
          oled_clearScreen(OLED_COLOR_WHITE);
          ReturnMODE = 0;
          PressSW3 = 0;
          ArrPointer = 0;/*Reset Forward 7segment pointer*/
          it = 1;/*First iteration enabled*/
          ObsWarn = 0;
          light_clearIrqStatus();
        }
      }

   switch (mode) {
   	case MODE_STATIONARY:

        //First iteration (Run once)
   		if(it == 1){

			//Display Oled
			sprintf(oled_disp,"Stationary Mode ");
			oled_putString(0,0,(uint8_t *) oled_disp, OLED_COLOR_BLACK, OLED_COLOR_WHITE);
			//7segment display off
			led7seg_setChar(0xFF, TRUE);
			//Red Blue LED off
			GPIO_ClearValue(2, (1 << 0));
			GPIO_ClearValue(0, (1 << 26));
			//16 LED off
			pca9532_setLeds(0x0000,0xffff);

			it = 0;
   		}
    			break;

   	case MODE_FORWARD:

        //First iteration (Run once)
        if(it == 1){

            //Display Oled
            sprintf(oled_disp,"Forward Mode    ");
            oled_putString(0,0,(uint8_t *) oled_disp, OLED_COLOR_BLACK, OLED_COLOR_WHITE);
    		//First Sample & Print
        	led7seg_setChar(SevenSEG[ArrPointer], TRUE);
    		readForwardsensors(&temp_value, &x, &y, &z); /*Read values from temperature sensor and accelerometer*/
    		x = x + xoff;
    		y = y + yoff;
    		z = z + zoff;
    		printForward(temp_value, x, y ,z);
    		ArrPointer++;
    		// Send string message from the LPC1769 to the UART terminal
	    	msg = "Entering Forward Mode.\r\n";
	    	UART_Send(LPC_UART3, (uint8_t *)msg , strlen(msg), BLOCKING);
	    	UARTnum++;

    		it = 0;
        }else{
        //After first iteration

        	//Every 1 second onwards
			if ((msTicks - oneSecondTicks) >= 1000) {
				oneSecondTicks = msTicks;
			//7segment Display
				led7seg_setChar(SevenSEG[ArrPointer], TRUE);
			//Sampling temperature sensor and accelerometer once every second
				readForwardsensors(&temp_value, &x, &y, &z); /*Read values from temperature sensor and accelerometer*/
				x = x + xoff;
				y = y + yoff;
				z = z + zoff;

			//Print values on OLED every 5, A, F
				if((ArrPointer == 5)||(ArrPointer == 10)||(ArrPointer == 15)){
					printForward(temp_value, x, y ,z);
				}
			//Send UART values every F
				if(ArrPointer == 15){
					sprintf(result, "%03d_Temp_%2.2f_ACC_%2.2f\r\n", UARTnum, temp_value/10.0, x/64.0);
					UART_Send(LPC_UART3, (uint8_t *) result, strlen(result), BLOCKING);
					UARTnum++;
					// Send Warning to UART if BLINK RED/BLUE
					if(Red == 1){
						msg = "Temperature too high.\r\n";
						UART_Send(LPC_UART3, (uint8_t *)msg , strlen(msg), BLOCKING);
						UARTnum++;
					}
					if(Blue == 1){
						msg = "Collision has been detected.\r\n";
						UART_Send(LPC_UART3, (uint8_t *)msg , strlen(msg), BLOCKING);
						UARTnum++;
					}

				}
				ArrPointer++;/*7seg array pointer increment & reset*/
				if(ArrPointer > 15){
					ArrPointer = 0;
				}
			}

			//Temperature too high
			if((temp_value/10.0) >= (TEMP_HIGH_THRESHOLD)) {
				Red = 1;/*Red LED blink*/
				if(PrintTempOnce == 1){
					sprintf(oled_disp,"Temp. too high ");
					oled_putString(0,10,(uint8_t *) oled_disp,OLED_COLOR_BLACK , OLED_COLOR_WHITE);
					PrintTempOnce = 0;
				}
			}

			//Collision Detected
			if((x >= ACC_THRESHOLD)||(x <= -ACC_THRESHOLD)){ /*64 = 1g*/
				Blue = 1;/*Blue LED blink*/
				if(PrintAirBOnce == 1){
					sprintf(oled_disp,"Airbag released");
					oled_putString(0,20,(uint8_t *) oled_disp,OLED_COLOR_BLACK , OLED_COLOR_WHITE);
					PrintAirBOnce = 0;
				}
			}

			//Toggle bit sequence every 333ns
			if((msTicks - LEDTicks)>= 333){
				LEDTicks = msTicks;
				click = -click;
				setBlinky(Red, Blue);
			}

        }
    			break;

    case MODE_REVERSE:

        //First iteration (Run once)
   		if(it == 1){

			//Display Oled
			sprintf(oled_disp,"Reverse Mode    ");
			oled_putString(0,0,(uint8_t *) oled_disp, OLED_COLOR_BLACK, OLED_COLOR_WHITE);
			//7segment display off
			led7seg_setChar(0xFF, TRUE);
			//Red Blue LED off
			GPIO_ClearValue(2, (1 << 0));
			GPIO_ClearValue(0, (1 << 26));
			//16 LED off
			pca9532_setLeds(0x0000,0xffff);
			// Send string message from the LPC1769 to the UART terminal
	    	msg = "Entering Reverse Mode.\r\n";
	    	UART_Send(LPC_UART3, (uint8_t *)msg , strlen(msg), BLOCKING);
	    	UARTnum++;

			it = 0;
   		}else{
   		//After first iteration

   			//Sampling light sensor
   			if ((msTicks - lightTicks) >= 1000) { /*Sample once every second*/
   				lightTicks = msTicks;
   				light_value = light_read();
   			}

   			if(ObsWarn == 1){
				if(PrintObsOnce == 1){
					light_clearIrqStatus();
					sprintf(oled_disp,"Obstacle near");
					oled_putString(0,20,(uint8_t *) oled_disp,OLED_COLOR_BLACK , OLED_COLOR_WHITE);
					msg = "Obstacle near.\r\n";
					UART_Send(LPC_UART3, (uint8_t *)msg , strlen(msg), BLOCKING);
					UARTnum++;
					pca9532_setLeds(0xFFFF, 0x0000);
					PrintObsOnce = 0;
				}
			}else{
   				//Turn on led array based on light sensor value (too HIGH - turn on LED, too LOW - turn off LED)
					incrementLED(light_value);

   			}
   		}
    			break;

    }
    }
}

void check_failed(uint8_t *file, uint32_t line)
{
	/* User can add his own implementation to report the file name and line number,
	 ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

	/* Infinite loop */
	while(1);
}


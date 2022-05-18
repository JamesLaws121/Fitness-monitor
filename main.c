/**********************************************************
 * ENCE361: Personal Fitness monitor project
 *
 * main.c
 *
 * D. Beukenholdt, J. Laws
 * Last modified: 18 May 2022
 *
 **********************************************************/


#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_ints.h"
#include "inc/hw_i2c.h"
#include "driverlib/pin_map.h"
#include "driverlib/systick.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/i2c.h"
#include "../OrbitOLED/OrbitOLEDInterface.h"
#include "utils/ustdlib.h"
#include "inc/hw_ints.h"
#include "driverlib/adc.h"
#include "driverlib/debug.h"
#include "utils/ustdlib.h"


#include "acc.h"
#include "i2c_driver.h"
#include "buttons4.h"
#include "circBufT.h"
#include "ADC.h"
#include "display.h"



//Interrupt constants
#define SYSTICK_RATE_HZ 30
#define DISPLAY_UPDATE_HZ 4
#define USER_INPUT_RATE_HZ 10
#define STEP_INPUT_RATE_HZ 6
#define ACC_UPDATE_HZ 15



//LED constants
#define LED1 GPIO_PIN_6
#define LED2 GPIO_PIN_7


//Define global variables
uint16_t step_count;
uint16_t step_goal;
int16_t step_cooldown = 20;
uint16_t magnitude_sum;
uint16_t average_magnitude;
uint8_t averages_counted = 1;



//Flags
bool display_update_flag = 0;
bool user_input_flag = 0;
bool step_update_flag = 0;
bool acc_input_flag = 0;




//====================================================================================
// SysTick interrupt handler
//====================================================================================
void SysTickIntHandler(void)
{
    static uint8_t user_input_delay = (SYSTICK_RATE_HZ / USER_INPUT_RATE_HZ);
    static uint8_t display_update_delay = SYSTICK_RATE_HZ / DISPLAY_UPDATE_HZ + 1; //+1 offset to prevent display update and user input being called in the same tick
    static uint8_t step_update_delay = (SYSTICK_RATE_HZ / STEP_INPUT_RATE_HZ);
    static uint8_t acc_update_delay = SYSTICK_RATE_HZ / ACC_UPDATE_HZ;

    // Trigger an ADC conversion for potentiometer
    ADCProcessorTrigger(ADC0_BASE, 3);

    // get accelerometer data
    acc_update_delay--;
    if (acc_update_delay == 0) {
        acc_input_flag = 1;
        acc_update_delay = SYSTICK_RATE_HZ / ACC_UPDATE_HZ;
    }


    //Set scheduling flags
    display_update_delay--;
    if (display_update_delay == 0) {
        display_update_flag = 1;
        display_update_delay = SYSTICK_RATE_HZ / DISPLAY_UPDATE_HZ;
    }

    user_input_delay--;
    if (user_input_delay == 0){
        user_input_flag = 1;
        user_input_delay = SYSTICK_RATE_HZ / USER_INPUT_RATE_HZ;
    }

    step_update_delay--;
    if(step_update_delay == 0){
        step_update_flag = 1;
        step_update_delay = SYSTICK_RATE_HZ / USER_INPUT_RATE_HZ;
    }

}




// Main
int main()
{
    //================================================================================
    // Setup Code (runs once)
    //================================================================================

    // Initial values for variables
    step_count = 0;
    step_goal = 0;


    // Initialize system clock
    SysCtlClockSet(SYSCTL_SYSDIV_10 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN |SYSCTL_XTAL_16MHZ);

    // Initialize required modules
    initAccl();
    displayInit();
    OLEDInitialise();
    initButtons();
    initADC();
    switchInit();

    // Set up the period for the SysTick timer
    SysTickPeriodSet(SysCtlClockGet() / SYSTICK_RATE_HZ);

    // Register the interrupt handler
    SysTickIntRegister(SysTickIntHandler);

    // Enable interrupts
    SysTickIntEnable();
    SysTickEnable();
    IntMasterEnable();


    //=========================================================================================
    // Main Loop
    //=========================================================================================
    while (1)
    {

        if(user_input_flag == 1){
            //Get input from user and take some action
            updateButtons();
            processUserInput(&step_count,&step_goal);
            user_input_flag = 0;
        }

        if(display_update_flag == 1){
            //Update the display
            displayUpdate(&step_count,&step_goal);
            display_update_flag = 0;
        }

        if(step_update_flag == 1){
            if(checkBump()) {
                step_count++;
            }
            step_update_flag = 0;
        }

        if(acc_input_flag == 1){
            updateAccBuffers();
            acc_input_flag = 0;
        }
    }
}
































/**********************************************************
 * ENCE361: Personal Fitness monitor project
 *
 * main.c
 *
 * D. Beukenholdt, J. Laws
 * Last modified: 11 May 2022
 *
 **********************************************************/


#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

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
//#include "stdlib.h"

#include "acc.h"
#include "i2c_driver.h"
#include "buttons4.h"
#include "circBufT.h"
#include "ADC.h"

//Define constants
#define STEP_DISTANCE 0.9
#define SYSTICK_RATE_HZ 30

//Switch constants
#define SW1_BUT_PERIPH  SYSCTL_PERIPH_GPIOA
#define SW1_BUT_PORT_BASE  GPIO_PORTA_BASE
#define SW1_BUT_PIN  GPIO_PIN_7
#define SW1_BUT_NORMAL  false


//Define global variables
uint8_t display_state;
uint16_t step_count;
uint16_t step_goal;

enum step_units{STEPS=0, PERCENT=1} step_unit;
enum dist_units{KILOMETRES=0, MILES=1} dist_unit;

bool updtBtnFlag = true;
bool updtDispFlag = true;
bool updtUsrProcFlag = true;
bool updtAccFlag = true;
bool updtAvgFlag = true;
bool updtChkBmpFlag = true;



//=====================================================================================
// lineUpdate: Updates a single line on the OLED display.
//=====================================================================================
void lineUpdate(char *str1, uint16_t num, char *str2, uint8_t charLine)
{
    char text_buffer[17]; //Display fits 16 characters wide.

    // Clear the line to be updated.
    OLEDStringDraw("                ", 0, charLine);

    // Draw a new string to the display.
    usnprintf(text_buffer, sizeof(text_buffer), "%s %3u %s", str1, num, str2);
    OLEDStringDraw(text_buffer, 0, charLine);
}


//====================================================================================
// displayUpdate: Updates the OLED display.
//====================================================================================
void displayUpdate(void)
{
    // If a state change has occurred, clear the display.
    static uint8_t prev_state = 0;
    if ((display_state != prev_state) || checkLongPush(NUM_BUTS)) {
        OLEDStringDraw("                ", 0,0);
        OLEDStringDraw("                ", 0,1);
        OLEDStringDraw("                ", 0,2);
        OLEDStringDraw("                ", 0,3);
    }
    prev_state = display_state;

    // Causes the display to blank for 1 frame if long push has occurred
    if (checkLongPush(NUM_BUTS)) {return;}

    // Update the display based on the current state.
    switch (display_state)
    {
    case 0:
        //0: Display current step count
        OLEDStringDraw("Step Count     ",0,0);
        if (step_unit == STEPS) {
            lineUpdate("", step_count, "steps", 1);

        }
        else if (step_unit == PERCENT) {
            uint8_t step_percent = (step_count * 100) / step_goal;
            lineUpdate("", step_percent, "%", 1);
        }
        break;

    case 1:
        //1: Display the current distance traveled
        OLEDStringDraw("Total Distance",0,0);
        OLEDStringDraw("                ", 0, 1);
        uint16_t distance = (step_count*STEP_DISTANCE);
        char text_buffer[17]; //Display fits 16 characters wide.

        if (dist_unit == KILOMETRES) {
            usnprintf(text_buffer, sizeof(text_buffer), " %d.%d km", (distance/1000), (distance % 1000)/10);
            OLEDStringDraw(text_buffer, 0, 1);
        }
        else if (dist_unit == MILES) {
            usnprintf(text_buffer, sizeof(text_buffer), " %d.%d miles", (distance/1609), (((distance*1000)/1609) % 1000)/10);
            OLEDStringDraw(text_buffer, 0, 1);
        }
        break;

    case 2:
        //2: Display the current goal
        OLEDStringDraw("Set Step Goal ",0,0);
        lineUpdate(" ", (getADCMean() / 100)*100, "steps", 1);
        break;

    default:
        //Invalid display_state: Blank screen
        OLEDStringDraw("                ", 0,0);
        OLEDStringDraw("                ", 0,1);
        OLEDStringDraw("                ", 0,2);
        OLEDStringDraw("                ", 0,3);
        break;
    }
}


//===================================================================================
// processUserInput: takes some action based on some user input.
//===================================================================================
void processUserInput(void)
{
    //Inputs to check regardless of state and test mode
    // left and right buttons should move between different screens
    if (checkButton(LEFT) == PUSHED) {
        if (display_state < 2) {
            display_state += 1;
        } else {
            display_state = 0;
        }
    }

    if (checkButton(RIGHT) == PUSHED) {
        if (display_state > 0) {
            display_state -= 1;
        } else {
            display_state = 2;
        }
    }


    // Test Mode
    if((GPIOPinRead (SW1_BUT_PORT_BASE, SW1_BUT_PIN) == SW1_BUT_PIN)){
        OLEDStringDraw("TEST MODE    ",0,3);
        if (checkButton(UP) == PUSHED) {
            step_count += 100;
        }
        if (checkButton(DOWN) == PUSHED) {
            step_count -= 500;
        }
        return;
    } else{
        OLEDStringDraw("                ", 0,3);
    }


    // State dependent inputs
    switch (display_state)
    {
    case 0: //0: Displaying total steps
        //UP: Toggle units between absolute steps and percentage of goal
        if (checkButton(UP) == PUSHED) {
            step_unit = !step_unit;
        }

        //LONG DOWN: Reset step count
        if (checkLongPush(DOWN) == 1){
            step_count = 0;
        }

        break;

    case 1: //1: Displaying total distance
        //UP: Toggle units between km and miles
        if (checkButton(UP) == PUSHED) {
            dist_unit = !dist_unit;
        }

        //LONG DOWN: Reset step count
        if (checkLongPush(DOWN) == 1){
            step_count = 0;
        }

        break;

    case 2: //2: Displaying current goal
        //Potentiometer: Change displayed step goal
        // This is currently being handled in ADC.c

        //DOWN: Commit step goal
        if (checkButton(DOWN) == PUSHED) {
            step_goal = (getADCMean() / 100)*100;
            display_state = 0;
        }
        break;

    default:
        break;
    }
}





//====================================================================================
// switchInit:
//====================================================================================
void switchInit(){
    SysCtlPeripheralEnable(SW1_BUT_PERIPH);
    GPIOPinTypeGPIOInput(SW1_BUT_PORT_BASE, SW1_BUT_PIN);
    GPIOPadConfigSet(SW1_BUT_PORT_BASE, SW1_BUT_PIN, GPIO_STRENGTH_2MA,GPIO_PIN_TYPE_STD_WPD);
}



//====================================================================================
// SysTick interrupt handler
//====================================================================================
void SysTickIntHandler(void)
{
    // Trigger an ADC conversion
    ADCProcessorTrigger(ADC0_BASE, 3);

}

void checkBump(){
    //
}

// Main
int main()
{
    //================================================================================
    // Setup Code (runs once)
    //================================================================================
    //orientation_t orientation;

    //vector3_t accl_data;
    vector3_t currentAverage;

    // Initial values for variables
    step_unit = STEPS;
    dist_unit = KILOMETRES;
    step_count = 1500;
    step_goal = 1700;
    display_state = 0;

    // Initialize system clock
    SysCtlClockSet(SYSCTL_SYSDIV_10 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN |SYSCTL_XTAL_16MHZ);

    // Initialize required modules
    initAccl();
    OLEDInitialise();
    initButtons();
    initADC();
    switchInit();

    // Set up the period for the SysTick timer.  The SysTick timer period is
    // set as a function of the system clock.
    SysTickPeriodSet(SysCtlClockGet() / SYSTICK_RATE_HZ);
    //
    // Register the interrupt handler
    SysTickIntRegister(SysTickIntHandler);
    //
    // Enable interrupt and device
    SysTickIntEnable();
    SysTickEnable();


    //allows interrupts
    IntMasterEnable();

    // Setup circular buffer for accelerometer data


    // Obtain initial set of accelerometer data
    uint8_t i;
    for(i = 0; i < 20; i++){
        updateAccBuffers();
    }
    currentAverage = getAverage();



    //=========================================================================================
    // Main Loop
    //=========================================================================================
    while (1)
    {
        // Set the program speed using a magic number
        // TODO: improve main loop to use SYSTICK interrupts for timing or something similar
        SysCtlDelay(SysCtlClockGet () / 32); //delay(s) = 3/magic number

        currentAverage = getAverage();
        //Gets acc data and places in buffers
        updateAccBuffers();

        if(updtChkBmpFlag){
            // checks for user steps
            checkBump();
        }

        if(updtBtnFlag){
            // Check buttons for user input
            updateButtons();
        }

        if(updtUsrProcFlag){
            //Take actions bases on user input
            processUserInput();
        }

        if(updtDispFlag){
            //Update the display
            displayUpdate();
        }


    }
}
































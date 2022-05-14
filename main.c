/**********************************************************
 * ENCE361: Personal Fitness monitor project
 *
 * main.c
 *
 * D. Beukenholdt, J. Laws
 * Last modified: 12 May 2022
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
#define DISPLAY_UPDATE_HZ 6
#define USER_INPUT_RATE_HZ 12
#define STEP_INPUT_RATE_HZ 8

//Switch constants
#define SW1_BUT_PERIPH  SYSCTL_PERIPH_GPIOA
#define SW1_BUT_PORT_BASE  GPIO_PORTA_BASE
#define SW1_BUT_PIN  GPIO_PIN_7
#define SW1_BUT_NORMAL  false

//LED constants
#define LED1 GPIO_PIN_6
#define LED2 GPIO_PIN_7


//Define global variables
uint8_t display_state;
uint16_t step_count;
uint16_t step_goal;

enum step_units{STEPS=0, PERCENT=1} step_unit;
enum dist_units{KILOMETRES=0, MILES=1} dist_unit;

//Flags
bool display_update_flag = 0;
bool user_input_flag = 0;
bool skip_frame_flag = 0;
bool step_update_flag = 0;



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
    if ((display_state != prev_state) || skip_frame_flag) {
        OLEDStringDraw("                ", 0,0);
        OLEDStringDraw("                ", 0,1);
        OLEDStringDraw("                ", 0,2);
        OLEDStringDraw("                ", 0,3);
    }
    prev_state = display_state;

    // Leave the display blank and reset skip_frame_flag if skip_frame_flag is true
    if (skip_frame_flag) {
        skip_frame_flag = 0;
        return;
    }

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
        OLEDStringDraw("Current Goal ",0,2);
        lineUpdate(" ", step_goal, "steps", 3);
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

    // Acknowledge a long push of any button
    if (checkLongPush(NUM_BUTS)) {skip_frame_flag = 1;}

    // Test Mode
    if((GPIOPinRead (SW1_BUT_PORT_BASE, SW1_BUT_PIN) == SW1_BUT_PIN)){
        GPIOPinWrite(GPIO_PORTC_BASE, LED1, LED1);
        GPIOPinWrite(GPIO_PORTC_BASE, LED2, LED2);
        if (checkButton(UP) == PUSHED) {
            step_count += 100;
        }
        if (checkButton(DOWN) == PUSHED) {
            step_count -= 500;
        }
        return;
    } else{
        GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_6, 0x00);
        GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_7, 0x00);
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
// switchInit: Initializes the debug switch
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
    static uint8_t user_input_delay = (SYSTICK_RATE_HZ / USER_INPUT_RATE_HZ);
    static uint8_t display_update_delay = SYSTICK_RATE_HZ / DISPLAY_UPDATE_HZ + 1; //+1 offset to prevent display update and user input being called in the same tick
    static uint8_t step_update_delay = (SYSTICK_RATE_HZ / STEP_INPUT_RATE_HZ);

    // Trigger an ADC conversion for potentiometer
    ADCProcessorTrigger(ADC0_BASE, 3);

    // get accelerometer data
    //updateAccBuffers();

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

void checkBump(){
    vector3_t currentAverage = getAverage();


    lineUpdate("", currentAverage.x, "x", 1);
    lineUpdate("", currentAverage.y, "y", 2);
    lineUpdate("", currentAverage.z, "z", 3);
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

    // Set up the period for the SysTick timer
    SysTickPeriodSet(SysCtlClockGet() / SYSTICK_RATE_HZ);

    // Register the interrupt handler
    SysTickIntRegister(SysTickIntHandler);

    // Enable interrupts
    SysTickIntEnable();
    SysTickEnable();
    IntMasterEnable();

    // Obtain initial set of accelerometer data
    uint8_t i;
    for(i = 0; i < 20; i++){
        updateAccBuffers();
    }

    currentAverage = getAverage();

    // Initialize LEDs
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);
    GPIOPadConfigSet(GPIO_PORTC_BASE, LED1, GPIO_STRENGTH_4MA, GPIO_PIN_TYPE_STD_WPD);
    GPIOPadConfigSet(GPIO_PORTC_BASE, LED2, GPIO_STRENGTH_4MA, GPIO_PIN_TYPE_STD_WPD);
    GPIODirModeSet(GPIO_PORTC_BASE, LED1, GPIO_DIR_MODE_OUT);
    GPIODirModeSet(GPIO_PORTC_BASE, LED2, GPIO_DIR_MODE_OUT);


    //=========================================================================================
    // Main Loop
    //=========================================================================================
    while (1)
    {

        if(user_input_flag == 1){
            //Get input from user and take some action
            updateButtons();
            processUserInput();
            user_input_flag = 0;
        }

        if(display_update_flag == 1){
            //Update the display
            //displayUpdate();
            display_update_flag = 0;
        }

        if(step_update_flag == 1){
            updateAccBuffers();
            checkBump();
            step_update_flag = 0;
        }

    }
}
































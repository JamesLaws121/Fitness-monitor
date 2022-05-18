/*
 * display.c
 *
 *  Created on: May 18, 2022
 *      Author: J.Laws, D. Beukenholdt
 */

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"
#include "driverlib/debug.h"
#include "../OrbitOLED/OrbitOLEDInterface.h"
#include "utils/ustdlib.h"

#include "display.h"
#include "buttons4.h"
#include "ADC.h"



//General constants
#define STEP_DISTANCE 0.9

//LED constants
#define LED1 GPIO_PIN_6
#define LED2 GPIO_PIN_7


//global variables
uint8_t display_state ;
bool skip_frame_flag;
uint16_t distance;


enum step_units{STEPS=0, PERCENT=1} step_unit;

enum dist_units{
    KILOMETRES=0,
    MILES=1
} dist_unit;


void displayInit(){
    step_unit = STEPS;
    dist_unit = KILOMETRES;
    display_state = 0;
    skip_frame_flag = 0;
    // Initialize LEDs
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);
    GPIOPadConfigSet(GPIO_PORTC_BASE, LED1, GPIO_STRENGTH_4MA, GPIO_PIN_TYPE_STD_WPD);
    GPIOPadConfigSet(GPIO_PORTC_BASE, LED2, GPIO_STRENGTH_4MA, GPIO_PIN_TYPE_STD_WPD);
    GPIODirModeSet(GPIO_PORTC_BASE, LED1, GPIO_DIR_MODE_OUT);
    GPIODirModeSet(GPIO_PORTC_BASE, LED2, GPIO_DIR_MODE_OUT);
}

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
void displayUpdate(uint16_t* step_count,uint16_t* step_goal)
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
            lineUpdate("", (*step_count), "steps", 1);

        }
        else if (step_unit == PERCENT) {
            uint8_t step_percent = ((*step_count) * 100) / (*step_goal);
            lineUpdate("", step_percent, "%", 1);
        }

        break;

    case 1:
        //1: Display the current distance traveled
        OLEDStringDraw("Total Distance",0,0);
        OLEDStringDraw("                ", 0, 1);
        distance = ((*step_count)*STEP_DISTANCE);
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
        lineUpdate(" ", (*step_goal), "steps", 3);
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
void processUserInput(uint16_t* step_count,uint16_t* step_goal)
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
    if(switchDown()){
        //Turns on lights for feedback to user
        GPIOPinWrite(GPIO_PORTC_BASE, LED1, LED1);
        GPIOPinWrite(GPIO_PORTC_BASE, LED2, LED2);
        if (checkButton(UP) == PUSHED) {
            (*step_count) += 100;
        }
        if (checkButton(DOWN) == PUSHED) {
            (*step_count) -= 500;
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
            step_unit = (enum step_units)!step_unit;
        }

        //LONG DOWN: Reset step count
        if (checkLongPush(DOWN) == 1){
            (*step_count) = 0;
        }

        break;

    case 1: //1: Displaying total distance
        //UP: Toggle units between km and miles
        if (checkButton(UP) == PUSHED) {
            dist_unit = (enum dist_units) !dist_unit;
        }

        //LONG DOWN: Reset step count
        if (checkLongPush(DOWN) == 1){
            (*step_count) = 0;
        }

        break;

    case 2: //2: Displaying current goal
        //Potentiometer: Change displayed step goal
        // This is currently being handled in ADC.c

        //DOWN: Commit step goal
        if (checkButton(DOWN) == PUSHED) {
            (*step_goal) = (getADCMean() / 100)*100;
            display_state = 0;
        }
        break;

    default:
        break;
    }
}


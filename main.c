/**********************************************************
 *
 * main.c
 *
 * Milestone 1 code which displays acceleration and orientation
 * D. Beukenholdt, J. Laws
 * Last modified: 21 Mar 2022
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
#include "driverlib/pin_map.h" //Needed for pin configure
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

#include "stdlib.h"

#include "acc.h"
#include "i2c_driver.h"
#include "buttons4.h"
#include "circBufT.h"




void initClock (void)
{
    // Set the clock rate
    SysCtlClockSet(SYSCTL_SYSDIV_10 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN |
                   SYSCTL_XTAL_16MHZ);
}


void initDisplay(void)
{
    OLEDInitialise();
}


void displayUpdate(char *str1, char *str2, int16_t num, uint8_t charLine, char *str3)
{
    char text_buffer[17]; //Display fits 16 characters wide.

    // Clear the line to be updated.
    OLEDStringDraw("                ", 0, charLine);

    // Draw a new string to the display.
    usnprintf(text_buffer, sizeof(text_buffer), "%s %s %3d %s", str1, str2, num, str3);
    OLEDStringDraw(text_buffer, 0, charLine);
}


int64_t averageData(uint32_t BUFF_SIZE,circBuf_t* buffer){
    // returns the mean of the data stored in the given buffer
    int64_t sum = 0;
    int32_t temp;
    int32_t average;

    int i;
    for(i = 0; i < BUFF_SIZE;i++){
        temp = readCircBuf(buffer);
        sum += temp;
    }

    average = ((sum / BUFF_SIZE));
    return average;
}



// Main
int main()
{
    //=========================================================================================
    // Setup Code (runs once)
    //=========================================================================================
    orientation_t orientation;
    vector3_t accl_data;
    vector3_t currentAverage;
    vector3_t adjustedAverage;
    uint8_t   accl_unit = 0;

    // Initialize required modules
    initClock();
    initAccl();
    initDisplay();
    initButtons();

    // Setup circular buffer for accelerometer data
    circBuf_t bufferZ;
    circBuf_t bufferX;
    circBuf_t bufferY;
    uint32_t BUFF_SIZE = 20;
    initCircBuf(&bufferZ, BUFF_SIZE);
    initCircBuf(&bufferX, BUFF_SIZE);
    initCircBuf(&bufferY, BUFF_SIZE);

    // Obtain initial set of accelerometer data
    uint8_t i;
    for(i = 0; i < 20; i++){
        accl_data = getAcclData();
        writeCircBuf(&bufferZ,accl_data.z);
        writeCircBuf(&bufferX,accl_data.x);
        writeCircBuf(&bufferY,accl_data.y);
    }
    currentAverage.x = averageData(BUFF_SIZE,&bufferX);
    currentAverage.y = averageData(BUFF_SIZE,&bufferY);
    currentAverage.z = averageData(BUFF_SIZE,&bufferZ);
    orientation = getOrientation(currentAverage);


    uint8_t orientation_counter = 0;
    uint8_t display_state = 0;


    //=========================================================================================
    // Main Loop
    //=========================================================================================
    while (1)
    {
        // Set the program speed using a magic number
        // TODO: improve main loop to use SYSTICK interrupts for timing or something similar
        SysCtlDelay(SysCtlClockGet () / 32); //delay(s) = 3/magic number

        // Obtain accelerometer data and write to circular buffer
        accl_data = getAcclData();
        writeCircBuf(&bufferX,accl_data.x);
        writeCircBuf(&bufferY,accl_data.y);
        writeCircBuf(&bufferZ,accl_data.z);

        // Take a new running average of acceleration
        currentAverage.x = averageData(BUFF_SIZE,&bufferX);
        currentAverage.y = averageData(BUFF_SIZE,&bufferY);
        currentAverage.z = averageData(BUFF_SIZE,&bufferZ);


        //Check buttons for user input and take some action.
        // UP: change acceleration units
        // Down: Set new reference orientation
        updateButtons();

        if ((checkButton(UP) == PUSHED) && (display_state == 1)) {
            accl_unit++;
            if (accl_unit >= 3) {accl_unit = 0;}
        }
        if (checkButton(DOWN) == PUSHED) {
            display_state = 0;
            orientation = getOrientation(currentAverage);
        }


        // Display information to the screen depending on the display state
        switch (display_state)
        {
        case 0: //0: Display reference orientation
            OLEDStringDraw("Orientation     ", 0,0);
            displayUpdate("Roll", ": ", orientation.roll, 1, "deg");
            displayUpdate("Pitch", ":", orientation.pitch, 2, "deg");
            OLEDStringDraw("                ", 0,3);
            // TODO: improve following code for keeping orientation on screen for 3 seconds by using a proper timer
            orientation_counter++;
            if(orientation_counter >= 32){ //keep the same as number used in SysCtlDelay above for approximately 3 second delay
                display_state = 1;
                orientation_counter = 0;
            }
            break;

        case 1: //1: Display acceleration data
            OLEDStringDraw("Acceleration", 0, 0);
            adjustedAverage = convert(currentAverage, accl_unit);
            displayUpdate("Accl", "X", adjustedAverage.x, 1, getAcclUnitStr(accl_unit));
            displayUpdate("Accl", "Y", adjustedAverage.y, 2, getAcclUnitStr(accl_unit));
            displayUpdate("Accl", "Z", adjustedAverage.z, 3, getAcclUnitStr(accl_unit));
            break;

        default: //Invalid display_state: Blank screen
            OLEDStringDraw("                ", 0,0);
            OLEDStringDraw("                ", 0,1);
            OLEDStringDraw("                ", 0,2);
            OLEDStringDraw("                ", 0,3);
            break;
        }




    }
}
































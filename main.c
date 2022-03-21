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

//#define DISPLAY_TICK


void initClock (void)
{
    // Set the clock rate to 20 MHz
    SysCtlClockSet(SYSCTL_SYSDIV_10 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN |
                   SYSCTL_XTAL_16MHZ);
}


void initDisplay(void)
{
    OLEDInitialise();
}



void displayUpdate(char *str1, char *str2, int16_t num, uint8_t charLine, char *str3)
{
    char text_buffer[17];           //Display fits 16 characters wide.

    // Clear the line to be updated.
    OLEDStringDraw("                ", 0, charLine);

    // Draw a new string to the display.
    usnprintf(text_buffer, sizeof(text_buffer), "%s %s %3d %s", str1, str2, num, str3);
    OLEDStringDraw(text_buffer, 0, charLine);
}


vector3_t sumData(vector3_t currentAverage,circBuf_t* buffer,vector3_t newValue,int old_value,uint32_t BUFF_SIZE){
    (*buffer).rindex = old_value;

    currentAverage.x = ((currentAverage.x * BUFF_SIZE) - readCircBuf(buffer)) / (BUFF_SIZE - 1);
    currentAverage.x = currentAverage.x + ((newValue.x - currentAverage.x) / BUFF_SIZE);

    currentAverage.z = ((currentAverage.z * BUFF_SIZE) - readCircBuf(buffer)) / (BUFF_SIZE - 1);
    currentAverage.z = currentAverage.z + ((newValue.z - currentAverage.z) / BUFF_SIZE);

    currentAverage.y = ((currentAverage.y * BUFF_SIZE) - readCircBuf(buffer)) / (BUFF_SIZE - 1);
    currentAverage.y = currentAverage.y + ((newValue.y - currentAverage.y) / BUFF_SIZE);

    return currentAverage;
}

// Main
int main()
{
    vector3_t accl_data;
    uint8_t   accl_unit = 0;

    initClock();
    initAccl();
    initDisplay();
    initButtons();

    circBuf_t buffer;
    uint32_t BUFF_SIZE = 20;
    vector3_t currentAverage;
    vector3_t adjustedAverage;

    currentAverage = getAcclData();

    initCircBuf(&buffer, BUFF_SIZE);

    int entry = 0; //keeps track of where the buffers up to


    printf("hello");
    OLEDStringDraw("Acceleration", 0, 0);


    while (1)
    {
        SysCtlDelay(SysCtlClockGet () / 8);

        accl_data = getAcclData();
        accl_data = convert(accl_data, accl_unit);

        writeCircBuf(&buffer,entry);


        currentAverage = sumData(currentAverage,&buffer,accl_data,entry,BUFF_SIZE);
        entry++;


        //Check Up button
        updateButtons();

        switch(checkButton(UP))
        {
        case PUSHED:
            accl_unit++;
            if (accl_unit >= 3) {accl_unit = 0;}
            break;
        default:
            break;
        }
        /*
        switch(checkButton(DOWN))
        {
        case PUSHED:

            break;
        default:
            break;
        }
        */

        // Display acceleration
        adjustedAverage = convert(currentAverage, accl_unit);
        displayUpdate("Accl", "X", adjustedAverage.x, 1, getAcclUnitStr(accl_unit));
        displayUpdate("Accl", "Y", adjustedAverage.y, 2, getAcclUnitStr(accl_unit));
        displayUpdate("Accl", "Z", adjustedAverage.z, 3, getAcclUnitStr(accl_unit));
    }

}

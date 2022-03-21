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


void displayUpdate(char *str1, char *str2, int16_t num, uint8_t charLine, char str3[])
{
    char text_buffer[17];           //Display fits 16 characters wide.

    // Clear the line to be updated.
    OLEDStringDraw("                ", 0, charLine);

    // Draw a new string to the display.
    usnprintf(text_buffer, sizeof(text_buffer), "%s %s %3d %s", str1, str2, num, str3);
    OLEDStringDraw(text_buffer, 0, charLine);
}

//=====================================================================
// Takes raw acceleration data and returns acceleration in
//  raw data, multiples of Gs, or metres per second per second.
//  unit: 0=raw, 1=Gs, 2=m/s/s
//=====================================================================
vector3_t changeUnits(vector3_t accl_raw, uint8_t unit){
    vector3_t accl_out;

    switch (unit) {
    case 0:
        accl_out = accl_raw;
        break;
    case 1:
        // TODO: convert to Gs
        break;
    case 2:
        // TODO: convert to m/s/s
        break;
    default:
        accl_out.x = 0;
        accl_out.y = 0;
        accl_out.z = 0;
    }
    return accl_out;
}


// Main
int main()
{
    vector3_t accl_data;
    vector3_t accl_data_raw;
    uint8_t   accl_unit = 0;
    char unit_str[8];


    initClock();
    initAccl();
    initDisplay();
    initButtons();



    OLEDStringDraw("Acceleration", 0, 0);

    while (1)
    {
        SysCtlDelay(SysCtlClockGet () / 6);

        accl_data_raw = getAcclData();

        //check for button presses
        updateButtons();

        switch(checkButton(UP))
        {
        case PUSHED:
            accl_unit ++;
            if (accl_unit >= 3) {accl_unit = 0;}
            break;
        default:
            break;
        }

        //display acceleration on screen
        accl_data = changeUnits(accl_data_raw, accl_unit);
        switch (accl_unit) {
        case 1:
            *unit_str = *"g";
            break;
        case 2:
            *unit_str = *"m/s/s";
            break;
        default:
            *unit_str = *"test";
            break;
        }
        // FIXME: unit string does not display correctly
        displayUpdate("Accl", "X", accl_data.x, 1, unit_str);
        displayUpdate("Accl", "Y", accl_data.y, 2, unit_str);
        displayUpdate("Accl", "Z", accl_data.z, 3, unit_str);
    }
}

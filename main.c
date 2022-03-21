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

#define DISPLAY_TICK



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



// Main
int main()
{
    vector3_t accl_data;
    uint8_t   accl_unit = 0;

    initClock();
    initAccl();
    initDisplay();
    initButtons();


    OLEDStringDraw("Acceleration", 0, 0);

    while (1)
    {
        SysCtlDelay(SysCtlClockGet () / 8);

        accl_data = getAcclData();



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

        // Display acceleration
        accl_data = convert(accl_data, accl_unit);
        displayUpdate("Accl", "X", accl_data.x, 1, getAcclUnitStr(accl_unit));
        displayUpdate("Accl", "Y", accl_data.y, 2, getAcclUnitStr(accl_unit));
        displayUpdate("Accl", "Z", accl_data.z, 3, getAcclUnitStr(accl_unit));
    }
}

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



// Returns a string representing the given unit
// 0=raw(no unit), 1=mGs, 2=m/s/s.
char* getUnitStr(int8_t unit_num){

    switch (unit_num){
    case 1:
        return "mG";
    case 2:
        return "m/s/s";
    default:
        return "";
    }
}


// Main
int main()
{
    vector3_t accl_data;
    uint8_t   unit = 0;

    initClock();
    initAccl();
    initDisplay();
    initButtons();


    OLEDStringDraw("Acceleration", 0, 0);

    while (1)
    {
        SysCtlDelay(SysCtlClockGet () / 150);

        accl_data_raw = getAcclData();
        accl_data = convert(accl_data,current_state);

        ///accl_data = getAcclData();
        //unit = changeUnits(&current_state,unit);

        //Check Up button
        updateButtons();

        switch(checkButton(UP))
        {
        case PUSHED:
            unit++;
            if (unit >= 3) {unit = 0;}
            break;
        default:
            break;
        }

        accl_data = convert(accl_data, unit);
        displayUpdate("Accl", "X", accl_data.x, 1,getUnitStr(unit));
        displayUpdate("Accl", "Y", accl_data.y, 2,getUnitStr(unit));
        displayUpdate("Accl", "Z", accl_data.z, 3,getUnitStr(unit));
    }
}

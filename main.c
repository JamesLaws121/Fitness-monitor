/**********************************************************
 *
 * main.c
 *
 * Milestone 1 code which displays acceleration and orientation
 *
 *    18 Mar 2022
 *
 **********************************************************/


#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_i2c.h"
#include "driverlib/pin_map.h" //Needed for pin configure
#include "driverlib/systick.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/i2c.h"
#include "../OrbitOLED/OrbitOLEDInterface.h"
#include "utils/ustdlib.h"

#include "acc.h"
#include "i2c_driver.h"


/*
typedef struct {
    int x;
    int y;
    int z;
}vector3_t;
*/

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

void displayUpdate (char *str1, char *str2, int16_t num, uint8_t charLine)
{
    char text_buffer[17];           //Display fits 16 characters wide.

    // "Undraw" the previous contents of the line to be updated.
    OLEDStringDraw ("                ", 0, charLine);

    // Form a new string for the line.  The maximum width specified for the
    //  number field ensures it is displayed right justified.
    usnprintf(text_buffer, sizeof(text_buffer), "%s %s %3d mg", str1, str2, num);
    // Update line on display.
    OLEDStringDraw (text_buffer, 0, charLine);
}

int main()
{
    vector3_t accl_data;

    initClock();
    initAccl();
    initDisplay();

    OLEDStringDraw("Acceleration", 0, 0);

    while (1)
    {
        SysCtlDelay(SysCtlClockGet () / 6);


        accl_data = getAcclData();
        accl_data = adjustData(accl_data);

        displayUpdate("Accl", "X", accl_data.x, 1);
        displayUpdate("Accl", "Y", accl_data.y, 2);
        displayUpdate("Accl", "Z", accl_data.z, 3);
    }
}

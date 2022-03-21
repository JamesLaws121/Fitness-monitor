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

#include "inc/hw_ints.h"
#include "driverlib/adc.h"

#include "driverlib/debug.h"

#include "utils/ustdlib.h"


#include "stdlib.h"

#include "acc.h"
#include "i2c_driver.h"
#include  "buttons4.h"

#define DISPLAY_TICK


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

void displayUpdate(char *str1, char *str2, int16_t num, uint8_t charLine,char *unit)
{
    char text_buffer[17];           //Display fits 16 characters wide.

    // "Undraw" the previous contents of the line to be updated.
    OLEDStringDraw("                ", 0, charLine);

    // Form a new string for the line.  The maximum width specified for the
    //  number field ensures it is displayed right justified.
    usnprintf(text_buffer, sizeof(text_buffer), "%s %s %3d %s", str1, str2, num,unit);
    // Update line on display.
    OLEDStringDraw(text_buffer, 0, charLine);
}

char* changeUnits(int8_t* current_state,char* unit){
    *current_state = 1 + (*current_state)%2;

    if(*current_state == 1){
        return "mg";
    }
    return "m/s";
}

int main()
{
    vector3_t accl_data;

    initClock();
    initAccl();
    initDisplay();
    initButtons();

    int8_t current_state = 0;
    char* unit = "Raw";

    OLEDStringDraw("Acceleration", 0, 0);

    while (1)
    {
        SysCtlDelay(SysCtlClockGet () / 150);


        accl_data = getAcclData();
        unit = changeUnits(&current_state,unit);

        //Check Up button

        updateButtons();

        uint8_t butState;
        butState = checkButton(UP);
        switch(butState)
        {
        case PUSHED:
            accl_data = adjustData(accl_data,current_state);
            unit = changeUnits(&current_state,unit);

            //OLEDStringDraw("hello", 0, 1);
            break;
        case RELEASED:
            break;
        }


        displayUpdate("Accl", "X", accl_data.x, 1,unit);
        displayUpdate("Accl", "Y", accl_data.y, 2,unit);
        displayUpdate("Accl", "Z", accl_data.z, 3,unit);
    }
}

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


vector3_t sumData(vector3_t currentAverage,uint32_t BUFF_SIZE,circBuf_t* buffer){

    /*
    if(currentAverage.x < 0){
        currentAverage.x *=-1;
    }
    currentAverage.x = ((currentAverage.x * BUFF_SIZE) - oldValue.x);
    currentAverage.x /= (BUFF_SIZE - 1);

    if(newValue.x - currentAverage.x < 0){
        currentAverage.x = currentAverage.x + (((newValue.x - currentAverage.x)*-1)/ BUFF_SIZE);
    } else{
        currentAverage.x = currentAverage.x + ((newValue.x - currentAverage.x) / BUFF_SIZE);
    }


    if(currentAverage.z < 0){
        currentAverage.z *=-1;
    }
    currentAverage.z = ((currentAverage.z * BUFF_SIZE) - oldValue.z);
    currentAverage.z /= (BUFF_SIZE - 1);
    if(newValue.z - currentAverage.z < 0){
        currentAverage.z = currentAverage.z + (((newValue.z - currentAverage.z)*-1)/ BUFF_SIZE);
    } else{
        currentAverage.z = currentAverage.z + ((newValue.z - currentAverage.z)/ BUFF_SIZE);
    }


    if(currentAverage.y < 0){
        currentAverage.y *=-1;
    }
    currentAverage.y = ((currentAverage.y * BUFF_SIZE) - oldValue.y) ;
    currentAverage.y /= (BUFF_SIZE - 1);
    if(newValue.y - currentAverage.y < 0){
        currentAverage.y = currentAverage.y + (((newValue.y - currentAverage.y)*-1)/ BUFF_SIZE);
    } else{
        currentAverage.y = currentAverage.y + ((newValue.y - currentAverage.y)/ BUFF_SIZE);
    }


    return currentAverage;
    */


    int i;
    vector3_t sum;
    sum.x = 0;
    sum.y = 0;
    sum.z = 0;
    vector3_t temp;

    for(i = 0; i < BUFF_SIZE;i++){
        temp = readCircBuf(buffer);
        if(temp.x < 0){
            temp.x *= -1;
        }
        if(temp.y < 0){
            temp.y *= -1;
        }
        if(temp.z < 0){
            temp.z *= -1;
        }
        sum.x = sum.x + temp.x;
        sum.y = sum.y + temp.y;
        sum.z = sum.z + temp.z;
    }
    currentAverage.x = ((sum.x / BUFF_SIZE));
    currentAverage.y = ((sum.y / BUFF_SIZE));
    currentAverage.z = ((sum.z / BUFF_SIZE));

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


    int i;
    for(i = 0; i < 20; i++){
        accl_data = getAcclData();
        writeCircBuf(&buffer,accl_data);
    }

    //int entry = 0; //keeps track of where the buffers up to
    //vector3_t oldValue; // keeps track of value in buffer that gets overwritten
    //oldValue = readCircBuf(&buffer);

    printf("hello");
    OLEDStringDraw("Acceleration", 0, 0);


    while (1)
    {
        SysCtlDelay(SysCtlClockGet () / 8);

        accl_data = getAcclData();
        //accl_data = convert(accl_data, accl_unit);

        //buffer.rindex = entry;
        //buffer.windex = entry;

        //oldValue = readCircBuf(&buffer);

        writeCircBuf(&buffer,accl_data);


        currentAverage = sumData(currentAverage,BUFF_SIZE,&buffer);

        /*
        if(entry < BUFF_SIZE){
            entry++;
        } else{
            entry = 0;
        }*/



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

/*======================================================
 acc.c
 C.P. Moore, D. Beukenholdt, J. Laws
========================================================*/


#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>


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


/*======================================================
 Initializes accelerometer
 written by C.P. Moore
 https://learn.canterbury.ac.nz/pluginfile.php/4291802/mod_folder/content/0/Week_3_lab_code.zip
========================================================*/
void initAccl (void)
{
    char    toAccl[] = {0, 0};  // parameter, value

    /*
     * Enable I2C Peripheral
     */
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C0);
    SysCtlPeripheralReset(SYSCTL_PERIPH_I2C0);

    /*
     * Set I2C GPIO pins
     */
    GPIOPinTypeI2C(I2CSDAPort, I2CSDA_PIN);
    GPIOPinTypeI2CSCL(I2CSCLPort, I2CSCL_PIN);
    GPIOPinConfigure(I2CSCL);
    GPIOPinConfigure(I2CSDA);

    /*
     * Setup I2C
     */
    I2CMasterInitExpClk(I2C0_BASE, SysCtlClockGet(), true);

    GPIOPinTypeGPIOInput(ACCL_INT2Port, ACCL_INT2);

    //Initialize ADXL345 Acceleromter

    // set +-2g, 13 bit resolution, active low interrupts
    toAccl[0] = ACCL_DATA_FORMAT;
    toAccl[1] = (ACCL_RANGE_2G | ACCL_FULL_RES);
    I2CGenTransmit(toAccl, 1, WRITE, ACCL_ADDR);

    toAccl[0] = ACCL_PWR_CTL;
    toAccl[1] = ACCL_MEASURE;
    I2CGenTransmit(toAccl, 1, WRITE, ACCL_ADDR);


    toAccl[0] = ACCL_BW_RATE;
    toAccl[1] = ACCL_RATE_100HZ;
    I2CGenTransmit(toAccl, 1, WRITE, ACCL_ADDR);

    toAccl[0] = ACCL_INT;
    toAccl[1] = 0x00;       // Disable interrupts from accelerometer.
    I2CGenTransmit(toAccl, 1, WRITE, ACCL_ADDR);

    toAccl[0] = ACCL_OFFSET_X;
    toAccl[1] = 0x00;
    I2CGenTransmit(toAccl, 1, WRITE, ACCL_ADDR);

    toAccl[0] = ACCL_OFFSET_Y;
    toAccl[1] = 0x00;
    I2CGenTransmit(toAccl, 1, WRITE, ACCL_ADDR);

    toAccl[0] = ACCL_OFFSET_Z;
    toAccl[1] = 0x00;
    I2CGenTransmit(toAccl, 1, WRITE, ACCL_ADDR);
}

//======================================================
// Function to read accelerometer
//======================================================
vector3_t getAcclData (void)
{
    char    fromAccl[] = {0, 0, 0, 0, 0, 0, 0}; // starting address, placeholders for data to be read.
    vector3_t acceleration;
    uint8_t bytesToRead = 6;

    fromAccl[0] = ACCL_DATA_X0;
    I2CGenTransmit(fromAccl, bytesToRead, READ, ACCL_ADDR);

    acceleration.x = ((fromAccl[2] << 8) | fromAccl[1]); // Return 16-bit acceleration readings.
    acceleration.y = ((fromAccl[4] << 8) | fromAccl[3]);
    acceleration.z = ((fromAccl[6] << 8) | fromAccl[5]);

    return acceleration;
}



//=====================================================================
// Takes raw acceleration data and returns acceleration in
//  raw data, milli Gs, or metres per second per second.
//  unit: 0=raw, 1=Gs, 2=m/s/s
//=====================================================================
vector3_t convert(vector3_t accl_raw, uint8_t unit){
    vector3_t accl_out;

    switch (unit) {
        case 0:
            //Raw
            accl_out = accl_raw;
            break;
        case 1:
            // raw --> milli g
            accl_out.x = accl_raw.x * 100;
            accl_out.y = accl_raw.y * 100;
            accl_out.z = accl_raw.z * 100;

            accl_out.x = accl_out.x / 256;
            accl_out.y = accl_out.y / 256;
            accl_out.z = accl_out.z / 256;

            accl_out.x = accl_out.x * 10;
            accl_out.y = accl_out.y * 10;
            accl_out.z = accl_out.z * 10;
            break;
        case 2:
            // raw --> ms^-1

            accl_out.x = accl_raw.x * 9.81;
            accl_out.y = accl_raw.y * 9.81;
            accl_out.z = accl_raw.z * 9.81;


            accl_out.x = accl_out.x / 256;
            accl_out.y = accl_out.y / 256;
            accl_out.z = accl_out.z / 256;



            break;
        default:
            accl_out.x = 0;
            accl_out.y = 0;
            accl_out.z = 0;
        }
    return accl_out;
}




//====================================================================
// Returns a string representing the given unit
// 0=raw(no unit), 1=mGs, 2=m/s/s.
//====================================================================
char* getAcclUnitStr(int8_t unit_num){

    switch (unit_num){
    case 1:
        return "mG";
    case 2:
        return "m/s/s";
    default:
        return "";
    }
}

orientation_t radiansToDegrees(orientation_t orientation){

    orientation.roll = ((orientation.roll*57.3)/1000);
    orientation.pitch = ((orientation.pitch*57.3)/1000);
    return orientation;
}

orientation_t getOrientation(vector3_t accl_raw)
{
    //Do a bunch of math to calculate orientation

    orientation_t orientation;
    orientation.roll = 0;
    orientation.pitch = 0;

    float temp = 0;

    temp = ((accl_raw.y*1000)/(accl_raw.z));
    temp /= 1000;
    orientation.pitch = atan(temp)*1000;




    temp = sqrt(pow((accl_raw.y),2) + pow((accl_raw.z),2));

    temp = ((accl_raw.x*-1)/temp);


    orientation.roll = atan(temp)*1000;


    return radiansToDegrees(orientation);
}


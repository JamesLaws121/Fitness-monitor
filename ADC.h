/*
 * ADC.h
 *
 *  Created on: Apr 27, 2022
 *
 */

#ifndef ADC_H_
#define ADC_H_

void SysTickIntHandler(void);

void ADCIntHandler(void);

void initClock (void);

void initADC (void);

uint16_t getStep_goal();

#endif /* ADC_H_ */

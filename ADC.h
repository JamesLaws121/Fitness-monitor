/*
 * ADC.h
 *
 *  Created on: Apr 27, 2022
 *      Author: Dell Latitude
 */

#ifndef ADC_H_
#define ADC_H_

void SysTickIntHandler(void);

void ADCIntHandler(void);

void initClock (void);

void  initADC (void);

#endif /* ADC_H_ */

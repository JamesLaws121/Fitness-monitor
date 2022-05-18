/**************************************
 * ADC.h
 *
 * P.J. Bones   UCECE, D. Beukenholdt, J. Laws
 *
 * Last modified: 18 May 2022
 **************************************/

#ifndef ADC_H_
#define ADC_H_

// The handler for the ADC conversion complete interrupt.
void ADCIntHandler(void);

// Initializes the ADC peripheral
void initADC (void);

// Returns the mean value in the ADCs circular buffer
uint16_t getADCMean();

#endif /* ADC_H_ */

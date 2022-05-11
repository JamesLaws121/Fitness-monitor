//*****************************************************************************
//
// ADC.h
//
//*****************************************************************************

#ifndef ADC_H_
#define ADC_H_

void ADCIntHandler(void);

void initADC (void);

uint16_t getADCMean();

#endif /* ADC_H_ */

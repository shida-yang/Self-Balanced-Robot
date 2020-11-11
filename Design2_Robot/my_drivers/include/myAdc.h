#ifndef MYADC_H_
#define MYADC_H_

#include <F28x_Project.h>
#include "adc.h"
#include "cputimer.h"
#include "interrupt.h"
//#include "math.h"

void initAdc(uint32_t base, ADC_ClkPrescale clkPrescale, ADC_Resolution resolution,
             ADC_SignalMode signalMode);
void initAdcSoc(uint32_t base, ADC_SOCNumber socNumber, ADC_Trigger trigger,
                ADC_Channel channel, uint32_t sampleWindow, ADC_IntNumber adcIntNum);
//float convertToFloat(Uint16 data, ADC_Resolution resolution);
//char* convertToString(float dataF, Uint16 numOfDecimals);

#endif

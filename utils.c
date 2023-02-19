#include "hal_adc.h"

#include "utils.h"

/**************************************************************************************************
 * @fn      adcReadOversampled
 *
 * @brief   Get oversampled value from ADC
 *
 * @param   channel - ADC channel
 * @param   resolution - ADC resolution
 * @param   reference - reference to use
 * @param   samplesCount - sample count
 *
 * @return  oversampled ADC readout
 **************************************************************************************************/
uint16 adcReadOversampled(uint8 channel, uint8 resolution, uint8 reference, uint8 samplesCount)
{
    HalAdcSetReference(reference);
    uint32 samplesSum = 0;
    for (uint8 i = 0; i < samplesCount; i++)
    {
        samplesSum += HalAdcRead(channel, resolution);
    }
    return (samplesSum / samplesCount);
}

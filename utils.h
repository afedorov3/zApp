#ifndef UTILS_H
#define UTILS_H

/*********************************************************************
 * @fn          COUNT_OF
 *
 * @brief       evaluates to number of elements in the array
 *
 * @param       a - static array
 */
#define COUNT_OF(a) (sizeof(a) / sizeof(0[a]))

/*********************************************************************
 * @fn          LAST_OF
 *
 * @brief       evaluates to the last element of the array
 *
 * @param       a - static array
 */
#define LAST_OF(a) ((a)[COUNT_OF(a)-1])
      
/*********************************************************************
 * @fn          DEVIATION
 *
 * @brief       evaluates to absolute difference between two numbers
 *
 * @param       a - first number
 * @param       b - second number
 */
#define DEVIATION(a, b) ((a) > (b) ? (a) - (b) : (b) - (a))

/*********************************************************************
 * @fn          MAP
 *
 * @brief       maps a number from one range to another
 *
 * @param       value - input value from the first range
 * @param       in_min - first range minimum
 * @param       in_max - first range maximum
 * @param       out_min - second range minimum
 * @param       out_max - second range maximum
 */
#define MAP(value, in_min, in_max, out_min, out_max) \
  (((value) - (in_min)) * ((out_max) - (out_min)) / ((in_max) - (in_min)) + (out_min))

/*********************************************************************
 * @fn          SCALE
 *
 * @brief       scales input value to a given power of ten scale
 *
 * @param       output - output variable
 * @param       value - input value
 * @param       scale - scale power
 */
#define SCALE(output, value, scale)               \
    do {                                          \
        int8 s = (scale);                         \
        int32 v = value;                          \
        while (s != 0)                            \
        {                                         \
          if (s > 0)                              \
            v *= 10, s--;                         \
          else                                    \
            v /= 10, s++;                         \
        }                                         \
        output = v;                               \
    } while (__LINE__ == -1)

/*********************************************************************
 * @fn          ADC2MV
 *
 * @brief       converts raw ADC value to input milliVolts
 *
 * @param       raw - raw ADC value
 * @param       reference - reference voltage in milliVolts
 * @param       resolution - ADC value resolution (HAL_ADC_RESOLUTION_*)
 */
#define ADC2MV(raw, reference, resolution)       \
    ((uint32)(raw) * (reference) * 3 / ((32 << (resolution) * 2) - 1))

extern uint16 adcReadOversampled(uint8 channel, uint8 resolution, uint8 reference, uint8 samplesCount);

#endif /* UTILS_H */

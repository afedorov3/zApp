#include "hal_adc.h"
#include "zcl.h"
#include "zcl_general.h"
#include "bdb_interface.h"
#include "utils.h"
#include "debug_print.h"

#include "battery.h"

/*********************************************************************
 * TYPEDEFS
 */
typedef struct
{
  uint16 mV;  // battery voltage in milliVolts
  uint8 perc; // BatteryPercentageRemaining value (unit is 0.5%)
} bat_charge_t;

/*********************************************************************
 * CONSTANTS
 */

/*
 * BAT_PMAP is the initializer for array of bat_charge_t structures
 * above and has to provide initialization for at least two elements
 */
#ifndef BAT_PMAP
#warning Using default battery charge percentage map for 2s AAA
// 2s AAA, based on Duracell MX2400 datasheet
#define BAT_PMAP { { 2500, 0 }, { 2580, 22 }, { 2730, 114 }, { 2985, 188 }, { 3200, 200 } }
#endif /* !BAT_PMAP */

// define BAT_FILTER_S as 0 to disable battery voltage filtering
#ifndef BAT_FILTER_S
#define BAT_FILTER_S         21600 // 6 hours
#endif /* !BAT_FILTER_S */

#if BAT_FILTER_S > 0
#undef BAT_FILTER
#define BAT_FILTER (1000000/(6283/((BAT_FILTER_S)/(APP_BAT_REPORT_INTERVAL_MS/1000)) + 1000))
#endif /* BAT_FILTER_S > 0 */

#ifndef ADC_VREF_MV
#define ADC_VREF_MV          1150
#endif /* ADC_VREF_MV */

#ifndef BAT_ADC_RESOLUTION
#define BAT_ADC_RESOLUTION   HAL_ADC_RESOLUTION_14
#endif /* BAT_ADC_RESOLUTION */

#ifndef POWER_CFG_ENDPOINT
#define POWER_CFG_ENDPOINT   1
#endif /* POWER_CFG_ENDPOINT */

/*********************************************************************
 * GLOBAL VARIABLES
 */
uint8  zclBattery_Voltage = BATTERY_INVALID;
uint8  zclBattery_PercentageRemainig = BATTERY_INVALID;
uint16 zclBattery_mV = BATTERY_MV_INVALID;

/*********************************************************************
 * LOCAL PROTOTYPES
 */
static inline uint8 zclBatteryPercentage(uint16 mV);

/*********************************************************************
 * LOCAL FUNCTIONS
 */

/*********************************************************************
 * @fn      zclBatteryPercentage
 *
 * @brief   Map battery voltage to charge remaining percentage
 *
 * @param   mV - battery voltage in milliVolts
 *
 * @return  charge remaining percentage
 */
static inline uint8 zclBatteryPercentage(uint16 mV)
{
  static const bat_charge_t bat_charge[] = BAT_PMAP;
  int i;

  if (mV <= bat_charge[0].mV) return bat_charge[0].perc;
  if (mV >= LAST_OF(bat_charge).mV) return LAST_OF(bat_charge).perc;

  for(i = 1; i < COUNT_OF(bat_charge); i++)
  {
    if (mV == bat_charge[i].mV) return bat_charge[i].perc;
    if (mV < bat_charge[i].mV) break;
  }

  return MAP(mV, bat_charge[i-1].mV, bat_charge[i].mV, bat_charge[i-1].perc, bat_charge[i].perc);
}

/*********************************************************************
 * GLOBAL FUNCTIONS
 */

/*********************************************************************
 * @fn      zclBatteryPercentage
 *
 * @brief   Measure and report battery state
 *
 * @param   forced - force the report bypassing BDB_REPORTING mechanics
 *
 * @return  none
 */
void zclBatteryReport(bool forced)
{
  (void)forced;

  static const zclReportCmd_t BatReportCmd =
    {
      .numAttr = 3,
      .attrList =
      {
        {
          .attrID = ATTRID_POWER_CFG_BATTERY_VOLTAGE,
          .dataType = ZCL_DATATYPE_UINT8,
          .attrData = (void*)(&zclBattery_Voltage)
        },
        {
          .attrID = ATTRID_POWER_CFG_BATTERY_PERCENTAGE_REMAINING,
          .dataType = ZCL_DATATYPE_UINT8,
          .attrData = (void *)(&zclBattery_PercentageRemainig)
        },
        {
          .attrID = ATTRID_POWER_CFG_BATTERY_VOLTAGE_MV,
          .dataType = ZCL_DATATYPE_UINT16,
          .attrData = (void *)(&zclBattery_mV)
        }
      }
    };

  uint16 rawADC = adcReadOversampled(HAL_ADC_CHANNEL_VDD, BAT_ADC_RESOLUTION, HAL_ADC_REF_125V, 10);
  uint16 mV = ADC2MV(rawADC, ADC_VREF_MV, BAT_ADC_RESOLUTION);
#ifdef BAT_FILTER
  if (zclBattery_mV != BATTERY_MV_INVALID)
    mV = ((uint32)zclBattery_mV * BAT_FILTER + (1000 - BAT_FILTER) * (uint32)mV) / 1000;
#endif
  zclBattery_mV = mV;
  zclBattery_Voltage = (zclBattery_mV + 50) / 100;
  zclBattery_PercentageRemainig = zclBatteryPercentage(zclBattery_mV);

#ifdef BDB_REPORTING
  if (forced)
  {
#endif /* BDB_REPORTING */
    afAddrType_t dstAddr = {
      .addrMode = (afAddrMode_t)AddrNotPresent,
      .addr.shortAddr = 0,
      .endPoint = POWER_CFG_ENDPOINT,
    };
    zcl_SendReportCmd(POWER_CFG_ENDPOINT, &dstAddr,
                      ZCL_CLUSTER_ID_GEN_POWER_CFG, (zclReportCmd_t*)(&BatReportCmd),
                      ZCL_FRAME_SERVER_CLIENT_DIR, FALSE, bdb_getZCLFrameCounter());
#ifdef BDB_REPORTING
  }
  else
  {
    bdb_RepChangedAttrValue(POWER_CFG_ENDPOINT, ZCL_CLUSTER_ID_GEN_POWER_CFG, ATTRID_POWER_CFG_BATTERY_VOLTAGE);
  }
#endif /* BDB_REPORTING */

  DBGF("BAT: %d ADC %d mV %d %%\r\n", rawADC, zclBattery_mV, (zclBattery_PercentageRemainig + 1) / 2);
}

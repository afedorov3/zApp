#ifndef BATTERY_H
#define BATTERY_H

#include "hal_defs.h"

// Custom attributes
#define ATTRID_POWER_CFG_BATTERY_VOLTAGE_MV                0x0210

#define BATTERY_INVALID 0xFF
#define BATTERY_MV_INVALID 0xFFFF

#ifndef APP_BAT_REPORT_INTERVAL_MS
#define APP_BAT_REPORT_INTERVAL_MS      ((uint32) 7200000)  // 120 minutes
#endif /* APP_BAT_REPORT_INTERVAL_MS */

extern uint8  zclBattery_Voltage;
extern uint8  zclBattery_PercentageRemainig;
extern uint16 zclBattery_mV;

extern void zclBatteryReport(bool forced);

#endif /* BATTERY_H */

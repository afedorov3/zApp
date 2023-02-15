#ifndef ALARM_REPORTING_H
#define ALARM_REPORTING_H

#include "hal_defs.h"

#define ALARM_MASK_NO_FAULT  0

extern uint8 zclAlarm_Mask;

extern void zclAlarmReport(void);

#endif /* ALARM_REPORTING_H */

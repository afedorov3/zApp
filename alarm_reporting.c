#include "zcl.h"
#include "zcl_general.h"
#include "bdb_interface.h"
#include "debug_print.h"

#include "alarm_reporting.h"

/*********************************************************************
 * CONSTANTS
 */

#ifndef GEN_BASIC_ENDPOINT
#define GEN_BASIC_ENDPOINT   1
#endif /* GEN_BASIC_ENDPOINT */

/*********************************************************************
 * GLOBAL VARIABLES
 */
uint8 zclAlarm_Mask = ALARM_MASK_NO_FAULT;

/*********************************************************************
 * GLOBAL FUNCTIONS
 */

/*********************************************************************
 * @fn      zclTHPremake_AlarmReport
 *
 * @brief   Report Alarm Mask
 *
 * @param   none
 *
 * @return  none
 */
void zclAlarmReport(void)
{
  static const zclReportCmd_t AlrmReportCmd =
    {
      .numAttr = 1,
      .attrList =
      {
        {
          .attrID = ATTRID_BASIC_ALARM_MASK,
          .dataType = ZCL_DATATYPE_BITMAP8,
          .attrData = (void*)(&zclAlarm_Mask)
        }
      }
    };
  static uint8 alarm_pval = ALARM_MASK_NO_FAULT;

  if (zclAlarm_Mask == alarm_pval)
    return;

  afAddrType_t dstAddr = {
    .addrMode = (afAddrMode_t)AddrNotPresent,
    .addr.shortAddr = 0,
    .endPoint = GEN_BASIC_ENDPOINT,
  };
  zcl_SendReportCmd(GEN_BASIC_ENDPOINT, &dstAddr,
                    ZCL_CLUSTER_ID_GEN_BASIC, (zclReportCmd_t*)(&AlrmReportCmd),
                    ZCL_FRAME_SERVER_CLIENT_DIR, FALSE, bdb_getZCLFrameCounter());
  alarm_pval = zclAlarm_Mask;
  
  DBGF("ALRM: %d\r\n", zclAlarm_Mask);
}

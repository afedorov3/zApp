#include "AF.h"
#include "OnBoard.h"
#include "bdb.h"
#include "bdb_interface.h"
#include "ZComDef.h"
#include "hal_led.h"
#include "hal_key.h"
#include "debug_print.h"

#include "factory_reset.h"

#ifndef FACTORY_RESET_BY_LONG_PRESS
    #define FACTORY_RESET_BY_LONG_PRESS TRUE
#endif

#ifndef FACTORY_RESET_BY_BOOT_COUNTER
    #define FACTORY_RESET_BY_BOOT_COUNTER TRUE
#endif

#ifndef FACTORY_RESET_HOLD_TIME_LONG
    #define FACTORY_RESET_HOLD_TIME_LONG ((uint32)10 * 1000)
#endif

#ifndef FACTORY_RESET_HOLD_TIME_FAST
    #define FACTORY_RESET_HOLD_TIME_FAST (uint32)1000
#endif

#ifndef FACTORY_RESET_BY_LONG_PRESS_PORT
    #define FACTORY_RESET_BY_LONG_PRESS_PORT 0x00
#endif

#ifndef FACTORY_RESET_BOOTCOUNTER_MAX_VALUE
    #define FACTORY_RESET_BOOTCOUNTER_MAX_VALUE 5
#endif

#ifndef FACTORY_RESET_BOOTCOUNTER_RESET_TIME
    #define FACTORY_RESET_BOOTCOUNTER_RESET_TIME ((uint32)10 * 1000)
#endif

#if !(FACTORY_RESET_BY_BOOT_COUNTER) && !(FACTORY_RESET_BY_LONG_PRESS)
#error At least one of FACTORY_RESET_BY_BOOT_COUNTER or FACTORY_RESET_BY_LONG_PRESS should be enabled
#endif /* !FACTORY_RESET_BY_BOOT_COUNTER && !FACTORY_RESET_BY_LONG_PRESS */

#if (FACTORY_RESET_BOOTCOUNTER_MAX_VALUE) < 3
#error FACTORY_RESET_BOOTCOUNTER_MAX_VALUE couldn't be less than 3
#endif /* FACTORY_RESET_BOOTCOUNTER_MAX_VALUE */

#define EVT_FACTORY_RESET 0x1000
#define EVT_FACTORY_BOOTCOUNTER_RESET 0x2000

static void zclFactoryResetter_ResetToFN(void);
#if FACTORY_RESET_BY_BOOT_COUNTER
static void zclFactoryResetter_ProcessBootCounter(void);
static void zclFactoryResetter_ResetBootCounter(void);
#endif /* FACTORY_RESET_BY_BOOT_COUNTER */

static uint8 zclFactoryResetter_TaskID;

/**************************************************************************************************
 * @fn      zclFactoryResetter_Init
 *
 * @brief   Initialize factory resetter task
 *
 * @param   task_id - ID of this task
 *
 * @return  None
 **************************************************************************************************/
void zclFactoryResetter_Init(uint8 task_id)
{
    zclFactoryResetter_TaskID = task_id;
    /**
     * We can't register more than one task, call in main app taks
     * zclFactoryResetter_HandleKeys(portAndAction, keyCode);
     * */
    // RegisterForKeys(task_id);
#if FACTORY_RESET_BY_BOOT_COUNTER
    zclFactoryResetter_ProcessBootCounter();
#endif
}

/**************************************************************************************************
 * @fn      zclFactoryResetter_event_loop
 *
 * @brief   Task event loop
 *
 * @param   task_id - ID of the task
 * @param   events - pending events
 *
 * @return  pending events still active after completion
 **************************************************************************************************/
uint16 zclFactoryResetter_event_loop(uint8 task_id, uint16 events)
{
    if (events & EVT_FACTORY_RESET)
    {
        DBG("EVT_FACTORY_RESET\r\n");
        zclFactoryResetter_ResetToFN();
        return (events ^ EVT_FACTORY_RESET);
    }

#if FACTORY_RESET_BY_BOOT_COUNTER
    if (events & EVT_FACTORY_BOOTCOUNTER_RESET)
    {
        DBG("EVT_FACTORY_BOOTCOUNTER_RESET\r\n");
        zclFactoryResetter_ResetBootCounter();
        return (events ^ EVT_FACTORY_BOOTCOUNTER_RESET);
    }
#endif /* FACTORY_RESET_BY_BOOT_COUNTER */

    return 0;
}

/**************************************************************************************************
 * @fn      zclFactoryResetter_HandleKeys
 *
 * @brief   Process keys
 *
 * @param   portAndAction - key port and action
 * @param   keyCode - key pin
 *
 * @return  None
 **************************************************************************************************/
void zclFactoryResetter_HandleKeys(uint8 portAndAction, uint8 keyCode)
{
#if FACTORY_RESET_BY_LONG_PRESS
#if FACTORY_RESET_BY_LONG_PRESS_PORT
    if (!((FACTORY_RESET_BY_LONG_PRESS_PORT) & portAndAction))
        return;
#endif /* FACTORY_RESET_BY_LONG_PRESS_PORT */

    if (portAndAction & HAL_KEY_RELEASE)
    {
        DBG("zclFactoryResetter: Key release\r\n");
        osal_stop_timerEx(zclFactoryResetter_TaskID, EVT_FACTORY_RESET);
    }
    else
    {
        DBG("zclFactoryResetter: Key press\r\n");
        uint32 timeout = bdbAttributes.bdbNodeIsOnANetwork ? FACTORY_RESET_HOLD_TIME_LONG : FACTORY_RESET_HOLD_TIME_FAST;
        osal_start_timerEx(zclFactoryResetter_TaskID, EVT_FACTORY_RESET, timeout);
    }
#endif /* FACTORY_RESET_BY_LONG_PRESS */
}

/**************************************************************************************************
 * @fn      zclFactoryResetter_ResetToFN
 *
 * @brief   Do factory reset
 *
 * @param   None
 *
 * @return  None
 **************************************************************************************************/
static void zclFactoryResetter_ResetToFN(void)
{
    HalLedSet(HAL_LED_1, HAL_LED_MODE_FLASH);
    DBGF("bdbAttributes.bdbNodeIsOnANetwork=%d bdbAttributes.bdbCommissioningMode=0x%X\r\n", bdbAttributes.bdbNodeIsOnANetwork, bdbAttributes.bdbCommissioningMode);
    DBG("zclFactoryResetter: Reset to FN\r\n");
    bdb_resetLocalAction();
}

#if FACTORY_RESET_BY_BOOT_COUNTER
/**************************************************************************************************
 * @fn      zclFactoryResetter_ProcessBootCounter
 *
 * @brief   Read, increment and check boot counter
 *
 * @param   None
 *
 * @return  None
 **************************************************************************************************/
static void zclFactoryResetter_ProcessBootCounter(void)
{
    uint16 bootCnt = 0;

    DBG("zclFactoryResetter_ProcessBootCounter\r\n");

    osal_start_timerEx(zclFactoryResetter_TaskID, EVT_FACTORY_BOOTCOUNTER_RESET, FACTORY_RESET_BOOTCOUNTER_RESET_TIME);

    if (osal_nv_item_init(ZCD_NV_BOOTCOUNTER, sizeof(bootCnt), &bootCnt) == ZSUCCESS) {
        osal_nv_read(ZCD_NV_BOOTCOUNTER, 0, sizeof(bootCnt), &bootCnt);
    }
    DBGF("bootCnt %d\r\n", bootCnt);
    bootCnt++;
    if (bootCnt >= (FACTORY_RESET_BOOTCOUNTER_MAX_VALUE)) {
        DBGF("bootCnt %d reached %d, executing factory reset\r\n", bootCnt, FACTORY_RESET_BOOTCOUNTER_MAX_VALUE);
        bootCnt = 0;
        osal_stop_timerEx(zclFactoryResetter_TaskID, EVT_FACTORY_BOOTCOUNTER_RESET);
        osal_start_timerEx(zclFactoryResetter_TaskID, EVT_FACTORY_RESET, 5000);
    }
    osal_nv_write(ZCD_NV_BOOTCOUNTER, 0, sizeof(bootCnt), &bootCnt);
}

/**************************************************************************************************
 * @fn      zclFactoryResetter_ResetBootCounter
 *
 * @brief   Reset boot counter
 *
 * @param   None
 *
 * @return  None
 **************************************************************************************************/
static void zclFactoryResetter_ResetBootCounter(void)
{
    uint16 bootCnt = 0;
    osal_nv_write(ZCD_NV_BOOTCOUNTER, 0, sizeof(bootCnt), &bootCnt);
    DBG("Boot counter reset\r\n");
}
#endif /* FACTORY_RESET_BY_BOOT_COUNTER */

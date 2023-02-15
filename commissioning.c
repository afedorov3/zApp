#include "OSAL_PwrMgr.h"
#include "ZDApp.h"
#include "bdb_interface.h"
#include "hal_key.h"
#include "hal_led.h"
#include "debug_print.h"

#include "commissioning.h"

#define APP_COMMISSIONING_END_DEVICE_REJOIN_MAX_DELAY ((uint32)1800000)     // 30 minutes 30 * 60 * 1000
#define APP_COMMISSIONING_END_DEVICE_REJOIN_START_DELAY ((uint32)10 * 1000) // 10 seconds
#define APP_COMMISSIONING_END_DEVICE_REJOIN_BACKOFF(delay) (delay = delay * 12 / 10)
#define APP_COMMISSIONING_END_DEVICE_REJOIN_TRIES 20

#ifndef APP_TX_POWER
    #define APP_TX_POWER TX_PWR_PLUS_4
#endif

#define EVT_COMMISSIONING_CLOCK_DOWN_POLING_RATE  0x0001
#define EVT_COMMISSIONING_END_DEVICE_REJOIN       0x0002

static void zclCommissioning_ProcessCommissioningStatus(bdbCommissioningModeMsg_t *bdbCommissioningModeMsg);
static void zclCommissioning_BindNotification(bdbBindNotificationData_t *data);
static void zclCommissioning_ResetBackoffRetry(void);
static void zclCommissioning_OnConnect(void);

extern bool requestNewTrustCenterLinkKey;

static byte rejoinsLeft = APP_COMMISSIONING_END_DEVICE_REJOIN_TRIES;
static uint32 rejoinDelay = APP_COMMISSIONING_END_DEVICE_REJOIN_START_DELAY;

static uint8 zclCommissioning_TaskId = 0;

/**************************************************************************************************
 * @fn      zclCommissioning_Init
 *
 * @brief   Initialize commissioning task
 *
 * @param   task_id - ID of this task
 *
 * @return  None
 **************************************************************************************************/
void zclCommissioning_Init(uint8 task_id)
{
    zclCommissioning_TaskId = task_id;

    bdb_RegisterCommissioningStatusCB(zclCommissioning_ProcessCommissioningStatus);
    bdb_RegisterBindNotificationCB(zclCommissioning_BindNotification);

    ZMacSetTransmitPower(APP_TX_POWER);

    // this is important to allow connects throught routers
    // to make this work, coordinator should be compiled with this flag #define TP2_LEGACY_ZC
    requestNewTrustCenterLinkKey = FALSE;
    bdb_StartCommissioning(BDB_COMMISSIONING_MODE_NWK_STEERING | BDB_COMMISSIONING_MODE_FINDING_BINDING);
}

/**************************************************************************************************
 * @fn      zclCommissioning_event_loop
 *
 * @brief   Task event loop
 *
 * @param   task_id - ID of the task
 * @param   events - pending events
 *
 * @return  pending events still active after completion
 **************************************************************************************************/
uint16 zclCommissioning_event_loop(uint8 task_id, uint16 events) {
    if (events & SYS_EVENT_MSG) {
        devStates_t zclApp_NwkState;
        afIncomingMSGPacket_t *MSGpkt;
        while ((MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive(zclCommissioning_TaskId))) {

            switch (MSGpkt->hdr.event) {
            case ZDO_STATE_CHANGE:
                HalLedSet(HAL_LED_1, HAL_LED_MODE_BLINK);
                zclApp_NwkState = (devStates_t)(MSGpkt->hdr.status);
                DBGF("NwkState=%d\r\n", zclApp_NwkState);
                if (zclApp_NwkState == DEV_END_DEVICE) {
                    HalLedSet(HAL_LED_1, HAL_LED_MODE_OFF);
                }
                break;

            case ZCL_INCOMING_MSG:
                if (((zclIncomingMsg_t *)MSGpkt)->attrCmd)
                {
                    osal_mem_free(((zclIncomingMsg_t *)MSGpkt)->attrCmd);
                }
                break;

            default:
                break;
            }

            // Release the memory
            osal_msg_deallocate((uint8 *)MSGpkt);
        }

        // return unprocessed events
        return (events ^ SYS_EVENT_MSG);
    }
    if (events & EVT_COMMISSIONING_END_DEVICE_REJOIN) {
        DBG("APP_END_DEVICE_REJOIN_EVT\r\n");
#if ZG_BUILD_ENDDEVICE_TYPE
        bdb_ZedAttemptRecoverNwk();
#endif
        return (events ^ EVT_COMMISSIONING_END_DEVICE_REJOIN);
    }

    if (events & EVT_COMMISSIONING_CLOCK_DOWN_POLING_RATE) {
        DBG("APP_CLOCK_DOWN_POLING_RATE_EVT\r\n");
        zclCommissioning_Sleep(true);
        return (events ^ EVT_COMMISSIONING_CLOCK_DOWN_POLING_RATE);
    }

    // Discard unknown events
    return 0;
}

/**************************************************************************************************
 * @fn      zclCommissioning_Sleep
 *
 * @brief   Initialize commissioning task
 *
 * @param   allow - true to allow sleep, false to prevent
 *
 * @return  None
 **************************************************************************************************/
void zclCommissioning_Sleep(uint8 allow) {
    DBGF("zclCommissioning_Sleep %d\r\n", allow);
#if defined(POWER_SAVING)
    if (allow) {
        NLME_SetPollRate(0);
    } else {
        NLME_SetPollRate(POLL_RATE);
    }
#endif
}

/**************************************************************************************************
 * @fn      zclCommissioning_HandleKeys
 *
 * @brief   Process keys
 *
 * @param   portAndAction - key port and action
 * @param   keyCode - key pin
 *
 * @return  None
 **************************************************************************************************/
void zclCommissioning_HandleKeys(uint8 portAndAction, uint8 keyCode)
{
    if (portAndAction & HAL_KEY_PRESS)
    {
#if ZG_BUILD_ENDDEVICE_TYPE
        if (devState == DEV_NWK_ORPHAN)
        {
            DBGF("devState=%d try to restore network\r\n", devState);
            bdb_ZedAttemptRecoverNwk();
        }
#endif
    }
#if defined(POWER_SAVING)
    NLME_SetPollRate(1);
#endif
}

/**************************************************************************************************
 * @fn      zclCommissioning_ProcessCommissioningStatus
 *
 * @brief   Commissioning status callback function
 *
 * @param   bdbCommissioningModeMsg - incoming commissioning message
 *
 * @return  None
 **************************************************************************************************/
static void zclCommissioning_ProcessCommissioningStatus(bdbCommissioningModeMsg_t *bdbCommissioningModeMsg)
{
    DBGF("bdbCommissioningMode=%d bdbCommissioningStatus=%d bdbRemainingCommissioningModes=0x%X\r\n",
         bdbCommissioningModeMsg->bdbCommissioningMode, bdbCommissioningModeMsg->bdbCommissioningStatus,
         bdbCommissioningModeMsg->bdbRemainingCommissioningModes);
    switch (bdbCommissioningModeMsg->bdbCommissioningMode)
    {
    case BDB_COMMISSIONING_INITIALIZATION:
        switch (bdbCommissioningModeMsg->bdbCommissioningStatus)
        {
        case BDB_COMMISSIONING_NO_NETWORK:
            DBG("No network\r\n");
            HalLedBlink(HAL_LED_1, 3, 50, 500);
            break;
        case BDB_COMMISSIONING_NETWORK_RESTORED:
            zclCommissioning_OnConnect();
            break;
        default:
            break;
        }
        break;

    case BDB_COMMISSIONING_NWK_STEERING:
        switch (bdbCommissioningModeMsg->bdbCommissioningStatus)
        {
        case BDB_COMMISSIONING_SUCCESS:
            HalLedBlink(HAL_LED_1, 5, 50, 500);
            DBG("BDB_COMMISSIONING_SUCCESS\r\n");
            zclCommissioning_OnConnect();
            break;

        default:
            HalLedSet(HAL_LED_1, HAL_LED_MODE_BLINK);
            break;
        }
        break;

    case BDB_COMMISSIONING_PARENT_LOST:
        DBG("BDB_COMMISSIONING_PARENT_LOST\r\n");
        switch (bdbCommissioningModeMsg->bdbCommissioningStatus)
        {
        case BDB_COMMISSIONING_NETWORK_RESTORED:
            zclCommissioning_ResetBackoffRetry();
            break;

        default:
            HalLedSet(HAL_LED_1, HAL_LED_MODE_BLINK);
            // // Parent not found, attempt to rejoin again after a exponential backoff delay
            DBGF("rejoinsLeft %d rejoinDelay=%ld\r\n", rejoinsLeft, rejoinDelay);
            if (rejoinsLeft > 0) {
                APP_COMMISSIONING_END_DEVICE_REJOIN_BACKOFF(rejoinDelay);
                rejoinsLeft -= 1;
            } else {
                rejoinDelay = APP_COMMISSIONING_END_DEVICE_REJOIN_MAX_DELAY;
            }
            osal_start_timerEx(zclCommissioning_TaskId, EVT_COMMISSIONING_END_DEVICE_REJOIN, rejoinDelay);
            break;
        }
        break;

    default:
        break;
    }
}

/**************************************************************************************************
 * @fn      zclCommissioning_BindNotification
 *
 * @brief   Bind notification callback function
 *
 * @param   bdbBindNotificationData - bind notification data
 *
 * @return  None
 **************************************************************************************************/
static void zclCommissioning_BindNotification(bdbBindNotificationData_t *bdbBindNotificationData)
{
    HalLedSet(HAL_LED_1, HAL_LED_MODE_BLINK);
    DBGF("Recieved bind request clusterId=0x%X dstAddr=0x%X ep=%d\r\n",
        bdbBindNotificationData->clusterId, bdbBindNotificationData->dstAddr,
        bdbBindNotificationData->ep);
    uint16 maxEntries = 0, usedEntries = 0;
    bindCapacity(&maxEntries, &usedEntries);
    DBGF("bindCapacity %d %usedEntries %d \r\n", maxEntries, usedEntries);
}

/**************************************************************************************************
 * @fn      zclCommissioning_ResetBackoffRetry
 *
 * @brief   Reset commissioning retry counter and delay
 *
 * @param   None
 *
 * @return  None
 **************************************************************************************************/
static void zclCommissioning_ResetBackoffRetry(void)
{
    rejoinsLeft = APP_COMMISSIONING_END_DEVICE_REJOIN_TRIES;
    rejoinDelay = APP_COMMISSIONING_END_DEVICE_REJOIN_START_DELAY;
}

/**************************************************************************************************
 * @fn      zclCommissioning_OnConnect
 *
 * @brief   Commissioning success routine
 *
 * @param   None
 *
 * @return  None
 **************************************************************************************************/
static void zclCommissioning_OnConnect(void)
{
    DBG("zclCommissioning_OnConnect \r\n");
    zclCommissioning_ResetBackoffRetry();
    osal_start_timerEx(zclCommissioning_TaskId, EVT_COMMISSIONING_CLOCK_DOWN_POLING_RATE, 10 * 1000);
}

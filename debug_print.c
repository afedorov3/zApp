#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "debug_print.h"

#ifndef DEBUG_PRINT_FORMAT_BUFLEN
#define DEBUG_PRINT_FORMAT_BUFLEN 100
#endif /* DEBUG_PRINT_FORMAT_BUFLEN */

#if defined(DEBUG_PRINT_UART) || defined(DEBUG_PRINT_MT)
#include "hal_assert.h"
#include "hal_defs.h"   /* st() */
#include "OnBoard.h"    /* MicroWait() */

#define DEBUG_PRINT_ASSERT()                                  \
    st (                                                      \
        static const uint8 msg[] = "!!ERROR DBGF overflow!!"; \
        DBG(msg);                                             \
        MicroWait(30000);                                     \
        halAssertHandler();                                   \
       )
#endif /* DEBUG_PRINT_UART || DEBUG_PRINT_MT */

#if defined(DEBUG_PRINT_UART)
#include "hal_uart.h"

#ifndef DEBUG_PRINT_UART_PORT
#define DEBUG_PRINT_UART_PORT HAL_UART_PORT_0
#endif /* DEBUG_PRINT_UART_PORT */
#ifndef DEBUG_PRINT_UART_BUFFLEN
#define DEBUG_PRINT_UART_BUFFLEN 128
#endif /* DEBUG_PRINT_UART_BUFFLEN */

bool DebugInit()
{
    halUARTCfg_t halUARTConfig;
    halUARTConfig.configured = TRUE;
    halUARTConfig.baudRate = HAL_UART_BR_115200;
    halUARTConfig.flowControl = FALSE;
    halUARTConfig.flowControlThreshold = 48; // this parameter indicates number of bytes left before Rx Buffer
                                             // reaches maxRxBufSize
    halUARTConfig.idleTimeout = 10;          // this parameter indicates rx timeout period in millisecond
    halUARTConfig.rx.maxBufSize = 0;
    halUARTConfig.tx.maxBufSize = DEBUG_PRINT_UART_BUFFLEN;
    halUARTConfig.intEnable = TRUE;
    halUARTConfig.callBackFunc = NULL;
    HalUARTInit();
    if (HalUARTOpen(DEBUG_PRINT_UART_PORT, &halUARTConfig) == HAL_UART_SUCCESS)
    {
        DBG("Initialized UART debug\r\n");
        return true;
    }
    return false;
}

void DBG(const uint8 *data)
{
    if (data == NULL)
        return;

    HalUARTWrite(DEBUG_PRINT_UART_PORT, (uint8*)data, strlen((const char *)data));
}

void DBGF(const char *format, ...)
{
    uint8 str[DEBUG_PRINT_FORMAT_BUFLEN];
    int cnt;

    va_list argp;
    va_start(argp, format);
    cnt = vsprintf((char *)str, format, argp);
    if (cnt >= DEBUG_PRINT_FORMAT_BUFLEN)
        DEBUG_PRINT_ASSERT();
    else if (cnt > 0)
        HalUARTWrite(DEBUG_PRINT_UART_PORT, str, cnt);
    va_end(argp);
}

#elif defined(DEBUG_PRINT_MT)
#include "MT.h"          /* debugThreshold */
#include "DebugTrace.h"  /* debug_str() */

bool DebugInit()
{
    debugThreshold = 0x04; // increase threshold as soon as we initialize debug module
    DBG("Initialized MT debug\r\n");
    return true;
}

void DBG(const uint8 *data)
{
    debug_str(data);
}

void DBGF(char *format, ...)
{
    uint8 str[DEBUG_PRINT_FORMAT_BUFLEN];

    va_list argp;
    va_start(argp, format);
    cnt = vsprintf((char *)str, format, argp);
    if (cnt >= DEBUG_PRINT_FORMAT_BUFLEN)
        DEBUG_PRINT_ASSERT();
    else if (cnt > 0)
        debug_str(data);
    va_end(argp);
}

#elif defined(DEBUG_PRINT_STDIO)

bool DebugInit()
{
    return true;
}

void DBG(uint8 *data)
{
    printf((const char*)data);
}

void DBGF(char *format, ...)
{
    va_list argp;
    va_start(argp, format);
    printf(format, argp);
    va_end(argp);
}

#endif /* DEBUG_PRINT_STDIO */

#ifndef DEBUG_PRINT_H
#define DEBUG_PRINT_H

#include "hal_types.h"

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)  \
  (byte & 0x80 ? '1' : '0'), \
  (byte & 0x40 ? '1' : '0'), \
  (byte & 0x20 ? '1' : '0'), \
  (byte & 0x10 ? '1' : '0'), \
  (byte & 0x08 ? '1' : '0'), \
  (byte & 0x04 ? '1' : '0'), \
  (byte & 0x02 ? '1' : '0'), \
  (byte & 0x01 ? '1' : '0')

#if defined(DEBUG_PRINT_UART) || defined(DEBUG_PRINT_MT) || defined(DEBUG_PRINT_STDIO)
extern bool DebugInit(void);
extern void DBG(const uint8 *data);
extern void DBGF(const char *format, ...);
#else /* DEBUG_PRINT_UART || DEBUG_PRINT_MT || DEBUG_PRINT_STDIO */
#define DebugInit()
#define DBG(s)
#define DBGF(f, ...)
#endif /* !DEBUG_PRINT */

#endif /* DEBUG_PRINT_H */

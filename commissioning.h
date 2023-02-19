#ifndef COMMISSIONING_H
#define COMMISSIONING_H

#include "hal_defs.h"

extern void zclCommissioning_Init(uint8 task_id);
extern uint16 zclCommissioning_event_loop(uint8 task_id, uint16 events);
extern void zclCommissioning_Sleep( uint8 allow );
extern void zclCommissioning_HandleKeys(uint8 portAndAction, uint8 keyCode);

#endif /* COMMISSIONING_H */

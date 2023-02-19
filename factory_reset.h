#ifndef FACTORY_RESET_H
#define FACTORY_RESET_H

#include "hal_defs.h"

extern void zclFactoryResetter_Init(uint8 task_id);
extern uint16 zclFactoryResetter_event_loop(uint8 task_id, uint16 events);
extern void zclFactoryResetter_HandleKeys(uint8 portAndAction, uint8 keyCode);

#endif

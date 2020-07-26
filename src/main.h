#include <stdbool.h>
#ifndef _MAIN_H_
#define _MAIN_H_

#define INVALID_PID -1

#define HOOKS_NUM 20

extern SceUID processId;
extern char titleid[32];
extern bool is_in_pspemu;

extern uint8_t used_funcs[HOOKS_NUM];
extern uint16_t TOUCH_SIZE[4];
extern uint8_t internal_touch_call;
extern uint8_t internal_ext_call;
extern void delayedStart();

#endif

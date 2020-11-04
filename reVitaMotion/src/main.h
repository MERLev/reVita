#ifndef _MAIN_H_
#define _MAIN_H_

#define HOOKS_NUM         17 // Hooked functions num

extern char titleid[16];
extern uint8_t used_funcs[HOOKS_NUM];
extern uint16_t TOUCH_SIZE[4];

extern int model;
extern uint8_t internal_touch_call;
extern uint8_t internal_ext_call;

extern void delayedStart();

#endif
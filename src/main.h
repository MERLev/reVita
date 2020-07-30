#ifndef _MAIN_H_
#define _MAIN_H_
#include <stdbool.h>

#define HOOKS_NUM 20

char titleid[32];

uint8_t used_funcs[HOOKS_NUM];
bool isInternalTouchCall;
bool isInternalExtCall;

#endif

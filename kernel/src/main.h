#ifndef _MAIN_H_
#define _MAIN_H_
#include <stdbool.h>

#define HOOKS_NUM 20

extern char titleid[32];

extern bool used_funcs[HOOKS_NUM];
extern bool isInternalTouchCall;
extern bool isInternalExtCall;

#endif

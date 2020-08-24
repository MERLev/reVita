#ifndef _MAIN_H_
#define _MAIN_H_
#include <stdbool.h>

#define HOOKS_NUM                   22
#define TOUCH_HOOKS_NUM 			4
#define CTRL_HOOKS_NUM 				HOOKS_NUM - TOUCH_HOOKS_NUM - 2

extern char titleid[32];

extern bool used_funcs[HOOKS_NUM];
extern bool isInternalTouchCall;
extern bool isInternalExtCall;

#endif

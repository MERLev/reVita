#ifndef _REMAP_H_
#define _REMAP_H_

#include <psp2/touch.h>
#define BUFFERS_NUM         64

typedef struct TouchPoint{
	uint16_t x, y;
}TouchPoint;

typedef union RemapRuleParam{
	TouchPoint touch;
}RemapRuleParam;

typedef struct RemapAction{
	uint8_t type;
	uint8_t action;
	RemapRuleParam param;
}RemapAction;

typedef struct RemapRule{
	RemapAction trigger;
	RemapAction emu;
	uint8_t propagate;
	uint8_t active;
}Rule;

extern int remap_controls(SceCtrlData *ctrl, int nBufs, int hookId);
extern void remap_patchToExt(SceCtrlData *ctrl);
extern int remap_touch(SceUInt32 port, SceTouchData *pData, SceUInt32 nBufs, uint8_t hookId);
extern void remap_resetCtrlBuffers(uint8_t hookId);
extern void remap_resetTouchBuffers(uint8_t hookId);
extern void remap_init();
extern void remap_destroy();

#endif
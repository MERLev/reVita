#ifndef _REMAP_H_
#define _REMAP_H_

#include <psp2/touch.h>
#define BUFFERS_NUM         64

enum REMAP_ACTION_TYPE{
    REMAP_TYPE_BUTTON = 0,
    REMAP_TYPE_COMBO,
    REMAP_TYPE_LEFT_ANALOG,
    REMAP_TYPE_LEFT_ANALOG_DIGITAL,
    REMAP_TYPE_RIGHT_ANALOG,
    REMAP_TYPE_RIGHT_ANALOG_DIGITAL,
    REMAP_TYPE_FRONT_TOUCH_ZONE,
    REMAP_TYPE_BACK_TOUCH_ZONE,
    REMAP_TYPE_FRONT_TOUCH_POINT,
    REMAP_TYPE_BACK_TOUCH_POINT,
    REMAP_TYPE_GYROSCOPE,
    REMAP_ACTION_TYPE_NUM
};

enum REMAP_ACTION{
    REMAP_ANALOG_UP = 0,
    REMAP_ANALOG_DOWN,
    REMAP_ANALOG_LEFT,
    REMAP_ANALOG_RIGHT,
    REMAP_TOUCH_ZONE_TL,
    REMAP_TOUCH_ZONE_TR,
    REMAP_TOUCH_ZONE_BL,
    REMAP_TOUCH_ZONE_BR,
    REMAP_TOUCH_CUSTOM,
    REMAP_GYRO_UP,
    REMAP_GYRO_DOWN,
    REMAP_GYRO_LEFT,
    REMAP_GYRO_RIGHT,
    REMAP_GYRO_ROLL_LEFT,
    REMAP_GYRO_ROLL_RIGHT,
    REMAP_ACTION_NUM
};

typedef struct TouchPoint{
	uint16_t x, y;
}TouchPoint;

typedef union RemapRuleParam{
	TouchPoint touch;
    uint32_t btn;
}RemapRuleParam;

typedef struct RemapAction{
	enum REMAP_ACTION_TYPE type;
	enum REMAP_ACTION action;
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
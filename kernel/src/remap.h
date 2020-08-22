#ifndef _REMAP_H_
#define _REMAP_H_

#include <psp2/touch.h>
#define BUFFERS_NUM         64

enum REMAP_ACTION_TYPE{
    REMAP_TYPE_BUTTON = 0,
    REMAP_TYPE_LEFT_ANALOG,
    REMAP_TYPE_LEFT_ANALOG_DIGITAL,
    REMAP_TYPE_RIGHT_ANALOG,
    REMAP_TYPE_RIGHT_ANALOG_DIGITAL,
    REMAP_TYPE_FRONT_TOUCH_ZONE,
    REMAP_TYPE_BACK_TOUCH_ZONE,
    REMAP_TYPE_FRONT_TOUCH_POINT,
    REMAP_TYPE_BACK_TOUCH_POINT,
    REMAP_TYPE_GYROSCOPE,
    REMAP_TYPE_REMAPSV,
    REMAP_TYPE_USYSACTIONS,
    REMAP_ACTION_TYPE_NUM
};

enum REMAP_ACTION{
    REMAP_ANALOG_UP = 0,
    REMAP_ANALOG_DOWN,
    REMAP_ANALOG_LEFT,
    REMAP_ANALOG_RIGHT,
    REMAP_TOUCH_ZONE_L,
    REMAP_TOUCH_ZONE_R,
    REMAP_TOUCH_ZONE_TL,
    REMAP_TOUCH_ZONE_TR,
    REMAP_TOUCH_ZONE_BL,
    REMAP_TOUCH_ZONE_BR,
    REMAP_TOUCH_CUSTOM,
    REMAP_TOUCH_SWIPE,
    REMAP_TOUCH_SWIPE_SMART_L,
    REMAP_TOUCH_SWIPE_SMART_R,
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

typedef struct TouchPoints2{
	TouchPoint a, b;
}TouchPoints2;

typedef struct RemapRuleParam{
	TouchPoint tPoint;
    TouchPoints2 tPoints;
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
	bool propagate;
    bool turbo;
	bool disabled;
}Rule;

#define HW_BUTTONS_NUM    21 // Supported physical buttons num
extern const uint32_t HW_BUTTONS[HW_BUTTONS_NUM];
extern TouchPoint T_FRONT_SIZE;
extern TouchPoint T_BACK_SIZE;

int remap_controls(SceCtrlData *ctrl, int nBufs, int hookId);
void remap_patchToExt(SceCtrlData *ctrl);
int remap_touch(SceUInt32 port, SceTouchData *pData, SceUInt32 nBufs, uint8_t hookId);
void remap_resetBuffers();
void remap_init();
void remap_destroy();
void remap_setup();

#endif
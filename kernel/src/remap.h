#ifndef _REMAP_H_
#define _REMAP_H_

#include <psp2/touch.h>
#define BUFFERS_NUM             64
#define MULTITOUCH_FRONT_NUM    6
#define MULTITOUCH_BACK_NUM		4

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
    REMAP_TYPE_SYSACTIONS,
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
    REMAP_TOUCH_SWIPE_SMART_DPAD,
    REMAP_GYRO_UP,
    REMAP_GYRO_DOWN,
    REMAP_GYRO_LEFT,
    REMAP_GYRO_RIGHT,
    REMAP_GYRO_ROLL_LEFT,
    REMAP_GYRO_ROLL_RIGHT,
    REMAP_SYS_RESET_SOFT,
    REMAP_SYS_RESET_COLD,
    REMAP_SYS_STANDBY,
    REMAP_SYS_SUSPEND,
    REMAP_SYS_DISPLAY_OFF,
    REMAP_SYS_KILL,
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

typedef struct EmulatedStick{
	uint32_t up, down, left, right;
}EmulatedStick;

typedef struct RuleData{
	SceCtrlData *ctrl;
	uint32_t btns, btnsEmu, btnsProp;
	struct RemapRule* rr;
	EmulatedStick analogLeftEmu, analogRightEmu, analogLeftProp, analogRightProp;
	uint8_t stickposval;
	enum RULE_STATUS* status;
	bool isTurboTick;
}RuleData;

typedef struct EmulatedTouchEvent{
	TouchPoint point, swipeEndPoint, swipeCurrentPoint;
	int id;
	int64_t tick;
	bool isSwipe;
	bool isSmartSwipe;
	bool isSwipeFinished;
}EmulatedTouchEvent;

typedef struct EmulatedTouch{
	EmulatedTouchEvent reports[MULTITOUCH_FRONT_NUM];
	uint8_t num;
}EmulatedTouch;

#define HW_BUTTONS_NUM    21 // Supported physical buttons num
extern const uint32_t HW_BUTTONS[HW_BUTTONS_NUM];
extern TouchPoints2 T_FRONT_SIZE;
extern TouchPoints2 T_BACK_SIZE;

struct RemapRule remap_createRemapRule();
int remap_controls(SceCtrlData *ctrl, int nBufs, int hookId);
void remap_patchToExt(SceCtrlData *ctrl);
int remap_touch(SceUInt32 port, SceTouchData *pData, SceUInt32 nBufs, uint8_t hookId);
void remap_resetBuffers();
void remap_init();
void remap_destroy();
void remap_setup();

#endif
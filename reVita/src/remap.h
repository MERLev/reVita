#ifndef _REMAP_H_
#define _REMAP_H_

#include <psp2/touch.h>
#include <stdbool.h>

#define BUFFERS_NUM             64
#define PORTS_NUM               5
#define PROC_NUM                2
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
    REMAP_TYPE_SYSACTIONS,
    REMAP_TYPE_REMAPSV_ACTIONS,
    REMAP_TYPE_DISABLED,
    REMAP_ACTION_TYPE_NUM
};

enum REMAP_ACTION{
    REMAP_ANALOG_UP = 0,
    REMAP_ANALOG_DOWN,
    REMAP_ANALOG_LEFT,
    REMAP_ANALOG_RIGHT,
    REMAP_TOUCH_ZONE_FULL,
    REMAP_TOUCH_ZONE_CENTER,
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
    REMAP_GYRO_SIM_UP,
    REMAP_GYRO_SIM_DOWN,
    REMAP_GYRO_SIM_LEFT,
    REMAP_GYRO_SIM_RIGHT,
    REMAP_GYRO_SIM_ROLL_LEFT,
    REMAP_GYRO_SIM_ROLL_RIGHT,
    REMAP_SYS_RESET_SOFT,
    REMAP_SYS_RESET_COLD,
    REMAP_SYS_STANDBY,
    REMAP_SYS_SUSPEND,
    REMAP_SYS_DISPLAY_OFF,
    REMAP_SYS_KILL,
    REMAP_SYS_BRIGHTNESS_INC,
    REMAP_SYS_BRIGHTNESS_DEC,
    REMAP_SYS_SAVE_BACKUP,
    REMAP_SYS_SAVE_RESTORE,
    REMAP_SYS_SAVE_DELETE,
    REMAP_SYS_CALIBRATE_MOTION,
    REMAP_SYS_TOGGLE_SECONDARY,
    REMAP_REM_SWAP_TOUCHPADS,
    REMAP_ACTION_NUM
};

enum TURBO_MODE{
    TURBO_DISABLED = 0,
    TURBO_SLOW,
    TURBO_MEDIUM,
    TURBO_FAST
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
    enum TURBO_MODE turbo;
    bool sticky;
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
	// bool isTurboTick;
    SceInt64* tickPressed;
    int port;
    int idx;
    bool* isSticky;
    bool gyroAnalogDeadzones[2];
}RuleData;

typedef struct EmulatedTouchEvent{
	TouchPoint point, swipeEndPoint, swipeCurrentPoint;
	int id;
	int64_t tick;
	bool isSwipe;
	bool isSmartSwipe;
	bool isSwipeFinished;
    int port;
    int ruleIdx;
}EmulatedTouchEvent;

typedef struct EmulatedTouch{
	EmulatedTouchEvent reports[MULTITOUCH_FRONT_NUM];
	uint8_t num;
}EmulatedTouch;

#define HW_BUTTONS_NUM    23 // Supported physical buttons num
extern const uint32_t HW_BUTTONS[HW_BUTTONS_NUM];
extern TouchPoints2 T_SIZE[SCE_TOUCH_PORT_MAX_NUM];

struct RemapRule remap_createRemapRule();

int remap_ctrl_getBufferNum(int port);
void remap_ctrl_updateBuffers(int port, SceCtrlData *ctrl, bool isPositiveLogic, bool isExt);
int remap_ctrl_readBuffer(int port, SceCtrlData *ctrl, int buffIdx, bool isPositiveLogic, bool isExt);

void remap_patchToExt(SceCtrlData *ctrl);
int remap_touch(SceUInt32 port, SceTouchData *pData, SceUInt32 nBufs, uint8_t hookId, 
        SceTouchData** remappedBuffers);
int remap_touchRegion(SceUInt32 port, SceTouchData *pData, SceUInt32 nBufs, uint8_t hookId);
void remap_resetBuffers();
void remap_init();
void remap_destroy();
void remap_setup();
void remap_swapSideButtons(SceCtrlData *ctrl);
void remap_fixSideButtons(SceCtrlData *ctrl);

#endif
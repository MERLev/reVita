#include "remap.h"

#ifndef _PROFILE_H_
#define _PROFILE_H_

#define REMAP_NUM                   25
#define TARGET_REMAPS               42 // Supported target remaps num

#define PROFILE_REMAP_NUM           38 // Supported buttons num
#define PROFILE_ANALOG_NUM          8
#define PROFILE_GYRO_NUM			10
#define PROFILE_TOUCH_NUM			18
#define PROFILE_CONTROLLER_NUM		3
#define PROFILE_SETTINGS_NUM		4

#define PROFILE_REMAP_DEF           16

enum{
	PROFILE_ANALOG_LEFT_DEADZONE_X = 0,
	PROFILE_ANALOG_LEFT_DEADZONE_Y,
	PROFILE_ANALOG_RIGHT_DEADZONE_X,
	PROFILE_ANALOG_RIGHT_DEADZONE_Y,
	PROFILE_ANALOG_LEFT_DIGITAL_X,
	PROFILE_ANALOG_LEFT_DIGITAL_Y,
	PROFILE_ANALOG_RIGHT_DIGITAL_X,
	PROFILE_ANALOG_RIGHT_DIGITAL_Y
};
enum{
	PROFILE_TOUCH_FRONT_POINT1_X = 0,
	PROFILE_TOUCH_FRONT_POINT1_Y,
	PROFILE_TOUCH_FRONT_POINT2_X,
	PROFILE_TOUCH_FRONT_POINT2_Y,
	PROFILE_TOUCH_FRONT_POINT3_X,
	PROFILE_TOUCH_FRONT_POINT3_Y,
	PROFILE_TOUCH_FRONT_POINT4_X,
	PROFILE_TOUCH_FRONT_POINT4_Y,
	PROFILE_TOUCH_REAR_POINT1_X,
	PROFILE_TOUCH_REAR_POINT1_Y,
	PROFILE_TOUCH_REAR_POINT2_X,
	PROFILE_TOUCH_REAR_POINT2_Y,
	PROFILE_TOUCH_REAR_POINT3_X,
	PROFILE_TOUCH_REAR_POINT3_Y,
	PROFILE_TOUCH_REAR_POINT4_X,
	PROFILE_TOUCH_REAR_POINT4_Y,
    PROFILE_TOUCH_FRONT_DISABLE,
    PROFILE_TOUCH_REAR_DISABLE
};
enum{
	PROFILE_GYRO_SENSIVITY_X = 0,
	PROFILE_GYRO_SENSIVITY_Y,
	PROFILE_GYRO_SENSIVITY_Z,
	PROFILE_GYRO_DEADZONE_X,
	PROFILE_GYRO_DEADZONE_Y,
	PROFILE_GYRO_DEADZONE_Z,
	PROFILE_GYRO_DEADBAND,
	PROFILE_GYRO_WHEEL,
	PROFILE_GYRO_RESET_BTN1,
	PROFILE_GYRO_RESET_BTN2
};
enum{
	PROFILE_CONTROLLER_ENABLED = 0,
	PROFILE_CONTROLLER_PORT,
	PROFILE_CONTROLLER_SWAP_BUTTONS
};
enum{
	PROFILE_SETTINGS_KEY1 = 0,
	PROFILE_SETTINGS_KEY2,
	PROFILE_SETTINGS_AUTOSAVE,
	PROFILE_SETTINGS_DELAY
};
typedef struct Profile{
    struct RemapRule remaps[REMAP_NUM];
    uint8_t remapsNum;
    uint8_t analog[PROFILE_ANALOG_NUM];
    uint8_t gyro[PROFILE_GYRO_NUM];
    uint16_t touch[PROFILE_TOUCH_NUM];
    uint8_t controller[PROFILE_CONTROLLER_NUM];
}Profile;

extern Profile profile;
extern Profile profile_def;
extern Profile profile_global;
extern uint8_t profile_settings[PROFILE_SETTINGS_NUM];
extern uint8_t profile_settings_def[PROFILE_SETTINGS_NUM];

void profile_addRemapRule(struct RemapRule rule);
void profile_removeRemapRule(uint8_t idx);
void profile_resetRemapRules();

extern void profile_resetAnalog();
extern void profile_resetTouch();
extern void profile_resetGyro();
extern void profile_resetController();
extern void profile_resetSettings();

int profile_saveSettings();
int profile_loadSettings();

int profile_save(char* titleId);
int profile_saveAsGlobal();
int profile_saveHome();
int profile_load(char* titleId);
int profile_loadGlobal();
void profile_loadGlobalCached();
int profile_loadHome();
void profile_loadHomeCached();

extern void profile_resetGlobal();
extern void profile_delete(char* titleid);

extern void profile_init();
extern void profile_destroy();

#endif
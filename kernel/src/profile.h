#ifndef _PROFILE_H_
#define _PROFILE_H_

#include "remap.h"

#define HOME "HOME"

#define REMAP_NUM                   25

enum PROFILE_ANALOG_ID{
	PROFILE_ANALOG_LEFT_DEADZONE_X = 0,
	PROFILE_ANALOG_LEFT_DEADZONE_Y,
	PROFILE_ANALOG_RIGHT_DEADZONE_X,
	PROFILE_ANALOG_RIGHT_DEADZONE_Y,
    PROFILE_ANALOG__NUM
};
enum PROFILE_TOUCH_ID{
	PROFILE_TOUCH_PSTV_MODE = 0,
	PROFILE_TOUCH_SWAP,
	PROFILE_TOUCH_SWIPE_DURATION,
	PROFILE_TOUCH_SWIPE_SMART_SENSIVITY,
	PROFILE_TOUCH_DRAW_POINTER_POINT,
	PROFILE_TOUCH_DRAW_POINTER_SWIPE,
	PROFILE_TOUCH_DRAW_POINTER_SMART_SWIPE,
    PROFILE_TOUCH__NUM
};
enum PROFILE_GYRO_ID{
	PROFILE_GYRO_SENSIVITY_X = 0,
	PROFILE_GYRO_SENSIVITY_Y,
	PROFILE_GYRO_SENSIVITY_Z,
	PROFILE_GYRO_DEADZONE_X,
	PROFILE_GYRO_DEADZONE_Y,
	PROFILE_GYRO_DEADZONE_Z,
	PROFILE_GYRO_DEADBAND,
	PROFILE_GYRO_WHEEL,
	PROFILE_GYRO_RESET_BTN1,
	PROFILE_GYRO_RESET_BTN2,
    PROFILE_GYRO__NUM
};
enum PROFILE_CONTROLLER_ID{
	PROFILE_CONTROLLER_ENABLED = 0,
	PROFILE_CONTROLLER_PORT,
	PROFILE_CONTROLLER_SWAP_BUTTONS,
    PROFILE_CONTROLLER__NUM
};
enum PROFILE_SETTINGS_ID{
	PROFILE_SETTINGS_KEY1 = 0,
	PROFILE_SETTINGS_KEY2,
	PROFILE_SETTINGS_AUTOSAVE,
	PROFILE_SETTINGS_DELAY,
    PROFILE_SETTINGS__NUM
};

typedef struct Profile{
	char titleid[32];
	int version;
    uint8_t remapsNum;
    struct RemapRule remaps[REMAP_NUM];
    uint8_t analog[PROFILE_ANALOG__NUM];
    uint8_t gyro[PROFILE_GYRO__NUM];
    uint16_t touch[PROFILE_TOUCH__NUM];
    uint8_t controller[PROFILE_CONTROLLER__NUM];
}Profile;

extern Profile profile;
extern Profile profile_def;
extern Profile profile_global;
extern Profile profile_home;
extern int32_t profile_settings[PROFILE_SETTINGS__NUM];
extern int32_t profile_settings_def[PROFILE_SETTINGS__NUM];

void profile_addRemapRule(struct RemapRule rule);
void profile_removeRemapRule(uint8_t idx);
void profile_resetRemapRules();

void profile_resetAnalog();
void profile_resetTouch();
void profile_resetGyro();
void profile_resetController();
void profile_resetSettings();
struct Profile createProfile();

bool profile_saveSettings();
bool profile_loadSettings();

bool profile_save(char* titleId);
bool profile_load(char* titleId);

void profile_localSave();
void profile_localLoad();
void profile_localReset();
void profile_localDelete();

void profile_saveAsGlobal();
void profile_loadFromGlobal();
void profile_resetGlobal();

void profile_init();
void profile_destroy();

#endif
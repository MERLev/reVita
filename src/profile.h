#include "remap.h"

#ifndef _PROFILE_H_
#define _PROFILE_H_

#define HOME "HOME"

#define REMAP_NUM                   25

#define PROFILE_ANALOG_NUM          8
#define PROFILE_GYRO_NUM			10
#define PROFILE_TOUCH_NUM			18
#define PROFILE_CONTROLLER_NUM		3
#define PROFILE_SETTINGS_NUM		4

enum PROFILE_ANALOG_ID{
	PROFILE_ANALOG_LEFT_DEADZONE_X = 0,
	PROFILE_ANALOG_LEFT_DEADZONE_Y,
	PROFILE_ANALOG_RIGHT_DEADZONE_X,
	PROFILE_ANALOG_RIGHT_DEADZONE_Y,
    PROFILE_ANALOG__NUM
}PROFILE_ANALOG_ID;
enum PROFILE_TOUCH_ID{
	PROFILE_TOUCH_SWAP = 0,
    PROFILE_TOUCH__NUM
}PROFILE_TOUCH_ID;
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
}PROFILE_GYRO_ID;
enum PROFILE_CONTROLLER_ID{
	PROFILE_CONTROLLER_ENABLED = 0,
	PROFILE_CONTROLLER_PORT,
	PROFILE_CONTROLLER_SWAP_BUTTONS,
    PROFILE_CONTROLLER__NUM
}PROFILE_CONTROLLER_ID;
enum PROFILE_SETTINGS_ID{
	PROFILE_SETTINGS_KEY1 = 0,
	PROFILE_SETTINGS_KEY2,
	PROFILE_SETTINGS_AUTOSAVE,
	PROFILE_SETTINGS_DELAY,
    PROFILE_SETTINGS__NUM
}PROFILE_SETTINGS_ID;

typedef struct Profile{
	char titleid[32];
    uint8_t remapsNum;
    struct RemapRule remaps[REMAP_NUM];
    uint8_t analog[PROFILE_ANALOG_NUM];
    uint8_t gyro[PROFILE_GYRO_NUM];
    uint16_t touch[PROFILE_TOUCH_NUM];
    uint8_t controller[PROFILE_CONTROLLER_NUM];
}Profile;

Profile profile;
Profile profile_def;
Profile profile_global;
int32_t profile_settings[PROFILE_SETTINGS_NUM];
int32_t profile_settings_def[PROFILE_SETTINGS_NUM];

void profile_addRemapRule(struct RemapRule rule);
void profile_removeRemapRule(uint8_t idx);
void profile_resetRemapRules();

void profile_resetAnalog();
void profile_resetTouch();
void profile_resetGyro();
void profile_resetController();
void profile_resetSettings();

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
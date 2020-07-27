#ifndef _PROFILE_H_
#define _PROFILE_H_

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
    uint8_t remap[PROFILE_REMAP_NUM];
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

extern void profile_resetRemap();
extern void profile_resetAnalog();
extern void profile_resetTouch();
extern void profile_resetGyro();
extern void profile_resetController();
extern void profile_resetSettings();

extern void profile_saveSettings();
extern void profile_loadSettings();
extern void profile_saveGlobal();
extern void profile_saveLocal();
extern void profile_loadGlobal();
extern void profile_loadLocal();
extern void profile_deleteGlobal();
extern void profile_deleteLocal();

extern void profile_init();
extern void profile_destroy();

#endif
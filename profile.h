#ifndef _PROFILE_H_
#define _PROFILE_H_

#define BUTTONS_NUM       			38 // Supported buttons num
#define ANOLOGS_OPTIONS_NUM			8
#define GYRO_OPTIONS_NUM			11
#define TOUCH_OPTIONS_NUM			18
#define CNTRL_OPTIONS_NUM			3
#define SETTINGS_NUM				4

#define MULTITOUCH_FRONT_NUM		6
#define MULTITOUCH_REAR_NUM			4

#define REMAP_DEF                   16 // Supported physical buttons num
#define ANALOGS_DEADZONE_DEF		30
#define ANALOGS_FORCE_DIGITAL_DEF	0
#define TOUCH_MODE_DEF				1

extern const uint16_t TOUCH_POINTS_DEF[16];
extern const uint8_t GYRO_DEF[GYRO_OPTIONS_NUM];
extern const uint8_t CNTRL_DEF[CNTRL_OPTIONS_NUM];
extern const uint8_t SETTINGS_DEF[SETTINGS_NUM];

extern uint8_t btn_mask[BUTTONS_NUM];
extern uint8_t analogs_options[ANOLOGS_OPTIONS_NUM];
extern uint8_t gyro_options[GYRO_OPTIONS_NUM];
extern uint16_t touch_options[TOUCH_OPTIONS_NUM];
extern uint8_t controller_options[CNTRL_OPTIONS_NUM];
extern uint8_t settings_options[SETTINGS_NUM];

extern void resetRemapsOptions();
extern void resetAnalogsOptions();
extern void resetTouchOptions();
extern void resetGyroOptions();
extern void resetCntrlOptions();
extern void resetSettingsOptions();

extern void saveSettings();
extern void loadSettings();
extern void saveGlobalConfig(void);
extern void saveGameConfig(void);
extern void loadGlobalConfig(void);
extern void loadGameConfig(void);

#endif
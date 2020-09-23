#ifndef _PROFILE_H_
#define _PROFILE_H_

#include "../remap.h"

#define HOME "HOME"
#define GLOBAL "GLOBAL"

#define REMAP_NUM                   25

enum PROF_ID{
	PR_AN_LEFT_DEADZONE_X = 0,
	PR_AN_LEFT_DEADZONE_Y,
	PR_AN_RIGHT_DEADZONE_X,
	PR_AN_RIGHT_DEADZONE_Y,
	PR_AN_MODE_WIDE,
	PR_TO_SWAP,
	PR_TO_SWIPE_DURATION,
	PR_TO_SWIPE_SMART_SENS,
	PR_TO_DRAW_POINT,
	PR_TO_DRAW_SWIPE,
	PR_TO_DRAW_SMART_SWIPE,
	PR_TO_DRAW_NATIVE,
	PR_GY_SENSIVITY_X,
	PR_GY_SENSIVITY_Y,
	PR_GY_SENSIVITY_Z,
	PR_GY_DEADZONE_X,
	PR_GY_DEADZONE_Y,
	PR_GY_DEADZONE_Z,
	PR_GY_DEADBAND,
	PR_GY_WHEEL,
	PR_CO_PATCH_EXT,
	PR_CO_PATCH_SYS,
	PR_CO_SWAP_BUTTONS,
	PR_CO_FIX_BUTTONS,
	PR_CO_EMULATE_DS4,
    PROF__NUM
};

enum PROFILE_ENTRY_TYPE{
	TYPE_INT32,
	TYPE_UINT32,
	TYPE_BOOL
};

typedef union ProfileEntryVal{
	unsigned int u;
	int i;
	bool b;
}ProfileEntryVal;

typedef struct ProfileEntry{
	uint32_t id;
	enum PROFILE_ENTRY_TYPE type;
	union ProfileEntryVal v;
	union ProfileEntryVal def;
	union ProfileEntryVal min;
	union ProfileEntryVal max;
	char* key;
}ProfileEntry;

typedef struct Profile{
	char titleid[32];
	int version;
    uint8_t remapsNum;
    struct RemapRule remaps[REMAP_NUM];
    struct ProfileEntry entries[PROF__NUM];
}Profile;

//ToDo remove those
extern const char*  HW_BUTTONS_STR[HW_BUTTONS_NUM];
int getButtonId(char* n);

extern Profile profile;
extern Profile profile_global;
extern Profile profile_home;

void profile_inc(ProfileEntry* pe, unsigned int val);
void profile_dec(ProfileEntry* pe, unsigned int val);

void profile_addRemapRule(struct RemapRule rule);
void profile_removeRemapRule(uint8_t idx);
void profile_resetRemapRules();
bool profile_isDef(ProfileEntry* pe);
void profile_resetEntry(ProfileEntry* entry);
void profile_resetAnalog();
void profile_resetTouch();
void profile_resetGyro();
void profile_resetController();
struct Profile createProfile();

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
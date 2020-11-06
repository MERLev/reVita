#ifndef _SETTINGS_H_
#define _SETTINGS_H_
#include "profile.h"

enum SETT_ID{
	SETT_REMAP_ENABLED = 0,
	SETT_AUTOSAVE,
	SETT_ANIMATION,
	// SETT_DELAY_INIT,
	SETT_THEME,
	POP_ALL,
	POP_SAVE,
	POP_LOAD,
	POP_BRIGHTNESS,
	POP_READY,
	POP_LOADING,
	POP_REVITA,
	POP_KILL,
    SETT__NUM
};

extern struct ProfileEntry settings[SETT__NUM];

bool settings_isDef(enum SETT_ID id);
void settings_reset(enum SETT_ID id);
void settings_resetAll();
void settings_resetAllPopups();
bool settings_save();

void settings_init();
void settings_destroy();
#endif
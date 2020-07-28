#include <vitasdkkern.h>
#include <taihen.h>
#include <stdio.h>
#include <stdbool.h>
#include <psp2kern/io/stat.h> 
#include "main.h"
#include "profile.h"
#include "log.h"

#define PATH "ux0:/data/remaPSV2"
#define NAME_GLOBAL "GLOBAL"
#define NAME_HOME "HOME"
#define NAME_SETTINGS "SETTINGS"
#define EXT "bin"

Profile profile;
Profile profile_def;
Profile profile_global;
Profile profile_home;
uint8_t profile_settings[PROFILE_SETTINGS_NUM];
uint8_t profile_settings_def[PROFILE_SETTINGS_NUM];

static char fname[128];

void profile_addRemapRule(struct RemapRule ui_ruleEdited){
	if (profile.remapsNum < (REMAP_NUM - 1)){
		profile.remaps[profile.remapsNum] = ui_ruleEdited;
		profile.remapsNum++;
	}
}

void profile_removeRemapRule(uint8_t idx){
	if (idx < 0 || idx >= profile.remapsNum)
		return;
	for (int i = idx; i < profile.remapsNum - 1; i++)
		profile.remaps[i] = profile.remaps[i + 1];
	profile.remapsNum--;
}

void profile_resetRemapRules(){
	profile.remapsNum = 0;
}

// Reset options per-menu
void profile_resetRemap(){
	for (int i = 0; i < PROFILE_REMAP_NUM; i++)
		profile.remap[i] = PROFILE_REMAP_DEF;
}
void profile_resetAnalog(){
	for (int i = 0; i < PROFILE_ANALOG_NUM; i++)
		profile.analog[i] = profile_def.analog[i];
}
void profile_resetTouch(){
	for (int i = 0; i < PROFILE_TOUCH_NUM; i++)
		profile.touch[i] = profile_def.touch[i];
}
void profile_resetGyro() {
	for (int i = 0; i < PROFILE_GYRO_NUM; i++)
		profile.gyro[i] = profile_def.gyro[i];
}
void profile_resetController(){
	for (int i = 0; i < PROFILE_CONTROLLER_NUM; i++)
		profile.controller[i] = profile_def.controller[i];
}
void profile_resetSettings(){
	for (int i = 0; i < PROFILE_SETTINGS_NUM; i++)
		profile_settings[i] = profile_settings_def[i];
}

int readProfile(struct Profile* p, char* name){
	SceUID fd;
	int ret;

	sprintf(fname, "%s/%s.%s", PATH, name, EXT);

	// Just in case the folder doesn't exist
	ret = ksceIoMkdir(PATH, 0777); 

	// Loading config file for the selected app if exists
	fd = ksceIoOpen(fname, SCE_O_RDONLY, 0777);
	//LOG("1: \n");
	if (!fd) return 0;
	ret = ksceIoRead(fd, p, sizeof(Profile));
	//LOG("2: %i\n", ret);
	if (ret < 0) return 0;
	ksceIoClose(fd);
	//LOG("3: %i\n", ret);
	if (ret < 0) return -0;
	return 1;
}
int writeProfile(struct Profile* p, char* name){
	SceUID fd;
	int ret;
	// Just in case the folder doesn't exist
	ksceIoMkdir(PATH, 0777); 
	
	// Opening remap config file and saving it
	sprintf(fname, "%s/%s.%s", PATH, name, EXT);
	fd = ksceIoOpen(fname, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
	if (!fd) return -0;
	ret = ksceIoWrite(fd, p, sizeof(Profile));
	if (ret < 0) return 0;
	ret = ksceIoClose(fd);
	if (ret < 0) return 0;
	return 1;
}
int profile_saveSettings(){
	int ret;
	// Just in case the folder doesn't exist
	ksceIoMkdir(PATH, 0777); 
	
	// Opening settings config file and saving the config
	sprintf(fname, "%s/%s.%s", PATH, NAME_SETTINGS, EXT);
	SceUID fd = ksceIoOpen(fname, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
	ret = ksceIoWrite(fd, profile_settings, PROFILE_SETTINGS_NUM);
	if (ret < 0) return 0;
	ret = ksceIoClose(fd);
	if (ret < 0) return 0;
	return 1;
}
int profile_loadSettings(){
	profile_resetSettings();
	int ret;
	// Just in case the folder doesn't exist
	ksceIoMkdir(PATH, 0777); 
	
	// Loading config file for the selected app if exists
	sprintf(fname, "%s/%s.%s", PATH, NAME_SETTINGS, EXT);
	SceUID fd = ksceIoOpen(fname, SCE_O_RDONLY, 0777);
	if (!fd) return 0;
	ret = ksceIoRead(fd, profile_settings, PROFILE_SETTINGS_NUM);
	if (ret < 0) return 0;
	ret = ksceIoClose(fd);
	if (ret < 0) return 0;
	return 1;
}

int profile_save(char* titleId) {
	return writeProfile(&profile, titleId);
}
int profile_saveAsGlobal() {
	profile_global = profile;
	return writeProfile(&profile_global, NAME_GLOBAL);
}
int profile_saveHome() {
	profile_home = profile;
	return writeProfile(&profile_home, NAME_HOME);
}

int profile_load(char* titleId) {
	if (readProfile(&profile, titleId)){
		return 1;
	}
	profile = profile_global;
	return -1;
}
int profile_loadGlobal() {
	return readProfile(&profile_global, NAME_GLOBAL);
}
void profile_loadGlobalCached(){
	profile = profile_global;
}
int profile_loadHome() {
	return readProfile(&profile_home, NAME_HOME);
}
void profile_loadHomeCached() {
	profile = profile_home;
}

void profile_delete(char* titleId){
	profile = profile_global;
	profile_save(titleId);
}
void profile_resetGlobal(){
	profile_global = profile_def;
	profile_saveAsGlobal();
}

void setDefProfile(){
	for (int i = 0; i < PROFILE_REMAP_NUM; i++)
		profile_def.remap[i] = PROFILE_REMAP_DEF;

	profile_def.analog[PROFILE_ANALOG_LEFT_DEADZONE_X] = 30;
	profile_def.analog[PROFILE_ANALOG_LEFT_DEADZONE_Y] = 30;
	profile_def.analog[PROFILE_ANALOG_RIGHT_DEADZONE_X] = 30;
	profile_def.analog[PROFILE_ANALOG_RIGHT_DEADZONE_Y] = 30;
	profile_def.analog[PROFILE_ANALOG_LEFT_DIGITAL_X] = 0;
	profile_def.analog[PROFILE_ANALOG_LEFT_DIGITAL_Y] = 0;
	profile_def.analog[PROFILE_ANALOG_RIGHT_DIGITAL_X] = 0;
	profile_def.analog[PROFILE_ANALOG_RIGHT_DIGITAL_Y] = 0;

	profile_def.touch[PROFILE_TOUCH_FRONT_POINT1_X] = 600;
	profile_def.touch[PROFILE_TOUCH_FRONT_POINT1_Y] = 272;
	profile_def.touch[PROFILE_TOUCH_FRONT_POINT2_X] = 1280;
	profile_def.touch[PROFILE_TOUCH_FRONT_POINT2_Y] = 272;
	profile_def.touch[PROFILE_TOUCH_FRONT_POINT3_X] = 600;
	profile_def.touch[PROFILE_TOUCH_FRONT_POINT3_Y] = 816;
	profile_def.touch[PROFILE_TOUCH_FRONT_POINT4_X] = 1280;
	profile_def.touch[PROFILE_TOUCH_FRONT_POINT4_Y] = 816;
	profile_def.touch[PROFILE_TOUCH_REAR_POINT1_X] = 600;
	profile_def.touch[PROFILE_TOUCH_REAR_POINT1_Y] = 272;
	profile_def.touch[PROFILE_TOUCH_REAR_POINT2_X] = 1280;
	profile_def.touch[PROFILE_TOUCH_REAR_POINT2_Y] = 272;
	profile_def.touch[PROFILE_TOUCH_REAR_POINT3_X] = 600;
	profile_def.touch[PROFILE_TOUCH_REAR_POINT3_Y] = 608;
	profile_def.touch[PROFILE_TOUCH_REAR_POINT4_X] = 1280;
	profile_def.touch[PROFILE_TOUCH_REAR_POINT4_Y] = 608;
	profile_def.touch[PROFILE_TOUCH_FRONT_DISABLE] = 1;
	profile_def.touch[PROFILE_TOUCH_REAR_DISABLE] = 1;

	profile_def.gyro[PROFILE_GYRO_SENSIVITY_X] = 127;
	profile_def.gyro[PROFILE_GYRO_SENSIVITY_Y] = 127;
	profile_def.gyro[PROFILE_GYRO_SENSIVITY_Z] = 127;
	profile_def.gyro[PROFILE_GYRO_DEADZONE_X] = 0;
	profile_def.gyro[PROFILE_GYRO_DEADZONE_Y] = 0;
	profile_def.gyro[PROFILE_GYRO_DEADZONE_Z] = 0;
	profile_def.gyro[PROFILE_GYRO_DEADBAND] = 0;
	profile_def.gyro[PROFILE_GYRO_WHEEL] = 0;
	profile_def.gyro[PROFILE_GYRO_RESET_BTN1] = 6;
	profile_def.gyro[PROFILE_GYRO_RESET_BTN2] = 7;

	profile_def.controller[PROFILE_CONTROLLER_ENABLED] = 0;
	profile_def.controller[PROFILE_CONTROLLER_PORT] = 0;
	profile_def.controller[PROFILE_CONTROLLER_SWAP_BUTTONS] = 0;

	profile_settings_def[PROFILE_SETTINGS_KEY1] = 4;
	profile_settings_def[PROFILE_SETTINGS_KEY2] = 3;
	profile_settings_def[PROFILE_SETTINGS_AUTOSAVE] = 1;
	profile_settings_def[PROFILE_SETTINGS_DELAY] = 10;
}
void profile_init(){
	setDefProfile();
	profile = profile_def;
	if (!profile_loadGlobal()){
		profile_saveAsGlobal();
	}
	profile = profile_global;
	if (!profile_loadHome()){
		profile_saveHome();
	}
	profile = profile_home;
	profile_loadSettings();
}
void profile_destroy(){
	
}
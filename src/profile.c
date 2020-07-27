#include <vitasdkkern.h>
#include <taihen.h>
#include <libk/stdio.h>
#include <stdbool.h>
#include <psp2kern/io/stat.h> 
#include "main.h"
#include "profile.h"

Profile profile;
Profile profile_def;
Profile profile_global;
uint8_t profile_settings[PROFILE_SETTINGS_NUM];
uint8_t profile_settings_def[PROFILE_SETTINGS_NUM];

static char fname[128];

void profile_addRemapRule(struct RemapRule rule){
	if (profile.remapsNum < (REMAP_NUM - 1)){
		profile.remaps[profile.remapsNum] = rule;
		profile.remapsNum++;
	}
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

void profile_saveSettings(){
	// Just in case the folder doesn't exist
	ksceIoMkdir("ux0:/data/remaPSV", 0777); 
	
	// Opening settings config file and saving the config
	SceUID fd = ksceIoOpen("ux0:/data/remaPSV/settings.bin", SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
	ksceIoWrite(fd, profile_settings, PROFILE_SETTINGS_NUM);
	ksceIoClose(fd);
}

void profile_loadSettings(){
	profile_resetSettings();
	
	// Just in case the folder doesn't exist
	ksceIoMkdir("ux0:/data/remaPSV", 0777); 
	
	// Loading config file for the selected app if exists
	SceUID fd = ksceIoOpen("ux0:/data/remaPSV/settings.bin", SCE_O_RDONLY, 0777);
	if (fd >= 0){
		ksceIoRead(fd, profile_settings, PROFILE_SETTINGS_NUM);
		ksceIoClose(fd);
	}
}

void profile_saveGlobal() {
	SceUID fd;
	// Just in case the folder doesn't exist
	ksceIoMkdir("ux0:/data/remaPSV", 0777); 
	
	// Opening remap config file and saving it
	fd = ksceIoOpen("ux0:/data/remaPSV/remap.bin", SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
	ksceIoWrite(fd, profile.remap, PROFILE_REMAP_NUM);
	ksceIoClose(fd);
	
	// Opening analog config file and saving the config
	fd = ksceIoOpen("ux0:/data/remaPSV/analogs.bin", SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
	ksceIoWrite(fd, profile.analog, PROFILE_ANALOG_NUM);
	ksceIoClose(fd);
	
	// Opening touch config file and saving the config
	fd = ksceIoOpen("ux0:/data/remaPSV/touch.bin", SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
	ksceIoWrite(fd, profile.touch, PROFILE_TOUCH_NUM*2);
	ksceIoClose(fd);
	
	// Opening gyro config file and saving the config
	fd = ksceIoOpen("ux0:/data/remaPSV/gyro.bin", SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
	ksceIoWrite(fd, profile.gyro, PROFILE_GYRO_NUM);
	ksceIoClose(fd);
	
	// Opening gyro config file and saving the config
	fd = ksceIoOpen("ux0:/data/remaPSV/controllers.bin", SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
	ksceIoWrite(fd, profile.controller, PROFILE_CONTROLLER_NUM);
	ksceIoClose(fd);
}

void profile_saveLocal() {
	SceUID fd;
	// Just in case the folder doesn't exist
	ksceIoMkdir("ux0:/data/remaPSV", 0777); 
	sprintf(fname, "ux0:/data/remaPSV/%s", titleid);
	ksceIoMkdir(fname, 0777);
	
	// Opening remap config file and saving it
	sprintf(fname, "ux0:/data/remaPSV/%s/remap.bin", titleid);
	fd = ksceIoOpen(fname, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
	ksceIoWrite(fd, profile.remap, PROFILE_REMAP_NUM);
	ksceIoClose(fd);
	
	// Opening analog config file and saving the config
	sprintf(fname, "ux0:/data/remaPSV/%s/analogs.bin", titleid);
	fd = ksceIoOpen(fname, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
	ksceIoWrite(fd, profile.analog, PROFILE_ANALOG_NUM);
	ksceIoClose(fd);
	
	// Opening touch config file and saving the config
	sprintf(fname, "ux0:/data/remaPSV/%s/touch.bin", titleid);
	fd = ksceIoOpen(fname, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
	ksceIoWrite(fd, profile.touch, PROFILE_TOUCH_NUM*2);
	ksceIoClose(fd);
	
	// Opening gyro config file and saving the config
	sprintf(fname, "ux0:/data/remaPSV/%s/gyro.bin", titleid);
	fd = ksceIoOpen(fname, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
	ksceIoWrite(fd, profile.gyro, PROFILE_GYRO_NUM);
	ksceIoClose(fd);
	
	// Opening gyro config file and saving the config
	sprintf(fname, "ux0:/data/remaPSV/%s/controllers.bin", titleid);
	fd = ksceIoOpen(fname, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
	ksceIoWrite(fd, profile.controller, PROFILE_CONTROLLER_NUM);
	ksceIoClose(fd);
}

void profile_loadGlobal() {
	profile_resetRemap();
	profile_resetAnalog();
	profile_resetTouch();
	profile_resetGyro();
	profile_resetController();
	
	SceUID fd;
	
	// Just in case the folder doesn't exist
	ksceIoMkdir("ux0:/data/remaPSV", 0777); 
	
	// Loading config file for the selected app if exists
	fd = ksceIoOpen("ux0:/data/remaPSV/remap.bin", SCE_O_RDONLY, 0777);
	if (fd >= 0){
		ksceIoRead(fd, profile.remap, PROFILE_REMAP_NUM);
		ksceIoClose(fd);
	}
	
	// Loading analog config file
	fd = ksceIoOpen("ux0:/data/remaPSV/analogs.bin", SCE_O_RDONLY, 0777);
	if (fd >= 0){
		ksceIoRead(fd, profile.analog, PROFILE_ANALOG_NUM);
		ksceIoClose(fd);
	}
	
	// Loading touch config file
	fd = ksceIoOpen("ux0:/data/remaPSV/touch.bin", SCE_O_RDONLY, 0777);
	if (fd >= 0){
		ksceIoRead(fd, profile.touch, PROFILE_TOUCH_NUM*2);
		ksceIoClose(fd);
	}
	
	// Loading gyro config file
	fd = ksceIoOpen("ux0:/data/remaPSV/gyro.bin", SCE_O_RDONLY, 0777);
	if (fd >= 0){
		ksceIoRead(fd, profile.gyro, PROFILE_GYRO_NUM);
		ksceIoClose(fd);
	}
	
	// Loading controllers config file
	fd = ksceIoOpen("ux0:/data/remaPSV/controllers.bin", SCE_O_RDONLY, 0777);
	if (fd >= 0){
		ksceIoRead(fd, profile.controller, PROFILE_CONTROLLER_NUM);
		ksceIoClose(fd);
	}
}

void profile_loadLocal() {
	// Check if folder exists
	SceIoStat stat = {0};
	sprintf(fname, "ux0:/data/remaPSV/%s", titleid);
    int ret = ksceIoGetstat(fname, &stat);
	if (ret < 0)
		return;
	
	profile_resetRemap();
	profile_resetAnalog();
	profile_resetTouch();
	profile_resetGyro();
	profile_resetController();
	
	SceUID fd;
	
	// Loading remap config file for the selected app if exists
	sprintf(fname, "ux0:/data/remaPSV/%s/remap.bin", titleid);
	fd = ksceIoOpen(fname, SCE_O_RDONLY, 0777);
	if (fd >= 0){
		ksceIoRead(fd, profile.remap, PROFILE_REMAP_NUM);
		ksceIoClose(fd);
	}
	
	// Loading analog config file for the selected app if exists
	sprintf(fname, "ux0:/data/remaPSV/%s/analogs.bin", titleid);
	fd = ksceIoOpen(fname, SCE_O_RDONLY, 0777);
	if (fd >= 0){
		ksceIoRead(fd, profile.analog, PROFILE_ANALOG_NUM);
		ksceIoClose(fd);
	}
	
	// Loading touch config file for the selected app if exists
	sprintf(fname, "ux0:/data/remaPSV/%s/touch.bin", titleid);
	fd = ksceIoOpen(fname, SCE_O_RDONLY, 0777);
	if (fd >= 0){
		ksceIoRead(fd, profile.touch, PROFILE_TOUCH_NUM*2);
		ksceIoClose(fd);
	}
	
	// Loading gyro config file for the selected app if exists
	sprintf(fname, "ux0:/data/remaPSV/%s/gyro.bin", titleid);
	fd = ksceIoOpen(fname, SCE_O_RDONLY, 0777);
	if (fd >= 0){
		ksceIoRead(fd, profile.gyro, PROFILE_GYRO_NUM);
		ksceIoClose(fd);
	}
	
	// Loading gyro config file for the selected app if exists
	sprintf(fname, "ux0:/data/remaPSV/%s/controllers.bin", titleid);
	fd = ksceIoOpen(fname, SCE_O_RDONLY, 0777);
	if (fd >= 0){
		ksceIoRead(fd, profile.controller, PROFILE_CONTROLLER_NUM);
		ksceIoClose(fd);
	}
}

void profile_deleteGlobal(){
	
}

void profile_deleteLocal(){
	
}

void profile_init(){
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

void profile_destroy(){
	
}
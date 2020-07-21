#include <vitasdk.h>
#include <taihen.h>
#include <libk/stdio.h>
#include "main.h"
#include "profile.h"

const uint16_t TOUCH_POINTS_DEF[16] = {
	600,  272,	//Front TL
	1280, 272,	//		TR
	600,  816,	//		BL
	1280, 816,	//		BR
	600,  272,	//Rear	TL
	1280, 272,	//		TR
	600,  608,	//		BL
	1280, 608	//		BR
};

const uint8_t GYRO_DEF[GYRO_OPTIONS_NUM] = {
	127, 127, 127,	//Sensivity
	15, 15, 15,		//Deadzone
	0,				//Deadband
	0,				//Whel mode
	6,				//Reset wheel btn 1 | LTrigger
	7,				//Reset wheel btn 2 | RTrigger
	0
};

const uint8_t CNTRL_DEF[CNTRL_OPTIONS_NUM] = {
	0, 0, 0
};

const uint8_t SETTINGS_DEF[SETTINGS_NUM] = {
	4, 3, //Opening keys
	1,	  //Autosave game profile
	10	  //Startup delay
};

uint8_t btn_mask[BUTTONS_NUM];
uint8_t analogs_options[ANOLOGS_OPTIONS_NUM];
uint8_t gyro_options[GYRO_OPTIONS_NUM];
uint16_t touch_options[TOUCH_OPTIONS_NUM];
uint8_t controller_options[CNTRL_OPTIONS_NUM];
uint8_t settings_options[SETTINGS_NUM];

static char fname[128];

// Reset options per-menu
void resetRemapsOptions(){
	for (int i = 0; i < BUTTONS_NUM; i++)
		btn_mask[i] = REMAP_DEF;
}
void resetAnalogsOptions(){
	for (int i = 0; i < ANOLOGS_OPTIONS_NUM; i++)
		analogs_options[i] = i < 4 ? ANALOGS_DEADZONE_DEF : ANALOGS_FORCE_DIGITAL_DEF;
}
void resetTouchOptions(){
	for (int i = 0; i < TOUCH_OPTIONS_NUM - 2; i++)
		touch_options[i] = TOUCH_POINTS_DEF[i];
	touch_options[16] = TOUCH_MODE_DEF;
	touch_options[17] = TOUCH_MODE_DEF;
}
void resetGyroOptions() {
	for (int i = 0; i < GYRO_OPTIONS_NUM; i++)
		gyro_options[i] = GYRO_DEF[i];
}
void resetCntrlOptions(){
	for (int i = 0; i < CNTRL_OPTIONS_NUM; i++)
		controller_options[i] = CNTRL_DEF[i];
}
void resetSettingsOptions(){
	for (int i = 0; i < SETTINGS_NUM; i++)
		settings_options[i] = SETTINGS_DEF[i];
}

void saveSettings(){
	// Just in case the folder doesn't exist
	sceIoMkdir("ux0:/data/remaPSV", 0777); 
	
	// Opening settings config file and saving the config
	SceUID fd = sceIoOpen("ux0:/data/remaPSV/settings.bin", SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
	sceIoWrite(fd, settings_options, SETTINGS_NUM);
	sceIoClose(fd);
}

void loadSettings(){
	resetSettingsOptions();
	
	// Just in case the folder doesn't exist
	sceIoMkdir("ux0:/data/remaPSV", 0777); 
	
	// Loading config file for the selected app if exists
	SceUID fd = sceIoOpen("ux0:/data/remaPSV/settings.bin", SCE_O_RDONLY, 0777);
	if (fd >= 0){
		sceIoRead(fd, settings_options, SETTINGS_NUM);
		sceIoClose(fd);
	}
}

void saveGlobalConfig(void) {
	SceUID fd;
	// Just in case the folder doesn't exist
	sceIoMkdir("ux0:/data/remaPSV", 0777); 
	
	// Opening remap config file and saving it
	fd = sceIoOpen("ux0:/data/remaPSV/remap.bin", SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
	sceIoWrite(fd, btn_mask, BUTTONS_NUM);
	sceIoClose(fd);
	
	// Opening analog config file and saving the config
	fd = sceIoOpen("ux0:/data/remaPSV/analogs.bin", SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
	sceIoWrite(fd, analogs_options, ANOLOGS_OPTIONS_NUM);
	sceIoClose(fd);
	
	// Opening touch config file and saving the config
	fd = sceIoOpen("ux0:/data/remaPSV/touch.bin", SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
	sceIoWrite(fd, touch_options, TOUCH_OPTIONS_NUM*2);
	sceIoClose(fd);
	
	// Opening gyro config file and saving the config
	fd = sceIoOpen("ux0:/data/remaPSV/gyro.bin", SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
	sceIoWrite(fd, gyro_options, GYRO_OPTIONS_NUM);
	sceIoClose(fd);
	
	// Opening gyro config file and saving the config
	fd = sceIoOpen("ux0:/data/remaPSV/controllers.bin", SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
	sceIoWrite(fd, controller_options, CNTRL_OPTIONS_NUM);
	sceIoClose(fd);
}

void saveGameConfig(void) {
	SceUID fd;
	// Just in case the folder doesn't exist
	sceIoMkdir("ux0:/data/remaPSV", 0777); 
	sprintf(fname, "ux0:/data/remaPSV/%s", titleid);
	sceIoMkdir(fname, 0777);
	
	// Opening remap config file and saving it
	sprintf(fname, "ux0:/data/remaPSV/%s/remap.bin", titleid);
	fd = sceIoOpen(fname, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
	sceIoWrite(fd, btn_mask, BUTTONS_NUM);
	sceIoClose(fd);
	
	// Opening analog config file and saving the config
	sprintf(fname, "ux0:/data/remaPSV/%s/analogs.bin", titleid);
	fd = sceIoOpen(fname, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
	sceIoWrite(fd, analogs_options, ANOLOGS_OPTIONS_NUM);
	sceIoClose(fd);
	
	// Opening touch config file and saving the config
	sprintf(fname, "ux0:/data/remaPSV/%s/touch.bin", titleid);
	fd = sceIoOpen(fname, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
	sceIoWrite(fd, touch_options, TOUCH_OPTIONS_NUM*2);
	sceIoClose(fd);
	
	// Opening gyro config file and saving the config
	sprintf(fname, "ux0:/data/remaPSV/%s/gyro.bin", titleid);
	fd = sceIoOpen(fname, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
	sceIoWrite(fd, gyro_options, GYRO_OPTIONS_NUM);
	sceIoClose(fd);
	
	// Opening gyro config file and saving the config
	sprintf(fname, "ux0:/data/remaPSV/%s/controllers.bin", titleid);
	fd = sceIoOpen(fname, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
	sceIoWrite(fd, controller_options, CNTRL_OPTIONS_NUM);
	sceIoClose(fd);
}

void loadGlobalConfig(void) {
	resetRemapsOptions();
	resetAnalogsOptions();
	resetTouchOptions();
	resetGyroOptions();
	resetCntrlOptions();
	
	SceUID fd;
	
	// Just in case the folder doesn't exist
	sceIoMkdir("ux0:/data/remaPSV", 0777); 
	
	// Loading config file for the selected app if exists
	fd = sceIoOpen("ux0:/data/remaPSV/remap.bin", SCE_O_RDONLY, 0777);
	if (fd >= 0){
		sceIoRead(fd, btn_mask, BUTTONS_NUM);
		sceIoClose(fd);
	}
	
	// Loading analog config file
	fd = sceIoOpen("ux0:/data/remaPSV/analogs.bin", SCE_O_RDONLY, 0777);
	if (fd >= 0){
		sceIoRead(fd, analogs_options, ANOLOGS_OPTIONS_NUM);
		sceIoClose(fd);
	}
	
	// Loading touch config file
	fd = sceIoOpen("ux0:/data/remaPSV/touch.bin", SCE_O_RDONLY, 0777);
	if (fd >= 0){
		sceIoRead(fd, touch_options, TOUCH_OPTIONS_NUM*2);
		sceIoClose(fd);
	}
	
	// Loading gyro config file
	fd = sceIoOpen("ux0:/data/remaPSV/gyro.bin", SCE_O_RDONLY, 0777);
	if (fd >= 0){
		sceIoRead(fd, gyro_options, GYRO_OPTIONS_NUM);
		sceIoClose(fd);
	}
	
	// Loading controllers config file
	fd = sceIoOpen("ux0:/data/remaPSV/controllers.bin", SCE_O_RDONLY, 0777);
	if (fd >= 0){
		sceIoRead(fd, controller_options, CNTRL_OPTIONS_NUM);
		sceIoClose(fd);
	}
}

void loadGameConfig(void) {
	// Check if folder exists
	SceIoStat stat = {0};
	sprintf(fname, "ux0:/data/remaPSV/%s", titleid);
    int ret = sceIoGetstat(fname, &stat);
	if (ret < 0)
		return;
	
	resetRemapsOptions();
	resetAnalogsOptions();
	resetTouchOptions();
	resetGyroOptions();
	resetCntrlOptions();
	
	SceUID fd;
	
	// Loading remap config file for the selected app if exists
	sprintf(fname, "ux0:/data/remaPSV/%s/remap.bin", titleid);
	fd = sceIoOpen(fname, SCE_O_RDONLY, 0777);
	if (fd >= 0){
		sceIoRead(fd, btn_mask, BUTTONS_NUM);
		sceIoClose(fd);
	}
	
	// Loading analog config file for the selected app if exists
	sprintf(fname, "ux0:/data/remaPSV/%s/analogs.bin", titleid);
	fd = sceIoOpen(fname, SCE_O_RDONLY, 0777);
	if (fd >= 0){
		sceIoRead(fd, analogs_options, ANOLOGS_OPTIONS_NUM);
		sceIoClose(fd);
	}
	
	// Loading touch config file for the selected app if exists
	sprintf(fname, "ux0:/data/remaPSV/%s/touch.bin", titleid);
	fd = sceIoOpen(fname, SCE_O_RDONLY, 0777);
	if (fd >= 0){
		sceIoRead(fd, touch_options, TOUCH_OPTIONS_NUM*2);
		sceIoClose(fd);
	}
	
	// Loading gyro config file for the selected app if exists
	sprintf(fname, "ux0:/data/remaPSV/%s/gyro.bin", titleid);
	fd = sceIoOpen(fname, SCE_O_RDONLY, 0777);
	if (fd >= 0){
		sceIoRead(fd, gyro_options, GYRO_OPTIONS_NUM);
		sceIoClose(fd);
	}
	
	// Loading gyro config file for the selected app if exists
	sprintf(fname, "ux0:/data/remaPSV/%s/controllers.bin", titleid);
	fd = sceIoOpen(fname, SCE_O_RDONLY, 0777);
	if (fd >= 0){
		sceIoRead(fd, controller_options, CNTRL_OPTIONS_NUM);
		sceIoClose(fd);
	}
}
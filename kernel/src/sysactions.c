#include <vitasdkkern.h>
#include <taihen.h>
#include <psp2/motion.h> 
#include <psp2kern/power.h> 
#include <psp2kern/appmgr.h> 
#include "vitasdkext.h"
#include "common.h"
#include "userspace.h"
#include "main.h"
#include "fio/fio.h"
#include "fio/profile.h"
#include "fio/settings.h"
#include "sysactions.h"
#include "gui/gui.h"
#include "log.h"

#define PATH_SAVE_BACKUP    "ux0:/data/remaPSV2/Save"
#define PATH_SAVE           "ux0:/user/00/savedata"

#define BRIGHTNESS_STEP     6
int brightnessLevel;

void sysactions_softReset(){
    kscePowerRequestSoftReset();
}
void sysactions_coldReset(){
    kscePowerRequestColdReset();
}
void sysactions_standby(){
    kscePowerRequestStandby();
}
void sysactions_suspend(){
    kscePowerRequestSuspend();
}
void sysactions_displayOff(){
    kscePowerRequestDisplayOff();
}
void sysactions_killCurrentApp(){
    if (processid == -1)
        return;
        
    if (settings[POP_KILL].v.b)
        gui_popupShow("Kill", titleid, 2*1000*1000);
    ksceAppMgrKillProcess(processid);
}
void brigtnessPopup(){
    int percentage = brightnessLevel / ((0xFFFF - 21) / 100);
    char header[40];
    char message[40] = "\0";
	sprintf(header, "Brightness: %i%%", percentage);
    for (int i = 0; i < 20; i++){
        strcat(message, i * 5 < percentage ? "|" : "-");
    }
	if (settings[POP_BRIGHTNESS].v.b)
        gui_popupShow(header, message, 2*1000*1000);
}

void sysactions_brightnessInc(){
    brightnessLevel = clamp(
        21 + (brightnessLevel + (0xFFFF - 21) * BRIGHTNESS_STEP / 100),
        21, 0xFFFF + 1);
    kscePowerSetDisplayBrightness(brightnessLevel);
	ksceRegMgrSetKeyInt("/CONFIG/DISPLAY", "brightness", brightnessLevel);
    brigtnessPopup();
}

void sysactions_brightnessDec(){
    brightnessLevel = clamp(
        21 + (brightnessLevel - (0xFFFF - 21) * BRIGHTNESS_STEP / 100),
        21, 0xFFFF + 1);
    kscePowerSetDisplayBrightness(brightnessLevel);
	ksceRegMgrSetKeyInt("/CONFIG/DISPLAY", "brightness", brightnessLevel);
    brigtnessPopup();
}

void sysactions_saveBackup(){
    char msg[64];
	sprintf(msg, "Backuping save for %s", titleid);
    gui_popupShow(msg, "Please, wait ...", 0);

	//Create dir if not exists
	ksceIoMkdir(PATH_SAVE_BACKUP, 0777); 

    char src[64];
    char dest[64];
	sprintf(src, "%s/%s", PATH_SAVE, titleid);
	sprintf(dest, "%s/%s", PATH_SAVE_BACKUP, titleid);
    
    if (fio_exist(src)){
        if (fio_copyDir(src, dest) == 0){
            gui_popupShow(msg, "Done !", 3*1000*1000);
            return;
        }
    }

    gui_popupShow(msg, "Failed !", 3*1000*1000);
}

void sysactions_saveRestore(){
    char msg[64];
	sprintf(msg, "Restoring save for %s", titleid);
    gui_popupShow(msg, "Please, wait ...", 0);

    char src[64];
    char dest[64];
	sprintf(src, "%s/%s", PATH_SAVE_BACKUP, titleid);
	sprintf(dest, "%s/%s", PATH_SAVE, titleid);
    
    if (fio_exist(src)){
        if (fio_copyDir(src, dest) == 0){
            gui_popupShow(msg, "Done !", 2*1000*1000);
            return;
        }
    }

    gui_popupShow(msg, "Failed !", 2*1000*1000);
}

void sysactions_calibrateMotion(){
	SceMotionState sms;
	int gyroRet = __sceMotionGetState(&sms);
    if (gyroRet >= 0){
        profile.entries[PR_GY_CALIBRATION_Z].v.i = clamp(
            (int)(sms.acceleration.x * 1000), 
            profile.entries[PR_GY_CALIBRATION_Z].min.i, 
            profile.entries[PR_GY_CALIBRATION_Z].max.i);
        gui_popupShow("Motion calibration", "Done !", 2*1000*1000);
    }
}

void sysactions_init(){
	ksceRegMgrGetKeyInt("/CONFIG/DISPLAY", "brightness", (int *)&brightnessLevel);
}
#include <vitasdkkern.h>
#include <taihen.h>
#include <psp2kern/power.h> 
#include <psp2kern/appmgr.h> 
#include "vitasdkext.h"
#include "common.h"
#include "main.h"
#include "fio/profile.h"
#include "sysactions.h"
#include "gui/gui.h"
#include "log.h"

#define BRIGHTNESS_STEP 10
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
    if (processid != -1)
        ksceAppMgrKillProcess(processid);
}
void brigtnessPopup(){
    int percentage = brightnessLevel / ((0xFFFF - 21) / 100);
    char header[40];
    char message[40] = "\0";
	sprintf(header, "Brightness: %i%%", percentage);
    for (int i = 0; i < 20; i++)
        strcat(message, i * 5 < percentage ? "|" : "-");
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

void sysactions_init(){
	ksceRegMgrGetKeyInt("/CONFIG/DISPLAY", "brightness", (int *)&brightnessLevel);
}
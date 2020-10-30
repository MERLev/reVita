#include <vitasdkkern.h>
#include <taihen.h>
#include <psp2kern/power.h> 
#include <psp2kern/appmgr.h> 
#include "vitasdkext.h"
#include "common.h"
#include "main.h"
#include "fio/profile.h"
#include "sysactions.h"
#include "log.h"

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
void sysactions_brightnessInc(){
    brightnessLevel = clamp(
        21 + (brightnessLevel + (0xFFFF - 21) * 0.1),
        21, 0xFFFF + 1);
    kscePowerSetDisplayBrightness(brightnessLevel);
}
void sysactions_brightnessDec(){
    brightnessLevel = clamp(
        21 + (brightnessLevel - (0xFFFF - 21) * 0.1),
        21, 0xFFFF + 1);
    kscePowerSetDisplayBrightness(brightnessLevel);
}

void sysactions_init(){
	ksceRegMgrGetKeyInt("/CONFIG/DISPLAY", "brightness", (int *)&brightnessLevel);
}
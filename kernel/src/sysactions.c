#include <vitasdkkern.h>
#include <taihen.h>
#include <psp2kern/power.h> 
#include <psp2kern/appmgr.h> 
#include "main.h"
#include "fio/profile.h"
#include "sysactions.h"

// void sceAppMgrDestroyOtherAppByPidForDriver();

// void killApp(){
//     if (strncmp(titleid, HOME, sizeof(titleid)))
//         return;
//     sceAppMgrDestroyOtherAppByPidForDriver(SceUID id);
// }

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
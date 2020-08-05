#include <vitasdkkern.h>
#include <psp2/motion.h>
#include <stdbool.h>
#include <string.h>
#include "profile.h"
#include "remapsv.h"
#include "log.h"

#define DELAY 5000
static SceUID mutex_sce_touch_peek_uid = -1;

bool userPluginLoaded = false;
bool hasStoredData = false;

//Arguments for sceMotionGetState
SceMotionState sms;
int ret; 

//Configuration
int configVersion = 0;
bool deadband;
bool patchExtEnabled;
int patchExtPort;

/*export*/ void remaPSV2k_userPluginReady(){
    userPluginLoaded = true;
}
/*export*/ int remaPSV2k_getConfigVersion(){
    if (profile.gyro[PROFILE_GYRO_DEADBAND] != deadband 
            || profile.controller[PROFILE_CONTROLLER_ENABLED] != patchExtEnabled
            || profile.controller[PROFILE_CONTROLLER_PORT] != patchExtPort){
        deadband = profile.gyro[PROFILE_GYRO_DEADBAND];
        patchExtEnabled = profile.controller[PROFILE_CONTROLLER_ENABLED];
        patchExtPort = profile.controller[PROFILE_CONTROLLER_PORT];
        configVersion = ksceKernelGetSystemTimeWide();
    }
    return configVersion;
}
/*export*/ bool remaPSV2k_getConfigDeadband(){
    return deadband;
}
/*export*/ bool remaPSV2k_getConfigPatchExtEnabled(){
    return patchExtEnabled;
}
/*export*/ int remaPSV2k_getConfigPatchExtPort(){
    return patchExtPort;
}
/*export*/ void remaPSV2k_setSceMotionState(SceMotionState *pData, int r){
    bool readyToReturn = false;
    bool hasUnsetData = true;

    while (!readyToReturn){
        if (ksceKernelLockMutex(mutex_sce_touch_peek_uid, 1, NULL) >= 0){
            if (hasUnsetData){
                hasUnsetData = false;
                
                if (r == 0 )
                    ksceKernelMemcpyUserToKernel(&sms, (uintptr_t)pData, sizeof(SceMotionState)); 
                ret = r;
                hasStoredData = true;;
            }
            if (!hasStoredData)
                readyToReturn = true;

            ksceKernelUnlockMutex(mutex_sce_touch_peek_uid, 1);
        }
        if (!readyToReturn)
            ksceKernelDelayThread(DELAY);
    }
}

int _sceMotionGetState(SceMotionState *pData){
    if (!userPluginLoaded)
        return 0;

    int result = 0;
    bool readyToReturn = false;

    while(!readyToReturn) {
        if (ksceKernelLockMutex(mutex_sce_touch_peek_uid, 1, NULL) >= 0){
            if (hasStoredData){
                if (ret == 0){
                    memcpy(pData, &sms, sizeof(SceMotionState));
                    result = ret;
                }
                readyToReturn = true;
                hasStoredData = false;
            }
            ksceKernelUnlockMutex(mutex_sce_touch_peek_uid, 1);
        }

        if (!readyToReturn)
            ksceKernelDelayThread(DELAY);
    }

    return result;
}

void userspace_init(){
    mutex_sce_touch_peek_uid = ksceKernelCreateMutex("remaPSV2_mutex_userspace", 0, 0, NULL);
}
void userspace_destroy(){
    if (mutex_sce_touch_peek_uid >= 0)
        ksceKernelDeleteMutex(mutex_sce_touch_peek_uid);
}
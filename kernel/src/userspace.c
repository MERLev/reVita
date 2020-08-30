#include <vitasdkkern.h>
#include <psp2/motion.h>
#include <stdbool.h>
#include <string.h>
#include "fio/profile.h"
#include "remapsv.h"
#include "log.h"

#define DELAY 1000
static SceUID mutex_sce_touch_peek_uid = -1;

bool userPluginLoaded = false;
bool hasStoredData = false;

//Arguments for sceMotionGetState
SceMotionState sms;
int ret; 

/*export*/ void remaPSV2k_userPluginReady(){
    userPluginLoaded = true;
}
/*export*/ int remaPSV2k_getProfileVersion(){
    return profile.version;
}
/*export*/ void remaPSV2k_getProfile(Profile* p){
    ksceKernelMemcpyKernelToUser((uintptr_t)&p[0], &profile, sizeof(profile));
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
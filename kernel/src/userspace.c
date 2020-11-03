#include <vitasdkkern.h>
#include <psp2/motion.h>
#include <stdbool.h>
#include <string.h>
#include "fio/profile.h"
#include "remapsv.h"
#include "log.h"

#define DELAY 1000
#define TTL   500##000
static SceUID mutex_sce_touch_peek_uid = -1;

// bool userPluginLoaded = false;
// bool hasStoredData = false;

//Arguments for sceMotionGetState
SceInt64 tick = 0;
SceMotionState sms;
int result; 

/*export*/ void remaPSV2k_getProfile(Profile* p){
    ksceKernelMemcpyKernelToUser((uintptr_t)&p[0], &profile, sizeof(profile));
}
/*export*/ void remaPSV2k_setSceMotionState(SceMotionState *pData, int r){
    ksceKernelLockMutex(mutex_sce_touch_peek_uid, 1, NULL);

    ksceKernelMemcpyUserToKernel(&sms, (uintptr_t)pData, sizeof(SceMotionState)); 
    result = r;
    tick = ksceKernelGetSystemTimeWide();

    ksceKernelUnlockMutex(mutex_sce_touch_peek_uid, 1);
}

int __sceMotionGetState(SceMotionState *pData){
    int ret = -1;
    ksceKernelLockMutex(mutex_sce_touch_peek_uid, 1, NULL);

    if (tick + TTL > ksceKernelGetSystemTimeWide()){
        memcpy(pData, &sms, sizeof(SceMotionState));
        ret = result;
    }
    
    ksceKernelUnlockMutex(mutex_sce_touch_peek_uid, 1);
    return ret;
}

void userspace_init(){
    mutex_sce_touch_peek_uid = ksceKernelCreateMutex("remaPSV2_mutex_userspace", 0, 0, NULL);
}
void userspace_destroy(){
    if (mutex_sce_touch_peek_uid >= 0)
        ksceKernelDeleteMutex(mutex_sce_touch_peek_uid);
}
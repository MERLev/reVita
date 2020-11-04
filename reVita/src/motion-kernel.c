#include <vitasdkkern.h>
#include <taihen.h>
#include <stdio.h>
#include <string.h>
#include <psp2/motion.h> 
#include <psp2/touch.h>
#include <psp2kern/kernel/sysmem.h> 

#include "vitasdkext.h"
#include "common.h"
#include "main.h"
#include "log.h"

bool initDone = false;
static SceUID thread_uid = -1;
static bool   isNotReady = true;
SceUID modid;
static SceUID shellPid;

int (*_sceMotionGetState)(SceMotionState *motionState);
int (*_sceMotionGetSensorState)(SceMotionSensorState *sensorState, int numRecords);
int (*_sceMotionGetBasicOrientation)(SceFVector3 *basicOrientation);
int (*_sceMotionRotateYaw)(float radians);
int (*_sceMotionGetTiltCorrection)(void);
int (*_sceMotionSetTiltCorrection)(int setValue);
int (*_sceMotionGetDeadband)(void);
int (*_sceMotionSetDeadband)(int setValue);
int (*_sceMotionSetAngleThreshold)(float angle);
float (*_sceMotionGetAngleThreshold)(void);
int (*_sceMotionReset)(void);
int (*_sceMotionMagnetometerOn)(void);
int (*_sceMotionMagnetometerOff)(void);
int (*_sceMotionGetMagnetometerState)(void);
int (*_sceMotionStartSampling)(void);
int (*_sceMotionStopSampling)(void);

SceUID mallocForUser(void ** base, int size){
    SceKernelAllocMemBlockKernelOpt opts;
    memset(&opts, 0, sizeof(opts));
    opts.size = sizeof(SceKernelAllocMemBlockKernelOpt);
    opts.pid = shellPid;

    SceUID memId = ksceKernelAllocMemBlock("remapsv2_user_mem", SCE_KERNEL_MEMBLOCK_TYPE_USER_RW, 
        ((0xfff + size) & ~0xfff), &opts);
    int retMemBase = ksceKernelGetMemBlockBase(memId, base);
    if (retMemBase < 0){
        LOG("memalloc id: %i, rturned: %i, addr: %08X\n", memId, retMemBase, (uint)base);
        return retMemBase;
    }
    return memId;
}

int ksceMotionGetState(SceMotionState *motionState){
    if (isNotReady) return -1;

    uint32_t state;
    ENTER_SYSCALL(state);

    SceMotionState* smsUser;
    SceUID memid = mallocForUser((void**)&smsUser, sizeof(SceMotionState));

    int ret = _sceMotionGetState(smsUser);
    // LOG("call returned: %i [%08X]\n", rett, (int)scd);

    ksceKernelMemcpyUserToKernel(motionState, (uintptr_t)smsUser, sizeof(SceMotionState));
    // LOG("Buttons: %08X\n", scdKernel.buttons);

    ksceKernelFreeMemBlock(memid);
    EXIT_SYSCALL(state);

    return ret;
}
int ksceMotionGetSensorState(SceMotionSensorState *sensorState, int numRecords){
    // Not implemented 
    return -1;
}
int ksceMotionGetBasicOrientation(SceFVector3 *basicOrientation){
    // Not implemented 
    return -1;
}
int ksceMotionRotateYaw(float radians){
    if (isNotReady) return -1;
    return _sceMotionRotateYaw(radians);
}
int ksceMotionGetTiltCorrection(void){
    if (isNotReady) return -1;
    return _sceMotionGetTiltCorrection();
}
int ksceMotionSetTiltCorrection(int setValue){
    if (isNotReady) return -1;
    return _sceMotionSetTiltCorrection(setValue);
}
int ksceMotionGetDeadband(void){
    if (isNotReady) return -1;
    return _sceMotionGetDeadband();
}
int ksceMotionSetDeadband(int setValue){
    if (isNotReady) return -1;
    return _sceMotionSetDeadband(setValue);
}
int ksceMotionSetAngleThreshold(float angle){
    if (isNotReady) return -1;
    return _sceMotionSetAngleThreshold(angle);
}
float ksceMotionGetAngleThreshold(void){
    if (isNotReady) return -1;
    return _sceMotionGetAngleThreshold();
}
int ksceMotionReset(void){
    if (isNotReady) return -1;
    return _sceMotionReset();
}
int ksceMotionMagnetometerOn(void){
    if (isNotReady) return -1;
    return _sceMotionMagnetometerOn();
}
int ksceMotionMagnetometerOff(void){
    if (isNotReady) return -1;
    return _sceMotionMagnetometerOff();
}
int ksceMotionGetMagnetometerState(void){
    if (isNotReady) return -1;
    return _sceMotionGetMagnetometerState();
}
int ksceMotionStartSampling(void){
    if (isNotReady) return -1;
    uint32_t state;
    ENTER_SYSCALL(state);
    int ret = _sceMotionStartSampling();
    EXIT_SYSCALL(state);
    return ret;
    // return _sceMotionStartSampling();
}
int ksceMotionStopSampling(void){
    if (isNotReady) return -1;
    return ksceMotionStopSampling();
}

#define IMPORT_OFFSET(offset, name)\
    int name##_ret = module_get_offset(shellPid, modid, 0, offset | 1, (uintptr_t*)&_##name);\
    if (name##_ret < 0)\
        LOG("ERROR: Get Offset for "#name": %08X\n", name##_ret);

static int threadInitMotion(SceSize args, void *argp) {
    while (isNotReady) {
        shellPid = ksceKernelSysrootGetShellPid();
        if (shellPid > 0){
            STRUCTS(tai_module_info_t, modInfo);
            int ret = taiGetModuleInfoForKernel(shellPid, "SceDriverUser", &modInfo);
            if (ret == 0){
                modid = ksceKernelKernelUidForUserUid(shellPid, modInfo.modid);
                STRUCTS(SceKernelModuleInfo, sceinfo);
                ret = ksceKernelGetModuleInfo(shellPid, modid, &sceinfo);
                if (ret == 0){
                    IMPORT_OFFSET(0x5e10, sceMotionGetState);
                    IMPORT_OFFSET(0x7548, sceMotionGetSensorState);
                    IMPORT_OFFSET(0x5d80, sceMotionGetBasicOrientation);
                    IMPORT_OFFSET(0x7f70, sceMotionRotateYaw);
                    IMPORT_OFFSET(0x7fa8, sceMotionGetTiltCorrection);
                    IMPORT_OFFSET(0x7fb4, sceMotionSetTiltCorrection);
                    IMPORT_OFFSET(0x7fd4, sceMotionGetDeadband);
                    IMPORT_OFFSET(0x7fe0, sceMotionSetDeadband);
                    IMPORT_OFFSET(0x802c, sceMotionSetAngleThreshold);
                    IMPORT_OFFSET(0x810c, sceMotionGetAngleThreshold);
                    IMPORT_OFFSET(0x7f08, sceMotionReset);
                    IMPORT_OFFSET(0x8134, sceMotionMagnetometerOn);
                    IMPORT_OFFSET(0x811c, sceMotionMagnetometerOff);
                    IMPORT_OFFSET(0x814c, sceMotionGetMagnetometerState);
                    IMPORT_OFFSET(0x8180, sceMotionStartSampling);
                    IMPORT_OFFSET(0x81ec, sceMotionStopSampling);
                    isNotReady = false;
                }
            }
        }
        
        ksceKernelDelayThread(1000 * 1000);
    }
    return 0;
}

void motion_init(){
        LOG("motion_init()\n");
    thread_uid = ksceKernelCreateThread("reVita_thread_motion", threadInitMotion, 0x3C, 0x3000, 0, 0x10000, 0);
    ksceKernelStartThread(thread_uid, 0, NULL);
}
void motion_destroy(){
    if (thread_uid >= 0) {
        isNotReady = 0;
        ksceKernelWaitThreadEnd(thread_uid, NULL, NULL);
        ksceKernelDeleteThread(thread_uid);
    }
}
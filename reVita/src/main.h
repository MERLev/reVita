#ifndef _MAIN_H_
#define _MAIN_H_
#include <stdbool.h>
#include <psp2/touch.h>
#include <psp2/appmgr.h>

enum H_ID{
    sceCtrlPeekBufferPositive_id = 0,
    sceCtrlReadBufferPositive_id,
    sceCtrlPeekBufferNegative_id,
    sceCtrlReadBufferNegative_id,
    sceCtrlPeekBufferPositive2_id,
    sceCtrlReadBufferPositive2_id,
    sceCtrlPeekBufferPositiveExt_id,
    sceCtrlReadBufferPositiveExt_id,
    sceCtrlPeekBufferPositiveExt2_id,
    sceCtrlReadBufferPositiveExt2_id,
    sceCtrlPeekBufferNegative2_id,
    sceCtrlReadBufferNegative2_id,
    ksceCtrlPeekBufferPositive_id,
    ksceCtrlReadBufferPositive_id,
    ksceCtrlPeekBufferNegative_id,
    ksceCtrlReadBufferNegative_id,
    ksceCtrlPeekBufferPositive2_id,
    ksceCtrlReadBufferPositive2_id,
    ksceCtrlPeekBufferPositiveExt_id,
    ksceCtrlReadBufferPositiveExt_id,
    ksceCtrlPeekBufferPositiveExt2_id,
    ksceCtrlReadBufferPositiveExt2_id,
    ksceCtrlPeekBufferNegative2_id,
    ksceCtrlReadBufferNegative2_id,
    ksceTouchPeek_id,
    ksceTouchRead_id,
    ksceTouchPeekRegion_id,
    ksceTouchReadRegion_id,
    ksceCtrlGetControllerPortInfo_id,
    ksceDisplaySetFrameBufInternal_id,
    ksceKernelInvokeProcEventHandler_id,
    ksceKernelGetProcessId_id,
    ksceRegMgrSetKeyInt_id,
    HOOKS_NUM
};

#define TOUCH_HOOKS_NUM 			4
#define CTRL_HOOKS_NUM 				16

extern char titleid[32];
extern int processid;
extern bool isPspemu;
extern bool isPSTV;
extern bool isPSTVTouchEmulation;
extern bool isSafeBoot;
extern SceUID shellPid = -1;
extern SceUID kernelPid = -1;

extern bool used_funcs[HOOKS_NUM];

void sync();
int ksceCtrlPeekBufferPositive_internal(int port, SceCtrlData *pad_data, int count);
int ksceCtrlPeekBufferPositive2_internal(int port, SceCtrlData *pad_data, int count);
int ksceTouchPeek_internal(SceUInt32 port, SceTouchData *pData, SceUInt32 nBufs);

bool isCallKernel();
bool isCallShell();
bool isCallActive();

#endif

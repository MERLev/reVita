#ifndef _MAIN_H_
#define _MAIN_H_
#include <stdbool.h>
#include <psp2/touch.h>
#include <psp2/appmgr.h>

enum H_ID{
    H_CT_PEEK_P = 0,
    H_CT_READ_P,
    H_CT_PEEK_N,
    H_CT_READ_N,
    H_CT_PEEK_P_2,
    H_CT_READ_P_2,
    H_CT_PEEK_P_EXT,
    H_CT_READ_P_EXT,
    H_CT_PEEK_P_EXT2,
    H_CT_READ_P_EXT2,
    H_CT_PEEK_N_2,
    H_CT_READ_N_2,
    H_K_CT_PEEK_P,
    H_K_CT_READ_P,
    H_K_CT_PEEK_N,
    H_K_CT_READ_N,
    H_K_TO_PEEK,
    H_K_TO_READ,
    H_K_TO_PEEK_R,
    H_K_TO_READ_R,
    H_K_CT_PORT_INFO,
    H_K_DISP_SET_FB,
    H_K_INV_PROC_EV_HANDLER,
    H_K_PROC_GET,
    HOOKS_NUM
};

#define TOUCH_HOOKS_NUM 			4
#define CTRL_HOOKS_NUM 				16

extern char titleid[32];
extern int processid;
extern SceUID shellPid;
extern bool isPspemu;
extern bool isPSTV;

extern bool used_funcs[HOOKS_NUM];

void sync();
int ksceCtrlPeekBufferPositive_internal(int port, SceCtrlData *pad_data, int count);
int ksceTouchPeek_internal(SceUInt32 port, SceTouchData *pData, SceUInt32 nBufs);
int ksceAppMgrLoadExec(const char *appPath, char *const argv[], const SceAppMgrExecOptParam *optParam);
int ksceKernelGetModuleInfo(SceUID pid, SceUID modid, SceKernelModuleInfo *info);
int ksceCtrlPeekBufferPositive2_internal(int port, SceCtrlData *ctrl, int nBufs);
int ksceCtrlPeekBufferPositiveExt_internal(int port, SceCtrlData *ctrl, int nBufs);
int ksceCtrlPeekBufferPositiveExt2_internal(int port, SceCtrlData *ctrl, int nBufs);

#endif

#ifndef _VITASDKEXT_H_
#define _VITASDKEXT_H_

#include <psp2/touch.h>
#include <psp2/appmgr.h> 

// Taken from Adrenaline
typedef struct {
	int savestate_mode;
	int num;
	unsigned int sp;
	unsigned int ra;

	int pops_mode;
	int draw_psp_screen_in_pops;
	char title[128];
	char titleid[12];
	char filename[256];

	int psp_cmd;
	int vita_cmd;
	int psp_response;
	int vita_response;
} SceAdrenaline;

typedef enum SceCtrlButtonsExt {
    SCE_CTRL_TOUCHPAD    = 0x04000000,             //!< Dualshock 4 Touchpad button
    SCE_CTRL_L2          = SCE_CTRL_LTRIGGER,
    SCE_CTRL_R2          = SCE_CTRL_RTRIGGER,
} SceCtrlButtonsExt;

typedef void* SceClibMspace;
SceUID ksceKernelSysrootGetShellPid();
SceBool ksceAppMgrIsExclusiveProcessRunning();

int module_get_offset(SceUID pid, SceUID modid, int segidx, size_t offset, uintptr_t *addr);
int module_get_export_func(SceUID pid, const char *modname, uint32_t libnid, uint32_t funcnid, uintptr_t *func);

int ksceTouchPeek (SceUInt32 port, SceTouchData *pData, SceUInt32 nBufs);
int ksceTouchRead (SceUInt32 port, SceTouchData *pData, SceUInt32 nBufs);
int ksceTouchSetSamplingState(SceUInt32 port, SceTouchSamplingState state);
int ksceTouchGetPanelInfo(SceUInt32 port, SceTouchPanelInfo *pPanelInfo);
int ksceTouchPeekRegion(SceUInt32 port, SceTouchData *pData, SceUInt32 nBufs, int region);
int ksceTouchReadRegion(SceUInt32 port, SceTouchData *pData, SceUInt32 nBufs, int region);

int ksceKernelGetModuleInfo(SceUID pid, SceUID modid, SceKernelModuleInfo *info);
SceUID ksceKernelGetProcessMainModule(SceUID pid);
int ksceCtrlPeekBufferPositive2(int port, SceCtrlData *ctrl, int nBufs);
int ksceCtrlPeekBufferPositiveExt(int port, SceCtrlData *ctrl, int nBufs);
int ksceCtrlPeekBufferPositiveExt2(int port, SceCtrlData *ctrl, int nBufs);
int ksceCtrlSetSamplingModeExt(int mode);

void vitasdkext_init();
void vitasdkext_destroy();

#endif
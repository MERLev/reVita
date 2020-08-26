#include <vitasdkkern.h>
#include <taihen.h>
#include <stdio.h>
#include <string.h>
#include <psp2/motion.h> 
#include <psp2/touch.h>
#include <psp2kern/kernel/sysmem.h> 

#include "vitasdkext.h"
#include "main.h"
#include "ui.h"
#include "remap.h"
#include "profile.h"
#include "common.h"
#include "log.h"
#include "userspace.h"
#include "remapsv.h"

#define INVALID_PID -1

int module_get_offset(SceUID pid, SceUID modid, int segidx, size_t offset, uintptr_t *addr);
int module_get_export_func(SceUID pid, const char *modname, uint32_t libnid, uint32_t funcnid, uintptr_t *func);
bool ksceAppMgrIsExclusiveProcessRunning();

static tai_hook_ref_t refs[HOOKS_NUM];
static SceUID         hooks[HOOKS_NUM];
static SceUID mutex_procevent_uid = -1;
static SceUID thread_uid = -1;
static bool   thread_run = true;

SceUID bufsMemId;
uint8_t* bufsMemBase;
SceTouchData* bufsStd[TOUCH_HOOKS_NUM];
SceCtrlData* bufsScd[CTRL_HOOKS_NUM];

char titleid[32] = "";
int processid = -1;

bool used_funcs[HOOKS_NUM];
bool isInternalTouchCall = false;
bool isInternalExtCall = false;

static uint64_t startTick;
static bool delayedStartDone = false;

SceUID (*_ksceKernelGetProcessMainModule)(SceUID pid);
int (*_ksceKernelGetModuleInfo)(SceUID pid, SceUID modid, SceKernelModuleInfo *info);

int onInput(SceCtrlData *ctrl, int nBufs, int hookId){
	//Nothing to do here
	if (nBufs < 1)
		return nBufs;	

	//Reset wheel gyro buttons pressed
	if (profile.gyro[7] == 1 
            && (ctrl[nBufs - 1].buttons & HW_BUTTONS[profile.gyro[8]]) 
			&& (ctrl[nBufs - 1].buttons & HW_BUTTONS[profile.gyro[9]])) {
		//ToDo
        //sceMotionReset();		
	}

	//In-menu inputs handling
	if (ui_opened){
		// ui_onInput(&ctrl[nBufs - 1]);
		
		//Nullify all inputs
		for (int i = 0; i < nBufs; i++){
			ctrl[i].buttons = 0;
            ctrl[i].lx = ctrl[i].ly = ctrl[i].rx = ctrl[i].ry = 127;
        }
		
		return nBufs;
	}
	
	//Execute remapping
	int ret = remap_controls(ctrl, nBufs, hookId);
	return ret;
}

int onInputNegative(SceCtrlData *ctrl, int nBufs, int hookId){	
	//Invert for negative logic
	for (int i = 0; i < nBufs; i++)
		ctrl[i].buttons = 0xFFFFFFFF - ctrl[i].buttons;
	
	//Call as for positive logic
	int ret = onInput(ctrl, nBufs, hookId);
	
	//Invert back for negative logic
	for (int i = 0; i < ret; i++)
		ctrl[i].buttons = 0xFFFFFFFF - ctrl[i].buttons;
	
	return ret;
}

int onKernelInput(SceCtrlData *ctrl, int nBufs, int hookId){
    return onInput(ctrl, nBufs, hookId);
}

int onKernelInputNegative(SceCtrlData *ctrl, int nBufs, int hookId){
    return onInputNegative(ctrl, nBufs, hookId);
}

int onTouch(SceUInt32 port, SceTouchData *pData, SceUInt32 nBufs, uint8_t hookId){
	//Disable in menu
    if (isInternalTouchCall) return nBufs;

	if (ui_opened) {
		pData[0] = pData[nBufs - 1];
		pData[0].reportNum = 0;
		return 1;
	} else {
		return remap_touch(port, pData, nBufs, hookId);
    }
	return nBufs;
}

/*export*/ int remaPSV2k_onTouch(SceUInt32 port, SceTouchData *pData, SceUInt32 nBufs, uint8_t hookId){
    if (nBufs < 1 || nBufs > BUFFERS_NUM) return nBufs;
	if (!profile.touch[PROFILE_TOUCH_PSTV_MODE]) return nBufs;
    // LOG("onTouchPatched%i(port:%i, nBufs:%i)\n", hookId, port, nBufs);	
    ksceKernelMemcpyUserToKernel(bufsStd[hookId], (uintptr_t)&pData[0], nBufs * sizeof(SceTouchData)); 
    int ret = onTouch(port, bufsStd[hookId], nBufs, hookId); 
    ksceKernelMemcpyKernelToUser((uintptr_t)&pData[0], bufsStd[hookId], ret * sizeof(SceTouchData)); 
    return ret;
}

#define DECL_FUNC_HOOK_PATCH_CTRL(index, name) \
    static int name##_patched(int port, SceCtrlData *ctrl, int nBufs) { \
		int ret = TAI_CONTINUE(int, refs[(index)], port, ctrl, nBufs); \
        if (ret < 1 || ret > BUFFERS_NUM) return ret; \
        if (profile.controller[PROFILE_CONTROLLER_ENABLED]) return ret; \
        ksceKernelMemcpyUserToKernel(bufsScd[(index)], (uintptr_t)&ctrl[0], ret * sizeof(SceCtrlData)); \
		ret = onInput(bufsScd[(index)], ret, (index)); \
        ksceKernelMemcpyKernelToUser((uintptr_t)&ctrl[0], bufsScd[(index)], ret * sizeof(SceCtrlData)); \
		used_funcs[(index)] = 1; \
        return ret; \
    }
DECL_FUNC_HOOK_PATCH_CTRL(0, sceCtrlPeekBufferPositive)
DECL_FUNC_HOOK_PATCH_CTRL(1, sceCtrlReadBufferPositive)

#define DECL_FUNC_HOOK_PATCH_CTRL_NEGATIVE(index, name) \
    static int name##_patched(int port, SceCtrlData *ctrl, int nBufs) { \
		int ret = TAI_CONTINUE(int, refs[(index)], port, ctrl, nBufs); \
        if (ret < 1 || ret > BUFFERS_NUM) return ret; \
        if (profile.controller[PROFILE_CONTROLLER_ENABLED]) return ret; \
        ksceKernelMemcpyUserToKernel(bufsScd[(index)], (uintptr_t)&ctrl[0], ret * sizeof(SceCtrlData)); \
		ret = onInputNegative(bufsScd[(index)], ret, (index)); \
        ksceKernelMemcpyKernelToUser((uintptr_t)&ctrl[0], bufsScd[(index)], ret * sizeof(SceCtrlData)); \
		used_funcs[(index)] = 1; \
        return ret; \
    }
DECL_FUNC_HOOK_PATCH_CTRL_NEGATIVE(2, sceCtrlPeekBufferNegative)
DECL_FUNC_HOOK_PATCH_CTRL_NEGATIVE(3, sceCtrlReadBufferNegative)

#define DECL_FUNC_HOOK_PATCH_CTRL_EXT(index, name) \
    static int name##_patched(int port, SceCtrlData *ctrl, int nBufs) { \
		int ret = TAI_CONTINUE(int, refs[(index)], port, ctrl, nBufs); \
        if (ret < 1 || ret > BUFFERS_NUM) return ret; \
		if (isInternalExtCall) return ret; \
        if (profile.controller[PROFILE_CONTROLLER_ENABLED] && \
                port != profile.controller[PROFILE_CONTROLLER_PORT]) return ret; \
        ksceKernelMemcpyUserToKernel(bufsScd[(index)], (uintptr_t)&ctrl[0], ret * sizeof(SceCtrlData)); \
		ret = onInput(bufsScd[(index)], ret, (index)); \
        ksceKernelMemcpyKernelToUser((uintptr_t)&ctrl[0], bufsScd[(index)], ret * sizeof(SceCtrlData)); \
		used_funcs[(index)] = 1; \
        return ret; \
    }
DECL_FUNC_HOOK_PATCH_CTRL_EXT(4, sceCtrlPeekBufferPositive2)
DECL_FUNC_HOOK_PATCH_CTRL_EXT(5, sceCtrlReadBufferPositive2)
DECL_FUNC_HOOK_PATCH_CTRL_EXT(6, sceCtrlPeekBufferPositiveExt)
DECL_FUNC_HOOK_PATCH_CTRL_EXT(7, sceCtrlReadBufferPositiveExt)
DECL_FUNC_HOOK_PATCH_CTRL_EXT(8, sceCtrlPeekBufferPositiveExt2)
DECL_FUNC_HOOK_PATCH_CTRL_EXT(9, sceCtrlReadBufferPositiveExt2)

#define DECL_FUNC_HOOK_PATCH_CTRL_EXT_NEGATIVE(index, name) \
    static int name##_patched(int port, SceCtrlData *ctrl, int nBufs) { \
		int ret = TAI_CONTINUE(int, refs[(index)], port, ctrl, nBufs); \
        if (ret < 1 || ret > BUFFERS_NUM) return ret; \
		if (isInternalExtCall) return ret; \
        ksceKernelMemcpyUserToKernel(bufsScd[(index)], (uintptr_t)&ctrl[0], ret * sizeof(SceCtrlData)); \
		ret = onInputNegative(bufsScd[(index)], ret, (index)); \
        ksceKernelMemcpyKernelToUser((uintptr_t)&ctrl[0], bufsScd[(index)], ret * sizeof(SceCtrlData)); \
		used_funcs[(index)] = 1; \
        return ret; \
    }
DECL_FUNC_HOOK_PATCH_CTRL_EXT_NEGATIVE(10, sceCtrlPeekBufferNegative2)
DECL_FUNC_HOOK_PATCH_CTRL_EXT_NEGATIVE(11, sceCtrlReadBufferNegative2)
/*
#define DECL_FUNC_HOOK_PATCH_CTRL_KERNEL(index, name) \
    static int name##_patched(int port, SceCtrlData *ctrl, int nBufs) { \
		int ret = TAI_CONTINUE(int, refs[(index)], port, ctrl, nBufs); \
        if (ret < 1 || ret > BUFFERS_NUM) return ret; \
		used_funcs[(index)] = 1; \
		ret = onKernelInput(ctrl, ret, (index)); \
    }
DECL_FUNC_HOOK_PATCH_CTRL_KERNEL(12, ksceCtrlPeekBufferPositive)
DECL_FUNC_HOOK_PATCH_CTRL_KERNEL(13, ksceCtrlReadBufferPositive)

#define DECL_FUNC_HOOK_PATCH_CTRL_KERNEL_NEGATIVE(index, name) \
    static int name##_patched(int port, SceCtrlData *ctrl, int nBufs) { \
		int ret = TAI_CONTINUE(int, refs[(index)], port, ctrl, nBufs); \
        if (ret < 1 || ret > BUFFERS_NUM) return ret; \
		used_funcs[(index)] = 1; \
		ret = onKernelInputNegative(ctrl, ret, (index)); \
    }
DECL_FUNC_HOOK_PATCH_CTRL_KERNEL_NEGATIVE(14, ksceCtrlPeekBufferNegative)
DECL_FUNC_HOOK_PATCH_CTRL_KERNEL_NEGATIVE(15, ksceCtrlReadBufferNegative)
*/
#define DECL_FUNC_HOOK_PATCH_TOUCH(index, name) \
    static int name##_patched(SceUInt32 port, SceTouchData *pData, SceUInt32 nBufs) { \
		int ret = TAI_CONTINUE(int, refs[(index)], port, pData, nBufs); \
	    if (profile.touch[PROFILE_TOUCH_PSTV_MODE]) return ret; \
        used_funcs[(index)] = 1; \
        if (nBufs < 1 || nBufs > BUFFERS_NUM) return nBufs; \
        return onTouch(port, pData, ret, (index - 16)); \
    }
DECL_FUNC_HOOK_PATCH_TOUCH(16, ksceTouchPeek)
DECL_FUNC_HOOK_PATCH_TOUCH(17, ksceTouchRead)

#define DECL_FUNC_HOOK_PATCH_TOUCH_REGION(index, name) \
    static int name##_patched(SceUInt32 port, SceTouchData *pData, SceUInt32 nBufs, int region) { \
		int ret = TAI_CONTINUE(int, refs[(index)], port, pData, nBufs, region); \
	    if (profile.touch[PROFILE_TOUCH_PSTV_MODE]) return ret; \
        used_funcs[(index)] = 1; \
        if (nBufs < 1 || nBufs > BUFFERS_NUM) return nBufs; \
        return onTouch(port, pData, ret, (index - 16)); \
    }
DECL_FUNC_HOOK_PATCH_TOUCH_REGION(18, ksceTouchPeekRegion)
DECL_FUNC_HOOK_PATCH_TOUCH_REGION(19, ksceTouchReadRegion)

int ksceDisplaySetFrameBufInternal_patched(int head, int index, const SceDisplayFrameBuf *pParam, int sync) {
    used_funcs[20] = 1;

    if (pParam)  
        ui_draw(pParam);

    return TAI_CONTINUE(int, refs[20], head, index, pParam, sync);
}

int ksceKernelInvokeProcEventHandler_patched(int pid, int ev, int a3, int a4, int *a5, int a6) {
    used_funcs[21] = 1;
    char titleidLocal[sizeof(titleid)];
    int ret = ksceKernelLockMutex(mutex_procevent_uid, 1, NULL);
    if (ret < 0)
        goto PROCEVENT_EXIT;
    ksceKernelGetProcessTitleId(pid, titleidLocal, sizeof(titleidLocal));
    if (!strncmp(titleidLocal, "main", sizeof(titleidLocal)))
        strnclone(titleidLocal, HOME, sizeof(titleidLocal));
    switch (ev) {
        case 1: //Start
        case 5: //Resume
            if (!strncmp(titleidLocal, "NPXS", 4)){ //If system app
                SceKernelModuleInfo info;
                info.size = sizeof(SceKernelModuleInfo);
                _ksceKernelGetModuleInfo(pid, _ksceKernelGetProcessMainModule(pid), &info);
                if(strncmp(info.module_name, "ScePspemu", 9) &&
			        strcmp(titleidLocal, "NPXS10012") && //not PS3Link
			        strcmp(titleidLocal, "NPXS10013")) //not PS4Link
                        break; //Use MAIN profile
            }
            
            if (strncmp(titleid, titleidLocal, sizeof(titleid))) {
                strnclone(titleid, titleidLocal, sizeof(titleid));
                processid = pid;
                ui_close();
                for (int i = 0; i < HOOKS_NUM; i++)
                    used_funcs[i] = false;
                profile_load(titleid);
                remap_resetBuffers();
                delayedStartDone = false;
                LOG("\nPROFILE LOAD: %s\n", titleid);
            }
            break;
        case 3: //Close
        case 4: //Suspend
            if (!strcmp(titleid, titleidLocal)){ //If current app suspended
                if (strncmp(titleid, HOME, sizeof(titleid))) {
                    strnclone(titleid, HOME, sizeof(titleid));
                    processid = -1;
                    ui_close();
                    for (int i = 0; i < HOOKS_NUM; i++)
                        used_funcs[i] = false;
                    profile_load(HOME);
                    remap_resetBuffers();
                    delayedStartDone = false;
                    LOG("\nPROFILE LOAD: %s\n", titleid);
                }
            }
            break;
        default:
            break;
    }

    ksceKernelUnlockMutex(mutex_procevent_uid, 1);

PROCEVENT_EXIT:
    return TAI_CONTINUE(int, refs[21], pid, ev, a3, a4, a5, a6);
}

static int main_thread(SceSize args, void *argp) {
    while (thread_run) {
        //Activate delayed start
        if (!delayedStartDone 
            && startTick + profile_settings[3] * 1000000 < ksceKernelGetSystemTimeWide()){
            remap_setup();
	        delayedStartDone = true;
        }

        SceCtrlData ctrl;
        if(ksceCtrlPeekBufferPositive(0, &ctrl, 1) < 1){
            ksceKernelDelayThread(30 * 1000);
            continue;
        }

        //Checking for menu triggering
        if (!ui_opened 
                && (ctrl.buttons & HW_BUTTONS[profile_settings[0]]) 
                && (ctrl.buttons & HW_BUTTONS[profile_settings[1]])) {
            remap_resetBuffers();
            ui_open();
        }

        //In-menu inputs handling
        if (ui_opened){
            ui_onInput(&ctrl);
        }
        
        ksceKernelDelayThread(30 * 1000);
    }
    return 0;
}

// Simplified generic hooking function
void hook(uint8_t hookId, const char *module, uint32_t library_nid, uint32_t func_nid, const void *func) {
    hooks[hookId] = taiHookFunctionExportForKernel(KERNEL_PID, &refs[hookId], module, library_nid, func_nid, func);
}

void _start() __attribute__ ((weak, alias ("module_start")));
int module_start(SceSize argc, const void *args) {
    LOG("\n RemaPSV2 started\n");

    snprintf(titleid, sizeof(titleid), HOME);
    profile_init();
    ui_init();
    remap_init();
    userspace_init();
    startTick = ksceKernelGetSystemTimeWide();

    mutex_procevent_uid = ksceKernelCreateMutex("remaPSV2_mutex_procevent", 0, 0, NULL);

	//Allocate memory for buffers
    bufsMemId = ksceKernelAllocMemBlock("remapsv2_bufs_main", 
        SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_RW, 
        (sizeof(SceTouchData) * BUFFERS_NUM * TOUCH_HOOKS_NUM  + sizeof(SceCtrlData) * BUFFERS_NUM * CTRL_HOOKS_NUM + 0xfff) & ~0xfff, 
        NULL);
    ksceKernelGetMemBlockBase(bufsMemId, (void**)&bufsMemBase);
    LOG("MEMORY ALLOC main[SceCtrlData] %i : %i\n", (int)bufsMemBase, (sizeof(SceTouchData) * BUFFERS_NUM * TOUCH_HOOKS_NUM  + sizeof(SceCtrlData) * BUFFERS_NUM * CTRL_HOOKS_NUM + 0xfff) & ~0xfff);
    for (int i = 0; i < TOUCH_HOOKS_NUM; i++)
        bufsStd[i] = (SceTouchData*)(bufsMemBase + sizeof(SceTouchData) * i * BUFFERS_NUM);
    for (int i = 0; i < CTRL_HOOKS_NUM; i++)
        bufsScd[i] = (SceCtrlData*)(bufsMemBase + 
                (sizeof(SceTouchData) * TOUCH_HOOKS_NUM * BUFFERS_NUM+ 
                sizeof(SceCtrlData) * i * BUFFERS_NUM));

    // Hooking functions
    for (int i = 0; i < HOOKS_NUM; i++)
        hooks[i] = 0;
    hook( 0,"SceCtrl", 0xD197E3C7, 0xA9C3CED6, sceCtrlPeekBufferPositive_patched);
    hook( 1,"SceCtrl", 0xD197E3C7, 0x67E7AB83, sceCtrlReadBufferPositive_patched);

    hook( 2,"SceCtrl", 0xD197E3C7, 0x104ED1A7, sceCtrlPeekBufferNegative_patched);
    hook( 3,"SceCtrl", 0xD197E3C7, 0x15F96FB0, sceCtrlReadBufferNegative_patched);

    hook( 4,"SceCtrl", 0xD197E3C7, 0x15F81E8C, sceCtrlPeekBufferPositive2_patched);
    hook( 5,"SceCtrl", 0xD197E3C7, 0xC4226A3E, sceCtrlReadBufferPositive2_patched);
	hook( 6,"SceCtrl", 0xD197E3C7, 0xA59454D3, sceCtrlPeekBufferPositiveExt_patched);
    hook( 7,"SceCtrl", 0xD197E3C7, 0xE2D99296, sceCtrlReadBufferPositiveExt_patched);
    hook( 8,"SceCtrl", 0xD197E3C7, 0x860BF292, sceCtrlPeekBufferPositiveExt2_patched);
    hook( 9,"SceCtrl", 0xD197E3C7, 0xA7178860, sceCtrlReadBufferPositiveExt2_patched);

    hook(10,"SceCtrl", 0xD197E3C7, 0x81A89660, sceCtrlPeekBufferNegative2_patched);
    hook(11,"SceCtrl", 0xD197E3C7, 0x27A0C5FB, sceCtrlReadBufferNegative2_patched);
    
    // hook(12,"SceCtrl", 0x7823A5D1, 0xEA1D3A34, ksceCtrlPeekBufferPositive_patched);
    // hook(13,"SceCtrl", 0x7823A5D1, 0x9B96A1AA, ksceCtrlReadBufferPositive_patched);

    // hook(14,"SceCtrl", 0x7823A5D1, 0x19895843, ksceCtrlPeekBufferNegative_patched);
    // hook(15,"SceCtrl", 0x7823A5D1, 0x8D4E0DD1, ksceCtrlReadBufferNegative_patched);

    hook(16,"SceTouch", TAI_ANY_LIBRARY, 0xBAD1960B, ksceTouchPeek_patched);
    hook(17,"SceTouch", TAI_ANY_LIBRARY, 0x70C8AACE, ksceTouchRead_patched);
    hook(18,"SceTouch", TAI_ANY_LIBRARY, 0x9B3F7207, ksceTouchPeekRegion_patched);
    hook(19,"SceTouch", TAI_ANY_LIBRARY, 0x9A91F624, ksceTouchReadRegion_patched);

	hook(20,"SceDisplay", 0x9FED47AC, 0x16466675, ksceDisplaySetFrameBufInternal_patched);
				
	hooks[21] = taiHookFunctionImportForKernel(KERNEL_PID, &refs[21],
           "SceProcessmgr", 0x887F19D0, 0x414CC813, ksceKernelInvokeProcEventHandler_patched);

    int ret = module_get_export_func(KERNEL_PID, "SceKernelModulemgr", 0xC445FA63, 0x20A27FA9, 
			(uintptr_t *)&_ksceKernelGetProcessMainModule); // 3.60
    if (ret < 0)
        module_get_export_func(KERNEL_PID, "SceKernelModulemgr", 0x92C9FFC2, 0x679F5144, 
			(uintptr_t *)&_ksceKernelGetProcessMainModule); // 3.65
    ret = module_get_export_func(KERNEL_PID, "SceKernelModulemgr", 0xC445FA63, 0xD269F915, 
			(uintptr_t *)&_ksceKernelGetModuleInfo); // 3.60
    if (ret < 0)
        module_get_export_func(KERNEL_PID, "SceKernelModulemgr", 0x92C9FFC2, 0xDAA90093, 
			(uintptr_t *)&_ksceKernelGetModuleInfo); // 3.65

    thread_uid = ksceKernelCreateThread("remaPSV2_thread", main_thread, 0x3C, 0x3000, 0, 0x10000, 0);
    ksceKernelStartThread(thread_uid, 0, NULL);

    log_flush();

    return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args) {
    if (thread_uid >= 0) {
        thread_run = 0;
        ksceKernelWaitThreadEnd(thread_uid, NULL, NULL);
        ksceKernelDeleteThread(thread_uid);
    }

    for (int i = 0; i < HOOKS_NUM; i++) {
        if (hooks[i] >= 0)
            taiHookReleaseForKernel(hooks[i], refs[i]);
    }

    if (mutex_procevent_uid >= 0)
        ksceKernelDeleteMutex(mutex_procevent_uid);

    //Free mem
	ksceKernelFreeMemBlock(bufsMemId);

    profile_destroy();
    ui_destroy();
    remap_destroy();
    userspace_destroy();
    log_flush();
    return SCE_KERNEL_STOP_SUCCESS;
}

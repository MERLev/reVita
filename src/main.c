#include <vitasdkkern.h>
#include <taihen.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <psp2/motion.h> 
#include <psp2/touch.h>
#include <psp2kern/kernel/sysmem.h> 

#include "vitasdkext.h"
#include "main.h"
#include "ui.h"
#include "ctrl.h"
#include "remap.h"
#include "profile.h"
#include "common.h"
#include "log.h"

int module_get_offset(SceUID pid, SceUID modid, int segidx, size_t offset, uintptr_t *addr);
int module_get_export_func(SceUID pid, const char *modname, uint32_t libnid, uint32_t funcnid, uintptr_t *func);
bool ksceAppMgrIsExclusiveProcessRunning();
//void _sceMotionReset(void);

static tai_hook_ref_t refs[HOOKS_NUM];
static SceUID         hooks[HOOKS_NUM];
static SceUID mutex_procevent_uid = -1;
static SceUID thread_uid = -1;
static bool   thread_run = true;

SceUID processId = INVALID_PID;
char titleid[32] = "";
bool is_in_pspemu = false;

uint8_t used_funcs[HOOKS_NUM];
uint16_t TOUCH_SIZE[4] = {
	1920, 1088,	//Front
	1919, 890	//Rear
};
uint8_t internal_touch_call = 0;
uint8_t internal_ext_call = 0;

static uint64_t startTick;
uint8_t delayedStartDone = 0;

SceUID (*_ksceKernelGetProcessMainModule)(SceUID pid);
int (*_ksceKernelGetModuleInfo)(SceUID pid, SceUID modid, SceKernelModuleInfo *info);
//int (*_sceMotionReset)(void);

void delayedStart(){
    //ksceKernelGetSystemTimeLow
	delayedStartDone = 1;
	// Enabling analogs sampling 
	ksceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG_WIDE);
	
	// Enabling both touch panels sampling
	ksceTouchSetSamplingState(SCE_TOUCH_PORT_FRONT, SCE_TOUCH_SAMPLING_STATE_START);
	ksceTouchSetSamplingState(SCE_TOUCH_PORT_BACK, SCE_TOUCH_SAMPLING_STATE_START);
	
	// Detecting touch panels size
    // Mby later can be used for proper ds4 support
	/*SceTouchPanelInfo pi;	
	int ret = ksceTouchGetPanelInfo(SCE_TOUCH_PORT_FRONT, &pi);
	if (ret >= 0){
		TOUCH_SIZE[0] = pi.maxAaX;
		TOUCH_SIZE[1] = pi.maxAaY;
	}
	ret = ksceTouchGetPanelInfo(SCE_TOUCH_PORT_BACK, &pi);
	if (ret >= 0){
		TOUCH_SIZE[2] = pi.maxAaX;
		TOUCH_SIZE[3] = pi.maxAaY;
	}*/
    
	//ToDo
	//ksceCtrlSetSamplingModeExt(SCE_CTRL_MODE_ANALOG_WIDE);
	// Enabling gyro sampling
	//_sceMotionReset();
	//sceMotionStartSampling();
	//if (profile_gyro[6] == 1) sceMotionSetDeadband(1);
	//else if (profile_gyro[6] == 2) sceMotionSetDeadband(0);
	//ToDo decide on sceMotionSetTiltCorrection usage
	//if (gyro_options[7] == 1) sceMotionSetTiltCorrection(0); 
}

int onInputExt(SceCtrlData *ctrl, int nBufs, int hookId){
	//Nothing to do here
	if (nBufs < 1)
		return nBufs;	

	//Reset wheel gyro buttons pressed
	if (profile_gyro[7] == 1 
            && (ctrl[nBufs - 1].buttons & HW_BUTTONS[profile_gyro[8]]) 
			&& (ctrl[nBufs - 1].buttons & HW_BUTTONS[profile_gyro[9]])) {
		//ToDo
        //sceMotionReset();		
	}
	
	//Combo to save profile used
	if (!ui_opened 
			&& (ctrl[nBufs - 1].buttons & SCE_CTRL_START) 
			&& (ctrl[nBufs - 1].buttons & SCE_CTRL_TRIANGLE)) {
		profile_loadGlobal();
		profile_saveLocal();
	}

	//Checking for menu triggering
	if (!ui_opened 
			&& (ctrl[nBufs - 1].buttons & HW_BUTTONS[profile_settings[0]]) 
			&& (ctrl[nBufs - 1].buttons & HW_BUTTONS[profile_settings[1]])) {
		remap_resetCtrlBuffers(hookId);
		ui_open();
	}
	//In-menu inputs handling
	if (ui_opened){
		ctrl_inputHandler(&ctrl[nBufs - 1]);
		
		//Nullify all inputs
		for (int i = 0; i < nBufs; i++)
			ctrl[i].buttons = 0;
		
		return nBufs;
	}
	
	//Execute remapping
	int ret = remap_controls(ctrl, nBufs, hookId);
	return ret;
}

int onInput(SceCtrlData *ctrl, int nBufs, int hookId){	
	//Patch for external controllers support
	if (!ui_opened)
		remap_patchToExt(&ctrl[nBufs - 1]);
	return onInputExt(ctrl, nBufs, hookId);
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

int onTouch(SceUInt32 port, SceTouchData *pData, SceUInt32 nBufs, uint8_t hookId){	
	//Disable in menu
	if (!internal_touch_call && ui_opened) {
		pData[0] = pData[nBufs - 1];
		pData[0].reportNum = 0;
		return 1;
	}
	
	if (ui_opened){	
		//Clear buffers when in menu
		remap_resetTouchBuffers(hookId);
	} else {
		return remap_touch(port, pData, nBufs, hookId);
	}
	return nBufs;
}

#define DECL_FUNC_HOOK_PATCH_CTRL(index, name) \
    static int name##_patched(int port, SceCtrlData *ctrl, int nBufs) { \
		int ret = TAI_CONTINUE(int, refs[(index)], port, ctrl, nBufs); \
        if (ret < 1 || ret > BUFFERS_NUM) return ret; \
        SceCtrlData ctrl_kernel[BUFFERS_NUM]; \
        ksceKernelMemcpyUserToKernel(&ctrl_kernel[0], (uintptr_t)&ctrl[0], ret * sizeof(SceCtrlData)); \
		ret = onInput(ctrl_kernel, ret, (index)); \
        ksceKernelMemcpyKernelToUser((uintptr_t)&ctrl[0], &ctrl_kernel, ret * sizeof(SceCtrlData)); \
		used_funcs[(index)] = 1; \
        return ret; \
    }
DECL_FUNC_HOOK_PATCH_CTRL(0, sceCtrlPeekBufferPositive)
DECL_FUNC_HOOK_PATCH_CTRL(1, sceCtrlPeekBufferPositive2)
DECL_FUNC_HOOK_PATCH_CTRL(2, sceCtrlReadBufferPositive)
DECL_FUNC_HOOK_PATCH_CTRL(3, sceCtrlReadBufferPositive2)

#define DECL_FUNC_HOOK_PATCH_CTRL_NEGATIVE(index, name) \
    static int name##_patched(int port, SceCtrlData *ctrl, int nBufs) { \
		int ret = TAI_CONTINUE(int, refs[(index)], port, ctrl, nBufs); \
        if (ret < 1 || ret > BUFFERS_NUM) return ret; \
        SceCtrlData ctrl_kernel[BUFFERS_NUM]; \
        ksceKernelMemcpyUserToKernel(&ctrl_kernel[0], (uintptr_t)&ctrl[0], ret * sizeof(SceCtrlData)); \
		ret = onInputNegative(ctrl_kernel, ret, (index)); \
        ksceKernelMemcpyKernelToUser((uintptr_t)&ctrl[0], &ctrl_kernel, ret * sizeof(SceCtrlData)); \
		used_funcs[(index)] = 1; \
        return ret; \
    }
DECL_FUNC_HOOK_PATCH_CTRL_NEGATIVE(8, sceCtrlPeekBufferNegative)
DECL_FUNC_HOOK_PATCH_CTRL_NEGATIVE(9, sceCtrlPeekBufferNegative2)
DECL_FUNC_HOOK_PATCH_CTRL_NEGATIVE(10, sceCtrlReadBufferNegative)
DECL_FUNC_HOOK_PATCH_CTRL_NEGATIVE(11, sceCtrlReadBufferNegative2)
#define DECL_FUNC_HOOK_PATCH_CTRL_EXT(index, name) \
    static int name##_patched(int port, SceCtrlData *ctrl, int nBufs) { \
		int ret = TAI_CONTINUE(int, refs[(index)], port, ctrl, nBufs); \
        if (ret < 1 || ret > BUFFERS_NUM) return ret; \
		if (internal_ext_call) return ret; \
        SceCtrlData ctrl_kernel[BUFFERS_NUM]; \
        ksceKernelMemcpyUserToKernel(&ctrl_kernel[0], (uintptr_t)&ctrl[0], ret * sizeof(SceCtrlData)); \
		ret = onInputExt(ctrl_kernel, ret, (index)); \
        ksceKernelMemcpyKernelToUser((uintptr_t)&ctrl[0], &ctrl_kernel, ret * sizeof(SceCtrlData)); \
		used_funcs[(index)] = 1; \
        return ret; \
    }
DECL_FUNC_HOOK_PATCH_CTRL_EXT(4, sceCtrlPeekBufferPositiveExt)
DECL_FUNC_HOOK_PATCH_CTRL_EXT(5, sceCtrlPeekBufferPositiveExt2)
DECL_FUNC_HOOK_PATCH_CTRL_EXT(6, sceCtrlReadBufferPositiveExt)
DECL_FUNC_HOOK_PATCH_CTRL_EXT(7, sceCtrlReadBufferPositiveExt2)
#define DECL_FUNC_HOOK_PATCH_TOUCH(index, name) \
    static int name##_patched(SceUInt32 port, SceTouchData *pData, SceUInt32 nBufs) { \
		int ret = TAI_CONTINUE(int, refs[(index)], port, pData, nBufs); \
		used_funcs[(index)] = 1; \
        return remap_touch(port, pData, ret, (index - 12)); \
    }
DECL_FUNC_HOOK_PATCH_TOUCH(12, ksceTouchRead)
//DECL_FUNC_HOOK_PATCH_TOUCH(13, ksceTouchRead2)
DECL_FUNC_HOOK_PATCH_TOUCH(14, ksceTouchPeek)
//DECL_FUNC_HOOK_PATCH_TOUCH(15, ksceTouchPeek2)

int ksceDisplaySetFrameBufInternal_patched(int head, int index, const SceDisplayFrameBuf *pParam, int sync) {
    used_funcs[16] = 1;

    if (pParam)  
        ui_draw(pParam);
    /*if (!head || !pParam)
        goto DISPLAY_HOOK_RET;

    if (is_in_pspemu)
        goto DISPLAY_HOOK_RET;*/

    //ui_draw(pParam);

    if (index && ksceAppMgrIsExclusiveProcessRunning())
        goto DISPLAY_HOOK_RET; // Do not draw over SceShell overlay


DISPLAY_HOOK_RET:
    return TAI_CONTINUE(int, refs[16], head, index, pParam, sync);
}

int ksceKernelInvokeProcEventHandler_patched(int pid, int ev, int a3, int a4, int *a5, int a6) {
    char titleidLocal[sizeof(titleid)];
    int ret = ksceKernelLockMutex(mutex_procevent_uid, 1, NULL);
    if (ret < 0)
        goto PROCEVENT_EXIT;

    switch (ev) {
        case 1: // startup
        case 5: // resume
            // Ignore startup events if exclusive proc is already running
            if (ksceAppMgrIsExclusiveProcessRunning()
                    && strncmp(titleid, "main", 4) != 0)
                goto PROCEVENT_UNLOCK_EXIT;

            // Check if pid is PspEmu
            SceKernelModuleInfo info;
            info.size = sizeof(SceKernelModuleInfo);
            _ksceKernelGetModuleInfo(pid, _ksceKernelGetProcessMainModule(pid), &info);
            if (!strncmp(info.module_name, "ScePspemu", 9)) {
                is_in_pspemu = true;
                //snprintf(titleidLocal, sizeof(titleidLocal), "ScePspemu");
                //break;
            }

            // Check titleid
            /*ksceKernelGetProcessTitleId(pid, titleidLocal, sizeof(titleidLocal));
            if (!strncmp(titleidLocal, "NPXS", 4))
                goto PROCEVENT_UNLOCK_EXIT;

            break;*/

        case 3: // exit
        case 4: // suspend
            // Check titleid
            ksceKernelGetProcessTitleId(pid, titleidLocal, sizeof(titleidLocal));
            if (!strncmp(titleidLocal, "NPXS", 4))
                goto PROCEVENT_UNLOCK_EXIT;

            snprintf(titleidLocal, sizeof(titleidLocal), "main");
            break;
    }

    if (ev == 1 || ev == 5 || ev == 3 || ev == 4) {
        if (strncmp(titleid, titleidLocal, sizeof(titleid))) {
            strncpy(titleid, titleidLocal, sizeof(titleid));
            // Set current pid
            processId = (ev == 1 || ev == 5) ? pid : INVALID_PID;
            delayedStartDone = 0;
            for (int i = 0; i < HOOKS_NUM; i++)
		        used_funcs[i] = 0;
            startTick = ksceKernelGetSystemTimeWide();
            profile_loadGlobal();
            profile_loadLocal();
        }
    }

PROCEVENT_UNLOCK_EXIT:
    ksceKernelUnlockMutex(mutex_procevent_uid, 1);

PROCEVENT_EXIT:
    return TAI_CONTINUE(int, refs[17], pid, ev, a3, a4, a5, a6);
}

static int main_thread(SceSize args, void *argp) {
    while (thread_run) {
        
        //Activate delayed start
        if (!delayedStartDone 
            && startTick + profile_settings[3] * 1000000 < ksceKernelGetSystemTimeWide()){
            delayedStart();
        }

        //log_flush();
        ksceKernelDelayThread(15 * 1000 * 1000);
        //ksceKernelDelayThread(10 * 1000);
    }
    return 0;
}

// Simplified generic hooking function
void hook(uint8_t hookId, const char *module, uint32_t library_nid, uint32_t func_nid, const void *func) {
    hooks[hookId] = taiHookFunctionExportForKernel(KERNEL_PID, &refs[hookId], module, library_nid, func_nid, func);
}

void _start() __attribute__ ((weak, alias ("module_start")));
int module_start(SceSize argc, const void *args) {
    LOG("Started\n");

    ui_init();
    ctrl_init();
    remap_init();
    startTick = ksceKernelGetSystemTimeWide();

    mutex_procevent_uid = ksceKernelCreateMutex("remaPSV2_mutex_procevent", 0, 0, NULL);

// Hooking functions
    hook( 0,"SceCtrl", 0xD197E3C7, 0xA9C3CED6, sceCtrlPeekBufferPositive_patched);
    hook( 1,"SceCtrl", 0xD197E3C7, 0x15F81E8C, sceCtrlPeekBufferPositive2_patched);
    hook( 2,"SceCtrl", 0xD197E3C7, 0x67E7AB83, sceCtrlReadBufferPositive_patched);
    hook( 3,"SceCtrl", 0xD197E3C7, 0xC4226A3E, sceCtrlReadBufferPositive2_patched);

	hook( 4,"SceCtrl", 0xD197E3C7, 0xA59454D3, sceCtrlPeekBufferPositiveExt_patched);
    hook( 5,"SceCtrl", 0xD197E3C7, 0x860BF292, sceCtrlPeekBufferPositiveExt2_patched);
    hook( 6,"SceCtrl", 0xD197E3C7, 0xE2D99296, sceCtrlReadBufferPositiveExt_patched);
    hook( 7,"SceCtrl", 0xD197E3C7, 0xA7178860, sceCtrlReadBufferPositiveExt2_patched);

    hook( 8,"SceCtrl", 0xD197E3C7, 0x104ED1A7, sceCtrlPeekBufferNegative_patched);
    hook( 9,"SceCtrl", 0xD197E3C7, 0x81A89660, sceCtrlPeekBufferNegative2_patched);
    hook(10,"SceCtrl", 0xD197E3C7, 0x15F96FB0, sceCtrlReadBufferNegative_patched);
    hook(11,"SceCtrl", 0xD197E3C7, 0x27A0C5FB, sceCtrlReadBufferNegative2_patched);

    // Hooks for user touch functions aren`t working
    // hook(12,"SceTouch", 0x3E4F4A81, 0x169A1D58, sceTouchRead_patched);
    // hook(13,"SceTouch", 0x3E4F4A81, 0x39401BEA, sceTouchRead2_patched);
    // hook(14,"SceTouch", 0x3E4F4A81, 0xFF082DF0, sceTouchPeek_patched);
    // hook(15,"SceTouch", 0x3E4F4A81, 0x3AD3D0A1, sceTouchPeek2_patched);

    //Mby we would need some additional addresses here
    hook(12,"SceTouch", TAI_ANY_LIBRARY, 0x70C8AACE, ksceTouchRead_patched);
    //hook(13,"SceTouch", TAI_ANY_LIBRARY, 0x9A91F624, sceTouchRead2_patched);
    hook(14,"SceTouch", TAI_ANY_LIBRARY, 0xBAD1960B, ksceTouchPeek_patched);
    //hook(15,"SceTouch", TAI_ANY_LIBRARY, 0x9B3F7207, sceTouchPeek2_patched);

	hook(16,"SceDisplay", 0x9FED47AC, 0x16466675, ksceDisplaySetFrameBufInternal_patched);
				
	hooks[17] = taiHookFunctionImportForKernel(KERNEL_PID, &refs[17],
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

    // Load main profile
    snprintf(titleid, sizeof(titleid), "main");
	profile_loadSettings();
	profile_loadGlobal();
	profile_loadLocal();

    thread_uid = ksceKernelCreateThread("remaPSV2_thread", main_thread, 0x3C, 0x3000, 0, 0x10000, 0);
    ksceKernelStartThread(thread_uid, 0, NULL);

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

    remap_destroy();
    ui_destroy();
    ctrl_destroy();
    log_flush();
    return SCE_KERNEL_STOP_SUCCESS;
}

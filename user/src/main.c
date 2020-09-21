#include <vitasdk.h>
#include <taihen.h>
#include <stdbool.h>
#include <psp2/motion.h> 
#include <psp2/kernel/threadmgr.h> 
#include "log.h"
#include "../../kernel/src/remapsv.h"

#define DELAY_CONFIG_CHECK 1000000
#define HOOKS_NUM 8

//Threads
// static SceUID thread_motion_uid = -1;
// static bool   thread_motion_run = true;
static SceUID thread_config_uid = -1;
static bool   thread_config_run = true;

static uint8_t current_hook = 0;
static SceUID hooks[HOOKS_NUM];
static tai_hook_ref_t refs[HOOKS_NUM];

bool isInternalTouchCall = false;

Profile profile;

bool btn_has(uint32_t btns, uint32_t btn){
	return (btns & btn) == btn;
}

void btn_add(uint32_t* btns, uint32_t btn){
	*btns = *btns | btn;
}

void btn_del(uint32_t* btns, uint32_t btn){
	*btns = *btns & ~btn;
}

void btn_toggle(uint32_t* btns, uint32_t btn){
	*btns = *btns ^ btn;
}

void swapSideButtons(uint32_t* btns){
	uint32_t oldBtns = *btns;
	btn_del(btns, SCE_CTRL_L2);
	btn_del(btns, SCE_CTRL_R2);
	btn_del(btns, SCE_CTRL_L1);
	btn_del(btns, SCE_CTRL_R1);
	if (btn_has(oldBtns, SCE_CTRL_L2)) 
		btn_add(btns, SCE_CTRL_L1);
	if (btn_has(oldBtns, SCE_CTRL_R2)) 
		btn_add(btns, SCE_CTRL_R1);
	if (btn_has(oldBtns, SCE_CTRL_L1)) 
		btn_add(btns, SCE_CTRL_L2);
	if (btn_has(oldBtns, SCE_CTRL_R1)) 
		btn_add(btns, SCE_CTRL_R2);
}

void fixSideButtons(uint32_t* btns){
	uint32_t oldBtns = *btns;
	btn_del(btns, SCE_CTRL_L2);
	btn_del(btns, SCE_CTRL_R2);
	btn_del(btns, SCE_CTRL_L1);
	btn_del(btns, SCE_CTRL_R1);
	btn_del(btns, SCE_CTRL_L3);
	btn_del(btns, SCE_CTRL_R3);
	if (btn_has(oldBtns, SCE_CTRL_L1)) 
		btn_add(btns, SCE_CTRL_L2);
	if (btn_has(oldBtns, SCE_CTRL_R1)) 
		btn_add(btns, SCE_CTRL_R2);
}

//Used to add support for R1/R3/L1/L3
int patchToExt(int port, SceCtrlData *ctrl, int nBufs, bool positive){
	SceCtrlData pstv_fakepad;
	//ToDo move elsewhere
	sceCtrlSetSamplingModeExt(SCE_CTRL_MODE_ANALOG_WIDE);
	// if (port == 0) port = 1;
	int ret = sceCtrlPeekBufferPositiveExt2(port, &pstv_fakepad, 1);
	if (ret > 0){
		if(!(port == 1 && profile.entries[PR_CO_EMULATE_DS4].v.b))
			fixSideButtons(&pstv_fakepad.buttons);
		ctrl[nBufs - 1] = pstv_fakepad;
		if (positive) 
			ctrl[nBufs - 1].buttons = pstv_fakepad.buttons;
		else 
			ctrl[nBufs - 1].buttons = 0xFFFFFFFF - pstv_fakepad.buttons;
	}
	return nBufs;
}

int sceCtrlPeekBufferPositive_patched(int port, SceCtrlData *ctrl, int nBufs) {
	int ret = TAI_CONTINUE(int, refs[0], port, ctrl, nBufs);
	if (!profile.entries[PR_CO_PATCH_EXT].v.b) return ret;
	if (ret > 0)
		ret = patchToExt(port, ctrl, ret, true);
	return ret;
}
int sceCtrlReadBufferPositive_patched(int port, SceCtrlData *ctrl, int nBufs) {
	int ret = TAI_CONTINUE(int, refs[1], port, ctrl, nBufs);
	if (!profile.entries[PR_CO_PATCH_EXT].v.b) return ret;
	if (ret > 0)
		ret = patchToExt(port, ctrl, ret, true);
	return ret;
}
int sceCtrlPeekBufferNegative_patched(int port, SceCtrlData *ctrl, int nBufs) {
	int ret = TAI_CONTINUE(int, refs[2], port, ctrl, nBufs);
	if (!profile.entries[PR_CO_PATCH_EXT].v.b) return ret;
	if (ret > 0)
		ret = patchToExt(port, ctrl, ret, false);
	return ret;
}
int sceCtrlReadBufferNegative_patched(int port, SceCtrlData *ctrl, int nBufs) {
	int ret = TAI_CONTINUE(int, refs[3], port, ctrl, nBufs);
	if (!profile.entries[PR_CO_PATCH_EXT].v.b) return ret;
	if (ret > 0)
		ret = patchToExt(port, ctrl, ret, false);
	return ret;
}

// Simplified generic hooking function
void hookFunction(uint32_t nid, const void *func) {
	hooks[current_hook] = taiHookFunctionImport(&refs[current_hook],TAI_MAIN_MODULE,TAI_ANY_LIBRARY,nid,func);
	current_hook++;
}

//Thread to send SceMotionState to kernel plugin
// static int motion_thread(SceSize args, void *argp) {
//     while (thread_motion_run) {
// 		SceMotionState motionstate;
//     	int ret = sceMotionGetState(&motionstate);
// 		remaPSV2k_setSceMotionState(&motionstate, ret);
//     }
//     return 0;
// }

//Thread to keep up-to-date config
static int config_thread(SceSize args, void *argp) {
    while (thread_config_run) {
		remaPSV2k_getProfile(&profile);
		// sceMotionSetDeadband(profile.entries[PR_GY_DEADBAND].v.b);
		sceKernelDelayThread(DELAY_CONFIG_CHECK);
    }
    return 0;
}

void _start() __attribute__ ((weak, alias ("module_start")));
int module_start(SceSize argc, const void *args) {
	LOGF("Plugin started\n");
	memset(&profile, 0, sizeof(profile));

	//Send ready to kernel plugin
	remaPSV2k_userPluginReady();
	
	//Start gyro sampling
	//sceMotionStartSampling();

	//Start threads
	thread_config_uid = sceKernelCreateThread("remaPSV2_u_config_thread", config_thread, 64, 0x3000, 0, 0x10000, 0);
    sceKernelStartThread(thread_config_uid, 0, NULL);
	// thread_motion_uid = sceKernelCreateThread("remaPSV2_u_motion_thread", motion_thread, 64, 0x3000, 0, 0x10000, 0);
    // sceKernelStartThread(thread_motion_uid, 0, NULL);

	//Hooking
	hookFunction(0xA9C3CED6, sceCtrlPeekBufferPositive_patched);
	hookFunction(0x67E7AB83, sceCtrlReadBufferPositive_patched);
	hookFunction(0x104ED1A7, sceCtrlPeekBufferNegative_patched);
	hookFunction(0x15F96FB0, sceCtrlReadBufferNegative_patched);

	return SCE_KERNEL_START_SUCCESS;
}
 
int module_stop(SceSize argc, const void *args) {
	
	// if (thread_motion_uid >= 0) {
    //     thread_motion_run = 0;
    //     sceKernelWaitThreadEnd(thread_motion_uid, NULL, NULL);
    //     sceKernelDeleteThread(thread_motion_uid);
    // }
	if (thread_config_uid >= 0) {
        thread_config_run = 0;
        sceKernelWaitThreadEnd(thread_config_uid, NULL, NULL);
        sceKernelDeleteThread(thread_config_uid);
    }

	return SCE_KERNEL_STOP_SUCCESS;
}
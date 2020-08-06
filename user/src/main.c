#include <vitasdk.h>
#include <taihen.h>
#include <stdbool.h>
#include <psp2/motion.h> 
#include <psp2/kernel/threadmgr.h> 
#include "log.h"
#include "../../kernel/src/remapsv.h"

#define DELAY_CONFIG_CHECK 1000000
#define HOOKS_NUM 4

//Threads
static SceUID thread_motion_uid = -1;
static bool   thread_motion_run = true;
static SceUID thread_config_uid = -1;
static bool   thread_config_run = true;

static uint8_t current_hook = 0;
static SceUID hooks[HOOKS_NUM];
static tai_hook_ref_t refs[HOOKS_NUM];

//Configuration
int configVersion = -1;
bool deadband;
bool patchExtEnabled;
int patchExtPort;

//Used to add support for R1/R3/L1/L3
int patchToExt(SceCtrlData *ctrl, int nBufs, bool positive){
	SceCtrlData pstv_fakepad;
	int ret = sceCtrlPeekBufferPositiveExt2(patchExtPort, &pstv_fakepad, 1);
	if (ret > 0){
		if (positive) 
			ctrl[nBufs - 1].buttons = pstv_fakepad.buttons;
		else 
			ctrl[nBufs - 1].buttons = 0xFFFFFFFF - pstv_fakepad.buttons;
	}
	return nBufs;
}

int sceCtrlPeekBufferPositive_patched(int port, SceCtrlData *ctrl, int nBufs) {
	int ret = TAI_CONTINUE(int, refs[0], port, ctrl, nBufs);
	LOG("sceCtrlPeekBufferPositive %i \n", ret);
	if (!patchExtEnabled) return ret;
	if (ret > 0)
		ret = patchToExt(ctrl, ret, true);
	return ret;
}
int sceCtrlReadBufferPositive_patched(int port, SceCtrlData *ctrl, int nBufs) {
	int ret = TAI_CONTINUE(int, refs[1], port, ctrl, nBufs);
	LOG("sceCtrlReadBufferPositive_patched %i \n", ret);
	if (!patchExtEnabled) return ret;
	if (ret > 0)
		ret = patchToExt(ctrl, ret, true);
	return ret;
}
int sceCtrlPeekBufferNegative_patched(int port, SceCtrlData *ctrl, int nBufs) {
	int ret = TAI_CONTINUE(int, refs[2], port, ctrl, nBufs);
	LOG("sceCtrlPeekBufferNegative_patched %i \n", ret);
	if (!patchExtEnabled) return ret;
	if (ret > 0)
		ret = patchToExt(ctrl, ret, false);
	return ret;
}
int sceCtrlReadBufferNegative_patched(int port, SceCtrlData *ctrl, int nBufs) {
	int ret = TAI_CONTINUE(int, refs[3], port, ctrl, nBufs);
	LOG("sceCtrlReadBufferNegative_patched %i \n", ret);
	if (!patchExtEnabled) return ret;
	if (ret > 0)
		ret = patchToExt(ctrl, ret, false);
	return ret;
}

// Simplified generic hooking function
void hookFunction(uint32_t nid, const void *func) {
	hooks[current_hook] = taiHookFunctionImport(&refs[current_hook],TAI_MAIN_MODULE,TAI_ANY_LIBRARY,nid,func);
	current_hook++;
}

//Thread to send SceMotionState to kernel plugin
static int motion_thread(SceSize args, void *argp) {
    while (thread_motion_run) {
		SceMotionState motionstate;
    	int ret = sceMotionGetState(&motionstate);
		remaPSV2k_setSceMotionState(&motionstate, ret);
    }
    return 0;
}

//Thread to keep up-to-date config
static int config_thread(SceSize args, void *argp) {
    while (thread_config_run) {
		int version = remaPSV2k_getConfigVersion();
		if (version != configVersion){
			configVersion = version;
			deadband = remaPSV2k_getConfigDeadband();
			patchExtEnabled = remaPSV2k_getConfigPatchExtEnabled();
			patchExtPort = remaPSV2k_getConfigPatchExtPort();
			LOG("Config updated : %i %i %i : %i\n", 
				deadband, patchExtEnabled, patchExtPort, configVersion);
			log_flush();
			sceMotionSetDeadband(deadband);
		}
		sceKernelDelayThread(DELAY_CONFIG_CHECK);
    }
    return 0;
}

void _start() __attribute__ ((weak, alias ("module_start")));
int module_start(SceSize argc, const void *args) {
	LOG("User plugin started\n");
	log_flush();

	//Send ready to kernel plugin
	remaPSV2k_userPluginReady();
	
	//Start gyro sampling
	sceMotionStartSampling();

	//Hooking
	hookFunction(0xA9C3CED6, sceCtrlPeekBufferPositive_patched);
	hookFunction(0x67E7AB83, sceCtrlReadBufferPositive_patched);
	hookFunction(0x104ED1A7, sceCtrlPeekBufferNegative_patched);
	hookFunction(0x15F96FB0, sceCtrlReadBufferNegative_patched);

	//Start threads
	thread_config_uid = sceKernelCreateThread("remaPSV2_u_config_thread", config_thread, 64, 0x3000, 0, 0x10000, 0);
    sceKernelStartThread(thread_config_uid, 0, NULL);
	thread_motion_uid = sceKernelCreateThread("remaPSV2_u_motion_thread", motion_thread, 64, 0x3000, 0, 0x10000, 0);
    // sceKernelStartThread(thread_motion_uid, 0, NULL);

	return SCE_KERNEL_START_SUCCESS;
}
 
int module_stop(SceSize argc, const void *args) {
	
	if (thread_motion_uid >= 0) {
        thread_motion_run = 0;
        sceKernelWaitThreadEnd(thread_motion_uid, NULL, NULL);
        sceKernelDeleteThread(thread_motion_uid);
    }
	if (thread_config_uid >= 0) {
        thread_config_run = 0;
        sceKernelWaitThreadEnd(thread_config_uid, NULL, NULL);
        sceKernelDeleteThread(thread_config_uid);
    }

	return SCE_KERNEL_STOP_SUCCESS;
}
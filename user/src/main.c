#include <vitasdk.h>
#include <taihen.h>
#include <stdbool.h>
#include <psp2/motion.h> 
#include <psp2/kernel/threadmgr.h> 
#include "log.h"
#include "../../kernel/src/remapsv.h"

#define DELAY_CONFIG_CHECK 1##000##000
#define DELAY_MOTION_SEND      16##666
#define HOOKS_NUM 8

// Threads
static SceUID thread_motion_uid = -1;
static bool   thread_motion_run = true;
static SceUID thread_profile_uid = -1;
static bool   thread_profile_run = true;

// Hooks
static uint8_t current_hook = 0;
static SceUID hooks[HOOKS_NUM];
static tai_hook_ref_t refs[HOOKS_NUM];

Profile profile;
static char titleid[16];

// Simplified generic hooking function
void hookFunction(uint32_t nid, const void *func) {
	hooks[current_hook] = taiHookFunctionImport(&refs[current_hook],TAI_MAIN_MODULE,TAI_ANY_LIBRARY,nid,func);
	current_hook++;
}

// Thread to send SceMotionState to kernel plugin
static int motion_thread(SceSize args, void *argp) {
	int timestamp = 0;
    while (thread_motion_run) {
		SceMotionState motionstate;
    	int ret = sceMotionGetState(&motionstate);
		if (ret >= 0 && timestamp != motionstate.timestamp){
			timestamp = motionstate.timestamp;
			remaPSV2k_setSceMotionState(&motionstate, ret);
		}
		sceKernelDelayThread(DELAY_MOTION_SEND);
    }
    return 0;
}

// Thread to keep up-to-date profile
static int profile_thread(SceSize args, void *argp) {
    while (thread_profile_run) {
		remaPSV2k_getProfile(&profile);
		// sceMotionSetDeadband(profile.entries[PR_GY_DEADBAND].v.b);
		sceKernelDelayThread(DELAY_CONFIG_CHECK);
    }
    return 0;
}

void _start() __attribute__ ((weak, alias ("module_start")));
int module_start(SceSize argc, const void *args) {
	LOGF("Plugin started\n");

	// Skip onto System apps and LiveArea
	sceAppMgrAppParamGetString(0, 12, titleid , 256); 
	if(strncmp(titleid, "NPXS", strlen("NPXS")) == 0)
		return SCE_KERNEL_START_SUCCESS;
	if(strcmp(titleid, "") == 0)
		return SCE_KERNEL_START_SUCCESS;

	memset(&profile, 0, sizeof(profile));

	//Send ready to kernel plugin
	// remaPSV2k_userPluginReady();
	
	//Start gyro sampling
	sceMotionStartSampling();

	//Start threads
	thread_profile_uid = sceKernelCreateThread("remaPSV2_u_profile_thread", profile_thread, 64, 0x3000, 0, 0x10000, 0);
    sceKernelStartThread(thread_profile_uid, 0, NULL);
	thread_motion_uid = sceKernelCreateThread("remaPSV2_u_motion_thread", motion_thread, 64, 0x3000, 0, 0x10000, 0);
    sceKernelStartThread(thread_motion_uid, 0, NULL);

	return SCE_KERNEL_START_SUCCESS;
}
 
int module_stop(SceSize argc, const void *args) {
	
	if (thread_motion_uid >= 0) {
        thread_motion_run = 0;
        sceKernelWaitThreadEnd(thread_motion_uid, NULL, NULL);
        sceKernelDeleteThread(thread_motion_uid);
    }
	if (thread_profile_uid >= 0) {
        thread_profile_run = 0;
        sceKernelWaitThreadEnd(thread_profile_uid, NULL, NULL);
        sceKernelDeleteThread(thread_profile_uid);
    }

	return SCE_KERNEL_STOP_SUCCESS;
}
#include <vitasdk.h>
#include <taihen.h>
#include <stdbool.h>
#include <psp2/motion.h> 
#include <psp2/kernel/threadmgr.h> 
#include "log.h"
#include "../../reVita/src/revita.h"
#include "DSMotionLibrary.h"

#define DELAY_STARTUP      5##000##000
#define DELAY_CONFIG_CHECK 1##000##000
#define DELAY_MOTION_SEND      16##666

// Threads
static SceUID thread_motion_uid = -1;
static bool   thread_motion_run = true;
static SceUID thread_profile_uid = -1;
static bool   thread_profile_run = true;

Profile profile;
uint64_t startTick;
int isDsmotionRunning = 0;

// Thread to send SceMotionState to kernel plugin
static int motion_thread(SceSize args, void *argp) {
	SceULong64 timestamp = 0;
    while (thread_motion_run) {
		SceMotionState motionstate;
    	int ret = sceMotionGetState(&motionstate);
		if (isDsmotionRunning && profile.entries[PR_GY_DS4_MOTION].v.b)
    		dsMotionGetState(&motionstate);
		if (ret >= 0 && timestamp != motionstate.hostTimestamp){
			timestamp = motionstate.hostTimestamp;
			reVita_setSceMotionState(&motionstate, ret);
		}
		sceKernelDelayThread(DELAY_MOTION_SEND);
    }
    return 0;
}

// Thread to keep up-to-date profile
static int profile_thread(SceSize args, void *argp) {
	// Wait for system to boot
	sceKernelDelayThread(DELAY_STARTUP);

	// Check if ds34motion is running
	tai_module_info_t info;
	info.size = sizeof(tai_module_info_t);
	isDsmotionRunning = taiGetModuleInfo("ds34motion", &info) == 0;

	// Start motion sampling
	sceMotionStartSampling();
	if (isDsmotionRunning)
		dsMotionStartSampling();

	// Keep profile up-to-date
    while (thread_profile_run) {
		reVita_getProfile(&profile);
		if (profile.entries[PR_GY_DEADBAND].v.u < 2)
			sceMotionSetDeadband(profile.entries[PR_GY_DEADBAND].v.b);

		sceKernelDelayThread(DELAY_CONFIG_CHECK);
    }
    return 0;
}

void _start() __attribute__ ((weak, alias ("module_start")));
int module_start(SceSize argc, const void *args) {
	LOGF("Plugin started\n");

	memset(&profile, 0, sizeof(profile));

	thread_profile_uid = sceKernelCreateThread("reVita_u_profile_thread", profile_thread, 64, 0x3000, 0, 0x10000, 0);
    sceKernelStartThread(thread_profile_uid, 0, NULL);

	thread_motion_uid = sceKernelCreateThread("reVita_u_motion_thread", motion_thread, 64, 0x3000, 0, 0x10000, 0);
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
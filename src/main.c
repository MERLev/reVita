#include <stddef.h>
#include <vitasdk.h>
#include <taihen.h>
#include <psp2/motion.h> 
#include <libk/string.h>
#include <stdlib.h>
#include <taipool.h>

#include "main.h"
#include "profile.h"
#include "ui.h"
#include "common.h"
#include "remap.h"

uint8_t used_funcs[HOOKS_NUM];
char titleid[16];
uint16_t TOUCH_SIZE[4] = {
	1920, 1088,	//Front
	1919, 890	//Rear
};
int model;
uint8_t internal_touch_call = 0;
uint8_t internal_ext_call = 0;

static uint64_t startTick;
static uint8_t delayedStartDone = 0;

static uint8_t current_hook = 0;
static SceUID hooks[HOOKS_NUM];
static tai_hook_ref_t refs[HOOKS_NUM];

void delayedStart(){
	delayedStartDone = 1;
	// Enabling analogs sampling 
	sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG_WIDE);
	sceCtrlSetSamplingModeExt(SCE_CTRL_MODE_ANALOG_WIDE);
	// Enabling gyro sampling
	sceMotionReset();
	sceMotionStartSampling();
	if (profile_gyro[6] == 1) sceMotionSetDeadband(1);
	else if (profile_gyro[6] == 2) sceMotionSetDeadband(0);
	//ToDo decide on sceMotionSetTiltCorrection usage
	//if (gyro_options[7] == 1) sceMotionSetTiltCorrection(0); 
	
	// Enabling both touch panels sampling
	sceTouchSetSamplingState(SCE_TOUCH_PORT_FRONT, SCE_TOUCH_SAMPLING_STATE_START);
	sceTouchSetSamplingState(SCE_TOUCH_PORT_BACK, SCE_TOUCH_SAMPLING_STATE_START);
	
	// Detecting touch panels size
	SceTouchPanelInfo pi;	
	int ret = sceTouchGetPanelInfo(SCE_TOUCH_PORT_FRONT, &pi);
	if (ret >= 0){
		TOUCH_SIZE[0] = pi.maxAaX;
		TOUCH_SIZE[1] = pi.maxAaY;
	}
	ret = sceTouchGetPanelInfo(SCE_TOUCH_PORT_BACK, &pi);
	if (ret >= 0){
		TOUCH_SIZE[2] = pi.maxAaX;
		TOUCH_SIZE[3] = pi.maxAaY;
	}
}

int onInputExt(SceCtrlData *ctrl, int nBufs, int hookId){
	//Nothing to do here
	if (nBufs < 1)
		return nBufs;	
	
	//Activate delayed start
	if (!delayedStartDone 
		&& startTick + profile_settings[3] * 1000000 < sceKernelGetProcessTimeWide()){
		delayedStart();
	}
	
	//Reset wheel gyro buttons pressed
	if (profile_gyro[7] == 1 &&
			(ctrl[nBufs - 1].buttons & HW_BUTTONS[profile_gyro[8]]) 
				&& (ctrl[nBufs - 1].buttons & HW_BUTTONS[profile_gyro[9]])) {
		sceMotionReset();		
	}
	
	//Combo to save profile used
	if (!ui_opened 
			&& (ctrl[nBufs - 1].buttons & SCE_CTRL_START) 
			&& (ctrl[nBufs - 1].buttons & SCE_CTRL_TRIANGLE)) {
		profile_loadGlobal();
		profile_saveLocal();
	}
	
	//Checking for menu triggering
	if (used_funcs[16] && !ui_opened 
			&& (ctrl[nBufs - 1].buttons & HW_BUTTONS[profile_settings[0]]) 
			&& (ctrl[nBufs - 1].buttons & HW_BUTTONS[profile_settings[1]])) {
		remap_resetCtrlBuffers(hookId);
		ui_open();
	}
	
	//In-menu inputs handling
	if (ui_opened){
		ui_inputHandler(&ctrl[nBufs - 1]);
		
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
	//Nothing to do here
	if (nBufs < 1)
		return nBufs;	
	
	//Patch for external controllers support
	if (!ui_opened)
		remap_patchToExt(&ctrl[nBufs - 1]);
	
	return onInputExt(ctrl, nBufs, hookId);
}

int onInputNegative(SceCtrlData *ctrl, int nBufs, int hookId){
	//Nothing to do here
	if (nBufs < 1)
		return nBufs;	
	
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
	//Nothing to do here
	if (nBufs < 1)
		return nBufs;	
	
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

int sceCtrlPeekBufferPositive_patched(int port, SceCtrlData *ctrl, int nBufs) {
	int ret = TAI_CONTINUE(int, refs[0], port, ctrl, nBufs);
	ret = onInput(ctrl, ret, 0);
	used_funcs[0] = 1;
	return ret;
}

int sceCtrlPeekBufferPositive2_patched(int port, SceCtrlData *ctrl, int nBufs) {
	int ret = TAI_CONTINUE(int, refs[1], port, ctrl, nBufs);
	ret = onInput(ctrl, ret, 1);
	used_funcs[1] = 1;
	return ret;
}

int sceCtrlReadBufferPositive_patched(int port, SceCtrlData *ctrl, int nBufs) {
	int ret = TAI_CONTINUE(int, refs[2], port, ctrl, nBufs);
	ret = onInput(ctrl, ret, 2);
	used_funcs[2] = 1;
	return ret;
}

int sceCtrlReadBufferPositive2_patched(int port, SceCtrlData *ctrl, int nBufs) {
	int ret = TAI_CONTINUE(int, refs[3], port, ctrl, nBufs);
	ret = onInput(ctrl, ret, 3);
	used_funcs[3] = 1;
	return ret;
}

int sceCtrlPeekBufferPositiveExt_patched(int port, SceCtrlData *ctrl, int nBufs) {
	int ret = TAI_CONTINUE(int, refs[4], port, ctrl, nBufs);
	if (internal_ext_call) return ret;
	ret = onInputExt(ctrl, ret, 4);
	used_funcs[4] = 1;
	return ret;
}

int sceCtrlPeekBufferPositiveExt2_patched(int port, SceCtrlData *ctrl, int nBufs) {
	int ret = TAI_CONTINUE(int, refs[5], port, ctrl, nBufs);
	if (internal_ext_call) return ret;
	ret = onInputExt(ctrl, ret, 5);
	used_funcs[5] = 1;
	return ret;
}

int sceCtrlReadBufferPositiveExt_patched(int port, SceCtrlData *ctrl, int nBufs) {
	int ret = TAI_CONTINUE(int, refs[6], port, ctrl, nBufs);
	if (internal_ext_call) return ret;
	ret = onInputExt(ctrl, ret, 6);
	used_funcs[6] = 1;
	return ret;
}

int sceCtrlReadBufferPositiveExt2_patched(int port, SceCtrlData *ctrl, int nBufs) {
	int ret = TAI_CONTINUE(int, refs[7], port, ctrl, nBufs);
	if (internal_ext_call) return ret;
	ret = onInputExt(ctrl, ret, 7);
	used_funcs[7] = 1;
	return ret;
}

int sceCtrlPeekBufferNegative_patched(int port, SceCtrlData *ctrl, int nBufs) {
	int ret = TAI_CONTINUE(int, refs[8], port, ctrl, nBufs);
	ret = onInputNegative(ctrl, ret, 8);
	used_funcs[8] = 1;
	return ret;
}

int sceCtrlPeekBufferNegative2_patched(int port, SceCtrlData *ctrl, int nBufs) {
	int ret = TAI_CONTINUE(int, refs[9], port, ctrl, nBufs);
	ret = onInputNegative(ctrl, ret, 9);
	used_funcs[9] = 1;
	return ret;
}

int sceCtrlReadBufferNegative_patched(int port, SceCtrlData *ctrl, int nBufs) {
	int ret = TAI_CONTINUE(int, refs[10], port, ctrl, nBufs);
	ret = onInputNegative(ctrl, ret, 10);
	used_funcs[10] = 1;
	return ret;
}

int sceCtrlReadBufferNegative2_patched(int port, SceCtrlData *ctrl, int nBufs) {
	int ret = TAI_CONTINUE(int, refs[11], port, ctrl, nBufs);
	ret = onInputNegative(ctrl, ret, 11);
	used_funcs[11] = 1;
	return ret;
}

int sceTouchRead_patched(SceUInt32 port, SceTouchData *pData, SceUInt32 nBufs) {
	int ret = TAI_CONTINUE(int, refs[12], port, pData, nBufs);
	used_funcs[12] = 1;
	return remap_touch(port, pData, ret, 0);
}

int sceTouchRead2_patched(SceUInt32 port, SceTouchData *pData, SceUInt32 nBufs) {
	int ret = TAI_CONTINUE(int, refs[13], port, pData, nBufs);
	used_funcs[13] = 1;
	return remap_touch(port, pData, ret, 1);
}

int sceTouchPeek_patched(SceUInt32 port, SceTouchData *pData, SceUInt32 nBufs) {
	int ret = TAI_CONTINUE(int, refs[14], port, pData, nBufs);
	used_funcs[14] = 1;
	return remap_touch(port, pData, ret, 2);
}

int sceTouchPeek2_patched(SceUInt32 port, SceTouchData *pData, SceUInt32 nBufs) {
	int ret = TAI_CONTINUE(int, refs[15], port, pData, nBufs);
	used_funcs[15] = 1;
	return remap_touch(port, pData, ret, 3);
}

int sceDisplaySetFrameBuf_patched(const SceDisplayFrameBuf *pParam, int sync) {
	ui_draw(pParam);
	used_funcs[16] = 1;
	return TAI_CONTINUE(int, refs[16], pParam, sync);
}	

// Simplified generic hooking function
void hookFunction(uint32_t nid, const void *func) {
	hooks[current_hook] = taiHookFunctionImport(&refs[current_hook],TAI_MAIN_MODULE,TAI_ANY_LIBRARY,nid,func);
	current_hook++;
}

void _start() __attribute__ ((weak, alias ("module_start")));
int module_start(SceSize argc, const void *args) {
	
	// Getting game Title ID
	sceAppMgrAppParamGetString(0, 12, titleid , 256);
	
	// For some reason, some system Apps are refusing to start 
	// if this plugin is active; so stop the
	// initialization of the module.
	if(!strcmp(titleid, "") ||
		(strcmp(titleid, "NPXS10028") && //not Adrenaline
			strcmp(titleid, "NPXS10013") && //not PS4Link
			strstr(titleid, "NPXS")))	 //System app
		return SCE_KERNEL_START_SUCCESS;
	
	//Set current tick for delayed startup calculation
	startTick = sceKernelGetProcessTimeWide();
	
	// Setup stuffs
	profile_loadSettings();
	profile_loadGlobal();
	profile_loadLocal();
	model = sceKernelGetModel();
	
	// Initializing used funcs table
	for (int i = 0; i < HOOKS_NUM; i++) {
		used_funcs[i] = 0;
	}
	
	// Initializing taipool mempool for dynamic memory managing
	taipool_init(1024 + 1 * (
		sizeof(SceCtrlData) * (HOOKS_NUM-5) * BUFFERS_NUM + 
		2 * sizeof(SceTouchData) * 4 * BUFFERS_NUM));
	remap_init();
	
	// Hooking functions
	hookFunction(0xA9C3CED6, sceCtrlPeekBufferPositive_patched);
	hookFunction(0x15F81E8C, sceCtrlPeekBufferPositive2_patched);
	hookFunction(0x67E7AB83, sceCtrlReadBufferPositive_patched);
	hookFunction(0xC4226A3E, sceCtrlReadBufferPositive2_patched);
	hookFunction(0xA59454D3, sceCtrlPeekBufferPositiveExt_patched);
	hookFunction(0x860BF292, sceCtrlPeekBufferPositiveExt2_patched);
	hookFunction(0xE2D99296, sceCtrlReadBufferPositiveExt_patched);
	hookFunction(0xA7178860, sceCtrlReadBufferPositiveExt2_patched);
	hookFunction(0x104ED1A7, sceCtrlPeekBufferNegative_patched);
	hookFunction(0x81A89660, sceCtrlPeekBufferNegative2_patched);
	hookFunction(0x15F96FB0, sceCtrlReadBufferNegative_patched);
	hookFunction(0x27A0C5FB, sceCtrlReadBufferNegative2_patched);
	if(!strcmp(titleid, "NPXS10013")) //PS4Link
		return SCE_KERNEL_START_SUCCESS;
	hookFunction(0x169A1D58, sceTouchRead_patched);
	hookFunction(0x39401BEA, sceTouchRead2_patched);
	hookFunction(0xFF082DF0, sceTouchPeek_patched);
	hookFunction(0x3AD3D0A1, sceTouchPeek2_patched);
	
	// For some reason, some Apps are refusing to start 
	// with framebuffer hooked; so skip hooking it
	if(!strcmp(titleid, "NPXS10028") || //Adrenaline
			!strcmp(titleid, "NPXS10013") || //PS4Link
			strstr(titleid, "PSPEMU"))	//ABM
		return SCE_KERNEL_START_SUCCESS;
	
	hookFunction(0x7A410B64, sceDisplaySetFrameBuf_patched);
	
	return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args) {
	// Freeing hooks
	while (current_hook-- > 0) {
		taiHookRelease(hooks[current_hook], refs[current_hook]);
	}
    taipool_term();
	
	return SCE_KERNEL_STOP_SUCCESS;
}
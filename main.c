#include <vitasdk.h>
#include <taihen.h>
#include <libk/string.h>
#include <libk/stdio.h>
#include "renderer.h"

#define HOOKS_NUM         17 // Hooked functions num
#define PHYS_BUTTONS_NUM  16 // Supported physical buttons num
#define BUTTONS_NUM       24 // Supported buttons num
#define MENU_ENTRIES      25 // Config menu entries num

static uint8_t show_menu = 0;
static uint8_t current_hook = 0;
static SceUID hooks[HOOKS_NUM];
static tai_hook_ref_t refs[HOOKS_NUM];
static int cfg_i = 0;
static uint32_t old_buttons;
static char titleid[16];
static char fname[128];
static uint8_t internal_touch_call = 0;
static uint8_t btn_mask[BUTTONS_NUM] = {
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 16, 16, 16, 16, 16, 16, 16
};

static uint32_t btns[BUTTONS_NUM] = {
	SCE_CTRL_CROSS, SCE_CTRL_CIRCLE, SCE_CTRL_TRIANGLE, SCE_CTRL_SQUARE,
	SCE_CTRL_START, SCE_CTRL_SELECT, SCE_CTRL_LTRIGGER, SCE_CTRL_RTRIGGER,
	SCE_CTRL_UP, SCE_CTRL_RIGHT, SCE_CTRL_LEFT, SCE_CTRL_DOWN, SCE_CTRL_L1,
	SCE_CTRL_R1, SCE_CTRL_L3, SCE_CTRL_R3
};

static char* str_btns[BUTTONS_NUM] = {
	"Cross", "Circle", "Triangle", "Square",
	"Start", "Select", "L Trigger (L2)", "R Trigger (L2)",
	"Up", "Right", "Left", "Down", "L1", "R1", "L3", "R3",
	"Touch (TL)", "Touch (TR)", "Touch (BL)", "Touch (BR)",
	"Rear (TL)", "Rear (TR)", "Rear (BL)", "Rear (BR)"
};

// Config Menu Renderer
void drawConfigMenu(){
	drawString(5, 10, "Thanks to Tain Sueiras, nobodywasishere and RaveHeart");
	drawString(5, 30, "for their awesome support on Patreon");
	drawString(5, 50, "remaPSV v.1.0 - CONFIG MENU");
	int i;
	for (i = 0; i < BUTTONS_NUM; i++){
		(i == cfg_i) ? setTextColor(0x0000FF00) : setTextColor(0x00FFFFFF);
		if (i > 12) drawStringF(480, 70 + (i-13)*20, "%s -> %s", str_btns[i], str_btns[btn_mask[i]]);
		else drawStringF(5, 70 + i*20, "%s -> %s", str_btns[i], str_btns[btn_mask[i]]);
	}
	(i == cfg_i) ? setTextColor(0x0000FF00) : setTextColor(0x00FFFFFF);
	drawString(480, 70 + (BUTTONS_NUM-13)*20, "Close Config Menu");
	setTextColor(0x00FF00FF);
}

void applyRemap(SceCtrlData *ctrl, uint8_t j){
	
	// Checking for menu triggering
	if ((ctrl->buttons & SCE_CTRL_START) && (ctrl->buttons & SCE_CTRL_SQUARE)){
		show_menu = 1;
		cfg_i = 0;
		return;
	}
	
	// Gathering real touch data
	SceTouchData front, rear;
	internal_touch_call = 1;
	sceTouchPeek(SCE_TOUCH_PORT_FRONT, &front, 1);
	sceTouchPeek(SCE_TOUCH_PORT_BACK, &rear, 1);
	internal_touch_call = 0;
	
	// Applying remap rules for physical buttons
	int i;
	uint32_t new_map = 0;
	for (i=0;i<PHYS_BUTTONS_NUM;i++){
		if (ctrl->buttons & btns[i]){
			if (!(new_map & btns[btn_mask[i]])) {
				new_map += btns[btn_mask[i]];
			}
		}
	}
	
	// Applying remap rules for front virtual buttons
	for (i=0;i<front.reportNum;i++){
		uint8_t btn_idx;
		if (front.report[i].x > 960 && front.report[i].y > 544) { // Bot Right
			btn_idx = PHYS_BUTTONS_NUM + 3;
			if (btn_mask[btn_idx] < PHYS_BUTTONS_NUM) {
				if (!(new_map & btns[btn_mask[btn_idx]])) {
					new_map += btns[btn_mask[btn_idx]];
				}
			}
		}else if (front.report[i].x <= 960 && front.report[i].y > 544) { // Bot Left
			btn_idx = PHYS_BUTTONS_NUM + 2;
			if (btn_mask[btn_idx] < PHYS_BUTTONS_NUM) {
				if (!(new_map & btns[btn_mask[btn_idx]])) {
					new_map += btns[btn_mask[btn_idx]];
				}
			}
		}else if (front.report[i].x > 960 && front.report[i].y <= 544) { // Top Right
			btn_idx = PHYS_BUTTONS_NUM + 1;
			if (btn_mask[btn_idx] < PHYS_BUTTONS_NUM) {
				if (!(new_map & btns[btn_mask[btn_idx]])) {
					new_map += btns[btn_mask[btn_idx]];
				}
			}
		}else if (front.report[i].x <= 960 && front.report[i].y <= 544) { // Top Left
			btn_idx = PHYS_BUTTONS_NUM;
			if (btn_mask[btn_idx] < PHYS_BUTTONS_NUM) {
				if (!(new_map & btns[btn_mask[btn_idx]])) {
					new_map += btns[btn_mask[btn_idx]];
				}
			}
		}
	}
	
	// Applying remap rules for rear virtual buttons
	for (i=0;i<rear.reportNum;i++){
		uint8_t btn_idx;
		if (rear.report[i].x > 960 && rear.report[i].y > 544) { // Bot Right
			btn_idx = PHYS_BUTTONS_NUM + 7;
			if (btn_mask[btn_idx] < PHYS_BUTTONS_NUM) {
				if (!(new_map & btns[btn_mask[btn_idx]])) {
					new_map += btns[btn_mask[btn_idx]];
				}
			}
		}else if (rear.report[i].x <= 960 && rear.report[i].y > 544) { // Bot Left
			btn_idx = PHYS_BUTTONS_NUM + 6;
			if (btn_mask[btn_idx] < PHYS_BUTTONS_NUM) {
				if (!(new_map & btns[btn_mask[btn_idx]])) {
					new_map += btns[btn_mask[btn_idx]];
				}
			}
		}else if (rear.report[i].x <= 960 && rear.report[i].y <= 544) { // Top Right
			btn_idx = PHYS_BUTTONS_NUM + 5;
			if (btn_mask[btn_idx] < PHYS_BUTTONS_NUM) {
				if (!(new_map & btns[btn_mask[btn_idx]])) {
					new_map += btns[btn_mask[btn_idx]];
				}
			}
		}else if (rear.report[i].x <= 960 && rear.report[i].y <= 544) { // Top Left
			btn_idx = PHYS_BUTTONS_NUM + 4;
			if (btn_mask[btn_idx] < PHYS_BUTTONS_NUM) {
				if (!(new_map & btns[btn_mask[btn_idx]])) {
					new_map += btns[btn_mask[btn_idx]];
				}
			}
		}
	}
	
	ctrl->buttons = new_map;
}

void applyRemapNegative(SceCtrlData *ctrl, uint8_t j){
	
	// Checking for menu triggering
	if ((!(ctrl->buttons & SCE_CTRL_START)) && (!(ctrl->buttons & SCE_CTRL_SQUARE))){
		show_menu = 1;
		cfg_i = 0;
		return;
	}
	
	// Gathering real touch data
	SceTouchData front, rear;
	internal_touch_call = 1;
	sceTouchPeek(SCE_TOUCH_PORT_FRONT, &front, 1);
	sceTouchPeek(SCE_TOUCH_PORT_BACK, &rear, 1);
	internal_touch_call = 0;
	
	// Applying remap rules for physical buttons
	int i;
	uint32_t new_map = 0xFFFFFFFF;
	for (i=0;i<BUTTONS_NUM;i++){
		if (!(ctrl->buttons & btns[i])){
			if (new_map & btns[btn_mask[i]]) {
				new_map -= btns[btn_mask[i]];
			}
		}
	}
	
	// Applying remap rules for front virtual buttons
	for (i=0;i<front.reportNum;i++){
		uint8_t btn_idx;
		if (front.report[i].x > 960 && front.report[i].y > 544) { // Bot Right
			btn_idx = PHYS_BUTTONS_NUM + 3;
			if (btn_mask[btn_idx] < PHYS_BUTTONS_NUM) {
				if (new_map & btns[btn_mask[btn_idx]]) {
					new_map -= btns[btn_mask[btn_idx]];
				}
			}
		}else if (front.report[i].x > 960 && front.report[i].y <= 544) { // Bot Left
			btn_idx = PHYS_BUTTONS_NUM + 2;
			if (btn_mask[btn_idx] < PHYS_BUTTONS_NUM) {
				if (new_map & btns[btn_mask[btn_idx]]) {
					new_map -= btns[btn_mask[btn_idx]];
				}
			}
		}else if (front.report[i].x <= 960 && front.report[i].y > 544) { // Top Right
			btn_idx = PHYS_BUTTONS_NUM + 1;
			if (btn_mask[btn_idx] < PHYS_BUTTONS_NUM) {
				if (new_map & btns[btn_mask[btn_idx]]) {
					new_map -= btns[btn_mask[btn_idx]];
				}
			}
		}else if (front.report[i].x <= 960 && front.report[i].y <= 544) { // Top Left
			btn_idx = PHYS_BUTTONS_NUM;
			if (btn_mask[btn_idx] < PHYS_BUTTONS_NUM) {
				if (new_map & btns[btn_mask[btn_idx]]) {
					new_map -= btns[btn_mask[btn_idx]];
				}
			}
		}
	}
	
	// Applying remap rules for rear virtual buttons
	for (i=0;i<rear.reportNum;i++){
		uint8_t btn_idx;
		if (rear.report[i].x > 960 && rear.report[i].y > 544) { // Bot Right
			btn_idx = PHYS_BUTTONS_NUM + 3;
			if (btn_mask[btn_idx] < PHYS_BUTTONS_NUM) {
				if (new_map & btns[btn_mask[btn_idx]]) {
					new_map -= btns[btn_mask[btn_idx]];
				}
			}
		}else if (rear.report[i].x > 960 && rear.report[i].y <= 544) { // Bot Left
			btn_idx = PHYS_BUTTONS_NUM + 2;
			if (btn_mask[btn_idx] < PHYS_BUTTONS_NUM) {
				if (new_map & btns[btn_mask[btn_idx]]) {
					new_map -= btns[btn_mask[btn_idx]];
				}
			}
		}else if (rear.report[i].x <= 960 && rear.report[i].y > 544) { // Top Right
			btn_idx = PHYS_BUTTONS_NUM + 1;
			if (btn_mask[btn_idx] < PHYS_BUTTONS_NUM) {
				if (new_map & btns[btn_mask[btn_idx]]) {
					new_map -= btns[btn_mask[btn_idx]];
				}
			}
		}else if (rear.report[i].x <= 960 && rear.report[i].y <= 544) { // Top Left
			btn_idx = PHYS_BUTTONS_NUM;
			if (btn_mask[btn_idx] < PHYS_BUTTONS_NUM) {
				if (new_map & btns[btn_mask[btn_idx]]) {
					new_map -= btns[btn_mask[btn_idx]];
				}
			}
		}
	}
	
	ctrl->buttons = new_map;
}

void saveConfig(void){
	
	// Opening config file for the running app
	sprintf(fname, "ux0:/data/remaPSV/%s.bin", titleid);
	SceUID fd = sceIoOpen(fname, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
	
	// Saving buttons mask
	sceIoWrite(fd, btn_mask, BUTTONS_NUM);
	sceIoClose(fd);
	
}

void loadConfig(void){
	
	sceIoMkdir("ux0:/data/remaPSV", 0777); // Just in case the folder doesn't exist
	
	// Getting game Title ID
	sceAppMgrAppParamGetString(0, 12, titleid , 256);
	
	// Loading config file for the selected app if exists
	sprintf(fname, "ux0:/data/remaPSV/%s.bin", titleid);
	SceUID fd = sceIoOpen(fname, SCE_O_RDONLY, 0777);
	if (fd >= 0){
		sceIoRead(fd, btn_mask, BUTTONS_NUM);
		sceIoClose(fd);
	}
	
}

// Input Handler for the Config Menu
void configInputHandler(SceCtrlData *ctrl){
	if ((ctrl->buttons & SCE_CTRL_DOWN) && (!(old_buttons & SCE_CTRL_DOWN))){
		cfg_i++;
		if (cfg_i >= MENU_ENTRIES) cfg_i = 0;
	}else if ((ctrl->buttons & SCE_CTRL_UP) && (!(old_buttons & SCE_CTRL_UP))){
		cfg_i--;
		if (cfg_i < 0) cfg_i = MENU_ENTRIES-1;
	}else if ((ctrl->buttons & SCE_CTRL_RIGHT) && (!(old_buttons & SCE_CTRL_RIGHT))){
		if (cfg_i != MENU_ENTRIES-1) btn_mask[cfg_i] = (btn_mask[cfg_i] + 1) % (PHYS_BUTTONS_NUM + 1);
	}else if ((ctrl->buttons & SCE_CTRL_LEFT) && (!(old_buttons & SCE_CTRL_LEFT))){
		if (cfg_i != MENU_ENTRIES-1) {
			if (btn_mask[cfg_i] == 0) btn_mask[cfg_i] = PHYS_BUTTONS_NUM;
			else btn_mask[cfg_i]--;
		}
	}else if ((ctrl->buttons & SCE_CTRL_CROSS) && (!(old_buttons & SCE_CTRL_CROSS))){
		if (cfg_i == MENU_ENTRIES-1){
			show_menu = 0;
			saveConfig();
		}
	}
	old_buttons = ctrl->buttons;
	ctrl->buttons = 0; // Nulling returned buttons
}

// Input Handler for the Config Menu (negative logic)
void configInputHandlerNegative(SceCtrlData *ctrl){
	if ((old_buttons & SCE_CTRL_DOWN) && (!(ctrl->buttons & SCE_CTRL_DOWN))){
		cfg_i++;
		if (cfg_i >= MENU_ENTRIES) cfg_i = 0;
	}else if ((old_buttons & SCE_CTRL_UP) && (!(ctrl->buttons & SCE_CTRL_UP))){
		cfg_i--;
		if (cfg_i < 0) cfg_i = MENU_ENTRIES-1;
	}else if ((old_buttons & SCE_CTRL_RIGHT) && (!(ctrl->buttons & SCE_CTRL_RIGHT))){
		if (cfg_i != MENU_ENTRIES-1) btn_mask[cfg_i] = (btn_mask[cfg_i] + 1) % (PHYS_BUTTONS_NUM + 1);
	}else if ((old_buttons & SCE_CTRL_LEFT) && (!(ctrl->buttons & SCE_CTRL_LEFT))){
		if (cfg_i != MENU_ENTRIES-1) {
			if (btn_mask[cfg_i] == 0) btn_mask[cfg_i] = PHYS_BUTTONS_NUM;
			else btn_mask[cfg_i]--;
		}
	}else if ((old_buttons & SCE_CTRL_CROSS) && (!(ctrl->buttons & SCE_CTRL_CROSS))){
		if (cfg_i == MENU_ENTRIES-1){
			show_menu = 0;
			saveConfig();
		}
	}
	old_buttons = ctrl->buttons;
	ctrl->buttons = 0xFFFFFFFF; // Nulling returned buttons
}

// Simplified generic hooking function
void hookFunction(uint32_t nid, const void *func){
	hooks[current_hook] = taiHookFunctionImport(&refs[current_hook],TAI_MAIN_MODULE,TAI_ANY_LIBRARY,nid,func);
	current_hook++;
}

int sceCtrlPeekBufferPositive_patched(int port, SceCtrlData *ctrl, int count) {
	int ret = TAI_CONTINUE(int, refs[0], port, ctrl, count);
	if (!show_menu) applyRemap(ctrl, 0);
	else configInputHandler(ctrl);
	return ret;
}

int sceCtrlPeekBufferPositive2_patched(int port, SceCtrlData *ctrl, int count) {
	int ret = TAI_CONTINUE(int, refs[1], port, ctrl, count);
	if (!show_menu) applyRemap(ctrl, 1);
	else configInputHandler(ctrl);
	return ret;
}

int sceCtrlReadBufferPositive_patched(int port, SceCtrlData *ctrl, int count) {
	int ret = TAI_CONTINUE(int, refs[2], port, ctrl, count);
	if (!show_menu) applyRemap(ctrl, 2);
	else configInputHandler(ctrl);
	return ret;
}

int sceCtrlReadBufferPositive2_patched(int port, SceCtrlData *ctrl, int count) {
	int ret = TAI_CONTINUE(int, refs[3], port, ctrl, count);
	if (!show_menu) applyRemap(ctrl, 3);
	else configInputHandler(ctrl);
	return ret;
}

int sceCtrlPeekBufferPositiveExt_patched(int port, SceCtrlData *ctrl, int count) {
	int ret = TAI_CONTINUE(int, refs[4], port, ctrl, count);
	if (!show_menu) applyRemap(ctrl, 4);
	else configInputHandler(ctrl);
	return ret;
}

int sceCtrlPeekBufferPositiveExt2_patched(int port, SceCtrlData *ctrl, int count) {
	int ret = TAI_CONTINUE(int, refs[5], port, ctrl, count);
	if (!show_menu) applyRemap(ctrl, 5);
	else configInputHandler(ctrl);
	return ret;
}

int sceCtrlReadBufferPositiveExt_patched(int port, SceCtrlData *ctrl, int count) {
	int ret = TAI_CONTINUE(int, refs[6], port, ctrl, count);
	if (!show_menu) applyRemap(ctrl, 6);
	else configInputHandler(ctrl);
	return ret;
}

int sceCtrlReadBufferPositiveExt2_patched(int port, SceCtrlData *ctrl, int count) {
	int ret = TAI_CONTINUE(int, refs[7], port, ctrl, count);
	if (!show_menu) applyRemap(ctrl, 7);
	else configInputHandler(ctrl);
	return ret;
}

int sceCtrlPeekBufferNegative_patched(int port, SceCtrlData *ctrl, int count) {
	int ret = TAI_CONTINUE(int, refs[8], port, ctrl, count);
	if (!show_menu) applyRemapNegative(ctrl, 8);
	else configInputHandlerNegative(ctrl);
	return ret;
}

int sceCtrlPeekBufferNegative2_patched(int port, SceCtrlData *ctrl, int count) {
	int ret = TAI_CONTINUE(int, refs[9], port, ctrl, count);
	if (!show_menu) applyRemapNegative(ctrl, 9);
	else configInputHandlerNegative(ctrl);
	return ret;
}

int sceCtrlReadBufferNegative_patched(int port, SceCtrlData *ctrl, int count) {
	int ret = TAI_CONTINUE(int, refs[10], port, ctrl, count);
	if (!show_menu) applyRemapNegative(ctrl, 10);
	else configInputHandlerNegative(ctrl);
	return ret;
}

int sceCtrlReadBufferNegative2_patched(int port, SceCtrlData *ctrl, int count) {
	int ret = TAI_CONTINUE(int, refs[11], port, ctrl, count);
	if (!show_menu) applyRemapNegative(ctrl, 11);
	else configInputHandlerNegative(ctrl);
	return ret;
}

int sceTouchRead_patched(SceUInt32 port, SceTouchData *pData, SceUInt32 nBufs){
	int ret = TAI_CONTINUE(int, refs[12], port, pData, nBufs);
	if (port == SCE_TOUCH_PORT_FRONT){
		if ((btn_mask[PHYS_BUTTONS_NUM] != PHYS_BUTTONS_NUM) ||
			(btn_mask[PHYS_BUTTONS_NUM+1] != PHYS_BUTTONS_NUM) ||
			(btn_mask[PHYS_BUTTONS_NUM+2] != PHYS_BUTTONS_NUM) ||
			(btn_mask[PHYS_BUTTONS_NUM+3] != PHYS_BUTTONS_NUM)){
			pData->reportNum = 0;
		}
	} else {
		if ((btn_mask[PHYS_BUTTONS_NUM+4] != PHYS_BUTTONS_NUM) ||
			(btn_mask[PHYS_BUTTONS_NUM+5] != PHYS_BUTTONS_NUM) ||
			(btn_mask[PHYS_BUTTONS_NUM+6] != PHYS_BUTTONS_NUM) ||
			(btn_mask[PHYS_BUTTONS_NUM+7] != PHYS_BUTTONS_NUM)){
			pData->reportNum = 0;
		}
	}
	return ret;
}

int sceTouchRead2_patched(SceUInt32 port, SceTouchData *pData, SceUInt32 nBufs){
	int ret = TAI_CONTINUE(int, refs[13], port, pData, nBufs);
	if (port == SCE_TOUCH_PORT_FRONT){
		if ((btn_mask[PHYS_BUTTONS_NUM] != PHYS_BUTTONS_NUM) ||
			(btn_mask[PHYS_BUTTONS_NUM+1] != PHYS_BUTTONS_NUM) ||
			(btn_mask[PHYS_BUTTONS_NUM+2] != PHYS_BUTTONS_NUM) ||
			(btn_mask[PHYS_BUTTONS_NUM+3] != PHYS_BUTTONS_NUM)){
			pData->reportNum = 0;
		}
	} else {
		if ((btn_mask[PHYS_BUTTONS_NUM+4] != PHYS_BUTTONS_NUM) ||
			(btn_mask[PHYS_BUTTONS_NUM+5] != PHYS_BUTTONS_NUM) ||
			(btn_mask[PHYS_BUTTONS_NUM+6] != PHYS_BUTTONS_NUM) ||
			(btn_mask[PHYS_BUTTONS_NUM+7] != PHYS_BUTTONS_NUM)){
			pData->reportNum = 0;
		}
	}
	return ret;
}

int sceTouchPeek_patched(SceUInt32 port, SceTouchData *pData, SceUInt32 nBufs){
	int ret = TAI_CONTINUE(int, refs[14], port, pData, nBufs);
	if (internal_touch_call) return ret;
	if (port == SCE_TOUCH_PORT_FRONT){
		if ((btn_mask[PHYS_BUTTONS_NUM] != PHYS_BUTTONS_NUM) ||
			(btn_mask[PHYS_BUTTONS_NUM+1] != PHYS_BUTTONS_NUM) ||
			(btn_mask[PHYS_BUTTONS_NUM+2] != PHYS_BUTTONS_NUM) ||
			(btn_mask[PHYS_BUTTONS_NUM+3] != PHYS_BUTTONS_NUM)){
			pData->reportNum = 0;
		}
	} else {
		if ((btn_mask[PHYS_BUTTONS_NUM+4] != PHYS_BUTTONS_NUM) ||
			(btn_mask[PHYS_BUTTONS_NUM+5] != PHYS_BUTTONS_NUM) ||
			(btn_mask[PHYS_BUTTONS_NUM+6] != PHYS_BUTTONS_NUM) ||
			(btn_mask[PHYS_BUTTONS_NUM+7] != PHYS_BUTTONS_NUM)){
			pData->reportNum = 0;
		}
	}
	return ret;
}

int sceTouchPeek2_patched(SceUInt32 port, SceTouchData *pData, SceUInt32 nBufs){
	int ret = TAI_CONTINUE(int, refs[15], port, pData, nBufs);
	if (port == SCE_TOUCH_PORT_FRONT){
		if ((btn_mask[PHYS_BUTTONS_NUM] != PHYS_BUTTONS_NUM) ||
			(btn_mask[PHYS_BUTTONS_NUM+1] != PHYS_BUTTONS_NUM) ||
			(btn_mask[PHYS_BUTTONS_NUM+2] != PHYS_BUTTONS_NUM) ||
			(btn_mask[PHYS_BUTTONS_NUM+3] != PHYS_BUTTONS_NUM)){
			pData->reportNum = 0;
		}
	} else {
		if ((btn_mask[PHYS_BUTTONS_NUM+4] != PHYS_BUTTONS_NUM) ||
			(btn_mask[PHYS_BUTTONS_NUM+5] != PHYS_BUTTONS_NUM) ||
			(btn_mask[PHYS_BUTTONS_NUM+6] != PHYS_BUTTONS_NUM) ||
			(btn_mask[PHYS_BUTTONS_NUM+7] != PHYS_BUTTONS_NUM)){
			pData->reportNum = 0;
		}
	}
	return ret;
}

int sceDisplaySetFrameBuf_patched(const SceDisplayFrameBuf *pParam, int sync) {
	if (show_menu){
		updateFramebuf(pParam);
		drawConfigMenu();
	}
	return TAI_CONTINUE(int, refs[16], pParam, sync);
}	

void _start() __attribute__ ((weak, alias ("module_start")));
int module_start(SceSize argc, const void *args) {
	
	// Setup stuffs
	loadConfig();
	
	// Enabling both touch panels sampling
	sceTouchSetSamplingState(SCE_TOUCH_PORT_FRONT, SCE_TOUCH_SAMPLING_STATE_START);
	sceTouchSetSamplingState(SCE_TOUCH_PORT_BACK, SCE_TOUCH_SAMPLING_STATE_START);
	
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
	hookFunction(0x169A1D58, sceTouchRead_patched);
	hookFunction(0x39401BEA, sceTouchRead2_patched);
	hookFunction(0xFF082DF0, sceTouchPeek_patched);
	hookFunction(0x3AD3D0A1, sceTouchPeek2_patched);
	hookFunction(0x7A410B64, sceDisplaySetFrameBuf_patched);
	
	return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args) {

	// Freeing hooks
	while (current_hook-- > 0){
		taiHookRelease(hooks[current_hook], refs[current_hook]);
	}
		
	return SCE_KERNEL_STOP_SUCCESS;
	
}
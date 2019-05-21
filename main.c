#include <vitasdk.h>
#include <taihen.h>
#include <libk/string.h>
#include <libk/stdio.h>
#include "renderer.h"

#define HOOKS_NUM         17 // Hooked functions num
#define PHYS_BUTTONS_NUM  16 // Supported physical buttons num
#define TARGET_REMAPS     18 // Supported target remaps num
#define BUTTONS_NUM       32 // Supported buttons num
#define MENU_MODES        4  // Menu modes num

#ifndef max
# define max(a,b) (((a)>(b))?(a):(b))
#endif

enum{
	MAIN_MENU = 0,
	REMAP_MENU,
	ANALOG_MENU,
	FUNCS_LIST
};

static int screen_h = 272;
static uint8_t show_menu = 0;
static uint8_t current_hook = 0;
static SceUID hooks[HOOKS_NUM];
static tai_hook_ref_t refs[HOOKS_NUM];
static int cfg_i = 0;
static int menu_i = MAIN_MENU;
static int model;
static uint32_t old_buttons;
static char titleid[16];
static char fname[128];
static uint8_t internal_touch_call = 0;
static uint8_t internal_ext_call = 0;
static uint8_t new_frame = 1;
static SceCtrlData pstv_fakepad;
static uint8_t analogs_deadzone[] = {50, 50, 50, 50};
static uint8_t btn_mask[BUTTONS_NUM] = {
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
	16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16
};
static uint8_t used_funcs[HOOKS_NUM-1];

static char* str_menus[MENU_MODES] = {
	"MAIN MENU", "REMAP MENU", "ANALOG MENU", "USED FUNCTIONS"
};

static char* str_main_menu[] = {
	"Change remap settings",
	"Change analog remap settings",
	"Show imported functions",
	"Return to the game"
};

static char* str_funcs[HOOKS_NUM-1] = {
	"sceCtrlPeekBufferPositive",
	"sceCtrlPeekBufferPositive2",
	"sceCtrlReadBufferPositive",
	"sceCtrlReadBufferPositive2",
	"sceCtrlPeekBufferPositiveExt",
	"sceCtrlPeekBufferPositiveExt2",
	"sceCtrlReadBufferPositiveExt",
	"sceCtrlReadBufferPositiveExt2",
	"sceCtrlPeekBufferNegative",
	"sceCtrlPeekBufferNegative2",
	"sceCtrlReadBufferNegative",
	"sceCtrlReadBufferNegative2",
	"sceTouchRead",
	"sceTouchRead2",
	"sceTouchPeek",
	"sceTouchPeek2"
};

static uint32_t btns[BUTTONS_NUM] = {
	SCE_CTRL_CROSS, SCE_CTRL_CIRCLE, SCE_CTRL_TRIANGLE, SCE_CTRL_SQUARE,
	SCE_CTRL_START, SCE_CTRL_SELECT, SCE_CTRL_LTRIGGER, SCE_CTRL_RTRIGGER,
	SCE_CTRL_UP, SCE_CTRL_RIGHT, SCE_CTRL_LEFT, SCE_CTRL_DOWN, SCE_CTRL_L1,
	SCE_CTRL_R1, SCE_CTRL_L3, SCE_CTRL_R3
};

static char* str_btns[BUTTONS_NUM] = {
	"Cross", "Circle", "Triangle", "Square",
	"Start", "Select", "L Trigger (L2)", "R Trigger (R2)",
	"Up", "Right", "Left", "Down", "L1", "R1", "L3", "R3",
	"Touch (TL)", "Touch (TR)", "Touch (BL)", "Touch (BR)",
	"Rear (TL)", "Rear (TR)", "Rear (BL)", "Rear (BR)",
	"Left Analog (L)", "Left Analog (R)", "Left Analog (U)",
	"Left Analog (D)", "Right Analog (L)", "Right Analog (R)",
	"Right Analog (U)", "Right Analog (D)"
};

static char* target_btns[TARGET_REMAPS] = {
	"Cross", "Circle", "Triangle", "Square", "Start",
	"Select", "L Trigger (L2)", "R Trigger (R2)", "Up",
	"Right", "Left", "Down", "L1", "R1", "L3", "R3",
	"Original", "Disable"
};

// Config Menu Renderer
void drawConfigMenu() {
	drawString(5, 10, "Thanks to Tain Sueiras, nobodywasishere and RaveHeart");
	drawString(5, 30, "for their awesome support on Patreon");
	drawStringF(5, 50, "remaPSV v.1.2 - %s", str_menus[menu_i]);
	int i, y = 70;
	int screen_entries = (screen_h - 50) / 20;
	switch (menu_i){
	case MAIN_MENU:
		for (i = max(0, cfg_i - (screen_entries - 2)); i < sizeof(str_main_menu)/sizeof(char*); i++) {
			(i == cfg_i) ? setTextColor(0x0000FF00) : setTextColor(0x00FFFFFF);
			drawStringF(5, y, "%s", str_main_menu[i]);
			y += 20;
			if (y + 20 > screen_h) break;
		}
		break;
	case REMAP_MENU:
		for (i = max(0, cfg_i - (screen_entries - 2)); i < BUTTONS_NUM; i++) {
			(i == cfg_i) ? setTextColor(0x0000FF00) : setTextColor(0x00FFFFFF);
			drawStringF(5, y, "%s -> %s", str_btns[i], target_btns[btn_mask[i]]);
			y += 20;
			if (y + 20 > screen_h) break;
		}
		if (y + 20 <= screen_h) {
			(i == cfg_i) ? setTextColor(0x0000FF00) : setTextColor(0x00FFFFFF);
			drawString(5, y, "Return to Main Menu");
		}
		break;
	case ANALOG_MENU:
		drawString(5, 80, "Left Analog:");
		drawString(5, 150, "Right Analog:");
		(0 == cfg_i) ? setTextColor(0x0000FF00) : setTextColor(0x00FFFFFF);
		drawStringF(5, 100, "X Axis Deadzone: %hhu", analogs_deadzone[0]);
		(1 == cfg_i) ? setTextColor(0x0000FF00) : setTextColor(0x00FFFFFF);
		drawStringF(5, 120, "Y Axis Deadzone: %hhu", analogs_deadzone[1]);
		(2 == cfg_i) ? setTextColor(0x0000FF00) : setTextColor(0x00FFFFFF);
		drawStringF(5, 170, "X Axis Deadzone: %hhu", analogs_deadzone[2]);
		(3 == cfg_i) ? setTextColor(0x0000FF00) : setTextColor(0x00FFFFFF);
		drawStringF(5, 190, "Y Axis Deadzone: %hhu", analogs_deadzone[3]);
		(4 == cfg_i) ? setTextColor(0x0000FF00) : setTextColor(0x00FFFFFF);
		drawString(5, 230, "Return to Main Menu");
		break;
	case FUNCS_LIST:
		for (i = max(0, cfg_i - (screen_entries - 2)); i < HOOKS_NUM - 1; i++) {
			(i == cfg_i) ? setTextColor(0x0000FF00) : setTextColor(0x00FFFFFF);
			drawStringF(5, y, "%s : %s", str_funcs[i], used_funcs[i] ? "Yes" : "No");
			y += 20;
			if (y + 20 > screen_h) break;
		}
		if (y + 20 <= screen_h) {
			(i == cfg_i) ? setTextColor(0x0000FF00) : setTextColor(0x00FFFFFF);
			drawString(5, y, "Return to Main Menu");
		}
		break;
	default:
		break;
	}
	setTextColor(0x00FF00FF);
}

void applyRemapRule(uint8_t btn_idx, uint32_t *map) {
	if (btn_mask[btn_idx] < PHYS_BUTTONS_NUM) { // Remap to physical
		if (!(*map & btns[btn_mask[btn_idx]])) {
			*map += btns[btn_mask[btn_idx]];
		}
	} else if (btn_mask[btn_idx] == PHYS_BUTTONS_NUM) { // Original remap
		if (btn_idx < PHYS_BUTTONS_NUM) {
			if (!(*map & btns[btn_idx])) {
				*map += btns[btn_idx];
			}
		}
	}
}

void applyRemap(SceCtrlData *ctrl, int count) {
	
	// Checking for menu triggering
	if ((ctrl->buttons & SCE_CTRL_START) && (ctrl->buttons & SCE_CTRL_SQUARE)) {
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
	for (i=0;i<PHYS_BUTTONS_NUM;i++) {
		if (ctrl->buttons & btns[i]) applyRemapRule(i, &new_map);
	}
	
	// Applying remap rules for front virtual buttons
	for (i=0;i<front.reportNum;i++) {
		if (front.report[i].x > 960 && front.report[i].y > 544) { // Bot Right
			applyRemapRule(PHYS_BUTTONS_NUM + 3, &new_map);
		}else if (front.report[i].x <= 960 && front.report[i].y > 544) { // Bot Left
			applyRemapRule(PHYS_BUTTONS_NUM + 2, &new_map);
		}else if (front.report[i].x > 960 && front.report[i].y <= 544) { // Top Right
			applyRemapRule(PHYS_BUTTONS_NUM + 1, &new_map);
		}else if (front.report[i].x <= 960 && front.report[i].y <= 544) { // Top Left
			applyRemapRule(PHYS_BUTTONS_NUM, &new_map);
		}
	}
	
	// Applying remap rules for rear virtual buttons
	for (i=0;i<rear.reportNum;i++) {
		if (rear.report[i].x > 960 && rear.report[i].y > 544) { // Bot Right
			applyRemapRule(PHYS_BUTTONS_NUM + 7, &new_map);
		}else if (rear.report[i].x <= 960 && rear.report[i].y > 544) { // Bot Left
			applyRemapRule(PHYS_BUTTONS_NUM + 6, &new_map);
		}else if (rear.report[i].x > 960 && rear.report[i].y <= 544) { // Top Right
			applyRemapRule(PHYS_BUTTONS_NUM + 5, &new_map);
		}else if (rear.report[i].x <= 960 && rear.report[i].y <= 544) { // Top Left
			applyRemapRule(PHYS_BUTTONS_NUM + 4, &new_map);
		}
	}
	
	// Applying remap rules for left analog
	if (ctrl->lx < 127 - analogs_deadzone[0]) { // Left
		applyRemapRule(PHYS_BUTTONS_NUM + 8, &new_map);
	} else if (ctrl->lx > 127 + analogs_deadzone[0]) { // Right
		applyRemapRule(PHYS_BUTTONS_NUM + 9, &new_map);
	}
	if (ctrl->ly < 127 - analogs_deadzone[1]) { // Up
		applyRemapRule(PHYS_BUTTONS_NUM + 10, &new_map);
	} else if (ctrl->ly > 127 + analogs_deadzone[1]) { // Down
		applyRemapRule(PHYS_BUTTONS_NUM + 11, &new_map);
	}
	
	// Applying remap rules for right analog
	if (ctrl->rx < 127 - analogs_deadzone[2]) { // Left
		applyRemapRule(PHYS_BUTTONS_NUM + 12, &new_map);
	} else if (ctrl->rx > 127 + analogs_deadzone[2]) { // Right
		applyRemapRule(PHYS_BUTTONS_NUM + 13, &new_map);
	}
	if (ctrl->ry < 127 - analogs_deadzone[3]) { // Up
		applyRemapRule(PHYS_BUTTONS_NUM + 14, &new_map);
	} else if (ctrl->ry > 127 + analogs_deadzone[3]) { // Down
		applyRemapRule(PHYS_BUTTONS_NUM + 15, &new_map);
	}
	
	// Nulling analogs if they're remapped
	if ((btn_mask[PHYS_BUTTONS_NUM+8] != PHYS_BUTTONS_NUM) ||
		(btn_mask[PHYS_BUTTONS_NUM+9] != PHYS_BUTTONS_NUM) ||
		(btn_mask[PHYS_BUTTONS_NUM+10] != PHYS_BUTTONS_NUM) ||
		(btn_mask[PHYS_BUTTONS_NUM+11] != PHYS_BUTTONS_NUM)) {
		for (i = 0; i < count; i++) {
			ctrl[i].lx = ctrl[i].ly = 127;
		}
	}
	if ((btn_mask[PHYS_BUTTONS_NUM+12] != PHYS_BUTTONS_NUM) ||
		(btn_mask[PHYS_BUTTONS_NUM+13] != PHYS_BUTTONS_NUM) ||
		(btn_mask[PHYS_BUTTONS_NUM+14] != PHYS_BUTTONS_NUM) ||
		(btn_mask[PHYS_BUTTONS_NUM+15] != PHYS_BUTTONS_NUM)) {
		for (i = 0; i < count; i++) {
			ctrl[i].rx = ctrl[i].ry = 127;
		}
	}
	
	for (i = 0; i < count; i++) {
		ctrl[i].buttons = new_map;
	}
}

void applyRemapNegative(SceCtrlData *ctrl, int count) {
	ctrl->buttons = 0xFFFFFFFF - ctrl->buttons;
	applyRemap(ctrl, count);
	int i;
	for (i = 0; i < count; i++) {
		ctrl[i].buttons = 0xFFFFFFFF - ctrl[i].buttons;
	}
}

void saveConfig(void) {
	// Opening config file for the running app
	sprintf(fname, "ux0:/data/remaPSV/%s.bin", titleid);
	SceUID fd = sceIoOpen(fname, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
	
	// Saving buttons mask
	sceIoWrite(fd, btn_mask, BUTTONS_NUM);
	sceIoClose(fd);
	
	// Opening analog config file and saving the config
	fd = sceIoOpen("ux0:/data/remaPSV/analogs.bin", SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
	sceIoWrite(fd, analogs_deadzone, 4);
	sceIoClose(fd);
}

void loadConfig(void) {
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
	
	// Loading analog config file
	fd = sceIoOpen("ux0:/data/remaPSV/analogs.bin", SCE_O_RDONLY, 0777);
	if (fd >= 0){
		sceIoRead(fd, analogs_deadzone, 4);
		sceIoClose(fd);
	}
}

// Input Handler for the Config Menu
void configInputHandler(SceCtrlData *ctrl, int count) {
	if (new_frame) {
		int menu_entries = 0;
		switch (menu_i) {
		case MAIN_MENU:
			menu_entries = sizeof(str_main_menu) / sizeof(char*);
			break;
		case REMAP_MENU:
			menu_entries = BUTTONS_NUM + 1;
			break;
		case ANALOG_MENU:
			menu_entries = 5;
			break;
		case FUNCS_LIST:
			menu_entries = HOOKS_NUM;
			break;
		default:
			break;
		}
		if ((ctrl->buttons & SCE_CTRL_DOWN) && (!(old_buttons & SCE_CTRL_DOWN))) {
			cfg_i++;
			if (cfg_i >= menu_entries) cfg_i = 0;
		}else if ((ctrl->buttons & SCE_CTRL_UP) && (!(old_buttons & SCE_CTRL_UP))) {
			cfg_i--;
			if (cfg_i < 0) cfg_i = menu_entries-1;
		}else if ((ctrl->buttons & SCE_CTRL_RIGHT) && (!(old_buttons & SCE_CTRL_RIGHT))) {
			if ((menu_i == REMAP_MENU) && (cfg_i != menu_entries-1)) btn_mask[cfg_i] = (btn_mask[cfg_i] + 1) % TARGET_REMAPS;
			else if ((menu_i == ANALOG_MENU) && (cfg_i != menu_entries-1)) analogs_deadzone[cfg_i] = (analogs_deadzone[cfg_i] + 1) % 128;
		}else if ((ctrl->buttons & SCE_CTRL_LEFT) && (!(old_buttons & SCE_CTRL_LEFT))) {
			if ((menu_i == REMAP_MENU) && (cfg_i != menu_entries-1)) {
				if (btn_mask[cfg_i] == 0) btn_mask[cfg_i] = TARGET_REMAPS - 1;
				else btn_mask[cfg_i]--;
			} else if ((menu_i == ANALOG_MENU) && (cfg_i != menu_entries-1)) {
				if (analogs_deadzone[cfg_i] == 0) analogs_deadzone[cfg_i] = 127;
				else analogs_deadzone[cfg_i]--;
			}
		}else if ((ctrl->buttons & SCE_CTRL_CROSS) && (!(old_buttons & SCE_CTRL_CROSS))) {
			if (cfg_i == menu_entries-1) {
				if (menu_i == MAIN_MENU) {
					show_menu = 0;
					saveConfig();
				} else {
					menu_i = MAIN_MENU;
					cfg_i = 0;
				}
			} else if (menu_i == MAIN_MENU) {
				menu_i = cfg_i + 1;
				cfg_i = 0;
			}
		}else if ((ctrl->buttons & SCE_CTRL_SELECT) && (!(old_buttons & SCE_CTRL_SELECT))) {
			if (menu_i == MAIN_MENU) {
				show_menu = 0;
				saveConfig();
			} else {
				menu_i = MAIN_MENU;
				cfg_i = 0;
			}
		}
	}
	new_frame = 0;
	old_buttons = ctrl->buttons;
	
	int i;
	for (i = 0; i < count; i++) {
		ctrl[i].buttons = 0; // Nulling returned buttons
	}
}

// Input Handler for the Config Menu (negative logic)
void configInputHandlerNegative(SceCtrlData *ctrl, int count) {
	ctrl->buttons = 0xFFFFFFFF - ctrl->buttons;
	configInputHandler(ctrl, count);
	int i;
	for (i = 0; i < count; i++) {
		ctrl[i].buttons = 0xFFFFFFFF; // Nulling returned buttons
	}
}

// Simplified generic hooking function
void hookFunction(uint32_t nid, const void *func) {
	hooks[current_hook] = taiHookFunctionImport(&refs[current_hook],TAI_MAIN_MODULE,TAI_ANY_LIBRARY,nid,func);
	current_hook++;
}

int sceCtrlPeekBufferPositive_patched(int port, SceCtrlData *ctrl, int count) {
	int ret = TAI_CONTINUE(int, refs[0], port, ctrl, count);
	switch (model) {
		case SCE_KERNEL_MODEL_VITATV:
			internal_ext_call = 1;
			sceCtrlPeekBufferPositiveExt2(port, &pstv_fakepad, count);
			ctrl->buttons = pstv_fakepad.buttons;
			internal_ext_call = 0;
		default:
			if (!show_menu) applyRemap(ctrl, count);
			else configInputHandler(ctrl, count);
			used_funcs[0] = 1;
			break;
	}
	return ret;
}

int sceCtrlPeekBufferPositive2_patched(int port, SceCtrlData *ctrl, int count) {
	int ret = TAI_CONTINUE(int, refs[1], port, ctrl, count);
	switch (model) {
		case SCE_KERNEL_MODEL_VITATV:
			internal_ext_call = 1;
			sceCtrlPeekBufferPositiveExt2(port, &pstv_fakepad, count);
			ctrl->buttons = pstv_fakepad.buttons;
			internal_ext_call = 0;
		default:
			if (!show_menu) applyRemap(ctrl, count);
			else configInputHandler(ctrl, count);
			used_funcs[1] = 1;
			break;
	}
	return ret;
}

int sceCtrlReadBufferPositive_patched(int port, SceCtrlData *ctrl, int count) {
	int ret = TAI_CONTINUE(int, refs[2], port, ctrl, count);
	switch (model) {
		case SCE_KERNEL_MODEL_VITATV:
			internal_ext_call = 1;
			sceCtrlReadBufferPositiveExt2(port, &pstv_fakepad, count);
			ctrl->buttons = pstv_fakepad.buttons;
			internal_ext_call = 0;
		default:
			if (!show_menu) applyRemap(ctrl, count);
			else configInputHandler(ctrl, count);
			used_funcs[2] = 1;
			break;
	}
	return ret;
}

int sceCtrlReadBufferPositive2_patched(int port, SceCtrlData *ctrl, int count) {
	int ret = TAI_CONTINUE(int, refs[3], port, ctrl, count);
	switch (model) {
		case SCE_KERNEL_MODEL_VITATV:
			internal_ext_call = 1;
			sceCtrlReadBufferPositiveExt2(port, &pstv_fakepad, count);
			ctrl->buttons = pstv_fakepad.buttons;
			internal_ext_call = 0;
		default:
			if (!show_menu) applyRemap(ctrl, count);
			else configInputHandler(ctrl, count);
			used_funcs[3] = 1;
			break;
	}
	return ret;
}

int sceCtrlPeekBufferPositiveExt_patched(int port, SceCtrlData *ctrl, int count) {
	int ret = TAI_CONTINUE(int, refs[4], port, ctrl, count);
	switch (model) {
		case SCE_KERNEL_MODEL_VITATV:
			internal_ext_call = 1;
			sceCtrlPeekBufferPositiveExt2(port, &pstv_fakepad, count);
			ctrl->buttons = pstv_fakepad.buttons;
			internal_ext_call = 0;
		default:
			if (!show_menu) applyRemap(ctrl, count);
			else configInputHandler(ctrl, count);
			used_funcs[4] = 1;
			break;
	}
	return ret;
}

int sceCtrlPeekBufferPositiveExt2_patched(int port, SceCtrlData *ctrl, int count) {
	int ret = TAI_CONTINUE(int, refs[5], port, ctrl, count);
	if (internal_ext_call) return ret;
	if (!show_menu) applyRemap(ctrl, count);
	else configInputHandler(ctrl, count);
	used_funcs[5] = 1;
	return ret;
}

int sceCtrlReadBufferPositiveExt_patched(int port, SceCtrlData *ctrl, int count) {
	int ret = TAI_CONTINUE(int, refs[6], port, ctrl, count);
	switch (model) {
		case SCE_KERNEL_MODEL_VITATV:
			internal_ext_call = 1;
			sceCtrlReadBufferPositiveExt2(port, &pstv_fakepad, count);
			ctrl->buttons = pstv_fakepad.buttons;
			internal_ext_call = 0;
		default:
			if (!show_menu) applyRemap(ctrl, count);
			else configInputHandler(ctrl, count);
			used_funcs[6] = 1;
			break;
	}
	return ret;
}

int sceCtrlReadBufferPositiveExt2_patched(int port, SceCtrlData *ctrl, int count) {
	int ret = TAI_CONTINUE(int, refs[7], port, ctrl, count);
	if (internal_ext_call) return ret;
	if (!show_menu) applyRemap(ctrl, count);
	else configInputHandler(ctrl, count);
	used_funcs[7] = 1;
	return ret;
}

int sceCtrlPeekBufferNegative_patched(int port, SceCtrlData *ctrl, int count) {
	int ret = TAI_CONTINUE(int, refs[8], port, ctrl, count);
	if (!show_menu) applyRemapNegative(ctrl, count);
	else configInputHandlerNegative(ctrl, count);
	used_funcs[8] = 1;
	return ret;
}

int sceCtrlPeekBufferNegative2_patched(int port, SceCtrlData *ctrl, int count) {
	int ret = TAI_CONTINUE(int, refs[9], port, ctrl, count);
	if (!show_menu) applyRemapNegative(ctrl, count);
	else configInputHandlerNegative(ctrl, count);
	used_funcs[9] = 1;
	return ret;
}

int sceCtrlReadBufferNegative_patched(int port, SceCtrlData *ctrl, int count) {
	int ret = TAI_CONTINUE(int, refs[10], port, ctrl, count);
	if (!show_menu) applyRemapNegative(ctrl, count);
	else configInputHandlerNegative(ctrl, count);
	used_funcs[10] = 1;
	return ret;
}

int sceCtrlReadBufferNegative2_patched(int port, SceCtrlData *ctrl, int count) {
	int ret = TAI_CONTINUE(int, refs[11], port, ctrl, count);
	if (!show_menu) applyRemapNegative(ctrl, count);
	else configInputHandlerNegative(ctrl, count);
	used_funcs[11] = 1;
	return ret;
}

int sceTouchRead_patched(SceUInt32 port, SceTouchData *pData, SceUInt32 nBufs) {
	int ret = TAI_CONTINUE(int, refs[12], port, pData, nBufs);
	if (port == SCE_TOUCH_PORT_FRONT) {
		if ((btn_mask[PHYS_BUTTONS_NUM] != PHYS_BUTTONS_NUM) ||
			(btn_mask[PHYS_BUTTONS_NUM+1] != PHYS_BUTTONS_NUM) ||
			(btn_mask[PHYS_BUTTONS_NUM+2] != PHYS_BUTTONS_NUM) ||
			(btn_mask[PHYS_BUTTONS_NUM+3] != PHYS_BUTTONS_NUM)) {
			pData->reportNum = 0;
		}
	}else {
		if ((btn_mask[PHYS_BUTTONS_NUM+4] != PHYS_BUTTONS_NUM) ||
			(btn_mask[PHYS_BUTTONS_NUM+5] != PHYS_BUTTONS_NUM) ||
			(btn_mask[PHYS_BUTTONS_NUM+6] != PHYS_BUTTONS_NUM) ||
			(btn_mask[PHYS_BUTTONS_NUM+7] != PHYS_BUTTONS_NUM)) {
			pData->reportNum = 0;
		}
	}
	used_funcs[12] = 1;
	return ret;
}

int sceTouchRead2_patched(SceUInt32 port, SceTouchData *pData, SceUInt32 nBufs) {
	int ret = TAI_CONTINUE(int, refs[13], port, pData, nBufs);
	if (port == SCE_TOUCH_PORT_FRONT) {
		if ((btn_mask[PHYS_BUTTONS_NUM] != PHYS_BUTTONS_NUM) ||
			(btn_mask[PHYS_BUTTONS_NUM+1] != PHYS_BUTTONS_NUM) ||
			(btn_mask[PHYS_BUTTONS_NUM+2] != PHYS_BUTTONS_NUM) ||
			(btn_mask[PHYS_BUTTONS_NUM+3] != PHYS_BUTTONS_NUM)) {
			pData->reportNum = 0;
		}
	}else {
		if ((btn_mask[PHYS_BUTTONS_NUM+4] != PHYS_BUTTONS_NUM) ||
			(btn_mask[PHYS_BUTTONS_NUM+5] != PHYS_BUTTONS_NUM) ||
			(btn_mask[PHYS_BUTTONS_NUM+6] != PHYS_BUTTONS_NUM) ||
			(btn_mask[PHYS_BUTTONS_NUM+7] != PHYS_BUTTONS_NUM)) {
			pData->reportNum = 0;
		}
	}
	used_funcs[13] = 1;
	return ret;
}

int sceTouchPeek_patched(SceUInt32 port, SceTouchData *pData, SceUInt32 nBufs) {
	int ret = TAI_CONTINUE(int, refs[14], port, pData, nBufs);
	if (internal_touch_call) return ret;
	if (port == SCE_TOUCH_PORT_FRONT) {
		if ((btn_mask[PHYS_BUTTONS_NUM] != PHYS_BUTTONS_NUM) ||
			(btn_mask[PHYS_BUTTONS_NUM+1] != PHYS_BUTTONS_NUM) ||
			(btn_mask[PHYS_BUTTONS_NUM+2] != PHYS_BUTTONS_NUM) ||
			(btn_mask[PHYS_BUTTONS_NUM+3] != PHYS_BUTTONS_NUM)) {
			pData->reportNum = 0;
		}
	}else {
		if ((btn_mask[PHYS_BUTTONS_NUM+4] != PHYS_BUTTONS_NUM) ||
			(btn_mask[PHYS_BUTTONS_NUM+5] != PHYS_BUTTONS_NUM) ||
			(btn_mask[PHYS_BUTTONS_NUM+6] != PHYS_BUTTONS_NUM) ||
			(btn_mask[PHYS_BUTTONS_NUM+7] != PHYS_BUTTONS_NUM)) {
			pData->reportNum = 0;
		}
	}
	used_funcs[14] = 1;
	return ret;
}

int sceTouchPeek2_patched(SceUInt32 port, SceTouchData *pData, SceUInt32 nBufs) {
	int ret = TAI_CONTINUE(int, refs[15], port, pData, nBufs);
	if (port == SCE_TOUCH_PORT_FRONT){
		if ((btn_mask[PHYS_BUTTONS_NUM] != PHYS_BUTTONS_NUM) ||
			(btn_mask[PHYS_BUTTONS_NUM+1] != PHYS_BUTTONS_NUM) ||
			(btn_mask[PHYS_BUTTONS_NUM+2] != PHYS_BUTTONS_NUM) ||
			(btn_mask[PHYS_BUTTONS_NUM+3] != PHYS_BUTTONS_NUM)) {
			pData->reportNum = 0;
		}
	}else {
		if ((btn_mask[PHYS_BUTTONS_NUM+4] != PHYS_BUTTONS_NUM) ||
			(btn_mask[PHYS_BUTTONS_NUM+5] != PHYS_BUTTONS_NUM) ||
			(btn_mask[PHYS_BUTTONS_NUM+6] != PHYS_BUTTONS_NUM) ||
			(btn_mask[PHYS_BUTTONS_NUM+7] != PHYS_BUTTONS_NUM)) {
			pData->reportNum = 0;
		}
	}
	used_funcs[15] = 1;
	return ret;
}

int sceDisplaySetFrameBuf_patched(const SceDisplayFrameBuf *pParam, int sync) {
	if (show_menu) {
		new_frame = 1;
		screen_h = pParam->height;
		updateFramebuf(pParam);
		drawConfigMenu();
	}
	return TAI_CONTINUE(int, refs[16], pParam, sync);
}	

void _start() __attribute__ ((weak, alias ("module_start")));
int module_start(SceSize argc, const void *args) {
	
	// Setup stuffs
	loadConfig();
	model = sceKernelGetModel();
	
	// Initializing used funcs table
	int i;
	for (i = 0; i < HOOKS_NUM - 1; i++) {
		used_funcs[i] = 0;
	}
	
	// Enabling both touch panels sampling
	sceTouchSetSamplingState(SCE_TOUCH_PORT_FRONT, SCE_TOUCH_SAMPLING_STATE_START);
	sceTouchSetSamplingState(SCE_TOUCH_PORT_BACK, SCE_TOUCH_SAMPLING_STATE_START);
	
	// Enabling analogs sampling
	sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG_WIDE);
	sceCtrlSetSamplingModeExt(SCE_CTRL_MODE_ANALOG_WIDE);
	
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
	while (current_hook-- > 0) {
		taiHookRelease(hooks[current_hook], refs[current_hook]);
	}
		
	return SCE_KERNEL_STOP_SUCCESS;
	
}
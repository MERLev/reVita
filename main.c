#include <vitasdk.h>
#include <taihen.h>
#include <psp2/motion.h> 
#include <libk/string.h>
#include <libk/stdio.h>
#include <stdlib.h>
#include "renderer.h"

#define HOOKS_NUM         17 // Hooked functions num
#define PHYS_BUTTONS_NUM  16 // Supported physical buttons num
#define TARGET_REMAPS     26 // Supported target remaps num
#define BUTTONS_NUM       38 // Supported buttons num
#define MENU_MODES        5  // Menu modes num
#define COLOR_DEFAULT     0x00FFFFFF
#define COLOR_HEADER      0x00FF00FF
#define COLOR_CURSOR      0x0000FF00
#define COLOR_ACTIVE      0x000000FF
#define ANALOGS_DEADZONE_DEF		30
#define ANALOGS_FORCE_DIGITAL_DEF	0
#define ANOLOGS_OPTIONS_NUM			8
#define GYRO_SENS_DEF				127
#define GYRO_DEADZONE_DEF			15
#define GYRO_OPTIONS_NUM			6

#ifndef max
# define max(a,b) (((a)>(b))?(a):(b))
#endif

enum{
	MAIN_MENU = 0,
	REMAP_MENU,
	ANALOG_MENU,
	GYRO_MENU,
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
static uint8_t analogs_options[ANOLOGS_OPTIONS_NUM];
static uint8_t gyro_options[GYRO_OPTIONS_NUM];
static uint8_t btn_mask[BUTTONS_NUM];
static uint8_t used_funcs[HOOKS_NUM-1];

static char* str_menus[MENU_MODES] = {
	"MAIN MENU", "REMAP MENU", "ANALOG MENU", "GYRO_MENU", "USED FUNCTIONS"
};

static char* str_main_menu[] = {
	"Change remap settings",
	"Change analog remap settings",
	"Change gyro remap settings",
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
	"Start", "Select", 
	"L Trigger (L2)", "R Trigger (R2)",
	"Up", "Right", "Left", "Down", 
	"L1", "R1", "L3", "R3",
	"Touch (TL)", "Touch (TR)", "Touch (BL)", "Touch (BR)",
	"Rear (TL)", "Rear (TR)", "Rear (BL)", "Rear (BR)",
	"Left Analog (L)", "Left Analog (R)", "Left Analog (U)", "Left Analog (D)", 
	"Right Analog (L)", "Right Analog (R)", "Right Analog (U)", "Right Analog (D)",
	"Gyro (L)", "Gyro (R)", "Gyro (U)", "Gyro (D)", "Gyro (Roll Left)", "Gyro (Roll Right)"
};

static char* target_btns[TARGET_REMAPS] = {
	"Cross", "Circle", "Triangle", "Square", 
	"Start", "Select", 
	"L Trigger (L2)", "R Trigger (R2)", 
	"Up", "Right", "Left", "Down", 
	"L1", "R1", "L3", "R3", 
	"Original", "Disable",
	"Left Analog (L)", "Left Analog (R)", "Left Analog (U)", "Left Analog (D)", 
	"Right Analog (L)", "Right Analog (R)", "Right Analog (U)", "Right Analog (D)"
};

// Generic clamp function
int32_t clamp(int32_t value, int32_t mini, int32_t maxi) {
	if (value < mini) { return mini; }
	if (value > maxi) { return maxi; }
	return value;
}

// Buttons to activate the remap menu, defaults to START + SQUARE
static uint8_t menuActivator_mask[2] = {
	4, 3
};

void resetRemaps(){
	for (int i = 0; i < BUTTONS_NUM; i++) {
		btn_mask[i] = PHYS_BUTTONS_NUM;
	}
}

void resetAnalogs(){
	for (int i = 0; i < ANOLOGS_OPTIONS_NUM; i++)
		analogs_options[i] = i < 4 ? ANALOGS_DEADZONE_DEF : ANALOGS_FORCE_DIGITAL_DEF;
}

void resetGyro(){
	for (int i = 0; i < GYRO_OPTIONS_NUM; i++)
		gyro_options[i] = i < 3 ? GYRO_SENS_DEF : GYRO_DEADZONE_DEF;
}

// Config Menu Renderer
void drawConfigMenu() {
	setTextColor(COLOR_HEADER);
	drawString(5, 10, "Thanks to Tain Sueiras, nobodywasishere and RaveHeart");
	drawString(5, 30, "for their awesome support on Patreon");
	drawStringF(5, 50, "remaPSV v.1.2 - %s", str_menus[menu_i]);
	int i, y = 70;
	int screen_entries = (screen_h - 50) / 20;
	switch (menu_i){
	case MAIN_MENU:
		for (i = max(0, cfg_i - (screen_entries - 3)); i < sizeof(str_main_menu)/sizeof(char*); i++) {
			setTextColor((i == cfg_i) ? COLOR_CURSOR : COLOR_DEFAULT);
			drawStringF(5, y, "%s", str_main_menu[i]);
			y += 20;
			if (y + 40 > screen_h) break;
		}
		setTextColor(COLOR_HEADER);
		drawString(5, 520, "(X): select   (select) or (O): save and close");
		break;
	case REMAP_MENU:
		for (i = max(0, cfg_i - (screen_entries - 3)); i < BUTTONS_NUM; i++) {
			setTextColor((i == cfg_i) ? COLOR_CURSOR : ((btn_mask[i] != PHYS_BUTTONS_NUM) ? COLOR_ACTIVE : COLOR_DEFAULT));
			drawStringF(5, y, "%s -> %s", str_btns[i], target_btns[btn_mask[i]]);
			y += 20;
			if (y + 40 > screen_h) break;
		}
		setTextColor(COLOR_HEADER);
		drawString(5, 520, "(<) or (>): change    ([]): reset   (start): reset all   (select) or (O): back");
		break;
	case ANALOG_MENU:
		y -= 20;
		setTextColor(COLOR_DEFAULT);
		drawString(5, y+=20, "Deadzone:");
		setTextColor((0 == cfg_i) ? COLOR_CURSOR : ((analogs_options[0] != ANALOGS_DEADZONE_DEF) ? COLOR_ACTIVE : COLOR_DEFAULT));
		drawStringF(15, y+=20, "Left Analog  [X Axis]: %hhu", analogs_options[0]);
		setTextColor((1 == cfg_i) ? COLOR_CURSOR : ((analogs_options[1] != ANALOGS_DEADZONE_DEF) ? COLOR_ACTIVE : COLOR_DEFAULT));
		drawStringF(15, y+=20, "Left Analog  [Y Axis]: %hhu", analogs_options[1]);
		setTextColor((2 == cfg_i) ? COLOR_CURSOR : ((analogs_options[2] != ANALOGS_DEADZONE_DEF) ? COLOR_ACTIVE : COLOR_DEFAULT));
		drawStringF(15, y+=20, "Right Analog [X Axis]: %hhu", analogs_options[2]);
		setTextColor((3 == cfg_i) ? COLOR_CURSOR : ((analogs_options[3] != ANALOGS_DEADZONE_DEF) ? COLOR_ACTIVE : COLOR_DEFAULT));
		drawStringF(15, y+=20, "Right Analog [Y Axis]: %hhu", analogs_options[3]);
		setTextColor(COLOR_DEFAULT);
		drawString(5, y+=20, "Force digital output:");
		setTextColor((4 == cfg_i) ? COLOR_CURSOR : ((analogs_options[4] != ANALOGS_FORCE_DIGITAL_DEF) ? COLOR_ACTIVE : COLOR_DEFAULT));
		drawStringF(15, y+=20, "Left Analog  [X Axis]: %s", analogs_options[4] ? "Yes" : "No");
		setTextColor((5 == cfg_i) ? COLOR_CURSOR : ((analogs_options[5] != ANALOGS_FORCE_DIGITAL_DEF) ? COLOR_ACTIVE : COLOR_DEFAULT));
		drawStringF(15, y+=20, "Left Analog  [Y Axis]: %s", analogs_options[5] ? "Yes" : "No");
		setTextColor((6 == cfg_i) ? COLOR_CURSOR : ((analogs_options[6] != ANALOGS_FORCE_DIGITAL_DEF) ? COLOR_ACTIVE : COLOR_DEFAULT));
		drawStringF(15, y+=20, "Right Analog [X Axis]: %s", analogs_options[6] ? "Yes" : "No");
		setTextColor((7 == cfg_i) ? COLOR_CURSOR : ((analogs_options[7] != ANALOGS_FORCE_DIGITAL_DEF) ? COLOR_ACTIVE : COLOR_DEFAULT));
		drawStringF(15, y+=20, "Right Analog [Y Axis]: %s", analogs_options[7] ? "Yes" : "No");
		setTextColor(COLOR_HEADER);
		drawString(5, 520, "(<) or (>): change    ([]): reset   (start): reset all    (select) or (O): back");
		break;
	case GYRO_MENU:
		y -= 20;
		setTextColor(COLOR_DEFAULT);
		drawString(5, y+=20, "Sensivity:");
		setTextColor((0 == cfg_i) ? COLOR_CURSOR : ((gyro_options[0] != GYRO_SENS_DEF) ? COLOR_ACTIVE : COLOR_DEFAULT));
		drawStringF(15, y+=20, "X Axis: %hhu", gyro_options[0]);
		setTextColor((1 == cfg_i) ? COLOR_CURSOR : ((gyro_options[1] != GYRO_SENS_DEF) ? COLOR_ACTIVE : COLOR_DEFAULT));
		drawStringF(15, y+=20, "Y Axis: %hhu", gyro_options[1]);
		setTextColor((2 == cfg_i) ? COLOR_CURSOR : ((gyro_options[2] != GYRO_SENS_DEF) ? COLOR_ACTIVE : COLOR_DEFAULT));
		drawStringF(15, y+=20, "Z Axis: %hhu", gyro_options[2]);
		setTextColor(COLOR_DEFAULT);
		drawString(5, y+=20, "Deadzone for digital mode:");
		setTextColor((3 == cfg_i) ? COLOR_CURSOR : ((gyro_options[3] != GYRO_DEADZONE_DEF) ? COLOR_ACTIVE : COLOR_DEFAULT));
		drawStringF(15, y+=20, "X Axis: %hhu", gyro_options[3]);
		setTextColor((4 == cfg_i) ? COLOR_CURSOR : ((gyro_options[4] != GYRO_DEADZONE_DEF) ? COLOR_ACTIVE : COLOR_DEFAULT));
		drawStringF(15, y+=20, "Y Axis: %hhu", gyro_options[4]);
		setTextColor((5 == cfg_i) ? COLOR_CURSOR : ((gyro_options[5] != GYRO_DEADZONE_DEF) ? COLOR_ACTIVE : COLOR_DEFAULT));
		drawStringF(15, y+=20, "Z Axis: %hhu", gyro_options[5]);
		setTextColor(COLOR_HEADER);
		drawString(5, 520, "(<) or (>): change    ([]): reset   (start): reset all    (select) or (O): back");
		break;
	case FUNCS_LIST:
		for (i = max(0, cfg_i - (screen_entries - 3)); i < HOOKS_NUM - 1; i++) {
			setTextColor((i == cfg_i) ? COLOR_CURSOR : (used_funcs[i] ? COLOR_ACTIVE : COLOR_DEFAULT));
			drawStringF(5, y, "%s : %s", str_funcs[i], used_funcs[i] ? "Yes" : "No");
			y += 20;
			if (y + 40 > screen_h) break;
		}
		setTextColor(COLOR_HEADER);
		drawString(5, 520, "(select) or (O): back");
		break;
	default:
		break;
	}
}

void applyRemapRule(uint8_t btn_idx, uint32_t* map, uint32_t* stickpos) {
	if (btn_mask[btn_idx] < PHYS_BUTTONS_NUM) { // Btn -> Btn
		if (!(*map & btns[btn_mask[btn_idx]])) {
			*map += btns[btn_mask[btn_idx]];
		}

	} else if (btn_mask[btn_idx] == PHYS_BUTTONS_NUM) { // Btn -> Original
		if (btn_idx < PHYS_BUTTONS_NUM) {
			if (!(*map & btns[btn_idx])) {
				*map += btns[btn_idx];
			}
		}
	} else if (PHYS_BUTTONS_NUM + 1 < btn_mask[btn_idx] && btn_mask[btn_idx] < PHYS_BUTTONS_NUM + 10) { 
		// Btn -> Analog
		stickpos[btn_mask[btn_idx] - (PHYS_BUTTONS_NUM + 2)] += 127;
	}
}

void applyRemapRuleForAnalog(uint8_t btn_idx, uint32_t* map, uint32_t* stickpos, uint8_t stickposval) {
	if (PHYS_BUTTONS_NUM + 1 < btn_mask[btn_idx] && btn_mask[btn_idx] < PHYS_BUTTONS_NUM + 10
		&& !analogs_options[4 + (int) ((btn_idx - PHYS_BUTTONS_NUM - 8) / 2)]) {
		// Analog -> Analog [ANALOG MODE]
		stickpos[btn_mask[btn_idx] - (PHYS_BUTTONS_NUM + 2)] += 127 - stickposval;
	} else {
		// Analog -> Analog [DIGITAL MODE] && Analog -> Button
		applyRemapRule(btn_idx, map, stickpos);
	}
}

void applyRemapRuleForGyro(uint8_t btn_idx, uint32_t* map, uint32_t* stickpos, float gyroval){
	if (PHYS_BUTTONS_NUM + 1 < btn_mask[btn_idx] && btn_mask[btn_idx] < PHYS_BUTTONS_NUM + 10) {
		// Gyro -> Analog remap
		stickpos[btn_mask[btn_idx] - (PHYS_BUTTONS_NUM + 2)] += gyroval;
	} else {
		// Gyro -> Btn remap
		if ((((btn_idx == PHYS_BUTTONS_NUM + 16 || btn_idx == PHYS_BUTTONS_NUM + 17)) && gyroval > gyro_options[3] * 10) ||
			(((btn_idx == PHYS_BUTTONS_NUM + 18 || btn_idx == PHYS_BUTTONS_NUM + 19)) && gyroval > gyro_options[4] * 10) ||
			(((btn_idx == PHYS_BUTTONS_NUM + 20 || btn_idx == PHYS_BUTTONS_NUM + 21)) && gyroval > gyro_options[5] * 10))
			applyRemapRule(btn_idx, map, stickpos);
	}
}

void applyRemap(SceCtrlData *ctrl, int count) {
	
	// Checking for menu triggering
	if ((ctrl->buttons & btns[menuActivator_mask[0]]) && (ctrl->buttons & btns[menuActivator_mask[1]])) {
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
	
	// Gathering gyro data	
	SceMotionState motionstate;
    sceMotionGetState(&motionstate);
	
	// Applying remap rules for physical buttons
	int i;
	uint32_t new_map = 0;
	uint32_t stickpos[8] = { };
	for (i=0;i<PHYS_BUTTONS_NUM;i++) {
		if (ctrl->buttons & btns[i]) applyRemapRule(i, &new_map, stickpos);
	}
	
	// Applying remap rules for front virtual buttons
	for (i=0;i<front.reportNum;i++) {
		if (front.report[i].x > 960 && front.report[i].y > 544)       // Bot Right
			applyRemapRule(PHYS_BUTTONS_NUM + 3, &new_map, stickpos);
		else if (front.report[i].x <= 960 && front.report[i].y > 544) // Bot Left
			applyRemapRule(PHYS_BUTTONS_NUM + 2, &new_map, stickpos);
		else if (front.report[i].x > 960 && front.report[i].y <= 544) // Top Right
			applyRemapRule(PHYS_BUTTONS_NUM + 1, &new_map, stickpos);
		else if (front.report[i].x <= 960 && front.report[i].y <= 544)// Top Left
			applyRemapRule(PHYS_BUTTONS_NUM, &new_map, stickpos);
	}
	
	// Applying remap rules for rear virtual buttons
	for (i=0;i<rear.reportNum;i++) {
		if (rear.report[i].x > 960 && rear.report[i].y > 544)        // Bot Right
			applyRemapRule(PHYS_BUTTONS_NUM + 7, &new_map, stickpos);
		else if (rear.report[i].x <= 960 && rear.report[i].y > 544)  // Bot Left
			applyRemapRule(PHYS_BUTTONS_NUM + 6, &new_map, stickpos);
		else if (rear.report[i].x > 960 && rear.report[i].y <= 544)  // Top Right
			applyRemapRule(PHYS_BUTTONS_NUM + 5, &new_map, stickpos);
		else if (rear.report[i].x <= 960 && rear.report[i].y <= 544) // Top Left
			applyRemapRule(PHYS_BUTTONS_NUM + 4, &new_map, stickpos);
	}
	
	// Applying remap rules for left analog
	if (ctrl->lx < 127 - analogs_options[0])			// Left
		applyRemapRuleForAnalog(PHYS_BUTTONS_NUM + 8, &new_map, stickpos, ctrl->lx);
	else if (ctrl->lx > 127 + analogs_options[0])		// Right
		applyRemapRuleForAnalog(PHYS_BUTTONS_NUM + 9, &new_map, stickpos, 255 - ctrl->lx);
	if (ctrl->ly < 127 - analogs_options[1])			// Up
		applyRemapRuleForAnalog(PHYS_BUTTONS_NUM + 10, &new_map, stickpos, ctrl->ly);
	else if (ctrl->ly > 127 + analogs_options[1])		// Down
		applyRemapRuleForAnalog(PHYS_BUTTONS_NUM + 11, &new_map, stickpos, 255 - ctrl->ly);
	
	// Applying remap rules for right analog
	if (ctrl->rx < 127 - analogs_options[2])	 		// Left
		applyRemapRuleForAnalog(PHYS_BUTTONS_NUM + 12, &new_map, stickpos, ctrl->rx);
	else if (ctrl->rx > 127 + analogs_options[2])		// Right
		applyRemapRuleForAnalog(PHYS_BUTTONS_NUM + 13, &new_map, stickpos, 255 - ctrl->rx);
	if (ctrl->ry < 127 - analogs_options[3])			// Up
		applyRemapRuleForAnalog(PHYS_BUTTONS_NUM + 14, &new_map, stickpos, ctrl->ry);
	else if (ctrl->ry > 127 + analogs_options[3])		// Down
		applyRemapRuleForAnalog(PHYS_BUTTONS_NUM + 15, &new_map, stickpos, 255 - ctrl->ry);
	
	// Applying remap for gyro
	if (motionstate.angularVelocity.y > 0)
		applyRemapRuleForGyro(PHYS_BUTTONS_NUM + 16,  &new_map, stickpos, 
			motionstate.angularVelocity.y * gyro_options[0]);
	if (motionstate.angularVelocity.y < 0)
		applyRemapRuleForGyro(PHYS_BUTTONS_NUM + 17,  &new_map, stickpos, 
			-motionstate.angularVelocity.y * gyro_options[0]);
	if (motionstate.angularVelocity.x > 0)
		applyRemapRuleForGyro(PHYS_BUTTONS_NUM + 18,  &new_map, stickpos, 
			motionstate.angularVelocity.x * gyro_options[1]);
	if (motionstate.angularVelocity.x < 0)
		applyRemapRuleForGyro(PHYS_BUTTONS_NUM + 19,  &new_map, stickpos, 
			-motionstate.angularVelocity.x * gyro_options[1]);
	if (motionstate.angularVelocity.z > 0)
		applyRemapRuleForGyro(PHYS_BUTTONS_NUM + 20,  &new_map, stickpos, 
			motionstate.angularVelocity.z * gyro_options[2]);
	if (motionstate.angularVelocity.z < 0)
		applyRemapRuleForGyro(PHYS_BUTTONS_NUM + 21,  &new_map, stickpos, 
			-motionstate.angularVelocity.z * gyro_options[2]);
	
	// Nulling analogs if they're remapped
	for (i = 0; i < count; i++) {				
		if ((ctrl[i].lx < 127 && btn_mask[PHYS_BUTTONS_NUM+8] != PHYS_BUTTONS_NUM) ||
			(ctrl[i].lx > 127 && btn_mask[PHYS_BUTTONS_NUM+9] != PHYS_BUTTONS_NUM))
			ctrl[i].lx = 127;
		if ((ctrl[i].ly < 127 && btn_mask[PHYS_BUTTONS_NUM+10] != PHYS_BUTTONS_NUM) ||
			(ctrl[i].ly > 127 && btn_mask[PHYS_BUTTONS_NUM+11] != PHYS_BUTTONS_NUM))
			ctrl[i].ly = 127;
		if ((ctrl[i].rx < 127 && btn_mask[PHYS_BUTTONS_NUM+12] != PHYS_BUTTONS_NUM) ||
			(ctrl[i].rx > 127 && btn_mask[PHYS_BUTTONS_NUM+13] != PHYS_BUTTONS_NUM))
			ctrl[i].rx = 127;
		if ((ctrl[i].ry < 127 && btn_mask[PHYS_BUTTONS_NUM+14] != PHYS_BUTTONS_NUM) ||
			(ctrl[i].ry > 127 && btn_mask[PHYS_BUTTONS_NUM+15] != PHYS_BUTTONS_NUM))
			ctrl[i].ry = 127;	
	}
	
	// Remove minimal drift if digital remap for stick directions is used
	for (i = 0; i < count; i++)
	{
		if (((stickpos[0] || stickpos[1]) && abs(ctrl[i].lx - 127) < analogs_options[0]) || 
			((stickpos[2] || stickpos[3]) && abs(ctrl[i].ly - 127) < analogs_options[1]))
			ctrl[i].lx = ctrl[i].ly = 127; 
		if (((stickpos[4] || stickpos[5]) && abs(ctrl[i].rx - 127) < analogs_options[2]) || 
			((stickpos[6] || stickpos[7]) && abs(ctrl[i].ry - 127) < analogs_options[3])) 
			ctrl[i].rx = ctrl[i].ry = 127;
	}

	// Apply digital remap for stick directions if used
	for (i = 0; i < count; i++) {
		ctrl[i].buttons = new_map;
		if (stickpos[0] || stickpos[1])
			ctrl[i].lx = clamp(ctrl[i].lx - stickpos[0] + stickpos[1], 0, 255);
		if (stickpos[2] || stickpos[3])
			ctrl[i].ly = clamp(ctrl[i].ly - stickpos[2] + stickpos[3], 0, 255);
		if (stickpos[4] || stickpos[5])
			ctrl[i].rx = clamp(ctrl[i].rx - stickpos[4] + stickpos[5], 0, 255);
		if (stickpos[6] || stickpos[7])
			ctrl[i].ry = clamp(ctrl[i].ry - stickpos[6] + stickpos[7], 0, 255);
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
	sceIoWrite(fd, analogs_options, ANOLOGS_OPTIONS_NUM);
	sceIoClose(fd);
	
	// Opening gyro config file and saving the config
	fd = sceIoOpen("ux0:/data/remaPSV/gyro.bin", SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
	sceIoWrite(fd, gyro_options, GYRO_OPTIONS_NUM);
	sceIoClose(fd);
}

void loadConfig(void) {
	resetRemaps();
	resetAnalogs();
	resetGyro();
	
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
	
	// Loading menu activator config file
	fd = sceIoOpen("ux0:/data/remaPSV/menuactivator.bin", SCE_O_RDONLY, 0777);
	if(fd >= 0){
		uint8_t temp;
		sceIoRead(fd, &temp, 1);
		menuActivator_mask[0] = (temp >> 4) & 0x0F;
		menuActivator_mask[1] = temp & 0x0F;
		sceIoClose(fd);
	}
	
	// Loading analog config file
	fd = sceIoOpen("ux0:/data/remaPSV/analogs.bin", SCE_O_RDONLY, 0777);
	if (fd >= 0){
		sceIoRead(fd, analogs_options, ANOLOGS_OPTIONS_NUM);
		sceIoClose(fd);
	}
	
	// Loading gyro config file
	fd = sceIoOpen("ux0:/data/remaPSV/gyro.bin", SCE_O_RDONLY, 0777);
	if (fd >= 0){
		sceIoRead(fd, gyro_options, GYRO_OPTIONS_NUM);
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
			menu_entries = BUTTONS_NUM;
			break;
		case ANALOG_MENU:
			menu_entries = ANOLOGS_OPTIONS_NUM;
			break;
		case GYRO_MENU:
			menu_entries = GYRO_OPTIONS_NUM;
			break;
		case FUNCS_LIST:
			menu_entries = HOOKS_NUM - 1;
			break;
		default:
			break;
		}
		if ((ctrl->buttons & SCE_CTRL_DOWN) && (!(old_buttons & SCE_CTRL_DOWN))) {
			if (++cfg_i >= menu_entries) cfg_i = 0;
		}else if ((ctrl->buttons & SCE_CTRL_UP) && (!(old_buttons & SCE_CTRL_UP))) {
			if (--cfg_i < 0) cfg_i = menu_entries-1;
		}else if ((ctrl->buttons & SCE_CTRL_RIGHT) && (!(old_buttons & SCE_CTRL_RIGHT))) {
			if (menu_i == REMAP_MENU) btn_mask[cfg_i] = (btn_mask[cfg_i] + 1) % TARGET_REMAPS;
			else if (menu_i == ANALOG_MENU) analogs_options[cfg_i] = (analogs_options[cfg_i] + 1) % 128;
			else if (menu_i == GYRO_MENU) {if (gyro_options[cfg_i] < 200) gyro_options[cfg_i]++; else gyro_options[cfg_i] = 0;}
		}else if ((ctrl->buttons & SCE_CTRL_LEFT) && (!(old_buttons & SCE_CTRL_LEFT))) {
			if (menu_i == REMAP_MENU) {
				if (btn_mask[cfg_i] == 0) btn_mask[cfg_i] = TARGET_REMAPS - 1;
				else btn_mask[cfg_i]--;
			} else if (menu_i == ANALOG_MENU) {
				if (cfg_i < 4){
					if (analogs_options[cfg_i] == 0) analogs_options[cfg_i] = 127;
					else analogs_options[cfg_i]--;
				} else analogs_options[cfg_i] = !analogs_options[cfg_i];
			} else if (menu_i == GYRO_MENU) {
				if (gyro_options[cfg_i] > 0) gyro_options[cfg_i]--; else gyro_options[cfg_i] = 200;
			}
		}else if ((ctrl->buttons & SCE_CTRL_SQUARE) && (!(old_buttons & SCE_CTRL_SQUARE))) {
			if (menu_i == REMAP_MENU) btn_mask[cfg_i] = PHYS_BUTTONS_NUM;
			else if (menu_i == ANALOG_MENU) analogs_options[cfg_i] = ANALOGS_DEADZONE_DEF;
			else if (menu_i == GYRO_MENU) gyro_options[cfg_i] = GYRO_SENS_DEF;
		}else if ((ctrl->buttons & SCE_CTRL_START) && (!(old_buttons & SCE_CTRL_START))) {
			if (menu_i == REMAP_MENU) resetRemaps();
			else if (menu_i == ANALOG_MENU) resetAnalogs();
			else if (menu_i == GYRO_MENU) resetGyro();
		}else if ((ctrl->buttons & SCE_CTRL_CROSS) && (!(old_buttons & SCE_CTRL_CROSS))) {
			if (menu_i == MAIN_MENU) {
				if (cfg_i == menu_entries-1) {
					show_menu = 0;
					saveConfig();
				} else {					
					menu_i = cfg_i + 1;
					cfg_i = 0;
				}
			}
		}else if (((ctrl->buttons & SCE_CTRL_SELECT) && (!(old_buttons & SCE_CTRL_SELECT)))
			||((ctrl->buttons & SCE_CTRL_CIRCLE) && (!(old_buttons & SCE_CTRL_CIRCLE)))) {
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
	
	// For some reason, Adrenaline refuses to start 
	// if this plugin is active; so stop the
	// initialization of the module.
	
	//Bypass for Adrenaline (NPXS10028) and ABM Bubbles (PSPEMUXXX)
	if(!strcmp(titleid, "NPXS10028") && !strstr(titleid, "PSPEMU"))
	{
	   return SCE_KERNEL_START_SUCCESS;
	}
	
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
	
	// Enabling gyro sampling
	sceMotionReset();
	sceMotionStartSampling();
	
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
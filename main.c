#include <vitasdk.h>
#include <taihen.h>
#include <psp2/motion.h> 
#include <libk/string.h>
#include <libk/stdio.h>
#include <stdlib.h>
#include "renderer.h"

#define HOOKS_NUM         17 // Hooked functions num
#define PHYS_BUTTONS_NUM  16 // Supported physical buttons num
#define TARGET_REMAPS     42 // Supported target remaps num
#define BUTTONS_NUM       38 // Supported buttons num
#define MENU_MODES        7  // Menu modes num
#define LONG_PRESS_TIME   400000
#define COLOR_DEFAULT     0x00FFFFFF
#define COLOR_HEADER      0x00FF00FF
#define COLOR_CURSOR      0x0000FF00
#define COLOR_ACTIVE      0x00FF0000
#define COLOR_DISABLE     0x000000FF
#define BUFFERS_NUM      			64
#define ANALOGS_DEADZONE_DEF		30
#define ANALOGS_FORCE_DIGITAL_DEF	0
#define ANOLOGS_OPTIONS_NUM			8
#define GYRO_SENS_DEF				127
#define GYRO_DEADZONE_DEF			15
#define GYRO_OPTIONS_NUM			6
#define MULTITOUCH_FRONT_NUM		6
#define MULTITOUCH_REAR_NUM			4
#define TOUCH_OPTIONS_NUM			18
#define TOUCH_MODE_DEF				1
#define CONN_CTRLS_NUM				5

#ifndef max
# define max(a,b) (((a)>(b))?(a):(b))
# define min(a,b) (((a)<(b))?(a):(b))
#endif

//Front resolution : 960x544
//Rear resolution: 960x445
static const uint16_t TOUCH_POINTS_DEF[8] = {
	300, 136,	//TL
	640, 136,	//TR
	300, 408,	//BL
	640, 408	//BR
};

enum{
	MAIN_MENU = 0,
	REMAP_MENU,
	ANALOG_MENU,
	TOUCH_MENU,
	GYRO_MENU,
	FUNCS_LIST,
	CONN_CTRLS
};

enum{
	POSITIVE = 0,
	NEGATIVE
};

static uint8_t btn_mask[BUTTONS_NUM];
static SceCtrlData remappedBuffers[HOOKS_NUM-5][BUFFERS_NUM];
static int remappedBuffersSizes[HOOKS_NUM];
static int remappedBuffersIdxs[HOOKS_NUM];
static SceCtrlData pstv_fakepad;
static uint8_t touchPointsFrontNum;
static uint8_t touchPointsRearNum;
static uint16_t touchPointsFront[MULTITOUCH_FRONT_NUM * 2];
static uint16_t touchPointsRear[MULTITOUCH_REAR_NUM * 2]; 

static uint8_t new_frame = 1;
static int screen_h = 272;
static int screen_w = 480;
static uint8_t show_menu = 0;
static int cfg_i = 0;
static int menu_i = MAIN_MENU;
static uint32_t curr_buttons;
static uint32_t old_buttons;
static uint64_t tick;
static uint64_t pressedTicks[PHYS_BUTTONS_NUM];

static uint8_t current_hook = 0;
static SceUID hooks[HOOKS_NUM];
static tai_hook_ref_t refs[HOOKS_NUM];
static int model;
static char titleid[16];
static char fname[128];
static uint8_t internal_touch_call = 0;
static uint8_t internal_ext_call = 0;

static uint8_t analogs_options[ANOLOGS_OPTIONS_NUM];
static uint8_t gyro_options[GYRO_OPTIONS_NUM];
static uint16_t touch_options[TOUCH_OPTIONS_NUM];
static uint8_t used_funcs[HOOKS_NUM-1];

static char* str_menus[MENU_MODES] = {
	"MAIN MENU", "REMAP MENU", "ANALOG MENU", "TOUCH MENU", "GYRO_MENU", "USED FUNCTIONS", "CONNECTED CONTROLLERS"
};

static char* str_main_menu[] = {
	"Change remap settings",
	"Change analog remap settings",
	"Change touch remap settings",
	"Change gyro remap settings",
	"Show imported functions",
	"Show connected gamepads",
	"Return to the game"
};

static char* str_touch_mode[3] = {
	"Always on", "Disable if remapped", "Disable"
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
	"Right Analog (L)", "Right Analog (R)", "Right Analog (U)", "Right Analog (D)",
	"Touch (TL)", "Touch (TR)", "Touch (BL)", "Touch (BR)",
	"Touch (Point [A])", "Touch (Point [B])", "Touch (Point [C])", "Touch (Point [D])",
	"Rear (TL)", "Rear (TR)", "Rear (BL)", "Rear (BR)",
	"Rear (Point [A])", "Rear (Point [B])", "Rear (Point [C])", "Rear (Point [D])",
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

void resetTouch(){
	for (int i = 0; i < TOUCH_OPTIONS_NUM - 2; i++)
		touch_options[i] = TOUCH_POINTS_DEF[i % 8];
	touch_options[16] = TOUCH_MODE_DEF;
	touch_options[17] = TOUCH_MODE_DEF;
}

void resetGyro(){
	for (int i = 0; i < GYRO_OPTIONS_NUM; i++)
		gyro_options[i] = i < 3 ? GYRO_SENS_DEF : GYRO_DEADZONE_DEF;
}

char* getControllerName(int id){
	if 		(id == 	SCE_CTRL_TYPE_UNPAIRED) return "Unpaired controller";
	else if (id == 	SCE_CTRL_TYPE_PHY) 		return "Physical controller for VITA";
	else if (id == 	SCE_CTRL_TYPE_VIRT) 	return "Virtual controller for PSTV";
	else if (id == 	SCE_CTRL_TYPE_DS3) 		return "DualShock 3";
	else if (id == 	SCE_CTRL_TYPE_DS4) 		return "DualShock 4";
	else 									return "Unknown controller";
}

int calcStartingIndex(int idx, int entriesNum, int screenEntries){
	int bottom_l = 3;
	if (idx < screenEntries + 1 - bottom_l) return 0;
	else if (idx < entriesNum - 1 - bottom_l) return idx - (screenEntries + 1 - bottom_l);
	else return entriesNum - screenEntries + 1 - bottom_l;
}
// Config Menu Renderer
void drawConfigMenu() {
	setTextColor(COLOR_HEADER);
	drawString(5, 10, "Thanks to Tain Sueiras, nobodywasishere and RaveHeart");
	drawString(5, 30, "for their awesome support on Patreon");
	drawStringF(5, 50, "remaPSV v.1.2 - %s", str_menus[menu_i]);
		
	int i, y = 50;
	int screen_entries = (screen_h - 50) / 20;
	int avaliable_entries = screen_entries - 3 - 1;
	switch (menu_i){
	case MAIN_MENU:
		for (i = calcStartingIndex(cfg_i, sizeof(str_main_menu)/sizeof(char*), avaliable_entries); i < sizeof(str_main_menu)/sizeof(char*); i++) {
			setTextColor((i == cfg_i) ? COLOR_CURSOR : COLOR_DEFAULT);
			drawStringF(5, y += 20, "%s", str_main_menu[i]);
			if (y + 40 > screen_h) break;
		}
		setTextColor(COLOR_HEADER);
		drawString(5, 520, "(X):select                                     (select) or (O) : save and close");
		break;
	case REMAP_MENU:
		for (i = calcStartingIndex(cfg_i, BUTTONS_NUM, avaliable_entries); i < BUTTONS_NUM; i++) {
			if (i == cfg_i) setTextColor(COLOR_CURSOR);
			else if (btn_mask[i] == PHYS_BUTTONS_NUM) setTextColor(COLOR_DEFAULT);
			else if (btn_mask[i] == PHYS_BUTTONS_NUM + 1) setTextColor(COLOR_DISABLE);
			else setTextColor(COLOR_ACTIVE);
			drawStringF(5, y += 20, "%s -> %s", str_btns[i], target_btns[btn_mask[i]]);
			if (y + 60 > screen_h) break;
		}
		setTextColor(COLOR_HEADER);
		drawString(5, 520, "(<)(>):change  (LT)(RT):section  ([]):reset  (start):reset all         (O):back");
		break;
	case ANALOG_MENU:
		for (i = calcStartingIndex(cfg_i, ANOLOGS_OPTIONS_NUM + 2, avaliable_entries); i < ANOLOGS_OPTIONS_NUM + 2; i++) {				
			if (y + 60 > screen_h) break;
			
			if (!(i % 5)){	//Headers
				setTextColor((i == cfg_i) ? COLOR_CURSOR : COLOR_DEFAULT);
				drawString(5, y+=20, (i == 0) ? "Deadzone:" : "Force digital output:");
				continue;
			}
			
			int o_idx = (i < 5) ? i - 1 : i - 2; //Index in options file
			if (i == cfg_i) setTextColor(COLOR_CURSOR);
			else if (analogs_options[o_idx] != ((o_idx < 4) ? ANALOGS_DEADZONE_DEF : ANALOGS_FORCE_DIGITAL_DEF)) 
				setTextColor(COLOR_ACTIVE);
			else setTextColor(COLOR_DEFAULT);
			if (o_idx < 4)
				drawStringF(15, y+=20, "%s [%s axis]: %hhu", 
					((o_idx / 2) % 2 ) ? "Right Analog" : "Left Analog ",
					(o_idx % 2) ? "Y" : "X",
					analogs_options[o_idx]);
			else 
				drawStringF(15, y+=20, "%s [%s axis]: %s", 
					((o_idx / 2) % 2 ) ? "Right Analog" : "Left Analog ",
					(o_idx % 2) ? "Y" : "X",
					analogs_options[o_idx] ? "Yes" : "No");	
		}/*
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
		drawString(5, 520, "(<)(>):change  ([]):reset  (start):reset all                           (O):back");*/
		break;
	case TOUCH_MENU:
		setTextColor(COLOR_DEFAULT);
		drawString(5, y+=20, "Front touch:");
		setTextColor((0 == cfg_i) ? COLOR_CURSOR : ((touch_options[0] != TOUCH_POINTS_DEF[0]) ? COLOR_ACTIVE : COLOR_DEFAULT));
		drawStringF(15, y+=20, "Point [A] x: %hu", touch_options[0]);
		setTextColor((1 == cfg_i) ? COLOR_CURSOR : ((touch_options[1] != TOUCH_POINTS_DEF[1]) ? COLOR_ACTIVE : COLOR_DEFAULT));
		drawStringF(15, y+=20, "Point [A] y: %hu", touch_options[1]);
		setTextColor((2 == cfg_i) ? COLOR_CURSOR : ((touch_options[2] != TOUCH_POINTS_DEF[2]) ? COLOR_ACTIVE : COLOR_DEFAULT));
		drawStringF(15, y+=20, "Point [B] x: %hu", touch_options[2]);
		setTextColor((3 == cfg_i) ? COLOR_CURSOR : ((touch_options[3] != TOUCH_POINTS_DEF[3]) ? COLOR_ACTIVE : COLOR_DEFAULT));
		drawStringF(15, y+=20, "Point [B] y: %hu", touch_options[3]);
		setTextColor((4 == cfg_i) ? COLOR_CURSOR : ((touch_options[4] != TOUCH_POINTS_DEF[4]) ? COLOR_ACTIVE : COLOR_DEFAULT));
		drawStringF(15, y+=20, "Point [C] x: %hu", touch_options[4]);
		setTextColor((5 == cfg_i) ? COLOR_CURSOR : ((touch_options[5] != TOUCH_POINTS_DEF[5]) ? COLOR_ACTIVE : COLOR_DEFAULT));
		drawStringF(15, y+=20, "Point [C] y: %hu", touch_options[5]);
		setTextColor((6 == cfg_i) ? COLOR_CURSOR : ((touch_options[6] != TOUCH_POINTS_DEF[6]) ? COLOR_ACTIVE : COLOR_DEFAULT));
		drawStringF(15, y+=20, "Point [D] x: %hu", touch_options[6]);
		setTextColor((7 == cfg_i) ? COLOR_CURSOR : ((touch_options[7] != TOUCH_POINTS_DEF[7]) ? COLOR_ACTIVE : COLOR_DEFAULT));
		drawStringF(15, y+=20, "Point [D] y: %hu", touch_options[7]);
		setTextColor(COLOR_DEFAULT);
		drawString(5, y+=20, "Rear touch:");
		setTextColor((8  == cfg_i) ? COLOR_CURSOR : ((touch_options[8] != TOUCH_POINTS_DEF[0]) ? COLOR_ACTIVE : COLOR_DEFAULT));
		drawStringF(15, y+=20, "Point [A] x: %hu", touch_options[8]);
		setTextColor((9  == cfg_i) ? COLOR_CURSOR : ((touch_options[9] != TOUCH_POINTS_DEF[1]) ? COLOR_ACTIVE : COLOR_DEFAULT));
		drawStringF(15, y+=20, "Point [A] y: %hu", touch_options[9]);
		setTextColor((10 == cfg_i) ? COLOR_CURSOR : ((touch_options[10] != TOUCH_POINTS_DEF[2]) ? COLOR_ACTIVE : COLOR_DEFAULT));
		drawStringF(15, y+=20, "Point [B] x: %hu", touch_options[10]);
		setTextColor((11 == cfg_i) ? COLOR_CURSOR : ((touch_options[11] != TOUCH_POINTS_DEF[3]) ? COLOR_ACTIVE : COLOR_DEFAULT));
		drawStringF(15, y+=20, "Point [B] y: %hu", touch_options[11]);
		setTextColor((12 == cfg_i) ? COLOR_CURSOR : ((touch_options[12] != TOUCH_POINTS_DEF[4]) ? COLOR_ACTIVE : COLOR_DEFAULT));
		drawStringF(15, y+=20, "Point [C] x: %hu", touch_options[12]);
		setTextColor((13 == cfg_i) ? COLOR_CURSOR : ((touch_options[13] != TOUCH_POINTS_DEF[5]) ? COLOR_ACTIVE : COLOR_DEFAULT));
		drawStringF(15, y+=20, "Point [C] y: %hu", touch_options[13]);
		setTextColor((14 == cfg_i) ? COLOR_CURSOR : ((touch_options[14] != TOUCH_POINTS_DEF[6]) ? COLOR_ACTIVE : COLOR_DEFAULT));
		drawStringF(15, y+=20, "Point [D] x: %hu", touch_options[14]);
		setTextColor((15 == cfg_i) ? COLOR_CURSOR : ((touch_options[15] != TOUCH_POINTS_DEF[7]) ? COLOR_ACTIVE : COLOR_DEFAULT));
		drawStringF(15, y+=20, "Point [D] y: %hu", touch_options[15]);
		if (16 == cfg_i)
				setTextColor(COLOR_CURSOR);
		else if (touch_options[16] == TOUCH_MODE_DEF)
			setTextColor(COLOR_DEFAULT);
		else if (touch_options[16] == 2)
			setTextColor(COLOR_DISABLE);
		else
			setTextColor(COLOR_ACTIVE);
		drawStringF(5, y+=20, "Front Touch Mode : %s", str_touch_mode[touch_options[16]]);
		if (17 == cfg_i)
				setTextColor(COLOR_CURSOR);
		else if (touch_options[17] == TOUCH_MODE_DEF)
			setTextColor(COLOR_DEFAULT);
		else if (touch_options[17] == 2)
			setTextColor(COLOR_DISABLE);
		else
			setTextColor(COLOR_ACTIVE);
		drawStringF(5, y+=20, "Rear Touch Mode : %s", str_touch_mode[touch_options[17]]);
		setTextColor(COLOR_HEADER);
		drawString(5, 520, "(<)(>):change  ([]):reset  (start):reset all                           (O):back");
		break;
	case GYRO_MENU:
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
		drawString(5, 520, "(<)(>):change  ([]):reset  (start):reset all                          (O): back");
		break;
	case FUNCS_LIST:
		for (i = calcStartingIndex(cfg_i, HOOKS_NUM - 1, avaliable_entries); i < HOOKS_NUM - 1; i++) {
			setTextColor((i == cfg_i) ? COLOR_CURSOR : (used_funcs[i] ? COLOR_ACTIVE : COLOR_DEFAULT));
			drawStringF(5, y += 20, "%s : %s", str_funcs[i], used_funcs[i] ? "Yes" : "No");
			if (y + 40 > screen_h) break;
		}
		setTextColor(COLOR_HEADER);
		drawString(5, 520, "                                                                       (O):back");
		break;
	case CONN_CTRLS:;
		SceCtrlPortInfo pi;
		int res = sceCtrlGetControllerPortInfo(&pi);
		if (res != 0)
			drawString(5, y+= 20, "Error getting port info");
		else
			for (int i = max(0, cfg_i - (avaliable_entries + 1)); i < 5; i++){
				setTextColor((5 == cfg_i) ? COLOR_CURSOR : ((pi.port[i] != SCE_CTRL_TYPE_UNPAIRED) ? COLOR_ACTIVE : COLOR_DEFAULT));
				drawStringF(5, y += 20, "Port %i: %s", i, getControllerName(pi.port[i]));
				if (y + 40 > screen_h) break;
			}		
		drawString(5, 520, "                                                                       (O):back");
	default:
		break;
	}
}

void applyRemapRule(uint8_t btn_idx, uint32_t* map, uint32_t* stickpos) {
	if (btn_mask[btn_idx] < PHYS_BUTTONS_NUM) { // -> Btn
		if (!(*map & btns[btn_mask[btn_idx]])) {
			*map += btns[btn_mask[btn_idx]];
		}

	} else if (btn_mask[btn_idx] == PHYS_BUTTONS_NUM) { // -> Original
		if (btn_idx < PHYS_BUTTONS_NUM) {
			if (!(*map & btns[btn_idx])) {
				*map += btns[btn_idx];
			}
		}										//  -> Analog
	} else if (PHYS_BUTTONS_NUM + 1 < btn_mask[btn_idx] && btn_mask[btn_idx] < PHYS_BUTTONS_NUM + 10) { 
		stickpos[btn_mask[btn_idx] - (PHYS_BUTTONS_NUM + 2)] += 127;
	} else if (btn_mask[btn_idx] < PHYS_BUTTONS_NUM + 24){	// -> Touch
		if (btn_mask[btn_idx] < PHYS_BUTTONS_NUM + 14){		//Front touch default
			if (touchPointsFrontNum == MULTITOUCH_FRONT_NUM) return;
			touchPointsFront[touchPointsFrontNum*2] = TOUCH_POINTS_DEF[(btn_mask[btn_idx] - (PHYS_BUTTONS_NUM + 10)) * 2] * 2;
			touchPointsFront[touchPointsFrontNum*2+1] = TOUCH_POINTS_DEF[(btn_mask[btn_idx] - (PHYS_BUTTONS_NUM + 10)) * 2 + 1] * 2;
			touchPointsFrontNum++;
		} else if (btn_mask[btn_idx] < PHYS_BUTTONS_NUM + 18){	//Front touch custom
			if (touchPointsFrontNum == MULTITOUCH_FRONT_NUM) return;
			touchPointsFront[touchPointsFrontNum*2] = touch_options[(btn_mask[btn_idx] - (PHYS_BUTTONS_NUM + 14)) * 2] * 2;
			touchPointsFront[touchPointsFrontNum*2+1] = touch_options[(btn_mask[btn_idx] - (PHYS_BUTTONS_NUM + 14)) * 2 + 1] * 2;
			touchPointsFrontNum++;
		} else if (btn_mask[btn_idx] < PHYS_BUTTONS_NUM + 22){	//Rear  touch default
			if (touchPointsRearNum == MULTITOUCH_REAR_NUM) return;
			touchPointsRear[touchPointsRearNum*2] = TOUCH_POINTS_DEF[(btn_mask[btn_idx] - (PHYS_BUTTONS_NUM + 18)) * 2] * 2;
			touchPointsRear[touchPointsRearNum*2+1] = TOUCH_POINTS_DEF[(btn_mask[btn_idx] - (PHYS_BUTTONS_NUM + 18)) * 2 + 1] * 2;
			touchPointsRearNum++;
		} else if (btn_mask[btn_idx] < PHYS_BUTTONS_NUM + 26){	//Rear touch custom
			if (touchPointsRearNum == MULTITOUCH_REAR_NUM) return;
			touchPointsRear[touchPointsRearNum*2] = touch_options[8 + (btn_mask[btn_idx] - (PHYS_BUTTONS_NUM + 22)) * 2] * 2;
			touchPointsRear[touchPointsRearNum*2+1] = touch_options[8 + (btn_mask[btn_idx] - (PHYS_BUTTONS_NUM + 22)) * 2 + 1] * 2;
			touchPointsRearNum++;
		}
	}
}

void applyRemapRuleForAnalog(uint8_t btn_idx, uint32_t* map, uint32_t* stickpos, uint8_t stickposval) {
	if (PHYS_BUTTONS_NUM + 1 < btn_mask[btn_idx] && btn_mask[btn_idx] < PHYS_BUTTONS_NUM + 10
		&& !analogs_options[4 + (int) ((btn_idx - PHYS_BUTTONS_NUM - 8) / 2)]) {
		// Analog -> Analog [ANALOG MODE]
		stickpos[btn_mask[btn_idx] - (PHYS_BUTTONS_NUM + 2)] += 127 - stickposval;
	} else {
		// Analog -> Analog [DIGITAL MODE] and Analog -> Button
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

void applyRemap(SceCtrlData *ctrl) {
	// Gathering real touch data
	touchPointsFrontNum = 0;
	touchPointsRearNum = 0;
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
	if ((ctrl->lx < 127 && btn_mask[PHYS_BUTTONS_NUM+8] != PHYS_BUTTONS_NUM) ||
		(ctrl->lx > 127 && btn_mask[PHYS_BUTTONS_NUM+9] != PHYS_BUTTONS_NUM))
		ctrl->lx = 127;
	if ((ctrl->ly < 127 && btn_mask[PHYS_BUTTONS_NUM+10] != PHYS_BUTTONS_NUM) ||
		(ctrl->ly > 127 && btn_mask[PHYS_BUTTONS_NUM+11] != PHYS_BUTTONS_NUM))
		ctrl->ly = 127;
	if ((ctrl->rx < 127 && btn_mask[PHYS_BUTTONS_NUM+12] != PHYS_BUTTONS_NUM) ||
		(ctrl->rx > 127 && btn_mask[PHYS_BUTTONS_NUM+13] != PHYS_BUTTONS_NUM))
		ctrl->rx = 127;
	if ((ctrl->ry < 127 && btn_mask[PHYS_BUTTONS_NUM+14] != PHYS_BUTTONS_NUM) ||
		(ctrl->ry > 127 && btn_mask[PHYS_BUTTONS_NUM+15] != PHYS_BUTTONS_NUM))
		ctrl->ry = 127;	
	
	
	// Remove minimal drift if digital remap for stick directions is used
	if (stickpos[0] || stickpos[1] || stickpos[2] || stickpos[3]){
		if (abs(ctrl->lx - 127) < analogs_options[0]) 
			ctrl->lx = 127;
		if (abs(ctrl->ly - 127) < analogs_options[1]) 
			ctrl->ly = 127;
	}
	if (stickpos[4] || stickpos[5] || stickpos[6] || stickpos[7]){
		if (abs(ctrl->rx - 127) < analogs_options[2]) 
			ctrl->rx = 127;
		if (abs(ctrl->ry - 127) < analogs_options[3]) 
			ctrl->ry = 127;
	}
	
	//Storing remap for analog axises
	if (stickpos[0] || stickpos[1])
		ctrl->lx = clamp(ctrl->lx - stickpos[0] + stickpos[1], 0, 255);
	if (stickpos[2] || stickpos[3])
		ctrl->ly = clamp(ctrl->ly - stickpos[2] + stickpos[3], 0, 255);
	if (stickpos[4] || stickpos[5])
		ctrl->rx = clamp(ctrl->rx - stickpos[4] + stickpos[5], 0, 255);
	if (stickpos[6] || stickpos[7])
		ctrl->ry = clamp(ctrl->ry - stickpos[6] + stickpos[7], 0, 255);
	
	//Storing remap for HW buttons
	ctrl->buttons = new_map;
}

void addVirtualTouches(SceTouchData *pData, uint16_t* touchPoints, 
		uint8_t touchPointsNum, uint8_t touchPointsMaxNum){
	int touchIdx = 0;
	while (touchIdx < touchPointsNum && pData->reportNum < touchPointsMaxNum){
		pData->report[pData->reportNum].x = touchPoints[touchIdx*2];
		pData->report[pData->reportNum].y = touchPoints[touchIdx*2 + 1];
		pData->reportNum ++;
		touchIdx ++;
	}
}

void updateTouchInfo(SceUInt32 port, SceTouchData *pData){	
	if (port == SCE_TOUCH_PORT_FRONT) {
		if (touch_options[16] == 2 || 
			(touch_options[16] == 1 &&
				((btn_mask[PHYS_BUTTONS_NUM] != PHYS_BUTTONS_NUM) ||
				(btn_mask[PHYS_BUTTONS_NUM+1] != PHYS_BUTTONS_NUM) ||
				(btn_mask[PHYS_BUTTONS_NUM+2] != PHYS_BUTTONS_NUM) ||
				(btn_mask[PHYS_BUTTONS_NUM+3] != PHYS_BUTTONS_NUM))))
			pData->reportNum = 0; //Disable pad
		addVirtualTouches(pData, touchPointsFront, touchPointsFrontNum, MULTITOUCH_FRONT_NUM);
	} else {
		if (touch_options[17] == 2 || 
			(touch_options[17] == 1 &&
				((btn_mask[PHYS_BUTTONS_NUM+4] != PHYS_BUTTONS_NUM) ||
				(btn_mask[PHYS_BUTTONS_NUM+5] != PHYS_BUTTONS_NUM) ||
				(btn_mask[PHYS_BUTTONS_NUM+6] != PHYS_BUTTONS_NUM) ||
				(btn_mask[PHYS_BUTTONS_NUM+7] != PHYS_BUTTONS_NUM))))
			pData->reportNum = 0; //Disable pad
		addVirtualTouches(pData, touchPointsRear, touchPointsRearNum, MULTITOUCH_REAR_NUM);
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
	
	// Opening touch config file and saving the config
	fd = sceIoOpen("ux0:/data/remaPSV/touch.bin", SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
	sceIoWrite(fd, touch_options, TOUCH_OPTIONS_NUM);
	sceIoClose(fd);
	
	// Opening gyro config file and saving the config
	fd = sceIoOpen("ux0:/data/remaPSV/gyro.bin", SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
	sceIoWrite(fd, gyro_options, GYRO_OPTIONS_NUM);
	sceIoClose(fd);
}

void loadConfig(void) {
	resetRemaps();
	resetAnalogs();
	resetTouch();
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
	
	// Loading touch config file
	fd = sceIoOpen("ux0:/data/remaPSV/touch.bin", SCE_O_RDONLY, 0777);
	if (fd >= 0){
		sceIoRead(fd, touch_options, TOUCH_OPTIONS_NUM);
		sceIoClose(fd);
	}
	
	// Loading gyro config file
	fd = sceIoOpen("ux0:/data/remaPSV/gyro.bin", SCE_O_RDONLY, 0777);
	if (fd >= 0){
		sceIoRead(fd, gyro_options, GYRO_OPTIONS_NUM);
		sceIoClose(fd);
	}
}

uint8_t isBtnActive(uint8_t btnNum){
	return ((curr_buttons & btns[btnNum]) && !(old_buttons & btns[btnNum])) 
		|| (pressedTicks[btnNum] != 0 && tick - pressedTicks[btnNum] > LONG_PRESS_TIME);
}

// Input Handler for the Config Menu
void configInputHandler(SceCtrlData *ctrl) {
	if (new_frame) {
		new_frame = 0;
		int menu_entries = 0;
		switch (menu_i) {
		case MAIN_MENU:
			menu_entries = sizeof(str_main_menu) / sizeof(char*);
			break;
		case REMAP_MENU:
			menu_entries = BUTTONS_NUM;
			break;
		case ANALOG_MENU:
			menu_entries = ANOLOGS_OPTIONS_NUM + 2;
			break;
		case TOUCH_MENU:
			menu_entries = TOUCH_OPTIONS_NUM;
			break;
		case GYRO_MENU:
			menu_entries = GYRO_OPTIONS_NUM;
			break;
		case FUNCS_LIST:
			menu_entries = HOOKS_NUM - 1;
			break;
		case CONN_CTRLS:
			menu_entries = CONN_CTRLS_NUM;
			break;
		default:
			break;
		}
		tick = ctrl->timeStamp;
		curr_buttons = ctrl->buttons;
		for (int i = 0; i < PHYS_BUTTONS_NUM; i++){
			if ((curr_buttons & btns[i]) && !(old_buttons & btns[i]))
				pressedTicks[i] = tick;
			else if (!(curr_buttons & btns[i]) && (old_buttons & btns[i]))
				pressedTicks[i] = 0;
			
			if (!isBtnActive(i))
				continue;
			
			switch (btns[i]) {
			case SCE_CTRL_DOWN:
				cfg_i = (cfg_i + 1) % menu_entries;
				break;
			case SCE_CTRL_UP:
				if (--cfg_i < 0) cfg_i = menu_entries  -1;
				break;
			case SCE_CTRL_RIGHT:
				switch (menu_i){
				case REMAP_MENU: 
					btn_mask[cfg_i] = (btn_mask[cfg_i] + 1) % TARGET_REMAPS;
					break;
				case ANALOG_MENU:
					if (!(cfg_i % 5)) break;//Skip headers
					int o_idx = (cfg_i < 5) ? cfg_i - 1 : cfg_i - 2;
					if (o_idx < 4) analogs_options[o_idx] = (analogs_options[o_idx] + 1) % 128;
					else analogs_options[o_idx] = !analogs_options[o_idx];
					break;
				case TOUCH_MENU:
					if (cfg_i < 16)
						touch_options[cfg_i] = (touch_options[cfg_i] + 1) % ((cfg_i % 2) ? 544 : 960);
					else
						touch_options[cfg_i] = (touch_options[cfg_i] + 1) % 3;
					break;
				case GYRO_MENU:
					gyro_options[cfg_i] = (gyro_options[cfg_i] + 1) % 200;
					break;
				}
				break;
			case SCE_CTRL_LEFT:
				switch (menu_i){
				case REMAP_MENU:
					if (btn_mask[cfg_i]) 	
						btn_mask[cfg_i]--;
					else
						btn_mask[cfg_i] = TARGET_REMAPS - 1;
					break;
				case ANALOG_MENU:
					if (!(cfg_i % 5)) break;//Skip headers
					int o_idx = (cfg_i < 5) ? cfg_i - 1 : cfg_i - 2;
					if (analogs_options[o_idx]) 	
						analogs_options[o_idx]--;
					else
						analogs_options[o_idx] = o_idx < 4 ? 127 : 1;
					break;
				case TOUCH_MENU:
					if (touch_options[cfg_i]) 	
						touch_options[cfg_i]--;
					else {
						if (cfg_i < 16)
							touch_options[cfg_i] = ((cfg_i % 2) ? 543 : 959);
						else 
							touch_options[cfg_i] = 2;
					}
					break;
				case GYRO_MENU:
					if (gyro_options[cfg_i]) 	
						gyro_options[cfg_i]--;
					else
						gyro_options[cfg_i] = 199;
					break;
				}
				break;
			case SCE_CTRL_LTRIGGER:
			case SCE_CTRL_L1:
				if (menu_i == REMAP_MENU){
					if (btn_mask[cfg_i] < 16)
						btn_mask[cfg_i] = 38;	//Rear touch custom
					else if (btn_mask[cfg_i] < 17)
						btn_mask[cfg_i] = 0;	//HW Buttons
					else if (btn_mask[cfg_i] < 18)
						btn_mask[cfg_i] = 16;	//Original
					else if (btn_mask[cfg_i] < 22)
						btn_mask[cfg_i] = 17;	//Disabled
					else if (btn_mask[cfg_i] < 26)
						btn_mask[cfg_i] = 18;	//Left stick
					else if (btn_mask[cfg_i] < 30)
						btn_mask[cfg_i] = 22;	//Right stick
					else if (btn_mask[cfg_i] < 34)
						btn_mask[cfg_i] = 26;	//Front touch default
					else if (btn_mask[cfg_i] < 38)
						btn_mask[cfg_i] = 30;	//Front touch custom
					else 
						btn_mask[cfg_i] = 34;	//Rear touch default
				}
				break;
			case SCE_CTRL_RTRIGGER:
			case SCE_CTRL_R1:
				if (menu_i == REMAP_MENU){
					if (btn_mask[cfg_i] < 16)
						btn_mask[cfg_i] = 16;	//Original
					else if (btn_mask[cfg_i] < 17)
						btn_mask[cfg_i] = 17;	//Disabled
					else if (btn_mask[cfg_i] < 18)
						btn_mask[cfg_i] = 18;	//Left stick
					else if (btn_mask[cfg_i] < 22)
						btn_mask[cfg_i] = 22;	//Right stick
					else if (btn_mask[cfg_i] < 26)
						btn_mask[cfg_i] = 26;	//Front touch default
					else if (btn_mask[cfg_i] < 30)
						btn_mask[cfg_i] = 30;	//Front touch custom
					else if (btn_mask[cfg_i] < 34)
						btn_mask[cfg_i] = 34;	//Rear touch default
					else if (btn_mask[cfg_i] < 38)
						btn_mask[cfg_i] = 38;	//Rear touch custom
					else 
						btn_mask[cfg_i] = 0;	//HW Buttons
				}
				break;
			case SCE_CTRL_SQUARE:
				switch (menu_i){
				case REMAP_MENU: 
					btn_mask[cfg_i] = PHYS_BUTTONS_NUM;
					break;
				case ANALOG_MENU:
					if (!(cfg_i % 5)) break;//Skip headers
					int o_idx = (cfg_i < 5) ? cfg_i - 1 : cfg_i - 2;
					analogs_options[o_idx] = (o_idx) ? ANALOGS_DEADZONE_DEF : ANALOGS_FORCE_DIGITAL_DEF;
					break;
				case TOUCH_MENU: 
					if (cfg_i < 16)
						touch_options[cfg_i] = TOUCH_POINTS_DEF[cfg_i % 8];
					else 
						touch_options[cfg_i] = TOUCH_MODE_DEF;
					break;
				case GYRO_MENU: 
					gyro_options[cfg_i] = (cfg_i < 3) ? GYRO_SENS_DEF : GYRO_DEADZONE_DEF;
					break;
				}
				break;
			case SCE_CTRL_START:
				switch (menu_i){
				case REMAP_MENU: 
					resetRemaps();
					break;
				case ANALOG_MENU: 
					resetAnalogs();
					break;
				case TOUCH_MENU: 
					resetTouch();
					break;
				case GYRO_MENU: 
					resetGyro();
					break;
				}
				break;
			case SCE_CTRL_CROSS:
				if (menu_i == MAIN_MENU){
					if (cfg_i == menu_entries-1) {
						show_menu = 0;
						saveConfig();
					} else {					
						menu_i = cfg_i + 1;
						cfg_i = 0;
					}
				}
				break;
			case SCE_CTRL_SELECT:
				if (menu_i == MAIN_MENU) {
					show_menu = 0;
					saveConfig();
				}
				break;
			case SCE_CTRL_CIRCLE:
				if (menu_i == MAIN_MENU) {
					show_menu = 0;
					saveConfig();
				} else {
					menu_i = MAIN_MENU;
					cfg_i = 0;
				}
				break;
			}
		}
		old_buttons = curr_buttons;
	}
}

void remap(SceCtrlData *ctrl, int count, int hookId, int logic) {
	
	//Nothing to do here
	if (count < 1)
		return;
	
	if (logic == NEGATIVE)
		ctrl[count - 1].buttons = 0xFFFFFFFF - ctrl[count - 1].buttons;
	
	//Checking for menu triggering
	if (!show_menu 
			&& (ctrl[count - 1].buttons & btns[menuActivator_mask[0]]) 
			&& (ctrl[count-1].buttons & btns[menuActivator_mask[1]])) {
		show_menu = 1;
		cfg_i = 0;
		//ToDo clear buffers
		remappedBuffersIdxs[hookId] = 0;
		remappedBuffersSizes[hookId] = 0;
	}
	if (show_menu){
		configInputHandler(&ctrl[count - 1]);
		for (int i = 0; i < count; i++)
			ctrl[i].buttons = (logic == POSITIVE) ? 0 : 0xFFFFFFFF;
		return;
	}
	
	int buffIdx = (remappedBuffersIdxs[hookId] + 1) % BUFFERS_NUM;
	
	//Storing copy of latest buffer
	remappedBuffersIdxs[hookId] = buffIdx;
	remappedBuffersSizes[hookId] = min(remappedBuffersSizes[hookId] + 1, BUFFERS_NUM);
	remappedBuffers[hookId][buffIdx] = ctrl[count-1]; 
	
	//Applying remap to latest buffer
	applyRemap(&remappedBuffers[hookId][buffIdx]);
	
	if (logic == NEGATIVE)
		remappedBuffers[hookId][buffIdx].buttons = 
			0xFFFFFFFF - remappedBuffers[hookId][buffIdx].buttons;
	
	//Restoring stored buffers
	for (int i = 0; i < count; i++)
		ctrl[i] = remappedBuffers[hookId]
			[(BUFFERS_NUM + buffIdx - count + i + 1) % BUFFERS_NUM];
	ctrl[count-1] = remappedBuffers[hookId][buffIdx];
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
			sceCtrlSetSamplingModeExt(SCE_CTRL_MODE_ANALOG);
			sceCtrlPeekBufferPositiveExt2(1, &pstv_fakepad, 1);
			ctrl[ret - 1].buttons = pstv_fakepad.buttons;
			internal_ext_call = 0;
		default:
			remap(ctrl, ret, 0, POSITIVE);
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
			sceCtrlSetSamplingModeExt(SCE_CTRL_MODE_ANALOG);
			sceCtrlPeekBufferPositiveExt2(1, &pstv_fakepad, 1);
			ctrl[ret - 1].buttons = pstv_fakepad.buttons;
			internal_ext_call = 0;
		default:
			remap(ctrl, ret, 1, POSITIVE);
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
			sceCtrlSetSamplingModeExt(SCE_CTRL_MODE_ANALOG);
			sceCtrlReadBufferPositiveExt2(1, &pstv_fakepad, 1);
			ctrl[ret - 1].buttons = pstv_fakepad.buttons;
			internal_ext_call = 0;
		default:
			remap(ctrl, ret, 2, POSITIVE);
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
			sceCtrlSetSamplingModeExt(SCE_CTRL_MODE_ANALOG);
			sceCtrlReadBufferPositiveExt2(1, &pstv_fakepad, 1);
			ctrl[ret - 1].buttons = pstv_fakepad.buttons;
			internal_ext_call = 0;
		default:
			remap(ctrl, ret, 3, POSITIVE);
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
			sceCtrlSetSamplingModeExt(SCE_CTRL_MODE_ANALOG);
			sceCtrlPeekBufferPositiveExt2(1, &pstv_fakepad, 1);
			ctrl[ret - 1].buttons = pstv_fakepad.buttons;
			internal_ext_call = 0;
		default:
			remap(ctrl, ret, 4, POSITIVE);
			used_funcs[4] = 1;
			break;
	}
	return ret;
}

int sceCtrlPeekBufferPositiveExt2_patched(int port, SceCtrlData *ctrl, int count) {
	int ret = TAI_CONTINUE(int, refs[5], port, ctrl, count);
	if (internal_ext_call) return ret;
	remap(ctrl, ret, 5, POSITIVE);
	used_funcs[5] = 1;
	return ret;
}

int sceCtrlReadBufferPositiveExt_patched(int port, SceCtrlData *ctrl, int count) {
	int ret = TAI_CONTINUE(int, refs[6], port, ctrl, count);
	switch (model) {
		case SCE_KERNEL_MODEL_VITATV:
			internal_ext_call = 1;
			sceCtrlSetSamplingModeExt(SCE_CTRL_MODE_ANALOG);
			sceCtrlReadBufferPositiveExt2(1, &pstv_fakepad, 1);
			ctrl[ret - 1].buttons = pstv_fakepad.buttons;
			internal_ext_call = 0;
		default:
			remap(ctrl, ret, 6, POSITIVE);
			used_funcs[6] = 1;
			break;
	}
	return ret;
}

int sceCtrlReadBufferPositiveExt2_patched(int port, SceCtrlData *ctrl, int count) {
	int ret = TAI_CONTINUE(int, refs[7], port, ctrl, count);
	if (internal_ext_call) return ret;
	remap(ctrl, ret, 7, POSITIVE);
	used_funcs[7] = 1;
	return ret;
}

int sceCtrlPeekBufferNegative_patched(int port, SceCtrlData *ctrl, int count) {
	int ret = TAI_CONTINUE(int, refs[8], port, ctrl, count);
	remap(ctrl, ret, 8, NEGATIVE);
	used_funcs[8] = 1;
	return ret;
}

int sceCtrlPeekBufferNegative2_patched(int port, SceCtrlData *ctrl, int count) {
	int ret = TAI_CONTINUE(int, refs[9], port, ctrl, count);
	remap(ctrl, ret, 9, NEGATIVE);
	used_funcs[9] = 1;
	return ret;
}

int sceCtrlReadBufferNegative_patched(int port, SceCtrlData *ctrl, int count) {
	int ret = TAI_CONTINUE(int, refs[10], port, ctrl, count);
	remap(ctrl, ret, 10, NEGATIVE);
	used_funcs[10] = 1;
	return ret;
}

int sceCtrlReadBufferNegative2_patched(int port, SceCtrlData *ctrl, int count) {
	int ret = TAI_CONTINUE(int, refs[11], port, ctrl, count);
	remap(ctrl, ret, 11, NEGATIVE);
	used_funcs[11] = 1;
	return ret;
}

int sceTouchRead_patched(SceUInt32 port, SceTouchData *pData, SceUInt32 nBufs) {
	int ret = TAI_CONTINUE(int, refs[12], port, pData, nBufs);
	if (!show_menu) updateTouchInfo(port, pData);
	used_funcs[12] = 1;
	return ret;
}

int sceTouchRead2_patched(SceUInt32 port, SceTouchData *pData, SceUInt32 nBufs) {
	int ret = TAI_CONTINUE(int, refs[13], port, pData, nBufs);
	if (!show_menu) updateTouchInfo(port, pData);
	used_funcs[13] = 1;
	return ret;
}

int sceTouchPeek_patched(SceUInt32 port, SceTouchData *pData, SceUInt32 nBufs) {
	int ret = TAI_CONTINUE(int, refs[14], port, pData, nBufs);
	if (internal_touch_call) return ret;
	if (!show_menu) updateTouchInfo(port, pData);
	used_funcs[14] = 1;
	return ret;
}

int sceTouchPeek2_patched(SceUInt32 port, SceTouchData *pData, SceUInt32 nBufs) {
	int ret = TAI_CONTINUE(int, refs[15], port, pData, nBufs);
	if (!show_menu) updateTouchInfo(port, pData);
	used_funcs[15] = 1;
	return ret;
}

int sceDisplaySetFrameBuf_patched(const SceDisplayFrameBuf *pParam, int sync) {
	if (show_menu) {
		new_frame = 1;
		screen_h = pParam->height;
		screen_w = pParam->width;
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
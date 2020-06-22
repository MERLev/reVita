#include <vitasdk.h>
#include <taihen.h>
#include <psp2/motion.h> 
#include <libk/string.h>
#include <libk/stdio.h>
#include <stdlib.h>
#include "renderer.h"

#define VERSION				1
#define SUBVERSION			2

#define HOOKS_NUM         17 // Hooked functions num
#define PHYS_BUTTONS_NUM  16 // Supported physical buttons num
#define TARGET_REMAPS     42 // Supported target remaps num
#define BUTTONS_NUM       38 // Supported buttons num
#define MENU_MODES        8  // Menu modes num
#define LONG_PRESS_TIME   350000	//0.35sec
#define COLOR_DEFAULT     0x00FFFFFF
#define COLOR_HEADER      0x00FF00FF
#define COLOR_CURSOR      0x0000FF00
#define COLOR_ACTIVE      0x0000FFFF
#define COLOR_DISABLE     0x000000FF
#define L_0    5		//Left margin for text
#define L_1    18		//Left margin for text
#define L_2    36
#define L_3    54

#define BUFFERS_NUM      			64
#define ANALOGS_DEADZONE_DEF		30
#define ANALOGS_FORCE_DIGITAL_DEF	0
#define ANOLOGS_OPTIONS_NUM			8
#define GYRO_SENS_DEF				127
#define GYRO_DEADZONE_DEF			15
#define GYRO_OPTIONS_NUM			6
#define MULTITOUCH_FRONT_NUM		6
#define MULTITOUCH_REAR_NUM			4
#define TOUCH_OPTIONS_NUM			19
#define TOUCH_MODE_DEF				1
#define CNTRL_OPTIONS_NUM			2
#define CREDITS_NUM					3

#ifndef max
# define max(a,b) (((a)>(b))?(a):(b))
# define min(a,b) (((a)<(b))?(a):(b))
# define lim(a,min,max) (((a)>(min))?((a<max)(a):(max)):(min))
#endif

static const uint16_t TOUCH_POINTS_DEF[16] = {
	600, 272,	//Front TL
	1280, 272,	//		TR
	600, 816,	//		BL
	1280, 816,	//		BR
	600, 272,	//Rear	TL
	1280, 272,	//		TR
	600, 608,	//		BL
	1280, 608	//		BR
};

static const uint8_t CNTRL_DEF[CNTRL_OPTIONS_NUM] = {
	0, 0,
};

enum{
	MAIN_MENU = 0,
	REMAP_MENU,
	ANALOG_MENU,
	TOUCH_MENU,
	GYRO_MENU,
	CNTRL_MENU,
	FUNCS_LIST,
	CREDITS_MENU
};

enum{
	POSITIVE = 0,
	NEGATIVE
};

static uint8_t btn_mask[BUTTONS_NUM];
static SceCtrlData remappedBuffers[HOOKS_NUM-5][BUFFERS_NUM];
static int remappedBuffersSizes[HOOKS_NUM];
static int remappedBuffersIdxs[HOOKS_NUM];

typedef struct EmulatedTouch{
	SceTouchReport reports[MULTITOUCH_FRONT_NUM];
	uint8_t num;
}EmulatedTouch;
EmulatedTouch etFront, etRear, prevEtFront, prevEtRear;
static uint8_t etFrontIdCounter = 64;
static uint8_t etRearIdCounter = 64;
static uint16_t TOUCH_SIZE[4] = {
	1920, 1088,	//Front
	1919, 890	//Rear
};

static uint8_t new_frame = 1;
static int screen_h = 272;
static int screen_w = 480;
static uint32_t ticker;
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
static uint8_t controller_options[CNTRL_OPTIONS_NUM];
static uint8_t used_funcs[HOOKS_NUM-1];

static char* str_menus[MENU_MODES] = {
	"MAIN MENU", 
	"REMAP MENU", 
	"ANALOG MENU", 
	"TOUCH MENU", 
	"GYRO_MENU", 
	"CONNECTED CONTROLLERS", 
	"USED FUNCTIONS",
	"CREDITS"
};

static char* str_main_menu[] = {
	"Change remap settings",
	"Change analog remap settings",
	"Change touch remap settings",
	"Change gyro remap settings",
	"Setup external gamepads",
	"Show imported functions",
	"Credits",
	"Return to the game"
};

static char* str_credits[] = {
	"Thanks to ", 
	"Tain Sueiras, nobodywasishere and RaveHeart",
	"for their awesome support on Patreon"
};

static char* str_yes_no[] = {
	"No", "Yes"
};

static char* str_touch_mode[] = {
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
static char* touch_points[4] = {"A", "B", "C", "D"};

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

void resetRemapsOptions(){
	for (int i = 0; i < BUTTONS_NUM; i++) {
		btn_mask[i] = PHYS_BUTTONS_NUM;
	}
}

void resetAnalogsOptions(){
	for (int i = 0; i < ANOLOGS_OPTIONS_NUM; i++)
		analogs_options[i] = i < 4 ? ANALOGS_DEADZONE_DEF : ANALOGS_FORCE_DIGITAL_DEF;
}

void resetTouchOptions(){
	for (int i = 0; i < TOUCH_OPTIONS_NUM - 3; i++)
		touch_options[i] = TOUCH_POINTS_DEF[i];
	touch_options[16] = TOUCH_MODE_DEF;
	touch_options[17] = TOUCH_MODE_DEF;
	touch_options[18] = 0;
}

void resetGyroOptions(){
	for (int i = 0; i < GYRO_OPTIONS_NUM; i++)
		gyro_options[i] = i < 3 ? GYRO_SENS_DEF : GYRO_DEADZONE_DEF;
}

void resetCntrlOptions(){
	for (int i = 0; i < CNTRL_OPTIONS_NUM; i++)
		controller_options[i] = CNTRL_DEF[i];
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
	int ret = max(0, idx - (screenEntries - bottom_l));
	while (ret > 0 && (entriesNum - ret - 1) < screenEntries) ret--;
	return ret;
}
// Config Menu Renderer
void drawConfigMenu() {
	setTextColor(COLOR_HEADER);
	drawStringF(L_0, 10, "remaPSV v.%hhu.%hhu - %s", VERSION, SUBVERSION, str_menus[menu_i]);
	//drawStringF(450, 10, "touchpad rear: %hu x %hu", debug3, debug4);
		
	int i, y = 10;
	int screen_entries = (screen_h - 50) / 20;
	int avaliable_entries = screen_entries - 1 - 1;
	switch (menu_i){
	case MAIN_MENU:
		for (i = calcStartingIndex(cfg_i, sizeof(str_main_menu)/sizeof(char*), avaliable_entries); i < sizeof(str_main_menu)/sizeof(char*); i++) {
			if (cfg_i == i){//Draw cursor
				setTextColor(COLOR_CURSOR);
				drawString(L_0, y + 20, (ticker % 16 < 8) ? "x" : "X");
			}
			
			setTextColor((i == cfg_i) ? COLOR_CURSOR : COLOR_DEFAULT);
			drawStringF(L_1, y += 20, "%s", str_main_menu[i]);
			if (y + 40 > screen_h) break;
		}
		setTextColor(COLOR_HEADER);
		drawString(L_0, screen_h - 24, "(X):select                                     (select) or (O) : save and close");
		break;
	case REMAP_MENU:
		for (i = calcStartingIndex(cfg_i, BUTTONS_NUM, avaliable_entries); i < BUTTONS_NUM; i++) {
			if (cfg_i == i){//Draw cursor
				setTextColor(COLOR_CURSOR);
				drawString(L_0, y + 20, (ticker % 16 < 8) ? "<" : ">");
			}
			
			if (i == cfg_i) setTextColor(COLOR_CURSOR);
			else if (btn_mask[i] == PHYS_BUTTONS_NUM) setTextColor(COLOR_DEFAULT);
			else if (btn_mask[i] == PHYS_BUTTONS_NUM + 1) setTextColor(COLOR_DISABLE);
			else setTextColor(COLOR_ACTIVE);
			drawStringF(L_1, y += 20, "%s -> %s", str_btns[i], target_btns[btn_mask[i]]);
			if (y + 60 > screen_h) break;
		}
		setTextColor(COLOR_HEADER);
		drawString(L_0, screen_h - 24, "(<)(>):change  (LT)(RT):section  ([]):reset  (start):reset all         (O):back");
		break;
	case ANALOG_MENU:
		for (i = calcStartingIndex(cfg_i, ANOLOGS_OPTIONS_NUM + 2, avaliable_entries); i < ANOLOGS_OPTIONS_NUM + 2; i++) {				
			if (y + 60 > screen_h) break;
			
			if (cfg_i == i){//Draw cursor
				setTextColor(COLOR_CURSOR);
				drawString(L_0, y + 20, !(i % 5) ? "-" : ((ticker % 16 < 8) ? "<" : ">"));
			}
			
			if (!(i % 5)){	//Headers
				setTextColor((i == cfg_i) ? COLOR_CURSOR : COLOR_DEFAULT);
				drawString(L_1, y+=20, (i == 0) ? "Deadzone:" : "Force digital output:");
				continue;
			}
			
			int o_idx = (i < 5) ? i - 1 : i - 2; //Index in options file
			if (i == cfg_i) setTextColor(COLOR_CURSOR);
			else if (analogs_options[o_idx] != ((o_idx < 4) ? ANALOGS_DEADZONE_DEF : ANALOGS_FORCE_DIGITAL_DEF)) 
				setTextColor(COLOR_ACTIVE);
			else setTextColor(COLOR_DEFAULT);
			if (o_idx < 4)
				drawStringF(L_2, y+=20, "%s [%s axis]: %hhu", 
					!(o_idx % 2) ? (((o_idx / 2) % 2 ) ? "Right Analog" : "Left Analog "): "            ",
					(o_idx % 2) ? "Y" : "X",
					analogs_options[o_idx]);
			else 
				drawStringF(L_2, y+=20, "%s [%s axis]: %s", 
					!(o_idx % 2) ? (((o_idx / 2) % 2 ) ? "Right Analog" : "Left Analog "): "            ",
					(o_idx % 2) ? "Y" : "X",
					analogs_options[o_idx] ? "Yes" : "No");	
		}
		setTextColor(COLOR_HEADER);
		drawString(L_0, screen_h - 24, "(<)(>):change  ([]):reset  (start):reset all                           (O):back");
		break;
	case TOUCH_MENU:
		for (i = calcStartingIndex(cfg_i, TOUCH_OPTIONS_NUM + 2, avaliable_entries); i < TOUCH_OPTIONS_NUM + 2; i++) {				
			if (y + 60 > screen_h) break;
			
			if (cfg_i == i){//Draw cursor
				setTextColor(COLOR_CURSOR);
				drawString(L_0, y + 20, (i == 0 || i == 9) ? "-" : ((ticker % 16 < 8) ? "<" : ">"));
			}
			
			if (i == 0 || i == 9){	//Headers
				setTextColor((i == cfg_i) ? COLOR_CURSOR : COLOR_DEFAULT);
				drawString(L_1, y+=20, (i == 0) ? "Front touch:" : "Rear touch:");
				continue;
			}
			
			if (i < 18){ //Points
				int o_idx = (i <= 8) ? i - 1 : i - 2; //Index in options file 
				if (i == cfg_i) setTextColor(COLOR_CURSOR);
				else if (touch_options[o_idx] != TOUCH_POINTS_DEF[o_idx]) 
					setTextColor(COLOR_ACTIVE);
				else setTextColor(COLOR_DEFAULT);
				if (o_idx < 16){
					if (!(o_idx % 2))
						drawStringF(L_2, y+=20, "Point [%s] x : %hu", 
							touch_points[(o_idx % 8)/2],
							touch_options[o_idx]);
					else 
						drawStringF(L_2, y+=20, "          y : %hu", 
							touch_options[o_idx]);
				}
				if (y + 60 > screen_h) break;
			}
			
			if (i == 18){ //Front touch mode
				if (18 == cfg_i) setTextColor(COLOR_CURSOR);
				else if (touch_options[16] == TOUCH_MODE_DEF) setTextColor(COLOR_DEFAULT);
				else if (touch_options[16] == 2) setTextColor(COLOR_DISABLE);
				else setTextColor(COLOR_ACTIVE);
				drawStringF(L_1, y+=20, "Front Touch Mode : %s", str_touch_mode[touch_options[16]]);
				if (y + 60 > screen_h) break;
			}
			
			if (i==19){ //Rear touch mode
				if (19 == cfg_i) setTextColor(COLOR_CURSOR);
				else if (touch_options[17] == TOUCH_MODE_DEF) setTextColor(COLOR_DEFAULT);
				else if (touch_options[17] == 2) setTextColor(COLOR_DISABLE);
				else setTextColor(COLOR_ACTIVE);
				drawStringF(L_1, y+=20, "Rear  Touch Mode : %s", str_touch_mode[touch_options[17]]);
				if (y + 60 > screen_h) break;
			}
			
			if (i==20){ //Touch compatibility
				if (20 == cfg_i) setTextColor(COLOR_CURSOR);
				else if (touch_options[18] == 0) setTextColor(COLOR_DEFAULT);
				else setTextColor(COLOR_DISABLE);
				drawStringF(L_1, y+=20, "Touch compatibility mode: %s", str_yes_no[touch_options[18]]);
				if (y + 60 > screen_h) break;
			}
		}
		//Draw blinking touch pointer
		if (cfg_i < 18 && cfg_i != 0 && cfg_i != 9){
			int o_idx = (cfg_i <= 8) ? cfg_i - 1 : cfg_i - 2; //Index in options file 
			int left = touch_options[o_idx - (o_idx % 2)] - 8;
			left *= (float)screen_w / ((o_idx < 8) ? TOUCH_SIZE[0] : TOUCH_SIZE[2]);
			left = min((max(0, left)), screen_w);
			int top = touch_options[o_idx - (o_idx % 2) + 1] - 10;
			top *= (float)screen_h / ((o_idx < 8) ? TOUCH_SIZE[1] : TOUCH_SIZE[3]); //Scale to framebuffer size
			top = min((max(0, top)), screen_h);//limit into screen
			setTextColor((ticker % 4) ? COLOR_CURSOR : COLOR_DISABLE);
			drawString(left, top, (ticker % 2) ? "" : "@");
		}
		setTextColor(COLOR_HEADER);
		drawString(L_0, screen_h - 24, "(<)(>)(RS)[TOUCH]:change  ([]):reset  (start):reset all  (O):back");
		break;
	case GYRO_MENU:
		for (i = calcStartingIndex(cfg_i, GYRO_OPTIONS_NUM + 2, avaliable_entries); i < GYRO_OPTIONS_NUM + 2; i++) {				
			if (y + 60 > screen_h) break;
			
			if (cfg_i == i){//Draw cursor
				setTextColor(COLOR_CURSOR);
				drawString(L_0, y + 20, !(i % 4) ? "-" : ((ticker % 16 < 8) ? "<" : ">"));
			}
			
			if (!(i % 4)){	//Headers
				setTextColor((i == cfg_i) ? COLOR_CURSOR : COLOR_DEFAULT);
				drawString(L_1, y+=20, (i == 0) ? "Sensivity:" : "Deadzone for digital mode:");
				continue;
			}
			
			int o_idx = (i < 4) ? i - 1 : i - 2; //Index in options file
			if (i == cfg_i) setTextColor(COLOR_CURSOR);
			else if (gyro_options[o_idx] != ((o_idx < 3) ? GYRO_SENS_DEF : GYRO_DEADZONE_DEF)) 
				setTextColor(COLOR_ACTIVE);
			else setTextColor(COLOR_DEFAULT);
			drawStringF(L_2, y+=20, "%s axis: %hhu", 
				((o_idx % 3) == 2) ? "Z" : ((o_idx % 3) ? "Y" : "X"),
				gyro_options[o_idx]);
		}
		setTextColor(COLOR_HEADER);
		drawString(L_0, screen_h - 24, "(<)(>):change  ([]):reset  (start):reset all                          (O): back");
		break;
	case CNTRL_MENU:;		
		SceCtrlPortInfo pi;
		int res = sceCtrlGetControllerPortInfo(&pi);
		if (res != 0){
			setTextColor(COLOR_DISABLE);
			drawString(L_1, y+= 20, "Error getting controllers info");
		} else {
			setTextColor(COLOR_CURSOR);
			drawString(L_0, y + 20 + 20 * (cfg_i % 2), (ticker % 16 < 8) ? "<" : ">");
			
			setTextColor(cfg_i == 0 ? COLOR_CURSOR : 
				(controller_options[0] == CNTRL_DEF[0] ? COLOR_DEFAULT : COLOR_ACTIVE));
			drawStringF(L_1, y += 20, "Using port: {%i} %s %s", 
				controller_options[0],
				getControllerName(pi.port[controller_options[0]]), 
				controller_options[0] == 0 ? "[DEFAULT]" : "");
				
			setTextColor(cfg_i == 1 ? COLOR_CURSOR : 
				(controller_options[1] == CNTRL_DEF[1] ? COLOR_DEFAULT : COLOR_ACTIVE));
			drawStringF(L_1, y += 20, "Swap L1<>LT R1<>RT: %s", str_yes_no[controller_options[1]]);
			
			y+=20;
			setTextColor(COLOR_DEFAULT);
			drawString(L_1, y+= 20, "Detected controllers:");
			for (int i = max(0, cfg_i - (avaliable_entries + 1)); i < 5; i++){
				setTextColor((L_1 == cfg_i) ? COLOR_CURSOR : ((pi.port[i] != SCE_CTRL_TYPE_UNPAIRED) ? COLOR_ACTIVE : COLOR_DEFAULT));
				drawStringF(L_1, y += 20, "Port %i: %s", i, getControllerName(pi.port[i]));
				if (y + 40 > screen_h) break;
			}	
		}
		setTextColor(COLOR_HEADER);
		drawString(L_0, screen_h - 24, "(<)(>):change  ([]):reset  (start):reset all                          (O): back");  
		break;
	case FUNCS_LIST:
		for (i = calcStartingIndex(cfg_i, HOOKS_NUM - 1, avaliable_entries); i < HOOKS_NUM - 1; i++) {
			if (cfg_i == i){//Draw cursor
				setTextColor(COLOR_CURSOR);
				drawString(L_0, y + 20, "-");
			}
			
			setTextColor((i == cfg_i) ? COLOR_CURSOR : (used_funcs[i] ? COLOR_ACTIVE : COLOR_DEFAULT));
			drawStringF(L_1, y += 20, "%s : %s", str_funcs[i], used_funcs[i] ? "Yes" : "No");
			if (y + 40 > screen_h) break;
		}
		setTextColor(COLOR_HEADER);
		drawString(L_1, screen_h - 24, "(O):back");                                                           
		break;   
	case CREDITS_MENU:
		y+=20;
		for (i = calcStartingIndex(cfg_i, CREDITS_NUM, avaliable_entries); i < CREDITS_NUM; i++) {			
			setTextColor(COLOR_DEFAULT);
			drawStringF(L_2, y += 20, "%s", str_credits[i]);
			if (y + 40 > screen_h) break;
		}
		setTextColor(COLOR_HEADER);
		drawString(L_1, screen_h - 24, "(O):back");                                                           
		break;                                                             
	default:
		break;
	}
}

void storeTouchPoint(EmulatedTouch *et, uint16_t x, uint16_t y){
	for (int i = 0; i < et->num; i++)
		if (et->reports[i].x == x && et->reports[i].y == y)
			return;
	et->reports[et->num].x = x;
	et->reports[et->num].y = y;
	et->num++;
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
			if (etFront.num == MULTITOUCH_FRONT_NUM) return;
			storeTouchPoint(&etFront,
				TOUCH_POINTS_DEF[(btn_mask[btn_idx] - (PHYS_BUTTONS_NUM + 10)) * 2],
				TOUCH_POINTS_DEF[(btn_mask[btn_idx] - (PHYS_BUTTONS_NUM + 10)) * 2 + 1]);
		} else if (btn_mask[btn_idx] < PHYS_BUTTONS_NUM + 18){	//Front touch custom
			if (etFront.num == MULTITOUCH_FRONT_NUM) return;
			storeTouchPoint(&etFront,
				touch_options[(btn_mask[btn_idx] - (PHYS_BUTTONS_NUM + 14)) * 2],
				touch_options[(btn_mask[btn_idx] - (PHYS_BUTTONS_NUM + 14)) * 2 + 1]);
		} else if (btn_mask[btn_idx] < PHYS_BUTTONS_NUM + 22){	//Rear  touch default
			if (etRear.num == MULTITOUCH_REAR_NUM) return;
			storeTouchPoint(&etRear,
				TOUCH_POINTS_DEF[8 + (btn_mask[btn_idx] - (PHYS_BUTTONS_NUM + 18)) * 2],
				TOUCH_POINTS_DEF[8 + (btn_mask[btn_idx] - (PHYS_BUTTONS_NUM + 18)) * 2 + 1]);
		} else if (btn_mask[btn_idx] < PHYS_BUTTONS_NUM + 26){	//Rear touch custom
			if (etRear.num == MULTITOUCH_REAR_NUM) return;
			storeTouchPoint(&etRear,
				touch_options[8 + (btn_mask[btn_idx] - (PHYS_BUTTONS_NUM + 22)) * 2],
				touch_options[8 + (btn_mask[btn_idx] - (PHYS_BUTTONS_NUM + 22)) * 2 + 1]);
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
	int left = TOUCH_SIZE[0]/2; int top = TOUCH_SIZE[1]/2;
	for (i=0;i<front.reportNum;i++) {
		if (front.report[i].x > left && front.report[i].y > top)       // Bot Right
			applyRemapRule(PHYS_BUTTONS_NUM + 3, &new_map, stickpos);
		else if (front.report[i].x <= left && front.report[i].y > top) // Bot Left
			applyRemapRule(PHYS_BUTTONS_NUM + 2, &new_map, stickpos);
		else if (front.report[i].x > left && front.report[i].y <= top) // Top Right
			applyRemapRule(PHYS_BUTTONS_NUM + 1, &new_map, stickpos);
		else if (front.report[i].x <= left && front.report[i].y <= top)// Top Left
			applyRemapRule(PHYS_BUTTONS_NUM, &new_map, stickpos);
	}
	
	// Applying remap rules for rear virtual buttons
	left = TOUCH_SIZE[2]/2; top = TOUCH_SIZE[3]/2;
	for (i=0;i<rear.reportNum;i++) {
		if (rear.report[i].x > left && rear.report[i].y > top)        // Bot Right
			applyRemapRule(PHYS_BUTTONS_NUM + 7, &new_map, stickpos);
		else if (rear.report[i].x <= left && rear.report[i].y > top)  // Bot Left
			applyRemapRule(PHYS_BUTTONS_NUM + 6, &new_map, stickpos);
		else if (rear.report[i].x > left && rear.report[i].y <= top)  // Top Right
			applyRemapRule(PHYS_BUTTONS_NUM + 5, &new_map, stickpos);
		else if (rear.report[i].x <= left && rear.report[i].y <= top) // Top Left
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

uint8_t generateTouchId(int x, int y, int panel){
	if (panel == SCE_TOUCH_PORT_FRONT){
		for (int i = 0; i < prevEtFront.num; i++)
			if (prevEtFront.reports[i].x == x && prevEtFront.reports[i].y == y)
				return prevEtFront.reports[i].id;
		etFrontIdCounter = (etFrontIdCounter + 1) % 127;
		return etFrontIdCounter;
	} else {
		for (int i = 0; i < prevEtRear.num; i++)
			if (prevEtRear.reports[i].x == x && prevEtRear.reports[i].y == y)
				return prevEtRear.reports[i].id;
		etRearIdCounter = (etRearIdCounter + 1) % 127;
		return etRearIdCounter;
	}
}


void addVirtualTouches(SceTouchData *pData, EmulatedTouch *et, 
		uint8_t touchPointsMaxNum, int panel){
	int touchIdx = 0;
	while (touchIdx < et->num && pData->reportNum < touchPointsMaxNum){
		pData->report[pData->reportNum].x = et->reports[touchIdx].x;
		pData->report[pData->reportNum].y = et->reports[touchIdx].y;
		et->reports[touchIdx].id = generateTouchId(
			et->reports[touchIdx].x, et->reports[touchIdx].y, panel);
		pData->report[pData->reportNum].id = et->reports[touchIdx].id;
		et->reports[touchIdx].id = pData->report[pData->reportNum].id;
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
		addVirtualTouches(pData, &etFront, 
			MULTITOUCH_FRONT_NUM, SCE_TOUCH_PORT_FRONT);
		prevEtFront = etFront;
		etFront.num = 0;
	} else {
		if (touch_options[17] == 2 || 
			(touch_options[17] == 1 &&
				((btn_mask[PHYS_BUTTONS_NUM+4] != PHYS_BUTTONS_NUM) ||
				(btn_mask[PHYS_BUTTONS_NUM+5] != PHYS_BUTTONS_NUM) ||
				(btn_mask[PHYS_BUTTONS_NUM+6] != PHYS_BUTTONS_NUM) ||
				(btn_mask[PHYS_BUTTONS_NUM+7] != PHYS_BUTTONS_NUM))))
			pData->reportNum = 0; //Disable pad
		addVirtualTouches(pData, &etRear, 
			MULTITOUCH_REAR_NUM, SCE_TOUCH_PORT_BACK);
		prevEtRear = etRear;
		etRear.num = 0;
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
	sceIoWrite(fd, touch_options, TOUCH_OPTIONS_NUM*2);
	sceIoClose(fd);
	
	// Opening gyro config file and saving the config
	fd = sceIoOpen("ux0:/data/remaPSV/gyro.bin", SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
	sceIoWrite(fd, gyro_options, GYRO_OPTIONS_NUM);
	sceIoClose(fd);
	
	// Opening gyro config file and saving the config
	fd = sceIoOpen("ux0:/data/remaPSV/controllers.bin", SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
	sceIoWrite(fd, controller_options, CNTRL_OPTIONS_NUM);
	sceIoClose(fd);
}

void loadConfig(void) {
	resetRemapsOptions();
	resetAnalogsOptions();
	resetTouchOptions();
	resetGyroOptions();
	
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
		sceIoRead(fd, touch_options, TOUCH_OPTIONS_NUM*2);
		sceIoClose(fd);
	}
	
	// Loading gyro config file
	fd = sceIoOpen("ux0:/data/remaPSV/gyro.bin", SCE_O_RDONLY, 0777);
	if (fd >= 0){
		sceIoRead(fd, gyro_options, GYRO_OPTIONS_NUM);
		sceIoClose(fd);
	}
	
	// Loading gyro config file
	fd = sceIoOpen("ux0:/data/remaPSV/controllers.bin", SCE_O_RDONLY, 0777);
	if (fd >= 0){
		sceIoRead(fd, controller_options, CNTRL_OPTIONS_NUM);
		sceIoClose(fd);
	}
}

uint8_t isBtnActive(uint8_t btnNum){
	return ((curr_buttons & btns[btnNum]) && !(old_buttons & btns[btnNum])) 
		|| (pressedTicks[btnNum] != 0 && tick - pressedTicks[btnNum] > LONG_PRESS_TIME);
}

void touchPicker(int padType){
	if ((padType == SCE_TOUCH_PORT_FRONT && (cfg_i < 1 || cfg_i >= 9)) ||
		(padType == SCE_TOUCH_PORT_BACK && (cfg_i < 9 || cfg_i >= 18)))
		return;
	int o_idx = (cfg_i < 9) ? cfg_i - 1 : cfg_i - 2;
	SceTouchData std;
	internal_touch_call = 1;
	int ret = sceTouchRead(padType, &std, 1);
	internal_touch_call = 0;
	if (ret && std.reportNum){
		touch_options[o_idx - (o_idx % 2)] = std.report[0].x;
		touch_options[o_idx - (o_idx % 2) + 1] = std.report[0].y;
	}
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
			menu_entries = TOUCH_OPTIONS_NUM + 2;
			touchPicker(SCE_TOUCH_PORT_FRONT);
			touchPicker(SCE_TOUCH_PORT_BACK);
			break;
		case GYRO_MENU:
			menu_entries = GYRO_OPTIONS_NUM + 2;
			break;
		case CNTRL_MENU:
			menu_entries = CNTRL_OPTIONS_NUM;
			break;
		case FUNCS_LIST:
			menu_entries = HOOKS_NUM - 1;
			break;
		default:
			break;
		}
		tick = ctrl->timeStamp;
		curr_buttons = ctrl->buttons;
		int o_idx; //options file real index
		
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
					o_idx = (cfg_i < 5) ? cfg_i - 1 : cfg_i - 2;
					if (o_idx < 4) analogs_options[o_idx] = (analogs_options[o_idx] + 1) % 128;
					else analogs_options[o_idx] = !analogs_options[o_idx];
					break;
				case TOUCH_MENU:
					if (cfg_i == 0 || cfg_i == 9) break;//Skip headers
					o_idx = (cfg_i < 9) ? cfg_i - 1 : cfg_i - 2;
					if (o_idx < 8)//Front Points xy
						touch_options[o_idx] = (touch_options[o_idx] + 1) 
							% ((o_idx % 2) ? TOUCH_SIZE[1] : TOUCH_SIZE[0]);
					else if (o_idx < 16)//Rear Points xy
						touch_options[o_idx] = (touch_options[o_idx] + 1)
							% ((o_idx % 2) ? TOUCH_SIZE[3] : TOUCH_SIZE[2]);
					else if (o_idx < 18)//Mode
						touch_options[o_idx] = (touch_options[o_idx] + 1) % 3;
					else 			//Compability
						touch_options[o_idx] = !touch_options[o_idx];
					break;
				case GYRO_MENU:
					if (!(cfg_i % 4)) break;//Skip headers
					o_idx = (cfg_i < 4) ? cfg_i - 1 : cfg_i - 2;
					gyro_options[o_idx] = (gyro_options[o_idx] + 1) % 200;
					break;
				case CNTRL_MENU:
					if (!cfg_i)
						controller_options[cfg_i] = min(5, controller_options[cfg_i] + 1);
					else
						controller_options[cfg_i] = !controller_options[cfg_i];
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
					o_idx = (cfg_i < 5) ? cfg_i - 1 : cfg_i - 2;
					if (analogs_options[o_idx]) 	
						analogs_options[o_idx]--;
					else
						analogs_options[o_idx] = o_idx < 4 ? 127 : 1;
					break;
				case TOUCH_MENU:
					if (cfg_i == 0 || cfg_i == 9) break;//Skip headers
					o_idx = (cfg_i < 9) ? cfg_i - 1 : cfg_i - 2;
					if (touch_options[o_idx]) 	
						touch_options[o_idx]--;
					else {
						if (o_idx < 8)//front points xy
							touch_options[o_idx] = ((o_idx % 2) ? TOUCH_SIZE[1] - 1 : TOUCH_SIZE[0] - 1);
						if (o_idx < 16)//rear points xy
							touch_options[o_idx] = ((o_idx % 2) ? TOUCH_SIZE[3] - 1 : TOUCH_SIZE[2] - 1);
						else if (o_idx < 18)//touch mode
							touch_options[o_idx] = 2;
						else //compability mode
							touch_options[o_idx] = !touch_options[o_idx];
					}
					break;
				case GYRO_MENU:
					if (!(cfg_i % 4)) break;//Skip headers
					o_idx = (cfg_i < 4) ? cfg_i - 1 : cfg_i - 2;
					if (gyro_options[o_idx]) 	
						gyro_options[o_idx]--;
					else
						gyro_options[o_idx] = 199;
					break;
				case CNTRL_MENU:
					if (!cfg_i)
						controller_options[cfg_i] = max(0, controller_options[cfg_i] - 1);
					else
						controller_options[cfg_i] = !controller_options[cfg_i];
					break;
				}
				break;
			case SCE_CTRL_LTRIGGER:
			case SCE_CTRL_L1:
				if (menu_i == REMAP_MENU){ //Sections navigation
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
				if (menu_i == REMAP_MENU){ //Sections navigation
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
					o_idx = (cfg_i < 5) ? cfg_i - 1 : cfg_i - 2;
					analogs_options[o_idx] = (o_idx < 4) ? ANALOGS_DEADZONE_DEF : ANALOGS_FORCE_DIGITAL_DEF;
					break;
				case TOUCH_MENU: 
					if (cfg_i == 0 || cfg_i == 9) break;//Skip headers
					o_idx = (cfg_i < 9) ? cfg_i - 1 : cfg_i - 2;
					if (o_idx < 16)
						touch_options[o_idx] = TOUCH_POINTS_DEF[o_idx];
					else if (o_idx < 16)
						touch_options[o_idx] = TOUCH_MODE_DEF;
					else
						touch_options[o_idx] = 0;
					break;
				case GYRO_MENU:
					if (!(cfg_i % 4)) break;//Skip headers
					o_idx = (cfg_i < 4) ? cfg_i - 1 : cfg_i - 2; 
					gyro_options[o_idx] = (o_idx < 3) ? GYRO_SENS_DEF : GYRO_DEADZONE_DEF;
					break;
				case CNTRL_MENU:
					controller_options[cfg_i] = CNTRL_DEF[cfg_i];
					break;
				}
				break;
			case SCE_CTRL_START:
				switch (menu_i){
				case REMAP_MENU: 
					resetRemapsOptions();
					break;
				case ANALOG_MENU: 
					resetAnalogsOptions();
					break;
				case TOUCH_MENU: 
					resetTouchOptions();
					break;
				case GYRO_MENU: 
					resetGyroOptions();
					break;
				case CNTRL_MENU: 
					resetCntrlOptions();
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
	if (count < 1)
		return;	//Nothing to do here
	
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

void swapTriggersBumpers(SceCtrlData *ctrl, int count){
	if (!controller_options[1])
		return;
	
	for (int i = 0; i < count; i++){
		uint32_t b = 0;
		for (int i = 0; i < PHYS_BUTTONS_NUM; i++)
			if (ctrl[i].buttons && btns[i]){
				if (btns[i] == SCE_CTRL_LTRIGGER) b+= SCE_CTRL_L1;
				else if (btns[i] == SCE_CTRL_L1) b+= SCE_CTRL_LTRIGGER;
				else if (btns[i] == SCE_CTRL_RTRIGGER) b+= SCE_CTRL_R1;
				else if (btns[i] == SCE_CTRL_R1) b+= SCE_CTRL_RTRIGGER;
				else b += btns[i];
			}
		ctrl[i].buttons = b;
	}
}

int patchToExt(int port, SceCtrlData *ctrl, int count, int read){
	//if (controller_options[0] <= 0 || controller_options[0] == port)
	if (controller_options[0] <= 0)
		return count;
	
	//sceCtrlSetSamplingModeExt(SCE_CTRL_MODE_ANALOG);
	SceCtrlData pstv_fakepad[count];
	int ret;
	internal_ext_call = 1;
	if (read)
		ret = sceCtrlReadBufferPositiveExt2(controller_options[0], &pstv_fakepad[0], count);
	else 
		ret = sceCtrlPeekBufferPositiveExt2(controller_options[0], &pstv_fakepad[0], count);
	internal_ext_call = 0;
	if (ret < 0)
		return count;
	
	//ToDo R1 <> RT mby ?
	for (int i = 0; i < ret; i++)
		ctrl[i].buttons = pstv_fakepad->buttons;
	return ret;
}

int onTouch(SceUInt32 port, SceTouchData *pData, SceUInt32 nBufs){
	if (!internal_ext_call && show_menu) { //Disable in menu
		pData[0] = pData[nBufs - 1];
		pData[0].reportNum = 0;
		return 1;
	}
	if (nBufs && !show_menu) updateTouchInfo(port, &pData[nBufs - 1]);
	if (touch_options[18]){//Compability mode - return only one buffer
		pData[0] = pData[nBufs - 1];
		return 1;
	}
	return nBufs;
}

// Simplified generic hooking function
void hookFunction(uint32_t nid, const void *func) {
	hooks[current_hook] = taiHookFunctionImport(&refs[current_hook],TAI_MAIN_MODULE,TAI_ANY_LIBRARY,nid,func);
	current_hook++;
}

int sceCtrlPeekBufferPositive_patched(int port, SceCtrlData *ctrl, int count) {
	int ret = TAI_CONTINUE(int, refs[0], port, ctrl, count);
	ret = patchToExt(port, ctrl, ret, 0);
	remap(ctrl, ret, 0, POSITIVE);
	used_funcs[0] = 1;
	return ret;
}

int sceCtrlPeekBufferPositive2_patched(int port, SceCtrlData *ctrl, int count) {
	int ret = TAI_CONTINUE(int, refs[1], port, ctrl, count);
	ret = patchToExt(port, ctrl, ret, 0);
	remap(ctrl, ret, 1, POSITIVE);
	used_funcs[1] = 1;
	return ret;
}

int sceCtrlReadBufferPositive_patched(int port, SceCtrlData *ctrl, int count) {
	int ret = TAI_CONTINUE(int, refs[2], port, ctrl, count);
	ret = patchToExt(port, ctrl, ret, 1);
	remap(ctrl, ret, 2, POSITIVE);
	used_funcs[2] = 1;
	return 0;
}

int sceCtrlReadBufferPositive2_patched(int port, SceCtrlData *ctrl, int count) {
	int ret = TAI_CONTINUE(int, refs[3], port, ctrl, count);
	ret = patchToExt(port, ctrl, ret, 1);
	remap(ctrl, ret, 3, POSITIVE);
	used_funcs[3] = 1;
	return ret;
}

int sceCtrlPeekBufferPositiveExt_patched(int port, SceCtrlData *ctrl, int count) {
	int ret = TAI_CONTINUE(int, refs[4], port, ctrl, count);
	ret = patchToExt(port, ctrl, ret, 0);
	remap(ctrl, ret, 4, POSITIVE);
	used_funcs[4] = 1;
	return ret;
}

int sceCtrlPeekBufferPositiveExt2_patched(int port, SceCtrlData *ctrl, int count) {
	int ret = TAI_CONTINUE(int, refs[5], port, ctrl, count);
	swapTriggersBumpers(ctrl, ret);
	if (internal_ext_call) return ret;
	remap(ctrl, ret, 5, POSITIVE);
	used_funcs[5] = 1;
	return ret;
}

int sceCtrlReadBufferPositiveExt_patched(int port, SceCtrlData *ctrl, int count) {
	int ret = TAI_CONTINUE(int, refs[6], port, ctrl, count);
	ret = patchToExt(port, ctrl, ret, 1);
	remap(ctrl, ret, 6, POSITIVE);
	used_funcs[6] = 1;
	return ret;
}

int sceCtrlReadBufferPositiveExt2_patched(int port, SceCtrlData *ctrl, int count) {
	int ret = TAI_CONTINUE(int, refs[7], port, ctrl, count);
	swapTriggersBumpers(ctrl, ret);
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
	used_funcs[12] = 1;
	return onTouch(port, pData, ret);
}

int sceTouchRead2_patched(SceUInt32 port, SceTouchData *pData, SceUInt32 nBufs) {
	int ret = TAI_CONTINUE(int, refs[13], port, pData, nBufs);
	used_funcs[13] = 1;
	return onTouch(port, pData, ret);
}

int sceTouchPeek_patched(SceUInt32 port, SceTouchData *pData, SceUInt32 nBufs) {
	int ret = TAI_CONTINUE(int, refs[14], port, pData, nBufs);
	used_funcs[14] = 1;
	return onTouch(port, pData, ret);
}

int sceTouchPeek2_patched(SceUInt32 port, SceTouchData *pData, SceUInt32 nBufs) {
	int ret = TAI_CONTINUE(int, refs[15], port, pData, nBufs);
	used_funcs[15] = 1;
	return onTouch(port, pData, ret);
}

int sceDisplaySetFrameBuf_patched(const SceDisplayFrameBuf *pParam, int sync) {
	if (show_menu) {
		new_frame = 1;
		ticker++;
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
	
	//Detecting touch panels size
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
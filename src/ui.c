#include <vitasdkkern.h>
#include <psp2/motion.h> 
#include <psp2/touch.h> 
#include <psp2kern/ctrl.h> 
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "vitasdkext.h"
#include "main.h"
#include "renderer.h"
#include "icons.h"
#include "ui.h"
#include "profile.h"
#include "common.h"
#include "log.h"

#define VERSION				2
#define SUBVERSION			1
#define SUBSUBVERSION		0

#define UI_WIDTH            480
#define UI_HEIGHT           272
#define HEADER_HEIGHT		(CHA_H + 6)

#define BOTTOM_OFFSET		5

#define COLOR_DEFAULT     	0x00C2C0BD
#define COLOR_CURSOR      	0x00FFFFFF
#define COLOR_HEADER      	0x00FF6600
#define COLOR_CURSOR_HEADER	0x00FFAA22
#define COLOR_ACTIVE      	0x0000B0B0
#define COLOR_CURSOR_ACTIVE	0x0000DDDD
#define COLOR_DANGER      	0x00000099
#define COLOR_CURSOR_DANGER	0x000000DD
#define COLOR_BG_HEADER   	0x00000000
#define COLOR_BG_BODY     	0x00171717
#define L_0    5		//Left margin for text
#define L_1    18		
#define L_2    36

static const char* str_menus[MENU_MODES] = {
	"MAIN MENU", 
	"REMAP RULES", 
	"ANALOG STICKS", 
	"TOUCH", 
	"GYROSCOPE", 
	"CONNECTED CONTROLLERS", 
	"HOOKS",
	"SETTINGS",
	"CREDITS",
	"PROFILES"
};
static const char* str_footer[MENU_MODES] = {
	"",
	"(L)(R):section ([])/(start):reset/all", 
	"([]):reset  (start):reset all", 
	"[TOUCH](RS):change  ([])/(start):reset", 
	"([]):reset  (start):reset all", 
	"([]):reset  (start):reset all", 
	"",
	"([]):reset  (start):reset all",
	"",
	""
};
static const char* str_credits[CREDITS_NUM] = {
	"                     updated by Mer1e ",
	"               with the help of S1ngy ",
	"       original author Rinnegatamante ",
	"",
	"Thanks to",
	"  Cassie, W0lfwang, TheIronUniverse,",
	"  Kiiro Yakumo and mantixero",
	"     for enduring endless crashes",
	"         while testing this thing",
	"  Vita Nuova community",
	"    for all the help I got there",
	"",
	"Original Rinnegatamante's thanks to", 
	"  Tain Sueiras, nobodywasishere and",
	"  RaveHeart for their awesome",
	"    support on Patreon"
};
static const char* str_yes_no[] = {
	"No", "Yes"
};
static const char* str_funcs[HOOKS_NUM-2] = {
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
	"ksceTouchRead",
	"ksceTouchRead2",
	"ksceTouchPeek",
	"ksceTouchPeek2",
};
static const char* str_btns[PHYS_BUTTONS_NUM] = {
	"Cross", "Circle", "Triangle", "Square",
	"Start", "Select", 
	"LT/L2", "RT/R2",
	"Up", "Right", "Left", "Down", 
	"L1", "R1", "L3", "R3"
};
static const char* str_sections[] = {
	"Buttons", "LStick", "RStick", 
	"FrontTouch", "RearTouch", "Gyroscope",
	"","Disabled"
};
static const char* str_analog_directions[] = {
	"Left", "Right", "Up", "Down"
};
static const char* str_touch_zones[] = {
	"TopLeft", "TopRight", "BotLeft", "BotRight"
};
static const char* str_gyro_directions[] = {
	"Left", "Right", "Up", "Down",
	"Roll Left","Roll Right"
};
static const char* str_touch_points[] = {
	"Point A", "Point B", "Point C", "Point D"
};
static char* str_deadband[] = {
	"Game default", "Enable", "Disable"
};
char* getControllerName(int id){
	if 		(id == 	SCE_CTRL_TYPE_UNPAIRED) return "Unpaired";
	else if (id == 	SCE_CTRL_TYPE_PHY) 		return "Physical VITA";
	else if (id == 	SCE_CTRL_TYPE_VIRT) 	return "Virtual PSTV";
	else if (id == 	SCE_CTRL_TYPE_DS3) 		return "DualShock 3";
	else if (id == 	SCE_CTRL_TYPE_DS4) 		return "DualShock 4";
	else 									return "Unknown";
}

static struct MenuOpt menu_main[MAIN_MENU_NUM] = {
	(MenuOpt){.name = "Remap rules", .idx = REMAP_MENU},
	(MenuOpt){.name = "Analog sticks", .idx = ANALOG_MENU},
	(MenuOpt){.name = "Touch", .idx = TOUCH_MENU},
	(MenuOpt){.name = "Gyroscope", .idx = GYRO_MENU},
	(MenuOpt){.name = "External gamepads", .idx = CNTRL_MENU},
	(MenuOpt){.name = "[DEBUG] Show hooks", .idx = FUNCS_LIST},
	(MenuOpt){.name = "Settings", .idx = SETTINGS_MENU},
	(MenuOpt){.name = "Profiles", .idx = PROFILES_MENU},
	(MenuOpt){.name = "Credits", .idx = CREDITS_MENU},
};
static struct MenuOpt menu_analog[ANALOG_MENU_NUM] = {
	(MenuOpt){.name = "Deadzone", .idx = HEADER_IDX},
	(MenuOpt){.name = "Left Analog  [X]", .idx = 0},
	(MenuOpt){.name = "             [Y]", .idx = 1},
	(MenuOpt){.name = "Right Analog [X]", .idx = 2},
	(MenuOpt){.name = "             [Y]", .idx = 3},
	(MenuOpt){.name = "Force digital", .idx = HEADER_IDX},
	(MenuOpt){.name = "Left Analog  [X]", .idx = 4},
	(MenuOpt){.name = "             [Y]", .idx = 5},
	(MenuOpt){.name = "Right Analog [X]", .idx = 6},
	(MenuOpt){.name = "             [Y]", .idx = 7}
};
static struct MenuOpt menu_touch[TOUCH_MENU_NUM] = {
	(MenuOpt){.name = "Front", .idx = HEADER_IDX},
	(MenuOpt){.name = "Point A           x", .idx = 0},
	(MenuOpt){.name = "                  y", .idx = 1},
	(MenuOpt){.name = "Point B           x", .idx = 2},
	(MenuOpt){.name = "                  y", .idx = 3},
	(MenuOpt){.name = "Point C           x", .idx = 4},
	(MenuOpt){.name = "                  y", .idx = 5},
	(MenuOpt){.name = "Point D           x", .idx = 6},
	(MenuOpt){.name = "                  y", .idx = 7},
	(MenuOpt){.name = "Disable if remapped", .idx = 16},
	(MenuOpt){.name = "Rear", .idx = HEADER_IDX},
	(MenuOpt){.name = "Point A           x", .idx = 8},
	(MenuOpt){.name = "                  y", .idx = 9},
	(MenuOpt){.name = "Point B           x", .idx = 10},
	(MenuOpt){.name = "                  y", .idx = 11},
	(MenuOpt){.name = "Point C           x", .idx = 12},
	(MenuOpt){.name = "                  y", .idx = 13},
	(MenuOpt){.name = "Point D           x", .idx = 14},
	(MenuOpt){.name = "                  y", .idx = 15},
	(MenuOpt){.name = "Disable if remapped", .idx = 17}
};
static struct MenuOpt menu_gyro[GYRO_MENU_NUM] = {
	(MenuOpt){.name = "Sensivity", .idx = HEADER_IDX},
	(MenuOpt){.name = "    X Axis", .idx = 0},
	(MenuOpt){.name = "    Y Axis", .idx = 1},
	(MenuOpt){.name = "    Z Axis", .idx = 2},
	(MenuOpt){.name = "Deadzone", .idx = HEADER_IDX},
	(MenuOpt){.name = "    X Axis", .idx = 3},
	(MenuOpt){.name = "    Y Axis", .idx = 4},
	(MenuOpt){.name = "    Z Axis", .idx = 5},
	(MenuOpt){.name = "More", .idx = HEADER_IDX},
	(MenuOpt){.name = "Deadband  ", .idx = 6},
	(MenuOpt){.name = "Wheel mode", .idx = 7},
};
static struct MenuOpt menu_settings[SETTINGS_MENU_NUM] = {
	(MenuOpt){.name = "Menu trigger first key", .idx = 0},
	(MenuOpt){.name = "            second key", .idx = 1},
	(MenuOpt){.name = "Save profile on close ", .idx = 2},
	(MenuOpt){.name = "Startup delay         ", .idx = 3}
};
static struct MenuOpt menu_profiles[PROFILE_MENU_NUM] = {
	(MenuOpt){.name = "Global", .idx = HEADER_IDX},
	(MenuOpt){.name = "Save", .idx = PROFILE_GLOBAL_SAVE},
	(MenuOpt){.name = "Load", .idx = PROFILE_GLOABL_LOAD},
	(MenuOpt){.name = "Delete", .idx = PROFILE_GLOBAL_DELETE},
	(MenuOpt){.name = "Game", .idx = HEADER_IDX},
	(MenuOpt){.name = "Save", .idx = PROFILE_LOCAL_SAVE},
	(MenuOpt){.name = "Load", .idx = PROFILE_LOCAL_LOAD},
	(MenuOpt){.name = "Delete", .idx = PROFILE_LOCAL_DELETE}
};
static struct MenuOpt menu_controllers[CONTROLLERS_MENU_NUM] = {
	(MenuOpt){.name = "Use external       ", .idx = 0},
	(MenuOpt){.name = "Selected controller", .idx = 1},
	(MenuOpt){.name = "Swap L1<>LT R1<>RT ", .idx = 2}
};

uint8_t ui_opened = 0;
uint8_t new_frame = 1;
static uint32_t ticker;
int cfg_i = 0;
MenuOpt ui_opt;
int menu_i = MAIN_MENU;

//Calculate starting index for scroll menu
int calcStartingIndex(int idx, int entriesNum, int screenEntries, int bottomOffset){
	int ret = max(0, idx - (screenEntries - bottomOffset - 1));
	while (ret > 0 && (entriesNum - ret) < screenEntries) ret--;
	return ret;
}
void setColorHeader(uint8_t isCursor){
	if (isCursor){
		renderer_setColor(COLOR_CURSOR_HEADER);
	} else {
		renderer_setColor(COLOR_HEADER);
	}
}
void setColor(uint8_t isCursor, uint8_t isDefault){
	if (isCursor){
		if (isDefault)
			renderer_setColor(COLOR_CURSOR);
		else 
			renderer_setColor(COLOR_CURSOR_ACTIVE);
	} else {
		if (isDefault)
			renderer_setColor(COLOR_DEFAULT);
		else 
			renderer_setColor(COLOR_ACTIVE);
	}
}
void drawScroll(int8_t up, int8_t down){
	renderer_setColor(COLOR_HEADER);
	if (up)
		renderer_drawImage(UI_WIDTH - ICN_ARROW_X - 3, HEADER_HEIGHT + 3, ICN_ARROW_X, ICN_ARROW_Y, ICN_ARROW_UP);
	if (down)
		renderer_drawImage(UI_WIDTH - ICN_ARROW_X - 3, UI_HEIGHT - HEADER_HEIGHT - ICN_ARROW_X - 3, ICN_ARROW_X, ICN_ARROW_Y, ICN_ARROW_DOWN);
}
void drawFullScroll(int8_t up, int8_t down, float pos){
	if (!up && !down) return;
	renderer_setColor(COLOR_HEADER);
	// uint16_t calculatedPos = HEADER_HEIGHT + 4 + ICN_ARROW_Y + pos * (UI_HEIGHT - 2 * HEADER_HEIGHT - 4 * ICN_ARROW_Y - 8);
	// renderer_drawImage(UI_WIDTH - ICN_ARROW_X - 3, calculatedPos, ICN_ARROW_X, ICN_ARROW_Y, ICN_ARROW_UP);
	// renderer_drawImage(UI_WIDTH - ICN_ARROW_X - 3, calculatedPos + ICN_ARROW_Y, ICN_ARROW_X, ICN_ARROW_Y, ICN_ARROW_DOWN);
	uint16_t calculatedPos = HEADER_HEIGHT + 4 + ICN_ARROW_Y + pos * (UI_HEIGHT - 2 * HEADER_HEIGHT - 2 * ICN_ARROW_Y - ICN_SLIDER_Y - 8);
	renderer_drawImage(UI_WIDTH - ICN_ARROW_X - 3, calculatedPos, ICN_SLIDER_X, ICN_SLIDER_Y, ICN_SLIDER);
	drawScroll(1,1);
}
void drawEditPointer(uint16_t x, uint16_t y){
	renderer_drawImage(x, y, ICN_ARROW_X, ICN_ARROW_Y, (ticker % 32 < 16) ? ICN_ARROW_LEFT : ICN_ARROW_RIGHT);
}

void drawHeader(){
	renderer_drawRectangle(0, 0, UI_WIDTH, HEADER_HEIGHT - 1, COLOR_BG_HEADER);//BG
	renderer_drawRectangle(0, HEADER_HEIGHT - 1, UI_WIDTH, 1, COLOR_HEADER);//Separator
	renderer_setColor(COLOR_HEADER);
	if (menu_i == MAIN_MENU)
		renderer_drawStringF(L_0, 3, "remaPSV2 v.%hhu.%hhu.%hhu", VERSION, SUBVERSION, SUBSUBVERSION);
	else	
		renderer_drawString(L_0, 3, str_menus[menu_i]);
	
	renderer_drawString(UI_WIDTH - CHA_W * strlen(titleid) - 10, 2, titleid);
}

void drawFooter(){
	renderer_drawRectangle(0, UI_HEIGHT - HEADER_HEIGHT, UI_WIDTH, 1, COLOR_HEADER);//Separator
	renderer_drawRectangle(0, UI_HEIGHT - (HEADER_HEIGHT - 1), UI_WIDTH, HEADER_HEIGHT - 1, COLOR_BG_HEADER);//BG
	renderer_setColor(COLOR_HEADER);                                                               
	renderer_drawStringF(L_0, UI_HEIGHT-HEADER_HEIGHT + 4, str_footer[menu_i]);
}

void drawMenu_main(int avaliableEntries, int y){
	int ii = calcStartingIndex(cfg_i, MAIN_MENU_NUM, avaliableEntries, BOTTOM_OFFSET);
	for (int i = ii; i < min(ii + avaliableEntries, MAIN_MENU_NUM); i++) {
		setColor(i == cfg_i, 1);
		renderer_drawString(L_1, y += CHA_H, menu_main[i].name);
	}
}

void drawMenu_remap(int avaliableEntries, int y){
	for (int i = calcStartingIndex(cfg_i, PROFILE_REMAP_NUM, avaliableEntries, BOTTOM_OFFSET); i < PROFILE_REMAP_NUM; i++) {
		if (cfg_i == i){//Draw cursor
			renderer_setColor(COLOR_CURSOR);
			renderer_drawString(L_0 + CHA_W*10, y + CHA_H, (ticker % 16 < 8) ? "<" : ">");
		}
		
		const char *srcSection = "", *srcAction = "", *targetSection = "", *targetAction = "";
		//Source section
		if (i == 0) srcSection = str_sections[0];
		else if (i == PHYS_BUTTONS_NUM)  srcSection = str_sections[3];
		else if (i == PHYS_BUTTONS_NUM + 4)  srcSection = str_sections[4];
		else if (i == PHYS_BUTTONS_NUM + 8) srcSection = str_sections[1];
		else if (i == PHYS_BUTTONS_NUM + 12) srcSection = str_sections[2];
		else if (i == PHYS_BUTTONS_NUM + 16) srcSection = str_sections[5];
		//Source  Action
		if (i < PHYS_BUTTONS_NUM) srcAction = str_btns[i];
		else if (i < PHYS_BUTTONS_NUM + 4)  srcAction = str_touch_zones[i - PHYS_BUTTONS_NUM];
		else if (i < PHYS_BUTTONS_NUM + 8)  srcAction = str_touch_zones[i - PHYS_BUTTONS_NUM-4];
		else if (i < PHYS_BUTTONS_NUM + 12) srcAction = str_analog_directions[i - PHYS_BUTTONS_NUM-8];
		else if (i < PHYS_BUTTONS_NUM + 16) srcAction = str_analog_directions[i - PHYS_BUTTONS_NUM-12];
		else if (i < PHYS_BUTTONS_NUM + 22) srcAction = str_gyro_directions[i - PHYS_BUTTONS_NUM-16];
		//Target Section
		if (profile_remap[i] < PHYS_BUTTONS_NUM) targetSection = str_sections[0];
		else if (profile_remap[i] == PHYS_BUTTONS_NUM)  targetSection = str_sections[6];
		else if (profile_remap[i] == PHYS_BUTTONS_NUM + 1)  targetSection = str_sections[7];
		else if (profile_remap[i] < PHYS_BUTTONS_NUM + 6)  targetSection = str_sections[1];
		else if (profile_remap[i] < PHYS_BUTTONS_NUM + 10)  targetSection = str_sections[2];
		else if (profile_remap[i] < PHYS_BUTTONS_NUM + 18)  targetSection = str_sections[3];
		else if (profile_remap[i] < PHYS_BUTTONS_NUM + 26)  targetSection = str_sections[4];
		//Target  Action
		if (profile_remap[i] < PHYS_BUTTONS_NUM) targetAction = str_btns[profile_remap[i]];
		else if (profile_remap[i] < PHYS_BUTTONS_NUM + 2)  targetAction = "";
		else if (profile_remap[i] < PHYS_BUTTONS_NUM + 6) targetAction = str_analog_directions[profile_remap[i] - PHYS_BUTTONS_NUM-2];
		else if (profile_remap[i] < PHYS_BUTTONS_NUM + 10) targetAction = str_analog_directions[profile_remap[i] - PHYS_BUTTONS_NUM-6];
		else if (profile_remap[i] < PHYS_BUTTONS_NUM + 14) targetAction = str_touch_zones[profile_remap[i] - PHYS_BUTTONS_NUM-10];
		else if (profile_remap[i] < PHYS_BUTTONS_NUM + 18) targetAction = str_touch_points[profile_remap[i] - PHYS_BUTTONS_NUM-14];
		else if (profile_remap[i] < PHYS_BUTTONS_NUM + 22) targetAction = str_touch_zones[profile_remap[i] - PHYS_BUTTONS_NUM-18];
		else if (profile_remap[i] < PHYS_BUTTONS_NUM + 26) targetAction = str_touch_points[profile_remap[i] - PHYS_BUTTONS_NUM-22];

		renderer_setColor(COLOR_HEADER);
		renderer_drawString(L_0, y += CHA_H, srcSection);
		
		if (i == cfg_i) renderer_setColor(COLOR_CURSOR);
		else if (profile_remap[i] == PHYS_BUTTONS_NUM) renderer_setColor(COLOR_DEFAULT);
		else if (profile_remap[i] == PHYS_BUTTONS_NUM + 1) renderer_setColor(COLOR_DANGER);
		else renderer_setColor(COLOR_ACTIVE);
		renderer_drawString(L_0 + CHA_W*11, y, srcAction);
		
		if (profile_remap[i] == PHYS_BUTTONS_NUM) renderer_setColor(COLOR_DEFAULT);
		else if (profile_remap[i] == PHYS_BUTTONS_NUM + 1) renderer_setColor(COLOR_DANGER);
		else renderer_setColor(COLOR_ACTIVE);
		if (profile_remap[i] != PHYS_BUTTONS_NUM)
			renderer_drawString(L_0 + CHA_W*21, y, " -> ");
		
		renderer_drawString(L_0 + CHA_W*25, y, targetSection);
		renderer_drawString(L_0 + CHA_W*36, y, targetAction);
		if (y + 60 > UI_HEIGHT) break;
	}
}

void drawMenu_analog(int avaliableEntries, int y){
	int ii = calcStartingIndex(cfg_i, ANALOG_MENU_NUM , avaliableEntries, BOTTOM_OFFSET);
	for (int i = ii; i < min(ii + avaliableEntries, ANALOG_MENU_NUM); i++) {			
		int8_t idx = menu_analog[i].idx;

		if (idx == HEADER_IDX){
			setColorHeader(cfg_i == i);
			renderer_drawString(L_1, y+=CHA_H, menu_analog[i].name);
		} else if (idx < 4){
			setColor(i == cfg_i, profile_analog[idx] == PROFILE_ANALOG_DEF[idx]);
			renderer_drawStringF(L_2, y += CHA_H, "%s: %hhu", 
					menu_analog[i].name, profile_analog[idx]);
		} else if (idx != HEADER_IDX) {
			setColor(i == cfg_i, profile_analog[idx] == PROFILE_ANALOG_DEF[idx]);
			renderer_drawStringF(L_2, y += CHA_H, "%s: %s", 
					menu_analog[i].name, str_yes_no[profile_analog[idx]]);
		}

		if (cfg_i == i && (idx != HEADER_IDX)){//Draw cursor
			renderer_setColor(COLOR_CURSOR);
			renderer_drawString(L_2 + 17*CHA_W, y, (ticker % 16 < 8) ? "<" : ">");
			//drawEditPointer(L_2 + 17*CHA_W, y + 2);
		}
	}
	drawFullScroll(ii > 0, ii + avaliableEntries < ANALOG_MENU_NUM, ((float)cfg_i) / (ANALOG_MENU_NUM-1));
}

void drawMenu_touch(int avaliableEntries, int y){
	int ii = calcStartingIndex(cfg_i, TOUCH_MENU_NUM , avaliableEntries, BOTTOM_OFFSET);
	for (int i = ii; i < min(ii + avaliableEntries, TOUCH_MENU_NUM); i++) {			
		int8_t idx = menu_touch[i].idx;

		if (idx == HEADER_IDX){
			setColorHeader(cfg_i == i);
			renderer_drawString(L_1, y+=CHA_H, menu_touch[i].name);
		} else if (idx == 16 || idx == 17){
			setColor(i == cfg_i, profile_touch[idx] == PROFILE_TOUCH_DEF[idx]);
			renderer_drawStringF(L_2, y += CHA_H, "%s: %s", 
					menu_touch[i].name, str_yes_no[profile_touch[idx]]);
		} else if (idx != HEADER_IDX) {
			setColor(i == cfg_i, profile_touch[idx] == PROFILE_TOUCH_DEF[idx]);
			renderer_drawStringF(L_2, y += CHA_H, "%s: %hu", 
					menu_touch[i].name, profile_touch[idx]);
		}

		if (cfg_i == i && (idx != HEADER_IDX)){//Draw cursor
			renderer_setColor(COLOR_CURSOR);
			renderer_drawString(L_2+ 20*CHA_W, y, (ticker % 16 < 8) ? "<" : ">");
		}
	}
	//drawScroll(ii > 0, ii + avaliableEntries < TOUCH_MENU_NUM);
	drawFullScroll(ii > 0, ii + avaliableEntries < TOUCH_MENU_NUM, ((float)cfg_i) / (TOUCH_MENU_NUM - 1));
}

void drawMenu_gyro(int avaliableEntries, int y){
	int ii = calcStartingIndex(cfg_i, GYRO_MENU_NUM , avaliableEntries, BOTTOM_OFFSET);
	for (int i = ii; i < min(ii + avaliableEntries, GYRO_MENU_NUM); i++) {		
		int8_t idx = menu_gyro[i].idx;
		
		if (idx == HEADER_IDX){
			setColorHeader(cfg_i == i);
			renderer_drawString(L_1, y+=CHA_H, menu_gyro[i].name);
		} else if (idx < 6){//Draw sens and deadzone option
			setColor(i == cfg_i, profile_gyro[idx] == PROFILE_GYRO_DEF[idx]);
			renderer_drawStringF(L_2, y += CHA_H, "%s: %hhu", menu_gyro[i].name, profile_gyro[idx]);
		} else if (idx == 6) {//Draw deadband option
			setColor(i == cfg_i, profile_gyro[idx] == PROFILE_GYRO_DEF[idx]);
			renderer_drawStringF(L_2, y += CHA_H, "%s: %s", menu_gyro[i].name, str_deadband[profile_gyro[idx]]);
		} else if (idx == 7) {//Draw wheel option
			setColor(i == cfg_i, profile_gyro[idx] == PROFILE_GYRO_DEF[idx]);
			renderer_drawStringF(L_2, y += CHA_H, "%s: %s", menu_gyro[i].name, str_yes_no[profile_gyro[idx]]);
		}

		if (cfg_i == i && (idx != HEADER_IDX)) {//Draw cursor
			renderer_setColor(COLOR_CURSOR);
			renderer_drawString(L_2 + 11 * CHA_W, y, (ticker % 16 < 8) ? "<" : ">");
		}
	}
	drawFullScroll(ii > 0, ii + avaliableEntries < GYRO_MENU_NUM, ((float)cfg_i)/(GYRO_MENU_NUM-1));
}

void drawMenu_controller(int avaliableEntries, int y){
	SceCtrlPortInfo pi;
	int res = ksceCtrlGetControllerPortInfo(&pi);
	if (res != 0){//Should not ever trigger
		renderer_setColor(COLOR_DANGER);
		renderer_drawString(L_1, y+= CHA_H, "Error getting controllers info");
		return;
	}
	
	int ii = calcStartingIndex(cfg_i, CONTROLLERS_MENU_NUM , avaliableEntries, BOTTOM_OFFSET);
	for (int i = ii; i < min(ii + avaliableEntries, CONTROLLERS_MENU_NUM); i++) {		
		int8_t idx = menu_controllers[i].idx;

		setColor(i == cfg_i, profile_controller[idx] == PROFILE_CONTROLLER_DEF[idx]);
		if (idx == 0 || idx == 2)//Use external controller / buttons swap
			renderer_drawStringF(L_1, y += CHA_H, "%s: %s", menu_controllers[i].name, str_yes_no[profile_controller[idx]]);
		else if (idx == 1){//Port selection
			renderer_drawStringF(L_1, y += CHA_H, "%s: {%i} %s", menu_controllers[i].name, profile_controller[idx],
					getControllerName(pi.port[profile_controller[1]]));
		}

		if (cfg_i == i && (idx != HEADER_IDX)) {//Draw cursor
			renderer_setColor(COLOR_CURSOR);
			renderer_drawString(L_1 + 20 * CHA_W, y, (ticker % 16 < 8) ? "<" : ">");
		}
	}

	//Ports stats
	y+=CHA_H;
	renderer_setColor(COLOR_HEADER);
	renderer_drawString(L_1, y+= CHA_H, "Detected controllers:");
	for (int i = 0; i < 5; i++){
		//setColor(0, pi.port[i] != SCE_CTRL_TYPE_UNPAIRED);
		renderer_setColor((pi.port[i] == SCE_CTRL_TYPE_UNPAIRED) ? COLOR_DANGER : COLOR_ACTIVE);
		renderer_drawStringF(L_2, y += CHA_H, "Port %i: %s", i, getControllerName(pi.port[i]));
	}
}

void drawMenu_hooks(int avaliableEntries, int y){
	int ii = calcStartingIndex(cfg_i, HOOKS_NUM - 2, avaliableEntries, avaliableEntries - 1);
	for (int i = ii; i < min(ii + avaliableEntries, HOOKS_NUM - 2); i++) {
		renderer_setColor((used_funcs[i] ? COLOR_ACTIVE : COLOR_DEFAULT));
		renderer_drawStringF(L_1, y += CHA_H, "%s : %s", str_funcs[i], str_yes_no[used_funcs[i]]);
	}
	drawFullScroll(ii > 0, ii + avaliableEntries < HOOKS_NUM - 2, 
			((float)cfg_i)/(HOOKS_NUM - 2 - (avaliableEntries - 1) - 1));
}

void drawMenu_settings(int avaliableEntries, int y){
	int ii = calcStartingIndex(cfg_i, SETTINGS_MENU_NUM , avaliableEntries, BOTTOM_OFFSET);
	for (int i = ii; i < min(ii + avaliableEntries, SETTINGS_MENU_NUM); i++) {		
		
		int8_t idx = menu_settings[i].idx;
		
		setColor(i == cfg_i, profile_settings[idx] == PROFILE_SETTINGS_DEF[idx]);
		if (idx < 2){//Draw opening buttons
			renderer_drawStringF(L_1, y += CHA_H, "%s: %s", menu_settings[i].name, str_btns[profile_settings[idx]]);
		} else if (idx == 2) {//Draw Save profile on close
			renderer_drawStringF(L_1, y += CHA_H, "%s: %s", menu_settings[i].name, str_yes_no[profile_settings[idx]]);
		} else if (idx == 3) {//Startup delay
			renderer_drawStringF(L_1, y += CHA_H, "%s: %hhu", menu_settings[i].name, profile_settings[idx]);
		}

		if (cfg_i == i && (idx != HEADER_IDX)) {//Draw cursor
			renderer_setColor(COLOR_CURSOR);
			renderer_drawString(L_1 + 23 * CHA_W, y, (ticker % 16 < 8) ? "<" : ">");
		}
	}
	drawFullScroll(ii > 0, ii + avaliableEntries < GYRO_MENU_NUM, ((float)cfg_i) / (SETTINGS_MENU_NUM-1));
}

void drawMenu_profiles(int avaliableEntries, int y){
	int ii = calcStartingIndex(cfg_i, PROFILE_MENU_NUM, avaliableEntries, BOTTOM_OFFSET);
	for (int i = ii; i < min(ii + avaliableEntries, PROFILE_MENU_NUM); i++) {
		if (menu_profiles[i].idx == HEADER_IDX){
			setColorHeader(cfg_i == i);
			renderer_drawString(L_1, y+=CHA_H, menu_profiles[i].name);
		} else {
			setColor(i == cfg_i, 1);
			renderer_drawString(L_2, y += CHA_H, menu_profiles[i].name);
		}
	}
}

void drawMenu_credits(int avaliableEntries, int y){
	int ii = calcStartingIndex(cfg_i, CREDITS_NUM, avaliableEntries, avaliableEntries - 1);
	for (int i = ii; i < min(CREDITS_NUM, ii + avaliableEntries); i++) {	
		renderer_setColor(COLOR_DEFAULT);
		renderer_drawString(L_0, y += CHA_H, str_credits[i]);
	}
	drawFullScroll(ii > 0, ii + avaliableEntries < CREDITS_NUM, 
			((float)cfg_i)/(CREDITS_NUM - (avaliableEntries - 1) - 1));
}

void drawTouchPointer(){
	int8_t idx = ui_opt.idx;
	if (menu_i != TOUCH_MENU || idx == HEADER_IDX ||  idx >= 16)
		return;
	int8_t ic_halfsize = ICN_TOUCH_X / 2;
	int left = profile_touch[idx - (idx % 2)] - 8;
	left *= (float)fbWidth / ((idx < 8) ? TOUCH_SIZE[0] : TOUCH_SIZE[2]);
	left = min((max(ic_halfsize, left)), fbWidth - ic_halfsize);
	int top = profile_touch[idx - (idx % 2) + 1] - 10;
	top *= (float)fbHeight / ((idx < 8) ? TOUCH_SIZE[1] : TOUCH_SIZE[3]); //Scale to framebuffer size
	top = min((max(ic_halfsize, top)), fbHeight - ic_halfsize);//limit into screen
	renderer_setColor((ticker % 8 < 4) ? COLOR_DANGER : COLOR_HEADER);
	renderer_drawImageDirectlyToFB(left - ic_halfsize, top - ic_halfsize, 64, 64, ICN_TOUCH);
}

void drawBody() {
	renderer_drawRectangle(0, HEADER_HEIGHT, UI_WIDTH, UI_HEIGHT -  2 * HEADER_HEIGHT, COLOR_BG_BODY);//BG
	//Draw menu
	int y = HEADER_HEIGHT - CHA_H / 2;
	int avaliableEntries = ((float)(UI_HEIGHT - 2 * HEADER_HEIGHT)) / CHA_H - 1;
	switch (menu_i){
		case MAIN_MENU: drawMenu_main(avaliableEntries, y); break;
		case REMAP_MENU: drawMenu_remap(avaliableEntries, y); break;
		case ANALOG_MENU: drawMenu_analog(avaliableEntries, y); break;
		case TOUCH_MENU: drawMenu_touch(avaliableEntries, y); break;
		case GYRO_MENU: drawMenu_gyro(avaliableEntries, y); break;
		case CNTRL_MENU: drawMenu_controller(avaliableEntries, y); break;
		case FUNCS_LIST: drawMenu_hooks(avaliableEntries, y); break;  
		case PROFILES_MENU: drawMenu_profiles(avaliableEntries, y); break; 
		case SETTINGS_MENU: drawMenu_settings(avaliableEntries, y); break; 
		case CREDITS_MENU: drawMenu_credits(avaliableEntries, y); break;                                                             
		default: break;
	}
}

void ui_setIdx(int i){
	cfg_i = i;
	switch (menu_i){
		case MAIN_MENU: ui_opt = menu_main[i]; break;
		case REMAP_MENU:  break;
		case ANALOG_MENU: ui_opt = menu_analog[i]; break;
		case TOUCH_MENU: ui_opt = menu_touch[i]; break;
		case GYRO_MENU: ui_opt = menu_gyro[i]; break;
		case CNTRL_MENU: ui_opt = menu_controllers[i]; break;
		case FUNCS_LIST: break;                                                               
		case PROFILES_MENU: ui_opt = menu_profiles[i]; break;   
		case SETTINGS_MENU: ui_opt = menu_settings[i]; break; 
		case CREDITS_MENU: break;                                                          
		default: break;
	}
}

//Draw directly to FB to overlay over everything else;
void drawDirectlyToFB(){
	drawTouchPointer();
}

void ui_draw(const SceDisplayFrameBuf *pParam){
	if (ui_opened) {
		new_frame = 1;
		ticker++;
		renderer_setFB(pParam);
		drawHeader();
		drawBody();
		drawFooter();
		renderer_writeToFB();
		drawDirectlyToFB();	
	}
}

void ui_open(const SceDisplayFrameBuf *pParam){
	ui_opened = 1;
	cfg_i = 0;
	ui_opt = menu_main[0];
}

void ui_init(){
	ui_opt = menu_main[0];
    renderer_init(UI_WIDTH, UI_HEIGHT);
}

void ui_destroy(){
	renderer_destroy();
}
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
#include "remap.h"

#define VERSION				"2.1.0"

#define UI_WIDTH            480
#define UI_HEIGHT           272
#define HEADER_HEIGHT		(CHA_H + 6)
#define STR_SIZE			30

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
#define L_0    				5		//Left margin for menu
#define L_1    				18		
#define L_2    				36

const char* str_btn_small[PHYS_BUTTONS_NUM] = {
	"X", "O", "T", "S", "-", "+", "L", "R", "^", ">", "<", "v", "l", "r", "{", "}"
};
const char* str_remap_type[REMAP_ACTION_TYPE_NUM] = {
	"BTN", "CMB", "LAN", "LAD", "RAN", "RAD", "FTZ", "BTZ", "FTP", "BTP", "GYR"
};
const char* str_remap_action[REMAP_ACTION_NUM] = {
	"UP", "DN", "LT", "RT", 
	"TL", "TR", "BL", "BR", "CU", 
	"UP", "DN", "LT", "RT", "RL", "RT"
};
static const char* str_yes_no[] = {
	"No", "Yes"
};
static const char* str_btns[PHYS_BUTTONS_NUM] = {
	"Cross", "Circle", "Triangle", "Square",
	"Start", "Select", 
	"LT/L2", "RT/R2",
	"Up", "Right", "Left", "Down", 
	"L1", "R1", "L3", "R3"
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

#define MENU_MAIN_NUM 9
static struct MenuEntry menu_main_entries[MENU_MAIN_NUM] = {
	(MenuEntry){.name = "Remap rules", .id = MENU_REMAP_ID},
	(MenuEntry){.name = "Analog sticks", .id = MENU_ANALOG_ID},
	(MenuEntry){.name = "Touch", .id = MENU_TOUCH_ID},
	(MenuEntry){.name = "Gyroscope", .id = MENU_GYRO_ID},
	(MenuEntry){.name = "External gamepads", .id = MENU_CONTROLLER_ID},
	(MenuEntry){.name = "[DEBUG] Show hooks", .id = MENU_HOKS_ID},
	(MenuEntry){.name = "Settings", .id = MENU_SETTINGS_ID},
	(MenuEntry){.name = "Profiles", .id = MENU_PROFILE_ID},
	(MenuEntry){.name = "Credits", .id = MENU_CREDITS_ID},};
static struct Menu menu_main = (Menu){
	.id = MENU_MAIN_ID, 
	.num = MENU_MAIN_NUM, 
	.name = "MAIN MENU",
	.entries = menu_main_entries};

#define MENU_ANALOG_NUM 10
static struct MenuEntry menu_analog_entries[MENU_ANALOG_NUM] = {
	(MenuEntry){.name = "Deadzone", .id = HEADER_IDX},
	(MenuEntry){.name = "Left Analog  [X]", .id = 0},
	(MenuEntry){.name = "             [Y]", .id = 1},
	(MenuEntry){.name = "Right Analog [X]", .id = 2},
	(MenuEntry){.name = "             [Y]", .id = 3},
	(MenuEntry){.name = "Force digital", .id = HEADER_IDX},
	(MenuEntry){.name = "Left Analog  [X]", .id = 4},
	(MenuEntry){.name = "             [Y]", .id = 5},
	(MenuEntry){.name = "Right Analog [X]", .id = 6},
	(MenuEntry){.name = "             [Y]", .id = 7}};
static struct Menu menu_analog = (Menu){
	.id = MENU_ANALOG_ID, 
	.parent = MENU_MAIN_ID,
	.num = MENU_ANALOG_NUM, 
	.name = "ANALOG STICKS", 
	.footer = "([]):reset  (start):reset all",
	.entries = menu_analog_entries};

#define MENU_TOUCH_NUM 20
static struct MenuEntry menu_touch_entries[MENU_TOUCH_NUM] = {
	(MenuEntry){.name = "Front", .id = HEADER_IDX},
	(MenuEntry){.name = "Point A           x", .id = 0},
	(MenuEntry){.name = "                  y", .id = 1},
	(MenuEntry){.name = "Point B           x", .id = 2},
	(MenuEntry){.name = "                  y", .id = 3},
	(MenuEntry){.name = "Point C           x", .id = 4},
	(MenuEntry){.name = "                  y", .id = 5},
	(MenuEntry){.name = "Point D           x", .id = 6},
	(MenuEntry){.name = "                  y", .id = 7},
	(MenuEntry){.name = "Disable if remapped", .id = 16},
	(MenuEntry){.name = "Rear", .id = HEADER_IDX},
	(MenuEntry){.name = "Point A           x", .id = 8},
	(MenuEntry){.name = "                  y", .id = 9},
	(MenuEntry){.name = "Point B           x", .id = 10},
	(MenuEntry){.name = "                  y", .id = 11},
	(MenuEntry){.name = "Point C           x", .id = 12},
	(MenuEntry){.name = "                  y", .id = 13},
	(MenuEntry){.name = "Point D           x", .id = 14},
	(MenuEntry){.name = "                  y", .id = 15},
	(MenuEntry){.name = "Disable if remapped", .id = 17}};
static struct Menu menu_touch = (Menu){
	.id = MENU_TOUCH_ID, 
	.parent = MENU_MAIN_ID,
	.num = MENU_TOUCH_NUM, 
	.name = "TOUCH", 
	.footer = "[TOUCH](RS):change  ([])/(start):reset",
	.entries = menu_touch_entries};

#define MENU_GYRO_NUM 11
static struct MenuEntry menu_gyro_entries[MENU_GYRO_NUM] = {
	(MenuEntry){.name = "Sensivity", .id = HEADER_IDX},
	(MenuEntry){.name = "    X Axis", .id = 0},
	(MenuEntry){.name = "    Y Axis", .id = 1},
	(MenuEntry){.name = "    Z Axis", .id = 2},
	(MenuEntry){.name = "Deadzone", .id = HEADER_IDX},
	(MenuEntry){.name = "    X Axis", .id = 3},
	(MenuEntry){.name = "    Y Axis", .id = 4},
	(MenuEntry){.name = "    Z Axis", .id = 5},
	(MenuEntry){.name = "More", .id = HEADER_IDX},
	(MenuEntry){.name = "Deadband  ", .id = 6},
	(MenuEntry){.name = "Wheel mode", .id = 7},};
static struct Menu menu_gyro = (Menu){
	.id = MENU_GYRO_ID, 
	.parent = MENU_MAIN_ID,
	.num = MENU_GYRO_NUM, 
	.name = "GYROSCOPE", 
	.footer = "([]):reset  (start):reset all",
	.entries = menu_gyro_entries};

#define MENU_CONTROLLER_NUM 3
static struct MenuEntry menu_controllers_entries[MENU_CONTROLLER_NUM] = {
	(MenuEntry){.name = "Use external       ", .id = 0},
	(MenuEntry){.name = "Selected controller", .id = 1},
	(MenuEntry){.name = "Swap L1<>LT R1<>RT ", .id = 2}};
static struct Menu menu_controllers = (Menu){
	.id = MENU_CONTROLLER_ID, 
	.parent = MENU_MAIN_ID,
	.num = MENU_CONTROLLER_NUM, 
	.name = "CONTROLLER", 
	.footer = "([]):reset  (start):reset all",
	.entries = menu_controllers_entries};

#define MENU_SETTINGS_NUM 4
static struct MenuEntry menu_settings_entries[MENU_SETTINGS_NUM] = {
	(MenuEntry){.name = "Menu trigger first key", .id = 0},
	(MenuEntry){.name = "            second key", .id = 1},
	(MenuEntry){.name = "Save profile on close ", .id = 2},
	(MenuEntry){.name = "Startup delay         ", .id = 3}};
static struct Menu menu_settings = (Menu){
	.id = MENU_SETTINGS_ID, 
	.parent = MENU_MAIN_ID,
	.num = MENU_SETTINGS_NUM, 
	.name = "SETTINGS", 
	.footer = "([]):reset  (start):reset all",
	.entries = menu_settings_entries};

#define MENU_PROFILE_NUM 8
static struct MenuEntry menu_profiles_entries[MENU_PROFILE_NUM] = {
	(MenuEntry){.name = "Global", .id = HEADER_IDX},
	(MenuEntry){.name = "Save", .id = PROFILE_GLOBAL_SAVE},
	(MenuEntry){.name = "Load", .id = PROFILE_GLOABL_LOAD},
	(MenuEntry){.name = "Delete", .id = PROFILE_GLOBAL_DELETE},
	(MenuEntry){.name = "Game", .id = HEADER_IDX},
	(MenuEntry){.name = "Save", .id = PROFILE_LOCAL_SAVE},
	(MenuEntry){.name = "Load", .id = PROFILE_LOCAL_LOAD},
	(MenuEntry){.name = "Delete", .id = PROFILE_LOCAL_DELETE}
};
static struct Menu menu_profiles = (Menu){
	.id = MENU_PROFILE_ID, 
	.parent = MENU_MAIN_ID,
	.num = MENU_PROFILE_NUM, 
	.name = "PROFILES", 
	.entries = menu_profiles_entries};

#define MENU_HOKS_NUM 18
static struct MenuEntry menu_hooks_entries[MENU_HOKS_NUM] = {
	(MenuEntry){.name = "sceCtrlPeekBufferPositive    ", .id = 0},
	(MenuEntry){.name = "sceCtrlPeekBufferPositive2   ", .id = 1},
	(MenuEntry){.name = "sceCtrlReadBufferPositive    ", .id = 2},
	(MenuEntry){.name = "sceCtrlReadBufferPositive2   ", .id = 3},
	(MenuEntry){.name = "sceCtrlPeekBufferPositiveExt ", .id = 4},
	(MenuEntry){.name = "sceCtrlPeekBufferPositiveExt2", .id = 5},
	(MenuEntry){.name = "sceCtrlReadBufferPositiveExt ", .id = 6},
	(MenuEntry){.name = "sceCtrlReadBufferPositiveExt2", .id = 7},
	(MenuEntry){.name = "sceCtrlPeekBufferNegative    ", .id = 8},
	(MenuEntry){.name = "sceCtrlPeekBufferNegative2   ", .id = 9},
	(MenuEntry){.name = "sceCtrlReadBufferNegative    ", .id =10},
	(MenuEntry){.name = "sceCtrlReadBufferNegative2   ", .id =11},    
	(MenuEntry){.name = "ksceCtrlReadBufferPositive   ", .id =12},
    (MenuEntry){.name = "ksceCtrlPeekBufferPositive   ", .id =13},
    (MenuEntry){.name = "ksceCtrlReadBufferNegative   ", .id =14},
    (MenuEntry){.name = "ksceCtrlPeekBufferNegative   ", .id =15},
	(MenuEntry){.name = "ksceTouchRead                ", .id =16},
	(MenuEntry){.name = "ksceTouchPeek                ", .id =17}
};
static struct Menu menu_hooks = (Menu){
	.id = MENU_HOKS_ID, 
	.parent = MENU_MAIN_ID,
	.num = MENU_HOKS_NUM, 
	.name = "HOOKS", 
	.entries = menu_hooks_entries};

#define MENU_CREDITS_NUM			16
static struct MenuEntry menu_credits_entries[MENU_CREDITS_NUM] = {
	(MenuEntry){.name = "                     updated by Mer1e "},
	(MenuEntry){.name = "               with the help of S1ngy "},
	(MenuEntry){.name = "       original author Rinnegatamante "},
	(MenuEntry){.name = ""},
	(MenuEntry){.name = "Thanks to"},
	(MenuEntry){.name = "  Cassie, W0lfwang, TheIronUniverse,"},
	(MenuEntry){.name = "  Kiiro Yakumo and mantixero"},
	(MenuEntry){.name = "     for enduring endless crashes"},
	(MenuEntry){.name = "         while testing this thing"},
	(MenuEntry){.name = "  Vita Nuova community"},
	(MenuEntry){.name = "    for all the help I got there"},
	(MenuEntry){.name = ""},
	(MenuEntry){.name = "Original Rinnegatamante's thanks to"},
	(MenuEntry){.name = "  Tain Sueiras, nobodywasishere and"},
	(MenuEntry){.name = "  RaveHeart for their awesome"},
	(MenuEntry){.name = "    support on Patreon"}
};
static struct Menu menu_credits = (Menu){
	.id = MENU_CREDITS_ID, 
	.parent = MENU_MAIN_ID,
	.num = MENU_CREDITS_NUM, 
	.name = "CREDITS", 
	.entries = menu_credits_entries};

#define MENU_REMAP_NUM 1
char str_remaps[REMAP_NUM + MENU_REMAP_NUM][STR_SIZE];
static struct MenuEntry menu_remap_entries_def[MENU_REMAP_NUM] = {
	(MenuEntry){.name = "<new remap rule>", .id = NEW_RULE_IDX}};
static struct MenuEntry menu_remap_entries[REMAP_NUM + MENU_REMAP_NUM];
static struct Menu menu_remap = (Menu){
	.id = MENU_REMAP_ID, 
	.parent = MENU_MAIN_ID,
	.num = 0, 
	.name = "REMAP RULES", 
	.footer = "([]):toggle (start):remove",
	.entries = menu_remap_entries};

#define MENU_PICK_BUTTON_NUM 16
static struct MenuEntry menu_pick_button_entries[MENU_PICK_BUTTON_NUM];
static struct Menu menu_pick_button = (Menu){
	.id = MENU_PICK_BUTTON_ID, 
	.parent = MENU_REMAP_TRIGGER_TYPE_ID,
	.num = MENU_PICK_BUTTON_NUM, 
	.name = "SELECT BUTTONS", 
	.footer = "([]):select (X):continue", 
	.entries = menu_pick_button_entries};

#define MENU_PICK_ANALOG_NUM 4
static struct MenuEntry menu_pick_analog_entries[MENU_PICK_ANALOG_NUM] = {
	(MenuEntry){.name = "Move left", .id = REMAP_ANALOG_LEFT},
	(MenuEntry){.name = "Move right", .id = REMAP_ANALOG_RIGHT},
	(MenuEntry){.name = "Move up", .id = REMAP_ANALOG_UP},
	(MenuEntry){.name = "Move down", .id = REMAP_ANALOG_DOWN}};
static struct Menu menu_pick_analog = (Menu){
	.id = MENU_PICK_BUTTON_ID, 
	.parent = MENU_REMAP_TRIGGER_TYPE_ID,
	.num = MENU_PICK_ANALOG_NUM, 
	.name = "SELECT ANALOG STICK DIRECTION", 
	.entries = menu_pick_analog_entries};

#define MENU_PICK_TOUCH_POINT_NUM 2
static struct MenuEntry menu_pick_touch_point_entries[MENU_PICK_TOUCH_POINT_NUM] = {
	(MenuEntry){.name = "Point x", .id = 0},
	(MenuEntry){.name = "      y", .id = 1}};
static struct Menu menu_pick_touch_point = (Menu){
	.id = MENU_PICK_TOUCH_POINT_ID, 
	.parent = MENU_REMAP_EMU_TYPE_ID,
	.num = MENU_PICK_TOUCH_POINT_NUM, 
	.name = "SELECT TOUCH POINT", 
	.entries = menu_pick_touch_point_entries};

#define MENU_PICK_TOUCH_ZONE_NUM 4
static struct MenuEntry menu_pick_touch_zone_entries[MENU_PICK_TOUCH_ZONE_NUM] = {
	(MenuEntry){.name = "Corner point 1 x", .id = 0},
	(MenuEntry){.name = "               y", .id = 1},
	(MenuEntry){.name = "Corner point 2 x", .id = 2},
	(MenuEntry){.name = "               y", .id = 3}};
static struct Menu menu_pick_touch_zone = (Menu){
	.id = MENU_PICK_TOUCH_ZONE_ID, 
	.parent = MENU_REMAP_TRIGGER_TYPE_ID,
	.num = MENU_PICK_TOUCH_ZONE_NUM, 
	.name = "SELECT TOUCH ZONE", 
	.entries = menu_pick_touch_zone_entries};

#define MENU_REMAP_TRIGGER_TYPE_NUM 6
static struct MenuEntry menu_remap_trigger_type_entries[MENU_REMAP_TRIGGER_TYPE_NUM] = {
	(MenuEntry){.name = "Buttons", .id = REMAP_TYPE_BUTTON},
	(MenuEntry){.name = "Analog Stick Left", .id = REMAP_TYPE_LEFT_ANALOG},
	(MenuEntry){.name = "Analog Stick Right", .id = REMAP_TYPE_RIGHT_ANALOG},
	(MenuEntry){.name = "Touch - Front Panel", .id = REMAP_TYPE_FRONT_TOUCH_ZONE},
	(MenuEntry){.name = "Touch - Back Panel", .id = REMAP_TYPE_BACK_TOUCH_ZONE},
	(MenuEntry){.name = "Gyroscope", .id = REMAP_TYPE_GYROSCOPE}
};
static struct Menu menu_remap_trigger_type = (Menu){
	.id = MENU_REMAP_TRIGGER_TYPE_ID, 
	.parent = MENU_REMAP_ID,
	.num = MENU_REMAP_TRIGGER_TYPE_NUM, 
	.name = "SELECT TRIGGER", 
	.entries = menu_remap_trigger_type_entries};

#define MENU_REMAP_TRIGGER_TOUCH_NUM 5
static struct MenuEntry menu_remap_trigger_touch_entries[MENU_REMAP_TRIGGER_TOUCH_NUM] = {
	(MenuEntry){.name = "Top Left Zone", .id = REMAP_TOUCH_ZONE_TL},
	(MenuEntry){.name = "Top Right Zone", .id = REMAP_TOUCH_ZONE_TR},
	(MenuEntry){.name = "Bottom Left Zone", .id = REMAP_TOUCH_ZONE_BL},
	(MenuEntry){.name = "Bottom Right Zone", .id = REMAP_TOUCH_ZONE_BR},
	(MenuEntry){.name = "Custom Zone", .id = REMAP_TOUCH_CUSTOM}
};
static struct Menu menu_remap_trigger_touch = (Menu){
	.id = MENU_REMAP_TRIGGER_TOUCH_ID, 
	.parent = MENU_REMAP_TRIGGER_TYPE_ID,
	.num = MENU_REMAP_TRIGGER_TOUCH_NUM, 
	.name = "SELECT TOUCH POINT", 
	.entries = menu_remap_trigger_touch_entries};

#define MENU_REMAP_TRIGGER_GYRO_NUM 6
static struct MenuEntry menu_remap_trigger_gyro_entries[MENU_REMAP_TRIGGER_GYRO_NUM] = {
	(MenuEntry){.name = "Move left", .id = REMAP_GYRO_LEFT},
	(MenuEntry){.name = "Move right", .id = REMAP_GYRO_RIGHT},
	(MenuEntry){.name = "Move up", .id = REMAP_GYRO_UP},
	(MenuEntry){.name = "Move down", .id = REMAP_GYRO_DOWN},
	(MenuEntry){.name = "Roll left", .id = REMAP_GYRO_ROLL_LEFT},
	(MenuEntry){.name = "Roll right", .id = REMAP_GYRO_ROLL_RIGHT}
};
static struct Menu menu_remap_trigger_gyro = (Menu){
	.id = MENU_REMAP_TRIGGER_GYRO_ID, 
	.parent = MENU_REMAP_TRIGGER_TYPE_ID,
	.num = MENU_REMAP_TRIGGER_GYRO_NUM, 
	.name = "SELECT GYRO MOVEMENT", 
	.entries = menu_remap_trigger_gyro_entries};

#define MENU_REMAP_EMU_TYPE_NUM 7
static struct MenuEntry menu_remap_emu_type_entries[MENU_REMAP_EMU_TYPE_NUM] = {
	(MenuEntry){.name = "Buttons", .id = REMAP_TYPE_BUTTON},
	(MenuEntry){.name = "Analog Stick Left", .id = REMAP_TYPE_LEFT_ANALOG},
	(MenuEntry){.name = "Analog Stick Left [DIGITAL]", .id = REMAP_TYPE_LEFT_ANALOG_DIGITAL},
	(MenuEntry){.name = "Analog Stick Right", .id = REMAP_TYPE_RIGHT_ANALOG},
	(MenuEntry){.name = "Analog Stick Right [DIGITAL]", .id = REMAP_TYPE_RIGHT_ANALOG_DIGITAL},
	(MenuEntry){.name = "Front Touch", .id = REMAP_TYPE_FRONT_TOUCH_POINT},
	(MenuEntry){.name = "Rear Touch", .id = REMAP_TYPE_BACK_TOUCH_POINT}
};
static struct Menu menu_remap_emu_type = (Menu){
	.id = MENU_REMAP_EMU_TYPE_ID, 
	.parent = MENU_REMAP_ID,
	.num = MENU_REMAP_EMU_TYPE_NUM, 
	.name = "SELECT EMU", 
	.entries = menu_remap_emu_type_entries};

#define MENU_REMAP_EMU_TOUCH_NUM 5
static struct MenuEntry menu_remap_emu_touch_entries[MENU_REMAP_EMU_TOUCH_NUM] = {
	(MenuEntry){.name = "Top Left Touch", .id = REMAP_TOUCH_ZONE_TL},
	(MenuEntry){.name = "Top Right Touch", .id = REMAP_TOUCH_ZONE_TR},
	(MenuEntry){.name = "Bottom Left Touch", .id = REMAP_TOUCH_ZONE_BL},
	(MenuEntry){.name = "Bottom Right Touch", .id = REMAP_TOUCH_ZONE_BR},
	(MenuEntry){.name = "Custom Touch", .id = REMAP_TOUCH_CUSTOM}};
static struct Menu menu_remap_emu_touch = (Menu){
	.id = MENU_REMAP_EMU_TOUCH_ID, 
	.parent = MENU_REMAP_EMU_TYPE_ID,
	.num = MENU_REMAP_EMU_TOUCH_NUM, 
	.name = "SELECT TOUCH POINT", 
	.entries = menu_remap_emu_touch_entries};

SceUID mem_uid;

uint8_t ui_opened = 0;
uint8_t ui_lines = 10;
uint8_t new_frame = 1;
static uint32_t ticker;

Menu* ui_menu;
MenuEntry* ui_entry;

struct RemapRule ui_ruleEdited; //Rule currently edited

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
	if (ui_menu->id == MENU_MAIN_ID){
		renderer_drawStringF(L_0, 3, "remaPSV2 v.%s", VERSION);
		renderer_drawString(UI_WIDTH - CHA_W * strlen(titleid) - 10, 2, titleid);
	} else	
		renderer_drawString(L_0, 3, ui_menu->name);
	
}
void drawFooter(){
	renderer_drawRectangle(0, UI_HEIGHT - HEADER_HEIGHT, UI_WIDTH, 1, COLOR_HEADER);//Separator
	renderer_drawRectangle(0, UI_HEIGHT - (HEADER_HEIGHT - 1), UI_WIDTH, HEADER_HEIGHT - 1, COLOR_BG_HEADER);//BG
	renderer_setColor(COLOR_HEADER);   
	if (ui_menu->footer)                                                            
		renderer_drawStringF(L_0, UI_HEIGHT-HEADER_HEIGHT + 4, ui_menu->footer);
}

void drawMenu_generic(int y){
	int ii = calcStartingIndex(ui_menu->idx, ui_menu->num, ui_lines, BOTTOM_OFFSET);
	for (int i = ii; i < min(ii + ui_lines, ui_menu->num); i++) {
		setColor(i == ui_menu->idx, 1);
		renderer_drawString(L_1, y += CHA_H, ui_menu->entries[i].name);
	}
}

void drawMenu_remap(int y){
	int ii = calcStartingIndex(ui_menu->idx, ui_menu->num, ui_lines, BOTTOM_OFFSET);
	for (int i = ii; i < min(ii + ui_lines, ui_menu->num); i++) {
		if (ui_menu->entries[i].id == NEW_RULE_IDX){
			setColor(i == ui_menu->idx, 1);
			renderer_drawString(L_1, y += CHA_H, ui_menu->entries[i].name);
		} else {
			setColor(i == ui_menu->idx, profile.remaps[ui_menu->entries[i].id].disabled);
			renderer_stripped(profile.remaps[ui_menu->entries[i].id].disabled);
			renderer_drawString(L_1, y += CHA_H, ui_menu->entries[i].name);
			renderer_stripped(0);
		}
	}
	drawFullScroll(ii > 0, ii + ui_lines < ui_menu->num, ((float)ui_menu->idx) / (ui_menu->num - 1));
}
void drawMenu_pickButton(int y){
	int ii = calcStartingIndex(ui_menu->idx, ui_menu->num, ui_lines, BOTTOM_OFFSET);
	for (int i = ii; i < min(ii + ui_lines, ui_menu->num); i++) {
		uint32_t btns = *(uint32_t*)ui_menu->data;
		setColor(i == ui_menu->idx, !btn_has(btns, HW_BUTTONS[ui_menu->entries[i].id]));
		renderer_drawString(L_1, y += CHA_H, ui_menu->entries[i].name);
	}
}
void drawMenu_analog(int y){
	int ii = calcStartingIndex(ui_menu->idx, ui_menu->num , ui_lines, BOTTOM_OFFSET);
	for (int i = ii; i < min(ii + ui_lines, ui_menu->num); i++) {			
		int8_t id = menu_analog_entries[i].id;

		if (id == HEADER_IDX){
			setColorHeader(ui_menu->idx == i);
			renderer_drawString(L_1, y+=CHA_H, menu_analog_entries[i].name);
		} else if (id < 4){
			setColor(i == ui_menu->idx, profile.analog[id] == profile_def.analog[id]);
			renderer_drawStringF(L_2, y += CHA_H, "%s: %hhu", 
					menu_analog_entries[i].name, profile.analog[id]);
		} else if (id != HEADER_IDX) {
			setColor(i == ui_menu->idx, profile.analog[id] == profile_def.analog[id]);
			renderer_drawStringF(L_2, y += CHA_H, "%s: %s", 
					menu_analog_entries[i].name, str_yes_no[profile.analog[id]]);
		}

		if (ui_menu->idx == i && (id != HEADER_IDX)){//Draw cursor
			renderer_setColor(COLOR_CURSOR);
			renderer_drawString(L_2 + 17*CHA_W, y, (ticker % 16 < 8) ? "<" : ">");
		}
	}
	drawFullScroll(ii > 0, ii + ui_lines < ui_menu->num, ((float)ui_menu->idx) / (ui_menu->num-1));
}
void drawMenu_touch(int y){
	int ii = calcStartingIndex(ui_menu->idx, ui_menu->num , ui_lines, BOTTOM_OFFSET);
	for (int i = ii; i < min(ii + ui_lines, ui_menu->num); i++) {			
		int8_t id = menu_touch_entries[i].id;

		if (id == HEADER_IDX){
			setColorHeader(ui_menu->idx == i);
			renderer_drawString(L_1, y+=CHA_H, menu_touch_entries[i].name);
		} else if (id == 16 || id == 17){
			setColor(i == ui_menu->idx, profile.touch[id] == profile_def.touch[id]);
			renderer_drawStringF(L_2, y += CHA_H, "%s: %s", 
					menu_touch_entries[i].name, str_yes_no[profile.touch[id]]);
		} else if (id != HEADER_IDX) {
			setColor(i == ui_menu->idx, profile.touch[id] == profile_def.touch[id]);
			renderer_drawStringF(L_2, y += CHA_H, "%s: %hu", 
					menu_touch_entries[i].name, profile.touch[id]);
		}

		if (ui_menu->idx == i && (id != HEADER_IDX)){//Draw cursor
			renderer_setColor(COLOR_CURSOR);
			renderer_drawString(L_2+ 20*CHA_W, y, (ticker % 16 < 8) ? "<" : ">");
		}
	}
	drawFullScroll(ii > 0, ii + ui_lines < ui_menu->num, ((float)ui_menu->idx) / (ui_menu->num - 1));
}
void drawMenu_pickTouchPoint(int y){
	int ii = calcStartingIndex(ui_menu->idx, ui_menu->num , ui_lines, BOTTOM_OFFSET);	
	RemapAction* ra = (RemapAction*)ui_menu->data;
	for (int i = ii; i < min(ii + ui_lines, ui_menu->num); i++) {	
		int8_t id = ui_menu->entries[i].id;
		int coord = (id == 0) ? ra->param.touch.x : ra->param.touch.y;
		setColor(i == ui_menu->idx, 1);
		//LOG("%i", coord);
		renderer_drawStringF(L_2, y += CHA_H, "%s: %hu", ui_menu->entries[i].name, coord);
		if (ui_menu->idx == i && (id != HEADER_IDX)){//Draw cursor
			renderer_setColor(COLOR_CURSOR);
			renderer_drawString(L_2+ 20*CHA_W, y, (ticker % 16 < 8) ? "<" : ">");
		}
	}
	drawFullScroll(ii > 0, ii + ui_lines < ui_menu->num, ((float)ui_menu->idx) / (ui_menu->num - 1));
}
void drawMenu_pickTouchZone(int y){
	int ii = calcStartingIndex(ui_menu->idx, ui_menu->num , ui_lines, BOTTOM_OFFSET);	
	RemapAction* ra = (RemapAction*)ui_menu->data;
	for (int i = ii; i < min(ii + ui_lines, ui_menu->num); i++) {	
		int8_t id = ui_menu->entries[i].id;
		int coord = 0;
		switch (id){
			case 0: coord = ra->param.zone.a.x; break;
			case 1: coord = ra->param.zone.a.y; break;
			case 2: coord = ra->param.zone.b.x; break;
			case 3: coord = ra->param.zone.b.y; break;}
		setColor(i == ui_menu->idx, 1);
		renderer_drawStringF(L_2, y += CHA_H, "%s: %hu", ui_menu->entries[i].name, coord);

		if (ui_menu->idx == i && (id != HEADER_IDX)){//Draw cursor
			renderer_setColor(COLOR_CURSOR);
			renderer_drawString(L_2+ 20*CHA_W, y, (ticker % 16 < 8) ? "<" : ">");
		}
	}
	drawFullScroll(ii > 0, ii + ui_lines < ui_menu->num, ((float)ui_menu->idx) / (ui_menu->num - 1));
}
void drawMenu_gyro(int y){
	int ii = calcStartingIndex(ui_menu->idx, ui_menu->num , ui_lines, BOTTOM_OFFSET);
	for (int i = ii; i < min(ii + ui_lines, ui_menu->num); i++) {		
		int8_t id = menu_gyro_entries[i].id;
		
		if (id == HEADER_IDX){
			setColorHeader(ui_menu->idx == i);
			renderer_drawString(L_1, y+=CHA_H, menu_gyro_entries[i].name);
		} else if (id < 6){//Draw sens and deadzone option
			setColor(i == ui_menu->idx, profile.gyro[id] == profile_def.gyro[id]);
			renderer_drawStringF(L_2, y += CHA_H, "%s: %hhu", menu_gyro_entries[i].name, profile.gyro[id]);
		} else if (id == 6) {//Draw deadband option
			setColor(i == ui_menu->idx, profile.gyro[id] == profile_def.gyro[id]);
			renderer_drawStringF(L_2, y += CHA_H, "%s: %s", menu_gyro_entries[i].name, str_deadband[profile.gyro[id]]);
		} else if (id == 7) {//Draw wheel option
			setColor(i == ui_menu->idx, profile.gyro[id] == profile_def.gyro[id]);
			renderer_drawStringF(L_2, y += CHA_H, "%s: %s", menu_gyro_entries[i].name, str_yes_no[profile.gyro[id]]);
		}

		if (ui_menu->idx == i && (id != HEADER_IDX)) {//Draw cursor
			renderer_setColor(COLOR_CURSOR);
			renderer_drawString(L_2 + 11 * CHA_W, y, (ticker % 16 < 8) ? "<" : ">");
		}
	}
	drawFullScroll(ii > 0, ii + ui_lines < ui_menu->num, ((float)ui_menu->idx)/(ui_menu->num-1));
}
void drawMenu_controller(int y){
	SceCtrlPortInfo pi;
	int res = ksceCtrlGetControllerPortInfo(&pi);
	if (res != 0){//Should not ever trigger
		renderer_setColor(COLOR_DANGER);
		renderer_drawString(L_1, y+= CHA_H, "Error getting controllers info");
		return;
	}
	
	int ii = calcStartingIndex(ui_menu->idx, ui_menu->num , ui_lines, BOTTOM_OFFSET);
	for (int i = ii; i < min(ii + ui_lines, ui_menu->num); i++) {		
		int8_t id = menu_controllers_entries[i].id;

		setColor(i == ui_menu->idx, profile.controller[id] == profile_def.controller[id]);
		if (id == 0 || id == 2)//Use external controller / buttons swap
			renderer_drawStringF(L_1, y += CHA_H, "%s: %s", menu_controllers_entries[i].name, str_yes_no[profile.controller[id]]);
		else if (id == 1){//Port selection
			renderer_drawStringF(L_1, y += CHA_H, "%s: {%i} %s", menu_controllers_entries[i].name, profile.controller[id],
					getControllerName(pi.port[profile.controller[1]]));
		}

		if (ui_menu->idx == i && (id != HEADER_IDX)) {//Draw cursor
			renderer_setColor(COLOR_CURSOR);
			renderer_drawString(L_1 + 20 * CHA_W, y, (ticker % 16 < 8) ? "<" : ">");
		}
	}

	//Ports stats
	y+=CHA_H;
	renderer_setColor(COLOR_HEADER);
	renderer_drawString(L_1, y+= CHA_H, "Detected controllers:");
	for (int i = 0; i < 5; i++){
		renderer_setColor((pi.port[i] == SCE_CTRL_TYPE_UNPAIRED) ? COLOR_DANGER : COLOR_ACTIVE);
		renderer_drawStringF(L_2, y += CHA_H, "Port %i: %s", i, getControllerName(pi.port[i]));
	}
}
void drawMenu_hooks(int y){
	int ii = calcStartingIndex(ui_menu->idx, ui_menu->num, ui_lines, ui_lines - 1);
	for (int i = ii; i < min(ii + ui_lines, ui_menu->num); i++) {
		renderer_setColor((used_funcs[ui_menu->entries[i].id] ? COLOR_ACTIVE : COLOR_DEFAULT));
		renderer_drawStringF(L_1, y += CHA_H, "%s : %s", ui_menu->entries[i].name, str_yes_no[used_funcs[ui_menu->entries[i].id]]);
	}
	drawFullScroll(ii > 0, ii + ui_lines < ui_menu->num, 
			((float)ui_menu->idx)/(ui_menu->num - (ui_lines - 1) - 1));
}
void drawMenu_settings(int y){
	int ii = calcStartingIndex(ui_menu->idx, ui_menu->num , ui_lines, BOTTOM_OFFSET);
	for (int i = ii; i < min(ii + ui_lines, ui_menu->num); i++) {		
		int8_t id = menu_settings_entries[i].id;
		
		setColor(i == ui_menu->idx, profile_settings[id] == profile_settings_def[id]);
		if (id < 2){//Draw opening buttons
			renderer_drawStringF(L_1, y += CHA_H, "%s: %s", menu_settings_entries[i].name, str_btns[profile_settings[id]]);
		} else if (id == 2) {//Draw Save profile on close
			renderer_drawStringF(L_1, y += CHA_H, "%s: %s", menu_settings_entries[i].name, str_yes_no[profile_settings[id]]);
		} else if (id == 3) {//Startup delay
			renderer_drawStringF(L_1, y += CHA_H, "%s: %hhu", menu_settings_entries[i].name, profile_settings[id]);
		}

		if (ui_menu->idx == i && (id != HEADER_IDX)) {//Draw cursor
			renderer_setColor(COLOR_CURSOR);
			renderer_drawString(L_1 + 23 * CHA_W, y, (ticker % 16 < 8) ? "<" : ">");
		}
	}
	drawFullScroll(ii > 0, ii + ui_lines < ui_menu->num, ((float)ui_menu->idx) / (ui_menu->num-1));
}
void drawMenu_profiles(int y){
	int ii = calcStartingIndex(ui_menu->idx, ui_menu->num, ui_lines, BOTTOM_OFFSET);
	for (int i = ii; i < min(ii + ui_lines, ui_menu->num); i++) {
		if (menu_profiles_entries[i].id == HEADER_IDX){
			setColorHeader(ui_menu->idx == i);
			renderer_drawString(L_1, y+=CHA_H, menu_profiles_entries[i].name);
		} else {
			setColor(i == ui_menu->idx, 1);
			renderer_drawString(L_2, y += CHA_H, menu_profiles_entries[i].name);
		}
	}
}
void drawMenu_credits(int y){
	int ii = calcStartingIndex(ui_menu->idx, ui_menu->num, ui_lines, ui_lines - 1);
	for (int i = ii; i < min(ui_menu->num, ii + ui_lines); i++) {	
		renderer_setColor(COLOR_DEFAULT);
		renderer_drawString(L_0, y += CHA_H, ui_menu->entries[i].name);
	}
	drawFullScroll(ii > 0, ii + ui_lines < ui_menu->num, 
			((float)ui_menu->idx)/(ui_menu->num - (ui_lines - 1) - 1));
}

void drawTouchPointer(uint32_t panel, TouchPoint* tp){
	int8_t ic_halfsize = ICN_TOUCH_X / 2;
	int left = tp->x - 8;
	left *= (float)fbWidth / ((panel == SCE_TOUCH_PORT_FRONT) ? TOUCH_SIZE[0] : TOUCH_SIZE[2]);
	left = min((max(ic_halfsize, left)), fbWidth - ic_halfsize);
	int top = tp->y - 10;
	top *= (float)fbHeight / ((panel == SCE_TOUCH_PORT_FRONT) ? TOUCH_SIZE[1] : TOUCH_SIZE[3]); //Scale to framebuffer size
	top = min((max(ic_halfsize, top)), fbHeight - ic_halfsize);//limit into screen
	renderer_setColor((ticker % 8 < 4) ? COLOR_DANGER : COLOR_HEADER);
	renderer_drawImageDirectlyToFB(left - ic_halfsize, top - ic_halfsize, 64, 64, ICN_TOUCH);
}

void drawTouchZone(uint32_t panel, TouchZone* tz){
	//ToDo draw rectangle
	drawTouchPointer(panel, &(TouchPoint){.x = tz->a.x, .y = tz->a.y});
	drawTouchPointer(panel, &(TouchPoint){.x = tz->b.x, .y = tz->b.y});
}

void drawBody() {
	renderer_drawRectangle(0, HEADER_HEIGHT, UI_WIDTH, UI_HEIGHT -  2 * HEADER_HEIGHT, COLOR_BG_BODY);//BG
	//Draw menu
	int y = HEADER_HEIGHT - CHA_H / 2;
	ui_lines = ((float)(UI_HEIGHT - 2 * HEADER_HEIGHT)) / CHA_H - 1;
	switch (ui_menu->id){
		case MENU_ANALOG_ID: drawMenu_analog(y); break;
		case MENU_TOUCH_ID: drawMenu_touch(y); break;
		case MENU_GYRO_ID: drawMenu_gyro(y); break;
		case MENU_CONTROLLER_ID: drawMenu_controller(y); break;
		case MENU_HOKS_ID: drawMenu_hooks(y); break;  
		case MENU_PROFILE_ID: drawMenu_profiles(y); break; 
		case MENU_SETTINGS_ID: drawMenu_settings(y); break; 
		case MENU_CREDITS_ID: drawMenu_credits(y); break; 
		case MENU_REMAP_ID: drawMenu_remap(y); break;
		case MENU_PICK_BUTTON_ID: drawMenu_pickButton(y); break;
		case MENU_PICK_TOUCH_POINT_ID: drawMenu_pickTouchPoint(y); break;
		case MENU_PICK_TOUCH_ZONE_ID: drawMenu_pickTouchZone(y); break;
		case MENU_MAIN_ID: 
		case MENU_REMAP_TRIGGER_TYPE_ID:
		case MENU_REMAP_TRIGGER_TOUCH_ID: 
		case MENU_REMAP_TRIGGER_GYRO_ID:                                                       
		case MENU_REMAP_EMU_TYPE_ID:
		case MENU_PICK_ANALOG_ID:
		case MENU_REMAP_EMU_TOUCH_ID: drawMenu_generic(y); break;
		default: break;
	}
}

void generateBtnComboName(char* str, uint32_t btns){
	int i = -1;
	char tmp[6] = "";
	while (++i < PHYS_BUTTONS_NUM && strlen(tmp) < 4)
		if (btn_has(btns, HW_BUTTONS[i])) 
			strcat(tmp, str_btn_small[i]);
	if (strlen(tmp) == 4)
		strcat(tmp, "~");
	strcat(str, tmp);
}
void generateRemapRuleName(char* str, struct RemapRule* ui_ruleEdited){
	strcpy(str, "");
	switch (ui_ruleEdited->trigger.type){
		case REMAP_TYPE_BUTTON :
		case REMAP_TYPE_COMBO :  
			strcat(str, str_remap_type[ui_ruleEdited->trigger.type]);
			strcat(str, "{");
			generateBtnComboName(str, ui_ruleEdited->trigger.param.btn);
			strcat(str, "}");
			break;
		default:
			strcat(str, str_remap_type[ui_ruleEdited->trigger.type]);
			strcat(str, "{");
			strcat(str, str_remap_action[ui_ruleEdited->trigger.action]);
			strcat(str, "}");
			break;
		}
	strcat(str, " -> ");
	switch (ui_ruleEdited->emu.type){
		case REMAP_TYPE_BUTTON :
		case REMAP_TYPE_COMBO :  
			strcat(str, str_remap_type[ui_ruleEdited->emu.type]);
			strcat(str, "{");
			generateBtnComboName(str, ui_ruleEdited->emu.param.btn);
			strcat(str, "}");
			break;
		default:
			strcat(str, str_remap_type[ui_ruleEdited->emu.type]);
			strcat(str, "{");
			strcat(str, str_remap_action[ui_ruleEdited->emu.action]);
			strcat(str, "}");
			break;
		}
}
void buildDynamicMenu(enum MENU_ID menuId){
	switch (menuId){
		case MENU_REMAP_ID:
			memset(&ui_ruleEdited, 0, sizeof(ui_ruleEdited));
			menu_remap.num = profile.remapsNum + MENU_REMAP_NUM;
			for (int i = 0; i < profile.remapsNum; i++){
				menu_remap_entries[i].id = i;
				generateRemapRuleName(menu_remap_entries[i].name, &profile.remaps[i]);
			}
			for (int i = 0; i < MENU_REMAP_NUM; i++){
				int idx = i + profile.remapsNum;
				menu_remap_entries[idx].id = menu_remap_entries_def[i].id;
				strcpy(menu_remap_entries[idx].name,
					menu_remap_entries_def[i].name);
			}
		break;
		default: break;
	}
}

void ui_setIdx(int idx){
	ui_menu->idx = idx;
	if (ui_menu->entries != NULL)
		ui_entry = &ui_menu->entries[idx];
}

struct Menu* findMenuById(enum MENU_ID menuId){
	switch (menuId){
		case MENU_MAIN_ID: return &menu_main;
		case MENU_ANALOG_ID: return &menu_analog;
		case MENU_TOUCH_ID: return &menu_touch;
		case MENU_GYRO_ID: return &menu_gyro;
		case MENU_CONTROLLER_ID: return &menu_controllers;
		case MENU_HOKS_ID: return &menu_hooks;                                                               
		case MENU_PROFILE_ID: return &menu_profiles;   
		case MENU_SETTINGS_ID: return &menu_settings; 
		case MENU_CREDITS_ID: return &menu_credits;
		case MENU_PICK_ANALOG_ID: return &menu_pick_button;
		case MENU_PICK_BUTTON_ID: return &menu_pick_analog;
		case MENU_PICK_TOUCH_POINT_ID: return &menu_pick_touch_point;
		case MENU_PICK_TOUCH_ZONE_ID: return &menu_pick_touch_zone;
		case MENU_REMAP_ID:  return &menu_remap;
		case MENU_REMAP_TRIGGER_TYPE_ID: return &menu_remap_trigger_type;
		case MENU_REMAP_TRIGGER_TOUCH_ID: return &menu_remap_trigger_touch;
		case MENU_REMAP_TRIGGER_GYRO_ID: return &menu_remap_trigger_gyro;
		case MENU_REMAP_EMU_TYPE_ID: return &menu_remap_emu_type;
		case MENU_REMAP_EMU_TOUCH_ID:  return &menu_remap_emu_touch;
		default: break;
	}
	return NULL;
}

void ui_openMenu(enum MENU_ID id){
	buildDynamicMenu(id);
	struct Menu* new_menu = findMenuById(id);
	new_menu->prev = ui_menu->id;
	ui_menu = new_menu;
	ui_setIdx(ui_menu->idx);
}
void ui_openMenuSmart(enum MENU_ID id, enum MENU_ID prevId, enum MENU_ID nextId, uint32_t data){
	buildDynamicMenu(id);
	ui_menu = findMenuById(id);
	ui_menu->next = nextId;
	ui_menu->prev = prevId;
	ui_menu->data = data;
	ui_setIdx(ui_menu->idx);
}
void ui_openMenuPrev(){
	buildDynamicMenu(ui_menu->prev);
	ui_menu = findMenuById(ui_menu->prev);
	ui_setIdx(ui_menu->idx);
}
void ui_openMenuNext(){
	buildDynamicMenu(ui_menu->next);
	ui_menu = findMenuById(ui_menu->next);
	ui_setIdx(ui_menu->idx);
}
void ui_openMenuParent(){
	buildDynamicMenu(ui_menu->parent);
	ui_menu = findMenuById(ui_menu->parent);
	ui_setIdx(ui_menu->idx);
}
void ui_nextEntry(){
	if (ui_menu->id == MENU_CREDITS_ID || ui_menu->id == MENU_HOKS_ID)
		ui_setIdx(min(ui_menu->idx + 1, ui_menu->num - ui_lines));
	else 
		ui_setIdx((ui_menu->idx + 1) % ui_menu->num);
}
void ui_prevEntry(){
	if (ui_menu->id == MENU_CREDITS_ID || ui_menu->id == MENU_HOKS_ID)
		ui_setIdx(max(0, ui_menu->idx - 1));
	else
		ui_setIdx((ui_menu->idx - 1 < 0) ? ui_menu->num - 1 : ui_menu->idx - 1);
}

//Draw directly to FB to overlay over everything else;
void drawDirectlyToFB(){
	RemapAction* ra;
	switch (ui_menu->id){
		case MENU_PICK_TOUCH_POINT_ID: 
		case MENU_PICK_TOUCH_ZONE_ID: 
			ra = (RemapAction*) ui_menu->data;
			uint32_t panel = (ra->type == REMAP_TYPE_FRONT_TOUCH_POINT || ra->type == REMAP_TYPE_FRONT_TOUCH_ZONE) ?
				SCE_TOUCH_PORT_FRONT : SCE_TOUCH_PORT_BACK;
			if (ui_menu->id == MENU_PICK_TOUCH_POINT_ID) drawTouchPointer(panel, &ra->param.touch); 
			else drawTouchZone(panel, &ra->param.zone); 
			break;
		default: break;
	}
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
	ui_menu = &menu_main;
	ui_setIdx(0);
	ui_setIdx(0);
	ui_opened = 1;
}
void ui_close(){
	ui_opened = 0;
}

void ui_init(){
	//Init Button remap menus with proper button names
	// for (int i = 0; i < REMAP_TRIGGER_BTN_SUB_NUM; i++)
	for (int i = 0; i < MENU_PICK_BUTTON_NUM; i++)
		menu_pick_button_entries[i] = (MenuEntry){.name = (char *)str_btns[i], .id = i};
	
	ui_menu = &menu_main;
	ui_setIdx(0);

	//Allocate memory for remap menu rule's names
	/*mem_uid = ksceKernelAllocMemBlock("ui_strings", SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_RW, 
		REMAP_NUM * STR_SIZE * sizeof(char), NULL);
	char* mem_pointer;
    ksceKernelGetMemBlockBase(mem_uid, (void**)&mem_pointer);
	for (int i = 0; i < REMAP_NUM; i++){
		menu_remap_entries[i].name = &mem_pointer[STR_SIZE * i];
	}*/
	for (int i = 0; i < REMAP_NUM + MENU_REMAP_NUM; i++){
		menu_remap_entries[i].name = &str_remaps[i][0];
	}

    renderer_init(UI_WIDTH, UI_HEIGHT);
}
void ui_destroy(){
	//ksceKernelFreeMemBlock(mem_uid);
	renderer_destroy();
}
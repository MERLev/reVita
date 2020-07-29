#include <vitasdkkern.h>

#include "main.h"
#include "ui.h"
#include "ui-control.h"
#include "ui-draw.h"
#include "profile.h"
#include "log.h"

#define STR_SIZE			30

Menu* menus[MENU_ID__NUM];
void onBuild_remap(Menu* m);

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
	.onButton = onButton_main,
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
	.onButton = onButton_analog,
	.onDraw = onDraw_analog,
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
	.onButton = onButton_touch,
	.onDraw = onDraw_touch,
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
	.onButton = onButton_gyro,
	.onDraw = onDraw_gyro,
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
	.onButton = onButton_controller,
	.onDraw = onDraw_controller,
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
	.onButton = onButton_settings,
	.onDraw = onDraw_settings,
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
	.onButton = onButton_profiles,
	.onDraw = onDraw_profiles,
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
	.onDraw = onDraw_hooks,
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
	.onDraw = onDraw_credits,
	.entries = menu_credits_entries};

#define MENU_PICK_BUTTON_NUM 16
static struct MenuEntry menu_pick_button_entries[MENU_PICK_BUTTON_NUM];
static struct Menu menu_pick_button = (Menu){
	.id = MENU_PICK_BUTTON_ID, 
	.parent = MENU_REMAP_TRIGGER_TYPE_ID,
	.num = MENU_PICK_BUTTON_NUM, 
	.name = "SELECT BUTTONS", 
	.footer = "([]):select (X):continue", 
	.onButton = onButton_pickButton,
	.onDraw = onDraw_pickButton,
	.entries = menu_pick_button_entries};

#define MENU_PICK_ANALOG_NUM 4
static struct MenuEntry menu_pick_analog_entries[MENU_PICK_ANALOG_NUM] = {
	(MenuEntry){.name = "Move left", .id = REMAP_ANALOG_LEFT},
	(MenuEntry){.name = "Move right", .id = REMAP_ANALOG_RIGHT},
	(MenuEntry){.name = "Move up", .id = REMAP_ANALOG_UP},
	(MenuEntry){.name = "Move down", .id = REMAP_ANALOG_DOWN}};
static struct Menu menu_pick_analog = (Menu){
	.id = MENU_PICK_ANALOG_ID, 
	.parent = MENU_REMAP_TRIGGER_TYPE_ID,
	.num = MENU_PICK_ANALOG_NUM, 
	.name = "SELECT ANALOG STICK DIRECTION", 
	.onButton = onButton_pickAnalog,
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
	.onInput = onInput_touchPicker,
	.onButton = onButton_pickTouchPoint,
	.onDraw = onDraw_pickTouchPoint,
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
	.onInput = onInput_touchPicker,
	.onButton = onButton_pickTouchZone,
	.onDraw = onDraw_pickTouchZone,
	.entries = menu_pick_touch_zone_entries};

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
	.onButton = onButton_remap,
	.onDraw = onDraw_remap,
	.onBuild = onBuild_remap,
	.entries = menu_remap_entries};

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
	.onButton = onButton_remapTriggerType,
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
	.onButton = onButton_remapTriggerTouch,
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
	.onButton = onButton_remapTriggerGyro,
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
	.onButton = onButton_remapEmuType,
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
	.onButton = onButton_remapEmuTouch,
	.entries = menu_remap_emu_touch_entries};

void registerMenu(Menu* m){
	menus[m->id] = m;
}
void registerMenus(){
	registerMenu(&menu_main);
	registerMenu(&menu_analog);
	registerMenu(&menu_touch);
	registerMenu(&menu_gyro);
	registerMenu(&menu_controllers);
	registerMenu(&menu_settings);
	registerMenu(&menu_credits);
	registerMenu(&menu_hooks);
	registerMenu(&menu_analog);
	registerMenu(&menu_profiles);

	registerMenu(&menu_pick_button);
	registerMenu(&menu_pick_analog);
	registerMenu(&menu_pick_touch_point);
	registerMenu(&menu_pick_touch_zone);

	registerMenu(&menu_remap);
	registerMenu(&menu_remap_trigger_type);
	registerMenu(&menu_remap_trigger_touch);
	registerMenu(&menu_remap_trigger_gyro);
	registerMenu(&menu_remap_emu_type);
	registerMenu(&menu_remap_emu_touch);
}

SceUID mem_uid;

uint8_t ui_opened = 0;
uint8_t ui_lines = 10;
uint8_t new_frame = 1;

Menu* ui_menu;
MenuEntry* ui_entry;

struct RemapRule ui_ruleEdited; //Rule currently edited

void onBuild_remap(Menu* m){
	//Allocate memory for remap menu rule's names
	for (int i = 0; i < REMAP_NUM + MENU_REMAP_NUM; i++){
		menu_remap_entries[i].name = &str_remaps[i][0];
	}

	memset(&ui_ruleEdited, 0, sizeof(ui_ruleEdited));
	m->num = profile.remapsNum + MENU_REMAP_NUM;
	for (int i = 0; i < profile.remapsNum; i++){
		m->entries[i].id = i;
		generateRemapRuleName(menu_remap_entries[i].name, &profile.remaps[i]);
	}
	for (int i = 0; i < MENU_REMAP_NUM; i++){
		int idx = i + profile.remapsNum;
		menu_remap_entries[idx].id = menu_remap_entries_def[i].id;
		strcpy(menu_remap_entries[idx].name,
			menu_remap_entries_def[i].name);
	}
}

void ui_setIdx(int idx){
	ui_menu->idx = idx;
	if (ui_menu->entries != NULL)
		ui_entry = &ui_menu->entries[idx];
}

void open(enum MENU_ID id){
	if (menus[id]->onBuild)
		menus[id]->onBuild(menus[id]);
	ui_menu = menus[id];
	ui_setIdx(ui_menu->idx);
}
void ui_openMenu(enum MENU_ID id){
	menus[id]->prev = ui_menu->id;
	open(id);
}
void ui_openMenuSmart(enum MENU_ID id, enum MENU_ID prevId, enum MENU_ID nextId, uint32_t data){
	menus[id]->data = data;
	menus[id]->next = nextId;
	menus[id]->prev = prevId;
	menus[id]->data = data;
	open(id);
}
void ui_openMenuPrev(){
	open(ui_menu->prev);
}
void ui_openMenuNext(){
	open(ui_menu->next);
}
void ui_openMenuParent(){
	open(ui_menu->parent);
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

void ui_onInput(SceCtrlData *ctrl){
	ctrl_onInput(ctrl);
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
    ui_control_init();
    ui_draw_init();

	//ToDo fix this not working
	//Init Button remap menus with proper button names
	for (int i = 0; i < MENU_PICK_BUTTON_NUM; i++)
		menu_pick_button_entries[i] = (MenuEntry){.name = (char *)&str_btns[i], .id = i};

	registerMenus();

	ui_menu = &menu_main;
	ui_setIdx(0);
}
void ui_destroy(){
	ui_control_destroy();
	ui_draw_destroy();
}
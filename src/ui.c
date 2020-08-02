#include <vitasdkkern.h>

#include "main.h"
#include "ui.h"
#include "ui-control.h"
#include "ui-draw.h"
#include "profile.h"
#include "log.h"

#define STR_SIZE 40

Menu* menus[MENU_ID__NUM];

void onBuild_remap(Menu* m);

#define MENU_MAIN_NUM 9
static struct MenuEntry menu_main_entries[MENU_MAIN_NUM] = {
	(MenuEntry){.name = "$X Remap rules", .data = MENU_REMAP_ID},
	(MenuEntry){.name = "$U Analog sticks", .data = MENU_ANALOG_ID},
	(MenuEntry){.name = "$F Touch", .data = MENU_TOUCH_ID},
	(MenuEntry){.name = "$Q Gyroscope", .data = MENU_GYRO_ID},
	(MenuEntry){.name = "   External gamepads", .data = MENU_CONTROLLER_ID},
	(MenuEntry){.name = "   Show hooks", .data = MENU_HOKS_ID},
	(MenuEntry){.name = "   Settings", .data = MENU_SETTINGS_ID},
	(MenuEntry){.name = "   Profiles", .data = MENU_PROFILE_ID},
	(MenuEntry){.name = "   Credits", .data = MENU_CREDITS_ID},};
static struct Menu menu_main = (Menu){
	.id = MENU_MAIN_ID, 
	.num = MENU_MAIN_NUM, 
	.name = "$P MAIN MENU",
	.footer = "$X$S$T$C$;$:$^$v$<$>$[${$,$]$}$.$p$P$+$-",
	.onButton = onButton_main,
	.entries = menu_main_entries};

#define MENU_ANALOG_NUM 10
static struct MenuEntry menu_analog_entries[MENU_ANALOG_NUM] = {
	(MenuEntry){.name = "Deadzone", .type = HEADER_TYPE},
	(MenuEntry){.name = "Left Analog  [X]", .data = PROFILE_ANALOG_LEFT_DEADZONE_X},
	(MenuEntry){.name = "             [Y]", .data = PROFILE_ANALOG_LEFT_DEADZONE_Y},
	(MenuEntry){.name = "Right Analog [X]", .data = PROFILE_ANALOG_RIGHT_DEADZONE_X},
	(MenuEntry){.name = "             [Y]", .data = PROFILE_ANALOG_RIGHT_DEADZONE_Y},
	(MenuEntry){.name = "Force digital", .type = HEADER_TYPE},
	(MenuEntry){.name = "Left Analog  [X]", .data = 4},
	(MenuEntry){.name = "             [Y]", .data = 5},
	(MenuEntry){.name = "Right Analog [X]", .data = 6},
	(MenuEntry){.name = "             [Y]", .data = 7}};
static struct Menu menu_analog = (Menu){
	.id = MENU_ANALOG_ID, 
	.parent = MENU_MAIN_ID,
	.num = MENU_ANALOG_NUM, 
	.name = "ANALOG STICKS", 
	.footer = "$Sreset  $:reset all",
	.onButton = onButton_analog,
	.onDraw = onDraw_analog,
	.entries = menu_analog_entries};

#define MENU_TOUCH_NUM 20
static struct MenuEntry menu_touch_entries[MENU_TOUCH_NUM] = {
	(MenuEntry){.name = "Front", .type = HEADER_TYPE},
	(MenuEntry){.name = "Point A           x", .data = 0},
	(MenuEntry){.name = "                  y", .data = 1},
	(MenuEntry){.name = "Point B           x", .data = 2},
	(MenuEntry){.name = "                  y", .data = 3},
	(MenuEntry){.name = "Point C           x", .data = 4},
	(MenuEntry){.name = "                  y", .data = 5},
	(MenuEntry){.name = "Point D           x", .data = 6},
	(MenuEntry){.name = "                  y", .data = 7},
	(MenuEntry){.name = "Disable if remapped", .data = 16},
	(MenuEntry){.name = "Back", .type = HEADER_TYPE},
	(MenuEntry){.name = "Point A           x", .data = 8},
	(MenuEntry){.name = "                  y", .data = 9},
	(MenuEntry){.name = "Point B           x", .data = 10},
	(MenuEntry){.name = "                  y", .data = 11},
	(MenuEntry){.name = "Point C           x", .data = 12},
	(MenuEntry){.name = "                  y", .data = 13},
	(MenuEntry){.name = "Point D           x", .data = 14},
	(MenuEntry){.name = "                  y", .data = 15},
	(MenuEntry){.name = "Disable if remapped", .data = 17}};
static struct Menu menu_touch = (Menu){
	.id = MENU_TOUCH_ID, 
	.parent = MENU_MAIN_ID,
	.num = MENU_TOUCH_NUM, 
	.name = "TOUCH", 
	.footer = "$F$B$U$uchange  $Sreset $:reset all",
	.onButton = onButton_touch,
	.onDraw = onDraw_touch,
	.entries = menu_touch_entries};

#define MENU_GYRO_NUM 11
static struct MenuEntry menu_gyro_entries[MENU_GYRO_NUM] = {
	(MenuEntry){.name = "Sensivity", .type = HEADER_TYPE},
	(MenuEntry){.name = "    X Axis", .data = 0},
	(MenuEntry){.name = "    Y Axis", .data = 1},
	(MenuEntry){.name = "    Z Axis", .data = 2},
	(MenuEntry){.name = "Deadzone", .type = HEADER_TYPE},
	(MenuEntry){.name = "    X Axis", .data = 3},
	(MenuEntry){.name = "    Y Axis", .data = 4},
	(MenuEntry){.name = "    Z Axis", .data = 5},
	(MenuEntry){.name = "More", .type = HEADER_TYPE},
	(MenuEntry){.name = "Deadband  ", .data = 6},
	(MenuEntry){.name = "Wheel mode", .data = 7},};
static struct Menu menu_gyro = (Menu){
	.id = MENU_GYRO_ID, 
	.parent = MENU_MAIN_ID,
	.num = MENU_GYRO_NUM, 
	.name = "GYROSCOPE", 
	.footer = "$Sreset  $:reset all",
	.onButton = onButton_gyro,
	.onDraw = onDraw_gyro,
	.entries = menu_gyro_entries};

#define MENU_CONTROLLER_NUM 3
static struct MenuEntry menu_controllers_entries[MENU_CONTROLLER_NUM] = {
	(MenuEntry){.name = "Use external       ", .data = 0},
	(MenuEntry){.name = "Selected controller", .data = 1},
	(MenuEntry){.name = "Swap L1<>LT R1<>RT ", .data = 2}};
static struct Menu menu_controllers = (Menu){
	.id = MENU_CONTROLLER_ID, 
	.parent = MENU_MAIN_ID,
	.num = MENU_CONTROLLER_NUM, 
	.name = "CONTROLLER", 
	.footer = "$Sreset  $:reset all",
	.onButton = onButton_controller,
	.onDraw = onDraw_controller,
	.entries = menu_controllers_entries};

#define MENU_SETTINGS_NUM 4
static struct MenuEntry menu_settings_entries[MENU_SETTINGS_NUM] = {
	(MenuEntry){.name = "Menu trigger first key", .data = 0},
	(MenuEntry){.name = "            second key", .data = 1},
	(MenuEntry){.name = "Save profile on close ", .data = 2},
	(MenuEntry){.name = "Startup delay         ", .data = 3}};
static struct Menu menu_settings = (Menu){
	.id = MENU_SETTINGS_ID, 
	.parent = MENU_MAIN_ID,
	.num = MENU_SETTINGS_NUM, 
	.name = "SETTINGS", 
	.footer = "$Sreset  $:reset all",
	.onButton = onButton_settings,
	.onDraw = onDraw_settings,
	.entries = menu_settings_entries};

#define MENU_PROFILE_NUM 8
static struct MenuEntry menu_profiles_entries[MENU_PROFILE_NUM] = {
	(MenuEntry){.name = "Global", .type = HEADER_TYPE},
	(MenuEntry){.name = "Save", .data = PROFILE_GLOBAL_SAVE},
	(MenuEntry){.name = "Load", .data = PROFILE_GLOABL_LOAD},
	(MenuEntry){.name = "Delete", .data = PROFILE_GLOBAL_DELETE},
	(MenuEntry){.name = "Game", .type = HEADER_TYPE},
	(MenuEntry){.name = "Save", .data = PROFILE_LOCAL_SAVE},
	(MenuEntry){.name = "Load", .data = PROFILE_LOCAL_LOAD},
	(MenuEntry){.name = "Delete", .data = PROFILE_LOCAL_DELETE}
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
	(MenuEntry){.name = "sceCtrlPeekBufferPositive    ", .data = 0},
	(MenuEntry){.name = "sceCtrlPeekBufferPositive2   ", .data = 1},
	(MenuEntry){.name = "sceCtrlReadBufferPositive    ", .data = 2},
	(MenuEntry){.name = "sceCtrlReadBufferPositive2   ", .data = 3},
	(MenuEntry){.name = "sceCtrlPeekBufferPositiveExt ", .data = 4},
	(MenuEntry){.name = "sceCtrlPeekBufferPositiveExt2", .data = 5},
	(MenuEntry){.name = "sceCtrlReadBufferPositiveExt ", .data = 6},
	(MenuEntry){.name = "sceCtrlReadBufferPositiveExt2", .data = 7},
	(MenuEntry){.name = "sceCtrlPeekBufferNegative    ", .data = 8},
	(MenuEntry){.name = "sceCtrlPeekBufferNegative2   ", .data = 9},
	(MenuEntry){.name = "sceCtrlReadBufferNegative    ", .data =10},
	(MenuEntry){.name = "sceCtrlReadBufferNegative2   ", .data =11},    
	(MenuEntry){.name = "ksceCtrlReadBufferPositive   ", .data =12},
    (MenuEntry){.name = "ksceCtrlPeekBufferPositive   ", .data =13},
    (MenuEntry){.name = "ksceCtrlReadBufferNegative   ", .data =14},
    (MenuEntry){.name = "ksceCtrlPeekBufferNegative   ", .data =15},
	(MenuEntry){.name = "ksceTouchRead                ", .data =16},
	(MenuEntry){.name = "ksceTouchPeek                ", .data =17}
};
static struct Menu menu_hooks = (Menu){
	.id = MENU_HOKS_ID, 
	.parent = MENU_MAIN_ID,
	.num = MENU_HOKS_NUM, 
	.name = "HOOKS", 
	.noIndent = true,
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
	.noIndent = true,
	.onDraw = onDraw_credits,
	.entries = menu_credits_entries};

#define MENU_PICK_BUTTON_NUM 20
static struct MenuEntry menu_pick_button_entries[MENU_PICK_BUTTON_NUM];
static struct Menu menu_pick_button = (Menu){
	.id = MENU_PICK_BUTTON_ID, 
	.parent = MENU_REMAP_TRIGGER_TYPE_ID,
	.num = MENU_PICK_BUTTON_NUM, 
	.name = "SELECT BUTTONS", 
	.footer = "$Sselect $Xcontinue", 
	.onButton = onButton_pickButton,
	.onDraw = onDraw_pickButton,
	.entries = menu_pick_button_entries};

#define MENU_PICK_ANALOG_LEFT_NUM 4
static struct MenuEntry menu_pick_analog_left_entries[MENU_PICK_ANALOG_LEFT_NUM] = {
	(MenuEntry){.name = "$L Move left", .data = REMAP_ANALOG_LEFT},
	(MenuEntry){.name = "$R Move right", .data = REMAP_ANALOG_RIGHT},
	(MenuEntry){.name = "$U Move up", .data = REMAP_ANALOG_UP},
	(MenuEntry){.name = "$D Move down", .data = REMAP_ANALOG_DOWN}};
static struct Menu menu_pick_analog_left = (Menu){
	.id = MENU_PICK_ANALOG_LEFT_ID, 
	.parent = MENU_REMAP_TRIGGER_TYPE_ID,
	.num = MENU_PICK_ANALOG_LEFT_NUM, 
	.name = "SELECT ANALOG STICK DIRECTION", 
	.onButton = onButton_pickAnalog,
	.entries = menu_pick_analog_left_entries};

#define MENU_PICK_ANALOG_RIGHT_NUM 4
static struct MenuEntry menu_pick_analog_right_entries[MENU_PICK_ANALOG_RIGHT_NUM] = {
	(MenuEntry){.name = "$l Move left", .data = REMAP_ANALOG_LEFT},
	(MenuEntry){.name = "$r Move right", .data = REMAP_ANALOG_RIGHT},
	(MenuEntry){.name = "$u Move up", .data = REMAP_ANALOG_UP},
	(MenuEntry){.name = "$d Move down", .data = REMAP_ANALOG_DOWN}};
static struct Menu menu_pick_analog_right = (Menu){
	.id = MENU_PICK_ANALOG_RIGHT_ID, 
	.parent = MENU_REMAP_TRIGGER_TYPE_ID,
	.num = MENU_PICK_ANALOG_RIGHT_NUM, 
	.name = "SELECT ANALOG STICK DIRECTION", 
	.onButton = onButton_pickAnalog,
	.entries = menu_pick_analog_right_entries};

#define MENU_PICK_TOUCH_POINT_NUM 2
static struct MenuEntry menu_pick_touch_point_entries[MENU_PICK_TOUCH_POINT_NUM] = {
	(MenuEntry){.name = "Point x", .data = 0},
	(MenuEntry){.name = "      y", .data = 1}};
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
	(MenuEntry){.name = "Corner point 1 x", .data = 0},
	(MenuEntry){.name = "               y", .data = 1},
	(MenuEntry){.name = "Corner point 2 x", .data = 2},
	(MenuEntry){.name = "               y", .data = 3}};
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
	(MenuEntry){.name = "<new remap rule>", .data = NEW_RULE_IDX}};
static struct MenuEntry menu_remap_entries[REMAP_NUM + MENU_REMAP_NUM];
static struct Menu menu_remap = (Menu){
	.id = MENU_REMAP_ID, 
	.parent = MENU_MAIN_ID,
	.num = 0, 
	.name = "REMAP RULES", 
	.footer = "$Stoggle $Tpropagate $:remove",
	.onButton = onButton_remap,
	.onDraw = onDraw_remap,
	.onBuild = onBuild_remap,
	.entries = menu_remap_entries};

#define MENU_REMAP_TRIGGER_TYPE_NUM 6
static struct MenuEntry menu_remap_trigger_type_entries[MENU_REMAP_TRIGGER_TYPE_NUM] = {
	(MenuEntry){.name = "$X Buttons", .data = REMAP_TYPE_BUTTON},
	(MenuEntry){.name = "$u Analog Stick Left", .data = REMAP_TYPE_LEFT_ANALOG},
	(MenuEntry){.name = "$U Analog Stick Right", .data = REMAP_TYPE_RIGHT_ANALOG},
	(MenuEntry){.name = "$F Touch - Front Panel", .data = REMAP_TYPE_FRONT_TOUCH_ZONE},
	(MenuEntry){.name = "$B Touch - Back Panel", .data = REMAP_TYPE_BACK_TOUCH_ZONE},
	(MenuEntry){.name = "$Q Gyroscope", .data = REMAP_TYPE_GYROSCOPE}
};
static struct Menu menu_remap_trigger_type = (Menu){
	.id = MENU_REMAP_TRIGGER_TYPE_ID, 
	.parent = MENU_REMAP_ID,
	.num = MENU_REMAP_TRIGGER_TYPE_NUM, 
	.name = "SELECT TRIGGER", 
	.onButton = onButton_remapTriggerType,
	.entries = menu_remap_trigger_type_entries};

#define MENU_REMAP_TRIGGER_TOUCH_FRONT_NUM 7
static struct MenuEntry menu_remap_trigger_front_touch_entries[MENU_REMAP_TRIGGER_TOUCH_FRONT_NUM] = {
	(MenuEntry){.name = "$1 Left Zone", .data = REMAP_TOUCH_ZONE_L},
	(MenuEntry){.name = "$2 Right Zone", .data = REMAP_TOUCH_ZONE_R},
	(MenuEntry){.name = "$3 Top Left Zone", .data = REMAP_TOUCH_ZONE_TL},
	(MenuEntry){.name = "$4 Top Right Zone", .data = REMAP_TOUCH_ZONE_TR},
	(MenuEntry){.name = "$5 Bottom Left Zone", .data = REMAP_TOUCH_ZONE_BL},
	(MenuEntry){.name = "$6 Bottom Right Zone", .data = REMAP_TOUCH_ZONE_BR},
	(MenuEntry){.name = "$7 Custom Zone", .data = REMAP_TOUCH_CUSTOM}
};
static struct Menu menu_remap_trigger_front_touch = (Menu){
	.id = MENU_REMAP_TRIGGER_TOUCH_FRONT_ID, 
	.parent = MENU_REMAP_TRIGGER_TYPE_ID,
	.num = MENU_REMAP_TRIGGER_TOUCH_FRONT_NUM, 
	.name = "SELECT TOUCH POINT", 
	.onButton = onButton_remapTriggerTouch,
	.entries = menu_remap_trigger_front_touch_entries};

#define MENU_REMAP_TRIGGER_TOUCH_BACK_NUM 7
static struct MenuEntry menu_remap_trigger_back_touch_entries[MENU_REMAP_TRIGGER_TOUCH_BACK_NUM] = {
	(MenuEntry){.name = "$1 Left Zone", .data = REMAP_TOUCH_ZONE_L},
	(MenuEntry){.name = "$2 Right Zone", .data = REMAP_TOUCH_ZONE_R},
	(MenuEntry){.name = "$3 Top Left Zone", .data = REMAP_TOUCH_ZONE_TL},
	(MenuEntry){.name = "$4 Top Right Zone", .data = REMAP_TOUCH_ZONE_TR},
	(MenuEntry){.name = "$5 Bottom Left Zone", .data = REMAP_TOUCH_ZONE_BL},
	(MenuEntry){.name = "$6 Bottom Right Zone", .data = REMAP_TOUCH_ZONE_BR},
	(MenuEntry){.name = "$7 Custom Zone", .data = REMAP_TOUCH_CUSTOM}
};
static struct Menu menu_remap_trigger_back_touch = (Menu){
	.id = MENU_REMAP_TRIGGER_TOUCH_BACK_ID, 
	.parent = MENU_REMAP_TRIGGER_TYPE_ID,
	.num = MENU_REMAP_TRIGGER_TOUCH_BACK_NUM, 
	.name = "SELECT TOUCH POINT", 
	.onButton = onButton_remapTriggerTouch,
	.entries = menu_remap_trigger_back_touch_entries};

#define MENU_REMAP_TRIGGER_GYRO_NUM 6
static struct MenuEntry menu_remap_trigger_gyro_entries[MENU_REMAP_TRIGGER_GYRO_NUM] = {
	(MenuEntry){.name = "$q Move left", .data = REMAP_GYRO_LEFT},
	(MenuEntry){.name = "$e Move right", .data = REMAP_GYRO_RIGHT},
	(MenuEntry){.name = "$w Move up", .data = REMAP_GYRO_UP},
	(MenuEntry){.name = "$s Move down", .data = REMAP_GYRO_DOWN},
	(MenuEntry){.name = "$Q Roll left", .data = REMAP_GYRO_ROLL_LEFT},
	(MenuEntry){.name = "$E Roll right", .data = REMAP_GYRO_ROLL_RIGHT}
};
static struct Menu menu_remap_trigger_gyro = (Menu){
	.id = MENU_REMAP_TRIGGER_GYRO_ID, 
	.parent = MENU_REMAP_TRIGGER_TYPE_ID,
	.num = MENU_REMAP_TRIGGER_GYRO_NUM, 
	.name = "SELECT GYRO MOVEMENT", 
	.onButton = onButton_remapTriggerGyro,
	.entries = menu_remap_trigger_gyro_entries};

#define MENU_REMAP_EMU_TYPE_NUM 9
static struct MenuEntry menu_remap_emu_type_entries[MENU_REMAP_EMU_TYPE_NUM] = {
	(MenuEntry){.name = "$X Buttons", .data = REMAP_TYPE_BUTTON},
	(MenuEntry){.name = "$U Analog Stick Left", .data = REMAP_TYPE_LEFT_ANALOG},
	(MenuEntry){.name = "$U Analog Stick Left [DIGITAL]", .data = REMAP_TYPE_LEFT_ANALOG_DIGITAL},
	(MenuEntry){.name = "$u Analog Stick Right", .data = REMAP_TYPE_RIGHT_ANALOG},
	(MenuEntry){.name = "$u Analog Stick Right [DIGITAL]", .data = REMAP_TYPE_RIGHT_ANALOG_DIGITAL},
	(MenuEntry){.name = "$F Front Touch", .data = REMAP_TYPE_FRONT_TOUCH_POINT},
	(MenuEntry){.name = "$B Back Touch", .data = REMAP_TYPE_BACK_TOUCH_POINT},
	(MenuEntry){.name = "   RemaPSV2", .data = REMAP_TYPE_REMAPSV},
	(MenuEntry){.name = "   Actions", .data = REMAP_TYPE_USYSACTIONS}
};
static struct Menu menu_remap_emu_type = (Menu){
	.id = MENU_REMAP_EMU_TYPE_ID, 
	.parent = MENU_REMAP_ID,
	.num = MENU_REMAP_EMU_TYPE_NUM, 
	.name = "SELECT EMU", 
	.onButton = onButton_remapEmuType,
	.entries = menu_remap_emu_type_entries};

#define MENU_REMAP_EMU_TOUCH_FRONT_NUM 7
static struct MenuEntry menu_remap_emu_touch_front_entries[MENU_REMAP_EMU_TOUCH_FRONT_NUM] = {
	(MenuEntry){.name = "$1 Left Touch", .data = REMAP_TOUCH_ZONE_L},
	(MenuEntry){.name = "$2 Right Touch", .data = REMAP_TOUCH_ZONE_R},
	(MenuEntry){.name = "$3 Top Left Touch", .data = REMAP_TOUCH_ZONE_TL},
	(MenuEntry){.name = "$4 Top Right Touch", .data = REMAP_TOUCH_ZONE_TR},
	(MenuEntry){.name = "$5 Bottom Left Touch", .data = REMAP_TOUCH_ZONE_BL},
	(MenuEntry){.name = "$6 Bottom Right Touch", .data = REMAP_TOUCH_ZONE_BR},
	(MenuEntry){.name = "$F Custom Touch", .data = REMAP_TOUCH_CUSTOM}};
static struct Menu menu_remap_emu_touch_front = (Menu){
	.id = MENU_REMAP_EMU_TOUCH_FRONT_ID, 
	.parent = MENU_REMAP_EMU_TYPE_ID,
	.num = MENU_REMAP_EMU_TOUCH_FRONT_NUM, 
	.name = "SELECT TOUCH POINT", 
	.onButton = onButton_remapEmuTouch,
	.entries = menu_remap_emu_touch_front_entries};

#define MENU_REMAP_EMU_TOUCH_BACK_NUM 7
static struct MenuEntry menu_remap_emu_touch_back_entries[MENU_REMAP_EMU_TOUCH_FRONT_NUM] = {
	(MenuEntry){.name = "$7 Left Touch", .data = REMAP_TOUCH_ZONE_L},
	(MenuEntry){.name = "$8 Right Touch", .data = REMAP_TOUCH_ZONE_R},
	(MenuEntry){.name = "$9 Top Left Touch", .data = REMAP_TOUCH_ZONE_TL},
	(MenuEntry){.name = "$0 Top Right Touch", .data = REMAP_TOUCH_ZONE_TR},
	(MenuEntry){.name = "$_ Bottom Left Touch", .data = REMAP_TOUCH_ZONE_BL},
	(MenuEntry){.name = "$= Bottom Right Touch", .data = REMAP_TOUCH_ZONE_BR},
	(MenuEntry){.name = "$B Custom Touch", .data = REMAP_TOUCH_CUSTOM}};
static struct Menu menu_remap_emu_touch_back = (Menu){
	.id = MENU_REMAP_EMU_TOUCH_BACK_ID, 
	.parent = MENU_REMAP_EMU_TYPE_ID,
	.num = MENU_REMAP_EMU_TOUCH_BACK_NUM, 
	.name = "SELECT TOUCH POINT", 
	.onButton = onButton_remapEmuTouch,
	.entries = menu_remap_emu_touch_back_entries};

void ui_fixIdx(Menu* m, int idx){
	for (int i = 0; i < m->num; i++)
		if(m->entries[(idx + i) % m->num].type != HEADER_TYPE){
			m->idx = (idx + i) % m->num;
			break;
		}
}

void registerMenu(Menu* m){
	menus[m->id] = m;
	ui_fixIdx(m, 0);
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
	registerMenu(&menu_pick_analog_left);
	registerMenu(&menu_pick_analog_right);
	registerMenu(&menu_pick_touch_point);
	registerMenu(&menu_pick_touch_zone);

	registerMenu(&menu_remap);
	registerMenu(&menu_remap_trigger_type);
	registerMenu(&menu_remap_trigger_front_touch);
	registerMenu(&menu_remap_trigger_back_touch);
	registerMenu(&menu_remap_trigger_gyro);
	registerMenu(&menu_remap_emu_type);
	registerMenu(&menu_remap_emu_touch_front);
	registerMenu(&menu_remap_emu_touch_back);
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
		m->entries[i].data = i;
		generateRemapRuleName(menu_remap_entries[i].name, &profile.remaps[i]);
	}
	for (int i = 0; i < MENU_REMAP_NUM; i++){
		int idx = i + profile.remapsNum;
		menu_remap_entries[idx].data = menu_remap_entries_def[i].data;
		strcpy(menu_remap_entries[idx].name,
			menu_remap_entries_def[i].name);
	}
}
void ui_setIdx(int idx){
	ui_menu->idx = idx;
	ui_fixIdx(ui_menu, idx);
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
	if (ui_menu->id == MENU_CREDITS_ID || ui_menu->id == MENU_HOKS_ID){
		ui_setIdx(max(0, ui_menu->idx - 1));
	} else {
		int idx = (ui_menu->num + ui_menu->idx - 1) % ui_menu->num;
		ui_menu->idx = idx;
		for (int i = 0; i < ui_menu->num; i++) //Search for prev non-header
			if(ui_menu->entries[(ui_menu->num + idx - i) % ui_menu->num].type != HEADER_TYPE){
				ui_menu->idx = (ui_menu->num + idx - i) % ui_menu->num;
				break;
			}
		if (ui_menu->entries != NULL)
			ui_entry = &ui_menu->entries[idx];
	}
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
		menu_pick_button_entries[i] = (MenuEntry){.name = (char *)&str_btns[i], .data = i};

	registerMenus();

	ui_menu = &menu_main;
	ui_setIdx(0);
}
void ui_destroy(){
	ui_control_destroy();
	ui_draw_destroy();
}
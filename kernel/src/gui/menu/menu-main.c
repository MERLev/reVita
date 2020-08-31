#include <vitasdkkern.h>
#include "../../main.h"
#include "../../fio/settings.h"
#include "../gui.h"

void onButton_main(uint32_t btn){
	switch (btn) {
		case SCE_CTRL_CROSS:
			gui_openMenu(gui_menu->entries[gui_menu->idx].dataUint);
			gui_setIdx(0);
			break;
		case SCE_CTRL_CIRCLE:
			gui_close();
			settings_save();
			if (settings[0].v.b)
				profile_save(titleid);
			remap_setup();
			break;
		case SCE_CTRL_SELECT:
			settings[SETT_REMAP_ENABLED].v.b = !settings[SETT_REMAP_ENABLED].v.b;
			break;
		default: onButton_generic(btn);
	}
}

#define MENU_MAIN_NUM 10
static struct MenuEntry menu_main_entries[MENU_MAIN_NUM] = {
	(MenuEntry){.name = "$X Remap rules", .dataUint = MENU_REMAP_ID},
	(MenuEntry){.name = "$U Analog sticks", .dataUint = MENU_ANALOG_ID},
	(MenuEntry){.name = "$F Touch", .dataUint = MENU_TOUCH_ID},
	(MenuEntry){.name = "$Q Gyroscope", .dataUint = MENU_GYRO_ID},
	(MenuEntry){.name = "$t External gamepads", .dataUint = MENU_CONTROLLER_ID},
	(MenuEntry){.name = "$b Show hooks", .dataUint = MENU_HOKS_ID},
	// (MenuEntry){.name = "$b Show buttons", .dataUint = MENU_DEBUG_BUTTONS_ID},
	(MenuEntry){.name = "$| Settings", .dataUint = MENU_SETT_ID},
	(MenuEntry){.name = "$| Hotkeys", .dataUint = MENU_HOTKEYS_ID},
	(MenuEntry){.name = "$/ Profiles", .dataUint = MENU_PROFILE_ID},
	(MenuEntry){.name = "$? Credits", .dataUint = MENU_CREDITS_ID},};
static struct Menu menu_main = (Menu){
	.id = MENU_MAIN_ID, 
	.num = MENU_MAIN_NUM, 
	.name = "$P MAIN MENU",
	.footer = "$;TOGGLE REMAPS                by Mer1e",
	.onButton = onButton_main,
	.entries = menu_main_entries};

void menu_initMain(){
	gui_registerMenu(&menu_main);
}
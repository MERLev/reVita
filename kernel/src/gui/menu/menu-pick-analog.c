#include <vitasdkkern.h>
#include "../../main.h"
#include "../../fio/settings.h"
#include "../gui.h"
#include "../renderer.h"

void onButton_pickAnalog(uint32_t btn){
	enum REMAP_ACTION* actn = (enum REMAP_ACTION*)gui_menu->data;
	switch (btn) {
		case SCE_CTRL_CROSS:
			*actn = gui_getEntry()->dataUint;
			if (gui_menu->next == MENU_REMAP_ID){
				if (ui_ruleEditedIdx >= 0) 
					profile.remaps[ui_ruleEditedIdx] = ui_ruleEdited;
				else
					profile_addRemapRule(ui_ruleEdited);
			}
			gui_openMenuNext();
			break;
		case SCE_CTRL_CIRCLE: gui_openMenuPrev(); break;
		default: onButton_generic(btn);
	}
}

#define MENU_PICK_ANALOG_LEFT_NUM 4
static struct MenuEntry menu_pick_analog_left_entries[MENU_PICK_ANALOG_LEFT_NUM] = {
	(MenuEntry){.name = "$L Move left", .dataUint = REMAP_ANALOG_LEFT},
	(MenuEntry){.name = "$R Move right", .dataUint = REMAP_ANALOG_RIGHT},
	(MenuEntry){.name = "$U Move up", .dataUint = REMAP_ANALOG_UP},
	(MenuEntry){.name = "$D Move down", .dataUint = REMAP_ANALOG_DOWN}};
static struct Menu menu_pick_analog_left = (Menu){
	.id = MENU_PICK_ANALOG_LEFT_ID, 
	.parent = MENU_REMAP_TRIGGER_TYPE_ID,
	.num = MENU_PICK_ANALOG_LEFT_NUM, 
	.name = "$L SELECT ANALOG STICK DIRECTION", 
	.onButton = onButton_pickAnalog,
	.entries = menu_pick_analog_left_entries};
	
#define MENU_PICK_ANALOG_RIGHT_NUM 4
static struct MenuEntry menu_pick_analog_right_entries[MENU_PICK_ANALOG_RIGHT_NUM] = {
	(MenuEntry){.name = "$l Move left", .dataUint = REMAP_ANALOG_LEFT},
	(MenuEntry){.name = "$r Move right", .dataUint = REMAP_ANALOG_RIGHT},
	(MenuEntry){.name = "$u Move up", .dataUint = REMAP_ANALOG_UP},
	(MenuEntry){.name = "$d Move down", .dataUint = REMAP_ANALOG_DOWN}};
static struct Menu menu_pick_analog_right = (Menu){
	.id = MENU_PICK_ANALOG_RIGHT_ID, 
	.parent = MENU_REMAP_TRIGGER_TYPE_ID,
	.num = MENU_PICK_ANALOG_RIGHT_NUM, 
	.name = "$l SELECT ANALOG STICK DIRECTION", 
	.onButton = onButton_pickAnalog,
	.entries = menu_pick_analog_right_entries};

void menu_initPickAnalog(){
	gui_registerMenu(&menu_pick_analog_left);
	gui_registerMenu(&menu_pick_analog_right);
}
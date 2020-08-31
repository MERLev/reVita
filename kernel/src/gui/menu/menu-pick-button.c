#include <vitasdkkern.h>
#include "../../common.h"
#include "../../fio/settings.h"
#include "../gui.h"
#include "../renderer.h"

const char* STR_BTN[HW_BUTTONS_NUM] = {
	"$X Cross", "$C Circle", "$T Triangle", "$S Square",
	"$: Start", "$; Select", 
	"$[ LT/L2", "$] RT/R2",
	"$^ Up", "$> Right", "$< Left", "$v Down", 
	"${ L1", "$} R1", "$( L3", "$) R3",
	"$+ Volume UP", "$- Volume DOWN", "$p POWER", "$P PS",
	"$t DS4 Touchpad"
};

void onButton_pickButton(uint32_t btn){
	uint32_t* btnP = (uint32_t *)gui_menu->data;
	switch (btn) {
		case SCE_CTRL_SQUARE:
			btn_toggle(btnP, HW_BUTTONS[gui_getEntry()->dataUint]);
			break;
		case SCE_CTRL_CROSS:
			if (!*btnP) break;
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

void onDraw_pickButton(unsigned int menuY){
    int y = menuY;
	int ii = gui_calcStartingIndex(gui_menu->idx, gui_menu->num, gui_lines, BOTTOM_OFFSET);
	uint32_t btns = *(uint32_t*)gui_menu->data;
	for (int i = ii; i < min(ii + gui_lines, gui_menu->num); i++) {
		gui_setColor(i == gui_menu->idx, !btn_has(btns, HW_BUTTONS[gui_menu->entries[i].dataUint]));
		renderer_drawString(L_1, y += CHA_H, STR_BTN[gui_menu->entries[i].dataUint]);
	}
}

#define MENU_PICK_BUTTON_NUM 21
static struct MenuEntry menu_pick_button_entries[MENU_PICK_BUTTON_NUM];
static struct Menu menu_pick_button = (Menu){
	.id = MENU_PICK_BUTTON_ID, 
	.parent = MENU_REMAP_TRIGGER_TYPE_ID,
	.num = MENU_PICK_BUTTON_NUM, 
	.name = "SELECT BUTTONS", 
	.footer = "$SSELECT $XCONTINUE", 
	.onButton = onButton_pickButton,
	.onDraw = onDraw_pickButton,
	.entries = menu_pick_button_entries};

struct Menu* menu_createPickButton(){

    return &menu_pick_button;
}

void menu_initPickButton(){
	//ToDo fix this not working
	//Init Button remap menus with proper button names
	for (int i = 0; i < MENU_PICK_BUTTON_NUM; i++)
		menu_pick_button_entries[i] = (MenuEntry){.name = (char *)&STR_BTN[i], .dataUint = i};

	gui_registerMenu(&menu_pick_button);
}
#include <vitasdkkern.h>
#include "../../vitasdkext.h"
#include "../../common.h"
#include "../../fio/settings.h"
#include "../gui.h"
#include "../renderer.h"
#include "../rendererv.h"
#include "menu.h"

void onButton_pickButton(uint32_t btn){
	uint32_t* btnP = gui_menu->dataPtr;
	switch (btn) {
		case SCE_CTRL_SQUARE:
			btn_toggle(btnP, gui_getEntry()->dataUint);
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
		case SCE_CTRL_TRIANGLE: 
			*btnP = 0;
			break;
		default: onButton_generic(btn);
	}
}

void onDraw_pickButton(uint menuY){
    int y = menuY;
	int ii = gui_calcStartingIndex(gui_menu->idx, gui_menu->num, gui_lines, BOTTOM_OFFSET);
	uint32_t btns = *(uint32_t*)gui_menu->dataPtr;
	for (int i = ii; i < min(ii + gui_lines, gui_menu->num); i++) {
		MenuEntry* me = &gui_menu->entries[i];
		gui_setColor(i == gui_menu->idx, !btn_has(btns, me->dataUint));
		rendererv_drawCharIcon(me->icn, L_1, y += CHA_H);
		rendererv_drawString(L_1 + 3*CHA_W, y, me->name);
	}
	gui_drawFullScroll(ii > 0, ii + gui_lines < gui_menu->num, ((float)gui_menu->idx)/(gui_menu->num-1));
}

void onDrawHeader_pickButton(){
	char str[21] = "\0";
	gui_generateBtnComboName(str, *(uint32_t*)gui_menu->dataPtr, 12);
	rendererv_drawStringF(L_0, 3, "%s: %s", gui_menu->name, str);
}

static struct MenuEntry menu_pick_button_entries[] = {
	(MenuEntry){.name = "Cross", 	.icn = ICON_BTN_CROSS, 		.dataUint = SCE_CTRL_CROSS},
	(MenuEntry){.name = "Circle", 	.icn = ICON_BTN_CIRCLE, 	.dataUint = SCE_CTRL_CIRCLE},
	(MenuEntry){.name = "Triangle", .icn = ICON_BTN_TRIANGLE, 	.dataUint = SCE_CTRL_TRIANGLE},
	(MenuEntry){.name = "Square", 	.icn = ICON_BTN_SQUARE, 	.dataUint = SCE_CTRL_SQUARE},
	(MenuEntry){.name = "Up", 		.icn = ICON_BTN_UP, 		.dataUint = SCE_CTRL_UP},
	(MenuEntry){.name = "Down", 	.icn = ICON_BTN_DONW, 		.dataUint = SCE_CTRL_DOWN},
	(MenuEntry){.name = "Left", 	.icn = ICON_BTN_LEFT, 		.dataUint = SCE_CTRL_LEFT},
	(MenuEntry){.name = "Right", 	.icn = ICON_BTN_RIGHT, 		.dataUint = SCE_CTRL_RIGHT},
	(MenuEntry){.name = "Start", 	.icn = ICON_BTN_START, 		.dataUint = SCE_CTRL_START},
	(MenuEntry){.name = "Select", 	.icn = ICON_BTN_SELECT, 	.dataUint = SCE_CTRL_SELECT},
	(MenuEntry){.name = "L1", 		.icn = ICON_BTN_L1, 		.dataUint = SCE_CTRL_L1},
	(MenuEntry){.name = "R1", 		.icn = ICON_BTN_R1, 		.dataUint = SCE_CTRL_R1},
	(MenuEntry){.name = "L2", 		.icn = ICON_BTN_L2, 		.dataUint = SCE_CTRL_L2},
	(MenuEntry){.name = "R2", 		.icn = ICON_BTN_R2, 		.dataUint = SCE_CTRL_R2},
	(MenuEntry){.name = "L3", 		.icn = ICON_BTN_L3, 		.dataUint = SCE_CTRL_L3},
	(MenuEntry){.name = "R3", 		.icn = ICON_BTN_R3, 		.dataUint = SCE_CTRL_R3},
	(MenuEntry){.name = "Vol Up", 	.icn = ICON_BTN_VOLUP, 		.dataUint = SCE_CTRL_VOLUP},
	(MenuEntry){.name = "Vol Down", .icn = ICON_BTN_VOLDOWN, 	.dataUint = SCE_CTRL_VOLDOWN},
	(MenuEntry){.name = "Power", 	.icn = ICON_BTN_POWER, 		.dataUint = SCE_CTRL_POWER},
	(MenuEntry){.name = "PS", 		.icn = ICON_BTN_PS, 		.dataUint = SCE_CTRL_PSBUTTON},
	(MenuEntry){.name = "DS4 Touch",.icn = ICON_BTN_DS4TOUCH, 	.dataUint = SCE_CTRL_TOUCHPAD},
	(MenuEntry){.name = "Headphone",.icn = ICON_HEADPHONES, 	.dataUint = SCE_CTRL_HEADPHONE}};
static struct Menu menu_pick_button = (Menu){
	.id = MENU_PICK_BUTTON_ID, 
	.parent = MENU_REMAP_TRIGGER_TYPE_ID,
	.name = "BUTTONS", 
	.footer = 	"$SSELECT $XCONTINUE $TCLEAR ALL        "
				"$CBACK                          $:CLOSE",
	.onButton = onButton_pickButton,
	.onDraw = onDraw_pickButton,
	.onDrawHeader = onDrawHeader_pickButton,
	.num = SIZE(menu_pick_button_entries), 
	.entries = menu_pick_button_entries};

void menu_initPickButton(){
	gui_registerMenu(&menu_pick_button);
}
#include <vitasdkkern.h>
#include "../../main.h"
#include "../../common.h"
#include "../../fio/settings.h"
#include "../gui.h"
#include "../renderer.h"

void onButton_touch(uint32_t btn){
	switch (btn) {
		case SCE_CTRL_START: profile_resetTouch(); break;
		default: onButton_genericEntries(btn);
	}
}

void onDraw_touch(unsigned int menuY){
    int y = menuY;
	int ii = gui_calcStartingIndex(gui_menu->idx, gui_menu->num , gui_lines, BOTTOM_OFFSET);
	for (int i = ii; i < min(ii + gui_lines, gui_menu->num); i++) {		
		gui_drawEntry(L_1, y+= CHA_H, &gui_menu->entries[i], gui_menu->idx == i);
	}
	gui_drawFullScroll(ii > 0, ii + gui_lines < gui_menu->num, ((float)gui_menu->idx) / (gui_menu->num - 1));
}

#define MENU_TOUCH_NUM 8
static struct MenuEntry menu_touch_entries[MENU_TOUCH_NUM] = {
	(MenuEntry){.name = "Swipe duration", 		.icn = ICON_SWIPE,	.dataPE = &profile.entries[PR_TO_SWIPE_DURATION]},
	(MenuEntry){.name = "Smart Swipe sensivity",.icn = ICON_SWIPE,	.dataPE = &profile.entries[PR_TO_SWIPE_SMART_SENS]},
	(MenuEntry){.name = "Swap touchpads", 		.icn = ICON_BT,		.dataPE = &profile.entries[PR_TO_SWAP]},
	(MenuEntry){.name = "Draw Pointer", 		.type = HEADER_TYPE},
	(MenuEntry){.name = "Touch Point", 			.icn = ICON_TOUCH,	.dataPE = &profile.entries[PR_TO_DRAW_POINT]},
	(MenuEntry){.name = "Swipe", 				.icn = ICON_SWIPE,	.dataPE = &profile.entries[PR_TO_DRAW_SWIPE]},
	(MenuEntry){.name = "Smart Swipe", 			.icn = ICON_SWIPE,	.dataPE = &profile.entries[PR_TO_DRAW_SMART_SWIPE]},
	(MenuEntry){.name = "Native", 				.icn = ICON_SWIPE,	.dataPE = &profile.entries[PR_TO_DRAW_NATIVE]}};
static struct Menu menu_touch = (Menu){
	.id = MENU_TOUCH_ID, 
	.parent = MENU_MAIN_ID,
	.num = MENU_TOUCH_NUM, 
	.name = "$F TOUCH", 
	.footer = "$SRESET  $:RESET ALL",
	.onButton = onButton_touch,
	.onDraw = onDraw_touch,
	.entries = menu_touch_entries};

void menu_initTouch(){
	gui_registerMenu(&menu_touch);
}
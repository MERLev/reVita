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
		int32_t id = gui_menu->entries[i].data;

		if (gui_menu->entries[i].type == HEADER_TYPE){
			gui_setColorHeader(gui_menu->idx == i);
			renderer_drawString(L_1, y+=CHA_H, gui_menu->entries[i].name);
			continue;
		}
		
		gui_setColor(i == gui_menu->idx, profile_isDef(id));
		renderer_drawString(L_1, y += CHA_H, gui_menu->entries[i].name);
		switch(id){
			case PR_TO_SWAP:
			case PR_TO_PSTV_MODE:
			case PR_TO_DRAW_POINT:
			case PR_TO_DRAW_SWIPE:
			case PR_TO_DRAW_SMART_SWIPE:
				gui_drawStringFRight(0, y, "%s", STR_YN[profile.entries[id].v.b]);
				break;
			case PR_TO_SWIPE_DURATION:
			case PR_TO_SWIPE_SMART_SENS:
				gui_drawStringFRight(0, y, "%u", profile.entries[id].v.u);
				break;
			default: break;
		}
	}
	gui_drawFullScroll(ii > 0, ii + gui_lines < gui_menu->num, ((float)gui_menu->idx) / (gui_menu->num - 1));
}

#define MENU_TOUCH_NUM 8
static struct MenuEntry menu_touch_entries[MENU_TOUCH_NUM] = {
	(MenuEntry){.name = "$V PSTV mode", .data = PR_TO_PSTV_MODE},
	(MenuEntry){.name = "$j Swipe duration", .data = PR_TO_SWIPE_DURATION},
	(MenuEntry){.name = "$j Smart Swipe sensivity", .data = PR_TO_SWIPE_SMART_SENS},
	(MenuEntry){.name = "$B Swap touchpads", .data = PR_TO_SWAP},
	(MenuEntry){.name = "Draw Pointer", .type = HEADER_TYPE},
	(MenuEntry){.name = "$i Touch Point", .data = PR_TO_DRAW_POINT},
	(MenuEntry){.name = "$j Swipe", .data = PR_TO_DRAW_SWIPE},
	(MenuEntry){.name = "$j Smart Swipe", .data = PR_TO_DRAW_SMART_SWIPE}};
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
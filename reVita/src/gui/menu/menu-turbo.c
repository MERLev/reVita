#include <vitasdkkern.h>
#include "../../main.h"
#include "../../common.h"
#include "../../fio/settings.h"
#include "../gui.h"
#include "../renderer.h"
#include "../rendererv.h"

void onButton_turbo(uint32_t btn){
	switch (btn) {
		case SCE_CTRL_SELECT: profile_resetTurbo(); break;
		default: onButton_genericEntries(btn);
	}
}

void onDraw_turbo(uint menuY){
    int y = menuY;
	int ii = gui_calcStartingIndex(gui_menu->idx, gui_menu->num , gui_lines, BOTTOM_OFFSET);
	for (int i = ii; i < min(ii + gui_lines, gui_menu->num); i++) {			
		gui_drawEntry(L_1, y+= CHA_H, &gui_menu->entries[i], gui_menu->idx == i);
	}
	gui_drawFullScroll(ii > 0, ii + gui_lines < gui_menu->num, ((float)gui_menu->idx) / (gui_menu->num-1));
}

static struct MenuEntry menu_turbo_entries[] = {
	(MenuEntry){.name = "Slow", 	.icn = ICON_TURBO_SLOW,  	.dataPE = &profile.entries[PR_TU_SLOW]},
	(MenuEntry){.name = "Medium", 	.icn = ICON_TURBO_MED,  	.dataPE = &profile.entries[PR_TU_MEDIUM]},
	(MenuEntry){.name = "Fast", 	.icn = ICON_TURBO_FAST,  	.dataPE = &profile.entries[PR_TU_FAST]}};
static struct Menu menu_turbo = (Menu){
	.id = MENU_TURBO_ID, 
	.parent = MENU_MAIN_PROFILE_ID,
	.name = "$K PROFILE > TURBO", 
	.footer = 	"$<$>CHANGE $SRESET $;RESET ALL         "
				"$CBACK                          $:CLOSE",
	.onButton = onButton_turbo,
	.onDraw = onDraw_turbo,
	.num = SIZE(menu_turbo_entries), 
	.entries = menu_turbo_entries};

void menu_initTurbo(){
	gui_registerMenu(&menu_turbo);
}
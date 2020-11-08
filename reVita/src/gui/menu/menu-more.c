#include <vitasdkkern.h>
#include "../../main.h"
#include "../../common.h"
#include "../../fio/settings.h"
#include "../gui.h"
#include "../renderer.h"
#include "../rendererv.h"

void onButton_more(uint32_t btn){
	switch (btn) {
		case SCE_CTRL_SELECT: profile_resetMore(); break;
		default: onButton_genericEntries(btn);
	}
}

void onDraw_more(uint menuY){
    int y = menuY;
	int ii = gui_calcStartingIndex(gui_menu->idx, gui_menu->num , gui_lines, BOTTOM_OFFSET);
	for (int i = ii; i < min(ii + gui_lines, gui_menu->num); i++) {			
		gui_drawEntry(L_1, y+= CHA_H, &gui_menu->entries[i], gui_menu->idx == i);
	}
	gui_drawFullScroll(ii > 0, ii + gui_lines < gui_menu->num, ((float)gui_menu->idx) / (gui_menu->num-1));
}

static struct MenuEntry menu_more_entries[] = {
	(MenuEntry){.name = "Delayed start", 			.icn = ICON_CONFIG, .dataPE = &profile.entries[PR_MO_DELAY_START]},
	(MenuEntry){.name = "Clear screen on close", 	.icn = ICON_CONFIG, .dataPE = &profile.entries[PR_MO_BLANK_FRAME]}};
static struct Menu menu_more = (Menu){
	.id = MENU_MORE_ID, 
	.parent = MENU_MAIN_PROFILE_ID,
	.name = "$| PROFILE > COMPABILITY", 
	.footer = 	"$<$>${$}CHANGE $SRESET $;RESET ALL     "
				"$CBACK                          $:CLOSE",
	.onButton = onButton_more,
	.onDraw = onDraw_more,
	.num = SIZE(menu_more_entries), 
	.entries = menu_more_entries};

void menu_initMore(){
	gui_registerMenu(&menu_more);
}
#include <vitasdkkern.h>
#include "../../log.h"
#include "../../main.h"
#include "../../common.h"
#include "../../fio/settings.h"
#include "../gui.h"
#include "../renderer.h"
#include "../rendererv.h"
#include "menu.h"

void onButton_touch(uint32_t btn){
	switch (btn) {
		case SCE_CTRL_SELECT: profile_resetTouch(); break;
		default: onButton_genericEntries(btn);
	}
}

static struct MenuEntry menu_touch_entries[] = {
	(MenuEntry){.name = "General", 				.type = HEADER_TYPE},
	(MenuEntry){.name = "Swipe duration", 		.icn = ICON_SWIPE,	.dataPE = &profile.entries[PR_TO_SWIPE_DURATION]},
	(MenuEntry){.name = "Smart Swipe sensitivity",.icn = ICON_SWIPE,	.dataPE = &profile.entries[PR_TO_SWIPE_SMART_SENS]},
	(MenuEntry){.name = "Swap touchpads", 		.icn = ICON_BT,		.dataPE = &profile.entries[PR_TO_SWAP]},
	(MenuEntry){.name = "Draw Pointer", 		.type = HEADER_TYPE},
	(MenuEntry){.name = "Touch Point", 			.icn = ICON_TOUCH,	.dataPE = &profile.entries[PR_TO_DRAW_POINT]},
	(MenuEntry){.name = "Swipe", 				.icn = ICON_SWIPE,	.dataPE = &profile.entries[PR_TO_DRAW_SWIPE]},
	(MenuEntry){.name = "Smart Swipe", 			.icn = ICON_SWIPE,	.dataPE = &profile.entries[PR_TO_DRAW_SMART_SWIPE]},
	(MenuEntry){.name = "Native", 				.icn = ICON_TOUCH,	.dataPE = &profile.entries[PR_TO_DRAW_NATIVE]}};
static struct Menu menu_touch = (Menu){
	.id = MENU_TOUCH_ID, 
	.parent = MENU_MAIN_PROFILE_ID,
	.name = "$F PROFILE > TOUCH", 
	.footer = 	"$<$>${$}CHANGE $SRESET $;RESET ALL     "
				"$CBACK                          $:CLOSE",
	.onButton = onButton_touch,
	.num = SIZE(menu_touch_entries), 
	.entries = menu_touch_entries};

void menu_initTouch(){
	gui_registerMenu(&menu_touch);
}
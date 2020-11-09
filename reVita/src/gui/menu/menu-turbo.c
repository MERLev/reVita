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

static struct MenuEntry menu_turbo_entries[] = {
	(MenuEntry){.name = "Slow", 	.icn = ICON_TURBO_SLOW,  	.dataPE = &profile.entries[PR_TU_SLOW]},
	(MenuEntry){.name = "Medium", 	.icn = ICON_TURBO_MED,  	.dataPE = &profile.entries[PR_TU_MEDIUM]},
	(MenuEntry){.name = "Fast", 	.icn = ICON_TURBO_FAST,  	.dataPE = &profile.entries[PR_TU_FAST]}};
static struct Menu menu_turbo = (Menu){
	.id = MENU_TURBO_ID, 
	.parent = MENU_MAIN_PROFILE_ID,
	.name = "$K PROFILE > TURBO", 
	.footer = 	"$<$>${$}CHANGE $SRESET $;RESET ALL     "
				"$CBACK                          $:CLOSE",
	.onButton = onButton_turbo,
	.num = SIZE(menu_turbo_entries), 
	.entries = menu_turbo_entries};

void menu_initTurbo(){
	gui_registerMenu(&menu_turbo);
}
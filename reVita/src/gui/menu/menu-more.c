#include <vitasdkkern.h>
#include "../../main.h"
#include "../../common.h"
#include "../../fio/settings.h"
#include "../gui.h"
#include "menu.h"

void onButton_more(uint32_t btn){
	switch (btn) {
		case SCE_CTRL_SELECT: profile_resetMore(); break;
		default: onButton_genericEntries(btn);
	}
}

static struct MenuEntry menu_more_entries[] = {
	(MenuEntry){.name = "Hack to enable System buttons",.icn = ICON_CONFIG, .dataPE = &profile.entries[PR_MO_SYS_BUTTONS]},
	(MenuEntry){.name = "Delayed start", 				.icn = ICON_CONFIG, .dataPE = &profile.entries[PR_MO_DELAY_START]},
	(MenuEntry){.name = "Prevent flicker", 				.icn = ICON_CONFIG, .dataPE = &profile.entries[PR_MO_NO_FLICKER]},
	(MenuEntry){.name = "Clear screen on close", 		.icn = ICON_CONFIG, .dataPE = &profile.entries[PR_MO_BLANK_FRAME]}};
static struct Menu menu_more = (Menu){
	.id = MENU_MORE_ID, 
	.parent = MENU_MAIN_PROFILE_ID,
	.name = "$| PROFILE > COMPABILITY", 
	.footer = 	"$<$>${$}CHANGE $SRESET $;RESET ALL     "
				"$CBACK                          $:CLOSE",
	.onButton = onButton_more,
	.num = SIZE(menu_more_entries), 
	.entries = menu_more_entries};

void menu_initMore(){
	gui_registerMenu(&menu_more);
}
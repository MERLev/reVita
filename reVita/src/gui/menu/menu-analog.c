#include <vitasdkkern.h>
#include "../../main.h"
#include "../../common.h"
#include "../../fio/settings.h"
#include "../gui.h"
#include "../renderer.h"
#include "../rendererv.h"
#include "menu.h"

void onButton_analog(uint32_t btn){
	switch (btn) {
		case SCE_CTRL_SELECT: profile_resetAnalog(); break;
		default: onButton_genericEntries(btn);
	}
}

static struct MenuEntry menu_analog_entries[] = {
	// (MenuEntry){.name = "General", .type = HEADER_TYPE},
	// (MenuEntry){.name = "Analog wide mode", .icn = ICON_LS_UP,  .dataPE = &profile.entries[PR_AN_MODE_WIDE]},
	(MenuEntry){.name = "Deadzone", .type = HEADER_TYPE},
	(MenuEntry){.name = "Left Analog", .icn = ICON_LS_UP,  .dataPE = &profile.entries[PR_AN_LEFT_DEADZONE]},
	(MenuEntry){.name = "Left Analog - X Axis", .icn = ICON_LS_LEFT,  .dataPE = &profile.entries[PR_AN_LEFT_DEADZONE_X]},
	(MenuEntry){.name = "Left Analog - Y Axis", .icn = ICON_LS_UP,.dataPE = &profile.entries[PR_AN_LEFT_DEADZONE_Y]},
	(MenuEntry){.name = "Right Analog", .icn = ICON_RS_UP,  .dataPE = &profile.entries[PR_AN_RIGHT_DEADZONE]},
	(MenuEntry){.name = "Right Analog - X Axis", .icn = ICON_RS_LEFT,  .dataPE = &profile.entries[PR_AN_RIGHT_DEADZONE_X]},
	(MenuEntry){.name = "Right Analog - Y Axis", .icn = ICON_RS_UP,.dataPE = &profile.entries[PR_AN_RIGHT_DEADZONE_Y]}};
static struct Menu menu_analog = (Menu){
	.id = MENU_ANALOG_ID, 
	.parent = MENU_MAIN_PROFILE_ID,
	.name = "$u PROFILE > ANALOG STICKS", 
	.footer = 	"$<$>${$}CHANGE $SRESET $;RESET ALL     "
				"$CBACK                          $:CLOSE",
	.onButton = onButton_analog,
	.num = SIZE(menu_analog_entries), 
	.entries = menu_analog_entries};

void menu_initAnalog(){
	gui_registerMenu(&menu_analog);
}
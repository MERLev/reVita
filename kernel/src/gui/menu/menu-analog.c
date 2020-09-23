#include <vitasdkkern.h>
#include "../../main.h"
#include "../../common.h"
#include "../../fio/settings.h"
#include "../gui.h"
#include "../renderer.h"

void onButton_analog(uint32_t btn){
	switch (btn) {
		case SCE_CTRL_START: profile_resetAnalog(); break;
		default: onButton_genericEntries(btn);
	}
}

void onDraw_analog(unsigned int menuY){
    int y = menuY;
	int ii = gui_calcStartingIndex(gui_menu->idx, gui_menu->num , gui_lines, BOTTOM_OFFSET);
	for (int i = ii; i < min(ii + gui_lines, gui_menu->num); i++) {			
		gui_drawEntry(L_1, y+= CHA_H, &gui_menu->entries[i], gui_menu->idx == i);
	}
	gui_drawFullScroll(ii > 0, ii + gui_lines < gui_menu->num, ((float)gui_menu->idx) / (gui_menu->num-1));
}

static struct MenuEntry menu_analog_entries[] = {
	(MenuEntry){.name = "Analog wide mode", .icn = ICON_LS_UP,  .dataPE = &profile.entries[PR_AN_MODE_WIDE]},
	(MenuEntry){.name = "Deadzone", .type = HEADER_TYPE},
	(MenuEntry){.name = "Left  Analog - X Axis", .icn = ICON_LS_LEFT,  .dataPE = &profile.entries[PR_AN_LEFT_DEADZONE_X]},
	(MenuEntry){.name = "Left  Analog - Y Axis", .icn = ICON_LS_UP,.dataPE = &profile.entries[PR_AN_LEFT_DEADZONE_Y]},
	(MenuEntry){.name = "Right Analog - X Axis", .icn = ICON_RS_LEFT,  .dataPE = &profile.entries[PR_AN_RIGHT_DEADZONE_X]},
	(MenuEntry){.name = "Right Analog - Y Axis", .icn = ICON_RS_UP,.dataPE = &profile.entries[PR_AN_RIGHT_DEADZONE_Y]}};
static struct Menu menu_analog = (Menu){
	.id = MENU_ANALOG_ID, 
	.parent = MENU_MAIN_ID,
	.name = "$u ANALOG STICKS", 
	.footer = "$SRESET  $:RESET ALL",
	.onButton = onButton_analog,
	.onDraw = onDraw_analog,
	.num = SIZE(menu_analog_entries), 
	.entries = menu_analog_entries};

void menu_initAnalog(){
	gui_registerMenu(&menu_analog);
}
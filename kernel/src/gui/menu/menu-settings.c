#include <vitasdkkern.h>
#include "../../common.h"
#include "../../fio/settings.h"
#include "../../fio/theme.h"
#include "../gui.h"
#include "../renderer.h"

void onButton_settings(uint32_t btn){
	int8_t id = gui_getEntry()->dataPE->id;
	switch (btn) {
		case SCE_CTRL_RIGHT: profile_inc(&settings[id], 1); break;
		case SCE_CTRL_LEFT: profile_dec(&settings[id], 1); break;
		case SCE_CTRL_SQUARE:
			settings_reset(id);
			if (id == SETT_THEME)
				theme_load(settings[id].v.u);
			break;
		case SCE_CTRL_START: 
			settings_resetAll(); 
			theme_load(settings[SETT_THEME].v.u);
			break;
		default: onButton_genericEntries(btn);break;
	}
}

void onDraw_settings(unsigned int menuY){
    int y = menuY;
	int ii = gui_calcStartingIndex(gui_menu->idx, gui_menu->num , gui_lines, BOTTOM_OFFSET);
	for (int i = ii; i < min(ii + gui_lines, gui_menu->num); i++) {		
		int32_t id = gui_menu->entries[i].dataPE->id;
		
		switch (id){
			case SETT_THEME:
				gui_setColor(i == gui_menu->idx, settings_isDef(id));
				renderer_drawString(L_1, y += CHA_H, gui_menu->entries[i].name);
				gui_drawStringFRight(0, y, "%s", settings[id].key);
				break;
			default: 
				gui_drawEntry(L_1, y+= CHA_H, &gui_menu->entries[i], gui_menu->idx == i);
				break;
		}
	}
	gui_drawFullScroll(ii > 0, ii + gui_lines < gui_menu->num, ((float)gui_menu->idx) / (gui_menu->num-1));
}

#define MENU_SETTINGS_NUM 4
static struct MenuEntry menu_settings_entries[MENU_SETTINGS_NUM] = {
	(MenuEntry){.name = "Remap rules enabled", .dataPE = &settings[SETT_REMAP_ENABLED]},
	(MenuEntry){.name = "Save profile on close", .dataPE = &settings[SETT_AUTOSAVE]},
	(MenuEntry){.name = "Startup delay", .dataPE = &settings[SETT_DELAY_INIT]},
	(MenuEntry){.name = "Theme", .dataPE = &settings[SETT_THEME]}};
static struct Menu menu_settings = (Menu){
	.id = MENU_SETT_ID, 
	.parent = MENU_MAIN_ID,
	.num = MENU_SETTINGS_NUM, 
	.name = "$| SETTINGS", 
	.footer = "$SRESET  $:RESET ALL",
	.onButton = onButton_settings,
	.onDraw = onDraw_settings,
	.entries = menu_settings_entries};

void menu_initSettings(){
	gui_registerMenu(&menu_settings);
}
#include <vitasdkkern.h>
#include "../../common.h"
#include "../../fio/settings.h"
#include "../../fio/theme.h"
#include "../gui.h"
#include "../renderer.h"

void onButton_settings(uint32_t btn){
	int8_t id = gui_getEntry()->data;
	switch (btn) {
		case SCE_CTRL_RIGHT:
			switch (id){
				case SETT_KEYS_MENU:
				case SETT_KEYS_REMAPS_TOOGLE:
					// Do nothing
					break;
				default: profile_inc(&settings[id], 1); break;
			}
			break;
		case SCE_CTRL_LEFT:
			switch (id){
				case SETT_KEYS_MENU:
				case SETT_KEYS_REMAPS_TOOGLE:
					// Do nothing
					break;
				default: profile_dec(&settings[id], 1); break;
			}
			break;
		case SCE_CTRL_SQUARE:
			settings_reset(id);
			if (id == SETT_THEME)
				theme_load(settings[id].v.u);
			break;
		case SCE_CTRL_CROSS:
			switch (id){
				case SETT_KEYS_MENU:
				case SETT_KEYS_REMAPS_TOOGLE:
					gui_openMenuSmart(MENU_PICK_BUTTON_ID, 
						gui_menu->id, gui_menu->id, (uint32_t)&settings[id].v.u); 
					break;
				default: break;
			}
			break;
		case SCE_CTRL_START: 
			settings_resetAll(); 
			theme_load(settings[SETT_THEME].v.u);
			break;
		default: onButton_generic(btn);break;
	}
}

void onDraw_settings(unsigned int menuY){
    int y = menuY;
	int ii = gui_calcStartingIndex(gui_menu->idx, gui_menu->num , gui_lines, BOTTOM_OFFSET);
	for (int i = ii; i < min(ii + gui_lines, gui_menu->num); i++) {		
		int32_t id = gui_menu->entries[i].data;
		
		gui_setColor(i == gui_menu->idx, settings_isDef(id));
		renderer_drawString(L_1, y += CHA_H, gui_menu->entries[i].name);
		switch (id){
			case SETT_KEYS_MENU:
			case SETT_KEYS_REMAPS_TOOGLE:
				;char str[10];
				str[0] = '\0';
				gui_generateBtnComboName(str, settings[id].v.u, 6);
				gui_drawStringFRight(0, y, "%s", str);
				break;
			case SETT_AUTOSAVE:
			case SETT_REMAP_ENABLED:
				gui_drawStringFRight(0, y, "%s", STR_YN[settings[id].v.b]);
				break;
			case SETT_DELAY_INIT:
				gui_drawStringFRight(0, y, "%hhu", settings[id].v.u);
				break;
			case SETT_THEME:
				gui_drawStringFRight(0, y, "%s", settings[id].key);
				break;
			default: break;
		}
	}
	gui_drawFullScroll(ii > 0, ii + gui_lines < gui_menu->num, ((float)gui_menu->idx) / (gui_menu->num-1));
}

#define MENU_SETTINGS_NUM 6
static struct MenuEntry menu_settings_entries[MENU_SETTINGS_NUM] = {
	(MenuEntry){.name = "Remap rules enabled", .data = SETT_REMAP_ENABLED},
	(MenuEntry){.name = "Menu trigger keys", .data = SETT_KEYS_MENU},
	(MenuEntry){.name = "Toggle remap rules keys", .data = SETT_KEYS_REMAPS_TOOGLE},
	(MenuEntry){.name = "Save profile on close", .data = SETT_AUTOSAVE},
	(MenuEntry){.name = "Startup delay", .data = SETT_DELAY_INIT},
	(MenuEntry){.name = "Theme", .data = SETT_THEME}};
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
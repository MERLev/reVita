#include <vitasdkkern.h>
#include "../../common.h"
#include "../../fio/settings.h"
#include "../../fio/theme.h"
#include "../gui.h"
#include "../renderer.h"
#include "../rendererv.h"

char* STR_THEME[THEME__NUM] = {
	"DARK",
	"LIGHT"
};

void onButton_popup(uint32_t btn){
	int8_t id = gui_getEntry()->dataPE->id;
	switch (btn) {
		case SCE_CTRL_RIGHT: profile_inc(&settings[id], 1); break;
		case SCE_CTRL_LEFT: profile_dec(&settings[id], 1); break;
		case SCE_CTRL_SQUARE: settings_reset(id); break;
		case SCE_CTRL_START: settings_resetAllPopups(); break;
		case SCE_CTRL_CIRCLE: 
			settings_save();
			gui_popupShowSuccess("$G Saving popups", "Done !", TTL_POPUP_SHORT);
			gui_openMenuParent();
			break;
		default: onButton_genericEntries(btn); break;
	}
}

void onButton_settings(uint32_t btn){
	int8_t id = gui_getEntry()->dataPE->id;
	switch (btn) {
		case SCE_CTRL_RIGHT: profile_inc(&settings[id], 1); break;
		case SCE_CTRL_LEFT: profile_dec(&settings[id], 1); break;
		case SCE_CTRL_SQUARE: settings_reset(id); break;
		case SCE_CTRL_START: settings_resetAll(); break;
		case SCE_CTRL_CIRCLE: 
			settings_save();
			gui_popupShowSuccess("$G Saving settings", "Done !", TTL_POPUP_SHORT);
			gui_openMenuParent();
			break;
		default: onButton_genericEntries(btn); break;
	}
	if (id == SETT_THEME || btn == SCE_CTRL_START) 
		theme_load(settings[SETT_THEME].v.u);
}

void onDraw_settings(uint menuY){
    int y = menuY;
	int ii = gui_calcStartingIndex(gui_menu->idx, gui_menu->num , gui_lines, BOTTOM_OFFSET);
	for (int i = ii; i < min(ii + gui_lines, gui_menu->num); i++) {		
		
		if (gui_menu->entries[i].type == HEADER_TYPE){
			gui_drawEntry(L_1, y+= CHA_H, &gui_menu->entries[i], gui_menu->idx == i); 
			continue;
		}

		int32_t id = gui_menu->entries[i].dataPE->id;
		switch (id){
			case SETT_THEME:
				gui_setColor(i == gui_menu->idx, settings_isDef(id));
				rendererv_drawString(L_1, y += CHA_H, gui_menu->entries[i].name);
				gui_drawStringFRight(0, y, "%s", STR_THEME[settings[id].v.u]);
				break;
			default: 
				gui_drawEntry(L_1, y+= CHA_H, &gui_menu->entries[i], gui_menu->idx == i);
				break;
		}
	}
	gui_drawFullScroll(ii > 0, ii + gui_lines < gui_menu->num, ((float)gui_menu->idx) / (gui_menu->num-1));
}

static struct MenuEntry menu_settings_entries[] = {
	(MenuEntry){.name = "Plugin enabled", 			.dataPE = &settings[SETT_REMAP_ENABLED]},
	(MenuEntry){.name = "Save profile on close", 	.dataPE = &settings[SETT_AUTOSAVE]},
	(MenuEntry){.name = "Animation", 				.dataPE = &settings[SETT_ANIMATION]},
	// (MenuEntry){.name = "Startup delay", 		.dataPE = &settings[SETT_DELAY_INIT]},
	(MenuEntry){.name = "Theme", 					.dataPE = &settings[SETT_THEME]}};
static struct Menu menu_settings = (Menu){
	.id = MENU_SETT_ID, 
	.parent = MENU_MAIN_SETTINGS_ID,
	.name = "$| SETTINGS > GLOBAL", 
	.footer = 	"$<$>CHANGE $SRESET $:RESET ALL   $CBACK",
	.onButton = onButton_settings,
	.onDraw = onDraw_settings,
	.num = SIZE(menu_settings_entries), 
	.entries = menu_settings_entries};

static struct MenuEntry menu_popup_entries[] = {
	(MenuEntry){.name = "Allow popups", 	.dataPE = &settings[POP_ALL]},
	(MenuEntry){.name = "Toggle individual popups", 	.type = HEADER_TYPE},
	(MenuEntry){.name = "Toggle RemaPSV2", 	.dataPE = &settings[POP_REVITA]},
	(MenuEntry){.name = "RemaPSV2 ready", 	.dataPE = &settings[POP_READY]},
	(MenuEntry){.name = "RemaPSV2 Loading", .dataPE = &settings[POP_LOADING]},
	(MenuEntry){.name = "Profile Save", 	.dataPE = &settings[POP_SAVE]},
	(MenuEntry){.name = "Profile Load", 	.dataPE = &settings[POP_LOAD]},
	(MenuEntry){.name = "Brightness", 		.dataPE = &settings[POP_BRIGHTNESS]},
	(MenuEntry){.name = "Kill application", .dataPE = &settings[POP_KILL]}};
static struct Menu menu_popup = (Menu){
	.id = MENU_POPUP_ID, 
	.parent = MENU_MAIN_SETTINGS_ID,
	.name = "$I SETTINGS > POPUPS", 
	.footer = 	"$<$>CHANGE $SRESET $:RESET ALL   $CBACK",
	.onButton = onButton_popup,
	.onDraw = onDraw_settings,
	.num = SIZE(menu_popup_entries), 
	.entries = menu_popup_entries};

void menu_initSettings(){
	gui_registerMenu(&menu_settings);
	gui_registerMenu(&menu_popup);
}
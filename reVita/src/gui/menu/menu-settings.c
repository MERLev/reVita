#include <vitasdkkern.h>
#include "../../common.h"
#include "../../fio/settings.h"
#include "../../fio/theme.h"
#include "../gui.h"
#include "menu.h"

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
		case SCE_CTRL_SELECT: settings_resetAllPopups(); break;
		case SCE_CTRL_CIRCLE: 
			settings_save();
			gui_popupShowSuccess("$G Saving popups", "Done !", TTL_POPUP_SHORT);
			gui_openMenuParent();
			break;
		case SCE_CTRL_START: 
			settings_save();
			gui_popupShowSuccess("$G Saving popups", "Done !", TTL_POPUP_SHORT);
		default: onButton_genericEntries(btn); break;
	}
}

void onButton_settings(uint32_t btn){
	int8_t id = gui_getEntry()->dataPE->id;
	switch (btn) {
		case SCE_CTRL_RIGHT: profile_inc(&settings[id], 1); break;
		case SCE_CTRL_LEFT: profile_dec(&settings[id], 1); break;
		case SCE_CTRL_SQUARE: settings_reset(id); break;
		case SCE_CTRL_SELECT: 
			settings_resetAll(); 
			break;
		case SCE_CTRL_CIRCLE: 
			settings_save();
			gui_popupShowSuccess("$G Saving settings", "Done !", TTL_POPUP_SHORT);
			gui_openMenuParent();
			break;
		case SCE_CTRL_START: 
			settings_save();
			gui_popupShowSuccess("$G Saving settings", "Done !", TTL_POPUP_SHORT);
		default: onButton_genericEntries(btn); break;
	}
	if (id == SETT_THEME || btn == SCE_CTRL_SELECT) 
		theme_load(settings[SETT_THEME].v.u);
}

struct MenuEntry menu_settings_entries[] = {
	(MenuEntry){.name = "Plugin enabled", 			.icn = ICON_MENU_SETTINGS,  .dataPE = &settings[SETT_REMAP_ENABLED]},
	(MenuEntry){.name = "Save profile on close", 	.icn = ICON_MENU_SETTINGS,  .dataPE = &settings[SETT_AUTOSAVE]},
	(MenuEntry){.name = "Animation", 				.icn = ICON_MENU_SETTINGS,  .dataPE = &settings[SETT_ANIMATION]},
	(MenuEntry){.name = "Theme", 					.icn = ICON_MENU_SETTINGS,  .dataPE = &settings[SETT_THEME], .dataPEStr = STR_THEME}};
static struct Menu menu_settings = (Menu){
	.id = MENU_SETT_ID, 
	.parent = MENU_MAIN_SETTINGS_ID,
	.name = "$| SETTINGS > GLOBAL", 
	.footer = 	"$<$>${$}CHANGE $SRESET $;RESET ALL     "
				"$CBACK                          $:CLOSE",
	.onButton = onButton_settings,
	.num = SIZE(menu_settings_entries), 
	.entries = menu_settings_entries};

static struct MenuEntry menu_popup_entries[] = {
	(MenuEntry){.name = "General", 			.type = HEADER_TYPE},
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
	.footer = 	"$<$>${$}CHANGE $SRESET $;RESET ALL     "
				"$CBACK                          $:CLOSE",
	.onButton = onButton_popup,
	.num = SIZE(menu_popup_entries), 
	.entries = menu_popup_entries};

void menu_initSettings(){
	gui_registerMenu(&menu_settings);
	gui_registerMenu(&menu_popup);
}
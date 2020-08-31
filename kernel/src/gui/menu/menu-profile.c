#include <vitasdkkern.h>
#include "../../common.h"
#include "../../fio/settings.h"
#include "../gui.h"
#include "../renderer.h"

enum PROFILE_ACTIONS{
	PROFILE_GLOBAL_SAVE = 0,
	PROFILE_GLOABL_LOAD,
	PROFILE_GLOBAL_RESET,
	PROFILE_LOCAL_SAVE,
	PROFILE_LOCAL_LOAD,
	PROFILE_LOCAL_RESET,
	PROFILE_LOCAL_DELETE
};

void onButton_profiles(uint32_t btn){
	switch (btn) {
		case SCE_CTRL_CROSS:
			switch (gui_getEntry()->dataUint){
				case PROFILE_LOCAL_SAVE:   profile_localSave(); break;
				case PROFILE_LOCAL_LOAD:   profile_localLoad(); break;
				case PROFILE_LOCAL_RESET:  profile_localReset(); break;
				case PROFILE_LOCAL_DELETE: profile_localDelete(); break;
				case PROFILE_GLOBAL_SAVE:  profile_saveAsGlobal(); break;
				case PROFILE_GLOABL_LOAD:  profile_loadFromGlobal(); break;
				case PROFILE_GLOBAL_RESET: profile_resetGlobal(); break;
				default: break;
			}
			break;
		default: onButton_generic(btn);
	}
}

void onDraw_profiles(unsigned int menuY){
    int y = menuY;
	int ii = gui_calcStartingIndex(gui_menu->idx, gui_menu->num, gui_lines, BOTTOM_OFFSET);
	for (int i = ii; i < min(ii + gui_lines, gui_menu->num); i++) {
		if (gui_menu->entries[i].type == HEADER_TYPE){
				gui_setColorHeader(gui_menu->idx == i);
			if (i == 0){
				renderer_drawStringF(L_1, y+=CHA_H, "%s [%s]", 
					gui_menu->entries[i].name, profile.titleid);
			} else {
				renderer_drawString(L_1, y+=CHA_H, gui_menu->entries[i].name);
			}
		} else {
			gui_setColor(i == gui_menu->idx, 1);
			renderer_drawString(L_2, y += CHA_H, gui_menu->entries[i].name);
		}
	}
}

#define MENU_PROFILE_NUM 9
static struct MenuEntry menu_profiles_entries[MENU_PROFILE_NUM] = {
	(MenuEntry){.name = "Local", .type = HEADER_TYPE},
	(MenuEntry){.name = "Save", .dataUint = PROFILE_LOCAL_SAVE},
	(MenuEntry){.name = "Load", .dataUint = PROFILE_LOCAL_LOAD},
	(MenuEntry){.name = "Reset", .dataUint = PROFILE_LOCAL_DELETE},
	(MenuEntry){.name = "Delete", .dataUint = PROFILE_LOCAL_RESET},
	(MenuEntry){.name = "Global", .type = HEADER_TYPE},
	(MenuEntry){.name = "Save as global", .dataUint = PROFILE_GLOBAL_SAVE},
	(MenuEntry){.name = "Load from global", .dataUint = PROFILE_GLOABL_LOAD},
	(MenuEntry){.name = "Reset global", .dataUint = PROFILE_GLOBAL_RESET}};
static struct Menu menu_profiles = (Menu){
	.id = MENU_PROFILE_ID, 
	.parent = MENU_MAIN_ID,
	.num = MENU_PROFILE_NUM, 
	.name = "$/ PROFILES", 
	.onButton = onButton_profiles,
	.onDraw = onDraw_profiles,
	.entries = menu_profiles_entries};

void menu_initProfile(){
	gui_registerMenu(&menu_profiles);
}
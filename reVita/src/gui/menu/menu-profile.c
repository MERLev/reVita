#include <vitasdkkern.h>
#include "../../common.h"
#include "../../main.h"
#include "../../fio/settings.h"
#include "../gui.h"
#include "../rendererv.h"
#include "menu.h"

enum PROFILE_ACTIONS{
	PROFILE_GLOBAL_SAVE = 0,
	PROFILE_GLOBAL_LOAD,
	PROFILE_GLOBAL_RESET,
	PROFILE_SHARED_SAVE,
	PROFILE_SHARED_LOAD,
	PROFILE_SHARED_DELETE,
	PROFILE_LOCAL_SAVE,
	PROFILE_LOCAL_LOAD,
	PROFILE_LOCAL_RESET,
	PROFILE_LOCAL_DELETE
};

void onButton_profiles(uint32_t btn){
	switch (btn) {
		case SCE_CTRL_CROSS:
			switch (gui_getEntry()->dataUint){
				case PROFILE_LOCAL_SAVE:   
					profile_saveLocal(); 
					isSafeBoot = false;
					gui_popupShowSuccess("$G Profile saved", profile.titleid, TTL_POPUP_SHORT);
					break;
				case PROFILE_LOCAL_LOAD:   
					profile_loadLocal(); 
					gui_popupShowSuccess("$H Profile loaded", profile.titleid, TTL_POPUP_SHORT);
					break;
				case PROFILE_LOCAL_RESET:  
					profile_resetLocal(); 
					gui_popupShowSuccess("$J Profile reset", profile.titleid, TTL_POPUP_SHORT);
					break;
				case PROFILE_LOCAL_DELETE: 
					profile_deleteLocal(); 
					gui_popupShowSuccess("$J Profile removed", profile.titleid, TTL_POPUP_SHORT);
					break;
				case PROFILE_GLOBAL_SAVE:  
					profile_saveAsGlobal();
					gui_popupShowSuccess("$G Profile saved", "as Global", TTL_POPUP_SHORT); 
					break;
				case PROFILE_GLOBAL_LOAD:  
					profile_loadFromGlobal();
					gui_popupShowSuccess("$H Profile loaded", "from Global", TTL_POPUP_SHORT);
					break;
				case PROFILE_GLOBAL_RESET: 
					profile_resetGlobal();
					gui_popupShowSuccess("$J Profile reset", "Global", TTL_POPUP_SHORT); 
					break;
				case PROFILE_SHARED_SAVE:  
					profile_saveAsShared();
					gui_popupShowSuccess("$G Profile saved", "as Shared", TTL_POPUP_SHORT); 
					break;
				case PROFILE_SHARED_LOAD:  
					profile_loadFromShared(); 
					gui_popupShowSuccess("$H Profile loaded", "from Shared", TTL_POPUP_SHORT);
					break;
				case PROFILE_SHARED_DELETE:
					profile_deleteShared();
					gui_popupShowSuccess("$J Profile removed", "Shared", TTL_POPUP_SHORT);
					break;
				default: break;
			}
			break;
		default: onButton_generic(btn);
	}
}

void onDrawEntry_profileName(int x, int y, MenuEntry* me, bool isSelected, bool hasHeaders){
    gui_setColorHeader(isSelected);
	rendererv_drawStringF(L_1, y, "%s [%s]", me->name, profile.titleid);
}

static struct MenuEntry menu_profiles_entries[] = {
	(MenuEntry){.name = "Local", .type = HEADER_TYPE, .onDraw = onDrawEntry_profileName},
	(MenuEntry){.name = "$G Save", .dataUint = PROFILE_LOCAL_SAVE},
	(MenuEntry){.name = "$H Load", .dataUint = PROFILE_LOCAL_LOAD},
	(MenuEntry){.name = "$J Reset", .dataUint = PROFILE_LOCAL_RESET},
	(MenuEntry){.name = "$J Delete", .dataUint = PROFILE_LOCAL_DELETE},
	(MenuEntry){.name = "Global", .type = HEADER_TYPE},
	(MenuEntry){.name = "$G Save as global", .dataUint = PROFILE_GLOBAL_SAVE},
	(MenuEntry){.name = "$H Load from global", .dataUint = PROFILE_GLOBAL_LOAD},
	(MenuEntry){.name = "$J Reset global", .dataUint = PROFILE_GLOBAL_RESET},
	(MenuEntry){.name = "Shared", .type = HEADER_TYPE},
	(MenuEntry){.name = "$G Save as shared", .dataUint = PROFILE_SHARED_SAVE},
	(MenuEntry){.name = "$H Load from shared", .dataUint = PROFILE_SHARED_LOAD},
	(MenuEntry){.name = "$J Reset shared", .dataUint = PROFILE_SHARED_DELETE}};
static struct Menu menu_profiles = (Menu){
	.id = MENU_PROFILE_ID, 
	.parent = MENU_MAIN_PROFILE_ID,
	.name = "$/ PROFILE > MANAGE PROFILES", 
	.footer = 	"$XSELECT                 $CBACK $:CLOSE",
	.onButton = onButton_profiles,
	.num = SIZE(menu_profiles_entries), 
	.entries = menu_profiles_entries};

void menu_initProfile(){
	gui_registerMenu(&menu_profiles);
}
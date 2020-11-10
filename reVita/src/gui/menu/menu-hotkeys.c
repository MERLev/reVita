#include <vitasdkkern.h>
#include "../../common.h"
#include "../../fio/hotkeys.h"
#include "../../fio/theme.h"
#include "../gui.h"
#include "menu.h"

void onButton_hotkeys(uint32_t btn){
	ProfileEntry* pe = gui_getEntry()->dataPEButton;
	switch (btn) {
		case SCE_CTRL_CROSS:
			gui_openMenuSmartPtr(MENU_PICK_BUTTON_ID, 
				gui_menu->id, gui_menu->id, &pe->v.u); 
			break;		
		case SCE_CTRL_TRIANGLE:
			gui_getEntry()->dataPEButton->v.u = 0;
			break;
		case SCE_CTRL_SQUARE: hotkeys_reset(pe->id); break;
		case SCE_CTRL_SELECT: hotkeys_resetAll(); break;
		case SCE_CTRL_CIRCLE: 
			hotkeys_save(); 
			gui_popupShowSuccess("$G Saving hotkeys", "Done !", TTL_POPUP_SHORT);
			gui_openMenuPrev();
			break;
		default: onButton_generic(btn);break;
	}
}

static struct MenuEntry menu_hotkeys_entries[] = {
	(MenuEntry){.name = "reVita", 				.type = HEADER_TYPE},
	(MenuEntry){.name = "Open menu", 			.dataPEButton = &hotkeys[HOTKEY_MENU]},
	(MenuEntry){.name = "Safe Start", 			.dataPEButton = &hotkeys[HOTKEY_SAFE_START]},
	(MenuEntry){.name = "Toggle state", 		.dataPEButton = &hotkeys[HOTKEY_REMAPS_TOOGLE]},
	(MenuEntry){.name = "Reset profile",				.dataPEButton = &hotkeys[HOTKEY_PROFILE_LOCAL_RESET]},
	(MenuEntry){.name = "Import profile from Shared",	.dataPEButton = &hotkeys[HOTKEY_PROFILE_SHARED_LOAD]},
	(MenuEntry){.name = "System", 				.type = HEADER_TYPE},
	(MenuEntry){.name = "Soft reset", 			.dataPEButton = &hotkeys[HOTKEY_RESET_SOFT]},
	(MenuEntry){.name = "Reboot", 				.dataPEButton = &hotkeys[HOTKEY_RESET_COLD]},
	(MenuEntry){.name = "Power Off", 			.dataPEButton = &hotkeys[HOTKEY_STANDBY]},
	(MenuEntry){.name = "Suspend", 				.dataPEButton = &hotkeys[HOTKEY_SUSPEND]},
	(MenuEntry){.name = "Display Off", 			.dataPEButton = &hotkeys[HOTKEY_DISPLAY_OFF]},
	(MenuEntry){.name = "Kill App", 			.dataPEButton = &hotkeys[HOTKEY_KILL_APP]},
	(MenuEntry){.name = "Brightness +", 		.dataPEButton = &hotkeys[HOTKEY_BRIGHTNESS_INC]},
	(MenuEntry){.name = "Brightness -", 		.dataPEButton = &hotkeys[HOTKEY_BRIGHTNESS_DEC]},
	(MenuEntry){.name = "Savegame", 			.type = HEADER_TYPE},
	(MenuEntry){.name = "Backup", 				.dataPEButton = &hotkeys[HOTKEY_SAVE_BACKUP]},
	(MenuEntry){.name = "Restore", 				.dataPEButton = &hotkeys[HOTKEY_SAVE_RESTORE]},
	(MenuEntry){.name = "Remove backup",		.dataPEButton = &hotkeys[HOTKEY_SAVE_DELETE]},
	(MenuEntry){.name = "Gyroscope", 			.type = HEADER_TYPE},
	(MenuEntry){.name = "Calibrate", 			.dataPEButton = &hotkeys[HOTKEY_MOTION_CALIBRATE]}};
static struct Menu menu_hotkeys = (Menu){
	.id = MENU_HOTKEYS_ID, 
	.parent = MENU_MAIN_SETTINGS_ID,
	.name = "$c SETTINGS > HOTKEYS", 
	.footer = 	"$XSELECT $TCLEAR $SRESET $;RESET ALL   "
				"$CBACK                          $:CLOSE",
	.onButton = onButton_hotkeys,
	.num = SIZE(menu_hotkeys_entries), 
	.entries = menu_hotkeys_entries};

void menu_initHotkeys(){
	gui_registerMenu(&menu_hotkeys);
}
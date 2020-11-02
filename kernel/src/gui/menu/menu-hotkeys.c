#include <vitasdkkern.h>
#include "../../common.h"
#include "../../fio/hotkeys.h"
#include "../../fio/theme.h"
#include "../gui.h"
#include "../renderer.h"
#include "../rendererv.h"

void onButton_hotkeys(uint32_t btn){
	int8_t id = gui_getEntry()->dataPE->id;
	switch (btn) {
		case SCE_CTRL_CROSS:
			gui_openMenuSmartPtr(MENU_PICK_BUTTON_ID, 
				gui_menu->id, gui_menu->id, &hotkeys[id].v.u); 
			break;
		case SCE_CTRL_SQUARE: hotkeys_reset(id); break;
		case SCE_CTRL_START: hotkeys_resetAll(); break;
		case SCE_CTRL_CIRCLE: 
			hotkeys_save(); 
			gui_openMenuPrev();
			break;
		default: onButton_generic(btn);break;
	}
}

void onDraw_hotkeys(uint menuY){
    int y = menuY;
	int ii = gui_calcStartingIndex(gui_menu->idx, gui_menu->num , gui_lines, BOTTOM_OFFSET);
	for (int i = ii; i < min(ii + gui_lines, gui_menu->num); i++) {		
		int32_t id = gui_menu->entries[i].dataPE->id;
		gui_setColor(i == gui_menu->idx, hotkeys_isDef(id));
		rendererv_drawString(L_1, y += CHA_H, gui_menu->entries[i].name);
		char str[10];
		str[0] = '\0';
		gui_generateBtnComboName(str, hotkeys[id].v.u, 6);
		gui_drawStringFRight(0, y, "%s", str);
	}
	gui_drawFullScroll(ii > 0, ii + gui_lines < gui_menu->num, ((float)gui_menu->idx) / (gui_menu->num-1));
}

static struct MenuEntry menu_hotkeys_entries[] = {
	(MenuEntry){.name = "Menu open", 			.dataPE = &hotkeys[HOTKEY_MENU]},
	(MenuEntry){.name = "Toggle remap rules", 	.dataPE = &hotkeys[HOTKEY_REMAPS_TOOGLE]},
	(MenuEntry){.name = "Soft reset", 			.dataPE = &hotkeys[HOTKEY_RESET_SOFT]},
	(MenuEntry){.name = "Reboot", 				.dataPE = &hotkeys[HOTKEY_RESET_COLD]},
	(MenuEntry){.name = "Power Off", 			.dataPE = &hotkeys[HOTKEY_STANDBY]},
	(MenuEntry){.name = "Suspend", 				.dataPE = &hotkeys[HOTKEY_SUSPEND]},
	(MenuEntry){.name = "Display Off", 			.dataPE = &hotkeys[HOTKEY_DISPLAY_OFF]},
	(MenuEntry){.name = "Kill App", 			.dataPE = &hotkeys[HOTKEY_KILL_APP]},
	(MenuEntry){.name = "Brightness +", 		.dataPE = &hotkeys[HOTKEY_BRIGHTNESS_INC]},
	(MenuEntry){.name = "Brightness -", 		.dataPE = &hotkeys[HOTKEY_BRIGHTNESS_DEC]}/*,
	(MenuEntry){.name = "Savefile backup", 		.dataPE = &hotkeys[HOTKEY_SAVE_BACKUP]},
	(MenuEntry){.name = "Savefile restore", 	.dataPE = &hotkeys[HOTKEY_SAVE_RESTORE]}*/};
static struct Menu menu_hotkeys = (Menu){
	.id = MENU_HOTKEYS_ID, 
	.parent = MENU_MAIN_SETTINGS_ID,
	.name = "$c SETTINGS > HOTKEYS", 
	.footer = 	"$XSELECT $SRESET $:RESET ALL     $CBACK",
	.onButton = onButton_hotkeys,
	.onDraw = onDraw_hotkeys,
	.num = SIZE(menu_hotkeys_entries), 
	.entries = menu_hotkeys_entries};

void menu_initHotkeys(){
	gui_registerMenu(&menu_hotkeys);
}
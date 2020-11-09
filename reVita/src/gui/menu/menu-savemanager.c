#include <vitasdkkern.h>
#include "../../common.h"
#include "../../main.h"
#include "../../sysactions.h"
#include "../../fio/settings.h"
#include "../gui.h"
#include "../renderer.h"
#include "../rendererv.h"

enum SAVEMANAGER_ACTIONS{
	SAVEMANAGER_BACKUP = 0,
	SAVEMANAGER_RESTORE,
	SAVEMANAGER_CLEAR,
	SAVEMANAGER_CLEAR_ALL
};

void onButton_savemanager(uint32_t btn){
	switch (btn) {
		case SCE_CTRL_CROSS:
			switch (gui_getEntry()->dataUint){
				case SAVEMANAGER_BACKUP:   	sysactions_saveBackup(); break;
				case SAVEMANAGER_RESTORE:   sysactions_saveRestore(); break;
				case SAVEMANAGER_CLEAR: 	sysactions_saveDelete(); break;
				case SAVEMANAGER_CLEAR_ALL: sysactions_saveDeleteAll(); break;
				default: break;
			}
			break;
		default: onButton_generic(btn);
	}
}

void onDraw_savemanager(uint menuY){
    int y = menuY;
	int ii = gui_calcStartingIndex(gui_menu->idx, gui_menu->num, gui_lines, BOTTOM_OFFSET);
	for (int i = ii; i < min(ii + gui_lines, gui_menu->num); i++) {
		if (gui_menu->entries[i].type == HEADER_TYPE){
				gui_setColorHeader(gui_menu->idx == i);
			if (i == 0){
				rendererv_drawStringF(L_1, y+=CHA_H, titleid);
			} else {
				rendererv_drawString(L_1, y+=CHA_H, gui_menu->entries[i].name);
			}
		} else {
			gui_setColor(i == gui_menu->idx, 1);
			rendererv_drawString(L_1+CHA_W, y += CHA_H, gui_menu->entries[i].name);
		}
	}
}

static struct MenuEntry menu_savemanager_entries[] = {
	(MenuEntry){.name = "", 					.type = HEADER_TYPE},
	(MenuEntry){.name = "$G Backup", 				.dataUint = SAVEMANAGER_BACKUP},
	(MenuEntry){.name = "$H Restore", 				.dataUint = SAVEMANAGER_RESTORE},
	(MenuEntry){.name = "$J Remove Backup", 		.dataUint = SAVEMANAGER_CLEAR},
	(MenuEntry){.name = "More", 				.type = HEADER_TYPE},
	(MenuEntry){.name = "$J Remove All Backups", 	.dataUint = SAVEMANAGER_CLEAR_ALL}};
static struct Menu menu_savemanager = (Menu){
	.id = MENU_SAVEMANAGER_ID, 
	.parent = MENU_MAIN_ID,
	.name = "$/ SAVE MANAGER", 
	.footer = 	"$XSELECT                 $CBACK $:CLOSE",
	.onButton = onButton_savemanager,
	.onDraw = onDraw_savemanager,
	.num = SIZE(menu_savemanager_entries), 
	.entries = menu_savemanager_entries};

void menu_initSavemanager(){
	gui_registerMenu(&menu_savemanager);
}
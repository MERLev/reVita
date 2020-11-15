#include <vitasdkkern.h>
#include <stdio.h>
#include "../../common.h"
#include "../../main.h"
#include "../../sysactions.h"
#include "../../fio/settings.h"
#include "../gui.h"
#include "../renderer.h"
#include "../rendererv.h"
#include "menu.h"

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

void onDrawEntry_saveName(int x, int y, MenuEntry* me, bool isSelected, bool hasHeaders){
    gui_setColorHeader(isSelected);

	char title[32];
    sprintf(title, removeSecondarySuffix(titleid));

	rendererv_drawStringF(L_1, y, title);
}

static struct MenuEntry menu_savemanager_entries[] = {
	(MenuEntry){.name = "", 						.type = HEADER_TYPE, .onDraw = onDrawEntry_saveName},
	(MenuEntry){.name = "$G Backup", 				.dataUint = SAVEMANAGER_BACKUP},
	(MenuEntry){.name = "$H Restore", 				.dataUint = SAVEMANAGER_RESTORE},
	(MenuEntry){.name = "$J Remove Backup", 		.dataUint = SAVEMANAGER_CLEAR},
	(MenuEntry){.name = "More", 					.type = HEADER_TYPE},
	(MenuEntry){.name = "$J Remove All Backups", 	.dataUint = SAVEMANAGER_CLEAR_ALL}};
static struct Menu menu_savemanager = (Menu){
	.id = MENU_SAVEMANAGER_ID, 
	.parent = MENU_MAIN_ID,
	.name = "$/ SAVE MANAGER", 
	.footer = 	"$XSELECT                 $CBACK $:CLOSE",
	.onButton = onButton_savemanager,
	.num = SIZE(menu_savemanager_entries), 
	.entries = menu_savemanager_entries};

void menu_initSavemanager(){
	gui_registerMenu(&menu_savemanager);
}
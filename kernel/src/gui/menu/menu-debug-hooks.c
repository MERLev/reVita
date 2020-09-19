#include <vitasdkkern.h>
#include "../../main.h"
#include "../../common.h"
#include "../../fio/settings.h"
#include "../../fio/theme.h"
#include "../gui.h"
#include "../renderer.h"

void onDraw_hooks(unsigned int menuY){
    int y = menuY;
	int ii = gui_calcStartingIndex(gui_menu->idx, gui_menu->num, gui_lines, gui_lines - 1);
	for (int i = ii; i < min(ii + gui_lines, gui_menu->num); i++) {
		renderer_setColor((used_funcs[gui_menu->entries[i].dataUint] ? theme[COLOR_ACTIVE] : theme[COLOR_DEFAULT]));
		renderer_drawStringF(L_1, y += CHA_H, gui_menu->entries[i].name);
		gui_drawStringFRight(0, y, "%s", STR_YN[used_funcs[gui_menu->entries[i].dataUint]]);
	}
	gui_drawFullScroll(ii > 0, ii + gui_lines < gui_menu->num, 
			((float)gui_menu->idx)/(gui_menu->num - (gui_lines - 1) - 1));
}

static struct MenuEntry menu_hooks_entries[] = {
	(MenuEntry){.name = "sceCtrlPeekBufferPositive", 	.dataUint = H_CT_PEEK_P},
	(MenuEntry){.name = "sceCtrlReadBufferPositive", 	.dataUint = H_CT_READ_P},
	(MenuEntry){.name = "sceCtrlPeekBufferNegative", 	.dataUint = H_CT_PEEK_N},
	(MenuEntry){.name = "sceCtrlReadBufferNegative", 	.dataUint = H_CT_READ_N},
	(MenuEntry){.name = "sceCtrlPeekBufferPositive2", 	.dataUint = H_CT_PEEK_P_2},
	(MenuEntry){.name = "sceCtrlReadBufferPositive2", 	.dataUint = H_CT_READ_P_2},
	(MenuEntry){.name = "sceCtrlPeekBufferPositiveExt", .dataUint = H_CT_PEEK_P_EXT},
	(MenuEntry){.name = "sceCtrlReadBufferPositiveExt", .dataUint = H_CT_READ_P_EXT},
	(MenuEntry){.name = "sceCtrlPeekBufferPositiveExt2",.dataUint = H_CT_PEEK_P_EXT2},
	(MenuEntry){.name = "sceCtrlReadBufferPositiveExt2",.dataUint = H_CT_READ_P_EXT2},
	(MenuEntry){.name = "sceCtrlPeekBufferNegative2", 	.dataUint = H_CT_PEEK_N_2},
	(MenuEntry){.name = "sceCtrlReadBufferNegative2", 	.dataUint = H_CT_READ_N_2},    
	(MenuEntry){.name = "ksceCtrlPeekBufferPositive", 	.dataUint = H_K_CT_PEEK_P},
    (MenuEntry){.name = "ksceCtrlReadBufferPositive", 	.dataUint = H_K_CT_READ_P},
    (MenuEntry){.name = "ksceCtrlPeekBufferNegative", 	.dataUint = H_K_CT_PEEK_N},
    (MenuEntry){.name = "ksceCtrlReadBufferNegative", 	.dataUint = H_K_CT_READ_N},
	(MenuEntry){.name = "ksceTouchPeek", 				.dataUint = H_K_TO_PEEK},
	(MenuEntry){.name = "ksceTouchRead", 				.dataUint = H_K_TO_READ},
	(MenuEntry){.name = "ksceTouchPeekRegion", 			.dataUint = H_K_TO_PEEK_R},
	(MenuEntry){.name = "ksceTouchReadRegion", 			.dataUint = H_K_TO_READ_R}
};
static struct Menu menu_hooks = (Menu){
	.id = MENU_HOKS_ID, 
	.parent = MENU_MAIN_ID,
	.name = "$b HOOKS", 
	.noIndent = true,
	.onDraw = onDraw_hooks,
	.num = SIZE(menu_hooks_entries), 
	.entries = menu_hooks_entries};

void menu_initDebugHooks(){
	gui_registerMenu(&menu_hooks);
}
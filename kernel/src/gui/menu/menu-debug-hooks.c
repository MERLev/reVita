#include <vitasdkkern.h>
#include "../../main.h"
#include "../../common.h"
#include "../../fio/settings.h"
#include "../../fio/theme.h"
#include "../gui.h"
#include "../renderer.h"

void onDraw_hooks(uint menuY){
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
	(MenuEntry){.name = "ksceCtrlPeekBufferPositive", 		.dataUint = ksceCtrlPeekBufferPositive_id},
    (MenuEntry){.name = "ksceCtrlReadBufferPositive", 		.dataUint = ksceCtrlReadBufferPositive_id},
    (MenuEntry){.name = "ksceCtrlPeekBufferNegative", 		.dataUint = ksceCtrlPeekBufferNegative_id},
    (MenuEntry){.name = "ksceCtrlReadBufferNegative", 		.dataUint = ksceCtrlReadBufferNegative_id},
	(MenuEntry){.name = "ksceCtrlPeekBufferPositive2", 		.dataUint = ksceCtrlPeekBufferPositive2_id},
	(MenuEntry){.name = "ksceCtrlReadBufferPositive2", 		.dataUint = ksceCtrlReadBufferPositive2_id},
	(MenuEntry){.name = "ksceCtrlPeekBufferPositiveExt", 	.dataUint = ksceCtrlPeekBufferPositiveExt_id},
	(MenuEntry){.name = "ksceCtrlReadBufferPositiveExt", 	.dataUint = ksceCtrlReadBufferPositiveExt_id},
	(MenuEntry){.name = "ksceCtrlPeekBufferPositiveExt2",	.dataUint = ksceCtrlPeekBufferPositiveExt2_id},
	(MenuEntry){.name = "ksceCtrlReadBufferPositiveExt2",	.dataUint = ksceCtrlReadBufferPositiveExt2_id},
	(MenuEntry){.name = "ksceCtrlPeekBufferNegative2", 		.dataUint = ksceCtrlPeekBufferNegative2_id},
	(MenuEntry){.name = "ksceCtrlReadBufferNegative2", 		.dataUint = ksceCtrlReadBufferNegative2_id},  
	(MenuEntry){.name = "ksceTouchPeek", 					.dataUint = ksceTouchPeek_id},
	(MenuEntry){.name = "ksceTouchRead", 					.dataUint = ksceTouchRead_id},
	(MenuEntry){.name = "ksceTouchPeekRegion", 				.dataUint = ksceTouchPeekRegion_id},
	(MenuEntry){.name = "ksceTouchReadRegion", 				.dataUint = ksceTouchReadRegion_id}
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
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
	(MenuEntry){.name = "sceCtrlPeekBufferPositive", 	.dataUint = sceCtrlPeekBufferPositive_id},
	(MenuEntry){.name = "sceCtrlReadBufferPositive", 	.dataUint = sceCtrlReadBufferPositive_id},
	(MenuEntry){.name = "sceCtrlPeekBufferNegative", 	.dataUint = sceCtrlPeekBufferNegative_id},
	(MenuEntry){.name = "sceCtrlReadBufferNegative", 	.dataUint = sceCtrlReadBufferNegative_id},
	(MenuEntry){.name = "sceCtrlPeekBufferPositive2", 	.dataUint = sceCtrlPeekBufferPositive2_id},
	(MenuEntry){.name = "sceCtrlReadBufferPositive2", 	.dataUint = sceCtrlReadBufferPositive2_id},
	(MenuEntry){.name = "sceCtrlPeekBufferPositiveExt", .dataUint = sceCtrlPeekBufferPositiveExt_id},
	(MenuEntry){.name = "sceCtrlReadBufferPositiveExt", .dataUint = sceCtrlReadBufferPositiveExt_id},
	(MenuEntry){.name = "sceCtrlPeekBufferPositiveExt2",.dataUint = sceCtrlPeekBufferPositiveExt2_id},
	(MenuEntry){.name = "sceCtrlReadBufferPositiveExt2",.dataUint = sceCtrlReadBufferPositiveExt2_id},
	(MenuEntry){.name = "sceCtrlPeekBufferNegative2", 	.dataUint = sceCtrlPeekBufferNegative2_id},
	(MenuEntry){.name = "sceCtrlReadBufferNegative2", 	.dataUint = sceCtrlReadBufferNegative2_id},    
	(MenuEntry){.name = "ksceCtrlPeekBufferPositive", 	.dataUint = ksceCtrlPeekBufferPositive_id},
    (MenuEntry){.name = "ksceCtrlReadBufferPositive", 	.dataUint = ksceCtrlReadBufferPositive_id},
    (MenuEntry){.name = "ksceCtrlPeekBufferNegative", 	.dataUint = ksceCtrlPeekBufferNegative_id},
    (MenuEntry){.name = "ksceCtrlReadBufferNegative", 	.dataUint = ksceCtrlReadBufferNegative_id},
	(MenuEntry){.name = "ksceTouchPeek", 				.dataUint = ksceTouchPeek_id},
	(MenuEntry){.name = "ksceTouchRead", 				.dataUint = ksceTouchRead_id},
	(MenuEntry){.name = "ksceTouchPeekRegion", 			.dataUint = ksceTouchPeekRegion_id},
	(MenuEntry){.name = "ksceTouchReadRegion", 			.dataUint = ksceTouchReadRegion_id}
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
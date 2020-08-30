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
		renderer_setColor((used_funcs[gui_menu->entries[i].data] ? theme[COLOR_ACTIVE] : theme[COLOR_DEFAULT]));
		renderer_drawStringF(L_1, y += CHA_H, gui_menu->entries[i].name);
		gui_drawStringFRight(0, y, "%s", STR_YN[used_funcs[gui_menu->entries[i].data]]);
	}
	gui_drawFullScroll(ii > 0, ii + gui_lines < gui_menu->num, 
			((float)gui_menu->idx)/(gui_menu->num - (gui_lines - 1) - 1));
}

#define MENU_HOKS_NUM 20
static struct MenuEntry menu_hooks_entries[MENU_HOKS_NUM] = {
	(MenuEntry){.name = "sceCtrlPeekBufferPositive", .data = 0},
	(MenuEntry){.name = "sceCtrlReadBufferPositive", .data = 1},
	(MenuEntry){.name = "sceCtrlPeekBufferNegative", .data = 2},
	(MenuEntry){.name = "sceCtrlReadBufferNegative", .data = 3},
	(MenuEntry){.name = "sceCtrlPeekBufferPositive2", .data = 4},
	(MenuEntry){.name = "sceCtrlReadBufferPositive2", .data = 5},
	(MenuEntry){.name = "sceCtrlPeekBufferPositiveExt", .data = 6},
	(MenuEntry){.name = "sceCtrlReadBufferPositiveExt", .data = 7},
	(MenuEntry){.name = "sceCtrlPeekBufferPositiveExt2", .data = 8},
	(MenuEntry){.name = "sceCtrlReadBufferPositiveExt2", .data = 9},
	(MenuEntry){.name = "sceCtrlPeekBufferNegative2", .data =10},
	(MenuEntry){.name = "sceCtrlReadBufferNegative2", .data =11},    
	(MenuEntry){.name = "ksceCtrlPeekBufferPositive", .data =12},
    (MenuEntry){.name = "ksceCtrlReadBufferPositive", .data =13},
    (MenuEntry){.name = "ksceCtrlPeekBufferNegative", .data =14},
    (MenuEntry){.name = "ksceCtrlReadBufferNegative", .data =15},
	(MenuEntry){.name = "ksceTouchPeek", .data =16},
	(MenuEntry){.name = "ksceTouchRead", .data =17},
	(MenuEntry){.name = "ksceTouchPeekRegion", .data =18},
	(MenuEntry){.name = "ksceTouchReadRegion", .data =19}
};
static struct Menu menu_hooks = (Menu){
	.id = MENU_HOKS_ID, 
	.parent = MENU_MAIN_ID,
	.num = MENU_HOKS_NUM, 
	.name = "$b HOOKS", 
	.noIndent = true,
	.onDraw = onDraw_hooks,
	.entries = menu_hooks_entries};

void menu_initDebugHooks(){
	gui_registerMenu(&menu_hooks);
}
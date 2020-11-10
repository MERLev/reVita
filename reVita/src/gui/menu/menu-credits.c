#include <vitasdkkern.h>
#include "../../common.h"
#include "../../fio/settings.h"
#include "../../fio/theme.h"
#include "../gui.h"
#include "../renderer.h"
#include "../rendererv.h"

static char* credits[] = {
	"reVita           author:$?Mer1e     ",
	" v.1.0           tester:$?bosshunter",
	"",
	"based on remaPSV by $?Rinnegatamante",
	"",
	"Thanks to ",
	" $?Rinnegatamante, remaPSV author",
	" $?S1ngyy, for analogs/gyro support",
	" $?pablojrl123, for menu buttons code",
	" $?Bythos, for various help",
	" $?teakhanirons for various help",
	" $?ellipticaldoor for testing",
	" $?Princess-of-Sleeping for help",
	" $?Derpy for testing",
	" $?W0lfwang for testing",
	" $?TheIronUniverse for testing",
	" $?mantixero for testing",
	" $?MightySashiman for testing",
	" $?Kiiro Yakumo for testing",
	" $?Nino1026 for testing.",
	" - Vita Nuova communinity",
	" - HENkaku communinity",
	"",
	"Includes code from plugins:",
	" - remaPSV, by $?Rinnegatamante",
	" - PSVshell, by $?Electry",
	" - VitaShell, by $?TheFloW",
	" - BetterTrackPlug, by $?FMudanyali"
};

void onDraw_credits(uint menuY){
    int y = menuY;
	int ii = gui_calcStartingIndex(gui_menu->idx, gui_menu->num, gui_lines, gui_lines - 1);
	for (int i = ii; i < min(gui_menu->num, ii + gui_lines); i++) {	
		rendererv_setColor(theme[COLOR_DEFAULT]);
		rendererv_drawString(L_1, y += CHA_H, credits[i]);
	}
	gui_drawFullScroll(ii > 0, ii + gui_lines < gui_menu->num, 
			((float)gui_menu->idx)/(gui_menu->num - (gui_lines - 1) - 1));
}

static struct Menu menu_credits = (Menu){
	.id = MENU_CREDITS_ID, 
	.parent = MENU_MAIN_ID,
	.name = "$? CREDITS", 
	.footer = 	"$CBACK                          $:CLOSE",
	.noIndent = true,
	.onDraw = onDraw_credits,
	.num = 28};

void menu_initCredits(){
	gui_registerMenu(&menu_credits);
}
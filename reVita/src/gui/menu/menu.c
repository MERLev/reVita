#include <vitasdkkern.h>
#include "../../fio/profile.h"
#include "../../fio/settings.h"
#include "../../fio/hotkeys.h"
#include "../../fio/theme.h"
#include "../../main.h"
#include "../../common.h"
#include "../../log.h"
#include "../gui.h"
#include "../renderer.h"
#include "../rendererv.h"
#include "../img/icons.h"
#include "../menu/menu.h"

bool menuHasHeaders(Menu* m){
	for (int i = 0; i < m->num; i++)
		if (m->entries[i].type == HEADER_TYPE)
			return true;
	return false;
}

void drawBoolFRight(int x, int y, bool b){
	b = b % 2;
	rendererv_setColor(theme[b ? COLOR_SUCCESS : COLOR_DANGER]);
	gui_drawStringFRight(0, y, STR_SWITCH[b]);
}

bool onDrawEntry_header(int x, int y, MenuEntry* me, bool isSelected, bool hasHeaders){
    if (me->type == HEADER_TYPE){
        rendererv_setColor(theme[COLOR_HEADER]);
        if (me->icn != ICON_NULL)
            rendererv_drawCharIcon(me->icn, L_1, y);
        rendererv_drawString(x, y, me->name);
        return true;
    }
    return false;
}

bool onDrawEntry_profileEntry(int x, int y, MenuEntry* me, bool isSelected, bool hasHeaders){
    if (me->dataPE != NULL){
        ProfileEntry* pe = me->dataPE;
        gui_setColor(isSelected, profile_isDef(pe));
        if (me->icn != ICON_NULL)
            rendererv_drawCharIcon(me->icn, L_1 + hasHeaders * CHA_W, y);
        rendererv_drawString(x + hasHeaders * CHA_W, y, me->name);

        if (me->dataPEStr == NULL){
            switch (pe->type){
                case TYPE_BOOL: 	drawBoolFRight(0, y, pe->v.b); break;
                case TYPE_INT32: 	gui_drawStringFRight(0, y, "%i", pe->v.i); break;
                case TYPE_UINT32: 	gui_drawStringFRight(0, y, "%u", pe->v.u); break;
            }
        } else {
            switch (pe->type){
                case TYPE_BOOL: 	gui_drawStringFRight(0, y, "%s", me->dataPEStr[pe->v.b]); break;
                case TYPE_INT32: 	gui_drawStringFRight(0, y, "%s", me->dataPEStr[pe->v.i]); break;
                case TYPE_UINT32: 	gui_drawStringFRight(0, y, "%s", me->dataPEStr[pe->v.u]); break;
            }
        }
        return true;
    }
    return false;
}

bool onDrawEntry_button(int x, int y, MenuEntry* me, bool isSelected, bool hasHeaders){
    if (me->dataPEButton != NULL){
        int32_t id = me->dataPEButton->id;
        gui_setColor(isSelected, hotkeys_isDef(id));
        if (me->icn != ICON_NULL)
            rendererv_drawCharIcon(me->icn, L_1 + hasHeaders * CHA_W, y);
        rendererv_drawString(x + hasHeaders * CHA_W, y, me->name);
        char str[10];
        str[0] = '\0';
        gui_generateBtnComboName(str, hotkeys[id].v.u, 6);
        gui_drawStringFRight(0, y, str);
        return true;
    }
    return false;
}

void onDrawEntry_generic(int x, int y, MenuEntry* me, bool isSelected, bool hasHeaders){
    if (onDrawEntry_header(x, y, me, isSelected, hasHeaders))
        return;

    if (onDrawEntry_profileEntry(x, y, me, isSelected, hasHeaders))
        return;

    if (onDrawEntry_button(x, y, me, isSelected, hasHeaders))
        return;

    // Draw generic
    gui_setColor(isSelected, 1);
    if (me->icn != ICON_NULL)
        rendererv_drawCharIcon(me->icn, L_1 + hasHeaders * CHA_W, y);
    rendererv_drawString(x + hasHeaders * CHA_W, y, me->name);
} 

void onDraw_generic(uint32_t menuY){
	int x = 0;
    int y = menuY;
	bool hasHeaders = menuHasHeaders(gui_menu);
	int ii = gui_calcStartingIndex(gui_menu->idx, gui_menu->num, gui_lines, BOTTOM_OFFSET);
	for (int i = ii; i < min(ii + gui_lines, gui_menu->num); i++) {
		MenuEntry* me = &gui_menu->entries[i];
        x = L_1 + CHA_W*(me->icn != ICON_NULL ? 3 : 0);
        y += CHA_H;
		onDrawEntry_generic(x, y, me, gui_menu->idx == i, hasHeaders);
	}

	// Draw scrollbar
	gui_drawFullScroll(ii > 0, ii + gui_lines < gui_menu->num, ((float)gui_menu->idx)/(gui_menu->num-1));
}


void onButton_null(uint32_t btn){
	//Do nothing
}

void onButton_generic(uint32_t btn){
	switch (btn) {
		case SCE_CTRL_DOWN: gui_nextEntry(); break;
		case SCE_CTRL_UP: gui_prevEntry(); break;
		case SCE_CTRL_CIRCLE: gui_openMenuParent(); break;
		case SCE_CTRL_START: gui_close(); break;
	}
}

void onButton_genericEntries(uint32_t btn){
	switch (btn) {
		case SCE_CTRL_LEFT: profile_dec(gui_getEntry()->dataPE, 1); break;
		case SCE_CTRL_RIGHT: profile_inc(gui_getEntry()->dataPE, 1); break;
		case SCE_CTRL_L1: profile_dec(gui_getEntry()->dataPE, 10); break;
		case SCE_CTRL_R1: profile_inc(gui_getEntry()->dataPE, 10); break;
		case SCE_CTRL_SQUARE: profile_resetEntry(gui_getEntry()->dataPE); break;
		default: onButton_generic(btn); break;
	}
}
#include <vitasdkkern.h>
#include <stdlib.h>
#include <psp2/touch.h> 
#include "../../vitasdkext.h"
#include "../../main.h"
#include "../../common.h"
#include "../../fio/settings.h"
#include "../../fio/theme.h"
#include "../gui.h"
#include "../renderer.h"
#include "../rendererv.h"
#include "../../log.h"

#define ANALOG_DEADZONE_PICKER	30

void drawTouchZone(uint32_t panel, TouchPoints2* tz){
	renderer_setColor(panel ? theme[COLOR_TOUCH_BOTTOM] : theme[COLOR_TOUCH_FRONT]);
	TouchPoints2 size = T_SIZE[panel];
	int ax = (float)fbWidth  / (size.b.x - size.a.x) * max(0, (tz->a.x - size.a.x)),
		bx = (float)fbWidth  / (size.b.x - size.a.x) * max(0, (tz->b.x - size.a.x)),
		ay = (float)fbHeight / (size.b.y - size.a.y) * max(0, (tz->a.y - size.a.y)),
		by = (float)fbHeight / (size.b.y - size.a.y) * max(0, (tz->b.y - size.a.y));
	renderer_drawLineThick(ax, ay, bx, ay, 2); // Horizontal 1
	renderer_drawLineThick(ax, by, bx, by, 2); // Horizontal 2
	renderer_drawLineThick(ax, ay, ax, by, 2); // Vertical 1
	renderer_drawLineThick(bx, ay, bx, by, 2); // Vertical 2
	gui_drawTouchPointerN(panel, &tz->a, "1");
	gui_drawTouchPointerN(panel, &tz->b, "2");
}

void drawTouchSwipe(uint32_t panel, TouchPoints2* tz){
	TouchPoints2 size = T_SIZE[panel];
	renderer_setColor(panel ? theme[COLOR_TOUCH_BOTTOM] : theme[COLOR_TOUCH_FRONT]);
	renderer_drawLineThick(
		(float)fbWidth  / (size.b.x - size.a.x) * max(0, (tz->a.x - size.a.x)), 
		(float)fbHeight / (size.b.y - size.a.y) * max(0, (tz->a.y - size.a.y)), 
		(float)fbWidth  / (size.b.x - size.a.x) * max(0, (tz->b.x - size.a.x)), 
		(float)fbHeight / (size.b.y - size.a.y) * max(0, (tz->b.y - size.a.y)), 
		3);
	gui_drawTouchPointerN(panel, &tz->a, "START");
	gui_drawTouchPointerN(panel, &tz->b, "END");
}

void onDrawFB_pickTouchPoint(){
	RemapAction* ra = gui_menu->dataPtr;
	uint32_t panel = (ra->type == REMAP_TYPE_FRONT_TOUCH_POINT) ? SCE_TOUCH_PORT_FRONT : SCE_TOUCH_PORT_BACK;
	gui_drawTouchPointer(panel, &ra->param.tPoint); 
}

void onDrawFB_pickTouchZone(){
	RemapAction* ra = gui_menu->dataPtr;
	uint32_t panel = (ra->type == REMAP_TYPE_FRONT_TOUCH_ZONE) ? SCE_TOUCH_PORT_FRONT : SCE_TOUCH_PORT_BACK;
	drawTouchZone(panel, &ra->param.tPoints); 
}

void onDrawFB_pickTouchSwipe(){
	RemapAction* ra = gui_menu->dataPtr;
	uint32_t panel = (ra->type == REMAP_TYPE_FRONT_TOUCH_POINT) ? SCE_TOUCH_PORT_FRONT : SCE_TOUCH_PORT_BACK;
	drawTouchSwipe(panel, &ra->param.tPoints); 
}

// Set custom touch point xy using LS and RS
void analogTouchPicker(TouchPoint* tp, SceCtrlData *ctrl, int port, int isLeftAnalog){
	int multi = 1 + btn_has(ctrl->buttons, SCE_CTRL_L1)  + btn_has(ctrl->buttons, SCE_CTRL_R1);
	int shiftX = ((float)((isLeftAnalog ? ctrl->lx : ctrl->rx) - 127));
	int shiftY = ((float)((isLeftAnalog ? ctrl->ly : ctrl->ry) - 127));
	TouchPoints2 size = T_SIZE[port];
	if (abs(shiftX) > ANALOG_DEADZONE_PICKER)
		tp->x = clamp(tp->x + shiftX * multi / 8, size.a.x, size.b.x);
	if (abs(shiftY) > ANALOG_DEADZONE_PICKER)
		tp->y = clamp(tp->y + shiftY * multi / 8, size.a.y, size.b.y);
}

// Set custom touch point xy using touch
void touchPicker(TouchPoint* tp, SceTouchPortType port, int num){
	SceTouchData std;
	int ret = ksceTouchPeek_internal(port, &std, 1);
	if (ret && std.reportNum > num){
		tp->x = std.report[num].x;
		tp->y = std.report[num].y;
	}
}

void onInput_touchPicker(SceCtrlData *ctrl){
	RemapAction* ra = gui_menu->dataPtr;
	SceTouchPortType port = (ra->type == REMAP_TYPE_FRONT_TOUCH_POINT || ra->type == REMAP_TYPE_FRONT_TOUCH_ZONE) ?
		SCE_TOUCH_PORT_FRONT : SCE_TOUCH_PORT_BACK;
	if ((ra->type == REMAP_TYPE_FRONT_TOUCH_POINT || ra->type == REMAP_TYPE_BACK_TOUCH_POINT) && ra->action != REMAP_TOUCH_SWIPE){
		touchPicker(&ra->param.tPoint, port, 0);
		analogTouchPicker(&ra->param.tPoint, ctrl, port, true);
		analogTouchPicker(&ra->param.tPoint, ctrl, port, false);
	} else if (ra->type == REMAP_TYPE_FRONT_TOUCH_ZONE || ra->type == REMAP_TYPE_BACK_TOUCH_ZONE || 
			((ra->type == REMAP_TYPE_FRONT_TOUCH_POINT || ra->type == REMAP_TYPE_BACK_TOUCH_POINT) && ra->action == REMAP_TOUCH_SWIPE)){
		TouchPoint* tpA = &ra->param.tPoints.a;
		TouchPoint* tpB = &ra->param.tPoints.b;
		touchPicker(&ra->param.tPoints.a, port, 0);
		touchPicker(&ra->param.tPoints.b, port, 1);
		analogTouchPicker(tpA, ctrl, port, true);
		analogTouchPicker(tpB, ctrl, port, false);
	}
}

void onButton_pickTouchPoint(uint32_t btn){
	RemapAction* ra = gui_menu->dataPtr;
	int port = ra->type == REMAP_TYPE_FRONT_TOUCH_POINT ? SCE_TOUCH_PORT_FRONT : SCE_TOUCH_PORT_BACK;
	TouchPoints2 size = T_SIZE[port];
	switch (btn) {
		case SCE_CTRL_CROSS:
			if(gui_menu->next == MENU_REMAP_ID){
				if (ui_ruleEditedIdx >= 0) 
					profile.remaps[ui_ruleEditedIdx] = ui_ruleEdited;
				else
					profile_addRemapRule(ui_ruleEdited);
			}
			gui_openMenuNext();
		case SCE_CTRL_RIGHT:
			switch (gui_getEntry()->dataUint){
				case 0: ra->param.tPoint.x = min(ra->param.tPoint.x + 1, size.b.x); break;
				case 1: ra->param.tPoint.y = min(ra->param.tPoint.y + 1, size.b.y); break;
			}
			break;
		case SCE_CTRL_LEFT:
			switch (gui_getEntry()->dataUint){
				case 0: ra->param.tPoint.x = max(ra->param.tPoint.x - 1, size.a.x); break;
				case 1: ra->param.tPoint.y = max(ra->param.tPoint.y - 1, size.a.y); break;
			}
			break;
		case SCE_CTRL_CIRCLE: gui_openMenuPrev();
		default: onButton_generic(btn);
	}
}

void onButton_pickTouchZone(uint32_t btn){
	RemapAction* ra = gui_menu->dataPtr;
	int port = ra->type == REMAP_TYPE_FRONT_TOUCH_POINT ? SCE_TOUCH_PORT_FRONT : SCE_TOUCH_PORT_BACK;
	TouchPoints2 size = T_SIZE[port];
	switch (btn) {
		case SCE_CTRL_CROSS:
			if(gui_menu->next == MENU_REMAP_ID){
				if (ui_ruleEditedIdx >= 0) 
					profile.remaps[ui_ruleEditedIdx] = ui_ruleEdited;
				else
					profile_addRemapRule(ui_ruleEdited);
			}
			gui_openMenuNext();
		case SCE_CTRL_RIGHT:
			switch (gui_getEntry()->dataUint){
				case 0: ra->param.tPoints.a.x = min(ra->param.tPoints.a.x + 1, size.b.x); break;
				case 1: ra->param.tPoints.a.y = min(ra->param.tPoints.a.y + 1, size.b.y); break;
				case 2: ra->param.tPoints.b.x = min(ra->param.tPoints.b.x + 1, size.b.x); break;
				case 3: ra->param.tPoints.b.y = min(ra->param.tPoints.b.y + 1, size.b.y); break;
			}
			break;
		case SCE_CTRL_LEFT:
			switch (gui_getEntry()->dataUint){
				case 0: ra->param.tPoints.a.x = max(ra->param.tPoints.a.x - 1, size.a.x); break;
				case 1: ra->param.tPoints.a.y = max(ra->param.tPoints.a.y - 1, size.a.y); break;
				case 2: ra->param.tPoints.b.x = max(ra->param.tPoints.b.x - 1, size.a.x); break;
				case 3: ra->param.tPoints.b.y = max(ra->param.tPoints.b.y - 1, size.a.y); break;
			}
			break;
		case SCE_CTRL_CIRCLE: gui_openMenuPrev();
		default: onButton_generic(btn);
	}
}

void onDraw_pickTouchPoint(uint menuY){
    int y = menuY;
	int ii = gui_calcStartingIndex(gui_menu->idx, gui_menu->num , gui_lines, BOTTOM_OFFSET);	
	RemapAction* ra = gui_menu->dataPtr;
	for (int i = ii; i < min(ii + gui_lines, gui_menu->num); i++) {	
		int32_t id = gui_menu->entries[i].dataUint;
		int coord = (id == 0) ? ra->param.tPoint.x : ra->param.tPoint.y;
		gui_setColor(i == gui_menu->idx, 1);
		rendererv_drawStringF(L_2, y += CHA_H, gui_menu->entries[i].name);
		gui_drawStringFRight(0, y, "%hu", coord);
	}
	gui_drawFullScroll(ii > 0, ii + gui_lines < gui_menu->num, ((float)gui_menu->idx) / (gui_menu->num - 1));
}

void onDraw_pickTouchZone(uint menuY){
    int y = menuY;
	int ii = gui_calcStartingIndex(gui_menu->idx, gui_menu->num , gui_lines, BOTTOM_OFFSET);	
	RemapAction* ra = gui_menu->dataPtr;
	for (int i = ii; i < min(ii + gui_lines, gui_menu->num); i++) {	
		int32_t id = gui_menu->entries[i].dataUint;
		int coord = 0;
		switch (id){
			case 0: coord = ra->param.tPoints.a.x; break;
			case 1: coord = ra->param.tPoints.a.y; break;
			case 2: coord = ra->param.tPoints.b.x; break;
			case 3: coord = ra->param.tPoints.b.y; break;}
		gui_setColor(i == gui_menu->idx, 1);
		rendererv_drawStringF(L_2, y += CHA_H, gui_menu->entries[i].name);
		gui_drawStringFRight(0, y, "%hu", coord);
	}
	gui_drawFullScroll(ii > 0, ii + gui_lines < gui_menu->num, ((float)gui_menu->idx) / (gui_menu->num - 1));
}

static struct MenuEntry menu_pick_touch_point_entries[] = {
	(MenuEntry){.name = "Point x", .dataUint = 0},
	(MenuEntry){.name = "      y", .dataUint = 1}};
static struct Menu menu_pick_touch_point = (Menu){
	.id = MENU_PICK_TOUCH_POINT_ID, 
	.parent = MENU_REMAP_EMU_TYPE_ID,
	.name = "$i SELECT TOUCH POINT", 
	.footer = 	"$U$uCHANGE $<$>CHANGE ${$}FASTER       "
				"$CBACK                          $:CLOSE", 
	.onInput = onInput_touchPicker,
	.onButton = onButton_pickTouchPoint,
	.onDraw = onDraw_pickTouchPoint,
	.onDrawFB = onDrawFB_pickTouchPoint,
	.num = SIZE(menu_pick_touch_point_entries), 
	.entries = menu_pick_touch_point_entries};

static struct MenuEntry menu_pick_touch_zone_entries[] = {
	(MenuEntry){.name = "Point 1 x", .dataUint = 0},
	(MenuEntry){.name = "        y", .dataUint = 1},
	(MenuEntry){.name = "Point 2 x", .dataUint = 2},
	(MenuEntry){.name = "        y", .dataUint = 3}};
static struct Menu menu_pick_touch_zone = (Menu){
	.id = MENU_PICK_TOUCH_ZONE_ID, 
	.parent = MENU_REMAP_TRIGGER_TYPE_ID,
	.name = "$i SELECT TOUCH ZONE", 
	.footer = 	"$U$uCHANGE $<$>CHANGE ${$}FASTER       "
				"$CBACK                          $:CLOSE", 
	.onInput = onInput_touchPicker,
	.onButton = onButton_pickTouchZone,
	.onDraw = onDraw_pickTouchZone,
	.onDrawFB = onDrawFB_pickTouchZone,
	.num = SIZE(menu_pick_touch_zone_entries), 
	.entries = menu_pick_touch_zone_entries};

static struct MenuEntry menu_pick_touch_swipe_entries[] = {
	(MenuEntry){.name = "Start Point x", .dataUint = 0},
	(MenuEntry){.name = "            y", .dataUint = 1},
	(MenuEntry){.name = "End   Point x", .dataUint = 2},
	(MenuEntry){.name = "            y", .dataUint = 3}};
static struct Menu menu_pick_touch_swipe = (Menu){
	.id = MENU_PICK_TOUCH_SWIPE_ID, 
	.parent = MENU_REMAP_TRIGGER_TYPE_ID,
	.name = "$j SELECT SWIPE POINTS", 
	.footer = 	"$U$uCHANGE $<$>CHANGE ${$}FASTER       "
				"$CBACK                          $:CLOSE", 
	.onInput = onInput_touchPicker,
	.onButton = onButton_pickTouchZone,
	.onDraw = onDraw_pickTouchZone,
	.onDrawFB = onDrawFB_pickTouchSwipe,
	.num = SIZE(menu_pick_touch_swipe_entries), 
	.entries = menu_pick_touch_swipe_entries};

void menu_initPickTouch(){
	gui_registerMenu(&menu_pick_touch_point);
	gui_registerMenu(&menu_pick_touch_zone);
	gui_registerMenu(&menu_pick_touch_swipe);
}
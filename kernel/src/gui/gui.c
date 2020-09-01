#include <vitasdkkern.h>
#include "../fio/profile.h"
#include "../fio/settings.h"
#include "../fio/hotkeys.h"
#include "../fio/theme.h"
#include "../main.h"
#include "../common.h"
#include "../log.h"
#include "gui.h"
#include "renderer.h"
#include "img/icons.h"
#include "menu/menu.h"

#define STR_SIZE                  40
#define DELAY_LONGPRESS   	400*1000	//0.400 sec
#define DELAY_REPEAT   		 60*1000	//0.070 sec

typedef struct BtnInfo{
	uint32_t btn;
	uint8_t btnId;
	bool isPressed;
	bool isLongPressed;
	int64_t tickOnPress;
	int64_t tickLastRepeat;
}BtnInfo;
BtnInfo btns[HW_BUTTONS_NUM];

static uint32_t ticker;

//Active emulated touchpoints
static EmulatedTouch etFront, etBack;

const char* STR_BTN_S[HW_BUTTONS_NUM] = {
	"$X", "$C", "$T", "$S", "$:", "$;", "$[", "$]", 
	"$^", "$>", "$<", "$v", "${", "$}", "$(", "$)", 
	"$+", "$-", "$p", "$P", "$t"
};
const char* STR_YN[2] = {
	"No", "Yes"
};
const char* STR_SWITCH[2] = {
	"$@$#", "$~$`"
};

Menu* menus[MENU_ID__NUM];
Menu* gui_menu;

SceUID mem_uid;
int64_t tickUIOpen = 0;
uint8_t gui_isOpen = false;
uint8_t gui_lines = 10;

struct MenuEntry* gui_getEntry(){
	return &gui_menu->entries[gui_menu->idx % gui_menu->num];
}
void gui_generateBtnComboName(char* str, uint32_t btns, int max){
	int i = -1;
	int counter = 0;
	while (++i < HW_BUTTONS_NUM){
		if (btn_has(btns, HW_BUTTONS[i])) {
			if (counter >=  max){
				str[strlen(str) - 2] = '.';
				str[strlen(str) - 1] = '.';
				break;
			} else {
				counter++;
				strcat(str, STR_BTN_S[i]);
			}
		}
	}
}

//Calculate starting index for scroll menu
int gui_calcStartingIndex(int idx, int entriesNum, int screenEntries, int bottomOffset){
	int ret = max(0, idx - (screenEntries - bottomOffset - 1));
	while (ret > 0 && (entriesNum - ret) < screenEntries) ret--;
	return ret;
}
void gui_setColorHeader(uint8_t isCursor){
	if (isCursor){
		renderer_setColor(theme[COLOR_CURSOR_HEADER]);
	} else {
		renderer_setColor(theme[COLOR_HEADER]);
	}
}
void gui_setColor(uint8_t isCursor, uint8_t isDefault){
	if (isCursor){
		if (isDefault) renderer_setColor(theme[COLOR_CURSOR_DEFAULT]);
		else renderer_setColor(theme[COLOR_CURSOR_ACTIVE]);
	} else {
		if (isDefault) renderer_setColor(theme[COLOR_DEFAULT]);
		else renderer_setColor(theme[COLOR_ACTIVE]);
	}
}
void gui_setColorExt(uint8_t isCursor, uint8_t isDefault, uint8_t isDisabled){
	if (isCursor){
		if     (isDisabled) renderer_setColor(theme[COLOR_CURSOR_DANGER]);
		else if (isDefault) renderer_setColor(theme[COLOR_CURSOR_DEFAULT]);
		else                renderer_setColor(theme[COLOR_CURSOR_ACTIVE]);
	} else {
		if     (isDisabled) renderer_setColor(theme[COLOR_DANGER]);
		else if (isDefault) renderer_setColor(theme[COLOR_DEFAULT]);
		else                renderer_setColor(theme[COLOR_ACTIVE]);
	}
}
void gui_drawStringFRight(int x, int y, const char *format, ...){
	char str[512] = { 0 };
	va_list va;

	va_start(va, format);
	vsnprintf(str, 512, format, va);
	va_end(va);

	renderer_drawString(UI_WIDTH - (strlen(str) + 2) * CHA_W - x, y, str);
}
void gui_drawBoolFRight(int x, int y, bool b){
	b = b % 2;
	renderer_setColor(b ? 0x00329e15 : theme[COLOR_DANGER]);
	gui_drawStringFRight(0, y, STR_SWITCH[b]);
}
void gui_drawScroll(int8_t up, int8_t down){
	renderer_setColor(theme[COLOR_HEADER]);
	if (up)
		renderer_drawImage(UI_WIDTH - ICN_ARROW_X - 3, HEADER_HEIGHT + 3, ICN_ARROW_X, ICN_ARROW_Y, ICN_ARROW_UP);
	if (down)
		renderer_drawImage(UI_WIDTH - ICN_ARROW_X - 3, UI_HEIGHT - HEADER_HEIGHT - ICN_ARROW_X - 3, ICN_ARROW_X, ICN_ARROW_Y, ICN_ARROW_DOWN);
}
void gui_drawFullScroll(int8_t up, int8_t down, float pos){
	if (!up && !down) return;
	renderer_setColor(theme[COLOR_HEADER]);
	uint16_t calculatedPos = HEADER_HEIGHT + 4 + ICN_ARROW_Y + pos * (UI_HEIGHT - 2 * HEADER_HEIGHT - 2 * ICN_ARROW_Y - ICN_SLIDER_Y - 8);
	renderer_drawImage(UI_WIDTH - ICN_ARROW_X - 3, calculatedPos, ICN_SLIDER_X, ICN_SLIDER_Y, ICN_SLIDER);
	gui_drawScroll(1,1);
}
void gui_drawEditPointer(uint16_t x, uint16_t y){
	renderer_drawImage(x, y, ICN_ARROW_X, ICN_ARROW_Y, (ticker % 32 < 16) ? ICN_ARROW_LEFT : ICN_ARROW_RIGHT);
}
void gui_drawEntry(uint8_t x, uint8_t y, MenuEntry* me, bool focus){
	if (me == NULL) return;
	focus = focus % 2;
	if (me->type == HEADER_TYPE){
		renderer_setColor(theme[COLOR_HEADER]);
		renderer_drawString(x, y, me->name);
		return;
	}
	if (me->dataPE == NULL) return;
	ProfileEntry* pe = me->dataPE;
	gui_setColor(focus, profile_isDef(pe));
	if (me->icn == ICON_NULL){
		renderer_drawString(x, y, me->name);
	} else {
		renderer_drawCharIcon(me->icn, x, y);
		renderer_drawString(x + CHA_W*3, y, me->name);
	}
	switch (pe->type){
		case TYPE_BOOL:
			gui_drawBoolFRight(0, y, pe->v.b);
			break;
		case TYPE_INT32:
			gui_drawStringFRight(0, y, "%i", pe->v.i);
			break;
		case TYPE_UINT32:
			gui_drawStringFRight(0, y, "%u", pe->v.u);
			break;
		default: break;
	}
}
void drawHeader(){
	renderer_drawRectangle(0, 0, UI_WIDTH, HEADER_HEIGHT - 1, theme[COLOR_BG_HEADER]);//BG
	renderer_drawRectangle(0, HEADER_HEIGHT - 1, UI_WIDTH, 1, theme[COLOR_HEADER]);//Separator
	if (gui_menu->id == MENU_MAIN_ID){
		renderer_drawStringF(L_0, 3, "     remaPSV2 v.%s", VERSION);
		gui_drawStringFRight(0, 2, titleid);
		if (settings[SETT_REMAP_ENABLED].v.b){
			renderer_setColor(0x00329e15);
			renderer_drawStringF(L_0, 3, "$~$`", VERSION);
		} else {
			renderer_setColor(theme[COLOR_DANGER]);
			renderer_drawStringF(L_0, 3, "$@$#", VERSION);
		}
	} else	
		renderer_drawString(L_0, 3, gui_menu->name);
	
}
void drawFooter(){
	renderer_drawRectangle(0, UI_HEIGHT - HEADER_HEIGHT, UI_WIDTH, 1, theme[COLOR_HEADER]);//Separator
	renderer_drawRectangle(0, UI_HEIGHT - (HEADER_HEIGHT - 1), UI_WIDTH, HEADER_HEIGHT - 1, theme[COLOR_BG_HEADER]);//BG
	renderer_setColor(theme[COLOR_HEADER]);   
	if (gui_menu->footer)                                                            
		renderer_drawStringF(L_0, UI_HEIGHT-HEADER_HEIGHT + 4, gui_menu->footer);
}
void drawIndent(){
	int y = (gui_menu->idx - gui_calcStartingIndex(gui_menu->idx, gui_menu->num, gui_lines, BOTTOM_OFFSET)) * CHA_H
		+ HEADER_HEIGHT + CHA_H / 2;
	renderer_drawRectangle(L_1 - 5, y - 1, UI_WIDTH - 2 * L_1 + 4, CHA_H + 2, theme[COLOR_BG_HEADER]);//BG
	renderer_setColor(theme[COLOR_HEADER]);   
}

void onDraw_generic(uint32_t menuY){
    int y = menuY;
	int ii = gui_calcStartingIndex(gui_menu->idx, gui_menu->num, gui_lines, BOTTOM_OFFSET);
	for (int i = ii; i < min(ii + gui_lines, gui_menu->num); i++) {
		MenuEntry* me = &gui_menu->entries[i];
		gui_setColor(i == gui_menu->idx, 1);
		if (me->icn == ICON_NULL){
			renderer_drawString(L_1, y += CHA_H, me->name);
		} else {
			renderer_drawCharIcon(me->icn, L_1, y += CHA_H);
			renderer_drawString(L_1 + CHA_W*3, y, me->name);
		}
	}
}

void gui_drawTouchPointer(uint32_t panel, TouchPoint* tp){
	TouchPoints2 size = (panel == SCE_TOUCH_PORT_FRONT) ? T_FRONT_SIZE : T_BACK_SIZE;
	int x = (float)fbWidth / (size.b.x - size.a.x) * (tp->x - size.a.x);
	int y = (float)fbHeight / (size.b.y - size.a.y) * (tp->y - size.a.y);
	renderer_setColor(theme[COLOR_TOUCH]);
	renderer_drawImageDirectlyToFB(x - 11, y - 5, ICN_TOUCH_X, ICN_TOUCH_Y, ICN_TOUCH);
}

void drawEmulatedPointersForPanel(uint32_t panel){
	EmulatedTouch* et = &etBack;
	if (panel == SCE_TOUCH_PORT_FRONT)
		et = &etFront;
	for (int i = 0; i < et->num; i++)
		if (et->reports[i].isSwipe){
			if (profile.entries[PR_TO_DRAW_SWIPE].v.b)
				gui_drawTouchPointer(panel, &et->reports[i].swipeCurrentPoint);
		} else if (et->reports[i].isSmartSwipe){
			if (profile.entries[PR_TO_DRAW_SMART_SWIPE].v.b)
				gui_drawTouchPointer(panel, &et->reports[i].swipeEndPoint);
		} else {
			if (profile.entries[PR_TO_DRAW_POINT].v.b)
				gui_drawTouchPointer(panel, &et->reports[i].point);
		}
}

void drawBody() {
	renderer_drawRectangle(0, HEADER_HEIGHT, UI_WIDTH, UI_HEIGHT -  2 * HEADER_HEIGHT, theme[COLOR_BG_BODY]);//BG
	if (!gui_menu->noIndent)
		drawIndent();
	//Draw menu
	uint32_t menuY = HEADER_HEIGHT - CHA_H / 2;
	gui_lines = ((float)(UI_HEIGHT - 2 * HEADER_HEIGHT)) / CHA_H - 1;
    
    if (gui_menu->onDraw)
        gui_menu->onDraw(menuY);
    else
        onDraw_generic(menuY);
}

//Draw directly to FB to overlay over everything else;
void drawDirectlyToFB(){
}

void gui_draw(const SceDisplayFrameBuf *pParam){
	ticker++;
	if (gui_isOpen) {
		renderer_setFB(pParam);
		drawHeader();
		drawBody();
		drawFooter();
		renderer_writeToFB(tickUIOpen);
		if (gui_menu->onDrawFB != NULL)
			gui_menu->onDrawFB();
	} else {
		renderer_setFB(pParam);
		drawEmulatedPointersForPanel(SCE_TOUCH_PORT_FRONT);
		drawEmulatedPointersForPanel(SCE_TOUCH_PORT_BACK);
	}
}

void gui_updateEmulatedTouch(SceTouchPortType panel, EmulatedTouch et){
	if (panel == SCE_TOUCH_PORT_FRONT)
		etFront = et;
	else 
		etBack = et;
}

void onButton_null(uint32_t btn){
	//Do nothing
}

void onButton_generic(uint32_t btn){
	switch (btn) {
		case SCE_CTRL_DOWN: gui_nextEntry(); break;
		case SCE_CTRL_UP: gui_prevEntry(); break;
		case SCE_CTRL_CIRCLE: gui_openMenuParent(); break;
	}
}

void onButton_genericEntries(uint32_t btn){
	switch (btn) {
		case SCE_CTRL_RIGHT: profile_inc(gui_getEntry()->dataPE, 1); break;
		case SCE_CTRL_LEFT: profile_dec(gui_getEntry()->dataPE, 1); break;
		case SCE_CTRL_SQUARE: profile_resetEntry(gui_getEntry()->dataPE); break;
		default: onButton_generic(btn); break;
	}
}

void ctrl_onInput(SceCtrlData *ctrl) {
	if (ctrl->buttons & HW_BUTTONS[hotkeys[HOTKEY_MENU].v.u])
		return; //Menu trigger butoons should not trigger any menu actions on menu open

	if (gui_menu->onInput)
		gui_menu->onInput(ctrl);

	int64_t tick = ksceKernelGetSystemTimeWide();
	for (int i = 0; i < HW_BUTTONS_NUM; i++){
		if (btn_has(ctrl->buttons, HW_BUTTONS[i])){
			if (btns[i].isPressed){
				if (btns[i].isLongPressed){
					if (tick > btns[i].tickLastRepeat + DELAY_REPEAT){
						btns[i].tickLastRepeat = tick;
						//OnLongPress event
						if (gui_menu->onButton)
							gui_menu->onButton(HW_BUTTONS[i]);
						else 
							onButton_generic(HW_BUTTONS[i]);
					}
				} else {
					if (tick > btns[i].tickOnPress + DELAY_LONGPRESS){
						btns[i].isLongPressed = true;
						btns[i].tickLastRepeat = btns[i].tickOnPress;
						//OnLongPressStart event
					}
				}
			} else {
				btns[i].isPressed = true;
				btns[i].tickOnPress = tick;
				//OnPress event
				if (gui_menu->onButton)
					gui_menu->onButton(HW_BUTTONS[i]);
				else 
					onButton_generic(HW_BUTTONS[i]);
			}
		} else {
			// if (btns[i].isPressed){
				//OnRelease event
			// }
			btns[i].isPressed = false;
			btns[i].isLongPressed = false;
		}
	}
}

void ui_fixIdx(Menu* m, int idx){
	for (int i = 0; i < m->num; i++)
		if(m->entries[(idx + i) % m->num].type != HEADER_TYPE){
			m->idx = (idx + i) % m->num;
			break;
		}
}

void gui_registerMenu(Menu* m){
	menus[m->id] = m;
	ui_fixIdx(m, 0);
}

void gui_setIdx(int idx){
	if (idx < 0 || idx >= gui_menu->num){
		gui_setIdx(0);
		return;
	}
	gui_menu->idx = idx;
	ui_fixIdx(gui_menu, idx);
}

void open(enum MENU_ID id){
	if (menus[id]->onBuild)
		menus[id]->onBuild(menus[id]);
	gui_menu = menus[id];
	gui_setIdx(gui_menu->idx);
}
void gui_openMenu(enum MENU_ID id){
	menus[id]->prev = gui_menu->id;
	open(id);
}
void gui_openMenuSmart(enum MENU_ID id, enum MENU_ID prevId, enum MENU_ID nextId){
	menus[id]->next = nextId;
	menus[id]->prev = prevId;
	open(id);
}
void gui_openMenuSmartU(enum MENU_ID id, enum MENU_ID prevId, enum MENU_ID nextId, uint32_t dataUint){
	menus[id]->dataUint = dataUint;
	gui_openMenuSmart(id, prevId, nextId);
}
void gui_openMenuSmartI(enum MENU_ID id, enum MENU_ID prevId, enum MENU_ID nextId, int32_t dataInt){
	menus[id]->dataInt = dataInt;
	gui_openMenuSmart(id, prevId, nextId);
}
void gui_openMenuSmartPtr(enum MENU_ID id, enum MENU_ID prevId, enum MENU_ID nextId, void* dataPtr){
	menus[id]->dataPtr = dataPtr;
	gui_openMenuSmart(id, prevId, nextId);
}
void gui_openMenuPrev(){
	open(gui_menu->prev);
}
void gui_openMenuNext(){
	open(gui_menu->next);
}
void gui_openMenuParent(){
	open(gui_menu->parent);
}
void gui_nextEntry(){
	if (gui_menu->id == MENU_CREDITS_ID || gui_menu->id == MENU_HOKS_ID)
		gui_setIdx(min(gui_menu->idx + 1, gui_menu->num - gui_lines));
	else 
		gui_setIdx((gui_menu->idx + 1) % gui_menu->num);
}
void gui_prevEntry(){
	if (gui_menu->id == MENU_CREDITS_ID || gui_menu->id == MENU_HOKS_ID){
		gui_setIdx(max(0, gui_menu->idx - 1));
	} else {
		int idx = (gui_menu->num + gui_menu->idx - 1) % gui_menu->num;
		gui_menu->idx = idx;
		for (int i = 0; i < gui_menu->num; i++) //Search for prev non-header
			if(gui_menu->entries[(gui_menu->num + idx - i) % gui_menu->num].type != HEADER_TYPE){
				gui_menu->idx = (gui_menu->num + idx - i) % gui_menu->num;
				break;
			}
	}
}

void gui_onInput(SceCtrlData *ctrl){
	ctrl_onInput(ctrl);
}

void gui_open(const SceDisplayFrameBuf *pParam){
	gui_menu = menus[MENU_MAIN_ID]; //&menu_main;
	gui_setIdx(0);
	gui_isOpen = true;
	tickUIOpen = ksceKernelGetSystemTimeWide();
	LOG("tickUIOpen=%lli\n", tickUIOpen);
}
void gui_close(){
	gui_isOpen = false;
	profile.version = ksceKernelGetSystemTimeWide();
	log_flush();
}

void gui_init(){
    renderer_init(UI_WIDTH, UI_HEIGHT);

	menu_initMain();
	menu_initAnalog();
	menu_initTouch();
	menu_initGyro();
	menu_initController();
	menu_initSettings();
	menu_initHotkeys();

	menu_initCredits();
	menu_initProfile();

	menu_initDebugHooks();
	menu_initDebugButtons();

	menu_initPickButton();
	menu_initPickAnalog();
	menu_initPickTouch();
	menu_initRemap();

	gui_menu = menus[MENU_MAIN_ID];
	gui_setIdx(0);
}
void gui_destroy(){
	renderer_destroy();
}
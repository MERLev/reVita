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
#include "rendererv.h"
#include "img/icons.h"
#include "menu/menu.h"

#define STR_SIZE                  (39)
#define BLACK 				0xFF000000
#define DELAY_LONGPRESS   	(400##000)	// 0.400 sec
#define DELAY_REPEAT   		 (60##000)	// 0.070 sec
#define DELAY_FOOTER   	 (2##500##000)	// 2     sec
#define TIME_MUTE_KEYS   (1##000##000)	// 0.333 sec

typedef struct BtnInfo{
	uint32_t btn;
	bool isPressed;
	bool isLongPressed;
	int64_t tickOnPress;
	int64_t tickLastRepeat;
}BtnInfo;
BtnInfo btns[HW_BUTTONS_NUM];

static uint32_t ticker;

//Active emulated touchpoints
static EmulatedTouch et[SCE_TOUCH_PORT_MAX_NUM];
static SceTouchData td[SCE_TOUCH_PORT_MAX_NUM];

const char* STR_BTN_S[HW_BUTTONS_NUM] = {
	"$X", "$C", "$T", "$S", 
	"$^", "$>", "$<", "$v", 
	"$:", "$;", 
	"${", "$}", "$,", "$.", "$(", "$)", 
	"$+", "$-", "$p", "$P", 
	"$t", "$h"
};
const char* STR_YN[2] = {
	"No", "Yes"
};
const char* STR_SWITCH[2] = {
	"$@$#", "$~$`"
};

enum MENU_ID whitelistedMenus[] = {
	MENU_MAIN_ID,
	MENU_MAIN_PROFILE_ID,
	MENU_MAIN_SETTINGS_ID,
	MENU_MAIN_DEVELOPER_ID,
	MENU_ANALOG_ID,
	MENU_TOUCH_ID,
	MENU_TURBO_ID,
	MENU_GYRO_ID,
	MENU_CONTROLLER_ID,
	MENU_MORE_ID,
	MENU_HOKS_ID,
	MENU_DEBUG_BUTTONS_ID,
	MENU_SETT_ID,
	MENU_POPUP_ID,
	MENU_HOTKEYS_ID,
	MENU_CREDITS_ID,
	MENU_PROFILE_ID,
	MENU_SAVEMANAGER_ID,
	MENU_REMAP_ID
};

Menu* menus[MENU_ID__NUM];
Menu* gui_menu;

SceUID mem_uid;
int64_t tickUIOpen = 0;
int64_t tickUIClose = 0;
int64_t tickMenuOpen = 0;
uint8_t gui_isOpen = false;
uint8_t gui_isBlankFrame = false;
uint8_t gui_lines = 10;
static SceUID mutex_gui_uid = -1;

// Popup params
static char popupHeader[40];
static char popupMessage[40];
static int popupTTL = 0;
static SceInt64 popupTick = -1;
static uint32_t popupColor;
static bool isPopupActive = false;

bool isMenuAllowed(){
	for (int i = 0; i < SIZE(whitelistedMenus); i++)
		if (gui_menu->id == whitelistedMenus[i])
			return true;
	return false;
}

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
		rendererv_setColor(theme[COLOR_CURSOR_HEADER]);
	} else {
		rendererv_setColor(theme[COLOR_HEADER]);
	}
}
void gui_setColor(uint8_t isCursor, uint8_t isDefault){
	if (isCursor){
		if (isDefault) rendererv_setColor(theme[COLOR_CURSOR_DEFAULT]);
		else rendererv_setColor(theme[COLOR_CURSOR_ACTIVE]);
	} else {
		if (isDefault) rendererv_setColor(theme[COLOR_DEFAULT]);
		else rendererv_setColor(theme[COLOR_ACTIVE]);
	}
}
void gui_setColorExt(uint8_t isCursor, uint8_t isDefault, uint8_t isDisabled){
	if (isCursor){
		if     (isDisabled) rendererv_setColor(theme[COLOR_CURSOR_DANGER]);
		else if (isDefault) rendererv_setColor(theme[COLOR_CURSOR_DEFAULT]);
		else                rendererv_setColor(theme[COLOR_CURSOR_ACTIVE]);
	} else {
		if     (isDisabled) rendererv_setColor(theme[COLOR_DANGER]);
		else if (isDefault) rendererv_setColor(theme[COLOR_DEFAULT]);
		else                rendererv_setColor(theme[COLOR_ACTIVE]);
	}
}
void gui_drawStringFRight(int x, int y, const char *format, ...){
	char str[512] = { 0 };
	va_list va;

	va_start(va, format);
	vsnprintf(str, 512, format, va);
	va_end(va);

	rendererv_drawString(UI_WIDTH - (strlen(str) + 2) * CHA_W - x, y, str);
}
void gui_drawScroll(int8_t up, int8_t down){
	rendererv_setColor(theme[COLOR_HEADER]);
	if (up)
		rendererv_drawImage(UI_WIDTH - ICN_ARROW_X - 3, HEADER_HEIGHT + 3, ICN_ARROW_X, ICN_ARROW_Y, ICN_ARROW_UP);
	if (down)
		rendererv_drawImage(UI_WIDTH - ICN_ARROW_X - 3, UI_HEIGHT - HEADER_HEIGHT - ICN_ARROW_X - 3, ICN_ARROW_X, ICN_ARROW_Y, ICN_ARROW_DOWN);
}
void gui_drawFullScroll(int8_t up, int8_t down, float pos){
	if (!up && !down) return;
	rendererv_setColor(theme[COLOR_HEADER]);
	uint16_t calculatedPos = HEADER_HEIGHT + 4 + ICN_ARROW_Y + pos * (UI_HEIGHT - 2 * HEADER_HEIGHT - 2 * ICN_ARROW_Y - ICN_SLIDER_Y - 8);
	rendererv_drawImage(UI_WIDTH - ICN_ARROW_X - 3, calculatedPos, ICN_SLIDER_X, ICN_SLIDER_Y, ICN_SLIDER);
	gui_drawScroll(1,1);
}
void gui_drawEditPointer(uint16_t x, uint16_t y){
	rendererv_drawImage(x, y, ICN_ARROW_X, ICN_ARROW_Y, (ticker % 32 < 16) ? ICN_ARROW_LEFT : ICN_ARROW_RIGHT);
}
void drawHeader(){
	rendererv_drawRectangle(0, 0, UI_WIDTH, HEADER_HEIGHT - 1, theme[COLOR_BG_HEADER]);//BG
	rendererv_drawRectangle(0, HEADER_HEIGHT - 1, UI_WIDTH, 1, theme[isSafeBoot ? COLOR_ACTIVE : COLOR_HEADER]);//Separator
	rendererv_setColor(theme[isSafeBoot ? COLOR_ACTIVE : COLOR_HEADER]);
	if (gui_menu->onDrawHeader != NULL)
		gui_menu->onDrawHeader();
	else 
		rendererv_drawString(L_0, 3, gui_menu->name);
}
void drawFooter(){
	rendererv_drawRectangle(0, UI_HEIGHT - (HEADER_HEIGHT - 1), UI_WIDTH, HEADER_HEIGHT - 1, theme[COLOR_BG_HEADER]);//BG
	rendererv_drawRectangle(0, UI_HEIGHT - HEADER_HEIGHT, UI_WIDTH, 1, theme[isSafeBoot ? COLOR_ACTIVE : COLOR_HEADER]);//Separator
	if (gui_menu->footer){
		rendererv_setColor(theme[isSafeBoot ? COLOR_ACTIVE : COLOR_HEADER]);
		if (strlen(gui_menu->footer) <= STR_SIZE){
			rendererv_drawStringF(L_0, UI_HEIGHT-HEADER_HEIGHT + 4, gui_menu->footer);
		} else {
			int strNum = (strlen(gui_menu->footer) - 1) / STR_SIZE + 1;
			int strIdx = ((ksceKernelGetSystemTimeWide() - tickMenuOpen) % (strNum * DELAY_FOOTER)) / DELAY_FOOTER;
			char sub[STR_SIZE+1];
        	strncpy(sub, &gui_menu->footer[strIdx * STR_SIZE], STR_SIZE);
			sub[STR_SIZE] = '\0';
			rendererv_drawStringF(L_0, UI_HEIGHT-HEADER_HEIGHT + 4, sub);
		}
	}
}
void drawIndent(){
	int y = (gui_menu->idx - gui_calcStartingIndex(gui_menu->idx, gui_menu->num, gui_lines, BOTTOM_OFFSET)) * CHA_H
		+ HEADER_HEIGHT + CHA_H / 2;
	rendererv_drawRectangle(L_1 - 5, y - 1, UI_WIDTH - 2 * L_1 + 4, CHA_H + 2, theme[COLOR_BG_HEADER]);//BG
	rendererv_setColor(theme[COLOR_HEADER]);   
}

void gui_drawTouchPointer(uint32_t panel, TouchPoint* tp){
	gui_drawTouchPointerN(panel, tp, NULL);
}

void gui_drawTouchPointerN(uint32_t panel, TouchPoint* tp, char* str){
	TouchPoints2 size = T_SIZE[panel];
	int x = (float)fbWidth / (size.b.x - size.a.x) * (tp->x - size.a.x);
	int y = (float)fbHeight / (size.b.y - size.a.y) * (tp->y - size.a.y);
	renderer_setColor(theme[COLOR_TOUCH_SHADOW]);
	renderer_drawImage(x - 10, y - 4, ICN_TOUCH_X, ICN_TOUCH_Y, ICN_TOUCH);
	renderer_setColor(panel ? theme[COLOR_TOUCH_BOTTOM] : theme[COLOR_TOUCH_FRONT]);
	renderer_drawImage(x - 11, y - 5, ICN_TOUCH_X, ICN_TOUCH_Y, ICN_TOUCH);
	if (str != NULL){
		renderer_setColor(theme[COLOR_TOUCH_SHADOW]);
		renderer_drawString(x + 19, y + 10, str);
		renderer_setColor(panel ? theme[COLOR_TOUCH_BOTTOM] : theme[COLOR_TOUCH_FRONT]);
		renderer_drawString(x + 18, y + 9, str);
	}
}

void drawEmulatedPointersForPanel(uint32_t panel){
	for (int i = 0; i < et[panel].num; i++){
		if (et[panel].reports[i].isSwipe){
			if (profile.entries[PR_TO_DRAW_SWIPE].v.b)
				gui_drawTouchPointer(panel, &et[panel].reports[i].swipeCurrentPoint);
		} else if (et[panel].reports[i].isSmartSwipe){
			if (profile.entries[PR_TO_DRAW_SMART_SWIPE].v.b)
				gui_drawTouchPointer(panel, &et[panel].reports[i].swipeEndPoint);
		} else {
			if (profile.entries[PR_TO_DRAW_POINT].v.b)
				gui_drawTouchPointer(panel, &et[panel].reports[i].point);
		}
	}
	if (profile.entries[PR_TO_DRAW_NATIVE].v.b && !isPSTVTouchEmulation){
		for (int i = 0; i < td[panel].reportNum; i++){
			TouchPoint tp = (TouchPoint){
				x: td[panel].report[i].x, 
				y: td[panel].report[i].y};
			gui_drawTouchPointer(panel, &tp);
		}
	}
}

void drawBody() {
	rendererv_drawRectangle(0, HEADER_HEIGHT, UI_WIDTH, UI_HEIGHT -  2 * HEADER_HEIGHT, theme[COLOR_BG_BODY]);
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

void drawBlankPopup(){
	renderer_setColor(BLACK);
	renderer_drawRectangle(
		CHA_W, CHA_H, 
		CHA_W * (max(strlen(popupHeader), strlen(popupMessage)) + 1), CHA_H * 3);
}

void drawPopup(){
	if (!settings[POP_ALL].v.b)
		return;
	renderer_setColor(theme[COLOR_BG_HEADER]);
	renderer_drawRectangle(
		CHA_W, CHA_H, 
		CHA_W * (max(strlen(popupHeader), strlen(popupMessage)) + 2), CHA_H * 3 + 1);
	renderer_setColor(popupColor);
	renderer_drawStringF(CHA_W * 1.5, CHA_H * 1.5 - 1, popupHeader);
	renderer_setColor(theme[COLOR_DEFAULT]);
	renderer_drawStringF(CHA_W * 1.5, CHA_H * 2.5 + 3, popupMessage);
}

void updatePopup(){
	if (!isPopupActive)
		return;

	if (popupTTL != 0 && (ksceKernelGetSystemTimeWide() > popupTTL + popupTick)){
		isPopupActive = false;
		drawBlankPopup();
		return;
	}

	drawPopup();
}

void popupShow(char* header, char* message, uint ttl, uint32_t clr){
	strclone(popupHeader, header);
	strclone(popupMessage, message);
	popupTTL = ttl;
	popupTick = ksceKernelGetSystemTimeWide();
	popupColor = clr;
	isPopupActive = true;
}

void gui_popupShow(char* header, char* message, uint ttl){
	popupShow(header, message, ttl, theme[COLOR_HEADER]);
}
void gui_popupShowSuccess(char* header, char* message, uint ttl){
	popupShow(header, message, ttl, theme[COLOR_SUCCESS]);
}
void gui_popupShowWarning(char* header, char* message, uint ttl){
	popupShow(header, message, ttl, theme[COLOR_ACTIVE]);
}
void gui_popupShowDanger(char* header, char* message, uint ttl){
	popupShow(header, message, ttl, theme[COLOR_DANGER]);
}

void gui_popupHide(){
	isPopupActive = false;
}

void gui_draw(const SceDisplayFrameBuf *pParam){
	ksceKernelLockMutex(mutex_gui_uid, 1, NULL);
	ticker++;
	if (gui_isOpen) {
		renderer_setFB(pParam);
		drawHeader();
		drawBody();
		drawFooter();
		renderer_writeFromVFB(tickUIOpen, settings[SETT_ANIMATION].v.b);
		if (gui_menu->onDrawFB != NULL)
			gui_menu->onDrawFB();
	} else {
		renderer_setFB(pParam);
		drawEmulatedPointersForPanel(SCE_TOUCH_PORT_FRONT);
		drawEmulatedPointersForPanel(SCE_TOUCH_PORT_BACK);
		if (gui_isBlankFrame){
			renderer_blankFrame();
			gui_isBlankFrame = false;
		} 
	}
	updatePopup();
    ksceKernelUnlockMutex(mutex_gui_uid, 1);
}

void gui_updateEmulatedTouch(SceTouchPortType panel, EmulatedTouch etouch, SceTouchData pData){
	et[panel] = etouch;
	td[panel] = pData;
}

void gui_input(SceCtrlData *ctrl) {
	if (tickUIOpen + TIME_MUTE_KEYS > ksceKernelGetSystemTimeWide())
		btn_del(&ctrl->buttons, hotkeys[HOTKEY_MENU].v.u);
		// return; // Menu trigger hotkey should not trigger any menu actions on menu open
	
	ksceKernelLockMutex(mutex_gui_uid, 1, NULL);
	
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
    ksceKernelUnlockMutex(mutex_gui_uid, 1);
}

void ui_fixIdx(Menu* m, int idx){
	if (m->entries == NULL) return;
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
	tickMenuOpen = ksceKernelGetSystemTimeWide();
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

void gui_open(const SceDisplayFrameBuf *pParam){
	if (rendererv_allocVirtualFB() < 0){
		LOG("memory allocation for menu failed\n");
		gui_popupShowDanger("Error", "Buy more RAM !", TTL_POPUP_LONG);
		return;
	}
	ksceKernelLockMutex(mutex_gui_uid, 1, NULL);
	gui_setIdx(0);
	gui_isOpen = true;
	tickUIOpen = tickMenuOpen = ksceKernelGetSystemTimeWide();
	LOG("gui_open() tickUIOpen=%lli\n", tickUIOpen);
    ksceKernelUnlockMutex(mutex_gui_uid, 1);
}
void gui_close(){
	ksceKernelLockMutex(mutex_gui_uid, 1, NULL);
	rendererv_freeVirtualFB();
	gui_isOpen = false;
	tickUIClose = ksceKernelGetSystemTimeWide();
	profile.version = ksceKernelGetSystemTimeWide();
	sync();
	LOGF("gui_close()\n");
	gui_isBlankFrame = (profile.entries[PR_MO_BLANK_FRAME].v.b);
	if (!isMenuAllowed())
		gui_menu = menus[MENU_MAIN_ID];
    ksceKernelUnlockMutex(mutex_gui_uid, 1);
	
	// Autosave
	if (settings[SETT_AUTOSAVE].v.b){
		if (isSafeBoot){
			gui_popupShowWarning("$! Safe Mode", "Profile autosaving is disabled.", TTL_POPUP_SHORT);
		} else {

			char profile_to_save[64];
			sprintf(profile_to_save, secondaryProfileLoaded ? "%s%s" : "%s", titleid, SECONDARY_PROFILE_SUFFIX);
			
			profile_save(profile_to_save);
			if (settings[POP_SAVE].v.b) {
				char pop_message[64];
				sprintf(pop_message, secondaryProfileLoaded ? "%s (secondary)" : "%s", titleid);
				gui_popupShowSuccess("$G Profile saved", pop_message, TTL_POPUP_SHORT);
			}
		}
	}
}

void gui_init(){
	memset(&btns, 0, sizeof(btns));
    mutex_gui_uid = ksceKernelCreateMutex("reVita_mutex_gui", 0, 0, NULL);
	gui_menu = menus[MENU_MAIN_ID]; //&menu_main;

    renderer_init();
    rendererv_init(UI_WIDTH, UI_HEIGHT);

	menu_initMain();
	menu_initAnalog();
	menu_initTouch();
	menu_initTurbo();
	menu_initGyro();
	menu_initController();
	menu_initMore();
	menu_initSettings();
	menu_initHotkeys();

	menu_initCredits();
	menu_initProfile();
	menu_initSavemanager();

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
    if (mutex_gui_uid >= 0)
        ksceKernelDeleteMutex(mutex_gui_uid);
}
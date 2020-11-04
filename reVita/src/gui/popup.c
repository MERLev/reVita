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

#define POPUP_NUM
typedef struct Popup{
    int id;
    char header[40];
    char message[40];
    int ttl;
    SceInt64 tick;
    bool isActive;
} Popup;

static Popup popups[POPUP_NUM];
static int num = 0;
static int idCounter = 0;

void drawPopup(){
	renderer_setColor(theme[COLOR_BG_HEADER]);
	renderer_drawRectangle(
		CHA_W, CHA_H, 
		CHA_W * (max(strlen(popupHeader), strlen(popupMessage)) + 1), CHA_H * 3);
	renderer_setColor(theme[COLOR_HEADER]);
	renderer_drawStringF(CHA_W * 1.5, CHA_H * 1.5, popupHeader);
	renderer_setColor(theme[COLOR_DEFAULT]);
	renderer_drawStringF(CHA_W * 1.5, CHA_H * 2.5, popupMessage);
}

void popup_reDraw(){
	if (!isPopupActive)
		return;

	if (ksceKernelGetSystemTimeWide() > popupTTL + popupTick){
		isPopupActive = false;
		return;
	}

	drawPopup();
}

void popup_show(char* header, char* message, uint ttl){
    Popup p = (Popup){
        .id = idCounter++,
        .ttl = ttl,
        .tick = ksceKernelGetSystemTimeWide(),
        .isActive = true
    };

    
}

void popup_hide(){
	isPopupActive = false;
}
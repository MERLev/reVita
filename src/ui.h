#ifndef _UI_H_
#define _UI_H_

;enum MENU_ID{
	MENU_MAIN_ID = 0,
	MENU_ANALOG_ID,
	MENU_TOUCH_ID,
	MENU_GYRO_ID,
	MENU_CONTROLLER_ID,
	MENU_HOKS_ID,
	MENU_DEBUG_BUTTONS_ID,
	MENU_SETTINGS_ID,
	MENU_CREDITS_ID,
	MENU_PROFILE_ID,

	MENU_PICK_BUTTON_ID,
	MENU_PICK_ANALOG_LEFT_ID,
	MENU_PICK_ANALOG_RIGHT_ID,
	MENU_PICK_TOUCH_POINT_ID,
	MENU_PICK_TOUCH_ZONE_ID,

	MENU_REMAP_ID,
	MENU_REMAP_TRIGGER_TYPE_ID,
	MENU_REMAP_TRIGGER_TOUCH_FRONT_ID,
	MENU_REMAP_TRIGGER_TOUCH_BACK_ID,
	MENU_REMAP_TRIGGER_GYRO_ID,
	MENU_REMAP_EMU_TYPE_ID,
	MENU_REMAP_EMU_TOUCH_FRONT_ID,
	MENU_REMAP_EMU_TOUCH_BACK_ID,
	MENU_ID__NUM
}MENU_ID;

enum PROFILE_ACTIONS{
	PROFILE_GLOBAL_SAVE = 0,
	PROFILE_GLOABL_LOAD,
	PROFILE_GLOBAL_DELETE,
	PROFILE_LOCAL_SAVE,
	PROFILE_LOCAL_LOAD,
	PROFILE_LOCAL_DELETE
}PROFILE_ACTIONS;

#define HEADER_TYPE         -1
#define TEXT_IDX            -2
#define NEW_RULE_IDX        -3

struct Menu;
typedef void (*onButtonF)(uint32_t btn);
typedef void (*onInputF)(SceCtrlData *ctrl);
typedef void (*onDrawF)();
typedef void (*onBuildF)(struct Menu* m);
typedef struct Menu{
	enum MENU_ID id;
	enum MENU_ID prev;
	enum MENU_ID next;
	enum MENU_ID parent;

	uint8_t idx;
	char* name;
	char* footer;
	bool noIndent;

	uint8_t num;
	struct MenuEntry* entries;

	onButtonF onButton;
	onInputF onInput;
	onDrawF onDraw;
	onBuildF onBuild;

	int32_t data;
}Menu;
typedef struct MenuEntry{
	char* name;
	int32_t type;
	int32_t data;
} MenuEntry;

uint8_t ui_opened;
uint8_t ui_lines;
uint8_t new_frame;
struct Menu* ui_menu;
MenuEntry* ui_entry;

struct RemapRule ui_ruleEdited; //Rule currently edited

void ui_openMenu(enum MENU_ID id);
void ui_openMenuSmart(enum MENU_ID id, enum MENU_ID prevId, enum MENU_ID nextId, uint32_t data);
void ui_openMenuPrev();
void ui_openMenuNext();
void ui_openMenuParent();
void ui_nextEntry();
void ui_prevEntry();

void ui_onInput(SceCtrlData *ctrl);
void ui_draw(const SceDisplayFrameBuf *pParam);
void ui_open();
void ui_close();
void ui_init();
void ui_destroy();

void ui_setIdx(int i);

#endif
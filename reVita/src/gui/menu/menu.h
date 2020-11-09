#ifndef _MENU_H_
#define _MENU_H_

void onDraw_generic(uint32_t menuY);
bool onDrawEntry_header(int x, int y, MenuEntry* me, bool isSelected, bool hasHeaders);
bool onDrawEntry_profileEntry(int x, int y, MenuEntry* me, bool isSelected, bool hasHeaders);
bool onDrawEntry_button(int x, int y, MenuEntry* me, bool isSelected, bool hasHeaders);
void onDrawEntry_generic(int x, int y, MenuEntry* me, bool isSelected, bool hasHeaders);

void onButton_null(uint32_t btn);
void onButton_generic(uint32_t btn);
void onButton_genericEntries(uint32_t btn);

void menu_initMain();
void menu_initAnalog();
void menu_initTouch();
void menu_initTurbo();
void menu_initGyro();
void menu_initController();
void menu_initMore();
void menu_initSettings();
void menu_initHotkeys();
void menu_initCredits();
void menu_initProfile();
void menu_initSavemanager();

void menu_initDebugHooks();
void menu_initDebugButtons();

void menu_initPickButton();
void menu_initPickAnalog();
void menu_initPickTouch();

void menu_initRemap();

#endif
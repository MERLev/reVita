#ifndef _UI_DRAW_H_
#define _UI_DRAW_H_

#include "common.h"
#include "remap.h"

static const char* str_btns[PHYS_BUTTONS_NUM];

void generateRemapRuleName(char* str, struct RemapRule* ui_ruleEdited);

void drawMenu_generic();

void drawMenu_remap();
void drawMenu_pickButton();
void drawMenu_analog();
void drawMenu_touch();
void drawMenu_pickTouchPoint();
void drawMenu_pickTouchZone();
void drawMenu_gyro();
void drawMenu_controller();
void drawMenu_hooks();
void drawMenu_settings();
void drawMenu_profiles();
void drawMenu_credits();

#endif
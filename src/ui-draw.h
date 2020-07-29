#ifndef _UI_DRAW_H_
#define _UI_DRAW_H_

#include "common.h"
#include "remap.h"

static const char* str_btns[PHYS_BUTTONS_NUM];

void generateRemapRuleName(char* str, struct RemapRule* ui_ruleEdited);

void onDraw_generic();

void onDraw_remap();
void onDraw_pickButton();
void onDraw_analog();
void onDraw_touch();
void onDraw_pickTouchPoint();
void onDraw_pickTouchZone();
void onDraw_gyro();
void onDraw_controller();
void onDraw_hooks();
void onDraw_settings();
void onDraw_profiles();
void onDraw_credits();

void ui_draw_init();
void ui_draw_destroy();
#endif
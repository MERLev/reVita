#ifndef _UI_CONTROL_H_
#define _UI_CONTROL_H_
#include <psp2kern/ctrl.h> 

void ctrl_init();
void ctrl_destroy();
void ctrl_onInput(SceCtrlData *ctrl);

void onInput_touchPicker(SceCtrlData *ctrl);

void onButton_generic(uint32_t btn);
void onButton_main(uint32_t btn);
void onButton_analog(uint32_t btn);
void onButton_touch(uint32_t btn);
void onButton_gyro(uint32_t btn);
void onButton_controller(uint32_t btn);
void onButton_settings(uint32_t btn);
void onButton_profiles(uint32_t btn);
void onButton_pickButton(uint32_t btn);
void onButton_pickAnalog(uint32_t btn);
void onButton_pickTouchPoint(uint32_t btn);
void onButton_pickTouchZone(uint32_t btn);
void onButton_remap(uint32_t btn);
void onButton_remapTriggerType(uint32_t btn);
void onButton_remapTriggerTouch(uint32_t btn);
void onButton_remapTriggerGyro(uint32_t btn);
void onButton_remapEmuType(uint32_t btn);
void onButton_remapEmuTouch(uint32_t btn);
#endif
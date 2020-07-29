#include <vitasdkkern.h>
#include <psp2/motion.h> 
#include <psp2/touch.h>
#include <stdlib.h>

#include "vitasdkext.h"
#include "main.h"
#include "remap.h"
#include "profile.h"
#include "common.h"
#include "log.h"

#define MULTITOUCH_FRONT_NUM		6
#define MULTITOUCH_BACK_NUM			4

TouchPoint T_FRONT_SIZE = (TouchPoint){1920, 1080};
TouchPoint T_BACK_SIZE  = (TouchPoint){1920, 890};
TouchPoint T_FRONT_TL, T_FRONT_TR, T_FRONT_BL, T_FRONT_BR,
	T_BACK_TL, T_BACK_TR, T_BACK_BL, T_BACK_BR;

typedef struct EmulatedStick{
	uint32_t up, down, left, right;
}EmulatedStick;

typedef struct EmulatedTouch{
	SceTouchReport reports[MULTITOUCH_FRONT_NUM];
	uint8_t num;
}EmulatedTouch;

//For now let's store them on the stack
static SceCtrlData _remappedBuffers[HOOKS_NUM-5][BUFFERS_NUM];
static SceTouchData _remappedBuffersFront[4][BUFFERS_NUM];
static SceTouchData _remappedBuffersBack[4][BUFFERS_NUM];

//Circular cache to store remapped keys buffers per each ctrs hook
static SceCtrlData *remappedBuffers[HOOKS_NUM-5];
static int remappedBuffersSizes[HOOKS_NUM];
static int remappedBuffersIdxs[HOOKS_NUM];

//Circular cache to store Touch buffers per each touch hook
static SceTouchData *remappedBuffersFront[4];
static int remappedBuffersFrontSizes[4];
static int remappedBuffersFrontIdxs[4];
static SceTouchData *remappedBuffersBack[4];
static int remappedBuffersBackSizes[4];
static int remappedBuffersBackIdxs[4];
uint8_t newEmulatedFrontTouchBuffer = 0;
uint8_t newEmulatedBackTouchBuffer = 0;

static EmulatedTouch etFront, etBack, prevEtFront, prevEtBack;
static uint8_t etFrontIdCounter = 64;
static uint8_t etBackIdCounter = 64;


void storeTouchPoint(EmulatedTouch *et, TouchPoint tp){
	for (int i = 0; i < et->num; i++)
		if (et->reports[i].x == tp.x && et->reports[i].y == tp.y)
			return;
	et->reports[et->num].x = tp.x;
	et->reports[et->num].y = tp.y;
	et->num++;
}

void addEmu(struct RemapAction* emu, uint32_t* btn, EmulatedStick* emustick) {
	EmulatedStick* stick = emustick;
	switch (emu->type){
		case REMAP_TYPE_BUTTON: 
			btn_add(btn, emu->param.btn);
			break;
		case REMAP_TYPE_RIGHT_ANALOG: 
		case REMAP_TYPE_RIGHT_ANALOG_DIGITAL: 
			stick = &emustick[1];
		case REMAP_TYPE_LEFT_ANALOG: 
		case REMAP_TYPE_LEFT_ANALOG_DIGITAL: 
			switch (emu->action){
				case REMAP_ANALOG_UP: stick->up += 127; break;
				case REMAP_ANALOG_DOWN: stick->down += 127; break;
				case REMAP_ANALOG_LEFT: stick->left += 127; break;
				case REMAP_ANALOG_RIGHT: stick->right += 127; break;
				default: break;
			}
			break;
		case REMAP_TYPE_FRONT_TOUCH_POINT: 
			switch (emu->action){
				case REMAP_TOUCH_ZONE_TL: storeTouchPoint(&etFront, T_FRONT_TL); break;
				case REMAP_TOUCH_ZONE_TR: storeTouchPoint(&etFront, T_FRONT_TR); break;
				case REMAP_TOUCH_ZONE_BL: storeTouchPoint(&etFront, T_FRONT_BL); break;
				case REMAP_TOUCH_ZONE_BR: storeTouchPoint(&etFront, T_FRONT_BR); break;
				case REMAP_TOUCH_CUSTOM: storeTouchPoint(&etFront, emu->param.touch); break;
				default: break;
			}
			break;
		case REMAP_TYPE_BACK_TOUCH_POINT: 
			switch (emu->action){
				case REMAP_TOUCH_ZONE_TL: storeTouchPoint(&etBack, T_BACK_TL); break;
				case REMAP_TOUCH_ZONE_TR: storeTouchPoint(&etBack, T_BACK_TR); break;
				case REMAP_TOUCH_ZONE_BL: storeTouchPoint(&etBack, T_BACK_BL); break;
				case REMAP_TOUCH_ZONE_BR: storeTouchPoint(&etBack, T_BACK_BR); break;
				case REMAP_TOUCH_CUSTOM: storeTouchPoint(&etBack, emu->param.touch); break;
				default: break;
			}
			break;
		default: break;
	}
}

void addEmuFromAnalog(struct RemapAction* emu, uint32_t* btn, EmulatedStick* emustick, uint8_t stickposval){
	EmulatedStick* stick = emustick;
	switch (emu->type){
		case REMAP_TYPE_RIGHT_ANALOG: 
			stick = &emustick[1];
		case REMAP_TYPE_LEFT_ANALOG: 
			switch (emu->action){
				case REMAP_ANALOG_UP: stick->up += stickposval; break;
				case REMAP_ANALOG_DOWN: stick->down += stickposval; break;
				case REMAP_ANALOG_LEFT: stick->left += stickposval; break;
				case REMAP_ANALOG_RIGHT: stick->right += stickposval; break;
				default: break;
			}
			break;
		default: addEmu(emu, btn, emustick); break;
	}
	
}
void addEmuFromGyro(struct RemapAction* emu, uint32_t* btn, EmulatedStick* emustick,  float gyroval){
// 	if (PHYS_BUTTONS_NUM + 1 < profile.remap[btn_idx] && profile.remap[btn_idx] < PHYS_BUTTONS_NUM + 10) {
// 		// Gyro -> Analog remap
// 		stickpos[profile.remap[btn_idx] - (PHYS_BUTTONS_NUM + 2)] = stickpos[profile.remap[btn_idx] - (PHYS_BUTTONS_NUM + 2)] + clamp(gyroval, -127, 127);
// 	} else {
// 		// Gyro -> Btn remap
// 		if ((((btn_idx == PHYS_BUTTONS_NUM + 16 || btn_idx == PHYS_BUTTONS_NUM + 17)) && gyroval > profile.gyro[3] * 10) ||
// 			(((btn_idx == PHYS_BUTTONS_NUM + 18 || btn_idx == PHYS_BUTTONS_NUM + 19)) && gyroval > profile.gyro[4] * 10) ||
// 			(((btn_idx == PHYS_BUTTONS_NUM + 20 || btn_idx == PHYS_BUTTONS_NUM + 21)) && gyroval > profile.gyro[5] * 10))
// 			applyRemapRule(btn_idx, map, stickpos);
// 	}
}

void applyRemap(SceCtrlData *ctrl) {
	// Gathering real touch data
	SceTouchData front, back;
	internal_touch_call = 1;
	ksceTouchPeek(SCE_TOUCH_PORT_FRONT, &front, 1);
	ksceTouchPeek(SCE_TOUCH_PORT_BACK, &back, 1);
	internal_touch_call = 0;

	uint32_t btns = ctrl->buttons;
	uint32_t propBtns = ctrl->buttons; //Propagated buttons
	uint32_t emuBtns = 0;

	EmulatedStick eSticks[2] = {(EmulatedStick){0, 0, 0, 0}, (EmulatedStick){0, 0, 0, 0}};
	EmulatedStick propSticks[2] = {(EmulatedStick){0, 0, 0, 0}, (EmulatedStick){0, 0, 0, 0}};

	//Set propagation sticks def values
	if (ctrl->lx < 128) propSticks[0].left = 127 - ctrl->lx;
	else propSticks[0].right = ctrl->lx - 127;
	if (ctrl->ly < 128) propSticks[0].up = 127 - ctrl->ly;
	else  propSticks[0].down = ctrl->ly - 127;
	if (ctrl->rx < 128) propSticks[1].left = 127 - ctrl->rx;
	else  propSticks[1].right = ctrl->rx - 127;
	if (ctrl->ry <= 128) propSticks[1].up = 127 - ctrl->ry;
	else  propSticks[1].down = ctrl->ry - 127;
	
	SceTouchData* std;

	//Applying remap rules
	for (int i = 0; i < profile.remapsNum; i++){
		struct RemapRule* rr = &profile.remaps[i];
		if (rr->disabled) continue;
		struct RemapAction* trigger = &rr->trigger;

		//Touch stuff
		std = &back;
		int left = T_BACK_SIZE.x / 2; 
		int top  = T_BACK_SIZE.x / 2;

		switch (trigger->type){
			case REMAP_TYPE_BUTTON: 
				if (btn_has(btns, trigger->param.btn)){
					if (!rr->propagate)
						btn_del(&propBtns, trigger->param.btn);
					addEmu(&rr->emu, &emuBtns, &eSticks[0]);
				}
				break;
			case REMAP_TYPE_LEFT_ANALOG: 
				switch(trigger->action){
					case REMAP_ANALOG_LEFT: 
						if (ctrl->lx >= 128 - profile.analog[PROFILE_ANALOG_LEFT_DEADZONE_X])
							break;
						addEmuFromAnalog(&rr->emu, &emuBtns, eSticks, 127 - ctrl->lx);
						if (!rr->propagate) propSticks[0].left = 0;
						break;
					case REMAP_ANALOG_RIGHT: 
						if (ctrl->lx < 128 + profile.analog[PROFILE_ANALOG_LEFT_DEADZONE_X])
							break;
						addEmuFromAnalog(&rr->emu, &emuBtns, eSticks, ctrl->lx - 127);
						if (!rr->propagate) propSticks[0].right = 0;
						break;
					case REMAP_ANALOG_UP: 
						if (ctrl->ly >= 128 - profile.analog[PROFILE_ANALOG_LEFT_DEADZONE_Y])
							break;
						addEmuFromAnalog(&rr->emu, &emuBtns, eSticks, 127 - ctrl->ly);
						if (!rr->propagate) propSticks[0].up = 0;
						break;
					case REMAP_ANALOG_DOWN: 
						if (ctrl->ly < 128 + profile.analog[PROFILE_ANALOG_LEFT_DEADZONE_Y])
							break;
						addEmuFromAnalog(&rr->emu, &emuBtns, eSticks, ctrl->ly - 127);
						if (!rr->propagate) propSticks[0].down = 0;
						break;
					default: break;
				}
				break;
			case REMAP_TYPE_RIGHT_ANALOG: 
				switch(trigger->action){
					case REMAP_ANALOG_LEFT: 
						if (ctrl->rx >= 128 - profile.analog[PROFILE_ANALOG_RIGHT_DEADZONE_X])
							break;
						addEmuFromAnalog(&rr->emu, &emuBtns, eSticks, 127 - ctrl->rx);
						if (!rr->propagate) propSticks[1].left = 0;
						break;
					case REMAP_ANALOG_RIGHT: 
						if (ctrl->rx < 128 + profile.analog[PROFILE_ANALOG_RIGHT_DEADZONE_X])
							break;
						addEmuFromAnalog(&rr->emu, &emuBtns, eSticks, ctrl->rx - 127);
						if (!rr->propagate) propSticks[1].right = 0;
						break;
					case REMAP_ANALOG_UP: 
						if (ctrl->ry >= 128 - profile.analog[PROFILE_ANALOG_RIGHT_DEADZONE_Y])
							break;
						addEmuFromAnalog(&rr->emu, &emuBtns, eSticks, 127 - ctrl->ry);
						if (!rr->propagate) propSticks[1].up = 0;
						break;
					case REMAP_ANALOG_DOWN: 
						if (ctrl->ry < 128 + profile.analog[PROFILE_ANALOG_RIGHT_DEADZONE_Y])
							break;
						addEmuFromAnalog(&rr->emu, &emuBtns, eSticks, ctrl->ry - 127);
						if (!rr->propagate) propSticks[1].down = 0;
						break;
					default: break;
				}
				break;
			case REMAP_TYPE_FRONT_TOUCH_ZONE: 
				std = &front;
				left = T_FRONT_SIZE.x / 2; 
				top  = T_FRONT_SIZE.y / 2;
			case REMAP_TYPE_BACK_TOUCH_ZONE: 
				for (i=0;i<std->reportNum;i++) {
					switch (trigger->action){
						case REMAP_TOUCH_ZONE_TL:
							if (std->report[i].x <= left && std->report[i].y <= top)
								addEmu(&rr->emu, &emuBtns, eSticks);
							break;
						case REMAP_TOUCH_ZONE_TR:
							if (std->report[i].x > left && std->report[i].y <= top)
								addEmu(&rr->emu, &emuBtns, eSticks);
							break;
						case REMAP_TOUCH_ZONE_BL:
							if (std->report[i].x <= left && std->report[i].y > top)
								addEmu(&rr->emu, &emuBtns, eSticks);
							break;
						case REMAP_TOUCH_ZONE_BR:
							if (std->report[i].x > left && std->report[i].y > top)
								addEmu(&rr->emu, &emuBtns, eSticks);
							break;
						case REMAP_TOUCH_CUSTOM:
							if (std->report[i].x >= trigger->param.zone.a.x && 
								std->report[i].x < trigger->param.zone.b.x && 
								std->report[i].y >= trigger->param.zone.a.y && 
								std->report[i].y < trigger->param.zone.b.y) 
									addEmu(&rr->emu, &emuBtns, eSticks);
							break;
						default: break;
					}
				}
				break;
			case REMAP_TYPE_GYROSCOPE: 
				//ToDo add gyro support
				// Applying remap for gyro
				// if (profile.gyro[7] == 0) {
				// 	if (motionstate.angularVelocity.y > 0)
				// 		applyRemapRuleForGyro(PHYS_BUTTONS_NUM + 16, &new_map, stickpos,
				// 			motionstate.angularVelocity.y * profile.gyro[0]);
				// 	if (motionstate.angularVelocity.y < 0)
				// 		applyRemapRuleForGyro(PHYS_BUTTONS_NUM + 17, &new_map, stickpos,
				// 			-motionstate.angularVelocity.y * profile.gyro[0]);
				// 	if (motionstate.angularVelocity.x > 0)
				// 		applyRemapRuleForGyro(PHYS_BUTTONS_NUM + 18, &new_map, stickpos,
				// 			motionstate.angularVelocity.x * profile.gyro[1]);
				// 	if (motionstate.angularVelocity.x < 0)
				// 		applyRemapRuleForGyro(PHYS_BUTTONS_NUM + 19, &new_map, stickpos,
				// 			-motionstate.angularVelocity.x * profile.gyro[1]);
				// 	if (motionstate.angularVelocity.z > 0)
				// 		applyRemapRuleForGyro(PHYS_BUTTONS_NUM + 20, &new_map, stickpos,
				// 			motionstate.angularVelocity.z * profile.gyro[2]);
				// 	if (motionstate.angularVelocity.z < 0)
				// 		applyRemapRuleForGyro(PHYS_BUTTONS_NUM + 21, &new_map, stickpos,
				// 			-motionstate.angularVelocity.z * profile.gyro[2]);
				// }
				// else {
				// 	// Applying remap for gyro wheel mode
				// 	if (motionstate.deviceQuat.y < 0)
				// 		applyRemapRuleForGyro(PHYS_BUTTONS_NUM + 16, &new_map, stickpos,
				// 			motionstate.deviceQuat.y * profile.gyro[0] * 4);
				// 	if (motionstate.deviceQuat.y > 0)
				// 		applyRemapRuleForGyro(PHYS_BUTTONS_NUM + 17, &new_map, stickpos,
				// 			-motionstate.deviceQuat.y * profile.gyro[0] * 4);
				// 	if (motionstate.deviceQuat.x < 0)
				// 		applyRemapRuleForGyro(PHYS_BUTTONS_NUM + 18, &new_map, stickpos,
				// 			motionstate.deviceQuat.x * profile.gyro[1] * 4);
				// 	if (motionstate.deviceQuat.x > 0)
				// 		applyRemapRuleForGyro(PHYS_BUTTONS_NUM + 19, &new_map, stickpos,
				// 			-motionstate.deviceQuat.x * profile.gyro[1] * 4);
				// 	if (motionstate.deviceQuat.z < 0)
				// 		applyRemapRuleForGyro(PHYS_BUTTONS_NUM + 20, &new_map, stickpos,
				// 			motionstate.deviceQuat.z * profile.gyro[2] * 4);
				// 	if (motionstate.deviceQuat.z > 0)
				// 		applyRemapRuleForGyro(PHYS_BUTTONS_NUM + 21, &new_map, stickpos,
				// 		-motionstate.deviceQuat.z * profile.gyro[2] * 4);
				// }
				break;
			default: break;
		}
	}
	// LOG("REMAP2: %i : (%i, %i)(%i, %i) -> [%lu, %lu, %lu, %lu][%lu, %lu, %lu, %lu]\n", 
	// 	((ctrl->buttons & SCE_CTRL_TRIANGLE) > 0), ctrl->lx, ctrl->ly, ctrl->rx, ctrl->ry,
	// 	eSticks[0].left, eSticks[0].right, eSticks[0].up, eSticks[0].down,
	// 	eSticks[1].left, eSticks[1].right, eSticks[1].up, eSticks[1].down);

	// Remove minimal drift if digital remap for stick directions is used
	// if (stickpos[0] || stickpos[1] || stickpos[2] || stickpos[3]){
	// 	if (abs(ctrl->lx - 127) < profile.analog[0]) 
	// 		ctrl->lx = 127;
	// 	if (abs(ctrl->ly - 127) < profile.analog[1]) 
	// 		ctrl->ly = 127;
	// }
	// if (stickpos[4] || stickpos[5] || stickpos[6] || stickpos[7]){
	// 	if (abs(ctrl->rx - 127) < profile.analog[2]) 
	// 		ctrl->rx = 127;
	// 	if (abs(ctrl->ry - 127) < profile.analog[3]) 
	// 		ctrl->ry = 127;
	// }

	//Storing emulated HW buttons
	btn_add(&emuBtns, propBtns);
	ctrl->buttons = emuBtns;

	//Restoring propagated values for analog sticks
	for (int i = 0; i < 2; i++){
		eSticks[i].left += propSticks[i].left;
		eSticks[i].right += propSticks[i].right;
		eSticks[i].up += propSticks[i].up;
		eSticks[i].down += propSticks[i].down;
	}

	//Storing remap for analog axises
	if (eSticks[0].left || eSticks[0].right)
		ctrl->lx = clamp(127  + eSticks[0].right- eSticks[0].left, 0, 255);
	if (eSticks[0].up || eSticks[0].down)
		ctrl->ly = clamp(127 + eSticks[0].down - eSticks[0].up, 0, 255);
	if (eSticks[1].left || eSticks[1].right)
		ctrl->rx = clamp(127 + eSticks[1].right - eSticks[1].left, 0, 255);
	if (eSticks[1].up || eSticks[1].down)
		ctrl->ry = clamp(127 + eSticks[1].down - eSticks[1].up, 0, 255);

	//Telling that new emulated touch buffer is ready to be takn
	newEmulatedFrontTouchBuffer = 1;
	newEmulatedBackTouchBuffer = 1;
}

int remap_controls(SceCtrlData *ctrl, int nBufs, int hookId) {	
	int buffIdx = (remappedBuffersIdxs[hookId] + 1) % BUFFERS_NUM;
	
	//Storing copy of latest buffer
	remappedBuffersIdxs[hookId] = buffIdx;
	remappedBuffersSizes[hookId] = min(remappedBuffersSizes[hookId] + 1, BUFFERS_NUM);
	remappedBuffers[hookId][buffIdx] = ctrl[nBufs-1];
	
	//Applying remap to latest buffer
	//ToDo
	applyRemap(&remappedBuffers[hookId][buffIdx]);
	
	//Limit returned buffers with amount we have cached
	nBufs = min(nBufs, remappedBuffersSizes[hookId]);
	
	//Restoring stored buffers
	
	for (int i = 0; i < nBufs; i++)
		ctrl[i] = remappedBuffers[hookId]
			[(BUFFERS_NUM + buffIdx - nBufs + i + 1) % BUFFERS_NUM];
	return nBufs;
}

void swapTriggersBumpers(SceCtrlData *ctrl){
	uint32_t b = 0;
	for (int j = 0; j < PHYS_BUTTONS_NUM; j++)
		if (ctrl->buttons & HW_BUTTONS[j]){
			if (HW_BUTTONS[j] == SCE_CTRL_LTRIGGER) b+= SCE_CTRL_L1;
			else if (HW_BUTTONS[j] == SCE_CTRL_L1) b+= SCE_CTRL_LTRIGGER;
			else if (HW_BUTTONS[j] == SCE_CTRL_RTRIGGER) b+= SCE_CTRL_R1;
			else if (HW_BUTTONS[j] == SCE_CTRL_R1) b+= SCE_CTRL_RTRIGGER;
			else b += HW_BUTTONS[j];
	}
	ctrl->buttons = b;
}

//Used to enable R1/R3/L1/L3
void remap_patchToExt(SceCtrlData *ctrl){
	if (!profile.controller[0])
		return;
	SceCtrlData pstv_fakepad;
	internal_ext_call = 1;
	//int ret = sceCtrlPeekBufferPositiveExt2(profile.controller[1], &pstv_fakepad, 1);
	int ret = ksceCtrlPeekBufferPositive(profile.controller[1], &pstv_fakepad, 1);
	internal_ext_call = 0;
	if (ret > 0){
		ctrl->buttons = pstv_fakepad.buttons;
		if (profile.controller[2])
			swapTriggersBumpers(ctrl);
	}
}

//Keep same touch id for continuus touches
uint8_t generateTouchId(int x, int y, int panel){ 
	if (panel == SCE_TOUCH_PORT_FRONT){
		for (int i = 0; i < prevEtFront.num; i++)
			if (prevEtFront.reports[i].x == x && prevEtFront.reports[i].y == y)
				return prevEtFront.reports[i].id;
		etFrontIdCounter = (etFrontIdCounter + 1) % 127;
		return etFrontIdCounter;
	} else {
		for (int i = 0; i < prevEtBack.num; i++)
			if (prevEtBack.reports[i].x == x && prevEtBack.reports[i].y == y)
				return prevEtBack.reports[i].id;
		etBackIdCounter = (etBackIdCounter + 1) % 127;
		return etBackIdCounter;
	}
}

void addVirtualTouches(SceTouchData *pData, EmulatedTouch *et, 
		uint8_t touchPointsMaxNum, int panel){
	int touchIdx = 0;
	while (touchIdx < et->num && pData->reportNum < touchPointsMaxNum){
		pData->report[pData->reportNum].x = et->reports[touchIdx].x;
		pData->report[pData->reportNum].y = et->reports[touchIdx].y;
		et->reports[touchIdx].id = generateTouchId(
			et->reports[touchIdx].x, et->reports[touchIdx].y, panel);
		pData->report[pData->reportNum].id = et->reports[touchIdx].id;
		pData->reportNum ++;
		touchIdx ++;
	}
}

void updateTouchInfo(SceUInt32 port, SceTouchData *pData){	
	if (port == SCE_TOUCH_PORT_FRONT) {
		// if ((profile.touch[16] == 1 && //Disable if remapped
		// 		(profile.remap[PHYS_BUTTONS_NUM] != PHYS_BUTTONS_NUM ||
		// 		 profile.remap[PHYS_BUTTONS_NUM+1] != PHYS_BUTTONS_NUM ||
		// 		 profile.remap[PHYS_BUTTONS_NUM+2] != PHYS_BUTTONS_NUM ||
		// 		 profile.remap[PHYS_BUTTONS_NUM+3] != PHYS_BUTTONS_NUM)) ||
		// 		 profile.remap[PHYS_BUTTONS_NUM]   == PHYS_BUTTONS_NUM+1 ||
		// 		 profile.remap[PHYS_BUTTONS_NUM+1] == PHYS_BUTTONS_NUM+1 ||
		// 		 profile.remap[PHYS_BUTTONS_NUM+2] == PHYS_BUTTONS_NUM+1 ||
		// 		 profile.remap[PHYS_BUTTONS_NUM+3] == PHYS_BUTTONS_NUM+1)
		// 	pData->reportNum = 0; //Disable pad
			
		if (!newEmulatedFrontTouchBuffer){//New touchbuffer not ready - using previous one
			addVirtualTouches(pData, &prevEtFront, 
				MULTITOUCH_FRONT_NUM, SCE_TOUCH_PORT_FRONT);
			return;
		}
		
		addVirtualTouches(pData, &etFront, 
			MULTITOUCH_FRONT_NUM, SCE_TOUCH_PORT_FRONT);
		prevEtFront = etFront;
		etFront.num = 0;
		newEmulatedFrontTouchBuffer = 0;
	} else {
		// if ((profile.touch[17] == 1 &&//Disable if remapped
		// 		(profile.remap[PHYS_BUTTONS_NUM+4] != PHYS_BUTTONS_NUM ||
		// 		 profile.remap[PHYS_BUTTONS_NUM+5] != PHYS_BUTTONS_NUM ||
		// 		 profile.remap[PHYS_BUTTONS_NUM+6] != PHYS_BUTTONS_NUM ||
		// 		 profile.remap[PHYS_BUTTONS_NUM+7] != PHYS_BUTTONS_NUM)) ||
		// 		 profile.remap[PHYS_BUTTONS_NUM+4] == PHYS_BUTTONS_NUM+1 ||
		// 		 profile.remap[PHYS_BUTTONS_NUM+5] == PHYS_BUTTONS_NUM+1 ||
		// 		 profile.remap[PHYS_BUTTONS_NUM+6] == PHYS_BUTTONS_NUM+1 ||
		// 		 profile.remap[PHYS_BUTTONS_NUM+7] == PHYS_BUTTONS_NUM+1)
		// 	pData->reportNum = 0; //Disable pad
			
		if (!newEmulatedBackTouchBuffer){//New touchbuffer not ready - using previous one
			addVirtualTouches(pData, &prevEtBack, 
				MULTITOUCH_BACK_NUM, SCE_TOUCH_PORT_BACK);
			return;
		}
		
		addVirtualTouches(pData, &etBack, 
			MULTITOUCH_BACK_NUM, SCE_TOUCH_PORT_BACK);
		prevEtBack = etBack;
		etBack.num = 0;
		newEmulatedBackTouchBuffer = 0;
	}
}

int remap_touch(SceUInt32 port, SceTouchData *pData, SceUInt32 nBufs, uint8_t hookId){
	if (port == SCE_TOUCH_PORT_FRONT){
		//Get next cache real index
		int buffIdx = (remappedBuffersFrontIdxs[hookId] + 1) % BUFFERS_NUM;

		//Storing copy of latest buffer
		remappedBuffersFrontIdxs[hookId] = buffIdx;
		remappedBuffersFrontSizes[hookId] = min(remappedBuffersFrontSizes[hookId] + 1, BUFFERS_NUM);
		remappedBuffersFront[hookId][buffIdx] = pData[nBufs-1];
		
		//Updating latest buffer with simulated touches
		updateTouchInfo(port, &remappedBuffersFront[hookId][buffIdx]);
		
		//Limit returned buufers num with what we have stored
		nBufs = min(nBufs, remappedBuffersFrontSizes[hookId]);
		
		//Restoring stored buffers
		for (int i = 0; i < nBufs; i++)
			pData[i] = remappedBuffersFront[hookId]
				[(BUFFERS_NUM + buffIdx - nBufs + i + 1) % BUFFERS_NUM];
	} else {
		//Real index
		int buffIdx = (remappedBuffersBackIdxs[hookId] + 1) % BUFFERS_NUM;

		//Storing copy of latest buffer
		remappedBuffersBackIdxs[hookId] = buffIdx;
		remappedBuffersBackSizes[hookId] = min(remappedBuffersBackSizes[hookId] + 1, BUFFERS_NUM);
		remappedBuffersBack[hookId][buffIdx] = pData[nBufs-1];
		
		//Updating latest buffer with simulated touches
		updateTouchInfo(port, &remappedBuffersBack[hookId][buffIdx]);
		
		//Limit returned buufers num with what we have stored
		nBufs = min(nBufs, remappedBuffersBackSizes[hookId]);
		
		//Restoring stored buffers
		for (int i = 0; i < nBufs; i++)
			pData[i] = remappedBuffersBack[hookId]
				[(BUFFERS_NUM + buffIdx - nBufs + i + 1) % BUFFERS_NUM];
	}
	return nBufs;
}

void remap_resetCtrlBuffers(uint8_t hookId){
	remappedBuffersIdxs[hookId] = 0;
	remappedBuffersSizes[hookId] = 0;
}
void remap_resetTouchBuffers(uint8_t hookId){
	remappedBuffersFrontIdxs[hookId] = 0;
	remappedBuffersBackIdxs[hookId] = 0;
	remappedBuffersFrontSizes[hookId] = 0;
	remappedBuffersBackSizes[hookId] = 0;
}

void remap_init(){
	for (int i = 0; i < HOOKS_NUM-5; i++) //Allocating mem for stored buffers
		remappedBuffers[i] = &_remappedBuffers[i][0];

	for (int i = 0; i < 4; i++){
		remappedBuffersFront[i] = (SceTouchData*)&_remappedBuffersFront[i][0];
		remappedBuffersBack[i] = (SceTouchData*)&_remappedBuffersBack[i][0];
	}

	T_FRONT_TL = (TouchPoint){T_FRONT_SIZE.x / 6, T_FRONT_SIZE.y / 4};
	T_FRONT_TR = (TouchPoint){T_FRONT_SIZE.x * 5 / 6, T_FRONT_SIZE.y / 4};
	T_FRONT_BL = (TouchPoint){T_FRONT_SIZE.x / 6, T_FRONT_SIZE.y * 3 / 4};
	T_FRONT_BR = (TouchPoint){T_FRONT_SIZE.x * 5 / 6, T_FRONT_SIZE.y * 3 / 4};
	T_BACK_TL = (TouchPoint){T_BACK_SIZE.x / 6, T_BACK_SIZE.y / 4};
	T_BACK_TR = (TouchPoint){T_BACK_SIZE.x * 5 / 6, T_BACK_SIZE.y / 4};
	T_BACK_BL = (TouchPoint){T_BACK_SIZE.x / 6, T_BACK_SIZE.y * 3 / 4};
	T_BACK_BR = (TouchPoint){T_BACK_SIZE.x * 5 / 6, T_BACK_SIZE.y * 3 / 4};
}

void remap_destroy(){

}
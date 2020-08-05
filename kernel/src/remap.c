#include <vitasdkkern.h>
#include <psp2/motion.h> 
#include <psp2/touch.h>
#include <stdlib.h>

#include "vitasdkext.h"
#include "main.h"
#include "userspace.h"
#include "remap.h"
#include "profile.h"
#include "common.h"
#include "log.h"

#define MULTITOUCH_FRONT_NUM		6
#define MULTITOUCH_BACK_NUM			4

TouchPoint T_FRONT_SIZE = (TouchPoint){1920, 1080};
TouchPoint T_BACK_SIZE  = (TouchPoint){1920, 890};
static TouchPoint 
	T_FRONT_L, T_FRONT_R, T_FRONT_TL, T_FRONT_TR, T_FRONT_BL, T_FRONT_BR,
	T_BACK_L, T_BACK_R, T_BACK_TL, T_BACK_TR, T_BACK_BL, T_BACK_BR;
static TouchZone 
	TZ_FRONT_L, TZ_FRONT_R, TZ_FRONT_TL, TZ_FRONT_TR, TZ_FRONT_BL, TZ_FRONT_BR,
	TZ_BACK_L, TZ_BACK_R, TZ_BACK_TL, TZ_BACK_TR, TZ_BACK_BL, TZ_BACK_BR;

const uint32_t HW_BUTTONS[HW_BUTTONS_NUM] = {
	SCE_CTRL_CROSS, SCE_CTRL_CIRCLE, SCE_CTRL_TRIANGLE, SCE_CTRL_SQUARE,
	SCE_CTRL_START, SCE_CTRL_SELECT, SCE_CTRL_LTRIGGER, SCE_CTRL_RTRIGGER,
	SCE_CTRL_UP, SCE_CTRL_RIGHT, SCE_CTRL_LEFT, SCE_CTRL_DOWN, SCE_CTRL_L1,
	SCE_CTRL_R1, SCE_CTRL_L3, SCE_CTRL_R3, 
	SCE_CTRL_VOLUP, SCE_CTRL_VOLDOWN, SCE_CTRL_POWER, SCE_CTRL_PSBUTTON,
	SCE_CTRL_TOUCHPAD
};

typedef struct EmulatedStick{
	uint32_t up, down, left, right;
}EmulatedStick;

typedef struct EmulatedTouch{
	SceTouchReport reports[MULTITOUCH_FRONT_NUM];
	uint8_t num;
}EmulatedTouch;

//For now let's store them on the stack
static SceCtrlData _remappedBuffers[HOOKS_NUM-4][BUFFERS_NUM];
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
bool newEmulatedFrontTouchBuffer = 0;
static SceTouchData *remappedBuffersBack[4];
static int remappedBuffersBackSizes[4];
static int remappedBuffersBackIdxs[4];
bool newEmulatedBackTouchBuffer = 0;

static EmulatedTouch etFront, etBack, prevEtFront, prevEtBack;
static uint8_t etFrontIdCounter = 64;
static uint8_t etBackIdCounter = 64;

bool inZone(const TouchPoint tp, const TouchZone tz){
	if (tp.x > min(tz.a.x, tz.b.x) && tp.x <= max(tz.a.x, tz.b.x) &&
		tp.y > min(tz.a.y, tz.b.y) && tp.y <= max(tz.a.y, tz.b.y))
		return true;
	return false;
}
bool reportInZone(SceTouchReport* str, const TouchZone tz){
	return inZone((TouchPoint){str->x, str->y}, tz);
}

void storeTouchPoint(EmulatedTouch *et, TouchPoint tp){
	for (int i = 0; i < et->num; i++)
		if (et->reports[i].x == tp.x && et->reports[i].y == tp.y)
			return;
	et->reports[et->num].x = tp.x;
	et->reports[et->num].y = tp.y;
	et->num++;
}

void swapTriggersBumpers(SceCtrlData *ctrl){
	uint32_t b = 0;
	for (int j = 0; j < HW_BUTTONS_NUM; j++)
		if (ctrl->buttons & HW_BUTTONS[j]){
			if (HW_BUTTONS[j] == SCE_CTRL_LTRIGGER) b+= SCE_CTRL_L1;
			else if (HW_BUTTONS[j] == SCE_CTRL_L1) b+= SCE_CTRL_LTRIGGER;
			else if (HW_BUTTONS[j] == SCE_CTRL_RTRIGGER) b+= SCE_CTRL_R1;
			else if (HW_BUTTONS[j] == SCE_CTRL_R1) b+= SCE_CTRL_RTRIGGER;
			else b += HW_BUTTONS[j];
	}
	ctrl->buttons = b;
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
				case REMAP_ANALOG_UP:    stick->up    += 127; break;
				case REMAP_ANALOG_DOWN:  stick->down  += 127; break;
				case REMAP_ANALOG_LEFT:  stick->left  += 127; break;
				case REMAP_ANALOG_RIGHT: stick->right += 127; break;
				default: break;
			}
			break;
		case REMAP_TYPE_FRONT_TOUCH_POINT: 
			switch (emu->action){
				case REMAP_TOUCH_ZONE_L:  storeTouchPoint(&etFront, T_FRONT_L);  break;
				case REMAP_TOUCH_ZONE_R:  storeTouchPoint(&etFront, T_FRONT_R);  break;
				case REMAP_TOUCH_ZONE_TL: storeTouchPoint(&etFront, T_FRONT_TL); break;
				case REMAP_TOUCH_ZONE_TR: storeTouchPoint(&etFront, T_FRONT_TR); break;
				case REMAP_TOUCH_ZONE_BL: storeTouchPoint(&etFront, T_FRONT_BL); break;
				case REMAP_TOUCH_ZONE_BR: storeTouchPoint(&etFront, T_FRONT_BR); break;
				case REMAP_TOUCH_CUSTOM:  storeTouchPoint(&etFront, emu->param.touch); break;
				default: break;
			}
			break;
		case REMAP_TYPE_BACK_TOUCH_POINT: 
			switch (emu->action){
				case REMAP_TOUCH_ZONE_L:  storeTouchPoint(&etBack, T_BACK_L);  break;
				case REMAP_TOUCH_ZONE_R:  storeTouchPoint(&etBack, T_BACK_R);  break;
				case REMAP_TOUCH_ZONE_TL: storeTouchPoint(&etBack, T_BACK_TL); break;
				case REMAP_TOUCH_ZONE_TR: storeTouchPoint(&etBack, T_BACK_TR); break;
				case REMAP_TOUCH_ZONE_BL: storeTouchPoint(&etBack, T_BACK_BL); break;
				case REMAP_TOUCH_ZONE_BR: storeTouchPoint(&etBack, T_BACK_BR); break;
				case REMAP_TOUCH_CUSTOM:  storeTouchPoint(&etBack, emu->param.touch); break;
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
				case REMAP_ANALOG_UP:    stick->up    += stickposval; break;
				case REMAP_ANALOG_DOWN:  stick->down  += stickposval; break;
				case REMAP_ANALOG_LEFT:  stick->left  += stickposval; break;
				case REMAP_ANALOG_RIGHT: stick->right += stickposval; break;
				default: break;
			}
			break;
		default: addEmu(emu, btn, emustick); break;
	}
	
}
void addEmuFromGyro(struct RemapAction* emu, uint32_t* btn, EmulatedStick* emustick,  float gyroval){
	EmulatedStick* stick = emustick;
	switch (emu->type){
		case REMAP_TYPE_RIGHT_ANALOG: 
			stick = &emustick[1];
		case REMAP_TYPE_LEFT_ANALOG: 
			switch (emu->action){
				case REMAP_ANALOG_UP:    stick->up    += clamp(gyroval, -127, 127); break;
				case REMAP_ANALOG_DOWN:  stick->down  += clamp(gyroval, -127, 127); break;
				case REMAP_ANALOG_LEFT:  stick->left  += clamp(gyroval, -127, 127); break;
				case REMAP_ANALOG_RIGHT: stick->right += clamp(gyroval, -127, 127); break;
				default: break;
			}
			break;
		default: addEmu(emu, btn, emustick); break;
	}
}

void applyRemap(SceCtrlData *ctrl) {
	if (profile.controller[PROFILE_CONTROLLER_SWAP_BUTTONS])
		swapTriggersBumpers(ctrl);

	// Gathering real touch data
	SceTouchData front, back;
	isInternalTouchCall = true;
	int frontRet = ksceTouchPeek(SCE_TOUCH_PORT_FRONT, &front, 1);
	int backRet = ksceTouchPeek(SCE_TOUCH_PORT_BACK, &back, 1);
	isInternalTouchCall = false;

	uint32_t btns = ctrl->buttons;
	uint32_t propBtns = ctrl->buttons; //Propagated buttons
	uint32_t emuBtns = 0;

	EmulatedStick eSticks[2] =    {(EmulatedStick){0, 0, 0, 0}, (EmulatedStick){0, 0, 0, 0}};
	EmulatedStick propSticks[2] = {(EmulatedStick){0, 0, 0, 0}, (EmulatedStick){0, 0, 0, 0}};

	SceMotionState sms;
	int ret = _sceMotionGetState(&sms);
	double d1 = 0;
	double d2 = 3.0;
	LOG("motion get state_: %lf\n", d2);
	LOG("motion get state : %i [%+1.3f %+1.3f %+1.3f] %+1.3f %+1.3f\n", ret, sms.angularVelocity.x, sms.angularVelocity.y, sms.angularVelocity.z, d1, d2);

	//Set propagation sticks def values
	if (ctrl->lx < 128) propSticks[0].left = 127 - ctrl->lx;
	else propSticks[0].right = ctrl->lx - 127;
	if (ctrl->ly < 128) propSticks[0].up = 127 - ctrl->ly;
	else  propSticks[0].down = ctrl->ly - 127;
	if (ctrl->rx < 128) propSticks[1].left = 127 - ctrl->rx;
	else  propSticks[1].right = ctrl->rx - 127;
	if (ctrl->ry <= 128) propSticks[1].up = 127 - ctrl->ry;
	else  propSticks[1].down = ctrl->ry - 127;

	//Applying remap rules
	for (int i = 0; i < profile.remapsNum; i++){
		struct RemapRule* rr = &profile.remaps[i];
		if (rr->disabled) continue;
		struct RemapAction* trigger = &rr->trigger;

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
				if (frontRet < 1) break;
				for (int j = 0; j < front.reportNum; j++) {
					TouchZone tz = TZ_FRONT_L;
					switch (trigger->action){
						case REMAP_TOUCH_ZONE_L:  tz = TZ_FRONT_L;  break;
						case REMAP_TOUCH_ZONE_R:  tz = TZ_FRONT_R;  break;
						case REMAP_TOUCH_ZONE_TL: tz = TZ_FRONT_TL; break;
						case REMAP_TOUCH_ZONE_TR: tz = TZ_FRONT_TR; break;
						case REMAP_TOUCH_ZONE_BL: tz = TZ_FRONT_BL; break;
						case REMAP_TOUCH_ZONE_BR: tz = TZ_FRONT_BR; break;
						case REMAP_TOUCH_CUSTOM:  tz = trigger->param.zone; break;
						default: break;
					}
					if (reportInZone(&front.report[j], tz)) addEmu(&rr->emu, &emuBtns, eSticks); 
				}
				break;
			case REMAP_TYPE_BACK_TOUCH_ZONE: 
				if (backRet < 1) break;
				for (int j = 0; j < back.reportNum; j++) {
					TouchZone tz = TZ_FRONT_L;
					switch (trigger->action){
						case REMAP_TOUCH_ZONE_L:  tz = TZ_BACK_L;  break;
						case REMAP_TOUCH_ZONE_R:  tz = TZ_BACK_R;  break;
						case REMAP_TOUCH_ZONE_TL: tz = TZ_BACK_TL; break;
						case REMAP_TOUCH_ZONE_TR: tz = TZ_BACK_TR; break;
						case REMAP_TOUCH_ZONE_BL: tz = TZ_BACK_BL; break;
						case REMAP_TOUCH_ZONE_BR: tz = TZ_BACK_BR; break;
						case REMAP_TOUCH_CUSTOM:  tz = trigger->param.zone; break;
						default: break;
					}
					if (reportInZone(&back.report[j], tz)) addEmu(&rr->emu, &emuBtns, eSticks); 
				}
				break;
			case REMAP_TYPE_GYROSCOPE: 
				if (ret != 0) break;
				switch (trigger->action){
					case REMAP_GYRO_UP:  
						if (sms.angularVelocity.x > 0) 
							addEmuFromGyro(&rr->emu, &emuBtns, eSticks, sms.angularVelocity.y * profile.gyro[1]);
						break;
					case REMAP_GYRO_DOWN:
						if (sms.angularVelocity.x < 0) 
							addEmuFromGyro(&rr->emu, &emuBtns, eSticks, - sms.angularVelocity.y * profile.gyro[1]);
						break;
					case REMAP_GYRO_LEFT:
						if (sms.angularVelocity.y > 0) 
							addEmuFromGyro(&rr->emu, &emuBtns, eSticks, sms.angularVelocity.x * profile.gyro[0]);
						break;
					case REMAP_GYRO_RIGHT:
						if (sms.angularVelocity.y < 0) 
							addEmuFromGyro(&rr->emu, &emuBtns, eSticks, - sms.angularVelocity.x * profile.gyro[0]);
						break;
					case REMAP_GYRO_ROLL_LEFT:
						if (sms.deviceQuat.z > 0) 
							addEmuFromGyro(&rr->emu, &emuBtns, eSticks, sms.deviceQuat.z * profile.gyro[2] * 4);
						break;
					case REMAP_GYRO_ROLL_RIGHT:
						if (sms.deviceQuat.z < 0) 
							addEmuFromGyro(&rr->emu, &emuBtns, eSticks, - sms.deviceQuat.z * profile.gyro[2] * 4);
						break;
					default: break;
				}
				break;
			default: break;
		}
	}

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
	ctrl->lx = clamp(127  + eSticks[0].right- eSticks[0].left, 0, 255);
	ctrl->ly = clamp(127 + eSticks[0].down - eSticks[0].up, 0, 255);
	ctrl->rx = clamp(127 + eSticks[1].right - eSticks[1].left, 0, 255);
	ctrl->ry = clamp(127 + eSticks[1].down - eSticks[1].up, 0, 255);

	//Telling that new emulated touch buffer is ready to be takn
	newEmulatedFrontTouchBuffer = true;
	newEmulatedBackTouchBuffer = false;
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

void removeReport(SceTouchData *pData, int idx){
	for (int i = idx + 1; i < pData->reportNum; i++)
		pData->report[i - 1] = pData->report[i];
	pData->reportNum--;
}
void updateTouchInfo(SceUInt32 port, SceTouchData *pData){	
	if (port == SCE_TOUCH_PORT_FRONT) {
		// Remove non-propagated remapped touches
		for (int i = 0; i < profile.remapsNum; i++)
			if (!profile.remaps[i].propagate && !profile.remaps[i].disabled &&
				profile.remaps[i].trigger.type == REMAP_TYPE_FRONT_TOUCH_ZONE){
				int j = 0;
				while (j < pData->reportNum){
					TouchZone tz = TZ_FRONT_L;
					switch (profile.remaps[i].trigger.action){
						case REMAP_TOUCH_ZONE_L:  tz = TZ_FRONT_L;  break;
						case REMAP_TOUCH_ZONE_R:  tz = TZ_FRONT_R;  break;
						case REMAP_TOUCH_ZONE_TL: tz = TZ_FRONT_TL; break;
						case REMAP_TOUCH_ZONE_TR: tz = TZ_FRONT_TR; break;
						case REMAP_TOUCH_ZONE_BL: tz = TZ_FRONT_BL; break;
						case REMAP_TOUCH_ZONE_BR: tz = TZ_FRONT_BR; break;
						case REMAP_TOUCH_CUSTOM:  tz = profile.remaps[i].trigger.param.zone; break;
						default: break;
					}
					if (reportInZone(&pData->report[j], tz))
						removeReport(pData, j);
					else 
						j++;
				}
			}

		if (!newEmulatedFrontTouchBuffer){//New touchbuffer not ready - using previous one
			addVirtualTouches(pData, &prevEtFront, 
				MULTITOUCH_FRONT_NUM, SCE_TOUCH_PORT_FRONT);
			return;
		}
		
		addVirtualTouches(pData, &etFront, 
			MULTITOUCH_FRONT_NUM, SCE_TOUCH_PORT_FRONT);
		prevEtFront = etFront;
		etFront.num = 0;
		newEmulatedFrontTouchBuffer = false;
	} else {
		// Remove non-propagated remapped touches
		for (int i = 0; i < profile.remapsNum; i++)
			if (!profile.remaps[i].propagate && !profile.remaps[i].disabled &&
				profile.remaps[i].trigger.type == REMAP_TYPE_BACK_TOUCH_ZONE){
				int j = 0;
				while (j < pData->reportNum){
					TouchZone tz = TZ_BACK_L;
					switch (profile.remaps[i].trigger.action){
						case REMAP_TOUCH_ZONE_L:  tz = TZ_BACK_L;  break;
						case REMAP_TOUCH_ZONE_R:  tz = TZ_BACK_R;  break;
						case REMAP_TOUCH_ZONE_TL: tz = TZ_BACK_TL; break;
						case REMAP_TOUCH_ZONE_TR: tz = TZ_BACK_TR; break;
						case REMAP_TOUCH_ZONE_BL: tz = TZ_BACK_BL; break;
						case REMAP_TOUCH_ZONE_BR: tz = TZ_BACK_BR; break;
						case REMAP_TOUCH_CUSTOM:  tz = profile.remaps[i].trigger.param.zone; break;
						default: break;
					}
					if (reportInZone(&pData->report[j], tz))
						removeReport(pData, j);
					else 
						j++;
				}
			}
			
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

void initTouchParams(){
	T_FRONT_L = (TouchPoint){T_FRONT_SIZE.x / 6, T_FRONT_SIZE.y / 2};
	T_FRONT_R = (TouchPoint){T_FRONT_SIZE.x * 5 / 6, T_FRONT_SIZE.y / 2};
	T_FRONT_TL = (TouchPoint){T_FRONT_SIZE.x / 6, T_FRONT_SIZE.y / 4};
	T_FRONT_TR = (TouchPoint){T_FRONT_SIZE.x * 5 / 6, T_FRONT_SIZE.y / 4};
	T_FRONT_BL = (TouchPoint){T_FRONT_SIZE.x / 6, T_FRONT_SIZE.y * 3 / 4};
	T_FRONT_BR = (TouchPoint){T_FRONT_SIZE.x * 5 / 6, T_FRONT_SIZE.y * 3 / 4};
	T_BACK_L = (TouchPoint){T_BACK_SIZE.x / 6, T_BACK_SIZE.y / 2};
	T_BACK_R = (TouchPoint){T_BACK_SIZE.x * 5 / 6, T_BACK_SIZE.y / 2};
	T_BACK_TL = (TouchPoint){T_BACK_SIZE.x / 6, T_BACK_SIZE.y / 4};
	T_BACK_TR = (TouchPoint){T_BACK_SIZE.x * 5 / 6, T_BACK_SIZE.y / 4};
	T_BACK_BL = (TouchPoint){T_BACK_SIZE.x / 6, T_BACK_SIZE.y * 3 / 4};
	T_BACK_BR = (TouchPoint){T_BACK_SIZE.x * 5 / 6, T_BACK_SIZE.y * 3 / 4};

	TZ_FRONT_L = (TouchZone){
		(TouchPoint){0, 0}, 
		(TouchPoint){T_FRONT_SIZE.x / 2, T_FRONT_SIZE.y}};
	TZ_FRONT_R = (TouchZone){
		(TouchPoint){T_FRONT_SIZE.x / 2, 0}, 
		(TouchPoint){T_FRONT_SIZE.x, T_FRONT_SIZE.y}};
	TZ_FRONT_TL = (TouchZone){
		(TouchPoint){0, 0}, 
		(TouchPoint){T_FRONT_SIZE.x / 2, T_FRONT_SIZE.y / 2}};
	TZ_FRONT_TR = (TouchZone){
		(TouchPoint){T_FRONT_SIZE.x / 2, 0}, 
		(TouchPoint){T_FRONT_SIZE.x, T_FRONT_SIZE.y / 2}};
	TZ_FRONT_BL = (TouchZone){
		(TouchPoint){0, T_FRONT_SIZE.y / 2}, 
		(TouchPoint){T_FRONT_SIZE.x / 2, T_FRONT_SIZE.y}};
	TZ_FRONT_BR = (TouchZone){
		(TouchPoint){T_FRONT_SIZE.x / 2, T_FRONT_SIZE.y / 2}, 
		(TouchPoint){T_FRONT_SIZE.x, T_FRONT_SIZE.y}};

	TZ_BACK_L = (TouchZone){
		(TouchPoint){0, 0}, 
		(TouchPoint){T_BACK_SIZE.x / 2, T_BACK_SIZE.y}};
	TZ_BACK_R = (TouchZone){
		(TouchPoint){T_BACK_SIZE.x / 2, 0}, 
		(TouchPoint){T_BACK_SIZE.x, T_BACK_SIZE.y}};
	TZ_BACK_TL = (TouchZone){
		(TouchPoint){0, 0}, 
		(TouchPoint){T_BACK_SIZE.x / 2, T_BACK_SIZE.y / 2}};
	TZ_BACK_TR = (TouchZone){
		(TouchPoint){T_BACK_SIZE.x / 2, 0}, 
		(TouchPoint){T_BACK_SIZE.x, T_BACK_SIZE.y / 2}};
	TZ_BACK_BL = (TouchZone){
		(TouchPoint){0, T_BACK_SIZE.y / 2}, 
		(TouchPoint){T_BACK_SIZE.x / 2, T_BACK_SIZE.y}};
	TZ_BACK_BR = (TouchZone){
		(TouchPoint){T_BACK_SIZE.x / 2, T_BACK_SIZE.y / 2}, 
		(TouchPoint){T_BACK_SIZE.x, T_BACK_SIZE.y}};
}
void remap_setup(){
    //ksceKernelGetSystemTimeLow

	// Enabling analogs sampling 
	ksceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG_WIDE);
	
	// Enabling both touch panels sampling
	ksceTouchSetSamplingState(SCE_TOUCH_PORT_FRONT, SCE_TOUCH_SAMPLING_STATE_START);
	ksceTouchSetSamplingState(SCE_TOUCH_PORT_BACK, SCE_TOUCH_SAMPLING_STATE_START);
	
	// Detecting touch panels size
	SceTouchPanelInfo pi;	
	if (ksceTouchGetPanelInfo(SCE_TOUCH_PORT_FRONT, &pi) >= 0){
		T_FRONT_SIZE.x = pi.maxAaX;
		T_FRONT_SIZE.y = pi.maxAaY;
	}
	if (ksceTouchGetPanelInfo(SCE_TOUCH_PORT_BACK, &pi) >= 0){
		T_BACK_SIZE.x = pi.maxAaX;
		T_BACK_SIZE.y = pi.maxAaY;
	}
}

void remap_init(){
	for (int i = 0; i < HOOKS_NUM-5; i++) //Allocating mem for stored buffers
		remappedBuffers[i] = &_remappedBuffers[i][0];
	for (int i = 0; i < 4; i++){
		remappedBuffersFront[i] = (SceTouchData*)&_remappedBuffersFront[i][0];
		remappedBuffersBack[i] = (SceTouchData*)&_remappedBuffersBack[i][0];
	}
	initTouchParams();
}

void remap_destroy(){

}
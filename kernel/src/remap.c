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

#define CTRL_HOOKS_NUM 				HOOKS_NUM - 4
#define TOUCH_HOOKS_NUM 			2
#define MULTITOUCH_FRONT_NUM		6
#define MULTITOUCH_BACK_NUM			4
#define TURBO_DELAY			        50*1000

enum RULE_STATUS{
	RS_NONACTIVE,   //Rule is stopped for some time
	RS_STARTED,     //Rule just started
	RS_ACTIVE,      //Rule is active for some time
	RS_STOPPED      //Rule just stopped
}RULE_STATUS;

//Status of each rule
enum RULE_STATUS rs[CTRL_HOOKS_NUM][REMAP_NUM];

TouchPoint T_FRONT_SIZE = (TouchPoint){1920, 1080};
TouchPoint T_BACK_SIZE  = (TouchPoint){1920, 890};
static TouchPoint 
	T_FRONT_L, T_FRONT_R, T_FRONT_TL, T_FRONT_TR, T_FRONT_BL, T_FRONT_BR,
	T_BACK_L, T_BACK_R, T_BACK_TL, T_BACK_TR, T_BACK_BL, T_BACK_BR;
static TouchPoints2 
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

typedef struct RuleData{
	SceCtrlData *ctrl;
	uint32_t btns, btnsEmu, btnsProp;
	struct RemapRule* rr;
	EmulatedStick analogLeftEmu, analogRightEmu, analogLeftProp, analogRightProp;
	uint8_t stickposval;
	enum RULE_STATUS* status;
	bool isTurboTick;
}RuleData;

typedef struct EmulatedTouchEvent{
	TouchPoint point, swipeEndPoint;
	int id;
	int64_t tick;
	bool isSwipe;
	bool isSmartSwipe;
	bool isSwipeFinished;
}EmulatedTouchEvent;

typedef struct EmulatedTouch{
	EmulatedTouchEvent reports[MULTITOUCH_FRONT_NUM];
	uint8_t num;
}EmulatedTouch;

//For now let's store buffers on the stack
static SceCtrlData _remappedBuffers[CTRL_HOOKS_NUM][BUFFERS_NUM];
static SceTouchData _remappedBuffersFront[TOUCH_HOOKS_NUM][BUFFERS_NUM];
static SceTouchData _remappedBuffersBack[TOUCH_HOOKS_NUM][BUFFERS_NUM];

//Circular cache to store remapped keys buffers per each ctrs hook
static SceCtrlData *remappedBuffers[CTRL_HOOKS_NUM];
static int remappedBuffersSizes[CTRL_HOOKS_NUM];
static int remappedBuffersIdxs[CTRL_HOOKS_NUM];

//Circular cache to store Touch buffers per each touch hook
static SceTouchData *remappedBuffersFront[TOUCH_HOOKS_NUM];
static int remappedBuffersFrontSizes[TOUCH_HOOKS_NUM];
static int remappedBuffersFrontIdxs[TOUCH_HOOKS_NUM];
bool newEmulatedFrontTouchBuffer = false;
static SceTouchData *remappedBuffersBack[TOUCH_HOOKS_NUM];
static int remappedBuffersBackSizes[TOUCH_HOOKS_NUM];
static int remappedBuffersBackIdxs[TOUCH_HOOKS_NUM];
bool newEmulatedBackTouchBuffer = false;

static EmulatedTouch etFront, etBack, prevEtFront, prevEtBack;
static uint8_t etFrontIdCounter = 64;
static uint8_t etBackIdCounter = 64;

bool updateStatus(enum RULE_STATUS* status, bool active){
	switch (*status){
		case RS_ACTIVE: if (!active) *status = RS_STOPPED; break;
		case RS_NONACTIVE: if (active) *status = RS_STARTED; break;
		case RS_STARTED: *status = active ? RS_ACTIVE : RS_STOPPED; break;
		case RS_STOPPED: *status = active ? RS_STARTED : RS_NONACTIVE; break;
	}
	return active;
}

bool inZone(const TouchPoint tp, const TouchPoints2 tz){
	if (tp.x > min(tz.a.x, tz.b.x) && tp.x <= max(tz.a.x, tz.b.x) &&
		tp.y > min(tz.a.y, tz.b.y) && tp.y <= max(tz.a.y, tz.b.y))
		return true;
	return false;
}
bool reportInZone(SceTouchReport* str, const TouchPoints2 tz){
	return inZone((TouchPoint){str->x, str->y}, tz);
}

void storeTouchPoint(EmulatedTouch *et, TouchPoint tp){
	for (int i = 0; i < et->num; i++)
		if (et->reports[i].point.x == tp.x && et->reports[i].point.y == tp.y)
			return;
	et->reports[et->num].point.x = tp.x;
	et->reports[et->num].point.y = tp.y;
	et->reports[et->num].isSwipe = false;
	et->reports[et->num].isSmartSwipe = false;
	et->num++;
}

void storeTouchSwipe(EmulatedTouch *et, TouchPoints2 tps){
	for (int i = 0; i < et->num; i++)
		if (et->reports[i].isSwipe == true &&
				et->reports[i].point.x == tps.a.x && et->reports[i].point.y == tps.a.y && 
				et->reports[i].swipeEndPoint.x == tps.b.x && et->reports[i].swipeEndPoint.y == tps.b.y)
			return;
	et->reports[et->num].point.x = tps.a.x;
	et->reports[et->num].point.y = tps.a.y;
	et->reports[et->num].swipeEndPoint.x = tps.b.x;
	et->reports[et->num].swipeEndPoint.y = tps.b.y;
	et->reports[et->num].tick = 0;
	et->reports[et->num].isSwipe = true;
	et->reports[et->num].isSmartSwipe = false;
	et->reports[et->num].isSwipeFinished = false;
	et->num++;
}

EmulatedTouchEvent* storeTouchSmartSwipe(EmulatedTouch *et, TouchPoint tp){
	for (int i = 0; i < et->num; i++)
		if (et->reports[i].isSmartSwipe == true &&
				et->reports[i].point.x == tp.x && et->reports[i].point.y == tp.y)
			return &et->reports[i];
	EmulatedTouchEvent* ete = &et->reports[et->num];
	ete->point.x = tp.x;
	ete->point.y = tp.y;
	ete->swipeEndPoint.x = tp.x;
	ete->swipeEndPoint.y = tp.y;
	ete->id = -1;
	ete->isSwipe = false;
	ete->isSmartSwipe = true;
	ete->isSwipeFinished = false;
	et->num++;
	return ete;
}

void removeEmuReport(EmulatedTouch *et, int idx){
	for (int i = idx + 1; i < et->num; i++)
		et->reports[i - 1] = et->reports[i];
	et->num--;
}

void removeEmuSmartSwipe(EmulatedTouch *et, TouchPoint tp){
	int i = 0;
	while (i < et->num){
		if (et->reports[i].isSmartSwipe && et->reports[i].point.x == tp.x && et->reports[i].point.y == tp.y) {
			removeEmuReport(et, i);
			break;
		}
		i++;
	}
}

void cleanEmuReports(EmulatedTouch *et){
	int i = 0;
	while (i < et->num)
		if ((!et->reports[i].isSwipe && !et->reports[i].isSmartSwipe) || et->reports[i].isSwipeFinished) 
			removeEmuReport(et, i);
		else 
			i++;
}

void swapTriggersBumpers(SceCtrlData *ctrl){
	uint32_t b = 0;
	for (int j = 0; j < HW_BUTTONS_NUM; j++)
		if (btn_has(ctrl->buttons, HW_BUTTONS[j]))
			switch(HW_BUTTONS[j]){
				case SCE_CTRL_LTRIGGER: btn_add(&b, SCE_CTRL_L1);       break;
				case SCE_CTRL_L1:       btn_add(&b, SCE_CTRL_LTRIGGER); break;
				case SCE_CTRL_RTRIGGER: btn_add(&b, SCE_CTRL_R1);       break;
				case SCE_CTRL_R1:       btn_add(&b, SCE_CTRL_RTRIGGER); break;
				default:                btn_add(&b, HW_BUTTONS[j]);     break;
			}
	ctrl->buttons = b;
}

// void addEmu(SceCtrlData *ctrl, struct RemapAction* emu, uint32_t* btn, EmulatedStick* emustick, bool isFirstTime) {
void addEmu(RuleData* rd) {
	EmulatedStick* stick = &rd->analogLeftEmu;
	struct RemapAction* emu = &rd->rr->emu;
	EmulatedTouchEvent* ete;
	switch (emu->type){
		case REMAP_TYPE_BUTTON: 
			if (rd->rr->turbo && rd->isTurboTick) break;
			btn_add(&rd->btnsEmu, emu->param.btn);
			break;
		case REMAP_TYPE_RIGHT_ANALOG: 
		case REMAP_TYPE_RIGHT_ANALOG_DIGITAL: 
			stick = &rd->analogRightEmu;
		case REMAP_TYPE_LEFT_ANALOG: 
		case REMAP_TYPE_LEFT_ANALOG_DIGITAL: 
			if (rd->rr->turbo && rd->isTurboTick) break;
			switch (emu->action){
				case REMAP_ANALOG_UP:    stick->up    += rd->stickposval; break;
				case REMAP_ANALOG_DOWN:  stick->down  += rd->stickposval; break;
				case REMAP_ANALOG_LEFT:  stick->left  += rd->stickposval; break;
				case REMAP_ANALOG_RIGHT: stick->right += rd->stickposval; break;
				default: break;
			}
			break;
		case REMAP_TYPE_FRONT_TOUCH_POINT: 
			if (rd->rr->turbo && rd->isTurboTick && 
				emu->action != REMAP_TOUCH_SWIPE && emu->action != REMAP_TOUCH_SWIPE_SMART_L && emu->action != REMAP_TOUCH_SWIPE_SMART_R)
					break;
			switch (emu->action){
				case REMAP_TOUCH_ZONE_L:  storeTouchPoint(&etFront, T_FRONT_L);  break;
				case REMAP_TOUCH_ZONE_R:  storeTouchPoint(&etFront, T_FRONT_R);  break;
				case REMAP_TOUCH_ZONE_TL: storeTouchPoint(&etFront, T_FRONT_TL); break;
				case REMAP_TOUCH_ZONE_TR: storeTouchPoint(&etFront, T_FRONT_TR); break;
				case REMAP_TOUCH_ZONE_BL: storeTouchPoint(&etFront, T_FRONT_BL); break;
				case REMAP_TOUCH_ZONE_BR: storeTouchPoint(&etFront, T_FRONT_BR); break;
				case REMAP_TOUCH_CUSTOM:  storeTouchPoint(&etFront, emu->param.tPoint); break;
				case REMAP_TOUCH_SWIPE:   
					if (*rd->status == RS_STARTED || (*rd->status == RS_ACTIVE && rd->rr->turbo && rd->isTurboTick)) 
						storeTouchSwipe(&etFront, emu->param.tPoints); 
					break;
				case REMAP_TOUCH_SWIPE_SMART_DPAD:  
					ete = storeTouchSmartSwipe(&etFront, emu->param.tPoint);
					if (btn_has(rd->btns, SCE_CTRL_LEFT)) ete->swipeEndPoint.x -= profile.touch[PROFILE_TOUCH_SWIPE_SMART_SENSIVITY];
					if (btn_has(rd->btns, SCE_CTRL_RIGHT)) ete->swipeEndPoint.x += profile.touch[PROFILE_TOUCH_SWIPE_SMART_SENSIVITY];
					if (btn_has(rd->btns, SCE_CTRL_UP)) ete->swipeEndPoint.y -= profile.touch[PROFILE_TOUCH_SWIPE_SMART_SENSIVITY];
					if (btn_has(rd->btns, SCE_CTRL_DOWN)) ete->swipeEndPoint.y += profile.touch[PROFILE_TOUCH_SWIPE_SMART_SENSIVITY];
					break;
				case REMAP_TOUCH_SWIPE_SMART_L:  
					ete = storeTouchSmartSwipe(&etFront, emu->param.tPoint);
					if (abs(127 - rd->ctrl->lx) > profile.analog[PROFILE_ANALOG_LEFT_DEADZONE_X])
						ete->swipeEndPoint.x = clamp(ete->swipeEndPoint.x + 
							((float)(rd->ctrl->lx - 127)) * profile.touch[PROFILE_TOUCH_SWIPE_SMART_SENSIVITY] / 127, 
							0, T_FRONT_SIZE.x);
					if (abs(127 - rd->ctrl->ly) > profile.analog[PROFILE_ANALOG_LEFT_DEADZONE_Y])
						ete->swipeEndPoint.y = clamp(ete->swipeEndPoint.y + 
							((float)(rd->ctrl->ly - 127)) * profile.touch[PROFILE_TOUCH_SWIPE_SMART_SENSIVITY] / 127, 
							0, T_FRONT_SIZE.y);
					break;
				case REMAP_TOUCH_SWIPE_SMART_R:  
					ete = storeTouchSmartSwipe(&etFront, emu->param.tPoint); 
					if (abs(127 - rd->ctrl->rx) > profile.analog[PROFILE_ANALOG_LEFT_DEADZONE_X])
						ete->swipeEndPoint.x = clamp(ete->swipeEndPoint.x + 
							((float)(rd->ctrl->rx - 127)) * profile.touch[PROFILE_TOUCH_SWIPE_SMART_SENSIVITY] / 127, 
							0, T_FRONT_SIZE.x);
					if (abs(127 - rd->ctrl->ry) > profile.analog[PROFILE_ANALOG_LEFT_DEADZONE_X])
						ete->swipeEndPoint.y = clamp(ete->swipeEndPoint.y + 
							((float)(rd->ctrl->ry - 127)) * profile.touch[PROFILE_TOUCH_SWIPE_SMART_SENSIVITY] / 127, 
							0, T_FRONT_SIZE.y);
					break;
				default: break;
			}
			break;
		case REMAP_TYPE_BACK_TOUCH_POINT: 
			if (rd->rr->turbo && rd->isTurboTick && 
				emu->action != REMAP_TOUCH_SWIPE && emu->action != REMAP_TOUCH_SWIPE_SMART_L && emu->action != REMAP_TOUCH_SWIPE_SMART_R)
					break;
			switch (emu->action){
				case REMAP_TOUCH_ZONE_L:  storeTouchPoint(&etBack, T_BACK_L);  break;
				case REMAP_TOUCH_ZONE_R:  storeTouchPoint(&etBack, T_BACK_R);  break;
				case REMAP_TOUCH_ZONE_TL: storeTouchPoint(&etBack, T_BACK_TL); break;
				case REMAP_TOUCH_ZONE_TR: storeTouchPoint(&etBack, T_BACK_TR); break;
				case REMAP_TOUCH_ZONE_BL: storeTouchPoint(&etBack, T_BACK_BL); break;
				case REMAP_TOUCH_ZONE_BR: storeTouchPoint(&etBack, T_BACK_BR); break;
				case REMAP_TOUCH_CUSTOM:  storeTouchPoint(&etBack, emu->param.tPoint); break;
				case REMAP_TOUCH_SWIPE:   
					if (*rd->status == RS_STARTED || (*rd->status == RS_ACTIVE && rd->rr->turbo && rd->isTurboTick)) 
						storeTouchSwipe(&etBack, emu->param.tPoints); 
					break;
				case REMAP_TOUCH_SWIPE_SMART_DPAD:  
					ete = storeTouchSmartSwipe(&etFront, emu->param.tPoint);
					if (btn_has(rd->btns, SCE_CTRL_LEFT)) ete->swipeEndPoint.x -= profile.touch[PROFILE_TOUCH_SWIPE_SMART_SENSIVITY];
					if (btn_has(rd->btns, SCE_CTRL_RIGHT)) ete->swipeEndPoint.x += profile.touch[PROFILE_TOUCH_SWIPE_SMART_SENSIVITY];
					if (btn_has(rd->btns, SCE_CTRL_UP)) ete->swipeEndPoint.y -= profile.touch[PROFILE_TOUCH_SWIPE_SMART_SENSIVITY];
					if (btn_has(rd->btns, SCE_CTRL_DOWN)) ete->swipeEndPoint.y += profile.touch[PROFILE_TOUCH_SWIPE_SMART_SENSIVITY];
					break;
				case REMAP_TOUCH_SWIPE_SMART_L:   
					ete = storeTouchSmartSwipe(&etBack, emu->param.tPoint);
					if (abs(127 - rd->ctrl->lx) > profile.analog[PROFILE_ANALOG_LEFT_DEADZONE_X])
						ete->swipeEndPoint.x = clamp(ete->swipeEndPoint.x + 
							((float)(rd->ctrl->lx - 127)) * profile.touch[PROFILE_TOUCH_SWIPE_SMART_SENSIVITY] / 127, 
							0, T_FRONT_SIZE.x);
					if (abs(127 - rd->ctrl->ly) > profile.analog[PROFILE_ANALOG_LEFT_DEADZONE_Y])
						ete->swipeEndPoint.y = clamp(ete->swipeEndPoint.y + 
							((float)(rd->ctrl->ly - 127)) * profile.touch[PROFILE_TOUCH_SWIPE_SMART_SENSIVITY] / 127, 
							0, T_FRONT_SIZE.y);
					break;
				case REMAP_TOUCH_SWIPE_SMART_R:  
					ete = storeTouchSmartSwipe(&etBack, emu->param.tPoint); 
					if (abs(127 - rd->ctrl->rx) > profile.analog[PROFILE_ANALOG_LEFT_DEADZONE_X])
						ete->swipeEndPoint.x = clamp(ete->swipeEndPoint.x + 
							((float)(rd->ctrl->rx - 127)) * profile.touch[PROFILE_TOUCH_SWIPE_SMART_SENSIVITY] / 127, 
							0, T_FRONT_SIZE.x);
					if (abs(127 - rd->ctrl->ry) > profile.analog[PROFILE_ANALOG_LEFT_DEADZONE_X])
						ete->swipeEndPoint.y = clamp(ete->swipeEndPoint.y + 
							((float)(rd->ctrl->ry - 127)) * profile.touch[PROFILE_TOUCH_SWIPE_SMART_SENSIVITY] / 127, 
							0, T_FRONT_SIZE.y);
					break;
				default: break;
			}
			break;
		default: break;
	}
}

void remEmu(RuleData* rd) {
	EmulatedTouch* et = &etBack;
	switch (rd->rr->emu.type){
		case REMAP_TYPE_FRONT_TOUCH_POINT: 
			et = &etFront;
		case REMAP_TYPE_BACK_TOUCH_POINT: 
			switch (rd->rr->emu.action){
				case REMAP_TOUCH_SWIPE_SMART_L:   
				case REMAP_TOUCH_SWIPE_SMART_R:   
				case REMAP_TOUCH_SWIPE_SMART_DPAD:   
					removeEmuSmartSwipe(et, rd->rr->emu.param.tPoint);
					break;
				default: break;
			}
			break;
		default: break;
	}
}

void applyRemap(SceCtrlData *ctrl, enum RULE_STATUS * statuses) {
	if (profile.controller[PROFILE_CONTROLLER_SWAP_BUTTONS])
		swapTriggersBumpers(ctrl);
	
	// Gathering real touch data
	SceTouchData front, back;
	isInternalTouchCall = true;
	int frontRet = ksceTouchPeek(SCE_TOUCH_PORT_FRONT, &front, 1);
	int backRet = ksceTouchPeek(SCE_TOUCH_PORT_BACK, &back, 1);
	isInternalTouchCall = false;

	RuleData rd;
	rd.isTurboTick = (ksceKernelGetSystemTimeWide() % TURBO_DELAY) < (TURBO_DELAY / 2);
	rd.btns = ctrl->buttons;
	rd.btnsProp = ctrl->buttons;
	rd.btnsEmu = 0;
	rd.ctrl = ctrl;

	SceMotionState sms;
	// int ret = _sceMotionGetState(&sms);
	int gyroRet = -1;

	//Set sticks def values
	rd.analogLeftEmu = rd.analogRightEmu = rd.analogLeftProp = rd.analogRightProp = (EmulatedStick){0, 0, 0, 0};
	if (ctrl->lx <= 127) rd.analogLeftProp.left = 127 - ctrl->lx;
	else rd.analogLeftProp.right = ctrl->lx - 127;
	if (ctrl->ly <= 127) rd.analogLeftProp.up = 127 - ctrl->ly;
	else  rd.analogLeftProp.down = ctrl->ly - 127;
	if (ctrl->rx <= 127) rd.analogRightProp.left = 127 - ctrl->rx;
	else  rd.analogRightProp.right = ctrl->rx - 127;
	if (ctrl->ry <= 127) rd.analogRightProp.up = 127 - ctrl->ry;
	else  rd.analogRightProp.down = ctrl->ry - 127;

	//Applying remap rules
	for (int i = 0; i < profile.remapsNum; i++){
		struct RemapRule* rr = &profile.remaps[i];
		rd.rr = &profile.remaps[i];
		if (rd.rr->disabled) continue;
		struct RemapAction* trigger = &rd.rr->trigger;
		rd.stickposval = 127;
		rd.status = &statuses[i];

		switch (trigger->type){
			case REMAP_TYPE_BUTTON: 
				if (updateStatus(rd.status, btn_has(rd.btns, trigger->param.btn))){
					if (!rd.rr->propagate) btn_del(&rd.btnsProp, trigger->param.btn);
					addEmu(&rd);
				} else {
					remEmu(&rd);
				}
				break;
			case REMAP_TYPE_LEFT_ANALOG: 
				switch(trigger->action){
					case REMAP_ANALOG_LEFT: 
						if (updateStatus(rd.status, ctrl->lx <= 127 - profile.analog[PROFILE_ANALOG_LEFT_DEADZONE_X])){
							if (!rr->propagate) rd.analogLeftProp.left = 0;
							rd.stickposval = 127 - ctrl->lx;
							addEmu(&rd);
						} else {
							remEmu(&rd);
						}
						break;
					case REMAP_ANALOG_RIGHT: 
						if (updateStatus(rd.status, ctrl->lx > 127 + profile.analog[PROFILE_ANALOG_LEFT_DEADZONE_X])){
							if (!rr->propagate) rd.analogLeftProp.right = 0;
							rd.stickposval = ctrl->lx - 127;
							addEmu(&rd);
						} else {
							remEmu(&rd);
						}
						break;
					case REMAP_ANALOG_UP: 
						if (updateStatus(rd.status, ctrl->ly <= 127 - profile.analog[PROFILE_ANALOG_LEFT_DEADZONE_Y])){
							if (!rr->propagate) rd.analogLeftProp.right = 0;
							rd.stickposval = 127 - ctrl->ly;
							addEmu(&rd);
						} else {
							remEmu(&rd);
						}
						break;
					case REMAP_ANALOG_DOWN: 
						if (updateStatus(rd.status, ctrl->ly > 127 + profile.analog[PROFILE_ANALOG_LEFT_DEADZONE_Y])){
							if (!rr->propagate) rd.analogLeftProp.right = 0;
							rd.stickposval = ctrl->ly - 127;
							addEmu(&rd);
						} else {
							remEmu(&rd);
						}
						break;
					default: break;
				}
				break;
			case REMAP_TYPE_RIGHT_ANALOG: 
				switch(trigger->action){
					case REMAP_ANALOG_LEFT: 
						if (updateStatus(rd.status, ctrl->rx <= 127 - profile.analog[PROFILE_ANALOG_RIGHT_DEADZONE_X])){
							if (!rr->propagate) rd.analogRightProp.left = 0;
							rd.stickposval = 127 - ctrl->rx;
							addEmu(&rd);
						} else {
							remEmu(&rd);
						}
						break;
					case REMAP_ANALOG_RIGHT: 
						if (updateStatus(rd.status, ctrl->rx > 127 - profile.analog[PROFILE_ANALOG_RIGHT_DEADZONE_X])){
							if (!rr->propagate) rd.analogRightProp.right = 0;
							rd.stickposval = ctrl->rx - 127;
							addEmu(&rd);
						} else {
							remEmu(&rd);
						}
						break;
					case REMAP_ANALOG_UP: 
						if (updateStatus(rd.status, ctrl->ry <= 127 - profile.analog[PROFILE_ANALOG_RIGHT_DEADZONE_Y])){
							if (!rr->propagate) rd.analogRightProp.up = 0;
							rd.stickposval = 127 - ctrl->ry;
							addEmu(&rd);
						} else {
							remEmu(&rd);
						}
						break;
					case REMAP_ANALOG_DOWN: 
						if (updateStatus(rd.status, ctrl->ry > 127 - profile.analog[PROFILE_ANALOG_RIGHT_DEADZONE_Y])){
							if (!rr->propagate) rd.analogRightProp.down = 0;
							rd.stickposval = ctrl->ry - 127;
							addEmu(&rd);
						} else {
							remEmu(&rd);
						}
						break;
					default: break;
				}
				break;
			case REMAP_TYPE_FRONT_TOUCH_ZONE:
				if (frontRet < 1) break;
				for (int j = 0; j < front.reportNum; j++) {
					TouchPoints2 tz = TZ_FRONT_L;
					switch (trigger->action){
						case REMAP_TOUCH_ZONE_L:  tz = TZ_FRONT_L;  break;
						case REMAP_TOUCH_ZONE_R:  tz = TZ_FRONT_R;  break;
						case REMAP_TOUCH_ZONE_TL: tz = TZ_FRONT_TL; break;
						case REMAP_TOUCH_ZONE_TR: tz = TZ_FRONT_TR; break;
						case REMAP_TOUCH_ZONE_BL: tz = TZ_FRONT_BL; break;
						case REMAP_TOUCH_ZONE_BR: tz = TZ_FRONT_BR; break;
						case REMAP_TOUCH_CUSTOM:  tz = trigger->param.tPoints; break;
						default: break;
					}
					if (updateStatus(rd.status, reportInZone(&front.report[j], tz))) {
						addEmu(&rd);
					} else {
						remEmu(&rd);
					}
				}
				break;
			case REMAP_TYPE_BACK_TOUCH_ZONE: 
				if (backRet < 1) break;
				for (int j = 0; j < back.reportNum; j++) {
					TouchPoints2 tz = TZ_FRONT_L;
					switch (trigger->action){
						case REMAP_TOUCH_ZONE_L:  tz = TZ_BACK_L;  break;
						case REMAP_TOUCH_ZONE_R:  tz = TZ_BACK_R;  break;
						case REMAP_TOUCH_ZONE_TL: tz = TZ_BACK_TL; break;
						case REMAP_TOUCH_ZONE_TR: tz = TZ_BACK_TR; break;
						case REMAP_TOUCH_ZONE_BL: tz = TZ_BACK_BL; break;
						case REMAP_TOUCH_ZONE_BR: tz = TZ_BACK_BR; break;
						case REMAP_TOUCH_CUSTOM:  tz = trigger->param.tPoints; break;
						default: break;
					}
					if (updateStatus(rd.status, reportInZone(&back.report[j], tz)))
						addEmu(&rd);
					else
						remEmu(&rd);
				}
				break;
			case REMAP_TYPE_GYROSCOPE: 
				if (gyroRet != 0) break;
				switch (trigger->action){
					case REMAP_GYRO_UP:  
						if (updateStatus(rd.status, sms.angularVelocity.x > 0)){
							rd.stickposval = clamp(sms.angularVelocity.x * profile.gyro[PROFILE_GYRO_SENSIVITY_Y], -127, 127);
							addEmu(&rd);
						} else {
							remEmu(&rd);
						}
						break;
					case REMAP_GYRO_DOWN:
						if (updateStatus(rd.status, sms.angularVelocity.x < 0)){
							rd.stickposval = clamp(- sms.angularVelocity.x * profile.gyro[PROFILE_GYRO_SENSIVITY_Y], -127, 127);
							addEmu(&rd);
						} else {
							remEmu(&rd);
						}
						break;
					case REMAP_GYRO_LEFT:
						if (updateStatus(rd.status, sms.angularVelocity.y > 0)){
							rd.stickposval = clamp(sms.angularVelocity.y * profile.gyro[PROFILE_GYRO_SENSIVITY_X], -127, 127);
							addEmu(&rd);
						} else {
							remEmu(&rd);
						}
						break;
					case REMAP_GYRO_RIGHT:
						if (updateStatus(rd.status, sms.angularVelocity.y < 0)){
							rd.stickposval = clamp(- sms.angularVelocity.y * profile.gyro[PROFILE_GYRO_SENSIVITY_X], -127, 127);
							addEmu(&rd);
						} else {
							remEmu(&rd);
						}
						break;
					case REMAP_GYRO_ROLL_LEFT:
						if (updateStatus(rd.status, sms.angularVelocity.z < 0)){
							rd.stickposval = clamp(sms.deviceQuat.z * profile.gyro[PROFILE_GYRO_SENSIVITY_Z] * 4, -127, 127);
							addEmu(&rd);
						} else {
							remEmu(&rd);
						}
						break;
					case REMAP_GYRO_ROLL_RIGHT:
						if (updateStatus(rd.status, sms.angularVelocity.z > 0)){
							rd.stickposval = clamp(- sms.deviceQuat.z * profile.gyro[PROFILE_GYRO_SENSIVITY_Z] * 4, -127, 127);
							addEmu(&rd);
						} else {
							remEmu(&rd);
						}
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
	btn_add(&rd.btnsEmu, rd.btnsProp);
	ctrl->buttons = rd.btnsEmu;

	//Restoring propagated values for analog sticks
	rd.analogLeftEmu.left += rd.analogLeftProp.left;
	rd.analogLeftEmu.right += rd.analogLeftProp.right;
	rd.analogLeftEmu.up += rd.analogLeftProp.up;
	rd.analogLeftEmu.down += rd.analogLeftProp.down;
	rd.analogRightEmu.left += rd.analogRightProp.left;
	rd.analogRightEmu.right += rd.analogRightProp.right;
	rd.analogRightEmu.up += rd.analogRightProp.up;
	rd.analogRightEmu.down += rd.analogRightProp.down;

	//Storing remap for analog axises
	ctrl->lx = clamp(127  + rd.analogLeftEmu.right- rd.analogLeftEmu.left, 0, 255);
	ctrl->ly = clamp(127 + rd.analogLeftEmu.down - rd.analogLeftEmu.up, 0, 255);
	ctrl->rx = clamp(127 + rd.analogRightEmu.right - rd.analogRightEmu.left, 0, 255);
	ctrl->ry = clamp(127 + rd.analogRightEmu.down - rd.analogRightEmu.up, 0, 255);

	//Telling that new emulated touch buffer is ready to be taken
	newEmulatedFrontTouchBuffer = true;
	newEmulatedBackTouchBuffer = true;
}

int remap_controls(SceCtrlData *ctrl, int nBufs, int hookId) {	
	int buffIdx = (remappedBuffersIdxs[hookId] + 1) % BUFFERS_NUM;
	
	//Storing copy of latest buffer
	remappedBuffersIdxs[hookId] = buffIdx;
	remappedBuffersSizes[hookId] = min(remappedBuffersSizes[hookId] + 1, BUFFERS_NUM);
	remappedBuffers[hookId][buffIdx] = ctrl[nBufs-1];
	
	//Applying remap to latest buffer
	applyRemap(&remappedBuffers[hookId][buffIdx], &rs[hookId][0]);
	
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
			if (prevEtFront.reports[i].point.x == x && prevEtFront.reports[i].point.y == y)
				return prevEtFront.reports[i].id;
		etFrontIdCounter = (etFrontIdCounter + 1) % 127;
		return etFrontIdCounter;
	} else {
		for (int i = 0; i < prevEtBack.num; i++)
			if (prevEtBack.reports[i].point.x == x && prevEtBack.reports[i].point.y == y)
				return prevEtBack.reports[i].id;
		etBackIdCounter = (etBackIdCounter + 1) % 127;
		return etBackIdCounter;
	}
}

void addVirtualTouches(SceTouchData *pData, EmulatedTouch *et, 
		uint8_t touchPointsMaxNum, int panel){
	int touchIdx = 0;
	while (touchIdx < et->num && pData->reportNum < touchPointsMaxNum){
		uint16_t ax = et->reports[touchIdx].point.x;
		uint16_t ay = et->reports[touchIdx].point.y;
		uint16_t bx = et->reports[touchIdx].swipeEndPoint.x;
		uint16_t by = et->reports[touchIdx].swipeEndPoint.y;
		if (et->reports[touchIdx].isSwipe){//Emulate swipe
			int64_t tick = ksceKernelGetSystemTimeWide();

			//Initialize swipe
			if (et->reports[touchIdx].tick == 0){
				et->reports[touchIdx].id = generateTouchId(ax, ay, panel);
				et->reports[touchIdx].tick = tick;
			}

			//Calculate swipe point pisition for current tick
			pData->report[pData->reportNum].x = clampSmart(
				ax + (bx - ax) * (tick - et->reports[touchIdx].tick) / (profile.touch[PROFILE_TOUCH_SWIPE_DURATION] * 1000), 
				ax, bx);
			pData->report[pData->reportNum].y = clampSmart(
				ay + (by - ay) * (tick - et->reports[touchIdx].tick) / (profile.touch[PROFILE_TOUCH_SWIPE_DURATION] * 1000), 
				ay, by);

			//If swipe point reached it's ending
			if (pData->report[pData->reportNum].x == bx && pData->report[pData->reportNum].y == by)
				et->reports[touchIdx].isSwipeFinished = true;
		} else if (et->reports[touchIdx].isSmartSwipe){ // Emulate smart swipe
			if (et->reports[touchIdx].id < 0)
				et->reports[touchIdx].id = generateTouchId(ax, ay, panel);
			pData->report[pData->reportNum].x = bx;
			pData->report[pData->reportNum].y = by;
		} else { //Emulate touch point
			pData->report[pData->reportNum].x = ax;
			pData->report[pData->reportNum].y = ay;
			et->reports[touchIdx].id = generateTouchId(ax, ay, panel);
		}
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
					TouchPoints2 tz = TZ_FRONT_L;
					switch (profile.remaps[i].trigger.action){
						case REMAP_TOUCH_ZONE_L:  tz = TZ_FRONT_L;  break;
						case REMAP_TOUCH_ZONE_R:  tz = TZ_FRONT_R;  break;
						case REMAP_TOUCH_ZONE_TL: tz = TZ_FRONT_TL; break;
						case REMAP_TOUCH_ZONE_TR: tz = TZ_FRONT_TR; break;
						case REMAP_TOUCH_ZONE_BL: tz = TZ_FRONT_BL; break;
						case REMAP_TOUCH_ZONE_BR: tz = TZ_FRONT_BR; break;
						case REMAP_TOUCH_CUSTOM:  tz = profile.remaps[i].trigger.param.tPoints; break;
						default: break;
					}
					if (reportInZone(&pData->report[j], tz))
						removeReport(pData, j);
					else 
						j++;
				}
			}

		if (!newEmulatedFrontTouchBuffer){//New touchbuffer not ready - using previous one
			addVirtualTouches(pData, &prevEtFront, MULTITOUCH_FRONT_NUM, SCE_TOUCH_PORT_FRONT);
			return;
		}
		
		addVirtualTouches(pData, &etFront, MULTITOUCH_FRONT_NUM, SCE_TOUCH_PORT_FRONT);
		prevEtFront = etFront;
		cleanEmuReports(&etFront);
		newEmulatedFrontTouchBuffer = false;
	} else {
		// Remove non-propagated remapped touches
		for (int i = 0; i < profile.remapsNum; i++)
			if (!profile.remaps[i].propagate && !profile.remaps[i].disabled &&
				profile.remaps[i].trigger.type == REMAP_TYPE_BACK_TOUCH_ZONE){
				int j = 0;
				while (j < pData->reportNum){
					TouchPoints2 tz = TZ_BACK_L;
					switch (profile.remaps[i].trigger.action){
						case REMAP_TOUCH_ZONE_L:  tz = TZ_BACK_L;  break;
						case REMAP_TOUCH_ZONE_R:  tz = TZ_BACK_R;  break;
						case REMAP_TOUCH_ZONE_TL: tz = TZ_BACK_TL; break;
						case REMAP_TOUCH_ZONE_TR: tz = TZ_BACK_TR; break;
						case REMAP_TOUCH_ZONE_BL: tz = TZ_BACK_BL; break;
						case REMAP_TOUCH_ZONE_BR: tz = TZ_BACK_BR; break;
						case REMAP_TOUCH_CUSTOM:  tz = profile.remaps[i].trigger.param.tPoints; break;
						default: break;
					}
					if (reportInZone(&pData->report[j], tz))
						removeReport(pData, j);
					else 
						j++;
				}
			}
			
		if (!newEmulatedBackTouchBuffer){//New touchbuffer not ready - using previous one
			addVirtualTouches(pData, &prevEtBack, MULTITOUCH_BACK_NUM, SCE_TOUCH_PORT_BACK);
			return;
		}
		
		addVirtualTouches(pData, &etBack, MULTITOUCH_BACK_NUM, SCE_TOUCH_PORT_BACK);
		prevEtBack = etBack;
		cleanEmuReports(&etBack);
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

void remap_resetBuffers(){
	for (int i = 0; i < CTRL_HOOKS_NUM; i++){
		remappedBuffersIdxs[i] = remappedBuffersSizes[i] = 0;
		for (int j = 0; j < REMAP_NUM; j++)
			rs[i][j] = RS_NONACTIVE;
	}
	for (int i = 0; i < TOUCH_HOOKS_NUM; i++){
		remappedBuffersFrontIdxs[i] = 
		remappedBuffersBackIdxs[i] = 
		remappedBuffersFrontSizes[i] = 
		remappedBuffersBackSizes[i] = 0;
	}
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

	TZ_FRONT_L = (TouchPoints2){
		(TouchPoint){0, 0}, 
		(TouchPoint){T_FRONT_SIZE.x / 2, T_FRONT_SIZE.y}};
	TZ_FRONT_R = (TouchPoints2){
		(TouchPoint){T_FRONT_SIZE.x / 2, 0}, 
		(TouchPoint){T_FRONT_SIZE.x, T_FRONT_SIZE.y}};
	TZ_FRONT_TL = (TouchPoints2){
		(TouchPoint){0, 0}, 
		(TouchPoint){T_FRONT_SIZE.x / 2, T_FRONT_SIZE.y / 2}};
	TZ_FRONT_TR = (TouchPoints2){
		(TouchPoint){T_FRONT_SIZE.x / 2, 0}, 
		(TouchPoint){T_FRONT_SIZE.x, T_FRONT_SIZE.y / 2}};
	TZ_FRONT_BL = (TouchPoints2){
		(TouchPoint){0, T_FRONT_SIZE.y / 2}, 
		(TouchPoint){T_FRONT_SIZE.x / 2, T_FRONT_SIZE.y}};
	TZ_FRONT_BR = (TouchPoints2){
		(TouchPoint){T_FRONT_SIZE.x / 2, T_FRONT_SIZE.y / 2}, 
		(TouchPoint){T_FRONT_SIZE.x, T_FRONT_SIZE.y}};

	TZ_BACK_L = (TouchPoints2){
		(TouchPoint){0, 0}, 
		(TouchPoint){T_BACK_SIZE.x / 2, T_BACK_SIZE.y}};
	TZ_BACK_R = (TouchPoints2){
		(TouchPoint){T_BACK_SIZE.x / 2, 0}, 
		(TouchPoint){T_BACK_SIZE.x, T_BACK_SIZE.y}};
	TZ_BACK_TL = (TouchPoints2){
		(TouchPoint){0, 0}, 
		(TouchPoint){T_BACK_SIZE.x / 2, T_BACK_SIZE.y / 2}};
	TZ_BACK_TR = (TouchPoints2){
		(TouchPoint){T_BACK_SIZE.x / 2, 0}, 
		(TouchPoint){T_BACK_SIZE.x, T_BACK_SIZE.y / 2}};
	TZ_BACK_BL = (TouchPoints2){
		(TouchPoint){0, T_BACK_SIZE.y / 2}, 
		(TouchPoint){T_BACK_SIZE.x / 2, T_BACK_SIZE.y}};
	TZ_BACK_BR = (TouchPoints2){
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
	for (int i = 0; i < CTRL_HOOKS_NUM; i++) //Allocating mem for stored buffers
		remappedBuffers[i] = &_remappedBuffers[i][0];
	for (int i = 0; i < TOUCH_HOOKS_NUM; i++){
		remappedBuffersFront[i] = (SceTouchData*)&_remappedBuffersFront[i][0];
		remappedBuffersBack[i] = (SceTouchData*)&_remappedBuffersBack[i][0];
	}
	remap_resetBuffers();
	initTouchParams();
}

void remap_destroy(){

}
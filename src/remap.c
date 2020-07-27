#include <vitasdkkern.h>
#include <psp2/motion.h> 
#include <psp2/touch.h>
#include <stdlib.h>

#include "vitasdkext.h"
#include "main.h"
#include "remap.h"
#include "profile.h"
#include "common.h"

#define MULTITOUCH_FRONT_NUM		6
#define MULTITOUCH_REAR_NUM			4

typedef struct EmulatedTouch{
	SceTouchReport reports[MULTITOUCH_FRONT_NUM];
	uint8_t num;
}EmulatedTouch;

//For now let's store them on the stack
static SceCtrlData _remappedBuffers[HOOKS_NUM-5][BUFFERS_NUM];
static SceTouchData _remappedBuffersFront[4][BUFFERS_NUM];
static SceTouchData _remappedBuffersRear[4][BUFFERS_NUM];

//Circular cache to store remapped keys buffers per each ctrs hook
static SceCtrlData *remappedBuffers[HOOKS_NUM-5];
static int remappedBuffersSizes[HOOKS_NUM];
static int remappedBuffersIdxs[HOOKS_NUM];

//Circular cache to store Touch buffers per each touch hook
static SceTouchData *remappedBuffersFront[4];
static int remappedBuffersFrontSizes[4];
static int remappedBuffersFrontIdxs[4];
static SceTouchData *remappedBuffersRear[4];
static int remappedBuffersRearSizes[4];
static int remappedBuffersRearIdxs[4];
uint8_t newEmulatedFrontTouchBuffer = 0;
uint8_t newEmulatedRearTouchBuffer = 0;

static EmulatedTouch etFront, etRear, prevEtFront, prevEtRear;
static uint8_t etFrontIdCounter = 64;
static uint8_t etRearIdCounter = 64;


void storeTouchPoint(EmulatedTouch *et, uint16_t x, uint16_t y){
	for (int i = 0; i < et->num; i++)
		if (et->reports[i].x == x && et->reports[i].y == y)
			return;
	et->reports[et->num].x = x;
	et->reports[et->num].y = y;
	et->num++;
}

//Anything -> Btn, Analog, Touch
void applyRemapRule(uint8_t btn_idx, uint32_t* map, uint32_t* stickpos) {
	if (profile.remap[btn_idx] < PHYS_BUTTONS_NUM) { // -> Btn
		if (!(*map & HW_BUTTONS[profile.remap[btn_idx]])) {
			*map += HW_BUTTONS[profile.remap[btn_idx]];
		}

	} else if (profile.remap[btn_idx] == PHYS_BUTTONS_NUM) { // -> Original
		if (btn_idx < PHYS_BUTTONS_NUM) {
			if (!(*map & HW_BUTTONS[btn_idx])) {
				*map += HW_BUTTONS[btn_idx];
			}
		}										//  -> Analog
	} else if (PHYS_BUTTONS_NUM + 1 < profile.remap[btn_idx] && profile.remap[btn_idx] < PHYS_BUTTONS_NUM + 10) { 
		stickpos[profile.remap[btn_idx] - (PHYS_BUTTONS_NUM + 2)] += 127;
	} else if (profile.remap[btn_idx] < PHYS_BUTTONS_NUM + 24){	// -> Touch
		if (profile.remap[btn_idx] < PHYS_BUTTONS_NUM + 14){		//Front touch default
			if (etFront.num == MULTITOUCH_FRONT_NUM) return;
			storeTouchPoint(&etFront,
				profile_def.touch[(profile.remap[btn_idx] - (PHYS_BUTTONS_NUM + 10)) * 2],
				profile_def.touch[(profile.remap[btn_idx] - (PHYS_BUTTONS_NUM + 10)) * 2 + 1]);
		} else if (profile.remap[btn_idx] < PHYS_BUTTONS_NUM + 18){	//Front touch custom
			if (etFront.num == MULTITOUCH_FRONT_NUM) return;
			storeTouchPoint(&etFront,
				profile.touch[(profile.remap[btn_idx] - (PHYS_BUTTONS_NUM + 14)) * 2],
				profile.touch[(profile.remap[btn_idx] - (PHYS_BUTTONS_NUM + 14)) * 2 + 1]);
		} else if (profile.remap[btn_idx] < PHYS_BUTTONS_NUM + 22){	//Rear  touch default
			if (etRear.num == MULTITOUCH_REAR_NUM) return;
			storeTouchPoint(&etRear,
				profile_def.touch[8 + (profile.remap[btn_idx] - (PHYS_BUTTONS_NUM + 18)) * 2],
				profile_def.touch[8 + (profile.remap[btn_idx] - (PHYS_BUTTONS_NUM + 18)) * 2 + 1]);
		} else if (profile.remap[btn_idx] < PHYS_BUTTONS_NUM + 26){	//Rear touch custom
			if (etRear.num == MULTITOUCH_REAR_NUM) return;
			storeTouchPoint(&etRear,
				profile.touch[8 + (profile.remap[btn_idx] - (PHYS_BUTTONS_NUM + 22)) * 2],
				profile.touch[8 + (profile.remap[btn_idx] - (PHYS_BUTTONS_NUM + 22)) * 2 + 1]);
		}
	}
}

//Used to handle analog->analog mapping
void applyRemapRuleForAnalog(uint8_t btn_idx, uint32_t* map, uint32_t* stickpos, uint8_t stickposval) {
	if (PHYS_BUTTONS_NUM + 1 < profile.remap[btn_idx] && profile.remap[btn_idx] < PHYS_BUTTONS_NUM + 10
		&& !profile.analog[4 + (int) ((btn_idx - PHYS_BUTTONS_NUM - 8) / 2)]) {
		// Analog -> Analog [ANALOG MODE]
		stickpos[profile.remap[btn_idx] - (PHYS_BUTTONS_NUM + 2)] += 127 - stickposval;
	} else {
		// Analog -> Analog [DIGITAL MODE] and Analog -> Button
		applyRemapRule(btn_idx, map, stickpos);
	}
}

//Used to handle analog->gyro mapping
void applyRemapRuleForGyro(uint8_t btn_idx, uint32_t* map, uint32_t* stickpos, float gyroval){
	if (PHYS_BUTTONS_NUM + 1 < profile.remap[btn_idx] && profile.remap[btn_idx] < PHYS_BUTTONS_NUM + 10) {
		// Gyro -> Analog remap
		stickpos[profile.remap[btn_idx] - (PHYS_BUTTONS_NUM + 2)] = stickpos[profile.remap[btn_idx] - (PHYS_BUTTONS_NUM + 2)] + clamp(gyroval, -127, 127);
	} else {
		// Gyro -> Btn remap
		if ((((btn_idx == PHYS_BUTTONS_NUM + 16 || btn_idx == PHYS_BUTTONS_NUM + 17)) && gyroval > profile.gyro[3] * 10) ||
			(((btn_idx == PHYS_BUTTONS_NUM + 18 || btn_idx == PHYS_BUTTONS_NUM + 19)) && gyroval > profile.gyro[4] * 10) ||
			(((btn_idx == PHYS_BUTTONS_NUM + 20 || btn_idx == PHYS_BUTTONS_NUM + 21)) && gyroval > profile.gyro[5] * 10))
			applyRemapRule(btn_idx, map, stickpos);
	}
}


void applyRemap(SceCtrlData *ctrl) {
	// Gathering real touch data
	SceTouchData front, rear;
	internal_touch_call = 1;
	ksceTouchPeek(SCE_TOUCH_PORT_FRONT, &front, 1);
	ksceTouchPeek(SCE_TOUCH_PORT_BACK, &rear, 1);
	internal_touch_call = 0;
	
	// Gathering gyro data	
	//SceMotionState motionstate;
	//ToDo replace with something
    //sceMotionGetState(&motionstate);
	
	
	// Applying remap rules for physical buttons
	int i;
	uint32_t new_map = 0;
	uint32_t stickpos[8] = { };
	for (i=0;i<PHYS_BUTTONS_NUM;i++) {
		if (ctrl->buttons & HW_BUTTONS[i]) applyRemapRule(i, &new_map, stickpos);
	}
	
	// Applying remap rules for front touches
	int left = TOUCH_SIZE[0]/2; int top = TOUCH_SIZE[1]/2;
	for (i=0;i<front.reportNum;i++) {
		if (front.report[i].x > left && front.report[i].y > top)       // Bot Right
			applyRemapRule(PHYS_BUTTONS_NUM + 3, &new_map, stickpos);
		else if (front.report[i].x <= left && front.report[i].y > top) // Bot Left
			applyRemapRule(PHYS_BUTTONS_NUM + 2, &new_map, stickpos);
		else if (front.report[i].x > left && front.report[i].y <= top) // Top Right
			applyRemapRule(PHYS_BUTTONS_NUM + 1, &new_map, stickpos);
		else if (front.report[i].x <= left && front.report[i].y <= top)// Top Left
			applyRemapRule(PHYS_BUTTONS_NUM, &new_map, stickpos);
	}
	
	// Applying remap rules for rear touches
	left = TOUCH_SIZE[2]/2; top = TOUCH_SIZE[3]/2;
	for (i=0;i<rear.reportNum;i++) {
		if (rear.report[i].x > left && rear.report[i].y > top)        // Bot Right
			applyRemapRule(PHYS_BUTTONS_NUM + 7, &new_map, stickpos);
		else if (rear.report[i].x <= left && rear.report[i].y > top)  // Bot Left
			applyRemapRule(PHYS_BUTTONS_NUM + 6, &new_map, stickpos);
		else if (rear.report[i].x > left && rear.report[i].y <= top)  // Top Right
			applyRemapRule(PHYS_BUTTONS_NUM + 5, &new_map, stickpos);
		else if (rear.report[i].x <= left && rear.report[i].y <= top) // Top Left
			applyRemapRule(PHYS_BUTTONS_NUM + 4, &new_map, stickpos);
	}

	// Applying remap rules for left analog
	if (ctrl->lx < 127 - profile.analog[0])			// Left
		applyRemapRuleForAnalog(PHYS_BUTTONS_NUM + 8, &new_map, stickpos, ctrl->lx);
	else if (ctrl->lx > 127 + profile.analog[0])		// Right
		applyRemapRuleForAnalog(PHYS_BUTTONS_NUM + 9, &new_map, stickpos, 255 - ctrl->lx);
	if (ctrl->ly < 127 - profile.analog[1])			// Up
		applyRemapRuleForAnalog(PHYS_BUTTONS_NUM + 10, &new_map, stickpos, ctrl->ly);
	else if (ctrl->ly > 127 + profile.analog[1])		// Down
		applyRemapRuleForAnalog(PHYS_BUTTONS_NUM + 11, &new_map, stickpos, 255 - ctrl->ly);
	
	// Applying remap rules for right analog
	if (ctrl->rx < 127 - profile.analog[2])	 		// Left
		applyRemapRuleForAnalog(PHYS_BUTTONS_NUM + 12, &new_map, stickpos, ctrl->rx);
	else if (ctrl->rx > 127 + profile.analog[2])		// Right
		applyRemapRuleForAnalog(PHYS_BUTTONS_NUM + 13, &new_map, stickpos, 255 - ctrl->rx);
	if (ctrl->ry < 127 - profile.analog[3])			// Up
		applyRemapRuleForAnalog(PHYS_BUTTONS_NUM + 14, &new_map, stickpos, ctrl->ry);
	else if (ctrl->ry > 127 + profile.analog[3])		// Down
		applyRemapRuleForAnalog(PHYS_BUTTONS_NUM + 15, &new_map, stickpos, 255 - ctrl->ry);
	
	/*
	// Applying remap for gyro
	if (profile.gyro[7] == 0) {
		if (motionstate.angularVelocity.y > 0)
			applyRemapRuleForGyro(PHYS_BUTTONS_NUM + 16, &new_map, stickpos,
				motionstate.angularVelocity.y * profile.gyro[0]);
		if (motionstate.angularVelocity.y < 0)
			applyRemapRuleForGyro(PHYS_BUTTONS_NUM + 17, &new_map, stickpos,
				-motionstate.angularVelocity.y * profile.gyro[0]);
		if (motionstate.angularVelocity.x > 0)
			applyRemapRuleForGyro(PHYS_BUTTONS_NUM + 18, &new_map, stickpos,
				motionstate.angularVelocity.x * profile.gyro[1]);
		if (motionstate.angularVelocity.x < 0)
			applyRemapRuleForGyro(PHYS_BUTTONS_NUM + 19, &new_map, stickpos,
				-motionstate.angularVelocity.x * profile.gyro[1]);
		if (motionstate.angularVelocity.z > 0)
			applyRemapRuleForGyro(PHYS_BUTTONS_NUM + 20, &new_map, stickpos,
				motionstate.angularVelocity.z * profile.gyro[2]);
		if (motionstate.angularVelocity.z < 0)
			applyRemapRuleForGyro(PHYS_BUTTONS_NUM + 21, &new_map, stickpos,
				-motionstate.angularVelocity.z * profile.gyro[2]);
	}
	else {
		// Applying remap for gyro wheel mode
		if (motionstate.deviceQuat.y < 0)
			applyRemapRuleForGyro(PHYS_BUTTONS_NUM + 16, &new_map, stickpos,
				motionstate.deviceQuat.y * profile.gyro[0] * 4);
		if (motionstate.deviceQuat.y > 0)
			applyRemapRuleForGyro(PHYS_BUTTONS_NUM + 17, &new_map, stickpos,
				-motionstate.deviceQuat.y * profile.gyro[0] * 4);
		if (motionstate.deviceQuat.x < 0)
			applyRemapRuleForGyro(PHYS_BUTTONS_NUM + 18, &new_map, stickpos,
				motionstate.deviceQuat.x * profile.gyro[1] * 4);
		if (motionstate.deviceQuat.x > 0)
			applyRemapRuleForGyro(PHYS_BUTTONS_NUM + 19, &new_map, stickpos,
				-motionstate.deviceQuat.x * profile.gyro[1] * 4);
		if (motionstate.deviceQuat.z < 0)
			applyRemapRuleForGyro(PHYS_BUTTONS_NUM + 20, &new_map, stickpos,
				motionstate.deviceQuat.z * profile.gyro[2] * 4);
		if (motionstate.deviceQuat.z > 0)
			applyRemapRuleForGyro(PHYS_BUTTONS_NUM + 21, &new_map, stickpos,
			-motionstate.deviceQuat.z * profile.gyro[2] * 4);
	}*/

	// Nulling analogs if they're remapped		
	if ((ctrl->lx < 127 && profile.remap[PHYS_BUTTONS_NUM+8] != PHYS_BUTTONS_NUM) ||
		(ctrl->lx > 127 && profile.remap[PHYS_BUTTONS_NUM+9] != PHYS_BUTTONS_NUM))
		ctrl->lx = 127;
	if ((ctrl->ly < 127 && profile.remap[PHYS_BUTTONS_NUM+10] != PHYS_BUTTONS_NUM) ||
		(ctrl->ly > 127 && profile.remap[PHYS_BUTTONS_NUM+11] != PHYS_BUTTONS_NUM))
		ctrl->ly = 127;
	if ((ctrl->rx < 127 && profile.remap[PHYS_BUTTONS_NUM+12] != PHYS_BUTTONS_NUM) ||
		(ctrl->rx > 127 && profile.remap[PHYS_BUTTONS_NUM+13] != PHYS_BUTTONS_NUM))
		ctrl->rx = 127;
	if ((ctrl->ry < 127 && profile.remap[PHYS_BUTTONS_NUM+14] != PHYS_BUTTONS_NUM) ||
		(ctrl->ry > 127 && profile.remap[PHYS_BUTTONS_NUM+15] != PHYS_BUTTONS_NUM))
		ctrl->ry = 127;	
	
	// Remove minimal drift if digital remap for stick directions is used
	if (stickpos[0] || stickpos[1] || stickpos[2] || stickpos[3]){
		if (abs(ctrl->lx - 127) < profile.analog[0]) 
			ctrl->lx = 127;
		if (abs(ctrl->ly - 127) < profile.analog[1]) 
			ctrl->ly = 127;
	}
	if (stickpos[4] || stickpos[5] || stickpos[6] || stickpos[7]){
		if (abs(ctrl->rx - 127) < profile.analog[2]) 
			ctrl->rx = 127;
		if (abs(ctrl->ry - 127) < profile.analog[3]) 
			ctrl->ry = 127;
	}
	
	//Storing remap for analog axises
	if (stickpos[0] || stickpos[1])
		ctrl->lx = clamp(ctrl->lx - stickpos[0] + stickpos[1], 0, 255);
	if (stickpos[2] || stickpos[3])
		ctrl->ly = clamp(ctrl->ly - stickpos[2] + stickpos[3], 0, 255);
	if (stickpos[4] || stickpos[5])
		ctrl->rx = clamp(ctrl->rx - stickpos[4] + stickpos[5], 0, 255);
	if (stickpos[6] || stickpos[7])
		ctrl->ry = clamp(ctrl->ry - stickpos[6] + stickpos[7], 0, 255);
	
	//Storing remap for HW buttons
	ctrl->buttons = new_map;
	
	//Telling that new emulated touch buffer is ready to be takn
	newEmulatedFrontTouchBuffer = 1;
	newEmulatedRearTouchBuffer = 1;
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
		for (int i = 0; i < prevEtRear.num; i++)
			if (prevEtRear.reports[i].x == x && prevEtRear.reports[i].y == y)
				return prevEtRear.reports[i].id;
		etRearIdCounter = (etRearIdCounter + 1) % 127;
		return etRearIdCounter;
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
		if ((profile.touch[16] == 1 && //Disable if remapped
				(profile.remap[PHYS_BUTTONS_NUM] != PHYS_BUTTONS_NUM ||
				 profile.remap[PHYS_BUTTONS_NUM+1] != PHYS_BUTTONS_NUM ||
				 profile.remap[PHYS_BUTTONS_NUM+2] != PHYS_BUTTONS_NUM ||
				 profile.remap[PHYS_BUTTONS_NUM+3] != PHYS_BUTTONS_NUM)) ||
				 profile.remap[PHYS_BUTTONS_NUM]   == PHYS_BUTTONS_NUM+1 ||
				 profile.remap[PHYS_BUTTONS_NUM+1] == PHYS_BUTTONS_NUM+1 ||
				 profile.remap[PHYS_BUTTONS_NUM+2] == PHYS_BUTTONS_NUM+1 ||
				 profile.remap[PHYS_BUTTONS_NUM+3] == PHYS_BUTTONS_NUM+1)
			pData->reportNum = 0; //Disable pad
			
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
		if ((profile.touch[17] == 1 &&//Disable if remapped
				(profile.remap[PHYS_BUTTONS_NUM+4] != PHYS_BUTTONS_NUM ||
				 profile.remap[PHYS_BUTTONS_NUM+5] != PHYS_BUTTONS_NUM ||
				 profile.remap[PHYS_BUTTONS_NUM+6] != PHYS_BUTTONS_NUM ||
				 profile.remap[PHYS_BUTTONS_NUM+7] != PHYS_BUTTONS_NUM)) ||
				 profile.remap[PHYS_BUTTONS_NUM+4] == PHYS_BUTTONS_NUM+1 ||
				 profile.remap[PHYS_BUTTONS_NUM+5] == PHYS_BUTTONS_NUM+1 ||
				 profile.remap[PHYS_BUTTONS_NUM+6] == PHYS_BUTTONS_NUM+1 ||
				 profile.remap[PHYS_BUTTONS_NUM+7] == PHYS_BUTTONS_NUM+1)
			pData->reportNum = 0; //Disable pad
			
		if (!newEmulatedRearTouchBuffer){//New touchbuffer not ready - using previous one
			addVirtualTouches(pData, &prevEtRear, 
				MULTITOUCH_REAR_NUM, SCE_TOUCH_PORT_BACK);
			return;
		}
		
		addVirtualTouches(pData, &etRear, 
			MULTITOUCH_REAR_NUM, SCE_TOUCH_PORT_BACK);
		prevEtRear = etRear;
		etRear.num = 0;
		newEmulatedRearTouchBuffer = 0;
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
		int buffIdx = (remappedBuffersRearIdxs[hookId] + 1) % BUFFERS_NUM;

		//Storing copy of latest buffer
		remappedBuffersRearIdxs[hookId] = buffIdx;
		remappedBuffersRearSizes[hookId] = min(remappedBuffersRearSizes[hookId] + 1, BUFFERS_NUM);
		remappedBuffersRear[hookId][buffIdx] = pData[nBufs-1];
		
		//Updating latest buffer with simulated touches
		updateTouchInfo(port, &remappedBuffersRear[hookId][buffIdx]);
		
		//Limit returned buufers num with what we have stored
		nBufs = min(nBufs, remappedBuffersRearSizes[hookId]);
		
		//Restoring stored buffers
		for (int i = 0; i < nBufs; i++)
			pData[i] = remappedBuffersRear[hookId]
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
	remappedBuffersRearIdxs[hookId] = 0;
	remappedBuffersFrontSizes[hookId] = 0;
	remappedBuffersRearSizes[hookId] = 0;
}

void remap_init(){
	for (int i = 0; i < HOOKS_NUM-5; i++) //Allocating mem for stored buffers
		remappedBuffers[i] = &_remappedBuffers[i][0];

	for (int i = 0; i < 4; i++){
		remappedBuffersFront[i] = (SceTouchData*)&_remappedBuffersFront[i][0];
		remappedBuffersRear[i] = (SceTouchData*)&_remappedBuffersRear[i][0];
	}
}

void remap_destroy(){

}
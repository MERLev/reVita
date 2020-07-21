#include <vitasdk.h>
#include <psp2/motion.h> 
#include <stdlib.h>

#include "main.h"
#include "profile.h"
#include "ui.h"
#include "common.h"

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

typedef struct EmulatedTouch{
	SceTouchReport reports[MULTITOUCH_FRONT_NUM];
	uint8_t num;
}EmulatedTouch;
EmulatedTouch etFront, etRear, prevEtFront, prevEtRear;
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
	if (btn_mask[btn_idx] < PHYS_BUTTONS_NUM) { // -> Btn
		if (!(*map & btns[btn_mask[btn_idx]])) {
			*map += btns[btn_mask[btn_idx]];
		}

	} else if (btn_mask[btn_idx] == PHYS_BUTTONS_NUM) { // -> Original
		if (btn_idx < PHYS_BUTTONS_NUM) {
			if (!(*map & btns[btn_idx])) {
				*map += btns[btn_idx];
			}
		}										//  -> Analog
	} else if (PHYS_BUTTONS_NUM + 1 < btn_mask[btn_idx] && btn_mask[btn_idx] < PHYS_BUTTONS_NUM + 10) { 
		stickpos[btn_mask[btn_idx] - (PHYS_BUTTONS_NUM + 2)] += 127;
	} else if (btn_mask[btn_idx] < PHYS_BUTTONS_NUM + 24){	// -> Touch
		if (btn_mask[btn_idx] < PHYS_BUTTONS_NUM + 14){		//Front touch default
			if (etFront.num == MULTITOUCH_FRONT_NUM) return;
			storeTouchPoint(&etFront,
				TOUCH_POINTS_DEF[(btn_mask[btn_idx] - (PHYS_BUTTONS_NUM + 10)) * 2],
				TOUCH_POINTS_DEF[(btn_mask[btn_idx] - (PHYS_BUTTONS_NUM + 10)) * 2 + 1]);
		} else if (btn_mask[btn_idx] < PHYS_BUTTONS_NUM + 18){	//Front touch custom
			if (etFront.num == MULTITOUCH_FRONT_NUM) return;
			storeTouchPoint(&etFront,
				touch_options[(btn_mask[btn_idx] - (PHYS_BUTTONS_NUM + 14)) * 2],
				touch_options[(btn_mask[btn_idx] - (PHYS_BUTTONS_NUM + 14)) * 2 + 1]);
		} else if (btn_mask[btn_idx] < PHYS_BUTTONS_NUM + 22){	//Rear  touch default
			if (etRear.num == MULTITOUCH_REAR_NUM) return;
			storeTouchPoint(&etRear,
				TOUCH_POINTS_DEF[8 + (btn_mask[btn_idx] - (PHYS_BUTTONS_NUM + 18)) * 2],
				TOUCH_POINTS_DEF[8 + (btn_mask[btn_idx] - (PHYS_BUTTONS_NUM + 18)) * 2 + 1]);
		} else if (btn_mask[btn_idx] < PHYS_BUTTONS_NUM + 26){	//Rear touch custom
			if (etRear.num == MULTITOUCH_REAR_NUM) return;
			storeTouchPoint(&etRear,
				touch_options[8 + (btn_mask[btn_idx] - (PHYS_BUTTONS_NUM + 22)) * 2],
				touch_options[8 + (btn_mask[btn_idx] - (PHYS_BUTTONS_NUM + 22)) * 2 + 1]);
		}
	}
}

//Used to handle analog->analog mapping
void applyRemapRuleForAnalog(uint8_t btn_idx, uint32_t* map, uint32_t* stickpos, uint8_t stickposval) {
	if (PHYS_BUTTONS_NUM + 1 < btn_mask[btn_idx] && btn_mask[btn_idx] < PHYS_BUTTONS_NUM + 10
		&& !analogs_options[4 + (int) ((btn_idx - PHYS_BUTTONS_NUM - 8) / 2)]) {
		// Analog -> Analog [ANALOG MODE]
		stickpos[btn_mask[btn_idx] - (PHYS_BUTTONS_NUM + 2)] += 127 - stickposval;
	} else {
		// Analog -> Analog [DIGITAL MODE] and Analog -> Button
		applyRemapRule(btn_idx, map, stickpos);
	}
}

//Used to handle analog->gyro mapping
void applyRemapRuleForGyro(uint8_t btn_idx, uint32_t* map, uint32_t* stickpos, float gyroval){
	if (PHYS_BUTTONS_NUM + 1 < btn_mask[btn_idx] && btn_mask[btn_idx] < PHYS_BUTTONS_NUM + 10) {
		// Gyro -> Analog remap
		stickpos[btn_mask[btn_idx] - (PHYS_BUTTONS_NUM + 2)] = stickpos[btn_mask[btn_idx] - (PHYS_BUTTONS_NUM + 2)] + clamp(gyroval, -127, 127);
	} else {
		// Gyro -> Btn remap
		if ((((btn_idx == PHYS_BUTTONS_NUM + 16 || btn_idx == PHYS_BUTTONS_NUM + 17)) && gyroval > gyro_options[3] * 10) ||
			(((btn_idx == PHYS_BUTTONS_NUM + 18 || btn_idx == PHYS_BUTTONS_NUM + 19)) && gyroval > gyro_options[4] * 10) ||
			(((btn_idx == PHYS_BUTTONS_NUM + 20 || btn_idx == PHYS_BUTTONS_NUM + 21)) && gyroval > gyro_options[5] * 10))
			applyRemapRule(btn_idx, map, stickpos);
	}
}

void applyRemap(SceCtrlData *ctrl) {
	// Gathering real touch data
	SceTouchData front, rear;
	internal_touch_call = 1;
	sceTouchPeek(SCE_TOUCH_PORT_FRONT, &front, 1);
	sceTouchPeek(SCE_TOUCH_PORT_BACK, &rear, 1);
	internal_touch_call = 0;
	
	// Gathering gyro data	
	SceMotionState motionstate;
    sceMotionGetState(&motionstate);
	
	// Applying remap rules for physical buttons
	int i;
	uint32_t new_map = 0;
	uint32_t stickpos[8] = { };
	for (i=0;i<PHYS_BUTTONS_NUM;i++) {
		if (ctrl->buttons & btns[i]) applyRemapRule(i, &new_map, stickpos);
	}
	
	// Applying remap rules for front virtual buttons
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
	
	// Applying remap rules for rear virtual buttons
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
	if (ctrl->lx < 127 - analogs_options[0])			// Left
		applyRemapRuleForAnalog(PHYS_BUTTONS_NUM + 8, &new_map, stickpos, ctrl->lx);
	else if (ctrl->lx > 127 + analogs_options[0])		// Right
		applyRemapRuleForAnalog(PHYS_BUTTONS_NUM + 9, &new_map, stickpos, 255 - ctrl->lx);
	if (ctrl->ly < 127 - analogs_options[1])			// Up
		applyRemapRuleForAnalog(PHYS_BUTTONS_NUM + 10, &new_map, stickpos, ctrl->ly);
	else if (ctrl->ly > 127 + analogs_options[1])		// Down
		applyRemapRuleForAnalog(PHYS_BUTTONS_NUM + 11, &new_map, stickpos, 255 - ctrl->ly);
	
	// Applying remap rules for right analog
	if (ctrl->rx < 127 - analogs_options[2])	 		// Left
		applyRemapRuleForAnalog(PHYS_BUTTONS_NUM + 12, &new_map, stickpos, ctrl->rx);
	else if (ctrl->rx > 127 + analogs_options[2])		// Right
		applyRemapRuleForAnalog(PHYS_BUTTONS_NUM + 13, &new_map, stickpos, 255 - ctrl->rx);
	if (ctrl->ry < 127 - analogs_options[3])			// Up
		applyRemapRuleForAnalog(PHYS_BUTTONS_NUM + 14, &new_map, stickpos, ctrl->ry);
	else if (ctrl->ry > 127 + analogs_options[3])		// Down
		applyRemapRuleForAnalog(PHYS_BUTTONS_NUM + 15, &new_map, stickpos, 255 - ctrl->ry);
	
	// Applying remap for gyro
	if (gyro_options[7] == 0) {
		if (motionstate.angularVelocity.y > 0)
			applyRemapRuleForGyro(PHYS_BUTTONS_NUM + 16, &new_map, stickpos,
				motionstate.angularVelocity.y * gyro_options[0]);
		if (motionstate.angularVelocity.y < 0)
			applyRemapRuleForGyro(PHYS_BUTTONS_NUM + 17, &new_map, stickpos,
				-motionstate.angularVelocity.y * gyro_options[0]);
		if (motionstate.angularVelocity.x > 0)
			applyRemapRuleForGyro(PHYS_BUTTONS_NUM + 18, &new_map, stickpos,
				motionstate.angularVelocity.x * gyro_options[1]);
		if (motionstate.angularVelocity.x < 0)
			applyRemapRuleForGyro(PHYS_BUTTONS_NUM + 19, &new_map, stickpos,
				-motionstate.angularVelocity.x * gyro_options[1]);
		if (motionstate.angularVelocity.z > 0)
			applyRemapRuleForGyro(PHYS_BUTTONS_NUM + 20, &new_map, stickpos,
				motionstate.angularVelocity.z * gyro_options[2]);
		if (motionstate.angularVelocity.z < 0)
			applyRemapRuleForGyro(PHYS_BUTTONS_NUM + 21, &new_map, stickpos,
				-motionstate.angularVelocity.z * gyro_options[2]);
	}
	else {
		// Applying remap for gyro wheel mode
		if (motionstate.deviceQuat.y < 0)
			applyRemapRuleForGyro(PHYS_BUTTONS_NUM + 16, &new_map, stickpos,
				motionstate.deviceQuat.y * gyro_options[0] * 4);
		if (motionstate.deviceQuat.y > 0)
			applyRemapRuleForGyro(PHYS_BUTTONS_NUM + 17, &new_map, stickpos,
				-motionstate.deviceQuat.y * gyro_options[0] * 4);
		if (motionstate.deviceQuat.x < 0)
			applyRemapRuleForGyro(PHYS_BUTTONS_NUM + 18, &new_map, stickpos,
				motionstate.deviceQuat.x * gyro_options[1] * 4);
		if (motionstate.deviceQuat.x > 0)
			applyRemapRuleForGyro(PHYS_BUTTONS_NUM + 19, &new_map, stickpos,
				-motionstate.deviceQuat.x * gyro_options[1] * 4);
		if (motionstate.deviceQuat.z < 0)
			applyRemapRuleForGyro(PHYS_BUTTONS_NUM + 20, &new_map, stickpos,
				motionstate.deviceQuat.z * gyro_options[2] * 4);
		if (motionstate.deviceQuat.z > 0)
			applyRemapRuleForGyro(PHYS_BUTTONS_NUM + 21, &new_map, stickpos,
			-motionstate.deviceQuat.z * gyro_options[2] * 4);
	}

	// Nulling analogs if they're remapped		
	if ((ctrl->lx < 127 && btn_mask[PHYS_BUTTONS_NUM+8] != PHYS_BUTTONS_NUM) ||
		(ctrl->lx > 127 && btn_mask[PHYS_BUTTONS_NUM+9] != PHYS_BUTTONS_NUM))
		ctrl->lx = 127;
	if ((ctrl->ly < 127 && btn_mask[PHYS_BUTTONS_NUM+10] != PHYS_BUTTONS_NUM) ||
		(ctrl->ly > 127 && btn_mask[PHYS_BUTTONS_NUM+11] != PHYS_BUTTONS_NUM))
		ctrl->ly = 127;
	if ((ctrl->rx < 127 && btn_mask[PHYS_BUTTONS_NUM+12] != PHYS_BUTTONS_NUM) ||
		(ctrl->rx > 127 && btn_mask[PHYS_BUTTONS_NUM+13] != PHYS_BUTTONS_NUM))
		ctrl->rx = 127;
	if ((ctrl->ry < 127 && btn_mask[PHYS_BUTTONS_NUM+14] != PHYS_BUTTONS_NUM) ||
		(ctrl->ry > 127 && btn_mask[PHYS_BUTTONS_NUM+15] != PHYS_BUTTONS_NUM))
		ctrl->ry = 127;	
	
	// Remove minimal drift if digital remap for stick directions is used
	if (stickpos[0] || stickpos[1] || stickpos[2] || stickpos[3]){
		if (abs(ctrl->lx - 127) < analogs_options[0]) 
			ctrl->lx = 127;
		if (abs(ctrl->ly - 127) < analogs_options[1]) 
			ctrl->ly = 127;
	}
	if (stickpos[4] || stickpos[5] || stickpos[6] || stickpos[7]){
		if (abs(ctrl->rx - 127) < analogs_options[2]) 
			ctrl->rx = 127;
		if (abs(ctrl->ry - 127) < analogs_options[3]) 
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
		if ((touch_options[16] == 1 && //Disable if remapped
				(btn_mask[PHYS_BUTTONS_NUM] != PHYS_BUTTONS_NUM ||
				 btn_mask[PHYS_BUTTONS_NUM+1] != PHYS_BUTTONS_NUM ||
				 btn_mask[PHYS_BUTTONS_NUM+2] != PHYS_BUTTONS_NUM ||
				 btn_mask[PHYS_BUTTONS_NUM+3] != PHYS_BUTTONS_NUM)) ||
				 btn_mask[PHYS_BUTTONS_NUM]   == PHYS_BUTTONS_NUM+1 ||
				 btn_mask[PHYS_BUTTONS_NUM+1] == PHYS_BUTTONS_NUM+1 ||
				 btn_mask[PHYS_BUTTONS_NUM+2] == PHYS_BUTTONS_NUM+1 ||
				 btn_mask[PHYS_BUTTONS_NUM+3] == PHYS_BUTTONS_NUM+1)
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
		if ((touch_options[17] == 1 &&//Disable if remapped
				(btn_mask[PHYS_BUTTONS_NUM+4] != PHYS_BUTTONS_NUM ||
				 btn_mask[PHYS_BUTTONS_NUM+5] != PHYS_BUTTONS_NUM ||
				 btn_mask[PHYS_BUTTONS_NUM+6] != PHYS_BUTTONS_NUM ||
				 btn_mask[PHYS_BUTTONS_NUM+7] != PHYS_BUTTONS_NUM)) ||
				 btn_mask[PHYS_BUTTONS_NUM+4] == PHYS_BUTTONS_NUM+1 ||
				 btn_mask[PHYS_BUTTONS_NUM+5] == PHYS_BUTTONS_NUM+1 ||
				 btn_mask[PHYS_BUTTONS_NUM+6] == PHYS_BUTTONS_NUM+1 ||
				 btn_mask[PHYS_BUTTONS_NUM+7] == PHYS_BUTTONS_NUM+1)
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

int remap(SceCtrlData *ctrl, int count, int hookId, int logic) {
	if (count < 1)
		return count;	//Nothing to do here
	
	//Invert for negative logic
	if (logic == NEGATIVE)
		ctrl[count - 1].buttons = 0xFFFFFFFF - ctrl[count - 1].buttons;
	
	//Reset wheel gyro buttons pressed
	if (gyro_options[7] == 1 &&
			(ctrl[count - 1].buttons & btns[gyro_options[8]]) 
				&& (ctrl[count - 1].buttons & btns[gyro_options[9]])) {
		sceMotionReset();		
	}
	
	if (!show_menu 
			&& (ctrl[count - 1].buttons & SCE_CTRL_START) 
			&& (ctrl[count - 1].buttons & SCE_CTRL_TRIANGLE)) {
		loadGlobalConfig();
		saveGameConfig();
	}
	
	//Checking for menu triggering
	if (used_funcs[16] && !show_menu 
			&& (ctrl[count - 1].buttons & btns[settings_options[0]]) 
			&& (ctrl[count - 1].buttons & btns[settings_options[1]])) {
		ui_show();
		//Clear buffers;
		remappedBuffersIdxs[hookId] = 0;
		remappedBuffersSizes[hookId] = 0;
	}
	if (show_menu){
		configInputHandler(&ctrl[count - 1]);
		for (int i = 0; i < count; i++)
			ctrl[i].buttons = (logic == POSITIVE) ? 0 : 0xFFFFFFFF;
		return count;
	}
	
	int buffIdx = (remappedBuffersIdxs[hookId] + 1) % BUFFERS_NUM;
	
	//Storing copy of latest buffer
	remappedBuffersIdxs[hookId] = buffIdx;
	remappedBuffersSizes[hookId] = min(remappedBuffersSizes[hookId] + 1, BUFFERS_NUM);
	remappedBuffers[hookId][buffIdx] = ctrl[count-1];
	
	//Applying remap to latest buffer
	applyRemap(&remappedBuffers[hookId][buffIdx]);
	
	//Invert for negative logic
	if (logic == NEGATIVE)
		remappedBuffers[hookId][buffIdx].buttons = 
			0xFFFFFFFF - remappedBuffers[hookId][buffIdx].buttons;
	
	//Limit returned buffers with amount we have cached
	count = min(count, remappedBuffersSizes[hookId]);
	
	//Restoring stored buffers
	for (int i = 0; i < count; i++)
		ctrl[i] = remappedBuffers[hookId]
			[(BUFFERS_NUM + buffIdx - count + i + 1) % BUFFERS_NUM];
	return count;
}

void swapTriggersBumpers(SceCtrlData *ctrl){
	uint32_t b = 0;
	for (int j = 0; j < PHYS_BUTTONS_NUM; j++)
		if (ctrl->buttons & btns[j]){
			if (btns[j] == SCE_CTRL_LTRIGGER) b+= SCE_CTRL_L1;
			else if (btns[j] == SCE_CTRL_L1) b+= SCE_CTRL_LTRIGGER;
			else if (btns[j] == SCE_CTRL_RTRIGGER) b+= SCE_CTRL_R1;
			else if (btns[j] == SCE_CTRL_R1) b+= SCE_CTRL_RTRIGGER;
			else b += btns[j];
	}
	ctrl->buttons = b;
}

//Used to enable R1/R3/L1/L3
void patchToExt(SceCtrlData *ctrl){
	if (!controller_options[0] || show_menu)
		return;
	SceCtrlData pstv_fakepad;
	internal_ext_call = 1;
	int ret = sceCtrlPeekBufferPositiveExt2(controller_options[1], &pstv_fakepad, 1);
	internal_ext_call = 0;
	if (ret > 0){
		ctrl->buttons = pstv_fakepad.buttons;
		if (controller_options[2])
			swapTriggersBumpers(ctrl);
	}
}

int retouch(SceUInt32 port, SceTouchData *pData, SceUInt32 nBufs, uint8_t hookId){
	if (!internal_touch_call && show_menu) { //Disable in menu
		pData[0] = pData[nBufs - 1];
		pData[0].reportNum = 0;
		return 1;
	}
	if (show_menu){	//Clear buffers when in menu
		remappedBuffersFrontIdxs[hookId] = 0;
		remappedBuffersRearIdxs[hookId] = 0;
		remappedBuffersFrontSizes[hookId] = 0;
		remappedBuffersRearSizes[hookId] = 0;
	}
	if (nBufs && !show_menu) {
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
	}
	return nBufs;
}

void remapInit(){
	for (int i = 0; i < HOOKS_NUM-5; i++) //Allocating mem for stored buffers
		remappedBuffers[i] = malloc(sizeof(SceCtrlData) * BUFFERS_NUM);
	for (int i = 0; i < 4; i++){
		remappedBuffersFront[i] = malloc(sizeof(SceTouchData) * BUFFERS_NUM);
		remappedBuffersRear[i] = malloc(sizeof(SceTouchData) * BUFFERS_NUM);
	}
}
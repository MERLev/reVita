#include <vitasdkkern.h>
#include <psp2/motion.h> 
#include <psp2/touch.h>
#include <psp2/appmgr.h>
#include <stdlib.h>

#include "vitasdkext.h"
#include "main.h"
#include "revita.h"
#include "remap.h"
#include "fio/profile.h"
#include "common.h"
#include "sysactions.h"
#include "gui/gui.h"
#include "log.h"

#define TURBO_DELAY			        (50*1000)
#define CACHE_CTRL_SIZE (sizeof(SceCtrlData) * BUFFERS_NUM * PORTS_NUM * PROC_NUM)
#define CACHE_TOUCH_SIZE (sizeof(SceTouchData) * BUFFERS_NUM * TOUCH_HOOKS_NUM * SCE_TOUCH_PORT_MAX_NUM)
#define MEM_SIZE ((0xfff + CACHE_CTRL_SIZE + CACHE_TOUCH_SIZE) & ~0xfff)

#define POS  1
#define NEG -1

enum RULE_STATUS{
	RS_NONACTIVE,   // Rule is stopped for some time
	RS_STARTED,     // Rule just started
	RS_ACTIVE,      // Rule is active for some time
	RS_STOPPED      // Rule just stopped
}RULE_STATUS;

typedef struct TouchCache{
	SceTouchData *buffers;
	int num;
}TouchCache;
typedef struct CtrlCache{
	SceCtrlData *buffers;
	int num;
}CtrlCache;

// Status of each rule
enum RULE_STATUS rs[PORTS_NUM][PROC_NUM][REMAP_NUM];
TouchPoints2 T_SIZE[SCE_TOUCH_PORT_MAX_NUM];

static TouchPoint 
	T_CENTER[SCE_TOUCH_PORT_MAX_NUM], 
	T_L[SCE_TOUCH_PORT_MAX_NUM], 
	T_R[SCE_TOUCH_PORT_MAX_NUM], 
	T_TL[SCE_TOUCH_PORT_MAX_NUM], 
	T_TR[SCE_TOUCH_PORT_MAX_NUM], 
	T_BL[SCE_TOUCH_PORT_MAX_NUM], 
	T_BR[SCE_TOUCH_PORT_MAX_NUM];
static TouchPoints2 
	TZ_CENTER[SCE_TOUCH_PORT_MAX_NUM], 
	TZ_L[SCE_TOUCH_PORT_MAX_NUM], 
	TZ_R[SCE_TOUCH_PORT_MAX_NUM], 
	TZ_TL[SCE_TOUCH_PORT_MAX_NUM], 
	TZ_TR[SCE_TOUCH_PORT_MAX_NUM], 
	TZ_BL[SCE_TOUCH_PORT_MAX_NUM], 
	TZ_BR[SCE_TOUCH_PORT_MAX_NUM];

const uint32_t HW_BUTTONS[HW_BUTTONS_NUM] = {
	SCE_CTRL_CROSS, SCE_CTRL_CIRCLE, SCE_CTRL_TRIANGLE, SCE_CTRL_SQUARE,
	SCE_CTRL_UP, SCE_CTRL_RIGHT, SCE_CTRL_LEFT, SCE_CTRL_DOWN,
	SCE_CTRL_START, SCE_CTRL_SELECT, 
	SCE_CTRL_L1, SCE_CTRL_R1, SCE_CTRL_L2, SCE_CTRL_R2, SCE_CTRL_L3, SCE_CTRL_R3, 
	SCE_CTRL_VOLUP, SCE_CTRL_VOLDOWN, SCE_CTRL_POWER, SCE_CTRL_PSBUTTON, SCE_CTRL_TOUCHPAD
};

SceUID remap_memId;
uint8_t* remap_memBase;

// Caches to store remapped buffers per each hook and port
static struct CtrlCache cacheCtrl[PORTS_NUM][PROC_NUM];
static struct TouchCache cacheTouch[TOUCH_HOOKS_NUM][SCE_TOUCH_PORT_MAX_NUM];

bool newEmulatedTouchBuffer[TOUCH_HOOKS_NUM][SCE_TOUCH_PORT_MAX_NUM];

static EmulatedTouch 
	et[SCE_TOUCH_PORT_MAX_NUM], 
	etPrev[SCE_TOUCH_PORT_MAX_NUM];
static uint8_t etIdCounter[SCE_TOUCH_PORT_MAX_NUM] = {64, 64};

// Create clean remap rule
struct RemapRule remap_createRemapRule(){
	struct RemapRule rr;
	memset(&rr, 0, sizeof(rr));
	rr.emu.param.tPoint.x = 200;
	rr.emu.param.tPoint.y = 200;
	rr.emu.param.tPoints.a.x = 200;
	rr.emu.param.tPoints.a.y = 200;
	rr.emu.param.tPoints.b.x = 1700;
	rr.emu.param.tPoints.b.y = 825;
	rr.trigger.param.tPoint.x = 200;
	rr.trigger.param.tPoint.y = 200;
	rr.trigger.param.tPoints.a.x = 200;
	rr.trigger.param.tPoints.a.y = 200;
	rr.trigger.param.tPoints.b.x = 1700;
	rr.trigger.param.tPoints.b.y = 825;
	return rr;
}

// Update rule status
bool updateStatus(enum RULE_STATUS* status, bool active){
	switch (*status){
		case RS_ACTIVE: if (!active) *status = RS_STOPPED; break;
		case RS_NONACTIVE: if (active) *status = RS_STARTED; break;
		case RS_STARTED: *status = active ? RS_ACTIVE : RS_STOPPED; break;
		case RS_STOPPED: *status = active ? RS_STARTED : RS_NONACTIVE; break;
	}
	return active;
}

// Is TouchPoint inside zone
bool inZone(const TouchPoint tp, const TouchPoints2 tz){
	if (tp.x >= min(tz.a.x, tz.b.x) && tp.x <= max(tz.a.x, tz.b.x) &&
		tp.y >= min(tz.a.y, tz.b.y) && tp.y <= max(tz.a.y, tz.b.y))
		return true;
	return false;
}

// Is SceTouchReport inside zone
bool reportInZone(SceTouchReport* str, const TouchPoints2 tz){
	return inZone((TouchPoint){str->x, str->y}, tz);
}

// Store emulated touchpoint
void storeTouchPoint(EmulatedTouch *et, TouchPoint tp, int port, int ruleIdx){
	for (int i = 0; i < et->num; i++)
		if (et->reports[i].point.x == tp.x && et->reports[i].point.y == tp.y)
			return;
	et->reports[et->num].point.x = tp.x;
	et->reports[et->num].point.y = tp.y;
	et->reports[et->num].isSwipe = false;
	et->reports[et->num].isSmartSwipe = false;
	et->reports[et->num].port = port;
	et->reports[et->num].ruleIdx = ruleIdx;
	et->num++;
}

// Store emulated swipe
void storeTouchSwipe(EmulatedTouch *et, TouchPoints2 tps, int port, int ruleIdx){
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
	et->reports[et->num].port = port;
	et->reports[et->num].ruleIdx = ruleIdx;
	et->num++;
}

// Store emulated controllable swipe
EmulatedTouchEvent* storeTouchSmartSwipe(EmulatedTouch *et, TouchPoint tp, int port, int ruleIdx){
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
	ete->port = port;
	ete->ruleIdx = ruleIdx;
	et->num++;
	return ete;
}

bool isTpEqual(TouchPoint tp1, TouchPoint tp2){
	return tp1.x == tp2.x && tp1.y == tp2.y;
}

void removeEmuReportByIdx(EmulatedTouch *et, int idx){
	for (int i = idx + 1; i < et->num; i++)
		et->reports[i - 1] = et->reports[i];
	et->num--;
}

void removeEmuReport(EmulatedTouch *etouch, int port, int ruleIdx){
	int i = 0;
	while (i < etouch->num){
		if (etouch->reports[i].port == port && etouch->reports[i].ruleIdx == ruleIdx){
			removeEmuReportByIdx(etouch, i);
			break;
		}
		i++;
	}
}

void cleanEmuReports(EmulatedTouch *et){
	int i = 0;
	while (i < et->num){
		if (et->reports[i].isSwipeFinished) 
			removeEmuReportByIdx(et, i);
		else 
			i++;
	}
}

// Add emulated event from rule
void addEmu(RuleData* rd) {
	struct RemapAction* emu = &rd->rr->emu;
	EmulatedTouchEvent* ete;
	switch (emu->type){
		case REMAP_TYPE_BUTTON: 
			if (rd->rr->turbo && rd->isTurboTick) break;
			btn_add(&rd->btnsEmu, emu->param.btn);
			break;
		case REMAP_TYPE_RIGHT_ANALOG: 
		case REMAP_TYPE_RIGHT_ANALOG_DIGITAL: 
		case REMAP_TYPE_LEFT_ANALOG: 
		case REMAP_TYPE_LEFT_ANALOG_DIGITAL: 
			if (rd->rr->turbo && rd->isTurboTick) break;
			;EmulatedStick* stick = (emu->type == REMAP_TYPE_LEFT_ANALOG || emu->type == REMAP_TYPE_LEFT_ANALOG_DIGITAL) ?
				&rd->analogLeftEmu : &rd->analogRightEmu;
			switch (emu->action){
				case REMAP_ANALOG_UP:    stick->up    += rd->stickposval; break;
				case REMAP_ANALOG_DOWN:  stick->down  += rd->stickposval; break;
				case REMAP_ANALOG_LEFT:  stick->left  += rd->stickposval; break;
				case REMAP_ANALOG_RIGHT: stick->right += rd->stickposval; break;
				default: break;
			}
			break;
		case REMAP_TYPE_FRONT_TOUCH_POINT: 
		case REMAP_TYPE_BACK_TOUCH_POINT:
			if (rd->port == 1)
				break;
			;int port = emu->type == REMAP_TYPE_FRONT_TOUCH_POINT ? SCE_TOUCH_PORT_FRONT : SCE_TOUCH_PORT_BACK;
			if (rd->rr->turbo && rd->isTurboTick && emu->action != REMAP_TOUCH_SWIPE 
					&& emu->action != REMAP_TOUCH_SWIPE_SMART_L && emu->action != REMAP_TOUCH_SWIPE_SMART_R)
				break;
			switch (emu->action){
				case REMAP_TOUCH_ZONE_CENTER:	storeTouchPoint(&et[port], T_CENTER[port], rd->port, rd->idx);  break;
				case REMAP_TOUCH_ZONE_L:  		storeTouchPoint(&et[port], T_L[port], rd->port, rd->idx);  break;
				case REMAP_TOUCH_ZONE_R:  		storeTouchPoint(&et[port], T_R[port], rd->port, rd->idx);  break;
				case REMAP_TOUCH_ZONE_TL: 		storeTouchPoint(&et[port], T_TL[port], rd->port, rd->idx); break;
				case REMAP_TOUCH_ZONE_TR: 		storeTouchPoint(&et[port], T_TR[port], rd->port, rd->idx); break;
				case REMAP_TOUCH_ZONE_BL: 		storeTouchPoint(&et[port], T_BL[port], rd->port, rd->idx); break;
				case REMAP_TOUCH_ZONE_BR: 		storeTouchPoint(&et[port], T_BR[port], rd->port, rd->idx); break;
				case REMAP_TOUCH_CUSTOM:  		storeTouchPoint(&et[port], emu->param.tPoint, rd->port, rd->idx); break;
				case REMAP_TOUCH_SWIPE:   
					if (*rd->status == RS_STARTED || (*rd->status == RS_ACTIVE && rd->rr->turbo && rd->isTurboTick)) 
						storeTouchSwipe(&et[port], emu->param.tPoints, rd->port, rd->idx); 
					break;
				case REMAP_TOUCH_SWIPE_SMART_DPAD:  
					ete = storeTouchSmartSwipe(&et[port], emu->param.tPoint, rd->port, rd->idx);
					if (btn_has(rd->btns, SCE_CTRL_LEFT)) 
						ete->swipeEndPoint.x = clamp(
							ete->swipeEndPoint.x - profile.entries[PR_TO_SWIPE_SMART_SENS].v.u,
							T_SIZE[port].a.x, 
							T_SIZE[port].b.x);
					if (btn_has(rd->btns, SCE_CTRL_RIGHT)) 
						ete->swipeEndPoint.x = clamp(
							ete->swipeEndPoint.x + profile.entries[PR_TO_SWIPE_SMART_SENS].v.u,
							T_SIZE[port].a.x, 
							T_SIZE[port].b.x);
					if (btn_has(rd->btns, SCE_CTRL_UP)) 
						ete->swipeEndPoint.y = clamp(
							ete->swipeEndPoint.y - profile.entries[PR_TO_SWIPE_SMART_SENS].v.u,
							T_SIZE[port].a.y, 
							T_SIZE[port].b.y);
					if (btn_has(rd->btns, SCE_CTRL_DOWN)) 
						ete->swipeEndPoint.y = clamp(
							ete->swipeEndPoint.y + profile.entries[PR_TO_SWIPE_SMART_SENS].v.u,
							T_SIZE[port].a.y, 
							T_SIZE[port].b.y);
					if (!rd->rr->propagate){
						btn_del(&rd->btnsProp, SCE_CTRL_LEFT);
						btn_del(&rd->btnsProp, SCE_CTRL_RIGHT);
						btn_del(&rd->btnsProp, SCE_CTRL_UP);
						btn_del(&rd->btnsProp, SCE_CTRL_DOWN);
					}
					break;
				case REMAP_TOUCH_SWIPE_SMART_L:  
					ete = storeTouchSmartSwipe(&et[port], emu->param.tPoint, rd->port, rd->idx);
					if (abs(127 - rd->ctrl->lx) > profile.entries[PR_AN_LEFT_DEADZONE_X].v.u)
						ete->swipeEndPoint.x = clamp(
							ete->swipeEndPoint.x + 
								((float)(rd->ctrl->lx - 127)) * profile.entries[PR_TO_SWIPE_SMART_SENS].v.u / 127,
							T_SIZE[port].a.x, 
							T_SIZE[port].b.x);
					if (abs(127 - rd->ctrl->ly) > profile.entries[PR_AN_LEFT_DEADZONE_Y].v.u)
						ete->swipeEndPoint.y = clamp(ete->swipeEndPoint.y + 
							((float)(rd->ctrl->ly - 127)) * profile.entries[PR_TO_SWIPE_SMART_SENS].v.u / 127,
							T_SIZE[port].a.y, 
							T_SIZE[port].b.y);
					if (!rd->rr->propagate)
						rd->analogLeftProp.left = rd->analogLeftProp.right = 
								rd->analogLeftProp.up = rd->analogLeftProp.down = 0;
					break;
				case REMAP_TOUCH_SWIPE_SMART_R:  
					ete = storeTouchSmartSwipe(&et[port], emu->param.tPoint, rd->port, rd->idx); 
					if (abs(127 - rd->ctrl->rx) > profile.entries[PR_AN_LEFT_DEADZONE_X].v.u)
						ete->swipeEndPoint.x = clamp(
							ete->swipeEndPoint.x + 
								((float)(rd->ctrl->rx - 127)) * profile.entries[PR_TO_SWIPE_SMART_SENS].v.u / 127,
							T_SIZE[port].a.x, 
							T_SIZE[port].b.x);
					if (abs(127 - rd->ctrl->ry) > profile.entries[PR_AN_LEFT_DEADZONE_X].v.u)
						ete->swipeEndPoint.y = clamp(
							ete->swipeEndPoint.y + 
								((float)(rd->ctrl->ry - 127)) * profile.entries[PR_TO_SWIPE_SMART_SENS].v.u / 127,
							T_SIZE[port].a.y, 
							T_SIZE[port].b.y);
					if (!rd->rr->propagate)
						rd->analogRightProp.left = rd->analogRightProp.right = 
								rd->analogRightProp.up = rd->analogRightProp.down = 0;
					break;
				default: break;
			}
			break;
		case REMAP_TYPE_SYSACTIONS:
			if (rd->port == 1)
				break;
			if (rd->rr->turbo && rd->isTurboTick)
				break;
			if (!rd->rr->turbo && *rd->status != RS_STARTED) 
				break;
			switch (emu->action){
				case REMAP_SYS_RESET_SOFT: 		sysactions_softReset(); break;
				case REMAP_SYS_RESET_COLD: 		sysactions_coldReset(); break;
				case REMAP_SYS_STANDBY: 		sysactions_standby(); break;
				case REMAP_SYS_SUSPEND: 		sysactions_suspend(); break;
				case REMAP_SYS_DISPLAY_OFF: 	sysactions_displayOff(); break;
				case REMAP_SYS_KILL: 			sysactions_killCurrentApp(); break;
				case REMAP_SYS_BRIGHTNESS_INC: 	sysactions_brightnessInc(); break;
				case REMAP_SYS_BRIGHTNESS_DEC: 	sysactions_brightnessDec(); break;
				case REMAP_SYS_SAVE_BACKUP: 	sysactions_saveRestore(); break;
				case REMAP_SYS_SAVE_RESTORE: 	sysactions_saveBackup(); break;
				case REMAP_SYS_SAVE_DELETE: 	sysactions_saveDelete(); break;
				case REMAP_SYS_CALIBRATE_MOTION:sysactions_calibrateMotion(); break;
				default: break;
			}
			break;
		case REMAP_TYPE_REMAPSV_ACTIONS:
			if (rd->port == 1)
				break;
			if (*rd->status != RS_STARTED) break;
			LOG("profile_inc(&profile.entries[%i], 1)\n", emu->action);
			profile_inc(&profile.entries[emu->action], 1);
        	gui_popupShow("Toggled", profile.entries[emu->action].key, TTL_POPUP_SHORT);
			break;
		case REMAP_TYPE_DISABLED:
			// Do nothing
			break;
		default: break;
	}
}

void remEmu(RuleData* rd) {
	switch (rd->rr->emu.type){
		case REMAP_TYPE_FRONT_TOUCH_POINT: 
		case REMAP_TYPE_BACK_TOUCH_POINT: 
			;int port = rd->rr->emu.type == REMAP_TYPE_FRONT_TOUCH_POINT ? SCE_TOUCH_PORT_FRONT : SCE_TOUCH_PORT_BACK;
			switch (rd->rr->emu.action){
				case REMAP_TOUCH_SWIPE: /* Remove on finish */ break;
				default: removeEmuReport(&et[port], rd->port, rd->idx);	break;
			}
			break;
		default: break;
	}
}

int convertGyroVal(float val, int sensId, int deadId, int antideadId){
	// Scale
	int scaledVal = clamp(val * 127 * profile.entries[sensId].v.u / 100, 0, 127);

	// Apply Deadzone and Anti-Deadzone
	return 127 * profile.entries[antideadId].v.u / 100 
		+ ((float)scaledVal - 127 * profile.entries[deadId].v.u / 100) 
		* (100 - profile.entries[antideadId].v.u) / (100 - profile.entries[deadId].v.u);
}

void gyroRule(RuleData* rd, float val, int multi, int sign, int sensId, int deadId, int antideadId){
	if (val * sign > 0){
		if (updateStatus(rd->status, abs(val * 100) > (int)profile.entries[deadId].v.u)){
			rd->stickposval = convertGyroVal(val * multi * sign, sensId, deadId, antideadId);
			addEmu(rd);
		}
	}
	if (*rd->status == RS_STOPPED)
		remEmu(rd);
}

float calibrate(float val, float add){
	val -= add;
	if (val > 1.0)
		val -= 2;
	if (val < -1.0)
		val += 2;
	return val;
}

void applyRemap(SceCtrlData *ctrl, enum RULE_STATUS* statuses, int port) {	
	// Gathering real touch data
	SceTouchData std[SCE_TOUCH_PORT_MAX_NUM];
	int retTouch[SCE_TOUCH_PORT_MAX_NUM] = {-1, -1};
	if (port <= 1) { // Touch should be used only by Player 1
		ksceTouchSetSamplingState(SCE_TOUCH_PORT_FRONT, SCE_TOUCH_SAMPLING_STATE_START);
		ksceTouchSetSamplingState(SCE_TOUCH_PORT_BACK, SCE_TOUCH_SAMPLING_STATE_START);
		retTouch[SCE_TOUCH_PORT_FRONT] = ksceTouchPeek_internal(SCE_TOUCH_PORT_FRONT, &std[SCE_TOUCH_PORT_FRONT], 1);
		retTouch[SCE_TOUCH_PORT_BACK] = ksceTouchPeek_internal(SCE_TOUCH_PORT_BACK, &std[SCE_TOUCH_PORT_BACK], 1);
	}
	
	// Read motion data
	SceMotionState sms;
	int gyroRet = -1;
	if (port <= 1) // Motion should be used only by Player 1
		gyroRet = revita_sceMotionGetState(&sms);

	if (gyroRet >= 0){
		sms.rotationMatrix.x.z = calibrate(sms.rotationMatrix.x.z, (float)(profile.entries[PR_GY_CALIBRATION_X].v.i) / 1000);
		sms.rotationMatrix.y.z = calibrate(sms.rotationMatrix.y.z, (float)(profile.entries[PR_GY_CALIBRATION_Y].v.i) / 1000);
		sms.acceleration.x = calibrate(sms.acceleration.x, (float)(profile.entries[PR_GY_CALIBRATION_Z].v.i) / 1000);
	}

	// Create RuleData
	RuleData rd;
	rd.isTurboTick = (ksceKernelGetSystemTimeWide() % TURBO_DELAY) < (TURBO_DELAY / 2);
	rd.btns = ctrl->buttons;
	rd.btnsProp = ctrl->buttons;
	rd.btnsEmu = 0;
	rd.ctrl = ctrl;
	rd.port = port;

	// Set sticks def values
	rd.analogLeftEmu = rd.analogRightEmu = rd.analogLeftProp = rd.analogRightProp = (EmulatedStick){0, 0, 0, 0};
	if (ctrl->lx <= 127) rd.analogLeftProp.left = 127 - ctrl->lx;
	else rd.analogLeftProp.right = ctrl->lx - 127;
	if (ctrl->ly <= 127) rd.analogLeftProp.up = 127 - ctrl->ly;
	else  rd.analogLeftProp.down = ctrl->ly - 127;
	if (ctrl->rx <= 127) rd.analogRightProp.left = 127 - ctrl->rx;
	else  rd.analogRightProp.right = ctrl->rx - 127;
	if (ctrl->ry <= 127) rd.analogRightProp.up = 127 - ctrl->ry;
	else  rd.analogRightProp.down = ctrl->ry - 127;

	// Applying remap rules
	for (int i = 0; i < profile.remapsNum; i++){
		rd.idx = i;
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
				}
				if (*rd.status == RS_STOPPED)
					remEmu(&rd);
				break;
			case REMAP_TYPE_LEFT_ANALOG: 
				switch(trigger->action){
					case REMAP_ANALOG_LEFT: 
						if (!rr->propagate) rd.analogLeftProp.left = 0;
						if (updateStatus(rd.status, ctrl->lx <= 127 - profile.entries[PR_AN_LEFT_DEADZONE_X].v.u)){
							rd.stickposval = 127 - ctrl->lx;
							addEmu(&rd);
						}
						if (*rd.status == RS_STOPPED)
							remEmu(&rd);
						break;
					case REMAP_ANALOG_RIGHT: 
						if (!rr->propagate) rd.analogLeftProp.right = 0;
						if (updateStatus(rd.status, ctrl->lx > 127 + profile.entries[PR_AN_LEFT_DEADZONE_X].v.u)){
							rd.stickposval = ctrl->lx - 127;
							addEmu(&rd);
						}
						if (*rd.status == RS_STOPPED)
							remEmu(&rd);
						break;
					case REMAP_ANALOG_UP: 
						if (!rr->propagate) rd.analogLeftProp.up = 0;
						if (updateStatus(rd.status, ctrl->ly <= 127 - profile.entries[PR_AN_LEFT_DEADZONE_Y].v.u)){
							rd.stickposval = 127 - ctrl->ly;
							addEmu(&rd);
						} 
						if (*rd.status == RS_STOPPED)
							remEmu(&rd);
						break;
					case REMAP_ANALOG_DOWN: 
						if (!rr->propagate) rd.analogLeftProp.down = 0;
						if (updateStatus(rd.status, ctrl->ly > 127 + profile.entries[PR_AN_LEFT_DEADZONE_Y].v.u)){
							rd.stickposval = ctrl->ly - 127;
							addEmu(&rd);
						} 
						if (*rd.status == RS_STOPPED)
							remEmu(&rd);
						break;
					default: break;
				}
				break;
			case REMAP_TYPE_RIGHT_ANALOG: 
				switch(trigger->action){
					case REMAP_ANALOG_LEFT: 
						if (!rr->propagate) rd.analogRightProp.left = 0;
						if (updateStatus(rd.status, ctrl->rx <= 127 - profile.entries[PR_AN_RIGHT_DEADZONE_X].v.u)){
							rd.stickposval = 127 - ctrl->rx;
							addEmu(&rd);
						} 
						if (*rd.status == RS_STOPPED)
							remEmu(&rd);
						break;
					case REMAP_ANALOG_RIGHT: 
						if (!rr->propagate) rd.analogRightProp.right = 0;
						if (updateStatus(rd.status, ctrl->rx > 127 + profile.entries[PR_AN_RIGHT_DEADZONE_X].v.u)){
							rd.stickposval = ctrl->rx - 127;
							addEmu(&rd);
						} 
						if (*rd.status == RS_STOPPED)
							remEmu(&rd);
						break;
					case REMAP_ANALOG_UP: 
						if (!rr->propagate) rd.analogRightProp.up = 0;
						if (updateStatus(rd.status, ctrl->ry <= 127 - profile.entries[PR_AN_RIGHT_DEADZONE_Y].v.u)){
							rd.stickposval = 127 - ctrl->ry;
							addEmu(&rd);
						} 
						if (*rd.status == RS_STOPPED)
							remEmu(&rd);
						break;
					case REMAP_ANALOG_DOWN: 
						if (!rr->propagate) rd.analogRightProp.down = 0;
						if (updateStatus(rd.status, ctrl->ry > 127 + profile.entries[PR_AN_RIGHT_DEADZONE_Y].v.u)){
							rd.stickposval = ctrl->ry - 127;
							addEmu(&rd);
						} 
						if (*rd.status == RS_STOPPED)
							remEmu(&rd);
						break;
					default: break;
				}
				break;
			case REMAP_TYPE_FRONT_TOUCH_ZONE:
			case REMAP_TYPE_BACK_TOUCH_ZONE: 
				;int tport = trigger->type == REMAP_TYPE_FRONT_TOUCH_ZONE ? SCE_TOUCH_PORT_FRONT : SCE_TOUCH_PORT_BACK;
				if (retTouch[tport] < 1 || retTouch[tport] > 64) 
					break;
				if (rr->emu.action == REMAP_TOUCH_SWIPE_SMART_DPAD || 
						rr->emu.action == REMAP_TOUCH_SWIPE_SMART_L ||
						rr->emu.action == REMAP_TOUCH_SWIPE_SMART_R)
					break;	// Disable touch -> Smart swipe
				bool found = false;
				for (int j = 0; j < std[tport].reportNum; j++) {
					TouchPoints2 tz = TZ_L[tport];
					switch (trigger->action){
						case REMAP_TOUCH_ZONE_FULL:  	tz = T_SIZE[tport];  break;
						case REMAP_TOUCH_ZONE_CENTER:  	tz = TZ_CENTER[tport];  break;
						case REMAP_TOUCH_ZONE_L:  		tz = TZ_L[tport];  break;
						case REMAP_TOUCH_ZONE_R:  		tz = TZ_R[tport];  break;
						case REMAP_TOUCH_ZONE_TL: 		tz = TZ_TL[tport]; break;
						case REMAP_TOUCH_ZONE_TR: 		tz = TZ_TR[tport]; break;
						case REMAP_TOUCH_ZONE_BL: 		tz = TZ_BL[tport]; break;
						case REMAP_TOUCH_ZONE_BR: 		tz = TZ_BR[tport]; break;
						case REMAP_TOUCH_CUSTOM:  		tz = trigger->param.tPoints; break;
						default: break;
					}
					if (reportInZone(&std[tport].report[j], tz)){
						found = true;
					}
				}
				if (updateStatus(rd.status, found)) {
					addEmu(&rd);
				} 
				if (*rd.status == RS_STOPPED)
					remEmu(&rd);
				break;
			case REMAP_TYPE_GYROSCOPE: 
				if (gyroRet != 0) break;
				switch (trigger->action){
				/*  T-------------T---------------T----------------T
					|             |      CAM      |       SIM      |
				    |-------------|---------------|----------------|
                    | horisontal  | AngVelocity.y |  rotation.x.z  |
                    | vertical    | AngVelocity.x |  rotation.y.y  |
					| roll        | AngVelocity.z | Acceleration.x |  
					|-------------|---------------|----------------|  */
					// Camera mode
					case REMAP_GYRO_LEFT:
						gyroRule(&rd, sms.angularVelocity.y, 4, POS, 
							PR_GY_SENSITIVITY_X, PR_GY_DEADZONE_X, PR_GY_ANTIDEADZONE_X);
						break;
					case REMAP_GYRO_RIGHT:
						gyroRule(&rd, sms.angularVelocity.y, 4, NEG, 
							PR_GY_SENSITIVITY_X, PR_GY_DEADZONE_X, PR_GY_ANTIDEADZONE_X);
						break;
					case REMAP_GYRO_UP: 
						gyroRule(&rd, sms.angularVelocity.x, 4, POS, 
							PR_GY_SENSITIVITY_Y, PR_GY_DEADZONE_Y, PR_GY_ANTIDEADZONE_Y);
						break;
					case REMAP_GYRO_DOWN:
						gyroRule(&rd, sms.angularVelocity.x, 4, NEG, 
							PR_GY_SENSITIVITY_Y, PR_GY_DEADZONE_Y, PR_GY_ANTIDEADZONE_Y);
						break;
					case REMAP_GYRO_ROLL_LEFT:
						gyroRule(&rd, sms.angularVelocity.z, 4, POS, 
							PR_GY_SENSITIVITY_Z, PR_GY_DEADZONE_Z, PR_GY_ANTIDEADZONE_Z);
						break;
					case REMAP_GYRO_ROLL_RIGHT:
						gyroRule(&rd, sms.angularVelocity.z, 4, NEG, 
							PR_GY_SENSITIVITY_Z, PR_GY_DEADZONE_Z, PR_GY_ANTIDEADZONE_Z);
						break;
					// Sim mode
					case REMAP_GYRO_SIM_LEFT: 
						gyroRule(&rd, sms.rotationMatrix.x.z, 8, NEG, 
							PR_GY_SENSITIVITY_X, PR_GY_DEADZONE_X, PR_GY_ANTIDEADZONE_X);
						break;
					case REMAP_GYRO_SIM_RIGHT:
						gyroRule(&rd, sms.rotationMatrix.x.z, 8, POS, 
							PR_GY_SENSITIVITY_X, PR_GY_DEADZONE_X, PR_GY_ANTIDEADZONE_X);
						break;
					case REMAP_GYRO_SIM_UP:
						gyroRule(&rd, sms.rotationMatrix.y.z, 8, NEG, 
							PR_GY_SENSITIVITY_Y, PR_GY_DEADZONE_Y, PR_GY_ANTIDEADZONE_Y);
						break;
					case REMAP_GYRO_SIM_DOWN:
						gyroRule(&rd, sms.rotationMatrix.y.z, 8, POS, 
							PR_GY_SENSITIVITY_Y, PR_GY_DEADZONE_Y, PR_GY_ANTIDEADZONE_Y);
						break;
					case REMAP_GYRO_SIM_ROLL_LEFT:
						gyroRule(&rd, sms.acceleration.x, 8, NEG, 
							PR_GY_SENSITIVITY_Z, PR_GY_DEADZONE_Z, PR_GY_ANTIDEADZONE_Z);
						break;
					case REMAP_GYRO_SIM_ROLL_RIGHT:
						gyroRule(&rd, sms.acceleration.x, 8, POS, 
							PR_GY_SENSITIVITY_Z, PR_GY_DEADZONE_Z, PR_GY_ANTIDEADZONE_Z);
						break;
					default: break;
				}
				break;
			default: break;
		}
	}

	// Remove minimal drift if digital remap for stick directions is used
	// if (stickpos[0] || stickpos[1] || stickpos[2] || stickpos[3]){
	// 	if (abs(ctrl->lx - 127) < profile.entries[0]) 
	// 		ctrl->lx = 127;
	// 	if (abs(ctrl->ly - 127) < profile.entries[1]) 
	// 		ctrl->ly = 127;
	// }
	// if (stickpos[4] || stickpos[5] || stickpos[6] || stickpos[7]){
	// 	if (abs(ctrl->rx - 127) < profile.entries[2]) 
	// 		ctrl->rx = 127;
	// 	if (abs(ctrl->ry - 127) < profile.entries[3]) 
	// 		ctrl->ry = 127;
	// }

	// Storing emulated HW buttons
	btn_add(&rd.btnsEmu, rd.btnsProp);
	ctrl->buttons = rd.btnsEmu;

	// Restoring propagated values for analog sticks
	rd.analogLeftEmu.left += rd.analogLeftProp.left;
	rd.analogLeftEmu.right += rd.analogLeftProp.right;
	rd.analogLeftEmu.up += rd.analogLeftProp.up;
	rd.analogLeftEmu.down += rd.analogLeftProp.down;
	rd.analogRightEmu.left += rd.analogRightProp.left;
	rd.analogRightEmu.right += rd.analogRightProp.right;
	rd.analogRightEmu.up += rd.analogRightProp.up;
	rd.analogRightEmu.down += rd.analogRightProp.down;

	// Storing remap for analog axises
	ctrl->lx = clamp(127  + rd.analogLeftEmu.right- rd.analogLeftEmu.left, 0, 255);
	ctrl->ly = clamp(127 + rd.analogLeftEmu.down - rd.analogLeftEmu.up, 0, 255);
	ctrl->rx = clamp(127 + rd.analogRightEmu.right - rd.analogRightEmu.left, 0, 255);
	ctrl->ry = clamp(127 + rd.analogRightEmu.down - rd.analogRightEmu.up, 0, 255);

	// Telling that new emulated touch buffer is ready to be taken
	for (int i = 0; i < TOUCH_HOOKS_NUM; i++)
		for (int j = 0; j < SCE_TOUCH_PORT_MAX_NUM; j++)
			newEmulatedTouchBuffer[i][j] = true;
}

// Swap L1<>L2 R1<>R2
void remap_swapSideButtons(SceCtrlData *ctrl){
	uint32_t oldBtns = *&ctrl->buttons;
	btn_del(&ctrl->buttons, SCE_CTRL_L1);
	btn_del(&ctrl->buttons, SCE_CTRL_R1);
	btn_del(&ctrl->buttons, SCE_CTRL_L2);
	btn_del(&ctrl->buttons, SCE_CTRL_R2);
	if (btn_has(oldBtns, SCE_CTRL_L2)) 
		btn_add(&ctrl->buttons, SCE_CTRL_L1);
	if (btn_has(oldBtns, SCE_CTRL_R2)) 
		btn_add(&ctrl->buttons, SCE_CTRL_R1);
	if (btn_has(oldBtns, SCE_CTRL_L1)) 
		btn_add(&ctrl->buttons, SCE_CTRL_L2);
	if (btn_has(oldBtns, SCE_CTRL_R1)) 
		btn_add(&ctrl->buttons, SCE_CTRL_R2);
	ctrl->lt = btn_has(oldBtns, SCE_CTRL_L1) ? 255 : 0;
	ctrl->rt = btn_has(oldBtns, SCE_CTRL_R1) ? 255 : 0;
}

// Change L1 -> L2, R1 -> R2, remove L1 R1 L3 R3
void remap_fixSideButtons(SceCtrlData *ctrl){
	uint32_t oldBtns = *&ctrl->buttons;
	btn_del(&ctrl->buttons, SCE_CTRL_L2);
	btn_del(&ctrl->buttons, SCE_CTRL_R2);
	btn_del(&ctrl->buttons, SCE_CTRL_L1);
	btn_del(&ctrl->buttons, SCE_CTRL_R1);
	btn_del(&ctrl->buttons, SCE_CTRL_L3);
	btn_del(&ctrl->buttons, SCE_CTRL_R3);
	if (btn_has(oldBtns, SCE_CTRL_L1)) 
		btn_add(&ctrl->buttons, SCE_CTRL_L2);
	if (btn_has(oldBtns, SCE_CTRL_R1)) 
		btn_add(&ctrl->buttons, SCE_CTRL_R2);
	ctrl->lt = 0;
	ctrl->rt = 0;
}

int remap_ctrl_getBufferNum(int port){
	return cacheCtrl[port][isCallShell()].num;
}

void fixScreenshotPropagation(SceCtrlData *ctrl){
	if (btn_has(ctrl->buttons, SCE_CTRL_PSBUTTON) && btn_has(ctrl->buttons, SCE_CTRL_START))
		btn_del(&ctrl->buttons, SCE_CTRL_START);
}

void remap_ctrl_updateBuffers(int port, SceCtrlData *ctrl, bool isPositiveLogic, bool isExt) {
	// Check if call is from Shell
	int isShell = isCallShell();

	// If buffer for timestamp is already remapped
	if (cacheCtrl[port][isShell].num > 0 && ctrl->timeStamp == cacheCtrl[port][isShell].buffers[cacheCtrl[port][isShell].num - 1].timeStamp){
		return;
	}

	// If buffer full - remove latest entry
	if (cacheCtrl[port][isShell].num >= BUFFERS_NUM){
		for (int i = 1; i < BUFFERS_NUM; i++)
			cacheCtrl[port][isShell].buffers[i - 1] = cacheCtrl[port][isShell].buffers[i];
		cacheCtrl[port][isShell].num--;
	}

	// Add curr ctrl to buffer
	int idx = cacheCtrl[port][isShell].num;
	cacheCtrl[port][isShell].num++;
	cacheCtrl[port][isShell].buffers[idx] = ctrl[0];

	// Invert for negative logic
	if (!isPositiveLogic)
		cacheCtrl[port][isShell].buffers[idx].buttons = 0xFFFFFFFF - cacheCtrl[port][isShell].buffers[idx].buttons;

	// Swap side buttons for non-Ext hooks for Vita mode
	if (!isExt)
		remap_swapSideButtons(&cacheCtrl[port][isShell].buffers[idx]);

	// Patch for more buttons
    if (!isExt){
    	SceCtrlData scd_ext;
        if (ksceCtrlPeekBufferPositive2_internal(port, &scd_ext, 1) > 0){
			cacheCtrl[port][isShell].buffers[idx].buttons |= scd_ext.buttons;
		}
    }

	// Apply remap
	applyRemap(&cacheCtrl[port][isShell].buffers[idx], &rs[port][0][0], port);

	// Fix screenshot propagation when sys buttons hack is active
	if (!isShell)
		fixScreenshotPropagation(&cacheCtrl[port][isShell].buffers[idx]);
}

int remap_ctrl_readBuffer(int port, SceCtrlData *ctrl, int buffIdx, bool isPositiveLogic, bool isExt){
	// Check if call is from Shell
	int isShell = isCallShell();
	
	// Not enough buffers cached
	if (buffIdx >= cacheCtrl[port][isShell].num)
		return false;

	// Read buffer from cache
	*ctrl = cacheCtrl[port][isShell].buffers[cacheCtrl[port][isShell].num - buffIdx];

	// Swap L1<>LT R1<>RT
	if (profile.entries[PR_CO_SWAP_BUTTONS].v.b)
		remap_swapSideButtons(ctrl);
		
	// Remove ext buttons for non-ext calls
	if (!isExt)
		remap_fixSideButtons(ctrl);

	// Invert back for negative logic
	if (!isPositiveLogic)
		ctrl->buttons = 0xFFFFFFFF - ctrl->buttons;

	return true;
}

//Keep same touch id for continuus touches
uint8_t generateTouchId(int x, int y, int port){ 
	for (int i = 0; i < etPrev[port].num; i++)
		if (etPrev[port].reports[i].point.x == x && etPrev[port].reports[i].point.y == y)
			return etPrev[port].reports[i].id;
	etIdCounter[port] = (etIdCounter[port] + 1) % 127;
	return etIdCounter[port];
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
			et->reports[touchIdx].swipeCurrentPoint.x = clampSmart(
				ax + (bx - ax) * (tick - et->reports[touchIdx].tick) / (profile.entries[PR_TO_SWIPE_DURATION].v.u * 1000), 
				ax, bx);
			et->reports[touchIdx].swipeCurrentPoint.y = clampSmart(
				ay + (by - ay) * (tick - et->reports[touchIdx].tick) / (profile.entries[PR_TO_SWIPE_DURATION].v.u * 1000), 
				ay, by);
			pData->report[pData->reportNum].x = et->reports[touchIdx].swipeCurrentPoint.x;
			pData->report[pData->reportNum].y = et->reports[touchIdx].swipeCurrentPoint.y;

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
void updateTouchInfo(SceUInt32 port, int hookId, SceTouchData *pData){	
	// Remove non-propagated remapped touches
	for (int i = 0; i < profile.remapsNum; i++){
		int ratype = (port == SCE_TOUCH_PORT_FRONT) ? REMAP_TYPE_FRONT_TOUCH_ZONE : REMAP_TYPE_BACK_TOUCH_ZONE;
		if (!profile.remaps[i].propagate && !profile.remaps[i].disabled && profile.remaps[i].trigger.type == ratype){
			int j = 0;
			while (j < pData->reportNum){
				TouchPoints2 tz = TZ_L[port];
				switch (profile.remaps[i].trigger.action){
					case REMAP_TOUCH_ZONE_FULL:  	tz = T_SIZE[port];  break;
					case REMAP_TOUCH_ZONE_CENTER:  	tz = TZ_CENTER[port];  break;
					case REMAP_TOUCH_ZONE_L:  		tz = TZ_L[port];  break;
					case REMAP_TOUCH_ZONE_R:  		tz = TZ_R[port];  break;
					case REMAP_TOUCH_ZONE_TL: 		tz = TZ_TL[port]; break;
					case REMAP_TOUCH_ZONE_TR: 		tz = TZ_TR[port]; break;
					case REMAP_TOUCH_ZONE_BL: 		tz = TZ_BL[port]; break;
					case REMAP_TOUCH_ZONE_BR: 		tz = TZ_BR[port]; break;
					case REMAP_TOUCH_CUSTOM:  		tz = profile.remaps[i].trigger.param.tPoints; break;
					default: break;
				}
				if (reportInZone(&pData->report[j], tz))
					removeReport(pData, j);
				else 
					j++;
			}
		}
	}

	if (profile.entries[PR_TO_SWAP].v.b)
		port = !port;
	
	gui_updateEmulatedTouch(port, et[port], *pData);

	if (!newEmulatedTouchBuffer[hookId][port]){//New touchbuffer not ready - using previous one
		addVirtualTouches(pData, &etPrev[port], MULTITOUCH_FRONT_NUM, port);
		return;
	}
	
	addVirtualTouches(pData, &et[port], MULTITOUCH_FRONT_NUM, port);
	etPrev[port] = et[port];
	cleanEmuReports(&et[port]);
	newEmulatedTouchBuffer[hookId][port] = false;
}

int remap_touch(SceUInt32 port, SceTouchData *pData, SceUInt32 nBufs, uint8_t hookId, 
		SceTouchData **remappedBuffers){
	// If buffer for timestamp is already remapped
	if (pData->timeStamp == cacheTouch[hookId][port].buffers[cacheTouch[hookId][port].num - 1].timeStamp){
		*remappedBuffers = &cacheTouch[hookId][port].buffers[cacheTouch[hookId][port].num - nBufs];
		return nBufs;
	}

	// If buffer full - remove latest entry
	if (cacheTouch[hookId][port].num >= BUFFERS_NUM){
		for (int i = 1; i < BUFFERS_NUM; i++)
			cacheTouch[hookId][port].buffers[i - 1] = cacheTouch[hookId][port].buffers[i];
		cacheTouch[hookId][port].num--;
	}
	
	// Add curr buffer to cache
	int idx = cacheTouch[hookId][port].num;
	cacheTouch[hookId][port].num++;
	cacheTouch[hookId][port].buffers[idx] = pData[0];
	
	// Updating latest buffer with simulated touches
	updateTouchInfo(port, hookId, &cacheTouch[hookId][port].buffers[idx]);
	
	// Limit returned buufers num with what we have stored
	nBufs = min(nBufs, cacheTouch[hookId][port].num);

	*remappedBuffers = &cacheTouch[hookId][port].buffers[idx + 1 - nBufs];
	return nBufs;
}

int remap_touchRegion(SceUInt32 port, SceTouchData *pData, SceUInt32 nBufs, uint8_t hookId){
	// Do not use buffering for regions
	updateTouchInfo(port, hookId, pData);
	return nBufs;
}

void remap_resetBuffers(){
	for (int i = 0; i < PORTS_NUM; i++){
		for (int j = 0; j < PROC_NUM; j++){
			cacheCtrl[i][j].num = 0;
			for (int k = 0; k < REMAP_NUM; k++)
				rs[i][j][k] = RS_NONACTIVE;
		}
	}
	for (int i = 0; i < SCE_TOUCH_PORT_MAX_NUM; i++){
		for (int j = 0; j < TOUCH_HOOKS_NUM; j++){
			cacheTouch[j][i].num = 0;
		}
		et[i].num = 0;
		gui_updateEmulatedTouch(i, et[i], (SceTouchData){.reportNum = 0});
	}
	
}

void initTouchParams(){
	for (int port = 0; port < SCE_TOUCH_PORT_MAX_NUM; port++){
		//Calculate predefined touchpoints
		T_CENTER[port] = (TouchPoint){
			T_SIZE[port].a.x + (T_SIZE[port].b.x - T_SIZE[port].a.x) / 2,
			T_SIZE[port].a.y + (T_SIZE[port].b.y - T_SIZE[port].a.y) / 2};
		T_L[port] = (TouchPoint){
			T_SIZE[port].a.x + (T_SIZE[port].b.x - T_SIZE[port].a.x) * 2 / 6,
			T_SIZE[port].a.y + (T_SIZE[port].b.y - T_SIZE[port].a.y) / 2};
		T_R[port] = (TouchPoint){
			T_SIZE[port].a.x + (T_SIZE[port].b.x - T_SIZE[port].a.x) * 5 / 6,
			T_SIZE[port].a.y + (T_SIZE[port].b.y - T_SIZE[port].a.y) / 2};
		T_TL[port] = (TouchPoint){
			T_SIZE[port].a.x + (T_SIZE[port].b.x - T_SIZE[port].a.x) * 2 / 6, 
			T_SIZE[port].a.y + (T_SIZE[port].b.y - T_SIZE[port].a.y) / 4};
		T_TR[port] = (TouchPoint){
			T_SIZE[port].a.x + (T_SIZE[port].b.x - T_SIZE[port].a.x) * 4 / 6, 
			T_SIZE[port].a.y + (T_SIZE[port].b.y - T_SIZE[port].a.y) / 4};
		T_BL[port] = (TouchPoint){
			T_SIZE[port].a.x + (T_SIZE[port].b.x - T_SIZE[port].a.x) * 2 / 6, 
			T_SIZE[port].a.y + (T_SIZE[port].b.y - T_SIZE[port].a.y) * 3 / 4};
		T_BR[port] = (TouchPoint){
			T_SIZE[port].a.x + (T_SIZE[port].b.x - T_SIZE[port].a.x) * 4 / 6, 
			T_SIZE[port].a.y + (T_SIZE[port].b.y - T_SIZE[port].a.y) * 3 / 4};

		//Calculate predefined touchzones
		TZ_CENTER[port] = (TouchPoints2){
			(TouchPoint){
				T_SIZE[port].a.x + (T_SIZE[port].b.x - T_SIZE[port].a.x) / 4, 
				T_SIZE[port].a.y + (T_SIZE[port].b.y - T_SIZE[port].a.y) / 4}, 
			(TouchPoint){
				T_SIZE[port].a.x + (T_SIZE[port].b.x - T_SIZE[port].a.x) * 3 / 4, 
				T_SIZE[port].a.y + (T_SIZE[port].b.y - T_SIZE[port].a.y) * 3 / 4}};
		TZ_L[port] = (TouchPoints2){
			(TouchPoint){
				T_SIZE[port].a.x, 
				T_SIZE[port].a.y}, 
			(TouchPoint){
				T_SIZE[port].a.x + (T_SIZE[port].b.x - T_SIZE[port].a.x) / 2, 
				T_SIZE[port].a.y + (T_SIZE[port].b.y - T_SIZE[port].a.y)}};
		TZ_R[port] = (TouchPoints2){
			(TouchPoint){
				T_SIZE[port].a.x + (T_SIZE[port].b.x - T_SIZE[port].a.x) / 2, 
				T_SIZE[port].a.y}, 
			(TouchPoint){
				T_SIZE[port].a.x + (T_SIZE[port].b.x - T_SIZE[port].a.x), 
				T_SIZE[port].a.y + (T_SIZE[port].b.y - T_SIZE[port].a.y)}};
		TZ_TL[port] = (TouchPoints2){
			(TouchPoint){0, 0}, 
			(TouchPoint){
				T_SIZE[port].a.x + (T_SIZE[port].b.x - T_SIZE[port].a.x) / 2, 
				T_SIZE[port].a.y + (T_SIZE[port].b.y - T_SIZE[port].a.y) / 2}};
		TZ_TR[port] = (TouchPoints2){
			(TouchPoint){
				T_SIZE[port].a.x + (T_SIZE[port].b.x - T_SIZE[port].a.x) / 2, 
				T_SIZE[port].a.y}, 
			(TouchPoint){
				T_SIZE[port].a.x + (T_SIZE[port].b.x - T_SIZE[port].a.x), 
				T_SIZE[port].a.y + (T_SIZE[port].b.y - T_SIZE[port].a.y) / 2}};
		TZ_BL[port] = (TouchPoints2){
			(TouchPoint){
				T_SIZE[port].a.x, 
				T_SIZE[port].a.y + (T_SIZE[port].b.y - T_SIZE[port].a.y) / 2}, 
			(TouchPoint){
				T_SIZE[port].a.x + (T_SIZE[port].b.x - T_SIZE[port].a.x) / 2, 
				T_SIZE[port].a.y + (T_SIZE[port].b.y - T_SIZE[port].a.y)}};
		TZ_BR[port] = (TouchPoints2){
			(TouchPoint){
				T_SIZE[port].a.x + (T_SIZE[port].b.x - T_SIZE[port].a.x) / 2, 
				T_SIZE[port].a.y + (T_SIZE[port].b.y - T_SIZE[port].a.y) / 2}, 
			(TouchPoint){
				T_SIZE[port].a.x + (T_SIZE[port].b.x - T_SIZE[port].a.x), 
				T_SIZE[port].a.y + (T_SIZE[port].b.y - T_SIZE[port].a.y)}};
	}
}

void remap_setup(){
	// Enabling analogs sampling 
	if (profile.entries[PR_AN_MODE_WIDE].v.b){
		ksceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG_WIDE);
		ksceCtrlSetSamplingModeExt(SCE_CTRL_MODE_ANALOG_WIDE);
	} else {
		ksceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG);
		ksceCtrlSetSamplingModeExt(SCE_CTRL_MODE_ANALOG);
	}
	
	// Enabling both touch panels sampling
	ksceTouchSetSamplingState(SCE_TOUCH_PORT_FRONT, SCE_TOUCH_SAMPLING_STATE_START);
	ksceTouchSetSamplingState(SCE_TOUCH_PORT_BACK, SCE_TOUCH_SAMPLING_STATE_START);
	
	// Detecting touch panels size
	SceTouchPanelInfo pi;
	
	for (int port = 0; port < SCE_TOUCH_PORT_MAX_NUM; port++){
		if (ksceTouchGetPanelInfo(port, &pi) == 0){
			T_SIZE[port].a.x = pi.minAaX;
			T_SIZE[port].a.y = pi.minAaY;
			T_SIZE[port].b.x = pi.maxAaX;
			T_SIZE[port].b.y = pi.maxAaY;
			initTouchParams();
		}
	}
}

void remap_init(){
	//Allocating mem for stored buffers
	remap_memId = ksceKernelAllocMemBlock("remapsv2_bufs_remap", 
        SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_RW, MEM_SIZE, NULL);
    ksceKernelGetMemBlockBase(remap_memId, (void**)&remap_memBase);
    LOG("MEMORY ALLOC remap cacheCtrl %i\n", CACHE_CTRL_SIZE);
    LOG("MEMORY ALLOC remap cacheTouch %i\n", CACHE_TOUCH_SIZE);
	for (int i = 0; i < PORTS_NUM; i++){
		for (int j = 0; j < PROC_NUM; j++){
			cacheCtrl[i][j].buffers = (SceCtrlData*)(remap_memBase + 
				sizeof(SceCtrlData) * BUFFERS_NUM * (i * PROC_NUM + j));
		}
	}
	for (int i = 0; i < TOUCH_HOOKS_NUM; i++){
    	for (int j = 0; j < SCE_TOUCH_PORT_MAX_NUM; j++){
			cacheTouch[i][j].buffers = (SceTouchData*)(remap_memBase + CACHE_CTRL_SIZE + 
				sizeof(SceTouchData) * BUFFERS_NUM * (i * SCE_TOUCH_PORT_MAX_NUM + j));
		}
	}
	
	for (int i = 0; i < TOUCH_HOOKS_NUM; i++)
		for (int j = 0; j < SCE_TOUCH_PORT_MAX_NUM; j++)
			newEmulatedTouchBuffer[i][j] = false;
	remap_resetBuffers();
	T_SIZE[SCE_TOUCH_PORT_FRONT] = (TouchPoints2){
		(TouchPoint){0, 0},
		(TouchPoint){1919, 1087}
	};
	T_SIZE[SCE_TOUCH_PORT_BACK]  = (TouchPoints2){
		(TouchPoint){0, 108},
		(TouchPoint){1919, 889}
	};
	initTouchParams();
}

void remap_destroy(){
    //Free mem
	ksceKernelFreeMemBlock(remap_memId);
}
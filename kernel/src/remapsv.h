#ifndef _REMAPSV_H_
#define _REMAPSV_H_

#include "profile.h"

/*export*/ int remaPSV2k_onTouch(SceUInt32 port, SceTouchData *pData, SceUInt32 nBufs, uint8_t hookId);

/*export*/ extern void remaPSV2k_userPluginReady();
/*export*/ extern int remaPSV2k_getProfileVersion();
/*export*/ extern void remaPSV2k_getProfile(Profile* p);
/*export*/ extern void remaPSV2k_setSceMotionState(SceMotionState *pData, int r);

#endif
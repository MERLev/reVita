#ifndef _REMAPSV_H_
#define _REMAPSV_H_

#include "fio/profile.h"
#include "fio/settings.h"

/*export*/ extern void remaPSV2k_userPluginReady();
/*export*/ extern int remaPSV2k_getProfileVersion();
/*export*/ extern void remaPSV2k_getProfile(Profile* p);
/*export*/ extern void remaPSV2k_setSceMotionState(SceMotionState *pData, int r);

#endif
#ifndef _REMAPSV_H_
#define _REMAPSV_H_

#include "fio/profile.h"
#include "fio/settings.h"

/*export*/ extern void reVita_userPluginReady();
/*export*/ extern int reVita_getProfileVersion();
/*export*/ extern void reVita_getProfile(Profile* p);
/*export*/ extern void reVita_setSceMotionState(SceMotionState *pData, int r);

#endif
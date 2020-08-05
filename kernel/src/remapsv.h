#ifndef _REMAPSV_H_
#define _REMAPSV_H_

/*export*/ extern void remaPSV2k_userPluginReady();
/*export*/ extern int remaPSV2k_getConfigVersion();
/*export*/ extern bool remaPSV2k_getConfigDeadband();
/*export*/ extern bool remaPSV2k_getConfigPatchExtEnabled();
/*export*/ extern int remaPSV2k_getConfigPatchExtPort();
/*export*/ extern void remaPSV2k_setSceMotionState(SceMotionState *pData, int r);

#endif
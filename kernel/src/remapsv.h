#ifndef _REMAPSV_H_
#define _REMAPSV_H_

/*export*/ int remaPSV2k_onCtrlPeekPositive(int port, SceCtrlData *ctrl, int nBufs);
/*export*/ int remaPSV2k_onCtrlReadPositive(int port, SceCtrlData *ctrl, int nBufs);
/*export*/ int remaPSV2k_onCtrlPeekNegative(int port, SceCtrlData *ctrl, int nBufs);
/*export*/ int remaPSV2k_onCtrlReadNegative(int port, SceCtrlData *ctrl, int nBufs);

/*export*/ extern void remaPSV2k_userPluginReady();
/*export*/ extern int remaPSV2k_getConfigVersion();
/*export*/ extern bool remaPSV2k_getConfigDeadband();
/*export*/ extern bool remaPSV2k_getConfigPatchExtEnabled();
/*export*/ extern int remaPSV2k_getConfigPatchExtPort();
/*export*/ extern void remaPSV2k_setSceMotionState(SceMotionState *pData, int r);

#endif
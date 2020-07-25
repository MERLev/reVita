#ifndef _VITASDKEXT_H_
#define _VITASDKEXT_H_

typedef void* SceClibMspace;
int ksceTouchPeek (SceUInt32 port, SceTouchData *pData, SceUInt32 nBufs);
int ksceTouchSetSamplingState(SceUInt32 port, SceTouchSamplingState state);
int ksceTouchGetPanelInfo(SceUInt32 port, SceTouchPanelInfo *pPanelInfo);

#endif
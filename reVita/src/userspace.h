#ifndef _USERSPACE_H_
#define _USERSPACE_H_

int __sceMotionGetState(SceMotionState *pData);
void __sceMotionReset();

void userspace_init();
void userspace_destroy();

#endif
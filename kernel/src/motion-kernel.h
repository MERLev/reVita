#ifndef _MOTION_KERNEL_H_
#define _MOTION_KERNEL_H_

int ksceMotionGetState(SceMotionState *motionState);
int ksceMotionGetSensorState(SceMotionSensorState *sensorState, int numRecords);
int ksceMotionGetBasicOrientation(SceFVector3 *basicOrientation);
int ksceMotionRotateYaw(float radians);
int ksceMotionGetTiltCorrection(void);
int ksceMotionSetTiltCorrection(int setValue);
int ksceMotionGetDeadband(void);
int ksceMotionSetDeadband(int setValue);
int ksceMotionSetAngleThreshold(float angle);
float ksceMotionGetAngleThreshold(void);
int ksceMotionReset(void);
int ksceMotionMagnetometerOn(void);
int ksceMotionMagnetometerOff(void);
int ksceMotionGetMagnetometerState(void);
int ksceMotionStartSampling(void);
int ksceMotionStopSampling(void);

void motion_init();
void motion_destroy();
#endif
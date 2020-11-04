#ifndef DSMotionLibrary_H
#define DSMotionLibrary_H

int dsMotionStartSampling();
int dsMotionGetState(SceMotionState *ms);
int dsMotionGetSensorState(SceMotionSensorState *sensorState, int numRecords);

#endif

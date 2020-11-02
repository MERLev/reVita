#ifndef _ACTIONS_H_
#define _ACTIONS_H_

extern int brightnessLevel;

void sysactions_softReset();
void sysactions_coldReset();
void sysactions_killCurrentApp();
void sysactions_standby();
void sysactions_suspend();
void sysactions_displayOff();
void sysactions_brightnessInc();
void sysactions_brightnessDec();
void sysactions_saveBackup();
void sysactions_saveRestore();

void sysactions_init();

#endif
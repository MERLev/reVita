#ifndef DS4VITA_H
#define DS4VITA_H

typedef enum DS4_PORT0_MERGE_MODE{
    DS4_MERGE_DISABLED = 0,
    DS4_MERGE_ENABLED = 1
}DS4_PORT0_MERGE_MODE;

/*export*/ int ds4vita_getPort();
/*export*/ void ds4vita_setPort(int port);
/*export*/ enum DS4_PORT0_MERGE_MODE ds4vita_getPort0MergeMode();
/*export*/ void ds4vita_setPort0MergeMode(enum DS4_PORT0_MERGE_MODE mode);

#endif
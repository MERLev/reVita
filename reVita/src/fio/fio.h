#ifndef _FIO_H_
#define _FIO_H_

bool fio_exist(const char *path);
bool fio_readFile(char* buff, int size, char* path, char* name, char* ext);
bool fio_writeFile(char* buff, int size, char* path, char* name, char* ext);
bool fio_deleteFile(char* path, char* name, char* ext);
int fio_delete(char* path);
int fio_deletePath(const char *path);
bool fio_copyFile(char *src, char *dest);
bool fio_copyDir(char *src, char *dest);

#endif
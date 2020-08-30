#ifndef _FIO_H_
#define _FIO_H_

bool fio_readFile(char* buff, int size, char* path, char* name, char* ext);
bool fio_writeFile(char* buff, int size, char* path, char* name, char* ext);
bool fio_deleteFile(char* path, char* name, char* ext);

#endif
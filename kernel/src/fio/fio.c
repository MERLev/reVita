#include <vitasdkkern.h>
#include <taihen.h>
#include <stdbool.h>
#include <stdio.h>

bool fio_readFile(char* buff, int size, char* path, char* name, char* ext){
	char fname[128];
	sprintf(fname, "%s/%s.%s", path, name, ext);
	SceUID fd;
	fd = ksceIoOpen(fname, SCE_O_RDONLY, 0777);
	if (fd < 0)
		return false;
	ksceIoRead(fd, buff, size);
	if (ksceIoClose(fd) < 0)
		return false;
	
	return true;
}
bool fio_writeFile(char* buff, int size, char* path, char* name, char* ext){
	//Create dir if not exists
	ksceIoMkdir(path, 0777); 

    char fname[128];
	sprintf(fname, "%s/%s.%s", path, name, ext);
	SceUID fd = ksceIoOpen(fname, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
	if (fd < 0)
		return false;
	ksceIoWrite(fd, buff, size);
	if (ksceIoClose(fd) < 0)
		return false;

	return true;
}
bool fio_deleteFile(char* path, char* name, char* ext){
	char fname[128];
	sprintf(fname, "%s/%s.%s", path, name, ext);
	if (ksceIoRemove(fname) >= 0)
		return true;
	return false;
}
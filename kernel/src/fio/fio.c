#include <vitasdkkern.h>
#include <taihen.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>

#define SCE_ERROR_ERRNO_ENOENT 0x80010002
#define SCE_ERROR_ERRNO_EEXIST 0x80010011
#define SCE_ERROR_ERRNO_ENODEV 0x80010013

char *save_blacklist[] = {
    "sce_pfs/",
    "sce_sys/safemem.dat",
    "sce_sys/keystone",
    "sce_sys/param.sfo",
    "sce_sys/sealedkey",
    NULL,
};

bool isBlacklisted(char* path){
    int i = 0;
    while (save_blacklist[i]) {
        if (strstr(path, save_blacklist[i])) {
            return true;
        }
        i += 1;
    }
	return false;
}

bool fio_exist(const char *path) {
    SceIoStat stat = {0};
    int ret = ksceIoGetstat(path, &stat);
    return ret != SCE_ERROR_ERRNO_ENOENT && ret != SCE_ERROR_ERRNO_ENODEV;
}

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

bool fio_copyFile(char *src, char *dest) {
    if (strcmp(src, dest) == 0 
			|| isBlacklisted(src) 
			|| isBlacklisted(dest))
		return -1;

    int ignore_error = strncmp(dest, "savedata0:", 10) == 0;

    SceUID fdsrc = ksceIoOpen(src, SCE_O_RDONLY, 0);
    if (!ignore_error && fdsrc < 0) {
        return fdsrc;
    }

    int size = ksceIoLseek(fdsrc, 0, SEEK_END);
    ksceIoLseek(fdsrc, 0, SEEK_SET);
    char buf[size];
    ksceIoRead(fdsrc, buf, size);
    ksceIoClose(fdsrc);

    SceUID fddst = ksceIoOpen(dest, SCE_O_WRONLY | SCE_O_CREAT, 0777);
    if (!ignore_error && fddst < 0) {
        return fddst;
    }
    ksceIoWrite(fddst, buf, size);
    ksceIoClose(fddst);

    return 1;
}

bool fio_copyDir(char *src, char *dest) {
    if (strcmp(src, dest) == 0 
			|| isBlacklisted(src) 
			|| isBlacklisted(dest))
		return -1;

    SceUID dfd = ksceIoDopen(src);

    if (!fio_exist(dest)) {
        int ret = ksceIoMkdir(dest, 0777);
        if (ret < 0 && ret != SCE_ERROR_ERRNO_EEXIST) {
            ksceIoDclose(dfd);
            return ret;
        }
    }

    int res = 0;

    do {
        SceIoDirent dir;
        memset(&dir, 0, sizeof(SceIoDirent));

        res = ksceIoDread(dfd, &dir);
        if (res > 0) {
            if (strcmp(dir.d_name, ".") == 0 || strcmp(dir.d_name, "..") == 0)
                continue;

            char new_src[strlen(src) + strlen(dir.d_name) + 2];
            snprintf(new_src, 1024, "%s/%s", src, dir.d_name);

            char new_dest[strlen(dest) + strlen(dir.d_name) + 2];
            snprintf(new_dest, 1024, "%s/%s", dest, dir.d_name);

            int ret = 0;

            if (SCE_S_ISDIR(dir.d_stat.st_mode)) {
                ret = fio_copyDir(new_src, new_dest);
            } else {
                ret = fio_copyFile(new_src, new_dest);
            }

            if (ret <= 0) {
                ksceIoDclose(dfd);
                return ret;
            }
        }
    } while (res > 0);

    ksceIoDclose(dfd);
    return 1;
}
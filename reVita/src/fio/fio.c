#include <vitasdkkern.h>
#include <taihen.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include "../log.h"

#define TRANSFER_SIZE 		 (128 * 1024)
#define MAX_PATH_SIZE 		 		(512)
#define ERROR_BLACKLISTED	   0x90010001
#define ERROR_SUBFOLDER	       0x90010002
#define SCE_ERROR_ERRNO_ENOENT 0x80010002
#define SCE_ERROR_ERRNO_EEXIST 0x80010011
#define SCE_ERROR_ERRNO_ENODEV 0x80010013

char *save_blacklist[] = {
    "sce_pfs/",
    "sce_sys/safemem.dat",
    "sce_sys/keystone",
    // "sce_sys/param.sfo",
    "sce_sys/sealedkey",
    NULL,
};

bool isBlacklisted(char* path){
    int i = 0;
    while (save_blacklist[i]) {
        if (strstr(path, save_blacklist[i])) {
			LOG(" Blacklisted: '%s'\n", path);
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
int fio_delete(char* path){
	return ksceIoRemove(path);
}

int fio_copyFile(char *src, char *dest){
	int ret = 0;

	// Check if blacklisted
    if (strcmp(src, dest) == 0 
			|| isBlacklisted(src) 
			|| isBlacklisted(dest))
		return ERROR_BLACKLISTED;

	// The destination is a subfolder of the source folder
	int len = strlen(src);
	if (strcmp(src, dest) == 0 
			&& (dest[len] == '/' || dest[len - 1] == '/')){
		return ERROR_SUBFOLDER;
	}

	// Open both files
	SceUID fdsrc = ksceIoOpen(src, SCE_O_RDONLY, 0);
	if (fdsrc < 0)
		return fdsrc;

	SceUID fddst = ksceIoOpen(dest, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
	if (fddst < 0){
		ksceIoClose(fdsrc);
		return fddst;
	}

	//Mem allocation for buffer
	char* buff;
	SceUID buff_uid  = ksceKernelAllocMemBlock("RemaPSV2_filecopy", 
		SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_RW, (TRANSFER_SIZE + 0xfff) & ~0xfff, NULL);
	if (buff_uid < 0){
		ret = buff_uid;
		goto ERROR_IO;
	}
    ksceKernelGetMemBlockBase(buff_uid, (void**)&buff);

	// Copy file using buffer
	while (true){
		int read = ksceIoRead(fdsrc, buff, TRANSFER_SIZE);
		if (read < 0){
			ret = read;
			goto ERROR;
		}
		if (read == 0)
			break;

		int written = ksceIoWrite(fddst, buff, read);
		if (written < 0){
			ret = written;
			goto ERROR;
		}
	}

	// Inherit file stat
	SceIoStat stat;
	memset(&stat, 0, sizeof(SceIoStat));
	ksceIoGetstatByFd(fdsrc, &stat);
	ksceIoChstatByFd(fddst, &stat, 0x3B);

ERROR_IO:
	// Close / Clean IO
	ksceIoClose(fddst);
	ksceIoClose(fdsrc);
	if (ret < 0)
		ksceIoRemove(dest);

ERROR: 
	// Free allocated memory
	ksceKernelFreeMemBlock(buff_uid);

    return ret;
}

int fio_copyDir(char *src, char *dest) {
    if (strcmp(src, dest) == 0 
			|| isBlacklisted(src) 
			|| isBlacklisted(dest))
		return 0;
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

            if (ret < 0 && ret != ERROR_BLACKLISTED) {
                ksceIoDclose(dfd);
                return ret;
            }
        }
    } while (res > 0);

    ksceIoDclose(dfd);
    return 0;
}

int fio_deletePath(const char *path){
	SceUID dfd = ksceIoDopen(path);
	if (dfd >= 0) {
		int res = 0;

		char new_path[MAX_PATH_SIZE];
		do {
			SceIoDirent dir;
			memset(&dir, 0, sizeof(SceIoDirent));

			res = ksceIoDread(dfd, &dir);
			if (res > 0) {
				new_path[0] = '\0';
				snprintf(new_path, MAX_PATH_SIZE, "%s%s%s", path, (path[strlen(path) - 1] == '/') ? "" : "/", dir.d_name);

				if (SCE_S_ISDIR(dir.d_stat.st_mode)) {
					int ret = fio_deletePath(new_path);
					if (ret <= 0) {
						ksceIoDclose(dfd);
						return ret;
					}
				} else {
					int ret = ksceIoRemove(new_path);
					if (ret < 0) {
						ksceIoDclose(dfd);
						return ret;
					}
				}
			}
		} while (res > 0);

		ksceIoDclose(dfd);

		int ret = ksceIoRmdir(path);
		if (ret < 0)
			return ret;
	} else {
		int ret = ksceIoRemove(path);
		if (ret < 0)
			return ret;
	}

	return 1;
}
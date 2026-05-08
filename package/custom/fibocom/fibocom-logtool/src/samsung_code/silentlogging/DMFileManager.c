#include "DMFileManager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/statvfs.h>
#include "log.h"
#include <errno.h>

static struct DMFileManager *sInstance = NULL;
#define DISK_USED_THRESHHOLD 85   //remain disk persen %
//TODO TEST
#define MANAGED_FILE_COUNT_THRESHHOLD 1000
//TODO END
#define MEGABYTE                (1024 * 1024)

static const char *IGNORE_FILE_LIST[] = {
    "NNEXT_PROFILE.nprf",
    "sbuff_profile.sdm",
    ".sbuff_header.sdm",
};

static const char *MANAGED_EXT[] = {
    ".sdm", ".gz", ".zip"
};

int isIgnoreFile(char* fileName) {
    unsigned int size = sizeof(IGNORE_FILE_LIST) / sizeof(IGNORE_FILE_LIST[0]);
    unsigned int i;
    for (i = 0; i < size; i++) {
        char* filter = IGNORE_FILE_LIST[i];
        if (strcmp(fileName, filter) == 0) {
            //ALOGD("matched in ignore file list. %s", filter);
            return 1;
        }
    } // end for i ~
    return 0;
}

int isManagedExt(char* fileName) {
    unsigned int size = sizeof(MANAGED_EXT) / sizeof(MANAGED_EXT[0]);
    unsigned int i;
    for (i = 0; i < size; i++) {
        char* ext = MANAGED_EXT[i];
        size_t pos = strlen(fileName) - strlen(ext);
        if (pos >= 0 && strcmp(fileName + pos, ext) == 0) {
            //ALOGD("file ext is matched with %s", ext);
            return 1;
        }
    } // end for i ~
    return 0;
}

struct DMFileManager* DMFileManager_getInstance() {
    if (sInstance == NULL) {
        sInstance = (struct DMFileManager*)malloc(sizeof(struct DMFileManager));
        if (sInstance != NULL) {
            sInstance->mFileList = NULL;
            sInstance->mCapacity = 0;
            sInstance->mManagedFileSize = 0;
            sInstance->mTotalSize = 0;
            sInstance->mMaxSize = 0;

            int capacity = -1;
            int managedFileSize = -1;
            DMFileManager_setLimit(sInstance, (unsigned int)capacity, (unsigned int)managedFileSize);

            char buf[128] = {0};
            DMFileManager_setBaseDir(sInstance, buf);
        }
    }
    return sInstance;
}

void DMFileManager_init(struct DMFileManager *manager) {
    manager->mFileList = NULL;
    manager->mCapacity = 0;
    manager->mManagedFileSize = 0;
    strcpy(manager->mBaseDir, "");
    manager->mTotalSize = 0;
    manager->mMaxSize = 0;
}

void DMFileManager_add(struct DMFileManager *manager, const char *fileName) {
    if (fileName != NULL && *fileName != 0) {
        struct DMFile dmFile;
        strcpy(dmFile.fileName, fileName);

        char filePath[512];
        strcpy(filePath, manager->mBaseDir);
        strcat(filePath, fileName);

        FILE *file = fopen(filePath, "rb");
        if (file == NULL) {
            LOG_PRINT_W("Cannot open file: %s\n", filePath);
            return;
        }

        fseek(file, 0, SEEK_END);
        dmFile.fileSize = ftell(file);
        fclose(file);

        manager->mTotalSize += dmFile.fileSize;
        LOG_PRINT_D("DMFileManager::%s fileName=%s fileSize=%d\n", __FUNCTION__, dmFile.fileName, dmFile.fileSize);

        // Add dmFile to mFileList
        struct Node* newNode = (struct Node*)malloc(sizeof(struct Node));
        newNode->data = dmFile;
        newNode->next = NULL;

        if (manager->mFileList == NULL) {
            manager->mFileList = newNode;
        } else {
            struct Node* current = manager->mFileList;
            while (current->next != NULL) {
                current = current->next;
            }
            current->next = newNode;
        }

        DMFileManager_shrink(manager);
    }
    else {
        //ALOGW("Invalid fileName.");
        LOG_PRINT_W("Invalid fileName.");
    }
    
}

void DMFileManager_removeAll(struct DMFileManager *manager) {
    LOG_PRINT_D("Removing all files...\n");
    struct Node* current = manager->mFileList;
    while (current != NULL) {
        struct DMFile dmFile = current->data;
        char path[512];
        strcpy(path, manager->mBaseDir);
        strcat(path, dmFile.fileName);

        if (remove(path) != 0) {
            LOG_PRINT_W("Failed to delete file %s.\n", dmFile.fileName);
        } else {
            LOG_PRINT_D("Remove file %s\n", dmFile.fileName);
            manager->mTotalSize += dmFile.fileSize;
        }

        struct Node* temp = current;
        current = current->next;
        free(temp);
    }

    manager->mFileList = NULL;
    manager->mTotalSize = 0;
}

void DMFileManager_refreshFileList(struct DMFileManager *manager) {
    manager->mTotalSize = 0;
    manager->mFileList = NULL;
    DMFileManager_createManagedFileList(manager);
}

int getListSize(struct Node* head) {
    size_t remainingFiles = 0;
    struct Node* current = head;
    while (current != NULL) {
        remainingFiles++;
        current = current->next;
    }
    return remainingFiles;
}

void DMFileManager_shrink(struct DMFileManager *manager) {
    while (manager->mFileList != NULL && manager->mFileList->next != NULL) {
        struct statvfs sb;
        //TODO Calculate user config logPath remain size
        if (statvfs(manager->mBaseDir, &sb) == 0) {
        //TODO END
            char buf[128] = {0};
            // MegaByte
            // property_get("persist.vendor.sys.diag.log.maxsize", buf, "0");
            manager->mMaxSize = atoi(buf);
            if (manager->mMaxSize == 0) {
                // default max size : 85% of Total storage(/log)
                manager->mMaxSize = sb.f_blocks * sb.f_bsize * manager->mCapacity / 100 / MEGABYTE;
            }
            unsigned int size = manager->mTotalSize / MEGABYTE;
            LOG_PRINT_D("mMaxSize=%dMB TotalSize=%dMB\n", manager->mMaxSize, size);
            if (size < manager->mMaxSize) {
                break;
            }
            struct DMFile dmFile = manager->mFileList->data;
            struct Node* temp = manager->mFileList;
            manager->mFileList = manager->mFileList->next;
            free(temp);
            char path[512];
            strcpy(path, manager->mBaseDir);
            strcat(path, dmFile.fileName);
            if (unlink(path) < 0) {
                LOG_PRINT_W("failed to delete file %s.\n", path);
                return;
            }
            manager->mTotalSize -= dmFile.fileSize;
            LOG_PRINT_D("----Remove old file %s\n", dmFile.fileName);
        }
    }

    while (manager->mManagedFileSize > 0 && getListSize(manager->mFileList) > manager->mManagedFileSize) {
        struct DMFile dmFile = manager->mFileList->data;
        struct Node* temp = manager->mFileList;
        manager->mFileList = manager->mFileList->next;
        free(temp);
        char path[512];
        strcpy(path, manager->mBaseDir);
        strcat(path, dmFile.fileName);
        if (unlink(path) < 0) {
            if (errno == ENOENT) {
                LOG_PRINT_W("Old file %s have been remove by others! \n", dmFile.fileName);
            } else {
                LOG_PRINT_E("failed to delete file %s. errno = %d \n", dmFile.fileName, errno);
                return;
            }
        }
        manager->mTotalSize -= dmFile.fileSize;
        LOG_PRINT_D("Remove old file %s\n", dmFile.fileName);
    }

    LOG_PRINT_D("Remained managed file size: %lu\n", getListSize(manager->mFileList));
}

void DMFileManager_setLimit(struct DMFileManager *manager, unsigned int capacity, unsigned int managedFileSize) {
    manager->mCapacity = capacity;
    if (manager->mCapacity == 0 || manager->mCapacity > DISK_USED_THRESHHOLD)
        manager->mCapacity = DISK_USED_THRESHHOLD;

    manager->mManagedFileSize = managedFileSize;
    if (manager->mManagedFileSize == 0 || manager->mManagedFileSize > MANAGED_FILE_COUNT_THRESHHOLD)
        manager->mManagedFileSize = MANAGED_FILE_COUNT_THRESHHOLD;

    //ALOGD("setLimit; capacity:%d managedFileSize:%d", mCapacity, mManagedFileSize);
    LOG_PRINT_D("setLimit; capacity:%d managedFileSize:%d", manager->mCapacity, manager->mManagedFileSize);
}

void DMFileManager_setBaseDir(struct DMFileManager *manager, const char *baseDir) {
    if (baseDir != NULL && *baseDir != 0) {
        //manager->mBaseDir = baseDir;
        strcpy(manager->mBaseDir, baseDir);
        //TODO
        // if (manager->mBaseDir.back() != '/') {
        //     manager->mBaseDir += "/";
        // }
    }
}

void DMFileManager_createManagedFileList(struct DMFileManager *manager) {
    DIR *dir;
    dir = opendir(manager->mBaseDir);
    if (dir != NULL) {
        manager->mTotalSize = 0;
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_type == DT_REG) {
                char fileName[256];
                strcpy(fileName, entry->d_name);
                if (isManagedExt(fileName) && !isIgnoreFile(fileName)) {
                    //ALOGD("[%lu] %s", mFileList.size(), fileName);
                    struct DMFile dmFile;
                    strcpy(dmFile.fileName, fileName);
                    char filePath[512];
                    sprintf(filePath, "%s%s", manager->mBaseDir, fileName);
                    FILE *file = fopen(filePath, "rb");
                    if (file == NULL) {
                        LOG_PRINT_W("cannot open file(%s)\n", filePath);
                    }
                    fseek(file, 0, SEEK_END);
                    dmFile.fileSize = ftell(file);
                    manager->mTotalSize += dmFile.fileSize;
                    //mFileList.push_back(dmFile);
                    // Add dmFile to mFileList
                    struct Node* newNode = (struct Node*)malloc(sizeof(struct Node));
                    newNode->data = dmFile;
                    newNode->next = NULL;

                    if (manager->mFileList == NULL) {
                        manager->mFileList = newNode;
                    } else {
                        struct Node* current = manager->mFileList;
                        while (current->next != NULL) {
                            current = current->next;
                        }
                        current->next = newNode;
                    }
                    fclose(file);
                }
                else {
                    LOG_PRINT_W("not a managed file. (%s)\n", fileName);
                }
            }
            else {
                //ALOGW("not a regular file (directory or others)");
            }
        } // end while ~
        closedir(dir);
    }
    else {
        //ALOGE("open directory(%s) error. errno=%d", mBaseDir, errno);
    }
}
#ifndef DMFILEMANAGER_H
#define DMFILEMANAGER_H

struct DMFile {
    char fileName[256];
    unsigned int fileSize;
};

struct Node {
    struct DMFile data;
    struct Node* next;
};

struct DMFileManager {
    struct Node *mFileList;
    unsigned int mCapacity;
    unsigned int mManagedFileSize;
    char mBaseDir[256];
    unsigned int mTotalSize;
    unsigned int mMaxSize;
};

struct DMFileManager* DMFileManager_getInstance();
void DMFileManager_init(struct DMFileManager *manager);
void DMFileManager_add(struct DMFileManager *manager, const char *fileName);
void DMFileManager_removeAll(struct DMFileManager *manager);
void DMFileManager_refreshFileList(struct DMFileManager *manager);
void DMFileManager_shrink(struct DMFileManager *manager);
void DMFileManager_setLimit(struct DMFileManager *manager, unsigned int capacity, unsigned int managedFileSize);
void DMFileManager_setBaseDir(struct DMFileManager *manager, const char *baseDir);
void DMFileManager_createManagedFileList(struct DMFileManager *manager);

#endif
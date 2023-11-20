#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bf.h"
#include "hash_file.h"
#define MAX_OPEN_FILES 20

#define CALL_BF(call)       \
{                           \
  BF_ErrorCode code = call; \
  if (code != BF_OK) {      \
    BF_PrintError(code);    \
    return BF_ERROR;        \
  }                         \
}
HT_table_file_entry *table;
int openFileCounter;
 
HT_ErrorCode HT_Init() {
    openFileCounter = 0;
    BF_Init(LRU);
    table = malloc(sizeof(HT_table_file_entry) * MAX_OPEN_FILES);
    if(!table) return HT_ERROR;
    return HT_OK;

}
//look for first open space in table VALVE PLEASE FIX
HT_ErrorCode HT_CreateIndex(const char *filename, int depth) {
    int fileDesc;
    BF_ErrorCode exists = BF_CreateFile(filename);
    if(exists != BF_OK) return HT_ERROR;
    //Create ht_info
    BF_Block *block;
    CALL_BF(BF_OpenFile(filename,&fileDesc));
    BF_Block_Init(&block);

    CALL_BF(BF_AllocateBlock(fileDesc,block));

    HT_info *data = (HT_info*)BF_Block_GetData(block); //Get pointer to beginning of block
    data->globalDepth = depth;
    data->maxRecordsPerBucket = 0;
    data->minRecordsPerBucket = 0;
    data->numOfBuckets = 0;
    data->totalRecords = 0;
    data->type = HASH;


    BF_Block_SetDirty(block);
    BF_UnpinBlock(block);

    CALL_BF(BF_CloseFile(fileDesc));
    BF_Block_Destroy(&block);

    return HT_OK;
}

HT_ErrorCode HT_OpenIndex(const char *fileName, int *indexDesc){
    int fileDesc;
    *indexDesc = openFileCounter;
    BF_OpenFile(fileName,&fileDesc);
    HT_info *info = getInfo(*indexDesc);
    table[openFileCounter].infoBlock = info; // πιθανώς το χειρότερο cast που έχω κάνει.

    openFileCounter++;

    return HT_OK;
}

HT_ErrorCode HT_CloseFile(int indexDesc) {
    BF_Block *block; 
    BF_Block_Init(&block);
    int block_num;
    int fileDesc = table[indexDesc].fileDesc;
    BF_GetBlockCounter(fileDesc, &block_num);
    for(int i = 0; i < block_num; i++) {
        CALL_BF(BF_GetBlock(fileDesc, i, block));
        CALL_BF(BF_UnpinBlock(block));
    }

    BF_Block_Destroy(&block);
    CALL_BF(BF_CloseFile(fileDesc));
    openFileCounter--;

    return HT_OK;
}

HT_ErrorCode HT_InsertEntry(int indexDesc, Record record) {
    
    HT_info *info = getInfo(indexDesc);
    
    int *hashTable;
    int depth = 2;
    
    depth << info->globalDepth;
    if(info->totalRecords == 0)  { //Init table
        hashTable = malloc(sizeof(int) * depth);
    }

    int hashNum = hash_Function(record.id, depth);
    if(!hashTable[hashNum]) {
        //create bucket
    } else {

    }
}

HT_ErrorCode HT_PrintAllEntries(int indexDesc, int *id) {
    //insert code here
    return HT_OK;
}

int hash_Function(int num, int globalDepth) {
    int hashValue = num; //will add actual hash function here
    return hashValue >> sizeof(int) - globalDepth;
}

HT_info *getInfo(int indexDesc) {
    BF_Block *block;

    int fileDesc = table[indexDesc].fileDesc;

    BF_Block_Init(&block);
    BF_GetBlock(fileDesc, 0, block);

    void* Pointer = BF_Block_GetData(block);

    HT_info* info = (HT_info*)Pointer;
    BF_Block_Destroy(&block);

    return info;
}

// depth 3

// [0, 1,  2,   3,  4,  5  ,6,  7]
// 000 001 010 011 100 101 110 111

// for(i ... i < globalDepth)
//     shiftright
//     add to var -> go to bucket of choice

// if(!bucket)
//     createbucket.....
// else
//     xwraei?!?!?
//     if not:
//         localdepth < global???
//         split...:
            

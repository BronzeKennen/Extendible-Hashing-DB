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
int openFileCounter = 0;

HT_ErrorCode HT_Init() {

    BF_Init(LRU);
    table = malloc(sizeof(HT_table_file_entry) * MAX_OPEN_FILES);
    if(!table) return HT_ERROR;
    return HT_OK;

}

HT_ErrorCode HT_CreateIndex(const char *filename, int depth) {
    int fileDesc;
    BF_ErrorCode exists = BF_CreateFile(filename);
    if(exists == BF_ERROR) return HT_ERROR;
    //Create ht_info
    BF_Block *block;
    HT_info info = {0, 0, 0, 0, depth };

    CALL_BF(BF_OpenFile(filename,&fileDesc));
    BF_Block_Init(&block);

    CALL_BF(BF_AllocateBlock(fileDesc,block));
    memcpy(block,&info,sizeof(HT_info));
    
    BF_Block_SetDirty(block);
    BF_UnpinBlock(block);

    CALL_BF(BF_CloseFile(fileDesc));
    BF_Block_Destroy(&block);

    return HT_OK;
}

HT_ErrorCode HT_OpenIndex(const char *fileName, int *indexDesc){
    int fileDesc;
    BF_OpenFile(fileName,&fileDesc);

    table[openFileCounter].fileDesc = fileDesc;
    BF_Block *block;
    BF_Block_Init(&block);
    CALL_BF(BF_GetBlock(fileDesc, 1, block));
    HT_info* info = BF_Block_GetData(block);
    table[openFileCounter].infoBlock = info; // πιθανώς το χειρότερο cast που έχω κάνει.
    *indexDesc = openFileCounter;
    openFileCounter++;
    BF_Block_Destroy(&block);
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
    
    return HT_OK;
}

HT_ErrorCode HT_InsertEntry(int indexDesc, Record record) {
    //insert code here
    return HT_OK;
}

HT_ErrorCode HT_PrintAllEntries(int indexDesc, int *id) {
    //insert code here
    return HT_OK;
}


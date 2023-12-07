#include "bf.h"
#include "hash_file.h"
#include "extras.h"
#include <stdlib.h>
#include <stdio.h>

void reHash(int fileDesc, int oldBlockPos, int newBlockPos, int *hashTable, int globalDepth) {
    BF_Block* oldBlock;
    BF_Block* newBlock;

    BF_Block_Init(&oldBlock);
    BF_Block_Init(&newBlock);
    
    BF_GetBlock(fileDesc, oldBlockPos, oldBlock);
    BF_GetBlock(fileDesc, newBlockPos, newBlock);

    HT_block_info *oldInfo = (HT_block_info*)BF_Block_GetData(oldBlock);
    HT_block_info *newInfo = (HT_block_info*)BF_Block_GetData(newBlock);
    
    int prevRecords = oldInfo->numOfRecords;

    oldInfo->localDepth++; //we are rehashing duh
    oldInfo->numOfRecords = 0; //reset num of records
    
    newInfo->localDepth = oldInfo->localDepth; //set new Blockinfo 
    newInfo->numOfRecords = 0;
    
    
    oldInfo++;
    Record *recOld = (Record*)oldInfo; //get pointer to records
    oldInfo--;

    newInfo++;
    Record *recNew = (Record*)newInfo; //get pointer to records
    newInfo--;

    for(int i = 0; i < prevRecords; i++) {
        Record rec = recOld[i];
        
        int hashNum = hash_Function(rec.id);
        // printf("This is the hashNum before MSBs %d\n", hashNum);
        hashNum = getMSBs(hashNum, globalDepth);

        if(hashTable[hashNum] == oldBlockPos) {
            recOld[oldInfo->numOfRecords] = rec;
            oldInfo->numOfRecords++;
        } else if(hashTable[hashNum] == newBlockPos){
            recNew[newInfo->numOfRecords] = rec;
            newInfo->numOfRecords++;
        } else {
            printf("%d | %p | %p\n",hashTable[hashNum],oldBlock,newBlock);
            //edw peftei mono meta apo resize.
            
        }
    }
    BF_Block_SetDirty(oldBlock);
    BF_Block_SetDirty(newBlock);

    BF_UnpinBlock(oldBlock);
    BF_UnpinBlock(newBlock); 

    BF_Block_Destroy(&oldBlock);
    BF_Block_Destroy(&newBlock);
}

int hash_Function(int num) {
    long long num2 = num * 2318589953;
    // printf("hashNum is %d\n", (int)num2);
    return (int)num2;
    //will create a real hashFunc
}


void resizeHashTable(HT_info *info) {
    int* hashTable = info->hashTable;
    // Set sizes for old and new table
    int oldIndexes = 2 << info->globalDepth - 1;
    int newIndexes = 2 << (info->globalDepth); 
    int* newHashTable = malloc(sizeof(int) * newIndexes);

    for(int i = 0; i < newIndexes; i++) {
        newHashTable[i] = -1;
    }
    // New indexes point as pairs to previous blocks
    for(int i = 0; i < oldIndexes; i++) {
        int blockPos = hashTable[i];
        newHashTable[2*i] = blockPos;
        newHashTable[2*i+1] = blockPos;
    }
    // Update HT_info. Need to check if it fits in one block.
    info->globalDepth++;
    free(hashTable);
    info->hashTable = newHashTable;
}

// Gets [MAX_INT_DIGITS - depth] MSBs of a number.
unsigned int getMSBs(unsigned int num, int depth) {
    return num >>= (sizeof(int)*8 - depth);
}
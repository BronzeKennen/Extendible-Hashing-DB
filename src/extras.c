#include "bf.h"
#include "hash_file.h"
#include "extras.h"
#include <stdlib.h>
#include <stdio.h>

void reHash(BF_Block* oldBlock, BF_Block* newBlock, BF_Block **hashTable, int globalDepth) {
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
    Record *recnew = (Record*)newInfo; //get pointer to records
    newInfo--;

    for(int i = 0; i < prevRecords; i++) {
        Record rec = recOld[i];
        
        int hashNum = hash_Function(rec.id);
        hashNum = getMSBs(hashNum, globalDepth);

        if(hashTable[hashNum] == oldBlock) {
            printf("Hashing in old block\n");
            recOld[oldInfo->numOfRecords] = rec;
            oldInfo->numOfRecords++;
        } else if(hashTable[hashNum] == newBlock){
            printf("Hashing in new block\n");
            recnew[newInfo->numOfRecords] = rec;
            newInfo->numOfRecords++;
        } else {
            printf("%p | %p | %p\n",hashTable[hashNum],oldBlock,newBlock);
            //edw peftei panta giati gamiete
        }
    }

}

int hash_Function(int num) {
    return num;
    //will create a real hashFunc
}


void resizeHashTable(HT_info *info) {
    printf("Resizing table...\n");
    BF_Block** hashTable = info->hashTable;
    // Set sizes for old and new table
    int oldIndexes = 2 << info->globalDepth;
    int newIndexes = 2 << (info->globalDepth + 1);
    BF_Block** newHashTable = malloc(sizeof(BF_Block*) * newIndexes);

    // New indexes point as pairs to previous blocks
    for(int i = 0; i < oldIndexes; i++) {
        BF_Block* block = hashTable[i];
        newHashTable[2*i] = block;
        newHashTable[2*i+1] = block;
    }

    // Update HT_info. Need to check if it fits in one block.
    info->globalDepth++;
    free(hashTable);
    info->hashTable = newHashTable;
    printf("Hash table now has total space of %d buckets.\n",(2 << info->globalDepth));
}

unsigned int getMSBs(unsigned int num, int depth) {
    printf("hashed at %d\n",(num >> (sizeof(int)*8 - depth)));
    return num >> (sizeof(int)*8 - depth);
}
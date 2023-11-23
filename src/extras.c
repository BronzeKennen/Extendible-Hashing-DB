#include "bf.h"
#include "hash_file.h"
#include "extras.h"
#include <stdlib.h>
#include <stdio.h>

void reHash(HT_info *info) {

}

int hash_Function(int num) {
    return num;
    //will create a real hashFunc
}


void resizeHashTable(HT_info *info) {
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
}

unsigned int getMSBs(unsigned int num, int depth) {
    printf("hashed at %d",(num >> (sizeof(int)*8 - depth)));
    return num >> (sizeof(int)*8 - depth);
}

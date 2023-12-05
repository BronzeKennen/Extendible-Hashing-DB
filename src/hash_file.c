#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bf.h"
#include "hash_file.h"
#include "extras.h"

#define MAX_OPEN_FILES 20

//Known issue: hashtable[0] is not null on init therefore you cannot tell if there is a bucket or not. Please fix

#define CALL_BF(call)       \
{                           \
  BF_ErrorCode code = call; \
  if (code != BF_OK) {      \
    BF_PrintError(code);    \
    return BF_ERROR;        \
  }                         \
}

#define RECORDS_PER_BLOCK (BF_BLOCK_SIZE - sizeof(HT_block_info)) / sizeof(Record)


// Global Data that keep track of open files
HT_table_file_entry *table;
int openFileCounter;


// Allocate memory for open files table and initialize values as empty.
HT_ErrorCode HT_Init() {
    openFileCounter = 0;
    CALL_BF(BF_Init(LRU));

    // Initialize the table of open files
    table = malloc(sizeof(HT_table_file_entry) * MAX_OPEN_FILES);
    if(!table) return HT_ERROR;
    for(int i=0; i<MAX_OPEN_FILES; i++){
        table[i].fileDesc = -1;
        table[i].fileName = '\0';
    }

    return HT_OK;

}

// Close any open files and free the memory of the open files table.
HT_ErrorCode HT_Destroy() {
        for(int i = 0; i < MAX_OPEN_FILES, openFileCounter > 0; i++) {
            if(table[i].fileDesc != -1) {
                HT_CloseFile(i);
                openFileCounter--;
            }
        } 
    free(table);
    return HT_OK;
}

// Create a new hash file with name = *filename and global depth = depth. 
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
    data->totalRecords = 0;
    data->type = HASH;


    BF_Block_SetDirty(block);
    CALL_BF(BF_UnpinBlock(block));

    CALL_BF(BF_CloseFile(fileDesc));
    BF_Block_Destroy(&block);

    return HT_OK;
}


// Open the file with name = *fileName and insert in open files table. The position of the file is stored in indexDesc.
HT_ErrorCode HT_OpenIndex(const char *fileName, int *indexDesc){
    if(openFileCounter == MAX_OPEN_FILES) {
        return HT_ERROR;
    }
    // Find the next available spot in the open file table and insert the file 
    for(int i=0; i<MAX_OPEN_FILES; i++){
        if(table[i].fileDesc == -1){
            *indexDesc = i;
            break;
        }
    }
    int fileDesc;
    BF_ErrorCode code = BF_OpenFile(fileName,&fileDesc);
    if(code != BF_OK) return HT_ERROR;
    HT_info *info = getInfo(*indexDesc);
    table[*indexDesc].fileDesc = fileDesc;
    table[*indexDesc].fileName = fileName;

    openFileCounter++;
    return HT_OK;
}

HT_ErrorCode HT_CloseFile(int indexDesc) {
    BF_Block *block; 
    int fileDesc = table[indexDesc].fileDesc;
    BF_Block_Init(&block);
    BF_AllocateBlock(fileDesc,block);
    
    
    BF_GetBlock(fileDesc,0,block);
    HT_info* info = (HT_info*)BF_Block_GetData(block);
    


    int block_num;
    BF_GetBlockCounter(fileDesc, &block_num);
    for(int i = 0; i < block_num; i++) {
        BF_Block_SetDirty(block);
        CALL_BF(BF_UnpinBlock(block));
    }

    BF_Block_Destroy(&block);
    CALL_BF(BF_CloseFile(fileDesc));  // eeeeeee kapou na epistrefw HT_ERROR!!!!!!
    // Free the table position
    table[indexDesc].fileDesc = -1;  
    table[indexDesc].fileName = '\0';
    openFileCounter--;

    // SAVE THE HASHTABLE.

    return HT_OK;
}

HT_ErrorCode HT_InsertEntry(int indexDesc, Record record) {
    
    HT_info *info = getInfo(indexDesc);
    
    int *hashTable; //The table consists of integers representing the position of the block in the file.
    int depth = 1;
    depth <<= info->globalDepth; //memory allocated grows exponentially based on global depth
    if(info->totalRecords == 0)  { //Init table
        hashTable = malloc(sizeof(int) * (depth));
        for(int i = 0; i < depth; i++) {
            hashTable[i] = -1; // arnhtiko index isws
        }
        info->hashTable = hashTable;
    } else { //if table already exists
        hashTable = info->hashTable;
    }

    BF_Block *block;
    BF_Block_Init(&block);


    int hashNum = hash_Function(record.id);
    hashNum = getMSBs(hashNum,info->globalDepth);
    int fileDesc = table[indexDesc].fileDesc;
    
    if(hashTable[hashNum] == -1) { //is there a bucket where entry hashed?
        printf("Bucket doesnt' exist, creating bucket...\n");
        BF_AllocateBlock(fileDesc,block);
        int blockPos;
        BF_GetBlockCounter(table[indexDesc].fileDesc, &blockPos);
        blockPos--;
        hashTable[hashNum] = blockPos;
        HT_block_info *data = (HT_block_info*)BF_Block_GetData(block); //Get pointer to beginning of block
        data->localDepth = 1;
        data++; // After block info
        Record *rec = (Record*)data;
        rec[0] = record; //This is going to be the first entry in the new bucket [OBVIOUSLY]
        data--;
        data->numOfRecords = 1; //auksanoume records profanws
        info->totalRecords++;
        printf("========%d======\n",hashTable[hashNum]);
        int bucketPointer = depth/2; 
        if(depth / 2 > hashNum) //poia einai ta filarakia, ta prwta misa i ta epomena misa?!?!?!!?
            bucketPointer = 1;
        for(int i = bucketPointer - 1; i < (depth >> data->localDepth); i++) { //create buddies, in localdepth 1 is half the table
            hashTable[i] = blockPos;
        }
        BF_Block_SetDirty(block);
        CALL_BF(BF_UnpinBlock(block)); //e nai


        //Update ht_info
        BF_GetBlock(fileDesc, 0, block); 
        void* voidData = BF_Block_GetData(block);
        voidData = info;
        BF_Block_SetDirty(block);
        CALL_BF(BF_UnpinBlock(block));
        BF_Block_Destroy(&block);
    } else {
        printf("Bucket exists, trying to insert...\n");
        BF_Block *oldBlock;
        BF_Block_Init(&oldBlock);
        //tha prepei kapws na kanoume getBlock kai na to briskoume ston disko, meta afou to vroume na sigoureftoume oti exoun idious pointers 
        int blockPos = hashTable[hashNum];
        BF_GetBlock(table[indexDesc].fileDesc, blockPos, oldBlock);
        printf("========%d======\n",blockPos);
        HT_block_info *blockInfo = (HT_block_info*)BF_Block_GetData(oldBlock); //Get pointer to beginning of block
        if(blockInfo->numOfRecords < RECORDS_PER_BLOCK) { //AN DEN EINAI GEMATO, TOTE APLA VAZOUME TO RECORD LMAO
            printf("Bucket is not full, inserting :) \n");
            //insert entry
            blockInfo++; // pame meta to info
            Record *recData = (Record*)blockInfo;
            blockInfo--;
            recData[blockInfo->numOfRecords] = record;
            blockInfo->numOfRecords++;
            info->totalRecords++;
        } else { //an apo tin alli einai gemato to bucket 
            printf("Bucket is FULL o_o\n");
            depth = 1;
            printf("LOCAL %d GLOBAL %d\n", blockInfo->localDepth, info->globalDepth);
            if(blockInfo->localDepth == info->globalDepth) {
                resizeHashTable(info);
                hashTable = info->hashTable;
                blockPos *= 2;
                blockPos = hashTable[2*hashNum];
            } 
            //(blockInfo->localDepth < info->globalDepth) <-this is why we are here 
            depth <<= info->globalDepth; //globaldepth might have changed
            int buddies = 2 << (info->globalDepth - blockInfo->localDepth);
            int oneHalf = buddies/2;
            int bucketPointer = depth/2; 
            if(bucketPointer > hashNum) //poia einai ta filarakia, ta prwta misa i ta epomena misa?!?!?!!?
                bucketPointer = 1;
            
            
            BF_ErrorCode code = BF_AllocateBlock(fileDesc,block);
            if(code != HT_OK) 
                BF_PrintError(code);
            int newBlockPos;
            BF_GetBlockCounter(table[indexDesc].fileDesc, &newBlockPos);
            printf("WE HAVE %d BLOCKS\n", newBlockPos);
            newBlockPos--;

            for(int i = bucketPointer - 1; i < depth; i++) { //vres ta filarakia, ta misa asta na deixnoun sto block pou ipirxe ta alla misa tha deixnoun sto kainourio
                if(hashTable[i] == blockPos) { //FOUND A BUDDY
                    if (oneHalf > 0) oneHalf--;
                    else hashTable[i] = newBlockPos;
                    buddies--;
                }
                if(buddies == 0) break;
            }
            reHash(table[indexDesc].fileDesc, blockPos, newBlockPos, hashTable, info->globalDepth);

            BF_Block_SetDirty(block);
            BF_UnpinBlock(block);
            BF_Block_Destroy(&block);
            printf("Records in block are %d out of %ld\n", blockInfo->numOfRecords, RECORDS_PER_BLOCK);
            HT_InsertEntry(indexDesc, record);
        }
        BF_Block_SetDirty(oldBlock);
        CALL_BF(BF_UnpinBlock(oldBlock));
        BF_Block_Destroy(&oldBlock);
    }
}

HT_ErrorCode HT_PrintAllEntries(int indexDesc, int *id) {
    // printf("%dFFFFFFFFFFFFFFFFFFFFFFFFFF\n", table[indexDesc].infoBlock->globalDepth);
    HT_info* info = getInfo(indexDesc);
    int hashNum = hash_Function(*id);
    hashNum = getMSBs(hashNum, info->globalDepth);

    int* hashTable = info->hashTable;
    int blockPos = hashTable[hashNum];

    BF_Block *block;
    BF_Block_Init(&block);

    BF_GetBlock(table[indexDesc].fileDesc, blockPos, block);

    HT_block_info* blockInfo = (HT_block_info*)BF_Block_GetData(block);
    blockInfo++;
    Record* rec = (Record*)blockInfo;
    blockInfo--;
    
    for(int i = 0; i < blockInfo->numOfRecords; i++) {
        if(rec[i].id == *id) {
            printf("%s ", rec[i].name);
            printf("%s ", rec[i].surname);
            printf("%s ", rec[i].city);
            printf("%d\n", rec[i].id);
        }
    }

    CALL_BF(BF_UnpinBlock(block));
    BF_Block_Destroy(&block);   
}

HT_ErrorCode HashStatistics(char* filename) {
    int indexDesc;
    for(indexDesc = 0; indexDesc < MAX_OPEN_FILES; indexDesc++) 
        if(table[indexDesc].fileName == filename) break;
    BF_Block* block;
    BF_Block_Init(&block);
    HT_info* info = getInfo(indexDesc);
    int blockNum;
    int fileDesc = table[indexDesc].fileDesc;
    BF_GetBlockCounter(fileDesc, &blockNum);
    blockNum--;
    int minRecords = RECORDS_PER_BLOCK;
    int maxRecords = 0;
    float averageRecords = 0;
    for(int i = 1; i < blockNum + 1; i++) {
        BF_GetBlock(fileDesc, i, block);
        char* data = BF_Block_GetData(block);
        HT_block_info* info = (HT_block_info*) data;
        if(info->numOfRecords < minRecords)
            minRecords = info->numOfRecords;
        if(info->numOfRecords > maxRecords)
            maxRecords = info->numOfRecords;
        averageRecords += info->numOfRecords;
        BF_UnpinBlock(block);
    }
    averageRecords = averageRecords / (float)blockNum;
    printf("Hash Statistics:\n\t Total number of buckets: %d\n\t Max records in a bucket: %d\n\t Min records in a bucket: %d\n\t Average records in a bucket: %f\n",
            blockNum, maxRecords, minRecords, averageRecords);
    return HT_OK;
}


// Return infoBlock of given index.
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


/*
    |1 |2 |11|4 |5 |6 |7 |8 |9 |10|

*/
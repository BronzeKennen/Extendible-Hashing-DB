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
    int fileDesc = table[indexDesc].fileDesc;
    BF_Block_Init(&block);
    BF_AllocateBlock(fileDesc,block);
    
    
    BF_GetBlock(fileDesc,0,block);
    HT_info* info = (HT_info*)BF_Block_GetData(block);
    


    int block_num;
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
        // IDEA TOU ALEXTURINGTHEGOAT: afou kanoume bf_alloc, kanoume bf_getblockcounter gia na doume posa blocks exoume
        // kai meta hashTable[hasNum] = bf_getblockcounter - 1. Etsi meta me to hashTable[hasNum] pou einai int, kanoume 
        // BF_GetBlock.
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
        BF_UnpinBlock(block); //e nai


        //Update ht_info
        BF_GetBlock(fileDesc, 0, block); 
        void* voidData = BF_Block_GetData(block);
        voidData = info;
        BF_Block_SetDirty(block);
        BF_UnpinBlock(block);
    
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
            
            BF_AllocateBlock(fileDesc,block);
            int newBlockPos;
            BF_GetBlockCounter(table[indexDesc].fileDesc, &newBlockPos);
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

            BF_Block_SetDirty(oldBlock);
            BF_Block_SetDirty(block);
            BF_UnpinBlock(oldBlock);
            BF_UnpinBlock(block);
            printf("Records in block are %d out of %ld\n", blockInfo->numOfRecords, RECORDS_PER_BLOCK);
            HT_InsertEntry(indexDesc, record);
        }
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

    BF_Block_Destroy(&block);
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

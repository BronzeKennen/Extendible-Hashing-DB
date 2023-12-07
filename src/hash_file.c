#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bf.h"
#include "hash_file.h"
#include "extras.h"


#define CALL_BF(call)       \
{                           \
  BF_ErrorCode code = call; \
  if (code != BF_OK) {      \
    BF_PrintError(code);    \
    return BF_ERROR;        \
  }                         \
}

#define RECORDS_PER_BLOCK (BF_BLOCK_SIZE - sizeof(HT_block_info)) / sizeof(Record)


// Global Data of the table and a counter that keeps track of open files .
HT_table_file_entry *table;
int openFileCounter;


// Allocate memory for open files table and initialize values as empty.
HT_ErrorCode HT_Init() {
    openFileCounter = 0;
    CALL_BF(BF_Init(LRU));

    // Allocate memory for the table of open files
    table = malloc(sizeof(HT_table_file_entry) * MAX_OPEN_FILES);
    if(!table) return HT_ERROR;
    for(int i = 0; i < MAX_OPEN_FILES; i++){
        // Init positions as empty.
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


HT_ErrorCode HT_CreateIndex(const char *filename, int depth) {

    int fileDesc;
    CALL_BF(BF_CreateFile(filename));

    // Create file for the dictionary.
    char dictFile[sizeof(filename) + 5] = "dict";
    strncat(dictFile,filename,sizeof(filename));
    CALL_BF(BF_CreateFile(dictFile));

    //Create HT_Info.
    BF_Block *block;
    BF_Block_Init(&block);

    CALL_BF(BF_OpenFile(filename,&fileDesc));
    CALL_BF(BF_AllocateBlock(fileDesc,block));

    HT_info *data = (HT_info*)BF_Block_GetData(block);
    data->globalDepth = depth;
    data->totalRecords = 0;
    data->type = HASH;

    // Set block as dirty so data is written and free memory.
    BF_Block_SetDirty(block);
    CALL_BF(BF_UnpinBlock(block));
    CALL_BF(BF_CloseFile(fileDesc));
    BF_Block_Destroy(&block);

    return HT_OK;
}


HT_ErrorCode HT_OpenIndex(const char *fileName, int *indexDesc) {
    // Make sure file exists.
    FILE* fp;
    if((fp = fopen(fileName, "r")) == NULL) {
        return HT_ERROR;
    }
    fclose(fp);
    int fileDesc;
    CALL_BF(BF_OpenFile(fileName,&fileDesc));

    // Make sure we have space in table.
    if(openFileCounter == MAX_OPEN_FILES) {
        return HT_ERROR;
    }
    // Open dictionary for later.
    int dictDesc;
    char dictFile[sizeof(fileName) + 5] = "dict";
    strncat(dictFile,fileName,sizeof(fileName));
    CALL_BF(BF_OpenFile(dictFile,&dictDesc));


    // Find the next available spot in the open file table and keep the index.
    for(int i=0; i<MAX_OPEN_FILES; i++){
        if(table[i].fileDesc == -1){
            *indexDesc = i;
            break;
        }
    }

    // Set the file and set the open file table index.
    table[*indexDesc].fileDesc = fileDesc;
    table[*indexDesc].fileName = fileName;
    openFileCounter++;

    // Get info for global depth.
    HT_info *info = getInfo(*indexDesc);   

    int buckets = 1 << info->globalDepth; // This is the total slots of the hash table.
    int dictBlocks;
    CALL_BF(BF_GetBlockCounter(dictDesc,&dictBlocks));
    int* hashTable = malloc(sizeof(int)*buckets); // Allocate memory for the table.
    if(!dictBlocks) { // If there are no blocks, it means that there are no records, so initialize all slots as empty.
        for(int i = 0; i < buckets; i++) {
            hashTable[i] = -1;
        }
        info->hashTable = hashTable; // Have the pointer of HT_Info point at the allocated memory
        return HT_OK;
    } else {
        BF_Block *block;
        BF_Block_Init(&block);
        int bucketsPerBlock = BF_BLOCK_SIZE / sizeof(int); // Slots of every segment of the table.
        for(int i = 0; i < dictBlocks; i++) { // Every block in the dictionary is a segment of the hash table.
            CALL_BF(BF_GetBlock(dictDesc,i,block));
            int* data = (int*)BF_Block_GetData(block);
            if(buckets < BF_BLOCK_SIZE / 4) {
                memcpy(&hashTable[i * bucketsPerBlock],data,buckets * 4);   // If remaining slots are less than a full block,
                                                                            // copy just as many as we need.
            } else {
                memcpy(&hashTable[i * bucketsPerBlock],data,BF_BLOCK_SIZE);
                buckets -= (int)BF_BLOCK_SIZE / 4; // Update remaing slots. 
            }
        }
            info->hashTable = hashTable; // Have the pointer of HT_Info point at the allocated memory
        return HT_OK;
    }
    return HT_ERROR;
}

HT_ErrorCode HT_CloseFile(int indexDesc) {
    // Make sure we have a file there.
    if(table[indexDesc].fileDesc == -1) {
        return HT_ERROR;
    }
    // Init memory for blocks.
    BF_Block *block; 
    BF_Block_Init(&block);
    
    // Get table and total table slots from HT_Info.
    HT_info* info = getInfo(indexDesc);
    int* hashTable = info->hashTable;
    int tableSlots = 1;
    tableSlots <<= info->globalDepth;

    int finalSlots = tableSlots; // Variable to use for the last block that is potentially half-full.

    // Every block is being set to dirty and getting unpinned dynamically, except the block of HT_Info.
    CALL_BF(BF_GetBlock(table[indexDesc].fileDesc, 0, block));
    BF_Block_SetDirty(block);
    CALL_BF(BF_UnpinBlock(block));


    // Save the dictionary.

    // Get file name and OpenFile.
    int dictDesc;
    const char* fileName = table[indexDesc].fileName;
    char dictFile[sizeof(fileName) + 5] = "dict";
    strncat(dictFile,fileName,sizeof(fileName));
    BF_OpenFile(dictFile, &dictDesc);


    BF_Block* block2;
    BF_Block_Init(&block2);
    int dictBlocks;
    CALL_BF(BF_GetBlockCounter(dictDesc, &dictBlocks));
    int tableBlocks = ((int)tableSlots / (BF_BLOCK_SIZE / sizeof(int))) + 1;
    for(int blockPos = 0; blockPos < tableBlocks; blockPos++) {
        // If there were already enough blocks in the dictionary get the block. Else, allocate a new one.
        if(blockPos < dictBlocks) {
            BF_GetBlock(dictDesc, blockPos, block2);
        } else {
            BF_AllocateBlock(dictDesc, block2);

        }
        // Get the data of the block and copy the table segment inside.
        int* tablePart = (int*)BF_Block_GetData(block2);
        if(tableSlots < (BF_BLOCK_SIZE / sizeof(int))) { // If the data is less than a full block, copy the actual size.
            memcpy(tablePart, &info->hashTable[blockPos * (BF_BLOCK_SIZE / sizeof(int)) ], sizeof(tableSlots)*tableSlots);
        } else {
            memcpy(tablePart, &info->hashTable[blockPos * ((BF_BLOCK_SIZE / sizeof(int)) )], BF_BLOCK_SIZE); // Copy an entire block.
            tableSlots -= BF_BLOCK_SIZE / sizeof(int); // Update remaining tableSlots for next loop.
        }
        BF_Block_SetDirty(block2); // Set all blocks as dirty and unpin them.
        BF_UnpinBlock(block2);
    }
    // Free memory and close both files.
    BF_Block_Destroy(&block);
    BF_Block_Destroy(&block2);
    CALL_BF(BF_CloseFile(table[indexDesc].fileDesc)); 
    CALL_BF(BF_CloseFile(dictDesc));

    // Free the table position and update open files.
    table[indexDesc].fileDesc = -1;  
    table[indexDesc].fileName = '\0';
    openFileCounter--;

    return HT_OK;
}

HT_ErrorCode HT_InsertEntry(int indexDesc, Record record) {
    
    // Get HT_Info to use later.
    HT_info *info = getInfo(indexDesc);
    
    int *hashTable; //The table consists of integers representing the position of the block in the file.
    int depth = 1;
    depth <<= info->globalDepth; //memory allocated grows exponentially based on global depth
    hashTable = info->hashTable;

    // Allocate memory for block struct.
    BF_Block *block;
    BF_Block_Init(&block);

    // Hash record id and get most significant bytes to determine position.
    int hashNum = hash_Function(record.id);
    hashNum = getMSBs(hashNum,info->globalDepth);
    int fileDesc = table[indexDesc].fileDesc;
    
    if(hashTable[hashNum] == -1) { // If bucket hasn't been created yet, create the bucket/block.
        BF_AllocateBlock(fileDesc,block);

        // Get block position and set the table of hashNum to that position.
        int blockPos;
        BF_GetBlockCounter(table[indexDesc].fileDesc, &blockPos);
        blockPos--;
        hashTable[hashNum] = blockPos;

        // Get block info and initialize it.
        HT_block_info *data = (HT_block_info*)BF_Block_GetData(block);
        data++; // Records are right after block info.
        Record *rec = (Record*)data;

        rec[0] = record;  // This is going to be the first entry in the new bucket, hence rec[0].

        // Return pointer to block info and set records and local depth as 1.
        data--; 
        data->localDepth = 1;
        data->numOfRecords = 1; 
        info->totalRecords++; // Update HT_Info.

        // Find and create buddies. Since local depth is 1, buddies are half of table.
        int bucketPointer = depth/2; // Start at the middle of the table.
        if(depth / 2 > hashNum) // If we hash in the first half, move pointer in the beggining.
            bucketPointer = 1;
        for(int i = bucketPointer - 1; i < (depth >> data->localDepth); i++) { 
            hashTable[i] = blockPos;
        }
        // Set new block as dirty, unpin and free memory.
        BF_Block_SetDirty(block);
        CALL_BF(BF_UnpinBlock(block)); 
        BF_Block_Destroy(&block);

    } else { // Bucket already exists, get the block.
        BF_Block *oldBlock;
        BF_Block_Init(&oldBlock);
        int blockPos = hashTable[hashNum];
        BF_GetBlock(table[indexDesc].fileDesc, blockPos, oldBlock);

        HT_block_info *blockInfo = (HT_block_info*)BF_Block_GetData(oldBlock);
        if(blockInfo->numOfRecords < RECORDS_PER_BLOCK) { // If there is space in the block, simply insert the entry.
            blockInfo++; // Right after info.
            Record *recData = (Record*)blockInfo;
            blockInfo--; // Reset info.
            recData[blockInfo->numOfRecords] = record; // Insert. 
            blockInfo->numOfRecords++;
            info->totalRecords++;
        } else { // If the bucket is full, we need to compare local and global depth.
            if(blockInfo->localDepth == info->globalDepth) { // If local is equal to global we need to resize the entire table.
                resizeHashTable(info);
                hashTable = info->hashTable; // Get the new table.
            } 
            // At this point, we are sure local depth is less than global depth.
            depth = 1;
            depth <<= info->globalDepth; // Global depth has changed. Get new number of slots.
            
            // We want to find all slots that are buddies and set half of them in the old block and half of them in the new.
            int buddies = 2 << (info->globalDepth - blockInfo->localDepth); // Number of buddies
            int oneHalf = buddies/2; // Half of buddies.

            // Discard half of the table to make search a bit faster.
            int bucketPointer = depth/2; 
            if(bucketPointer > hashNum) 
                bucketPointer = 1;
            
            // Allocate block and keep its position.
            CALL_BF(BF_AllocateBlock(fileDesc,block));
            int newBlockPos;
            BF_GetBlockCounter(table[indexDesc].fileDesc, &newBlockPos);
            newBlockPos--;

            // Find buddies and set the value.
            for(int i = bucketPointer - 1; i < depth; i++) { 
                if(hashTable[i] == blockPos) { // Found the first.
                    if (oneHalf > 0) oneHalf--; // Keep the first half as it is
                    else hashTable[i] = newBlockPos; // Set the second half to the new block.
                    buddies--;
                }
                if(buddies == 0) break;
            }

            // Rehash all the records using the new depth.
            reHash(table[indexDesc].fileDesc, blockPos, newBlockPos, hashTable, info->globalDepth);

            // Set new block as dirty, unpin and free the memory.
            BF_Block_SetDirty(block);
            CALL_BF(BF_UnpinBlock(block));
            BF_Block_Destroy(&block);

            // At this point, even after rehashing, there is a possibility that the bucket is still full. For this reason
            // we need to recursively call HT_InsertEntry.
            HT_InsertEntry(indexDesc, record);
        }
        // Set old block as dirty, unpin and free the memory.
        BF_Block_SetDirty(oldBlock);
        CALL_BF(BF_UnpinBlock(oldBlock));
        BF_Block_Destroy(&oldBlock);
    }
}

HT_ErrorCode HT_PrintAllEntries(int indexDesc, int *id) {

    HT_info* info = getInfo(indexDesc);

    // Init memory for block.
    BF_Block *block;
    BF_Block_Init(&block);

    if(id == NULL) { // We need to print every record.
        int blockNum;
        CALL_BF(BF_GetBlockCounter(table[indexDesc].fileDesc, &blockNum));
        for(int blockPos = 1; blockPos < blockNum; blockPos++) { // Block 0 is info, we don't want that.

            // Get the block.
            CALL_BF(BF_GetBlock(table[indexDesc].fileDesc, blockPos, block));

            // Get block info for number of records.
            HT_block_info* blockInfo = (HT_block_info*)BF_Block_GetData(block);
            blockInfo++;
            Record* rec = (Record*)blockInfo; // Right after info.
            blockInfo--; // Reset info.
            
            // Print all the records.
            for(int i = 0; i < blockInfo->numOfRecords; i++) {
                printf("%s ", rec[i].name);
                printf("%s ", rec[i].surname);
                printf("%s ", rec[i].city);
                printf("%d\n", rec[i].id);
            }

            // Unpin block.
            CALL_BF(BF_UnpinBlock(block));
        }
        // Free the memory.
        BF_Block_Destroy(&block);
        return HT_OK;
    } 

    // Get hash number and block position.
    int hashNum = hash_Function(*id);
    hashNum = getMSBs(hashNum, info->globalDepth);
    int* hashTable = info->hashTable;
    int blockPos = hashTable[hashNum];


    // Get the block.
    CALL_BF(BF_GetBlock(table[indexDesc].fileDesc, blockPos, block));

    // Get block info for number of records.
    HT_block_info* blockInfo = (HT_block_info*)BF_Block_GetData(block);
    blockInfo++;
    Record* rec = (Record*)blockInfo; // Right after info.
    blockInfo--; // Reset info.
    
    // Search the records for the specific ID.
    int foundEntry = 0;
    for(int i = 0; i < blockInfo->numOfRecords; i++) {
        if(rec[i].id == *id) {
            printf("%s ", rec[i].name);
            printf("%s ", rec[i].surname);
            printf("%s ", rec[i].city);
            printf("%d\n", rec[i].id);
            foundEntry = 1;
        }
    }
    if(!foundEntry) {
        printf("No entries found with ID %d\n", *id);
    }

    // Unpin the block and free the memory.
    CALL_BF(BF_UnpinBlock(block));
    BF_Block_Destroy(&block);   
    return HT_OK;
}

HT_ErrorCode HashStatistics(char* filename) {
    // Get indexDesc from open file table.
    int indexDesc;
    int foundFile = 0;
    for(indexDesc = 0; indexDesc < MAX_OPEN_FILES; indexDesc++) 
        if(table[indexDesc].fileName == filename) {
            foundFile = 1;
            break;
        }
    if(!foundFile) {
        return HT_ERROR;
    }
    // Init memory for block and get info.
    BF_Block* block;
    BF_Block_Init(&block);
    HT_info* info = getInfo(indexDesc);


    int fileDesc = table[indexDesc].fileDesc;
    int blockNum;
    BF_GetBlockCounter(fileDesc, &blockNum); // Number of blocks
    blockNum--; // Range is 0 to blockNum - 1.

    // Set initial values to be updated.
    int minRecords = RECORDS_PER_BLOCK;
    int maxRecords = 0;
    float averageRecords = 0; // for total and average records.

    // Loop through all the blocks' info and check the values.
    for(int i = 1; i < blockNum + 1; i++) {
        BF_GetBlock(fileDesc, i, block);
        char* data = BF_Block_GetData(block);
        HT_block_info* info = (HT_block_info*) data;

        // Update min and max if needed.
        if(info->numOfRecords < minRecords)
            minRecords = info->numOfRecords;
        if(info->numOfRecords > maxRecords)
            maxRecords = info->numOfRecords;

        // Add to the total.
        averageRecords += info->numOfRecords;
        BF_UnpinBlock(block);
    }
    averageRecords = averageRecords / (float)blockNum; // Get average records.
    printf("Hash Statistics of %s:\n\tTotal number of buckets: %d\n\tTotal number of records: %d\n\tMax records in a bucket: %d\n\tMin records in a bucket: %d\n\tAverage records in a bucket: %f\n",
            filename, blockNum, info->totalRecords, maxRecords, minRecords, averageRecords);

    BF_Block_Destroy(&block);
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "bf.h"
#include "hash_file.h"
#include "extras.h"
#include "acutest.h"


#define RECORDS_NUM 1000 // you can change it if you want

const char* names[] = {
  "Yannis",
  "Christofos",
  "Sofia",
  "Marianna",
  "Vagelis",
  "Maria",
  "Iosif",
  "Dionisis",
  "Konstantina",
  "Theofilos",
  "Giorgos",
  "Dimitris"
};

const char* surnames[] = {
  "Ioannidis",
  "Svingos",
  "Karvounari",
  "Rezkalla",
  "Nikolopoulos",
  "Berreta",
  "Koronis",
  "Gaitanis",
  "Oikonomou",
  "Mailis",
  "Michas",
  "Halatsis"
};

const char* cities[] = {
  "Athens",
  "San Francisco",
  "Los Angeles",
  "Amsterdam",
  "London",
  "New York",
  "Tokyo",
  "Hong Kong",
  "Munich",
  "Miami"
};

#define CALL_OR_DIE(call)     \
  {                           \
    HT_ErrorCode code = call; \
    if (code != HT_OK) {      \
      printf("Error\n");      \
      exit(code);             \
    }                         \
  }

void test_create(void) {
    TEST_ASSERT(HT_Init() == HT_OK);

    TEST_ASSERT(HT_CreateIndex("test1.db", 2) == HT_OK); //Should work
    TEST_ASSERT(HT_CreateIndex("test1.db", 8) != HT_OK); //Should not work, file already exists. Error is being printed.
    TEST_ASSERT(HT_CreateIndex("test2.db", 4) == HT_OK); //Should work

    TEST_ASSERT(HT_Destroy() == HT_OK);
}

int indexDesc[MAX_OPEN_FILES + 1];

void test_open_and_close(void) {
    HT_Init();

    // Test open.
    TEST_ASSERT(HT_OpenIndex("test1.db", &indexDesc[0]) == HT_OK); // This will fail if you call tests again. Insert won't.
    TEST_ASSERT(indexDesc[0] == 0);
    TEST_ASSERT(HT_OpenIndex("test2.db", &indexDesc[1]) == HT_OK); 
    TEST_ASSERT(indexDesc[1] == 1);
    TEST_ASSERT(HT_OpenIndex("dx.db", &indexDesc[2]) != HT_OK); // The file doesnt exist.
    for(int i = 2; i < MAX_OPEN_FILES; i++) {
      TEST_ASSERT(HT_OpenIndex("test1.db", &indexDesc[i]) == HT_OK);
      TEST_ASSERT(indexDesc[i] == i);
    }
    TEST_ASSERT(HT_OpenIndex("test1.db", &indexDesc[MAX_OPEN_FILES]) != HT_OK); // No room for more files.

    // Test close.
    TEST_ASSERT(HT_CloseFile(indexDesc[1]) == HT_OK); // Close file.
    TEST_ASSERT(HT_CloseFile(indexDesc[1]) != HT_OK); // Shouldn't work, there isn't an open file there.

    // Open in the empty spot to make sure open file counter works.
    TEST_ASSERT(HT_OpenIndex("test2.db", &indexDesc[1]) == HT_OK); 
    TEST_ASSERT(indexDesc[1] == 1);

    HT_Destroy();
}

void test_insert(void) {  
    HT_Init();

    int indexDesc;
    HT_OpenIndex("test1.db",&indexDesc);
    
    Record record;
    BF_Block *block;
    BF_Block_Init(&block);

    // Random seed
    srand(12569874);
    int r;

    HT_info* info = getInfo(indexDesc);
    int recordsBefore = info->totalRecords;
    int numOfRecords = RECORDS_NUM; // Test number, you can change it if you want.
    for(int id = 0; id < numOfRecords; id++) {

      // Skip id = 5 on purpose to use in PrintAllEntries.
      if(id == 5)
        continue;

      //Create a random record
      record.id = id; 
      r = rand() % 12;
      memcpy(record.name, names[r], strlen(names[r]) + 1);
      r = rand() % 12;
      memcpy(record.surname, surnames[r], strlen(surnames[r]) + 1);
      r = rand() % 10;
      memcpy(record.city, cities[r], strlen(cities[r]) + 1);

      // Insert the entry.
      HT_InsertEntry(indexDesc,record);

      // Close and open the file again to see if it works correctly.
      if(id == numOfRecords / 2) {
          HT_CloseFile(indexDesc);
          HT_OpenIndex("test1.db", &indexDesc);
      }
    }
    info = getInfo(indexDesc);
    TEST_ASSERT(info->totalRecords == recordsBefore + numOfRecords - 1);
    HT_CloseFile(indexDesc); 
    HT_Destroy();
}

void test_printentries(void) {
    int indexDesc;
    HT_Init();
    HT_OpenIndex("test1.db",&indexDesc);
    // HT_info *info = getInfo(indexDesc);
    BF_Block *block;
    BF_Block_Init(&block);
    BF_GetBlock(indexDesc,0,block);
    HT_info *info = (HT_info*)BF_Block_GetData(block);    
    // Print every entry.
    printf("\nThese are all the entries:\n");
    HT_PrintAllEntries(indexDesc, NULL);

    // Print 3 entries.
    printf("\nThese are 3 entries:\n");
    int rand = info->totalRecords / 2;
    HT_PrintAllEntries(indexDesc, &rand);
    rand = info->totalRecords / 3;
    HT_PrintAllEntries(indexDesc, &rand);
    rand = info->totalRecords / 4;
    HT_PrintAllEntries(indexDesc, &rand);

    // Search for ID = 5. Should not be found.
    rand = 5;
    printf("\nSearching for ID = 5:\n");
    BF_UnpinBlock(block);
    BF_Block_Destroy(&block);
    HT_PrintAllEntries(indexDesc, &rand);
    HT_CloseFile(indexDesc);
    HT_Destroy();
}

void test_hashstatistics(void) {
    HT_Init();

    // Test for a file that exists.
    int indexDesc;
    HT_OpenIndex("test1.db", &indexDesc);
    printf("\n");
    TEST_ASSERT(HashStatistics("test1.db") == HT_OK);

    // File name doesn't exist or isn't open.
    TEST_ASSERT(HashStatistics("dx.db") != HT_OK);
    HT_CloseFile(indexDesc);
    HT_Destroy();
}



TEST_LIST = {
   { "create", test_create },
   { "open and close", test_open_and_close },
   { "insert", test_insert },
   { "printEntries", test_printentries },
   { "hashStatistics", test_hashstatistics },
   { NULL, NULL }     /* zeroed record marking the end of the list */
};
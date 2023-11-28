#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "bf.h"
#include "hash_file.h"
#include "extras.h"
#include "acutest.h"


#define RECORDS_NUM 1000 // you can change it if you want
#define GLOBAL_DEPTH 2 // you can change it if you want
#define FILE_NAME "data.db"

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

    TEST_ASSERT(HT_CreateIndex("test1.db", 4) == HT_OK); //Should work
    TEST_ASSERT(HT_CreateIndex("test1.db", 8) != HT_OK); //Should not work, file already exists

}
int indexDesc1;

void test_open(void) {

    HT_Init();
    TEST_ASSERT(HT_OpenIndex("test1.db", &indexDesc1) == HT_OK);

    TEST_ASSERT(indexDesc1 == 0);

}

void test_close(void) {
    HT_Init();
    TEST_ASSERT(HT_OpenIndex("test1.db", &indexDesc1) == HT_OK);
    TEST_ASSERT(HT_CloseFile(indexDesc1) == HT_OK);
    
}

void test_insert(void) {  
    HT_Init();
    int indexDesc;
    HT_OpenIndex("test1.db",&indexDesc);
    
    Record record;
    BF_Block *block;
    
    BF_Block_Init(&block);
    srand(12569874);
    int r;
    printf("Insert Entries\n");

    for(int id = 0; id < 50; id++) {
    //Create a random record
      record.id = id; // MALAKA ME TI ID TO EVAZES TOSH WRA?
      printf("Creating record %d\n",id);
      r = rand() % 12;
      memcpy(record.name, names[r], strlen(names[r]) + 1);
      r = rand() % 12;
      memcpy(record.surname, surnames[r], strlen(surnames[r]) + 1);
      r = rand() % 10;
      memcpy(record.city, cities[r], strlen(cities[r]) + 1);

      //Try to insert an entry
      HT_InsertEntry(indexDesc,record);
      printf("\"Succesfully\" inserted record %d\n",id);
    }
    HT_info *info = getInfo(indexDesc);
    TEST_ASSERT(info->totalRecords == 50);
    
    //Try to get the supposed block we placed the entry
    BF_GetBlock(indexDesc,1,block);
}

void test_printentries(void) {
  
}

void test_hashstatistics(void) {
  
}


void testMSBs(void) {
  TEST_ASSERT(getMSBs(2512368892,3) == 4);
}

TEST_LIST = {
   { "create", test_create },
   { "open", test_open },
   { "close", test_close },
   { "insert", test_insert },
   { "printEntries", test_printentries },
   { "hashStatistics", test_hashstatistics },
   {"testgetMSBs", testMSBs},
   { NULL, NULL }     /* zeroed record marking the end of the list */
};
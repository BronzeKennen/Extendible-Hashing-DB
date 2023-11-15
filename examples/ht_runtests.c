#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "bf.h"
#include "hash_file.h"
#include "acutest.h"


#define RECORDS_NUM 1000 // you can change it if you want
#define GLOBAL_DEPT 2 // you can change it if you want
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

    TEST_ASSERT(HT_CreateIndex("create.db", 4) == HT_OK); //Should work
    TEST_ASSERT(HT_CreateIndex("create.db", 8) != HT_OK); //Should not work, file already exists

}

void test_open(void) {

    HT_Init();
    int fileDesc;
    HT_OpenIndex("create.db", &fileDesc);

}

void test_close(void) {
    
}

void test_insert(void) {
  
}

void test_printentries(void) {
  
}

void test_hashstatistics(void) {
  
}

TEST_LIST = {
   { "create", test_create },
   { "open", test_open },
   { "close", test_close },
   { "insert", test_insert },
   { "printEntries", test_printentries },
   { "hashStatistics", test_hashstatistics },
   { NULL, NULL }     /* zeroed record marking the end of the list */
};
#include "hash_file.h"

int hash_Function(int num);

void reHash(int fileDesc, int oldBlockPos, int newBlockPos, int *hashTable, int globalDepth);

unsigned int getMSBs(unsigned int num, int depth);

void resizeHashTable(HT_info *info);

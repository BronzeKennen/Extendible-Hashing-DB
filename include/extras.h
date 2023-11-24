#include "hash_file.h"

int hash_Function(int num);

void reHash(BF_Block *oldBlock, BF_Block *newBlock, BF_Block **hashTable, int globalDepth);

unsigned int getMSBs(unsigned int num, int depth);

void resizeHashTable(HT_info *info);

#include "hash_file.h"

int hash_Function(int num);

void reHash(HT_info *info);

unsigned int getMSBs(unsigned int num, int depth);

void resizeHashTable(HT_info *info);

/* Sistemas Operativos, DEI/IST/ULisboa 2019-20 */

#ifndef FS_H
#define FS_H

#include "lib/bst.h"
#include "sync.h"

typedef struct tecnicofs {
    node **bstRoot;
    int nextINumber;
    syncMech*bstLock;
} tecnicofs;

int obtainNewInumber(tecnicofs *fs);

tecnicofs *new_tecnicofs();

void free_tecnicofs(tecnicofs *fs, int numberBuckets);

void create(tecnicofs *fs, char *name, int inumber, int bstNumber);

void delete(tecnicofs *fs, char *name, int bstNumber);

int lookup(tecnicofs *fs, char *name, int bstNumber);

void print_tecnicofs_tree(FILE *fp, tecnicofs *fs, int numberBuckets);

#endif /* FS_H */

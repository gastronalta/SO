/* Sistemas Operativos, DEI/IST/ULisboa 2019-20 */

#include "fs.h"
#include "lib/bst.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "sync.h"


int obtainNewInumber(tecnicofs *fs) {
    int newInumber = ++(fs->nextINumber);
    return newInumber;
}

tecnicofs *new_tecnicofs(int numberBuckets) {
    tecnicofs *fs = malloc(sizeof(tecnicofs));
    if (!fs) {
        perror("failed to allocate tecnicofs");
        exit(EXIT_FAILURE);
    }

    fs->bstRoot = malloc(sizeof(node *) * numberBuckets);
    if (!(fs->bstRoot)) {
        perror("failed to allocate bst");
        exit(EXIT_FAILURE);
    }

    fs->bstLock = malloc(sizeof(syncMech) * numberBuckets);
    if (!(fs->bstLock)) {
        perror("failed to allocate bastLock");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < numberBuckets; ++i) {
        fs->bstRoot[i] = NULL;
        sync_init(&(fs->bstLock[i]));
    }

    fs->nextINumber = 0;

    return fs;
}

void free_tecnicofs(tecnicofs *fs, int numberBuckets) {
    for (int i = 0; i < numberBuckets; ++i) {
        free_tree(fs->bstRoot[i]);
        sync_destroy(&(fs->bstLock[i]));
    }
    free(fs->bstLock);                                          //migh not need it
    free(fs->bstRoot);
    free(fs);
}

void create(tecnicofs *fs, char *name, int inumber, int bstNum) {
    sync_wrlock(&(fs->bstLock[bstNum]));
    fs->bstRoot[bstNum] = insert(fs->bstRoot[bstNum], name, inumber);
    sync_unlock(&(fs->bstLock[bstNum]));
}

void delete(tecnicofs *fs, char *name, int bstNum) {
    sync_wrlock(&(fs->bstLock[bstNum]));
    fs->bstRoot[bstNum] = remove_item(fs->bstRoot[bstNum], name);
    sync_unlock(&(fs->bstLock[bstNum]));
}

int lookup(tecnicofs *fs, char *name, int bstNum) {
    sync_rdlock(&(fs->bstLock[bstNum]));
    int inumber = 0;
    node *searchNode = search(fs->bstRoot[bstNum], name);
    if (searchNode) {
        inumber = searchNode->inumber;
    }
    sync_unlock(&(fs->bstLock[bstNum]));
    return inumber;
}

void print_tecnicofs_tree(FILE *fp, tecnicofs *fs, int numberBuckets) {
    for (int i = 0; i < numberBuckets; ++i) {
        print_tree(fp, fs->bstRoot[i]);
    }
}

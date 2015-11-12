/*
 * Code inspired by but not copied from that of James Aspnes in CPSC 223
 */

#include <stdlib.h>
#include <assert.h>
#include <string.h>

typedef struct table *Table;

Table TableCreate(int size);

void TableDestroy(Table t);

unsigned long HASH(int p, int k, int size);

int TableLinearInsert(Table *t, int code, int prefix, int c, int maxBits, int index);

int TableInsert(Table *t, int code, int prefix, int c, int maxBits,int usage);

int TableGet(Table t, int prefix, int c);
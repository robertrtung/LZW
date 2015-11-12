/*
 * Parts of hashtable Implementation inspired by but not copied from
 * that of James Aspnes in CPSC 223
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include "/c/cs323/Hwk4/code.h"
#include "./lzwHashTable.h"

//Element in table representing code-string pair
struct elt {
    struct elt *next;//pointer to next element
    int code;//numerical code
    int prefix;//code of the prefix
    int c;//last character of code
    int usagecount;//number of times seen/used
};

//Table that stores all of the code-string pairs
struct table{
    int size;//size of table
    int n;//number of elements in table
    struct elt **table;//array/hashtable that stores the elements
};

/*
 * Create a table of size size
 */
Table TableCreate(int size){
    Table t;
    t = malloc(sizeof(struct table));
    assert(t != 0);

    t->n = 2;//leave 0 and 1 for flags as mentioned in encode
    t->size = size;
    t->table = malloc(sizeof(struct elt *) * t->size);

    assert(t->table != 0);

    //initialize table
    for(int i = 0; i < t->size; i++){
        t->table[i] = 0;
    }

    return t;
}

/*
 * Free memory in table
 */
void TableDestroy(Table t){
    struct elt *e;
    struct elt *next;

    for(int i = 0; i < t->size; i++) {
        for(e = t->table[i]; e != 0; e = next) {
            next = e->next;
            free(e);
        }
    }

    free(t->table);
    free(t);
}

/*
 * Hash function as provided by Professor Eisenstat
 */
unsigned long HASH(int p, int k, int size){
    return (((unsigned)(p) << CHAR_BIT) ^ ((unsigned) (k))) % size;
}

/*
 * Insert into the table in array order (used for array version of table)
 * Takes in a pointer to the table, the code, prefix and final character being
 * placed in the table, the max number of bits of the table
 * and the index of insertion
 */
int TableLinearInsert(Table *t, int code, int prefix, 
                        int c, int maxBits, int index){
    if((*t)->n >= (1 << maxBits)){
        //Table already maxed out
        return 0;
    }
    struct elt *e;
    int returned=0;

    e = malloc(sizeof(*e));

    assert(e);

    e->code = code;
    e->prefix = prefix;
    e->c = c;
    e->usagecount = 0;

    while((*t)->n >= (*t)->size){
        //table surpassed max load factor; increase size of table
        (*t)->size *= 2;
        (*t)->table = realloc((*t)->table,sizeof(struct elt *)*((*t)->size));
        for(int i=((*t)->size)/2;i<(*t)->size;i++){
            (*t)->table[i] = 0;
        }
        returned = 1;
    }
    assert(index == ((*t)->n));
    e->next = (*t)->table[index];
    (*t)->table[index] = e;

    ((*t)->n)++;
    return returned;
}

/*
 * Insert value into hashtable version of table
 * Takes in a pointer to the table, the code, prefix and final character being
 * placed in the table, the max number of bits allowed
 * and the usagecount of the element.
 */
int TableInsert(Table *t, int code, int prefix, int c, int maxBits, int usage){
    if((*t)->n >= (1 << maxBits)){
        return 0;
    }
    struct elt *e;
    int returned=0;
    unsigned long h;

    e = malloc(sizeof(*e));

    assert(e);

    e->code = code;
    e->prefix = prefix;
    e->c = c;
    e->usagecount = usage;

    if((*t)->n >= (*t)->size){
        //table surpassed max load factor
        //make larger table and rehash all elements
        Table t2 = TableCreate((*t)->size * 2);
        struct elt *temp;
        for(int i=0;i<(*t)->size;i++){
            temp = (*t)->table[i];
            while(temp!=0){
                TableInsert(&t2,temp->code,temp->prefix,
                            temp->c,maxBits,temp->usagecount);
                temp = temp->next;
            }
        }
        TableDestroy((*t));
        (*t) = t2;
        returned = 1;
    }

    h = HASH(prefix,c,(*t)->size);

    e->next = (*t)->table[h];
    (*t)->table[h] = e;

    ((*t)->n)++;
    return returned;
}

/*
 * Returns the code of the element with given prefix and final character
 * from the table given
 */
int TableGet(Table t, int prefix, int c){
    struct elt *e;

    for(e = t->table[HASH(prefix,c,t->size)]; e != 0; e = e->next){
        if(e->prefix==prefix && e->c==c){
            return e->code;
        }
    }

    return -1;
}
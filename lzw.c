/*
 * LZW
 * This is an implementation of Lempel-Ziv-Welch compression,
 * allowing for pruning of the table, input and output tables,
 * and variable numbers of maximum amounts of bits.
 * by: Robert Tung
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include "/c/cs323/Hwk4/code.h"
#include "./lzwHashTable.h"
#include "./fcode.h"
#include <errno.h>

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

#define EMPTY (-1)
#define INITIAL_BITS (9)
#define MAX_MAX_BITS (24)
#define BIT_FLAG (1)
#define PRUNE_FLAG (0)
#define ASCII_TOTAL (256)
#define AFTER_ASCII (258)

/*
 * Function prunes the table and the table array.
 * It takes in the max number of bits allowed,
 * the minimum usage count allowed when pruning,
 * pointers to the table as a hashtable and as an array, and the initial size.
 * It returns the number of bits needed at the end of the pruning.
 */
int pruneTable(long maxBits, long prune, Table *tarr, Table *t, int initSize);

/*
 * This function encodes the input stream.
 * It takes in the max number of bits allowed,
 * strings for the file to print a table to and get a table from,
 * and the minimum usage count allowed when pruning.
 */
void encode(long maxBits, char *out, char *in, long prune);

/*
 * Recursively prints elements starting from their position in the table
 * and tracing back to print all the prefixes.
 * Takes in the table and the final character of the string being printed
 */
void decodePrint(struct elt **arrayTable, int C);

/*
 * This function decodes the input stream sent from encode.
 * It takes in a strings for the file to print a table to.
 */
void decode(char *out);

int main(int argc, char **argv){
	long maxBits=12;//max number of bits allowed
	char *out = 0;//name of file to print table to
	char *in = 0;//name of file to read table from
	long prune=0;//minimum usage count upon pruning
	long currM;//the maxBits value to send to encode
	char *end;//used in strtol to check for errors

	char *program = malloc(sizeof(char) * 7);//name of program being called

	//populate name of program
	for(int i=0;i<6;i++){
		program[i] = argv[0][strlen(argv[0])-6+i];
	}
	program[6] = '\0';

	if(strcmp(program,"encode")==0){
		for(int i=1;i<argc;i++){
			if(strcmp(argv[i],"-m")==0){
				i++;
				if(i < argc){
					//read in m flag
					currM = strtol(argv[i],&end,10);
					if((errno == ERANGE) || ((*end) != '\0')){
						//m flag not a valid long
						fprintf(stderr,"LZW: Error reading in -m flag\n");
						free(program);
						return 1;
					}
				} else{
					//reached end of argument list before m amount
					fprintf(stderr,"LZW: %s needs another argument \n",
							argv[i-1]);
					free(program);
					return 1;
				}
				if(currM <= 0){
					//reached end of argument list before m amount
					fprintf(stderr,"LZW: invalid -m value \n");
					free(program);
					return 1;
				}
				if(currM<=8 || currM>20){
					maxBits = 12;
				} else{
					maxBits = currM;
				}
			} else if(strcmp(argv[i],"-o")==0){
				i++;
				if(i < argc){
					out = argv[i];
				} else{
					//reached end of argument list before out name
					fprintf(stderr,"LZW: %s needs another argument \n",
							argv[i-1]);
					free(program);
					return 1;
				}
			} else if(strcmp(argv[i],"-i")==0){
				i++;
				if(i < argc){
					in = argv[i];
				} else{
					//reached end of argument list before in name
					fprintf(stderr,"LZW: %s needs another argument \n",
							argv[i-1]);
					free(program);
					return 1;
				}
			} else if(strcmp(argv[i],"-p")==0){
				i++;
				if(i < argc){
					//read in p flag
					prune = strtol(argv[i],&end,10);
					if((errno == ERANGE) || ((*end) != '\0')){
						//p flag not a valid long
						fprintf(stderr,"LZW: Error reading in -p flag\n");
						free(program);
						return 1;
					}
					if(prune <= 0){
						//reached end of argument list before m amount
						fprintf(stderr,"LZW: invalid -p value \n");
						free(program);
						return 1;
					}
				} else{
					//reached end of argument list before p amount
					fprintf(stderr,"LZW: %s needs another argument \n",
							argv[i-1]);
					free(program);
					return 1;
				}
			} else{
				//flag is not one of those allowed
				fprintf(stderr,"LZW: %s is not a valid flag\n",
						argv[i]);
				free(program);
				return 1;
			}
		}
		//encode using the flags read in
		encode(maxBits,out,in,prune);
	} else if(strcmp(program,"decode")==0){
		for(int i=1;i<argc;i++){
			if(strcmp(argv[i],"-o")==0){
				i++;
				if(i < argc){
					out = argv[i];
				} else{
					//reached end of argument list before out name
					fprintf(stderr,"LZW: %s needs another argument \n",
							argv[i-1]);
					free(program);
					return 1;
				}
			} else{
				//flag is not one of those allowed
				fprintf(stderr,"LZW: %s is not a valid flag\n",argv[i]);
				free(program);
				return 1;
			}
		}
		//encode using the flags read in
		decode(out);
	} else{
		//name of program is not one of those allowed
		fprintf(stderr,"LZW: argument should call encode or decode\n");
		free(program);
		return 1;
	}
	free(program);
	return 0;
}

int pruneTable(long maxBits, long prune, Table *tarr, Table *t, int initSize){
	Table tnew = TableCreate(1 << initSize);//new hashtable
	Table tarrnew = TableCreate(1 << initSize);//new array version of table
	int endSize = initSize;//number of bits needed at end

	//array whose indices are old codes and values are corresponding new codes
	//used to determine the new prefix of elements when inserting
	int *newCodes = malloc(sizeof(int) * (*t)->n);
	for(int i=0;i<(*t)->n;i++){
		newCodes[i] = -1;
	}

	//initialize our table with ASCII values
	for(int i=2;i<AFTER_ASCII;i++){
		TableInsert(&tnew,i,EMPTY,i-2,maxBits,0);
		TableLinearInsert(&tarrnew,i,EMPTY,i-2,maxBits,i);
		newCodes[i] = i;
	}

	int curr = AFTER_ASCII;//current index being inserted in array
	for(int i=AFTER_ASCII;i<(*tarr)->n;i++){
		//if values usage count is above prune, insert it into new table
		if(((*tarr)->table[i] != 0) && (*tarr)->table[i]->usagecount >= prune){
			//record oldcode-newcode association
			newCodes[(*tarr)->table[i]->code] = tnew->n;
			//somehow the prefix hasn't been associated
			if(newCodes[(*tarr)->table[i]->prefix] == -1){
				fprintf(stderr, "LZW: Table Corrupt\n");
				return -1;
			}
			//insert the element into the table and the array version of the table
			if(TableInsert(&tnew,tnew->n,newCodes[(*tarr)->table[i]->prefix],
							(*tarr)->table[i]->c,maxBits,0)){
				endSize++;
			}
			TableLinearInsert(&tarrnew,tarrnew->n,
							newCodes[(*tarr)->table[i]->prefix],
							(*tarr)->table[i]->c,maxBits,curr);
			curr++;
		}
	}

	//make t and tarr point to the new tables
	TableDestroy((*t));
	TableDestroy((*tarr));
	free(newCodes);
	(*t) = tnew;
	(*tarr) = tarrnew;

	//if we exactly filled up the table
	if((*t)->n == (1 << (endSize))){
		endSize++;
	}
	return endSize;
}

void encode(long maxBits, char *out, char *in, long prune){
	//send the correct flags to decode
	if(in == 0){
		printf("%ld:%ld:%ld:%s\n",maxBits,prune,(unsigned long) 0,"");
	} else{
		printf("%ld:%ld:%ld:%s\n",maxBits,prune,strlen(in),in);
	}
	int start=2;//need 0 and 1 to tell decode to increase numBits and prune
	int tableSize = 1 << INITIAL_BITS;//size of table
	long numBits = INITIAL_BITS;//current number of bits printed
	Table t = TableCreate(tableSize);//create table
	Table tarr = TableCreate(tableSize);//create array table

	//initialize table with ASCII values
	for(int i=start;i<(start+ASCII_TOTAL);i++){
		if(TableInsert(&t,i,EMPTY,i-start,maxBits,0)==1){
			numBits++;
		}
		TableLinearInsert(&tarr,i,EMPTY,i-start,maxBits,i);
	}

	int inP;//prefix read in in-table
	char inC;//character read in in-table
	int inTableRead;//current read from in-table

	if(in != 0){
		FILE *input = fopen(in,"r");
		//read in table from in to get the start value
		if(input){
			while((inTableRead = fgetc(input)) != EOF){
				if(inTableRead != ':'){
					//did not fit style of table that I used
					fprintf(stderr, "LZW: In-Table Corrupt\n");
					TableDestroy(t);
					TableDestroy(tarr);
					exit(1);
					fclose(input);
					return;
				}
				inP = fgetBits(MAX_MAX_BITS,input);
				inC = fgetc(input);
				if((inP < (tarr->size)) && tarr->table[inP] == 0){
					//somehow prefix not in table yet
					fprintf(stderr, "LZW: In-Table Corrupt\n");
					TableDestroy(t);
					TableDestroy(tarr);
					exit(1);
					fclose(input);
					return;
				}
				//insert values into table
				if(TableInsert(&t,t->n,inP,inC,maxBits,0)==1){
					numBits++;
				}
				TableLinearInsert(&tarr,tarr->n,inP,inC,maxBits,tarr->n);
			}
			fclose(input);
		} else{
			//file not opened for whatever reason
			fprintf(stderr, "LZW: Could not open file\n");
			TableDestroy(t);
			TableDestroy(tarr);
			exit(1);
			return;
		}
	}
	int C = EMPTY;//prefix of newly read character
	int K;//newly read character
	int index = EMPTY;//index to insert element into table
	struct elt *e;//element to be inserted into table
	int curr = t->n;//current index for arrayTable

	while((K = getchar()) != EOF){
		index = TableGet(t,C,K);
		if(index != EMPTY){
			//element already in table
			//increment usage count of sequence in array table
		    if((tarr->table[index]->prefix == C) 
		    	&& (tarr->table[index]->c == K)){
		    	(tarr->table[index]->usagecount)++;
		    }
			//increment usage count of sequence in hashtable
			for(e = t->table[HASH(C,K,t->size)]; e != 0; e = e->next){
		        if((e->prefix == C) && (e->c == K)){
		            (e->usagecount)++;
		            break;
		        }
		    }
			C = index;
		} else{
			//element not yet in table
			//print element
			putBits(numBits,C);
			//insert element into table
			if(TableInsert(&t,t->n,C,K,maxBits,0)==1){
				putBits(numBits,BIT_FLAG);
				numBits++;
			}
			TableLinearInsert(&tarr,tarr->n,C,K,maxBits,curr);
			//if we can still insert, increment index to insert into
			if(t->n < (1 << maxBits)){
				curr++;
			}
			//if table has reached max size and it's time to prune
			if(t->size == (1 << maxBits) && t->n == (1 << maxBits) 
				&& (prune != 0)){
				//send code to tell decode to prune
				putBits(numBits,PRUNE_FLAG);
				//prune the table and update the number of bits
				numBits = pruneTable(maxBits,prune,&tarr,&t,INITIAL_BITS);
				//there was an error detected when pruning
				if(numBits == -1){
					TableDestroy(t);
					TableDestroy(tarr);
					exit(1);
					return;
				}
				curr = t->n;
			}
			C = TableGet(t,EMPTY,K);
		}
	}
	//at the very end if we read a value that was in table, still print it
	if(C != EMPTY){
		putBits(numBits,C);
	}
	//print the remaining bits still in table
	flushBits();	
	if(out != 0){
		FILE *output = fopen(out,"w");
		//print table
		if(output){
			for(int i=start+ASCII_TOTAL;i<tarr->n;i++){
				//To check for corruption and to check when we're done
				fputc(':',output);
				fputBits(MAX_MAX_BITS,tarr->table[i]->prefix,output);
				fputc(tarr->table[i]->c,output);
			}
			fclose(output);
		} else{
			//out table not openable
			fprintf(stderr, "LZW: Could not open file\n");
			TableDestroy(t);
			TableDestroy(tarr);
			exit(1);
			return;
		}
	}
	TableDestroy(t);
	TableDestroy(tarr);
}

void decodePrint(struct elt **arrayTable, int C){
	struct elt *e = arrayTable[C];
	if(e->prefix != EMPTY){
		//recursively print prefix then character
		decodePrint(arrayTable,e->prefix);
		printf("%c", (char) e->c);
	} else{
		//earliest character so just print it
		printf("%c", (char) e->c);
	}
}

void decode(char *out){
	long maxBits;//max number of bits allowed
	long prune;//usagecount lower bound for pruning
	long inSize;//size of name of in-table file

	//read in maxBits, prune, and input table name
	if(scanf("%ld:%ld:%ld:",&maxBits,&prune,&inSize) != 3){
		//not all values read in correctly
		fprintf(stderr, "LZW: Stream corrupted\n");
		exit(1);
		return;
	}
	if(inSize < 0){
		//not all values read in correctly
		fprintf(stderr, "LZW: Invalid inSize, Stream corrupted\n");
		exit(1);
		return;
	}

	char *in = 0;//name of file for in-table
	char c;//used to read and ensure format is maintained
	if(inSize == 0){
		in = 0;
	} else{
		in = malloc(sizeof(char)*(inSize+1));
		for(int i=0;i<inSize;i++){
			in[i] = getchar();
		}
		in[inSize] = '\0';
	}
	if((c = getchar())!='\n'){
		//did not fit style of table that I used
		fprintf(stderr, "LZW: Stream corrupted\n");
		if(in != 0){
			free(in);
		}
		exit(1);
		return;
	}

	long numBits = INITIAL_BITS;//number of bits to print out
	int arraySize = 1 << INITIAL_BITS;//size of tables
	Table t = TableCreate(arraySize);
	Table tarr = TableCreate(arraySize);

	int start=2;//0 and 1 reserved to tell decode to increase numBits and prune

	//initialize table with ascii values
	for(int i=start;i<(start+ASCII_TOTAL);i++){
		TableLinearInsert(&tarr,i,EMPTY,i-start,maxBits,i);
		if(TableInsert(&t,i,EMPTY,i-start,maxBits,0)==1){
			numBits++;
		}
	}

	int inP;//prefix read from in-table
	char inC;//character read from in-table
	int inTableRead;//current read from in-table
	if(in != 0){
		FILE *input = fopen(in,"r");
		//read in table from in to get the start value
		if(input){
			while((inTableRead = fgetc(input)) != EOF){
				if(inTableRead != ':'){
					//did not fit style of table that I used
					fprintf(stderr, "LZW: In-Table Corrupt\n");
					free(in);
					TableDestroy(t);
					TableDestroy(tarr);
					exit(1);
					return;
					fclose(input);
				}
				inP = fgetBits(MAX_MAX_BITS,input);
				inC = fgetc(input);
				if((inP < (tarr->size)) && tarr->table[inP] == 0){
					//somehow prefix is not already in the table
					//must be corrupt
					fprintf(stderr, "LZW: In-Table Corrupt\n");
					TableDestroy(t);
					TableDestroy(tarr);
					exit(1);
					fclose(input);
					return;
				}
				TableInsert(&t,t->n,inP,inC,maxBits,0);
				if(TableLinearInsert(&tarr,tarr->n,inP,inC,maxBits,tarr->n)){
					numBits++;
				}
			}
			fclose(input);
		} else{
			//file not openable
			fprintf(stderr, "LZW: Could not open file\n");
			free(in);
			TableDestroy(t);
			TableDestroy(tarr);
			exit(1);
			return;
		}
		free(in);
	}
	int oldC = EMPTY;//previous code
	int newC;//current code
	int C;//current code - changed when tracing stack
	int curr = t->n;//current index in array
	struct elt *e;//element to be inserted
	int currC;//previous code - changed when tracing stack

	while((newC = C = getBits(numBits)) != EOF){
		if(C == 0){
			//code says to prune
			currC = oldC;
			while(currC != EMPTY){
				//increment usagecounts of previous element
				//and all prefixes of element
				for(e = t->table[HASH(tarr->table[currC]->prefix,
									tarr->table[currC]->c,t->size)];
									e != 0; e = e->next){
			        if((e->prefix==tarr->table[currC]->prefix) 
			        	&& (e->c==tarr->table[currC]->c)){
			            (e->usagecount)++;
			            break;
			        }
			    }
				(tarr->table[currC]->usagecount)++;
				currC = tarr->table[currC]->prefix;
			}
			//prune table and 
			numBits = pruneTable(maxBits,prune,&tarr,&t,INITIAL_BITS);
			//error found in pruneTable
			if(numBits == -1){
				TableDestroy(t);
				TableDestroy(tarr);
				exit(1);
				return;
			}
			//update current index and previous code
			curr = t->n;
			oldC = EMPTY;

			//resize tables if needed
			while(tarr->n >= tarr->size){
				tarr->size *= 2;
				tarr->table = realloc(tarr->table,
						sizeof(*tarr->table)*(tarr->size));
				for(int i=(tarr->size)/2;i<tarr->size;i++){
					tarr->table[i] = 0;
				}
			}
			while(t->n >= t->size){
				t->size *= 2;
				t->table = realloc(t->table,sizeof(*t->table)*(t->size));
				for(int i=(t->size)/2;i<t->size;i++){
					t->table[i] = 0;
				}
			}
			continue;
		}
		if(C == 1){
			//code says to increment bits
			numBits++;
			continue;
		}
		if((C < 0) || (C > curr)){
			//code not legal and thus corrupt
			fprintf(stderr, "LZW: Byte Stream corrupt\n");
			TableDestroy(t);
			TableDestroy(tarr);
			exit(1);
			return;
		}
		//if(tarr->table[C]==0){
			//KwKwK
			//assert(curr == tarr->n);
			//assert(C==curr);
			//assert(oldC != EMPTY);
		//}
		if(oldC != EMPTY){
			if(t->n < (1 << maxBits)){
				//table not full so we should insert into arrayTable
				tarr->table[curr] = malloc(sizeof(struct elt));
				tarr->table[curr]->code = curr;
				tarr->table[curr]->usagecount = 0;
				tarr->table[curr]->next = 0;
				tarr->table[curr]->prefix = oldC;
			}
			//update usagecounts
			currC = oldC;
			while(currC != EMPTY){
				for(e = t->table[HASH(tarr->table[currC]->prefix,
					tarr->table[currC]->c,t->size)]; e != 0; e = e->next){
			        if((e->prefix==tarr->table[currC]->prefix) 
			        	&& (e->c==tarr->table[currC]->c)){
			            (e->usagecount)++;
			            break;
			        }
			    }
				(tarr->table[currC]->usagecount)++;
				currC = tarr->table[currC]->prefix;
			}
			if(t->n < (1 << maxBits)){
				//table not full so we should insert into table
				while(tarr->table[C]->prefix != EMPTY){
					C = tarr->table[C]->prefix;
				}
				tarr->table[curr]->c = tarr->table[C]->c;
				(tarr->n)++;
				TableInsert(&t,t->n,tarr->table[curr]->prefix,
							tarr->table[curr]->c,maxBits,0);
				curr++;
			}
			//increase size of tables if needed
			while(curr >= tarr->size){
				tarr->size *= 2;
				tarr->table = realloc(tarr->table,
								sizeof(*tarr->table)*(tarr->size));
				for(int i=(tarr->size)/2;i<tarr->size;i++){
					tarr->table[i] = 0;
				}
			}
			while(curr >= t->size){
				t->size *= 2;
				t->table = realloc(t->table,sizeof(*t->table)*(t->size));
				for(int i=(t->size)/2;i<t->size;i++){
					t->table[i] = 0;
				}
			}
		}
		decodePrint(tarr->table,newC);
		oldC = newC;
	}
	if(out != 0){
		FILE *output = fopen(out,"w");
		//print table
		if(output){
			for(int i=AFTER_ASCII;i<tarr->n;i++){
				//To check for corruption and to check when we're done
				fputc(':',output);
				fputBits(MAX_MAX_BITS,tarr->table[i]->prefix,output);
				fputc(tarr->table[i]->c,output);
			}
			fclose(output);
		} else{
			//file not openable
			fprintf(stderr, "LZW: Could not open file\n");
			TableDestroy(t);
			TableDestroy(tarr);
			exit(1);
			return;
		}
	}
	TableDestroy(t);
	TableDestroy(tarr);
}
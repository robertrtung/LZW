// fcode.c                                         Stan Eisenstat (09/23/09)
//
// Implementation of putBits/getBits described in code.h

#include <stdio.h>
#include <stdlib.h>
#include "/c/cs323/Hwk4/code.h"

// == PUTBITS MODULE =======================================================

// Write CODE (NBITS bits) to standard output
void fputBits (int nBits, int code, FILE *f);

// Flush remaining bits to standard output
void fflushBits (FILE *f);

// == GETBITS MODULE =======================================================

// Return next code (#bits = NBITS) from input stream or EOF on end-of-file
int fgetBits (int nBits, FILE *f);

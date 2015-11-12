// lzw.h                                          Stan Eisenstat (10/19/15)
//
// Header files and macros for encode and decode

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include "/c/cs323/Hwk4/code.h"

// Write message to stderr using format FORMAT
#define WARN(format,...) fprintf (stderr, "LZW: " format "\n", __VA_ARGS__)

// Write message to stderr using format FORMAT and exit.
#define DIE(format,...)  WARN(format,__VA_ARGS__), exit (EXIT_FAILURE)

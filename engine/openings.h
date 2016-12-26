#ifndef OPENINGS_H
#define OPENINGS_H

#include "bitmap.h"
typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;

typedef struct _entry_t {
    BITVAR key;
    uint16 move;
    uint16 weight;
    uint32 learn;
} entry_t;

BITVAR computeKey(board *b, BITVAR *k);
int open_open(char *filename);
void close_open();
int find_moves(board *b, BITVAR key, MOVESTORE *e, int max);
MOVESTORE probe_book(board *b);
#endif

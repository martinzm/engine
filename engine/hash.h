#ifndef HASH_H
#define HASH_H
#include "bitmap.h"

//#define KMOVES_DEPTH 256
#define KMOVES_WIDTH 2

typedef struct _kmoves {
	MOVESTORE move;
	int value;
} kmoves;

void setupRandom(board *b);
void initRandom();
BITVAR getKey(board *b);

void storeHash(hashStore *, hashEntry * hash, int side, int ply, int depth, struct _statistics *);
int retrieveHash(hashStore *, hashEntry *hash, int side, int ply, int depth, int use_previous, struct _statistics *);
void storePVHash(hashStore *, hashEntry * hash, int ply, struct _statistics *);
int initHash(hashStore *);
int invalidateHash(hashStore *);

hashStore * allocateHashStore(int hashLen);
int freeHashStore(hashStore *);

int clear_killer_moves();
int update_killer_move(int ply, MOVESTORE move);
int check_killer_move(int ply, MOVESTORE move);

int generateRandomFile(char *n);

#endif

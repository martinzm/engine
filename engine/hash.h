#ifndef HASH_H
#define HASH_H
#include "bitmap.h"

//#define KMOVES_DEPTH 256
#define KMOVES_WIDTH 2

typedef struct _kmoves {
	MOVESTORE move;
	int value;
} kmoves;

void setupPawnRandom(board *b);
void setupRandom(board *b);
void initRandom();
BITVAR getPawnKey(board *b);
BITVAR getKey(board *b);

void storeHash(hashStore *, hashEntry * hash, int side, int ply, int depth, struct _statistics *);
int retrieveHash(hashStore *, hashEntry *hash, int side, int ply, int depth, int use_previous, struct _statistics *);
void storePawnHash(hashPawnStore *, hashPawnEntry * hash, struct _statistics *);
int retrievePawnHash(hashPawnStore *, hashPawnEntry *hash, struct _statistics *);

void storePVHash(hashStore *, hashEntry * hash, int ply, struct _statistics *);
int initHash(hashStore *);
int invalidateHash(hashStore *);
int initPawnHash(hashPawnStore *);
int invalidatePawnHash(hashPawnStore *);

void storeExactPV(hashStore * hs, BITVAR key, BITVAR map, tree_store * orig, int level);
int restoreExactPV(hashStore * hs, BITVAR key, BITVAR map, int level, tree_store * rest);

hashStore * allocateHashStore(int hashLen, int hashPVLen);
int freeHashStore(hashStore *);

hashPawnStore * allocateHashPawnStore(int hashLen);
int freeHashPawnStore(hashPawnStore *);

int clear_killer_moves();
int update_killer_move(int ply, MOVESTORE move, struct _statistics *);
int check_killer_move(int ply, MOVESTORE move, struct _statistics *);

int generateRandomFile(char *n);

hhTable * allocateHHTable(void);
int freeHHTable(hhTable *);
int clearHHTable(hhTable *);
int updateHHTable(board *, hhTable *, move_entry *, int, int, int, int);
int checkHHTable(hhTable *, int, int, int);

#endif

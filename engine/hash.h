#ifndef HASH_H
#define HASH_H
#include "bitmap.h"

//#define HASHSIZE 491333
#define HASHSIZE 256000
//#define HASHSIZE 16024000
#define HASHPOS 4
#define HASH_RESEED 123456

#define KMOVES_DEPTH 256
#define KMOVES_WIDTH 2

// realisticky depth nebude vice nez 256/512
// age by slo omezit na 64?
// scoretype - jsou jen 4

typedef struct _hashEntry {
	BITVAR key;
	BITVAR map;
	int32_t value; //
	int32_t bestmove; //
	int16_t depth; //
	int8_t age; //
	uint8_t  scoretype;
// 8,8,4,4,2,1,1
} hashEntry;

typedef struct _hashEntry_e {
	hashEntry e[HASHPOS];
} hashEntry_e;

typedef struct _kmoves {
	int move;
	int value;
} kmoves;

void setupRandom(board *b);
void initRandom();
BITVAR getKey(board *b);

void storeHash(hashEntry * hash, int side, int ply, int depth, struct _statistics *);
int retrieveHash(hashEntry *hash, int side, int ply, struct _statistics *);
void storePVHash(hashEntry * hash, int ply, struct _statistics *);
int initHash();
int invalidateHash();

int clear_killer_moves();
int update_killer_move(int ply, int move);
int check_killer_move(int ply, int move);

int generateRandomFile(char *n);

#endif

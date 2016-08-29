 
#include "hash.h"
#include "generate.h"
#include "search.h"
#include "utils.h"
#include "globals.h"
#include "evaluate.h"
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include "randoms.h"

unsigned long long hashStores, hashStoreColl, hashAttempts, hashHits, hashColls, hashMiss, hashStoreMiss, hashStoreInPlace, hashStoreHits;

kmoves kmove_store[KMOVES_DEPTH * KMOVES_WIDTH];

void printHashStats()
{
char buf[512];
	sprintf(buf, "Get:%lld, GHit:%lld,%%%lld, GMiss:%lld, GCol: %lld\n", hashAttempts, hashHits, hashHits*100/(hashAttempts+1), hashMiss, hashColls);
	LOGGER_1("HASH:", buf, "");
	sprintf(buf, "Stores:%lld, SHit:%lld, SInPlace:%lld, SMiss:%lld SCCol:%lld\n",hashStores, hashStoreHits, hashStoreInPlace, hashStoreMiss, hashColls);
	LOGGER_1("HASH:", buf, "");
}

BITVAR getRandom2(int *i)
{
	BITVAR ret;
	unsigned char r,l, res;
	(*i)++;
	int rd = open("/dev/urandom", O_RDONLY);
	l=0;
	ret=0;
	while (l < 8)
	{
		res = read(rd, ((char*)&r), sizeof (unsigned char));
		if (res < 0)
		{
			// error, unable to read /dev/random
		}
		else {
			l += 1;
			ret<<=8;
			ret+=r;
		}
	}
	close(rd);

//	ret=((unsigned long long) rand() << 33)^((unsigned long long) rand() <<16)^((unsigned long long) rand());
	return ret;
}

BITVAR getRandom(int *i)
{
	(*i)++;
	return RANDOMS[*i];
}

// sq, sd, pie = 768
// ep = 8
// sideKey = 1
// castle = 4

void initRandom()
{
int *y;
int sq,sd,pc,f, i;
	y=&i;
	i=-1;
		for(sq=0;sq<ER_SQUARE;sq++)
			for(sd=0;sd<ER_SIDE;sd++) {
				for(pc=0;pc<ER_PIECE;pc++){
					randomTable[sd][sq][pc]=getRandom(y);
//					printf("Hash rand: %d:%d:%d=%llx\n", sq,sd,pc,randomTable[sq][sd][pc]);
				}
			}
		sideKey=getRandom(y);
		for(sq=A1;sq<=H1;sq++) {
			epKey[sq]=getRandom(y);
			epKey[sq+8]=epKey[sq];
			epKey[sq+16]=epKey[sq];
			epKey[sq+24]=epKey[sq];
			epKey[sq+32]=epKey[sq];
			epKey[sq+40]=epKey[sq];
			epKey[sq+48]=epKey[sq];
			epKey[sq+56]=epKey[sq];
		}

		for(sd=0;sd<ER_SIDE;sd++) {
			castleKey[sd][NOCASTLE]=0;
			castleKey[sd][QUEENSIDE]=getRandom(y);
			castleKey[sd][KINGSIDE]=getRandom(y);
			castleKey[sd][BOTHSIDES]=castleKey[sd][QUEENSIDE]^castleKey[sd][KINGSIDE];
		}
		hashTable=(hashEntry_e*) malloc(sizeof(hashEntry_e)*HASHSIZE);
		for(f=0;f<HASHSIZE;f++) {
				for(i=0;i<HASHPOS; i++) {
					hashTable[f].e[i].age=0;
				}
		}
		hashStoreColl=hashStores=hashMiss=hashHits=hashColls=hashAttempts=hashStoreHits=hashStoreMiss=hashStoreInPlace=0;
		hashValidId=1;
}

BITVAR getKey(board *b)
{
BITVAR x;
BITVAR key;
unsigned char from;
                
	key=0;
	x = b->colormaps[WHITE];
	while (x) {
		from = LastOne(x);
		x=ClrNorm(from,x);
		key^=randomTable[WHITE][from][b->pieces[from]];
	}
	x = b->colormaps[BLACK];
	while (x) {
		from = LastOne(x);
		x=ClrNorm(from,x);
		key^=randomTable[BLACK][from][b->pieces[from]&PIECEMASK];
	}
	key^=castleKey[WHITE][b->castle[WHITE]];
	key^=castleKey[BLACK][b->castle[BLACK]];
	if(b->side==BLACK) key^=sideKey;
	if(b->ep!=-1) key^=epKey[b->ep]; 
	return key;
}

void setupRandom(board *b)
{
	b->key=getKey(b);
	b->positions[b->move-b->move_start]=b->key;
	b->posnorm[b->move-b->move_start]=b->norm;
}

/*
	if mated, then score propagated is -MATESCORE+current_DEPTH for in mate position
	we should store score into hash table modified by distance from current depth to depth the mate position occurred
 */

void storeHash(hashEntry * hash, int side, int ply, int depth){
int i, ii,c,q,m,f, isM;

//	return;
	hashStores++;
	
	f=hash->key%HASHSIZE;

	switch(isMATE(hash->value)) {
		case -1:
			hash->value-=ply;
			break;
		case  1:
			hash->value+=ply;
			break;
		default:
			break;
	}

	for(i=0;i<HASHPOS;i++) {
		if((hash->key==hashTable[f].e[i].key)) {
// mame nas zaznam
			hashStoreHits++;
			hashStoreInPlace++;
			c=i;
			if((hashTable[f].e[i].map!=hash->map)) hashStoreColl++;
			goto replace;
		}
	}
	q=9999999;
	for(i=0;i<HASHPOS;i++) {
		if((hashTable[f].e[i].age!=hashValidId)) {
			if(hashTable[f].e[i].depth<q) {
				q=hashTable[f].e[i].depth;
				c=i;
			}
		}
	}
	if(i<HASHPOS) goto replace;

	c=0;
	q=9999999;
	for(i=0;i<HASHPOS;i++) {
		if(hashTable[f].e[i].depth<q) {
			q=hashTable[f].e[i].depth;
			c=i;
		}
	}
//	if(i<HASHPOS) goto replace;

replace:
//	i=c;
//	hashTable[f].e[c].count=c;
	hashTable[f].e[c].depth=hash->depth;
	hashTable[f].e[c].value=hash->value;
	hashTable[f].e[c].key=hash->key;
	hashTable[f].e[c].bestmove=hash->bestmove;
	hashTable[f].e[c].scoretype=hash->scoretype;
	hashTable[f].e[c].age=hashValidId;
	hashTable[f].e[c].map=hash->map;
	if(hash->bestmove==0) {
		printf("error!\n");
	}
}

void storePVHash(hashEntry * hash, int ply){
int i, ii,c,q,m,f, isM;

//	return;
	hashStores++;

	f=hash->key%HASHSIZE;

	switch(isMATE(hash->value)) {
		case -1:
			hash->value-=ply;
			break;
		case  1:
			hash->value+=ply;
			break;
		default:
			break;
	}

	hash->scoretype=NO_SC;
	hash->depth=0;
	for(i=0;i<HASHPOS;i++) {
		if((hash->key==hashTable[f].e[i].key)) {
// mame nas zaznam
			c=i;
			if((hashTable[f].e[i].map==hash->map)) {
				hash->scoretype=hashTable[f].e[i].scoretype;
				hash->depth=hashTable[f].e[i].depth;
			}
			goto replace;
		}
	}
	q=9999999;
	for(i=0;i<HASHPOS;i++) {
		if((hashTable[f].e[i].age!=hashValidId)) {
			if(hashTable[f].e[i].depth<q) {
				q=hashTable[f].e[i].depth;
				c=i;
			}
		}
	}
	if(i<HASHPOS) goto replace;

	c=0;
	q=9999999;
	for(i=0;i<HASHPOS;i++) {
		if(hashTable[f].e[i].depth<q) {
			q=hashTable[f].e[i].depth;
			c=i;
		}
	}
//	if(i<HASHPOS) goto replace;

replace:
//	i=c;
//	hashTable[f].e[c].count=c;
	hashTable[f].e[c].depth=hash->depth;
	hashTable[f].e[c].value=hash->value;
	hashTable[f].e[c].key=hash->key;
	hashTable[f].e[c].bestmove=hash->bestmove;
	hashTable[f].e[c].scoretype=hash->scoretype;
	hashTable[f].e[c].age=hashValidId;
	hashTable[f].e[c].map=hash->map;
	if(hash->bestmove==0) {
		printf("error!\n");
	}
}

int initHash(){
int f,c;

	for(f=0;f<HASHSIZE;f++) {
		for(c=0; c< HASHPOS; c++) {
			hashTable[f].e[c].depth=0;
			hashTable[f].e[c].value=0;
			hashTable[f].e[c].key=0;
			hashTable[f].e[c].bestmove=0;
			hashTable[f].e[c].scoretype=0;
			hashTable[f].e[c].age=0;
			hashTable[f].e[c].map=0;
		}
	}
	return 0;
}

int retrieveHash(hashEntry *hash, int side, int ply)
{
int f,xx,i;

		hashAttempts++;
		xx=0;

		f=hash->key%HASHSIZE;
		for(i=0; i< HASHPOS; i++) {
			if((hashTable[f].e[i].key==hash->key)) {
				if((hashTable[f].e[i].map!=hash->map)) xx=1;
				break;
			}
//			if((hashTable[f].e[i].age==hashValidId) && (hashTable[f].e[i].key==hash->key) && (hashTable[f].e[i].map!=hash->map)) xx=1;
		}

		if(xx==1) {
			hashColls++;
			hashMiss++;
			return 0;
		}
		if(i==HASHPOS) {
			hashMiss++;
			return 0;
		}
		hash->depth=hashTable[f].e[i].depth;
		hash->value=hashTable[f].e[i].value;
		hash->bestmove=hashTable[f].e[i].bestmove;
		hash->scoretype=hashTable[f].e[i].scoretype;
		hash->age=hashTable[f].e[i].age;
// update age aby bylo jasne, ze je to pouzito i ve stavajicim hledani
		hashTable[f].e[i].age=hashValidId;
		hashHits++;

		switch(isMATE(hash->value)) {
			case -1:
				hash->value+=ply;
				break;
			case  1:
				hash->value-=ply;
				break;
			default:
				break;
		}
		return 1;
}

int invalidateHash(){
	hashValidId++;
return 0;
}

int clear_killer_moves(){
int i,f;
kmoves *g;
	killer_moves=kmove_store;
	for(f=0;f<KMOVES_DEPTH;f++) {
		g=&(killer_moves[f*2]);
		for(i=0;i<2;i++) {
			g->move=0;
			g->value=0;
			g++;
		}
	}
return 0;
}

// just 2 killers
int update_killer_move(int ply, int move) {
kmoves *a, *b;
	a=&(killer_moves[ply*2]);
	if(a->move==move) return 1;
	b=a+1;
	*b=*a;
	a->move=move;
return 0;
}

int check_killer_move(int ply, int move) {
kmoves *a, *b;
	a=&(killer_moves[ply*2]);
	b=a+1;
	if(a->move==move) return 1;
	if(b->move==move) return 2;
	if(ply>2) {
// two plies shallower
		a-=4;
		b-=4;
	if(a->move==move) return 3;
	if(b->move==move) return 4;
	}
	return 0;
}

int generateRandomFile(char *n)
{
FILE *h;
unsigned long long i1, i2, i3, i4, i5, i6, i7, i8;
int q, y;

	h=fopen(n, "w");
	q=0;
	fprintf(h, "BITVAR RANDOMS[]= {\n");
	for(y=0;y<99;y++) {
			i1=getRandom(&q);
			i2=getRandom(&q);
			i3=getRandom(&q);
			i4=getRandom(&q);
			i5=getRandom(&q);
			i6=getRandom(&q);
			i7=getRandom(&q);
			i8=getRandom(&q);
			fprintf(h,"\t\t\t\t0x%08llX, 0x%08llX, 0x%08llX, 0x%08llX, 0x%08llX, 0x%08llX, 0x%08llX, 0x%08llX,\n", i1, i2, i3, i4, i5, i6, i7, i8);
	}
	i1=getRandom(&q);
	i2=getRandom(&q);
	i3=getRandom(&q);
	i4=getRandom(&q);
	i5=getRandom(&q);
	i6=getRandom(&q);
	i7=getRandom(&q);
	i8=getRandom(&q);
	fprintf(h,"\t\t\t\t0x%08llX, 0x%08llX, 0x%08llX, 0x%08llX, 0x%08llX, 0x%08llX, 0x%08llX, 0x%08llX\n", i1, i2, i3, i4, i5, i6, i7, i8);
	fprintf(h, "\t\t};");
	fclose(h);

	return 0;
}

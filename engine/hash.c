 
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
	sprintf(buf, "Get:%lld, GHit:%lld,%%%d, GMiss:%lld, GCol: %lld\n", hashAttempts, hashHits, hashHits*100/(hashAttempts+1), hashMiss, hashColls);
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
					hashTable[f].e[i].valid=0;
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
	b->positions[b->rule50move]=b->key;
	b->posnorm[b->rule50move]=b->norm;
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

// muze dojit k situaci kdy je vice klicu na stejne pozici v hash
// pak ke klici jsou ruzne mapy
// v ramci mapy je jina hloubka

// s ma ruzne stavy
// neni tu nic - 0
// klic stejna mapa - 1
// klic stejna mapa, - 2
// klic ruzna mapa - 3


//	printf("HASHSS:%llx:%llx\n", hash->key, hash->map);
//	if(hash->key==0xd6d58975025c5c98 && hash->map==0x4a851458b2b04e92) {
//		printf("SSSSSSSSSSSSSS");
//		c=1;
//	}
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
		if((hashTable[f].e[i].valid==hashValidId)&&(hash->key==hashTable[f].e[i].key)&&(hashTable[f].e[i].map==hash->map)) {
			hashStoreMiss++;
// mame nas zaznam

#if 0
			sprintfMoveSimple(hashTable[f].e[i].bestmove, b2);
			sprintfMoveSimple(hash->bestmove, b3);
			sprintf(b,"%llx, value: n:%d, o:%d, score: n:%d, o:%d, depth: n:%d, o:%d, move: n:%s, o:%s", hashTable[f].e[i].key,  hash->value, hashTable[f].e[i].value, hash->scoretype, hashTable[f].e[i].scoretype, hash->depth, hashTable[f].e[i].depth, b3, b2);
			logger("InPlR:",b,"\n");
#endif
//!!
			if((hashTable[f].e[i].depth<hash->depth) || (hash->scoretype!=FAILLOW_SC)){
				hashTable[f].e[i].depth=hash->depth;
				hashTable[f].e[i].value=hash->value;
				hashTable[f].e[i].bestmove=hash->bestmove;
				hashTable[f].e[i].scoretype=hash->scoretype;
				hashTable[f].e[i].count+=HASHPOS;
				hashStoreInPlace++;
			}
			return;
		}
	}
	for(i=0;i<HASHPOS;i++) {
		if((hashTable[f].e[i].valid==hashValidId)&&(hash->key==hashTable[f].e[i].key)) {
			hashStoreColl++;
// spatna signatura
			break;
		}
	}
//	if(i==HASHPOS) hashStoreMiss++;
// tady nas klic neni...

	c=0;
	q=-1;
	for(ii=0;ii<HASHPOS;ii++) {
		if(hashTable[f].e[ii].valid==hashValidId) {
			if(q==-1) {
				q=ii;
				m=hashTable[f].e[ii].count;
			}
			if(m<hashTable[f].e[ii].count) {
				m=hashTable[f].e[ii].count;
				q=ii;
			}
			if(hashTable[f].e[ii].count>c) {
				c=hashTable[f].e[ii].count;
			}
		}
	}
	c++;

	for(ii=0;ii<HASHPOS;ii++) {
		if((hashTable[f].e[ii].valid!=hashValidId)) break;
	}
	if(ii==HASHPOS) {
		ii=q;
		hashStoreMiss++;
	} else hashStoreHits++;

	hashTable[f].e[ii].count=c;
	hashTable[f].e[ii].depth=hash->depth;
	hashTable[f].e[ii].value=hash->value;
	hashTable[f].e[ii].key=hash->key;
	hashTable[f].e[ii].bestmove=hash->bestmove;
	hashTable[f].e[ii].scoretype=hash->scoretype;
	hashTable[f].e[ii].valid=hashValidId;
	hashTable[f].e[ii].map=hash->map;
	if(hash->bestmove==0) {
		printf("error!\n");
	}
}

int retrieveHash(hashEntry *hash, int side, int ply)
{
int f,xx,i;

		hashAttempts++;
		xx=0;

		f=hash->key%HASHSIZE;
		for(i=0; i< HASHPOS; i++) {
			if((hashTable[f].e[i].valid==hashValidId) && (hashTable[f].e[i].key==hash->key) && (hashTable[f].e[i].map==hash->map)) break;
			if((hashTable[f].e[i].valid==hashValidId) && (hashTable[f].e[i].key==hash->key) && (hashTable[f].e[i].map!=hash->map)) xx=1;
		}

		if(xx==1) {
			hashColls++;
		}
		if(i==HASHPOS) {
			hashMiss++;
			return 0;
		}
		hash->depth=hashTable[f].e[i].depth;
		hash->value=hashTable[f].e[i].value;
		hash->bestmove=hashTable[f].e[i].bestmove;
		hash->scoretype=hashTable[f].e[i].scoretype;
		hash->valid=hashTable[f].e[i].valid;
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

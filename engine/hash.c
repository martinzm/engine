/*
    Carrot is a UCI chess playing engine by Martin Žampach.
    <https://github.com/martinzm/Carrot>     <martinzm@centrum.cz>

    Carrot is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Carrot is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>
*/

#include "generate.h"
#include "search.h"
#include "utils.h"
#include "globals.h"
#include "evaluate.h"
#include "movgen.h"
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include "randoms.h"
//#include "randoms2.h"
#include <assert.h>

kmoves kmove_store[MAXPLY * KMOVES_WIDTH];

#define HASHBITS

BITVAR getRandom2(int *i)
{
	BITVAR ret;
	int l;
	size_t r;
	(*i)++;
	int rd = open("/dev/urandom", O_RDONLY);
	l=0;
	ret=0;
	while (l < 8)
	{
		ssize_t res = read(rd, &r, sizeof (uint8_t));
		if (res < 0)
		{
			// error, unable to read /dev/random
		}
		else {
			l += 1;
			ret<<=8;
			ret+=(uint8_t)r;
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
}

BITVAR getKey(board *b)
{
BITVAR x;
BITVAR key;
int from;
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

void storeHash(hashStore * hs, hashEntry * hash, int side, int ply, int depth, struct _statistics *s){
int i,c,q;
BITVAR f, hi;

//	return;
	s->hashStores++;
	
//	f=hash->key%(BITVAR)hs->hashlen;
//	hi=hash->key/(BITVAR)hs->hashlen;

	f=hash->key & (BITVAR)(hs->hashlen-1);
	hi=hash->key;

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
		if((hi==hs->hash[f].e[i].key)) {
// mame nas zaznam
			s->hashStoreHits++;
			s->hashStoreInPlace++;
			c=i;
			if((hs->hash[f].e[i].map!=hash->map)) s->hashStoreColl++;
			goto replace;
		}
	}
	q=9999999;
	for(i=0;i<HASHPOS;i++) {
		if((hs->hash[f].e[i].age!=hs->hashValidId)) {
			if(hs->hash[f].e[i].depth<q) {
				q=hs->hash[f].e[i].depth;
				c=i;
			}
		}
	}
	if(i<HASHPOS) goto replace;

	c=0;
	q=9999999;
	for(i=0;i<HASHPOS;i++) {
		if(hs->hash[f].e[i].depth<q) {
			q=hs->hash[f].e[i].depth;
			c=i;
		}
	}

replace:
	hs->hash[f].e[c].depth=hash->depth;
	hs->hash[f].e[c].value=hash->value;
	hs->hash[f].e[c].key=hi;
	hs->hash[f].e[c].bestmove=hash->bestmove;
	hs->hash[f].e[c].scoretype=hash->scoretype;
	hs->hash[f].e[c].age=(uint8_t)hs->hashValidId;
	hs->hash[f].e[c].map=hash->map;
}

void storeExactPV(hashStore * hs, BITVAR key, BITVAR map, tree_store * orig, int level){
int i,c,q,n,m;
BITVAR f, hi;
char b2[256], buff[2048];

	if(level>MAXPLY) {
		LOGGER_0("Error Depth: %d\n", level);
		abort();
	}

	f=key%(BITVAR)hs->hashPVlen;
	hi=key/(BITVAR)hs->hashPVlen;

//	LOGGER_0("ExPV: STORE key: 0x%08llX, ply %d, f: 0x%08llX, hi: 0x%08llX, map: 0x%08llX, age: %d\n", key, level, f, hi, map, hs->hashValidId );

	for(i=0;i<16;i++) {
		if((hi==hs->pv[f].e[i].key)) {
// mame nas zaznam
			c=i;
			goto replace;
		}
	}
	for(i=0;i<16;i++) {
		if((hs->pv[f].e[i].age!=hs->hashValidId)) {
			c=i;
			goto replace;
		}
	}
	c=0;
replace:
//	LOGGER_0("ExPV: REPLACING key: c: %d, f: 0x%08llX, hi: 0x%08llX, map: 0x%08llX, age: %d\n",c, f,hs->pv[f].e[c].key, hs->pv[f].e[c].map, hs->pv[f].e[c].age );
	hs->pv[f].e[c].key=hi;
	hs->pv[f].e[c].age=(uint8_t)hs->hashValidId;
	hs->pv[f].e[c].map=map;
	for(n=level,m=0;n<=MAXPLY;n++,m++) {
		hs->pv[f].e[c].pv[m]=orig->tree[level][n];
	}
#if 0
	sprintf(buff,"!!! ");
	for(m=0;m<=50;m++) {
				sprintfMoveSimple(hs->pv[f].e[c].pv[m].move, b2);
				strcat(buff, b2);
				strcat(buff," ");
	}
	LOGGER_0("%s\n",buff);
#endif
}

int restoreExactPV(hashStore * hs, BITVAR key, BITVAR map, int level, tree_store * rest){
int i,q,n,m,c;
BITVAR f, hi;
char buff[2048], b2[256];

	if(level>MAXPLY) {
		LOGGER_0("Error Depth: %d\n", level);
		abort();
	}

	f=key%(BITVAR)hs->hashPVlen;
	hi=key/(BITVAR)hs->hashPVlen;

//	LOGGER_1("ExPV: RESTORING key: 0x%08llX, ply %d, f: 0x%08llX, hi: 0x%08llX, map: 0x%08llX\n", key, level, f, hi, map );

	for(i=0;i<16;i++) {
		if((hi==hs->pv[f].e[i].key)&&(hs->pv[f].e[i].age==hs->hashValidId)&&(hs->pv[f].e[i].map==map)) {
// mame nas zaznam
	sprintf(buff,"!!! ");
			for(n=level+1,m=0;n<=MAXPLY;n++,m++) {
				rest->tree[level][n]=hs->pv[f].e[i].pv[m];
			}
#if 0
		for(m=0;m<=50;m++) {
				sprintfMoveSimple(hs->pv[f].e[i].pv[m].move, b2);
				strcat(buff, b2);
				strcat(buff," ");
		}
	LOGGER_0("%s\n",buff);
#endif
			return 1;
		}
	}
	LOGGER_1("ExPV: NO restore!\n");
	for(c=0;c<16;c++) {
//		LOGGER_0("ExPV: DUMP key: c: %d, f: 0x%08llX, hi: 0x%08llX, map: 0x%08llX, age: %d\n",c, f,hs->pv[f].e[c].key, hs->pv[f].e[c].map, hs->pv[f].e[c].age );
		if(hi==hs->pv[f].e[c].key) {
//			LOGGER_0("ExPV: Key match\n");
			if(hs->pv[f].e[c].age!=hs->hashValidId) LOGGER_0("ExPV: wrong AGE\n");
			if(hs->pv[f].e[c].map!=map) LOGGER_0("ExPV: wrong MAP\n");
		}
	}
	return 0;
}

void storePVHash(hashStore *hs, hashEntry * hash, int ply, struct _statistics *s){
int i,c,q;
BITVAR f, hi;

//	return;
	s->hashStores++;

	f=hash->key%(BITVAR)hs->hashlen;
	hi=hash->key/(BITVAR)hs->hashlen;

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

	hash->scoretype=NO_NULL;
	hash->depth=0;
	for(i=0;i<HASHPOS;i++) {
		if((hi==hs->hash[f].e[i].key)) {
// mame nas zaznam
			c=i;
			if((hs->hash[f].e[i].map==hash->map)) {
				hash->scoretype=hs->hash[f].e[i].scoretype;
				hash->depth=hs->hash[f].e[i].depth;
			}
			goto replace;
		}
	}
	q=9999999;
	for(i=0;i<HASHPOS;i++) {
		if((hs->hash[f].e[i].age!=hs->hashValidId)) {
			if(hs->hash[f].e[i].depth<q) {
				q=hs->hash[f].e[i].depth;
				c=i;
			}
		}
	}
	if(i<HASHPOS) goto replace;
	c=0;
	q=9999999;
	for(i=0;i<HASHPOS;i++) {
		if(hs->hash[f].e[i].depth<q) {
			q=hs->hash[f].e[i].depth;
			c=i;
		}
	}

replace:
	hs->hash[f].e[c].depth=hash->depth;
	hs->hash[f].e[c].value=hash->value;
	hs->hash[f].e[c].key=hi;
	hs->hash[f].e[c].bestmove=hash->bestmove;
	hs->hash[f].e[c].scoretype=hash->scoretype;
	hs->hash[f].e[c].age=(uint8_t)hs->hashValidId;
	hs->hash[f].e[c].map=hash->map;
//	if(hash->bestmove==0) {
//		printf("error!\n");
//	}
}

int initHash(hashStore * hs){
int f,c;

#if 0
	for(f=0;f<hs->hashlen;f++) {
		for(c=0; c< HASHPOS; c++) {
			hs->hash[f].e[c].depth=0;
			hs->hash[f].e[c].value=0;
			hs->hash[f].e[c].key=0;
			hs->hash[f].e[c].bestmove=0;
			hs->hash[f].e[c].scoretype=0;
			hs->hash[f].e[c].age=0;
			hs->hash[f].e[c].map=0;
		}
	}
	for(f=0;f<hs->hashPVlen;f++) {
		for(c=0; c< 16; c++) {
			hs->pv[f].e[c].key=0;
			hs->pv[f].e[c].age=0;
			hs->pv[f].e[c].map=0;
		}
	}
#endif

	memset(hs->hash, 0, sizeof(hashEntry_e)*hs->hashlen);
	memset(hs->pv, 0, sizeof(hashEntryPV_e)*hs->hashPVlen);
	hs->hashValidId=1;
	return 0;
}

int retrieveHash(hashStore *hs, hashEntry *hash, int side, int ply, int depth, int use_previous, struct _statistics *s)
{
int xx,i;
BITVAR f,hi;
		s->hashAttempts++;
		xx=0;

//		LOGGER_0("Get Hash Hit!\n");

//		f=hash->key%(BITVAR)hs->hashlen;
//		hi=hash->key/(BITVAR)hs->hashlen;

		f=hash->key & (BITVAR)(hs->hashlen-1);
		hi=hash->key;

		for(i=0; i< HASHPOS; i++) {
			if((hs->hash[f].e[i].key==hi) && (hs->hash[f].e[i].map==hash->map) && ((use_previous>0)||((use_previous==0)&&(hs->hash[f].e[i].age==hs->hashValidId)))) break;
		}
		if(i==HASHPOS) {
			s->hashMiss++;
			return 0;
		}
		*hash=(hs->hash[f].e[i]);
// update age aby bylo jasne, ze je to pouzito i ve stavajicim hledani
		if(depth<hash->depth) hs->hash[f].e[i].age=(uint8_t)hs->hashValidId;
		s->hashHits++;

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

void dumpHash(board *b, hashStore *hs, hashEntry *hash, int side, int ply, int depth, int use_previous){
BITVAR k;
int xx,i;
BITVAR f,hi;
char b2[10];

	k=getKey(b);
	printBoardNice(b);
		xx=0;

		f=hash->key%(BITVAR)hs->hashlen;
		hi=hash->key/(BITVAR)hs->hashlen;
		for(i=0; i< HASHPOS; i++) {
			if((hs->hash[f].e[i].key==hi) && (hs->hash[f].e[i].map==hash->map) && ((use_previous>0)||((use_previous==0)&&(hs->hash[f].e[i].age==hs->hashValidId)))) break;
		}
		if(i==HASHPOS) {
		  LOGGER_0("HASH MISS\n");
		} else {
				LOGGER_0("hash HI entry key %lld, hi %lld ",hs->hash[f].e[i].key,hi); if(hs->hash[f].e[i].key==hi) LOGGER_0("match\n"); else LOGGER_0("Fail\n");
				LOGGER_0("hash MAP entry map %lld, map %lld ",hs->hash[f].e[i].map,hash->map); if(hs->hash[f].e[i].map==hash->map) LOGGER_0("match\n");else LOGGER_0("Fail\n");
				LOGGER_0("hash Previous %d\n", use_previous);
				LOGGER_0("hash AGE entry map %lld, ageID %lld ", hs->hash[f].e[i].age,hs->hashValidId); if(hs->hash[f].e[i].age==hs->hashValidId) LOGGER_0("match\n");else LOGGER_0("Fail\n");
				LOGGER_0("hash DEPTH %d\n",hash->depth);
				LOGGER_0("hash MATE %d\n", isMATE(hash->value));
				sprintfMoveSimple(hash->bestmove, b2);
				LOGGER_0("hash VAL %d, hash MOVE %s\n", hash->value, b2);
				LOGGER_0("hash HASH material key used %lld, recomputed %lld", hash->key, k); if(hash->key == k) LOGGER_0("match\n");else LOGGER_0("Fail\n");
		}
}

int invalidateHash(hashStore *hs){
// check for NULL should be part of a caller!
	if(hs!=NULL) {
		hs->hashValidId++;
		if(hs->hashValidId>63) hs->hashValidId=1;
	}
return 0;
}

int clear_killer_moves(kmoves *km){
int i,f;
kmoves *g;
	for(f=0;f<MAXPLY;f++) {
		g=&(km[f*KMOVES_WIDTH]);
		for(i=0;i<KMOVES_WIDTH;i++) {
			g->move=NA_MOVE;
			g->value=0;
			g++;
		}
	}
return 0;
}

// just 2 killers
int update_killer_move(kmoves *km, int ply, MOVESTORE move, struct _statistics *s) {
kmoves *a, *b;
	a=&(km[ply*KMOVES_WIDTH]);
	if(a->move==move) return 1;
	b=a+1;
	*b=*a;
	a->move=move;
return 0;
}

int get_killer_move(kmoves *km, int ply, int id, MOVESTORE *move) {
kmoves *a;
int i;
char b2[256];

	assert(id<KMOVES_WIDTH);
	assert(ply<MAXPLY);
	*move = (km+ ply*KMOVES_WIDTH+id)->move;
//	sprintfMoveSimple(*move, b2);
//	LOGGER_0("Killer_move %s\n", b2);
	if(*move==NA_MOVE) return 0;
return 1;
}

kmoves *allocateKillerStore(void){
kmoves *m;
	m = (kmoves *) malloc(sizeof(kmoves)*KMOVES_WIDTH*MAXPLY);
	if(m==NULL) abort();
	clear_killer_moves(m);
	return m;
}

int freeKillerStore(kmoves *m){
	free(m);
	return 1;
}

hashStore * allocateHashStore(size_t hashBytes, int hashPVLen) {
hashStore * hs;

int hl, hp;
int msk=1;
size_t hashLen;

	hashLen= hashBytes/sizeof(hashEntry_e);
// just make sure hashLen is power of 2
	while(msk<hashLen){
		msk<<=1;
		msk|=1;
	}
	msk>>=1;
	msk++;
	LOGGER_0("HASHLEN %d, msk %d\n", hashLen, msk);
	hashLen=msk;
	hl=hashLen;
	hp=hashPVLen;
	hs = (hashStore *) aligned_alloc(128, sizeof(hashStore)*2 + sizeof(hashEntry_e)*hl + sizeof(hashEntryPV_e)*hp);
	hs->hashlen=hashLen;
	hs->hash = (hashEntry_e*) (hs+1);
	hs->hashPVlen=hashPVLen;
	hs->pv= (hashEntryPV_e*) (hs->hash+hashLen);
	initHash(hs);

return hs;
}

hashPawnStore * allocateHashPawnStore(size_t hashBytes) {
hashPawnStore * hs;

size_t hl, hp;
size_t hashLen;
int msk=1;

	hashLen= hashBytes/sizeof(hashPawnEntry_e);
// just make sure hashLen is power of 2
	while(msk<hashLen){
		msk<<=1;
		msk|=1;
	}
	msk>>=1;
	msk++;
	LOGGER_0("HASHLEN %d, msk %d\n", hashLen, msk);
	hashLen=msk;

	hl=hashLen;
	hs = (hashPawnStore *) aligned_alloc(128, sizeof(hashPawnStore)*2 + sizeof(hashPawnEntry_e)*hl);
	hs->hashlen=hashLen;
	hs->hash = (hashPawnEntry_e*) (hs+1);
	initPawnHash(hs);

return hs;
}

int freeHashStore(hashStore *hs)
{
	free(hs);
	return 1;
}

int freeHashPawnStore(hashPawnStore *hs)
{
	free(hs);
	return 1;
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

hashPawnEntry * storePawnHash(hashPawnStore * hs, hashPawnEntry * hash, struct _statistics *s){
int i,c,q;
BITVAR f, hi;

	s->hashPawnStores++;	
	f=hash->key%(BITVAR)hs->hashlen;
	hi=hash->key/(BITVAR)hs->hashlen;

	for(i=0;i<HASHPAWNPOS;i++) {
		if((hi==hs->hash[f].e[i].key)) {
// mame nas zaznam
			s->hashPawnStoreHits++;
			s->hashPawnStoreInPlace++;
			c=i;
			if((hs->hash[f].e[i].map!=hash->map)) s->hashPawnStoreColl++;
			goto replace;
		}
	}
	for(i=0;i<HASHPAWNPOS;i++) {
		if((hs->hash[f].e[i].age!=hs->hashValidId)) {
				c=i;
		}
	}
	if(i<HASHPAWNPOS) goto replace;
	c=0;
replace:
	hs->hash[f].e[c].value=hash->value;
	hs->hash[f].e[c].age=(uint8_t)hs->hashValidId;
	hs->hash[f].e[c].map=hash->map;
	hs->hash[f].e[c].key=hi;
	return &(hs->hash[f].e[c]);
}

int invalidatePawnHash(hashPawnStore *hs){
	hs->hashValidId++;
	if(hs->hashValidId>63) hs->hashValidId=1;
return 0;
}

int initPawnHash(hashPawnStore * hs){
int f,c;

	for(f=0;f<hs->hashlen;f++) {
		for(c=0; c< HASHPAWNPOS; c++) {
			hs->hash[f].e[c].key=0;
			hs->hash[f].e[c].age=0;
			hs->hash[f].e[c].map=0;
		}
	}
	hs->hashValidId=1;
	return 0;
}

hashPawnEntry * retrievePawnHash(hashPawnStore *hs, hashPawnEntry *hash, struct _statistics *s)
{
int xx,i;
BITVAR f,hi;
	s->hashPawnAttempts++;
//	xx=0;
	f=hash->key%(BITVAR)hs->hashlen;
	hi=hash->key/(BITVAR)hs->hashlen;
	for(i=0; i< HASHPAWNPOS; i++) {
		if((hs->hash[f].e[i].key==hi) && (hs->hash[f].e[i].map==hash->map) &&
			(hs->hash[f].e[i].age==hs->hashValidId)) break;
		}
	if(i==HASHPAWNPOS) {
		s->hashPawnMiss++;
		return NULL;
	}
//	*hash=hs->hash[f].e[i];
//	memcpy(hash, &(hs->hash[f].e[i]), sizeof(hashPawnEntry));
	s->hashPawnHits++;
	return &(hs->hash[f].e[i]);
}

void setupPawnRandom(board *b)
{
	b->pawnkey=getPawnKey(b);
}

BITVAR getPawnKey(board *b)
{
BITVAR x;
BITVAR key;
int from;
	key=0;
//	x = b->colormaps[WHITE]&(b->maps[PAWN]|b->maps[KING]);
	x = b->colormaps[WHITE]&(b->maps[PAWN]);
	while (x) {
		from = LastOne(x);
		x=ClrNorm(from,x);
		key^=randomTable[WHITE][from][b->pieces[from]];
	}
//	x = b->colormaps[BLACK]&(b->maps[PAWN]|b->maps[KING]);
	x = b->colormaps[BLACK]&(b->maps[PAWN]);
	while (x) {
		from = LastOne(x);
		x=ClrNorm(from,x);
		key^=randomTable[BLACK][from][b->pieces[from]&PIECEMASK];
	}
	return key;
}

hhTable * allocateHHTable(void) {
hhTable *hh;
	hh = (hhTable *) malloc(sizeof(hhTable));
	if(hh==NULL) abort();
	return hh;
}

int freeHHTable(hhTable *hh) {
	free(hh);
	return 0;
}

int clearHHTable(hhTable *hh) {
	int f,q;
	for(q=PAWN;q<ER_PIECE;q++) {
		for(f=0;f<64;f++) hh->val[0][q][f]=hh->val[1][q][f]=0;
	}
	return 0;
}

int updateHHTable(board *b, hhTable *hh, move_entry *m, int cutoff, int side, int depth, int ply){
int fromPos, toPos, piece, val;

	fromPos=UnPackFrom(m[cutoff].move);
	toPos=UnPackTo(m[cutoff].move);
	piece=b->pieces[fromPos]&PIECEMASK;
	hh->val[side][piece][toPos]+=(depth*depth);
	return 0;
}

int checkHHTable(hhTable *hh, int side, int piece, int square){
	return hh->val[side][piece][square];
}

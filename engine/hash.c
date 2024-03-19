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
	l = 0;
	ret = 0;
	while (l < 8) {
		ssize_t res = read(rd, &r, sizeof(uint8_t));
		if (res < 0) {
			// error, unable to read /dev/random
		} else {
			l += 1;
			ret <<= 8;
			ret += (uint8_t) r;
		}
	}
	close(rd);

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
	int sq, sd, pc, i;
	y = &i;
	i = -1;
	for (sq = 0; sq < ER_SQUARE; sq++)
		for (sd = 0; sd < ER_SIDE; sd++) {
			for (pc = 0; pc < ER_PIECE; pc++) {
				randomTable[sd][sq][pc] = getRandom(y);
			}
		}
	sideKey = getRandom(y);
	for (sq = A1; sq <= H1; sq++) {
		epKey[sq] = getRandom(y);
		epKey[sq + 8] = epKey[sq];
		epKey[sq + 16] = epKey[sq];
		epKey[sq + 24] = epKey[sq];
		epKey[sq + 32] = epKey[sq];
		epKey[sq + 40] = epKey[sq];
		epKey[sq + 48] = epKey[sq];
		epKey[sq + 56] = epKey[sq];
	}
		epKey[0] = 0;

	for (sd = 0; sd < ER_SIDE; sd++) {
		castleKey[sd][NOCASTLE] = 0;
		castleKey[sd][QUEENSIDE] = getRandom(y);
		castleKey[sd][KINGSIDE] = getRandom(y);
		castleKey[sd][BOTHSIDES] = castleKey[sd][QUEENSIDE]
			^ castleKey[sd][KINGSIDE];
	}
}

BITVAR getKey(board *b)
{
	BITVAR x;
	BITVAR key;
	int from;
	key = 0;
	x = b->colormaps[WHITE];
	while (x) {
		from = LastOne(x);
		x = ClrNorm(from, x);
		key ^= randomTable[WHITE][from][b->pieces[from]];
	}
	x = b->colormaps[BLACK];
	while (x) {
		from = LastOne(x);
		x = ClrNorm(from, x);
		key ^= randomTable[BLACK][from][b->pieces[from] & PIECEMASK];
	}
	key ^= castleKey[WHITE][b->castle[WHITE]];
	key ^= castleKey[BLACK][b->castle[BLACK]];
	if (b->side == BLACK)
		key ^= sideKey;
	if (b->ep != -1)
		key ^= epKey[b->ep];
	return key;
}

void setupRandom(board *b)
{
	b->key = getKey(b);
	b->positions[b->move - b->move_start] = b->key;
	b->posnorm[b->move - b->move_start] = b->norm;
}

/*
 if mated, then score propagated is -MATESCORE+current_DEPTH for in mate position
 we should store score into hash table modified by distance from current depth to depth the mate position occurred
 */

void storeHash(hashStore *hs, hashEntry *hash, int side, int ply, int depth, BITVAR ver, struct _statistics *s)
{
	int i, c, q;
	BITVAR f, hi;
	BITVAR pld;
	BITVAR *h;

	s->hashStores++;
	f = hash->key & (BITVAR) (hs->hashlen - 1);
	hi = hash->key;
	h=hs->hash+3*f*HASHPOS;

	switch (isMATE(hash->value)) {
	case -1:
		hash->value -= ply;
		break;
	case 1:
		hash->value += ply;
		break;
	default:
		break;
	}

	c=HASHPOS- 3;
// hs->hash je 2x BITVAR, prvni je klic, druhy je kompozit

	for (i = 0; i < HASHPOS; i += 3) {
		if ((hi == h[i]) && (h[i+2]==ver)) {
// mame nas zaznam
			s->hashStoreHits++;
			s->hashStoreInPlace++;
			c = i;
			goto replace;
		}
	}
	for (i = HASHPOS- 3; i >= 0; i -= 3) {
		if ((h[i+1] & 0x3F) != hs->hashValidId) {
				c = i;
				goto replace;
		}
	}
	q=0x1FF+1;
	for (i = 0; i < HASHPOS; i+= 3) {
		int qq=(h[i+1]>>(6+32+15)) & 0x1FFUL;
		if (qq < q) {
			q = qq;
			c = i;
		}
	}

replace:
	PACKHASH(pld, hash->bestmove, hash->value, hash->depth, hash->scoretype, hs->hashValidId);

/*
	pld=
		((hash->scoretype & 0x3UL) << (6+32+15+9))|
		((hash->depth & 0x1FFUL) << (6+32+15))|
		((hash->bestmove & 0x7FFFUL) << (6+32))|
		((hash->value & 0xFFFFFFFFUL)<<6)|
		(hs->hashValidId & 0x3FUL);
*/

	h[c]=hi;
	h[c+1]=pld;

	h[c+2]=ver;
}
// 6 32 15 9 2, 0 6 38 53 62 64
void storeExactPV(hashStore *hs, BITVAR key, BITVAR ver, tree_store *orig, int level)
{
	int i, c, n, m;
	BITVAR f, hi;

	if (level > MAXPLY) {
		LOGGER_0("Error Depth: %d\n", level);
		abort();
	}

	f = key % (BITVAR) hs->hashPVlen;
	hi = key / (BITVAR) hs->hashPVlen;
	for (i = 0; i < 16; i++) {
		if ((hi == hs->pv[f].e[i].key)) {
// mame nas zaznam
			c = i;
			goto replace;
		}
	}
	for (i = 0; i < 16; i++) {
		if ((hs->pv[f].e[i].age != hs->hashValidId)) {
			c = i;
			goto replace;
		}
	}
	c = 0;
	replace: hs->pv[f].e[c].key = hi;
	hs->pv[f].e[c].age = (uint8_t) hs->hashValidId;
//	hs->pv[f].e[c].map = map;
	for (n = level, m = 0; n <= MAXPLY; n++, m++) {
		hs->pv[f].e[c].pv[m] = orig->tree[level][n];
	}
}

int restoreExactPV(hashStore *hs, BITVAR key, BITVAR ver, int level, tree_store *rest)
{
	int i, n, m, c;
	BITVAR f, hi;
	char buff[2048];

	if (level > MAXPLY) {
		LOGGER_0("Error Depth: %d\n", level);
		abort();
	}

	f = key % (BITVAR) hs->hashPVlen;
	hi = key / (BITVAR) hs->hashPVlen;
	for (i = 0; i < 16; i++) {
		if ((hi == hs->pv[f].e[i].key) && (hs->pv[f].e[i].age == hs->hashValidId)) {
// mame nas zaznam
//			sprintf(buff, "!!! ");
			for (n = level + 1, m = 0; n <= MAXPLY; n++, m++) {
				rest->tree[level][n] = hs->pv[f].e[i].pv[m];
			}
			return 1;
		}
	} LOGGER_1("ExPV: NO restore!\n");
	for (c = 0; c < 16; c++) {
		if (hi == hs->pv[f].e[c].key) {
			if (hs->pv[f].e[c].age != hs->hashValidId)
				LOGGER_0("ExPV: wrong AGE\n");
		}
	}
	return 0;
}

void storePVHash(hashStore *hs, hashEntry *hash, int ply, BITVAR ver, struct _statistics *s)
{
	hash->scoretype = NO_NULL;
	hash->depth = 0;
//	storeHash(hs, hash, -1, ply, 0, ver, s);
}

int initHash(hashStore *hs)
{
	memset(hs->hash, 0, sizeof(BITVAR) *3U * hs->hashlen);
	memset(hs->pv, 0, sizeof(hashEntryPV_e) * hs->hashPVlen);
	hs->hashValidId = 1;
	return 0;
}

int retrieveHash(hashStore *hs, hashEntry *hash, int side, int ply, int depth, int use_previous, BITVAR ver,struct _statistics *s)
{
	int i;
	BITVAR f, hi, pld;
	BITVAR *h;
	
	s->hashAttempts++;
	f = hash->key & (BITVAR) (hs->hashlen - 1);
	hi = hash->key;
	h=hs->hash+ 3 *f*HASHPOS;

	for (i = 0; i < HASHPOS; i+= 3) {
		if ((h[i] == hi)
			&& (h[i+2]==ver)
			&& ((use_previous > 0)
				|| ((use_previous == 0)
					&& ((h[i+1]& 0x3F)
						== hs->hashValidId))))
			break;
	}
	if (i >= HASHPOS) {
		s->hashMiss++;
		return 0;
	}
	
	pld=h[i+1];
/*
	hash->scoretype = (pld>>62)&0x3UL;
	hash->depth = (pld>>53)& 0x1FFUL;
	hash->bestmove = (pld>>38)& 0x7FFFUL;
	hash->value = (pld>>6)& 0xFFFFFFFFUL;
	hash->age = pld & 0x3FUL;
*/
	
	UNPACKHASH(pld, hash->bestmove, hash->value, hash->depth, hash->scoretype, hash->age)
	
	h[i+1]= (pld & 0xFFFFFFFFFFFFFFC0) | (hs->hashValidId & 0x3F);
	h[i+2]= ver;
	
	s->hashHits++;

	switch (isMATE(hash->value)) {
	case -1:
		hash->value += ply;
		break;
	case 1:
		hash->value -= ply;
		break;
	default:
		break;
	}
	return 1;
}

void dumpHash(board *b, hashStore *hs, hashEntry *hash, int side, int ply, int depth, int use_previous)
{
#if 0
	BITVAR k;
	int i;
	BITVAR f, hi;
	char b2[10];

	k = getKey(b);
	printBoardNice(b);
	f = hash->key % (BITVAR) hs->hashlen;
	hi = hash->key / (BITVAR) hs->hashlen;
	for (i = 0; i < HASHPOS; i++) {
		if ((hs->hash[f].e[i].key == hi)
			&& ((use_previous > 0)
				|| ((use_previous == 0)
					&& (hs->hash[f].e[i].age
						== hs->hashValidId))))
			break;
	}
	if (i == HASHPOS) {
		LOGGER_0("HASH MISS\n");
	} else {
		LOGGER_0("hash HI entry key %lld, hi %lld ",
			hs->hash[f].e[i].key, hi);
		if (hs->hash[f].e[i].key == hi)
			LOGGER_0("match\n");
		else
			LOGGER_0("Fail\n");
		LOGGER_0("hash Previous %d\n", use_previous);
		LOGGER_0("hash AGE entry map %lld, ageID %lld ",
			hs->hash[f].e[i].age, hs->hashValidId);
		if (hs->hash[f].e[i].age == hs->hashValidId)
			LOGGER_0("match\n");
		else
			LOGGER_0("Fail\n");
		LOGGER_0("hash DEPTH %d\n", hash->depth);
		LOGGER_0("hash MATE %d\n", isMATE(hash->value));
		sprintfMoveSimple(hash->bestmove, b2);
		LOGGER_0("hash VAL %d, hash MOVE %s\n", hash->value, b2);
		LOGGER_0("hash HASH material key used %lld, recomputed %lld",
			hash->key, k);
		if (hash->key == k)
			LOGGER_0("match\n");
		else
			LOGGER_0("Fail\n");
	}
#endif
}

int invalidateHash(hashStore *hs)
{
// check for NULL should be part of a caller!
	if (hs != NULL) {
		hs->hashValidId++;
		if (hs->hashValidId > 63)
			hs->hashValidId = 1;
	}
	return 0;
}

int clear_killer_moves(kmoves *km)
{
	int i, f;
	kmoves *g;
	for (f = 0; f < MAXPLY; f++) {
		g = &(km[f * KMOVES_WIDTH]);
		for (i = 0; i < KMOVES_WIDTH; i++) {
			g->move = NA_MOVE;
			g->value = 0;
			g++;
		}
	}
	return 0;
}

// just 2 killers
int update_killer_move(kmoves *km, int ply, MOVESTORE move, struct _statistics *s)
{
	kmoves *a, *b;
	a = &(km[ply * KMOVES_WIDTH]);
	if (a->move == move)
		return 1;
	b = a + 1;
	*b = *a;
	a->move = move;
	return 0;
}

int get_killer_move(kmoves *km, int ply, int id, MOVESTORE *move)
{

	assert(id < KMOVES_WIDTH);
	assert(ply < MAXPLY);
	*move = (km + ply * KMOVES_WIDTH + id)->move;
	if (*move == NA_MOVE)
		return 0;
	return 1;
}

kmoves* allocateKillerStore(void)
{
	kmoves *m;
	m = (kmoves*) malloc(sizeof(kmoves) * KMOVES_WIDTH * MAXPLY);
	if (m == NULL)
		abort();
	clear_killer_moves(m);
	return m;
}

int freeKillerStore(kmoves *m)
{
	free(m);
	return 1;
}

hashStore* allocateHashStore(size_t hashBytes, unsigned int hashPVLen)
{
	hashStore *hs;

	unsigned int msk = 0;
	size_t hashLen, hl, hp;
	size_t xx;
	int l=0;

//	hashLen = hashBytes / sizeof(hashEntry_e);
	hashLen = hashBytes / (HASHPOS*2*sizeof(BITVAR));
// just make sure hashLen is power of 2
	while (msk <= hashLen) {
		msk <<= 1;
		msk |= 1;
		l++;
	}
	msk >>= 1;
	l--;
	msk++;
	LOGGER_0("Bytes: %d, hashEntry %d, HASHLEN %o, msk %o, mask len %d\n", hashBytes, sizeof(hashEntry_e), hashLen, msk, l);
	hashLen = msk;
	hl = hashLen;
	hp = (size_t) hashPVLen;
	xx = (((sizeof(hashStore) * 2 + sizeof(BITVAR)*3 * hl * HASHPOS + sizeof(hashEntryPV_e) * hp))+0xFFU);
	hs = (hashStore*) aligned_alloc(128,xx);
	hs->hashlen = (size_t) hashLen;
	hs->llen=l;
	hs->hash = (BITVAR*) (hs + 1);
	hs->hashPVlen = hashPVLen;
	hs->pv = (hashEntryPV_e*) (hs->hash + hashLen);
	initHash(hs);

	return hs;
}

hashPawnStore* allocateHashPawnStore(size_t hashBytes)
{
	hashPawnStore *hs;

	size_t hl;
	size_t hashLen;
	int msk = 0;
	int l=0;

	hashLen = hashBytes / sizeof(hashPawnEntry_e);
// just make sure hashLen is power of 2
	while (msk <= hashLen) {
		msk <<= 1;
		msk |= 1;
		l++;
	}
	msk >>= 1;
	l--;
	msk++;
	LOGGER_0("HASHLEN %d, msk %d\n", hashLen, msk);
	hashLen = msk;

	hl = hashLen;
	hs = (hashPawnStore*) aligned_alloc(128,(
		sizeof(hashPawnStore) * 2 + sizeof(hashPawnEntry_e) * hl + 0XFFL)&(~0xFFUL));
	hs->hashlen = hashLen;
	hs->llen=l;
	hs->hash = (hashPawnEntry_e*) (hs + 1);
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

	h = fopen(n, "w");
	q = 0;
	fprintf(h, "BITVAR RANDOMS[]= {\n");
	for (y = 0; y < 99; y++) {
		i1 = getRandom(&q);
		i2 = getRandom(&q);
		i3 = getRandom(&q);
		i4 = getRandom(&q);
		i5 = getRandom(&q);
		i6 = getRandom(&q);
		i7 = getRandom(&q);
		i8 = getRandom(&q);
		fprintf(h,
			"\t\t\t\t0x%08llX, 0x%08llX, 0x%08llX, 0x%08llX, 0x%08llX, 0x%08llX, 0x%08llX, 0x%08llX,\n",
			i1, i2, i3, i4, i5, i6, i7, i8);
	}
	i1 = getRandom(&q);
	i2 = getRandom(&q);
	i3 = getRandom(&q);
	i4 = getRandom(&q);
	i5 = getRandom(&q);
	i6 = getRandom(&q);
	i7 = getRandom(&q);
	i8 = getRandom(&q);
	fprintf(h,
		"\t\t\t\t0x%08llX, 0x%08llX, 0x%08llX, 0x%08llX, 0x%08llX, 0x%08llX, 0x%08llX, 0x%08llX\n",
		i1, i2, i3, i4, i5, i6, i7, i8);
	fprintf(h, "\t\t};");
	fclose(h);

	return 0;
}

hashPawnEntry* storePawnHash(hashPawnStore *hs, hashPawnEntry *hash, BITVAR ver, struct _statistics *s)
{
	int i, c;
	BITVAR f, hi;

	s->hashPawnStores++;
	f = hash->key % (BITVAR) hs->hashlen;
	hi = hash->key / (BITVAR) hs->hashlen;

	for (i = 0; i < HASHPAWNPOS; i++) {
		if ((hi == hs->hash[f].e[i].key)) {
// mame nas zaznam
			s->hashPawnStoreHits++;
			s->hashPawnStoreInPlace++;
			c = i;
			goto replace;
		}
	}
	c=0;
	for (i = 0; i < HASHPAWNPOS; i++) {
		if ((hs->hash[f].e[i].age != hs->hashValidId)) {
			c = i;
			break;
		}
	}
//	if (i >= HASHPAWNPOS) c = 0;
	replace: hs->hash[f].e[c].value = hash->value;
	hs->hash[f].e[c].age = (uint8_t) hs->hashValidId;
	hs->hash[f].e[c].key = hi;
	return &(hs->hash[f].e[c]);
}

int invalidatePawnHash(hashPawnStore *hs)
{
	if(hs!=NULL) {
		hs->hashValidId++;
		if (hs->hashValidId > 63)
			hs->hashValidId = 1;
	}
	return 0;
}

int initPawnHash(hashPawnStore *hs)
{
	int f, c;

	for (f = 0; f < hs->hashlen; f++) {
		for (c = 0; c < HASHPAWNPOS; c++) {
			hs->hash[f].e[c].key = 0;
			hs->hash[f].e[c].age = 0;
			hs->hash[f].e[c].map = 0;
		}
	}
	hs->hashValidId = 1;
	return 0;
}

hashPawnEntry* retrievePawnHash(hashPawnStore *hs, hashPawnEntry *hash, BITVAR ver, struct _statistics *s)
{
	int i;
	BITVAR f, hi;
	s->hashPawnAttempts++;
	f = hash->key % (BITVAR) hs->hashlen;
	hi = hash->key / (BITVAR) hs->hashlen;
	for (i = 0; i < HASHPAWNPOS; i++) {
		if ((hs->hash[f].e[i].key == hi)
			&& (hs->hash[f].e[i].age == hs->hashValidId))
			break;
	}
	if (i == HASHPAWNPOS) {
		s->hashPawnMiss++;
		return NULL;
	}
	s->hashPawnHits++;
	return &(hs->hash[f].e[i]);
}

void setupPawnRandom(board *b)
{
	b->pawnkey = getPawnKey(b);
}

BITVAR getPawnKey(board *b)
{
	BITVAR x;
	BITVAR key;
	int from;
	key = 0;
	x = b->colormaps[WHITE] & (b->maps[PAWN]);
	while (x) {
		from = LastOne(x);
		x = ClrNorm(from, x);
		key ^= randomTable[WHITE][from][b->pieces[from]];
	}
	x = b->colormaps[BLACK] & (b->maps[PAWN]);
	while (x) {
		from = LastOne(x);
		x = ClrNorm(from, x);
		key ^= randomTable[BLACK][from][b->pieces[from] & PIECEMASK];
	}
	return key;
}

hhTable* allocateHHTable(void)
{
	hhTable *hh;
	hh = (hhTable*) malloc(sizeof(hhTable));
	if (hh == NULL)
		abort();
	return hh;
}

int freeHHTable(hhTable *hh)
{
	free(hh);
	return 0;
}

int clearHHTable(hhTable *hh)
{
	int v[ER_PIECE];
	v[PAWN] = P_OR;
	v[KNIGHT] = N_OR;
	v[BISHOP] = B_OR;
	v[ROOK] = R_OR;
	v[QUEEN] = Q_OR;
	v[KING] = K_OR_M;

	int f, q;
	for (q = PAWN; q < ER_PIECE; q++) {
		for (f = 0; f < 64; f++)
			hh->val[0][q][f] = hh->val[1][q][f] = v[q];
	}
	return 0;
}

int updateHHTable(board *b, hhTable *hh, move_entry *m, int cutoff, int side, int depth, int ply)
{
	int fromPos, toPos, piece;
	int upd;

	fromPos = UnPackFrom(m[cutoff].move);
	toPos = UnPackTo(m[cutoff].move);
	piece = b->pieces[fromPos] & PIECEMASK;
	upd=Min(depth*depth, 1600);
	hh->val[side][piece][toPos] += upd;
	return 0;
}

int updateHHTable2(board *b, hhTable *hh, move_entry *m, int cutoff, int side, int bonus)
{
	int fromPos, toPos, piece;
	int upd;

	fromPos = UnPackFrom(m[cutoff].move);
	toPos = UnPackTo(m[cutoff].move);
	piece = b->pieces[fromPos] & PIECEMASK;
	hh->val[side][piece][toPos] += 32*bonus - hh->val[side][piece][toPos] * abs(bonus)/512;
	return 0;
}

int updateHHTableGood(board *b, hhTable *hh, move_entry *m, int cutoff, int side, int depth, int ply){
	return updateHHTable2(b, hh, m, cutoff, side, Min(depth*depth, 400));
}
int updateHHTableBad(board *b, hhTable *hh, move_entry *m, int cutoff, int side, int depth, int ply){
	return updateHHTable2(b, hh, m, cutoff, side, -Min(depth*depth/4, 400));
}

int checkHHTable(hhTable *hh, int side, int piece, int square)
{
	return hh->val[side][piece][square];
}

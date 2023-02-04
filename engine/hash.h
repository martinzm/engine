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

#ifndef HASH_H
#define HASH_H
#include "bitmap.h"

void setupPawnRandom(board *b);
void setupRandom(board *b);
void initRandom();
BITVAR getPawnKey(board *b);
BITVAR getKey(board *b);

void storeHash(hashStore *, hashEntry * hash, int side, int ply, int depth, struct _statistics *);
int retrieveHash(hashStore *, hashEntry *hash, int side, int ply, int depth, int use_previous, struct _statistics *);
hashPawnEntry * storePawnHash(hashPawnStore *, hashPawnEntry * hash, struct _statistics *);
hashPawnEntry * retrievePawnHash(hashPawnStore *, hashPawnEntry *hash, struct _statistics *);

void storePVHash(hashStore *, hashEntry * hash, int ply, struct _statistics *);
int initHash(hashStore *);
int invalidateHash(hashStore *);
int initPawnHash(hashPawnStore *);
int invalidatePawnHash(hashPawnStore *);
void dumpHash(board *,hashStore *, hashEntry *, int, int, int, int);

void storeExactPV(hashStore * hs, BITVAR key, BITVAR map, tree_store * orig, int level);
int restoreExactPV(hashStore * hs, BITVAR key, BITVAR map, int level, tree_store * rest);

hashStore * allocateHashStore(size_t hashBytes, int hashPVLen);
int freeHashStore(hashStore *);

hashPawnStore * allocateHashPawnStore(size_t hashBytes);
int freeHashPawnStore(hashPawnStore *);

int clear_killer_moves(kmoves *km);
int update_killer_move(kmoves *km, int ply, MOVESTORE move, struct _statistics *);
int get_killer_move(kmoves *km, int ply, int id, MOVESTORE *move);
kmoves *allocateKillerStore(void);
int freeKillerStore(kmoves *km);

int generateRandomFile(char *n);

hhTable * allocateHHTable(void);
int freeHHTable(hhTable *);
int clearHHTable(hhTable *);
int updateHHTable(board *, hhTable *, move_entry *, int, int, int, int);
int checkHHTable(hhTable *, int, int, int);

#endif

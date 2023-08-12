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

#ifndef MOVGEN_H
#define MOVGEN_H

#include "bitmap.h"

typedef struct _undo {
	int8_t castle[ER_SIDE];
	int8_t side;
	MOVESTORE move;
	int8_t captured;  //what was captured
	int8_t moved;  // promoted to in case promotion, otherwise the same as old
	int8_t old;  //what was the old piece
	int16_t rule50move;
	int8_t ep;
	int8_t prev_ep;
	int8_t mindex_validity;
	int psq_b;
	int psq_e;
	BITVAR key;
	BITVAR pawnkey;
	BITVAR old50key;  //what was the old key
	BITVAR old50pos;  //what was the old position
} UNDO;

#define CHECKFLAG (1<<15)

//#define PackMove(from,to,prom,spec)  ((MOVESTORE)((((from) & 0x3F) | (((to) & 0x3F) <<6) | (((prom) & 7) <<12))|((spec&1)<<15)))
//#define PackMove(from,to,prom,spec)  ((MOVESTORE)((((from) & 0x3F) | (((to) & 0x3F) <<6) | (((prom) & 7) <<12))))
#define PackMove(from,to,prom,spec)  ((MOVESTORE)((((from) & 0x3F) | (((to) & 0x3F) <<6) | (((prom) & 7) <<12))))
//#define PackMoveF(from,to,prom,spec)  ((MOVESTORE)((((from) & 0x3F) | (((to) & 0x3F) <<6) | (((prom) & 7) <<12))))
#define PackMoveF(from,to,prom,spec)  ((MOVESTORE)((((from) & 0x3F) | (((to) & 0x3F) <<6)))|(ER_PIECE<<12))
#define UnPackFrom(a)  ((int) ((a) & 0x3F))
#define UnPackTo(a)    ((int) (((a)>>6) & 0x3F))
#define UnPackProm(a)  ((int) (((a) >>12) & 7))
#define UnPackCheck(a) ((int) ((a>>15) & 1))
#define UnPackPPos(a)  ((MOVESTORE) ((a) & 0xFFF)) //only from & to extracted

int isMoveValid(board*, MOVESTORE, const attack_model*, int, tree_store*);

BITVAR isInCheck_Eval(board *b, attack_model *a, int side);
void generateCapturesN(const board *const b, const attack_model *a, move_entry **m, int gen_u);
void generateMovesN(const board *const b, const attack_model *a, move_entry **m);
void generateInCheckMovesN(const board *const b, const attack_model *a, move_entry **m, int gen_u);
void generateQuietCheckMovesN(const board *const b, const attack_model *a, move_entry **m);
int alternateMovGen(board *b, MOVESTORE *filter);
UNDO MakeMove(board *b, MOVESTORE move);
UNDO MakeNullMove(board *b);
void UnMakeMove(board *b, UNDO u);
void UnMakeNullMove(board *b, UNDO u);
int is_quiet_move(board const * const , attack_model const * const, move_entry const * const);

void printfMove(board *b, MOVESTORE m);
void sprintfMoveSimple(MOVESTORE m, char *buf);
void sprintfMove(board *b, MOVESTORE m, char *buf);
int gradeMoveInRow(board*, attack_model*, MOVESTORE, move_entry*, int);

void SelectBestO(move_cont *mv);
int sortMoveListNew_Init(board *b, attack_model *a, move_cont *mv);
int getNextMove(board *b, attack_model *a, move_cont *mv, int ply, int side, int incheck, move_entry **mm, tree_store*);
int getNextCap(board *b, attack_model *a, move_cont *mv, int ply, int side, int incheck, move_entry **mm, tree_store*);

int simple_pre_movegen_n(board *b, attack_model *a, int side);
int simple_pre_movegen_n2(board const *b, attack_model *a, int side);
int simple_pre_movegen_n2check(const board * constb, attack_model *a, int side);

int getNextCheckin(board*, attack_model*, move_cont*, int, int, int, move_entry**, tree_store*);

void generateCapturesN2(const board *const b, attack_model *a, move_entry **m, int gen_u);
void generateMovesN2(const board *const b, attack_model *a, move_entry **m);
void generateInCheckMovesN2(const board *const b, const attack_model *a, move_entry **m, int gen_u);
void mvsfrom2(const board * const, int const, int const, FuncAttacks, bmv **, BITVAR const, BITVAR const);
void mvsfroma2(const board * const, int const, int const, bmv **, BITVAR const, BITVAR const);


#endif

#ifndef ATTACKS_H
#define ATTACKS_H

#include "bitmap.h"

inline BITVAR RookAttacks(board *b, int pos) { return getnormvector(b->norm,pos) | get90Rvector(b->r90R,pos); }
inline BITVAR BishopAttacks(board *b, int pos) { return get45Rvector(b->r45R,pos) | get45Lvector(b->r45L,pos); }
inline BITVAR QueenAttacks(board *b, int pos) { return RookAttacks(b, pos) | BishopAttacks(b, pos); }
BITVAR KnightAttacks(board *b, int pos); 

BITVAR DiagAttacks_2(board *b, int pos);
BITVAR NormAttacks_2(board *b, int pos);

BITVAR AttackedTo(board *b, int pos);
BITVAR AttackedTo_A(board *b, int to, int side);
BITVAR AttackedTo_B(board *b, int to, int side);
int GetLVA_to(board *b, int to, int side, BITVAR ignore);
BITVAR WhitePawnAttacks(board *b, attack_model *a);
BITVAR BlackPawnAttacks(board *b, attack_model *a);
BITVAR WhitePawnMoves(board *b, attack_model *a);
BITVAR BlackPawnMoves(board *b, attack_model *a);

BITVAR FillNorth(BITVAR, BITVAR, BITVAR);
BITVAR FillSouth(BITVAR, BITVAR, BITVAR);
BITVAR FillEast(BITVAR, BITVAR, BITVAR);
BITVAR FillWest(BITVAR, BITVAR, BITVAR);
BITVAR FillNorthEast(BITVAR, BITVAR, BITVAR);
BITVAR FillNorthWest(BITVAR, BITVAR, BITVAR);
BITVAR FillSouthEast(BITVAR, BITVAR, BITVAR);
BITVAR FillSouthWest(BITVAR, BITVAR, BITVAR);

int isInCheck(board *b, int side);
BITVAR KingAvoidSQ(board *, attack_model *, int);


#endif

/*
 *
 * $Id: attacks.h,v 1.5.4.3 2006/02/09 20:30:07 mrt Exp $
 *
 */
 
#ifndef ATTACKS_H
#define ATTACKS_H

#include "bitmap.h"

BITVAR RookAttacks(board *b, int pos);
BITVAR BishopAttacks(board *b, int pos);
BITVAR QueenAttacks(board *b, int pos);
BITVAR KnightAttacks(board *b, int pos);
BITVAR AttackedTo(board *b, int pos);
BITVAR AttackedTo_A(board *b, int to, int side);
BITVAR AttackedTo_B(board *b, int to, int side);
int GetLVA_to(board *b, int to, int side, BITVAR ignore);
BITVAR WhitePawnAttacks(board *b, attack_model *a, BITVAR *atmap);
BITVAR BlackPawnAttacks(board *b, attack_model *a, BITVAR *atmap);
BITVAR WhitePawnMoves(board *b, attack_model *a);
BITVAR BlackPawnMoves(board *b, attack_model *a);

BITVAR FillNorth(BITVAR, BITVAR, BITVAR);
BITVAR FillSouth(BITVAR, BITVAR, BITVAR);

int isInCheck(board *b, int side);


#endif

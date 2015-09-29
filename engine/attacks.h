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
BITVAR AttackedTo(board *b, int pos);
BITVAR AttackedTo_A(board *b, int to, unsigned int side);
BITVAR WhitePawnAttacks(board *b);
BITVAR BlackPawnAttacks(board *b);

int isInCheck(board *b, int side);


#endif

/*
 *
 * $Id: attacks.c,v 1.8.6.4 2006/02/23 20:50:03 mrt Exp $
 *
 */
 
#include "attacks.h"
#include "bitmap.h"
#include "generate.h"
#include "globals.h"

BITVAR RookAttacks(board *b, int pos)
{
BITVAR ret;
//		printmask(get90Rvector(b->r90R,pos) ,"XXXX");
	ret =getnormvector(b->norm,pos);
	ret|=get90Rvector(b->r90R,pos);
	return ret;
}

BITVAR BishopAttacks(board *b, int pos)
{
		return get45Rvector(b->r45R,pos) | get45Lvector(b->r45L,pos);
}

BITVAR QueenAttacks(board *b, int pos)
{
		return RookAttacks(b, pos) | BishopAttacks(b, pos);
}

// generate bitmap containing all pieces attacking this square
BITVAR AttackedTo(board *b, int pos)
{
	BITVAR ret;
	ret= (RookAttacks(b, pos) & b->maps[ROOK]);
	ret|=(BishopAttacks(b, pos) & b->maps[BISHOP]);
	ret|=(QueenAttacks(b, pos) & b->maps[QUEEN]);
	ret|=(attack.maps[KNIGHT][pos] & b->maps[KNIGHT]);
	ret|=(attack.maps[KING][pos] & b->maps[KING]);
	ret|=(attack.pawn_att[WHITE][pos] & b->maps[PAWN] & (b->colormaps[BLACK]));
	ret|=(attack.pawn_att[BLACK][pos] & b->maps[PAWN] & (b->colormaps[WHITE]));

//	printmask(ret,"att");
	return ret;
}


// utoky kralem jsou take pocitany, nebot uvazujeme naprosto nechranene pole.
// odpovida na otazku, kdo utoci na pole TO obsazene stranou SIDE (tj utocnik je z druhe strany nez SIDE)
BITVAR AttackedTo_A_OLD(board *b, int to, unsigned int side)
{
	BITVAR cr, di, cr2, di2, cr_a, di_a, kn_a, pn_a, ki_a, ret, nnorm;
	int s, ff;

	s=side^1;
	nnorm=~normmark[to];

	cr=(attack.maps[ROOK][to]) & (b->maps[ROOK]|b->maps[QUEEN])&(b->colormaps[s]);
	di_a=cr_a = 0;
	while(cr) {
		ff = LastOne(cr);
		cr2=rays[to][ff] & (~normmark[ff]) & nnorm;
		if(!(cr2 & b->norm)) cr_a |= normmark[ff];
		ClrLO(cr);
	}

	di=(attack.maps[BISHOP][to]) & (b->maps[BISHOP]|b->maps[QUEEN])&(b->colormaps[s]);
	while(di) {
		ff = LastOne(di);
		di2=rays[to][ff] & (~normmark[ff]) & nnorm;
		if(!(di2 & b->norm)) di_a |= normmark[ff];
		ClrLO(di);
	}

//	kn_a=attack.maps[KNIGHT][to] & b->maps[KNIGHT] & b->colormaps[s];
//	pn_a=attack.pawn_att[s^1][to] & b->maps[PAWN] & b->colormaps[s];
//	ki_a=(attack.maps[KING][to] & b->maps[KING])& b->colormaps[s];
//	ret=cr_a|di_a|kn_a|pn_a|ki_a;

	kn_a=attack.maps[KNIGHT][to] & b->maps[KNIGHT];
	pn_a=attack.pawn_att[side][to] & b->maps[PAWN];
	ki_a=(attack.maps[KING][to] & b->maps[KING]);
	ret=(cr_a|di_a|kn_a|pn_a|ki_a) & b->colormaps[s];
	return ret;
}

BITVAR AttackedTo_A(board *b, int to, unsigned int side)
{
	BITVAR cr, di, cr_a, di_a, kn_a, pn_a, ki_a, ret;
	int s, ff;

	s=side^1;

	cr=(attack.maps[ROOK][to]) & (b->maps[ROOK]|b->maps[QUEEN])&(b->colormaps[s]);
	di_a=cr_a = 0;
	while(cr) {
		ff = LastOne(cr);
		if(!(rays_int[to][ff] & b->norm)) cr_a |= normmark[ff];
		ClrLO(cr);
	}

	di=(attack.maps[BISHOP][to]) & (b->maps[BISHOP]|b->maps[QUEEN])&(b->colormaps[s]);
	while(di) {
		ff = LastOne(di);
		if(!(rays_int[to][ff] & b->norm)) di_a |= normmark[ff];
		ClrLO(di);
	}

	kn_a=attack.maps[KNIGHT][to] & b->maps[KNIGHT];
	pn_a=attack.pawn_att[side][to] & b->maps[PAWN];
	ki_a=(attack.maps[KING][to] & b->maps[KING]);
	ret=(cr_a|di_a|kn_a|pn_a|ki_a) & b->colormaps[s];
	return ret;
}

//is side in check ?
// must take care of pawn attacks
// pawn attack only in forward direction

int isInCheck(board *b, int side)
{
BITVAR q;
unsigned char z;	
	z=((side == WHITE) ? BLACK : WHITE);
	q=(AttackedTo(b,b->king[side])&(b->colormaps[z]));
//	printboard(b);
	if(q!=0) {
	    return 1;
	}
	return 0;
}

// generate bitmap of white/black pawn attacked squares. EP attack is not included
BITVAR WhitePawnAttacks(board *b)
{
BITVAR x;
	x=b->maps[PAWN] & b->colormaps[WHITE];
	return ((x & ~(FILEH | RANK8))<<9 | (x & ~(FILEA | RANK8))<<7);
}

BITVAR BlackPawnAttacks(board *b)
{
BITVAR x;
	x=b->maps[PAWN] & b->colormaps[BLACK];
	return ((x & ~(FILEA | RANK1))>>9 | (x & ~(FILEH | RANK1))>>7);
}

// propagate pieces north, along empty squares - ie iboard is occupancy inversed, 1 means empty square
BITVAR FillNorth(BITVAR pieces, BITVAR iboard) {
BITVAR flood = pieces;
//	printmask(pieces, "pieces");
//	printmask(iboard, "iboard");
	flood |= pieces = (pieces << 8) & iboard;
	flood |= pieces = (pieces << 8) & iboard;
	flood |= pieces = (pieces << 8) & iboard;
	flood |= pieces = (pieces << 8) & iboard;
	flood |= pieces = (pieces << 8) & iboard;
	flood |= pieces = (pieces << 8) & iboard;
	flood |=          (pieces << 8) & iboard;

	return flood;
}

BITVAR FillSouth(BITVAR pieces, BITVAR iboard) {
BITVAR flood = pieces;
	flood |= pieces = (pieces >> 8) & iboard;
	flood |= pieces = (pieces >> 8) & iboard;
	flood |= pieces = (pieces >> 8) & iboard;
	flood |= pieces = (pieces >> 8) & iboard;
	flood |= pieces = (pieces >> 8) & iboard;
	flood |= pieces = (pieces >> 8) & iboard;
	flood |=          (pieces >> 8) & iboard;

	return flood;
}

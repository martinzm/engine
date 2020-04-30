/*
 *
 * $Id: attacks.c,v 1.8.6.4 2006/02/23 20:50:03 mrt Exp $
 *
 */
 
#include "attacks.h"
#include "bitmap.h"
#include "generate.h"
#include "globals.h"
#include "movgen.h"

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

BITVAR KnightAttacks(board *b, int pos)
{
		return (attack.maps[KNIGHT][pos] & b->maps[KNIGHT]);
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
BITVAR AttackedTo_A_OLD(board *b, int to, int side)
{
	BITVAR cr, di, cr2, di2, cr_a, di_a, kn_a, pn_a, ki_a, ret, nnorm;
	int s, ff;

	s=side^1;
	nnorm=~normmark[to];

	cr=(attack.maps[ROOK][to]) & (b->maps[ROOK]|b->maps[QUEEN])&(b->colormaps[s]);
	di_a=cr_a = 0;
	while(cr) {
		ff = LastOne(cr);
		cr2=attack.rays[to][ff] & (~normmark[ff]) & nnorm;
		if(!(cr2 & b->norm)) cr_a |= normmark[ff];
		ClrLO(cr);
	}

	di=(attack.maps[BISHOP][to]) & (b->maps[BISHOP]|b->maps[QUEEN])&(b->colormaps[s]);
	while(di) {
		ff = LastOne(di);
		di2=attack.rays[to][ff] & (~normmark[ff]) & nnorm;
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


// get LVA attacker to square to from side
int GetLVA_to(board *b, int to, int side, BITVAR ignore)
{
	BITVAR cr, di, kn_a, pn_a, ki_a, norm;
	int s, ff;

	s=side^1;

	norm=b->norm & ignore;
	pn_a=(attack.pawn_att[s][to] & b->maps[PAWN] & norm & b->colormaps[side]);
	if(pn_a) return LastOne(pn_a);
	kn_a=(attack.maps[KNIGHT][to] & b->maps[KNIGHT] & norm & b->colormaps[side]);
	if(kn_a)return LastOne(kn_a);

	di=attack.maps[BISHOP][to] & b->maps[BISHOP] & b->colormaps[side] & norm;
	while(di) {
		ff = LastOne(di);
		if(!(attack.rays_int[to][ff] & norm)) return ff;
		ClrLO(di);
	}
	
	cr=attack.maps[ROOK][to] & b->maps[ROOK] & b->colormaps[side] & norm;
	while(cr) {
		ff = LastOne(cr);
		if(!(attack.rays_int[to][ff] & norm)) return ff;
		ClrLO(cr);
	}

	di=((attack.maps[BISHOP][to] & b->maps[QUEEN]) | (attack.maps[ROOK][to] & b->maps[QUEEN]))&(b->colormaps[side]) & norm;
	while(di) {
		ff = LastOne(di);
		if(!(attack.rays_int[to][ff] & norm)) return ff;
		ClrLO(di);
	}

	ki_a=(attack.maps[KING][to] & b->maps[KING] & b->colormaps[side]) & norm;
	if(ki_a) return LastOne(ki_a); else return -1;
}

// get LVA attacker to square to from side
int GetLVA_to2(board *b, int to, int side, BITVAR ignore)
{
	BITVAR cr, di, kn_a, pn_a, ki_a, norm;
	int s, ff;

	s=side^1;

	norm=b->norm & ignore;
	pn_a=(attack.pawn_att[s][to] & b->maps[PAWN] & norm & b->colormaps[side]);
	if(pn_a) return LastOne(pn_a);
	kn_a=(attack.maps[KNIGHT][to] & b->maps[KNIGHT] & norm & b->colormaps[side]);
	if(kn_a)return LastOne(kn_a);

	di=attack.maps[BISHOP][to] & b->maps[BISHOP] & b->colormaps[side] & norm;
	while(di) {
		ff = LastOne(di);
		if(!(attack.rays_int[to][ff] & norm)) return ff;
		ClrLO(di);
	}
	
	cr=attack.maps[ROOK][to] & b->maps[ROOK] & b->colormaps[side] & norm;
	while(cr) {
		ff = LastOne(cr);
		if(!(attack.rays_int[to][ff] & norm)) return ff;
		ClrLO(cr);
	}

	di=((attack.maps[BISHOP][to] & b->maps[QUEEN]) | (attack.maps[ROOK][to] & b->maps[QUEEN]))&(b->colormaps[side]) & norm;
	while(di) {
		ff = LastOne(di);
		if(!(attack.rays_int[to][ff] & norm)) return ff;
		ClrLO(di);
	}

	ki_a=(attack.maps[KING][to] & b->maps[KING] & b->colormaps[side]) & norm;
	if(ki_a) return LastOne(ki_a); else return -1;
}
// create full map of attackers to mentioned square belonging to side
BITVAR AttackedTo_A(board *b, int to, int side)
{
	BITVAR cr, di, cr_a, di_a, kn_a, pn_a, ki_a, ret;
	int s, ff;

	s=side^1;

	cr=(attack.maps[ROOK][to]) & (b->maps[ROOK]|b->maps[QUEEN])&(b->colormaps[s]);
	di_a=cr_a = 0;
	while(cr) {
		ff = LastOne(cr);
		if(!(attack.rays_int[to][ff] & b->norm)) cr_a |= normmark[ff];
		ClrLO(cr);
	}

	di=(attack.maps[BISHOP][to]) & (b->maps[BISHOP]|b->maps[QUEEN])&(b->colormaps[s]);
	while(di) {
		ff = LastOne(di);
		if(!(attack.rays_int[to][ff] & b->norm)) di_a |= normmark[ff];
		ClrLO(di);
	}

	kn_a=attack.maps[KNIGHT][to] & b->maps[KNIGHT];
	pn_a=attack.pawn_att[side][to] & b->maps[PAWN];
	ki_a=(attack.maps[KING][to] & b->maps[KING]);
	ret=(cr_a|di_a|kn_a|pn_a|ki_a) & b->colormaps[s];
	return ret;
}

// just answer to question if square belonging to side is under attack from opponent
BITVAR AttackedTo_B(board *b, int to, int side)
{
	BITVAR cr;
	int s, ff;

	s=side^1;

	cr=((attack.maps[ROOK][to])&(b->maps[ROOK]|b->maps[QUEEN])&(b->colormaps[s])) | ((attack.maps[BISHOP][to]) & (b->maps[BISHOP]|b->maps[QUEEN])&(b->colormaps[s]));
	while(cr) {
		ff = LastOne(cr);
		if(!(attack.rays_int[to][ff] & b->norm)) return 1;
		ClrLO(cr);
	}

	if(attack.pawn_att[side][to] & b->maps[PAWN] & b->colormaps[s]) return 1;
	if(attack.maps[KNIGHT][to] & b->maps[KNIGHT] & b->colormaps[s]) return 1;
	if(attack.maps[KING][to] & b->maps[KING] & b->colormaps[s]) return 1;
	return 0;
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
// generate from current board;
// returns all squares where pawns attack - include even attacks by pinned pawns
// atmap contains map of all squares which can be captured by pawns
BITVAR WhitePawnAttacks(board *b, attack_model *a, BITVAR *atmap)
{
BITVAR x,r,r2, pins, mv;
int from;
	pins=((a->ke[WHITE].cr_pins | a->ke[WHITE].di_pins) & b->maps[PAWN] & b->colormaps[WHITE]);
	x=b->maps[PAWN] & b->colormaps[WHITE] & (~pins);
	r2=r=((x & ~(FILEH | RANK8))<<9 | (x & ~(FILEA | RANK8))<<7);
	while(pins!=0UL) {
		from = LastOne(pins);
		mv = (attack.pawn_att[WHITE][from]) & a->ke[WHITE].blocker_ray[from];
		r|=mv;
		r2|=(attack.pawn_att[WHITE][from]);
		ClrLO(pins);
	}
	*atmap=r;
	return r2;
}

BITVAR BlackPawnAttacks(board *b, attack_model *a, BITVAR *atmap)
{
BITVAR x,r,r2, pins, mv;
int from;
	pins=((a->ke[BLACK].cr_pins | a->ke[BLACK].di_pins) & b->maps[PAWN] & b->colormaps[BLACK]);
	x=b->maps[PAWN] & b->colormaps[BLACK] & (~pins);
	r2=r=((x & ~(FILEA | RANK1))>>9 | (x & ~(FILEH | RANK1))>>7);
//pins
	while(pins!=0) {
		from = LastOne(pins);
		mv = (attack.pawn_att[BLACK][from]) & a->ke[BLACK].blocker_ray[from];
		r|=mv;
		r2|=(attack.pawn_att[BLACK][from]);
		ClrLO(pins);
	}
	*atmap=r;
	return r2;
}

// generate all possible pawn moves for current board 
BITVAR WhitePawnMoves(board *b, attack_model *a)
{
BITVAR x, pins, r;
int from;
	pins=((a->ke[WHITE].cr_pins | a->ke[WHITE].di_pins) & b->maps[PAWN] & b->colormaps[WHITE]);
	x=b->maps[PAWN] & b->colormaps[WHITE] & (~pins);
	r = ((x << 8) & (~b->norm));
	r|= (((r&RANK3) << 8) & (~b->norm));
//pins
	while(pins!=0) {
		from = LastOne(pins);
		r|= (attack.pawn_move[WHITE][from]) & a->ke[WHITE].blocker_ray[from] & (~b->norm);
		ClrLO(pins);
	}
return r;
}

BITVAR BlackPawnMoves(board *b, attack_model *a){
BITVAR x, pins, r;
int from;
	pins=((a->ke[BLACK].cr_pins | a->ke[BLACK].di_pins) & b->maps[PAWN] & b->colormaps[BLACK]);
	x=b->maps[PAWN] & b->colormaps[BLACK] & (~pins);
	r = ((x >> 8) & (~b->norm));
	r|= (((x&RANK6) >> 8) & (~b->norm));
	while(pins!=0) {
		from = LastOne(pins);
		r|= (attack.pawn_move[BLACK][from]) & a->ke[BLACK].blocker_ray[from] & (~b->norm);
		ClrLO(pins);
	}
return r;
}

// propagate pieces north, along empty squares - ie iboard is occupancy inversed, 1 means empty square
// result has squares in between initial position and stop set, not including initial position and final(blocked) squares
BITVAR FillNorth(BITVAR pieces, BITVAR iboard, BITVAR init) {
BITVAR flood = init;
//	printmask(pieces, "pieces");
//	printmask(iboard, "iboard");
	flood |= pieces = ((pieces << 8) & iboard);
	flood |= pieces = ((pieces << 8) & iboard);
	flood |= pieces = ((pieces << 8) & iboard);
	flood |= pieces = ((pieces << 8) & iboard);
	flood |= pieces = ((pieces << 8) & iboard);
	flood |= pieces = ((pieces << 8) & iboard);
	flood |=          ((pieces << 8) & iboard);
	return flood;
}

BITVAR FillSouth(BITVAR pieces, BITVAR iboard, BITVAR init) {
BITVAR flood = init;
	flood |= pieces = (pieces >> 8) & iboard;
	flood |= pieces = (pieces >> 8) & iboard;
	flood |= pieces = (pieces >> 8) & iboard;
	flood |= pieces = (pieces >> 8) & iboard;
	flood |= pieces = (pieces >> 8) & iboard;
	flood |= pieces = (pieces >> 8) & iboard;
	flood |=          (pieces >> 8) & iboard;

	return flood;
}

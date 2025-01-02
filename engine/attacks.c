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

#include "attacks.h"
#include "bitmap.h"
#include "generate.h"
#include "globals.h"
#include "movgen.h"
#include "utils.h"
#include <assert.h>
#include "evaluate.h"

#if 0
extern inline BITVAR KnightAttacks(board const *b, int pos)
{
	return (attack.maps[KNIGHT][pos] & b->maps[KNIGHT]);
}
#endif

// generate bitmap containing all pieces attacking this square
BITVAR AttackedTo(board *b, int pos)
{
BITVAR ret;

	ret  = (RookAttacks(b, pos) & (b->maps[ROOK] | b->maps[QUEEN]));
	ret |= (BishopAttacks(b, pos) & (b->maps[BISHOP] | b->maps[QUEEN]));
	ret |= (attack.maps[KNIGHT][pos] & b->maps[KNIGHT]);
	ret |= (attack.maps[KING][pos] & b->maps[KING]);
	ret |= (attack.pawn_att[WHITE][pos] & b->maps[PAWN]
		& (b->colormaps[BLACK]));
	ret |= (attack.pawn_att[BLACK][pos] & b->maps[PAWN]
		& (b->colormaps[WHITE]));

	return ret;
}


BITVAR DiagAttacks_2(board *b, int pos)
{
	BITVAR t11, t12, t21, t22;
	get45Rvector2(b->r45R, pos, &t11, &t21);
	get45Lvector2(b->r45L, pos, &t12, &t22);
	return (t21 | t22);
}

BITVAR NormAttacks_2(board *b, int pos)
{
	BITVAR t11, t12, t21, t22;
	getnormvector2(b->norm, pos, &t11, &t21);
	get90Rvector2(b->r90R, pos, &t12, &t22);
	return (t21 | t22);
}

// get LVA attacker to square to from side
int GetLVA_to(board *b, int to, int side, BITVAR ignore)
{
	BITVAR cr, di, kn_a, pn_a, ki_a, norm, ns;
	int s, ff;

	s = Flip(side);

	norm = b->norm & ignore;
	ns = norm & b->colormaps[side];
	pn_a = (attack.pawn_att[s][to] & b->maps[PAWN] & ns);
	if (pn_a)
		return LastOne(pn_a);
	kn_a = (attack.maps[KNIGHT][to] & b->maps[KNIGHT] & ns);
	if (kn_a)
		return LastOne(kn_a);

	di = attack.maps[BISHOP][to] & b->maps[BISHOP] & ns;
	while (di) {
		ff = LastOne(di);
		if (!(attack.rays_int[to][ff] & norm))
			return ff;
		ClrLO(di);
	}
	
	cr = attack.maps[ROOK][to] & b->maps[ROOK] & ns;
	while (cr) {
		ff = LastOne(cr);
		if (!(attack.rays_int[to][ff] & norm))
			return ff;
		ClrLO(cr);
	}

	di = ((attack.maps[BISHOP][to] & b->maps[QUEEN])
		| (attack.maps[ROOK][to] & b->maps[QUEEN])) & ns;
	while (di) {
		ff = LastOne(di);
		if (!(attack.rays_int[to][ff] & norm))
			return ff;
		ClrLO(di);
	}

	ki_a = attack.maps[KING][to] & b->maps[KING] & ns;
	if (ki_a)
		return LastOne(ki_a);
	else
		return -1;
}

// propagate pieces north, along empty squares - ie iboard is occupancy inversed, 1 means empty square
// result has squares in between initial position and stop set, not including initial position, includes final(blocked) squares
BITVAR FillNorth(BITVAR pieces, BITVAR iboard, BITVAR init)
{
	BITVAR flood = init;
	flood |= pieces = ((pieces << 8) & iboard);
	flood |= pieces = ((pieces << 8) & iboard);
	flood |= pieces = ((pieces << 8) & iboard);
	flood |= pieces = ((pieces << 8) & iboard);
	flood |= pieces = ((pieces << 8) & iboard);
	flood |= pieces = ((pieces << 8) & iboard);

	return flood << 8;
}

BITVAR FillSouth(BITVAR pieces, BITVAR iboard, BITVAR init)
{
	BITVAR flood = init;
	flood |= pieces = (pieces >> 8) & iboard;
	flood |= pieces = (pieces >> 8) & iboard;
	flood |= pieces = (pieces >> 8) & iboard;
	flood |= pieces = (pieces >> 8) & iboard;
	flood |= pieces = (pieces >> 8) & iboard;
	flood |= pieces = (pieces >> 8) & iboard;

	return flood >> 8;
}

BITVAR FillWest(BITVAR pieces, BITVAR iboard, BITVAR init)
{
	BITVAR flood = init;
	const BITVAR N = 0x7f7f7f7f7f7f7f7f;
	iboard &= N;
	flood |= pieces = (pieces >> 1) & iboard;
	flood |= pieces = (pieces >> 1) & iboard;
	flood |= pieces = (pieces >> 1) & iboard;
	flood |= pieces = (pieces >> 1) & iboard;
	flood |= pieces = (pieces >> 1) & iboard;
	flood |= pieces = (pieces >> 1) & iboard;

	return (flood >> 1) & N;
}

BITVAR FillEast(BITVAR pieces, BITVAR iboard, BITVAR init)
{
	BITVAR flood = init;
	const BITVAR N = 0xfefefefefefefefe;
	iboard &= N;
	flood |= pieces = (pieces << 1) & iboard;
	flood |= pieces = (pieces << 1) & iboard;
	flood |= pieces = (pieces << 1) & iboard;
	flood |= pieces = (pieces << 1) & iboard;
	flood |= pieces = (pieces << 1) & iboard;
	flood |= pieces = (pieces << 1) & iboard;

	return (flood << 1) & N;
}

BITVAR FillNorthEast(BITVAR pieces, BITVAR iboard, BITVAR init)
{
	BITVAR flood = init;
	const BITVAR N = 0xfefefefefefefefe;
	iboard &= N;
	flood |= pieces = (pieces << 9) & iboard;
	flood |= pieces = (pieces << 9) & iboard;
	flood |= pieces = (pieces << 9) & iboard;
	flood |= pieces = (pieces << 9) & iboard;
	flood |= pieces = (pieces << 9) & iboard;
	flood |= pieces = (pieces << 9) & iboard;

	return (flood << 9) & N;
}

BITVAR FillNorthWest(BITVAR pieces, BITVAR iboard, BITVAR init)
{
	BITVAR flood = init;
	const BITVAR N = 0x7f7f7f7f7f7f7f7f;
	iboard &= N;
	flood |= pieces = (pieces << 7) & iboard;
	flood |= pieces = (pieces << 7) & iboard;
	flood |= pieces = (pieces << 7) & iboard;
	flood |= pieces = (pieces << 7) & iboard;
	flood |= pieces = (pieces << 7) & iboard;
	flood |= pieces = (pieces << 7) & iboard;

	return (flood << 7) & N;
}

BITVAR FillSouthEast(BITVAR pieces, BITVAR iboard, BITVAR init)
{
	BITVAR flood = init;
	const BITVAR N = 0xfefefefefefefefe;
	iboard &= N;
	flood |= pieces = (pieces >> 7) & iboard;
	flood |= pieces = (pieces >> 7) & iboard;
	flood |= pieces = (pieces >> 7) & iboard;
	flood |= pieces = (pieces >> 7) & iboard;
	flood |= pieces = (pieces >> 7) & iboard;
	flood |= pieces = (pieces >> 7) & iboard;

	return (flood >> 7) & N;
}

BITVAR FillSouthWest(BITVAR pieces, BITVAR iboard, BITVAR init)
{
	BITVAR flood = init;
	const BITVAR N = 0x7f7f7f7f7f7f7f7f;
	iboard &= N;
	flood |= pieces = (pieces >> 9) & iboard;
	flood |= pieces = (pieces >> 9) & iboard;
	flood |= pieces = (pieces >> 9) & iboard;
	flood |= pieces = (pieces >> 9) & iboard;
	flood |= pieces = (pieces >> 9) & iboard;
	flood |= pieces = (pieces >> 9) & iboard;

	return (flood >> 9) & N;
}

// it generates squares OPSIDE king cannot step on, it ignores PINS
// builds all squares attacked by side 

BITVAR KingAvoidSQ(board const *b, attack_model *a, int side)
{
	BITVAR ret, empty, set1, set2, set3, set4;
	int from, opside;
	
	opside = Flip(side);
	empty = ~b->norm;
// remove opside king to allow attack propagation beyond it
	empty |= normmark[b->king[opside]];

	set1 = b->colormaps[side] & (b->maps[QUEEN] | b->maps[ROOK]);
	set2 = b->colormaps[side] & (b->maps[QUEEN] | b->maps[BISHOP]);
	ret = FillNorth(set1, empty, set1)
		| FillSouth(set1, empty, set1)
		| FillEast(set1, empty, set1)
		| FillWest(set1, empty, set1)
		| FillNorthEast(set2, empty, set2)
		| FillSouthWest(set2, empty, set2)
		| FillNorthWest(set2, empty, set2)
		| FillSouthEast(set2, empty, set2);
	set3 = b->colormaps[side] & b->maps[PAWN];
	ret |= (side == WHITE) ? (((set3 << 9) & 0xfefefefefefefefe) | ((set3 << 7) & 0x7f7f7f7f7f7f7f7f)) :
							 (((set3 >> 7) & 0xfefefefefefefefe) | ((set3 >> 9) & 0x7f7f7f7f7f7f7f7f));
	set4 = (b->maps[KNIGHT] & b->colormaps[side]);
	while (set4) {
		from = LastOne(set4);
		ret |= (attack.maps[KNIGHT][from]);
		ClrLO(set4);
	}
// fold in my king as purpose is to cover all squares opside king cannot step on
	ret |= (attack.maps[KING][b->king[side]]);
	return ret;
}

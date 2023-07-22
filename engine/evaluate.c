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

#include <stdlib.h>
#include <string.h>
#include "evaluate.h"
#include "generate.h"
#include "movgen.h"
#include "attacks.h"
#include "bitmap.h"
#include "pers.h"
#include "utils.h"
#include "globals.h"
#include "assert.h"

uint8_t t_phase(int p, int n, int b, int r, int q) {
	int v[] = { 0, 6, 6, 9, 18 };
	int nc[] = { 16, 4, 4, 4, 2 };
	int tot = nc[PAWN]*v[PAWN] + nc[KNIGHT]*v[KNIGHT] + nc[BISHOP]*v[BISHOP] + nc[ROOK]*v[ROOK] + nc[QUEEN]*v[QUEEN];
	int i = p*v[PAWN] + n*v[KNIGHT] + b*v[BISHOP] + r*v[ROOK] + q*v[QUEEN];
	return (uint8_t) ((Min(i, tot)*255)/tot) & 255;
}

uint8_t eval_phase(board const *b, personality const *p)
{
	int i1, i2, i3, i4, i5;
	if (b->mindex_validity == 1) {
		return p->mat_faze[b->mindex];
	} else {
		i1 = BitCount(b->maps[PAWN]);
		i2 = BitCount(b->maps[KNIGHT]);
		i3 = BitCount(b->maps[BISHOP]);
		i4 = BitCount(b->maps[ROOK]);
		i5 = BitCount(b->maps[QUEEN]);
		return t_phase(i1, i2, i3, i4, i5);
	}
}

int PSQSearch(int from, int to, int piece, int side, int phase, personality *p)
{
	int res, be, en;

	be = p->piecetosquare[MG][side][piece][to]
		- p->piecetosquare[MG][side][piece][from];
	en = p->piecetosquare[EG][side][piece][to]
		- p->piecetosquare[EG][side][piece][from];
	res = ((be * phase + en * (255 - phase)) / 255);
	return res / 10;
}

/*
 * mobility model can be built from simple_pre_movegen result 
 * 
 */

#ifdef TUNING
#define MAKEMOB2(q, piece, side, from, st) \
		m=a->me[from].pos_att_tot=BitCount(q & togo[side]); \
		m2=BitCount(q & togo[side] & unsafe[side]); \
		a->me[from].pos_mob_tot_b=p->mob_val[MG][side][piece][m-m2]; \
		a->me[from].pos_mob_tot_e=p->mob_val[EG][side][piece][m-m2]; \
		ADD_STACKER(st, mob_val[MG][side][piece][m-m2], 1, BAs, side) \
		ADD_STACKER(st, mob_val[EG][side][piece][m-m2], 1, BAs, side) \
		if(p->mobility_unsafe==1) { \
			a->me[from].pos_mob_tot_b+=p->mob_uns[MG][side][piece][m2]; \
			a->me[from].pos_mob_tot_e+=p->mob_uns[EG][side][piece][m2]; \
			ADD_STACKER(st, mob_uns[MG][side][piece][m2], 1, BAs, side) \
			ADD_STACKER(st, mob_uns[EG][side][piece][m2], 1, BAs, side) \
		} 
	
#else
#define MAKEMOB2(q, piece, side, from, st) \
		m=a->me[from].pos_att_tot=BitCount(q & togo[side]); \
		m2=BitCount(q & togo[side] & unsafe[side]); \
		a->me[from].pos_mob_tot_b=p->mob_val[MG][side][piece][m-m2]; \
		a->me[from].pos_mob_tot_e=p->mob_val[EG][side][piece][m-m2]; \
			a->me[from].pos_mob_tot_b+=(p->mob_uns[MG][side][piece][m2]*(p->mobility_unsafe==1)); \
			a->me[from].pos_mob_tot_e+=(p->mob_uns[EG][side][piece][m2]*(p->mobility_unsafe==1)); \
	
#endif

int make_mobility_modelN2(const board *const b, attack_model *a, personality const *p, stacker *st)
{
	int from, epn, opside, orank;
	BITVAR x, q, pins[2], epbmp, tmp, kpin, nmf, tmq, t2[ER_PIECE], tmm, tma;
	BITVAR togo[2], unsafe[2];
	BITVAR np[ER_PIECE + 1];
	BITVAR pi[ER_PIECE + 1];
	int tt[8],f,ff, pp, piece, side, m, m2 ;

	bmv mm[64];
	bmv *ip,*ib,*in,*ir,*iq,*ik,*ii, *ix;

	a->pos_c[PAWN]=a->pos_c[KNIGHT]=a->pos_c[BISHOP]=a->pos_c[ROOK]=a->pos_c[QUEEN]=a->pos_c[KING]=-1;
	a->pos_c[PAWN+BLACKPIECE]=a->pos_c[KNIGHT+BLACKPIECE]=a->pos_c[BISHOP+BLACKPIECE]
		=a->pos_c[ROOK+BLACKPIECE]=a->pos_c[QUEEN+BLACKPIECE]=a->pos_c[KING+BLACKPIECE]
		=a->pos_c[ER_PIECE]=-1;

	pins[WHITE] = ((a->ke[WHITE].cr_pins | a->ke[WHITE].di_pins));
	pins[BLACK] = ((a->ke[BLACK].cr_pins | a->ke[BLACK].di_pins));

	a->pa_at[WHITE] = a->pa_at[BLACK] = a->pa_mo[WHITE] = a->pa_mo[BLACK] = 0;
	orank = 0;
	x = b->maps[PAWN] & b->colormaps[WHITE];
	while (x) {
		from=LastOne(x);
		nmf  = NORMM(from);
		a->pos_m[PAWN][++(a->pos_c[PAWN])]=from;
		tmp  = (nmf << 8) & (~b->norm);
		tmm = tmp |= (((tmp&RANK3) << 8) & (~b->norm));
		tmp |= tma = attack.pawn_att[WHITE][from];
		q = a->mvs[from] = (pins[WHITE]&nmf) ? tmp&attack.rays_dir[b->king[WHITE]][from] : tmp;
		a->pa_at[WHITE] |= q&tma;
		a->pa_mo[WHITE] |= q&tmm;
		ClrLO(x);
	}
	orank = 56;
	x = b->maps[PAWN] & b->colormaps[BLACK];
	while (x) {
		from=LastOne(x);
		nmf  = NORMM(from);
		a->pos_m[PAWN+BLACKPIECE][++(a->pos_c[PAWN+BLACKPIECE])]=from;
		tmp  = (nmf >> 8) & (~b->norm);
		tmm = tmp |= (((tmp&RANK6) >> 8) & (~b->norm));
		tmp |= tma = attack.pawn_att[BLACK][from];
		q = a->mvs[from] = (pins[BLACK]&nmf) ? tmp&attack.rays_dir[b->king[BLACK]][from] : tmp;
		a->pa_at[BLACK] |= q&tma;
		a->pa_mo[BLACK] |= q&tmm;
		ClrLO(x);
	}
	if(b->ep != -1) {
		epbmp =
			(b->ep != -1 && (a->ke[b->side].ep_block == 0)) ? attack.ep_mask[b->ep]
				& b->maps[PAWN] & b->colormaps[b->side] : 0;
		x = b->maps[PAWN] & epbmp & b->colormaps[b->side];
		while (x) {
			epn = b->side == WHITE ? 1 : -1;
			from = LastOne(x);
			nmf = NORMM(from);
			kpin = (nmf & pins[b->side]) ? attack.rays_dir[b->king[b->side]][from] : FULLBITMAP;
			if (NORMM(getPos(getFile(b->ep), getRank(b->ep) + epn)) & kpin) {
				q = a->mvs[from] |= NORMM(b->ep);
				a->pa_at[b->side] |= q;
				a->pa_mo[b->side] |= a->mvs[from] & (~q);
			}
			ClrLO(x);
		}
	}

	for(side=0;side<=1;side++) {
		from = b->king[side];
		a->pos_m[KING+side*BLACKPIECE][++(a->pos_c[KING+side*BLACKPIECE])]=from;
		a->mvs[from] = (attack.maps[KING][from])
			& (~attack.maps[KING][b->king[Flip(side)]])
			& (~a->att_by_side[Flip(side)]);
		if (b->castle[side]) {
			if (b->castle[side] & QUEENSIDE) {
				if ((attack.rays[C1 + orank][E1 + orank]
					& ((a->att_by_side[Flip(side)]
						| attack.maps[KING][b->king[Flip(side)]]))) == 0
					&& ((attack.rays[B1 + orank][D1 + orank] & b->norm) == 0))
					a->mvs[from] |= NORMM(C1 + orank);
			}
			if (b->castle[side] & KINGSIDE) {
				if ((attack.rays[E1 + orank][G1 + orank]
					& (a->att_by_side[Flip(side)]
						| attack.maps[KING][b->king[Flip(side)]])) == 0
					&& ((attack.rays[F1 + orank][G1 + orank] & b->norm) == 0))
					a->mvs[from] |= NORMM(G1 + orank);
			}
		}
	}

	togo[WHITE]   = ~(b->colormaps[WHITE] | a->pa_at[BLACK]);
	togo[BLACK]   = ~(b->colormaps[BLACK] | a->pa_at[WHITE]);
	unsafe[WHITE] = a->pa_at[BLACK];
	unsafe[BLACK] = a->pa_at[WHITE];

	togo[WHITE] |= ((b->colormaps[WHITE] & ~unsafe[WHITE])*(p->mobility_protect == 1));
	togo[BLACK] |= ((b->colormaps[BLACK] & ~unsafe[BLACK])*(p->mobility_protect == 1));
	togo[WHITE] |= ((unsafe[WHITE] & ~b->norm)*(p->mobility_unsafe == 1));
	togo[BLACK] |= ((unsafe[BLACK] & ~b->norm)*(p->mobility_unsafe == 1));
	togo[WHITE] |= ((unsafe[WHITE] & b->colormaps[WHITE])*(p->mobility_unsafe == 1));
	togo[BLACK] |= ((unsafe[BLACK] & b->colormaps[BLACK])*(p->mobility_unsafe == 1));

	side=WHITE;
	ii=mm;
	mvsfrom2(b, QUEEN, side, QueenAttacks, &ii, FULLBITMAP, b->colormaps[side]);
	mvsfrom2(b, ROOK, side, RookAttacks, &ii, FULLBITMAP, b->colormaps[side]) ;
	mvsfrom2(b, BISHOP, side, BishopAttacks, &ii, FULLBITMAP, b->colormaps[side]) ;
	mvsfroma2(b, KNIGHT, side, &ii, FULLBITMAP, b->colormaps[side]) ;

	for(ix=mm; ix<ii;ix++) {
		x = a->mvs[ix->fr] = ((((pins[WHITE] >> (ix->fr))&1)-1)|(ix->mr))&(ix->mm);
		MAKEMOB2(x, ix->pi, WHITE, ix->fr, st);
		a->pos_m[ix->pi][++a->pos_c[ix->pi]] = ix->fr;
	}

	side=BLACK;
	ii=mm;
	mvsfrom2(b, QUEEN, side, QueenAttacks, &ii, FULLBITMAP, b->colormaps[side]);
	mvsfrom2(b, ROOK, side, RookAttacks, &ii, FULLBITMAP, b->colormaps[side]) ;
	mvsfrom2(b, BISHOP, side, BishopAttacks, &ii, FULLBITMAP, b->colormaps[side]) ;
	mvsfroma2(b, KNIGHT, side, &ii, FULLBITMAP, b->colormaps[side]) ;

	for(ix=mm; ix<ii;ix++) {
		x = a->mvs[ix->fr] = ((((pins[BLACK] >> (ix->fr))&1)-1)|(ix->mr))&(ix->mm);
		MAKEMOB2(x, ix->pi, BLACK, ix->fr, st);
		a->pos_m[ix->pi+BLACKPIECE][++a->pos_c[ix->pi+BLACKPIECE]] = ix->fr;
	}
	return 0;
}

/*
 * Evaluation
 * simple eval function score = A*X+B*Y+Z
 * get values (X,Y,Z,...) for features a position has, 
 * example: feature= "material/Value of Pawn", value= "number of PAWNs in given position"
 * compute final score by multiplying values with feature coefficients (A,B,...)
 *
 * PAWNs are static/not moving much - so results for PAWN related features can be cached
 * some PAWN related features (like KING shelter) that are slightly variable can be precomputed and choosen at time
 *
 * 
 */

/*
 * Pawns
 * potential passer (path to promotion is not blocked by pawns)
 * blocked - pawn in the way
 * stopped - opposite pawn attacks path
 * doubled - blocked by own pawn
 * isolated - no helping pawns on sides
 * backward - ???
 *
 * attacks
 *
 * outposts
 * holes
 * king shelter
 * 
 */

// no evaluation, only features discovery

int analyze_pawn(board const *b, attack_model const *a, PawnStore *ps, int side, personality const *p)
{
	int opside;

	BITVAR dir;
	BITVAR temp, t2;
	int file, rank, tt1, tt2, from, f, i, n, x, r, dpush;

	opside = Flip(side);
	for (f = 0; f < 8; f++) {
		ps->prot_p_p[side][f] = EMPTYBITMAP;
		ps->prot_p_c[side][f] = EMPTYBITMAP;
	}
	
// iterate pawns
	f = 0;
	from = ps->pawns[side][f];
	while (from != -1) {
		file = getFile(from);
		rank = getRank(from);
		dpush = ((side == WHITE) && (rank == RANKi2))
			| ((side == BLACK) && (rank == RANKi7));
		ps->not_pawns_file[side] &= (~attack.file[from]);
		
// span is path from pawn to next pawn before me [][][0] or behind me [][][1], or to edge of board
		dir = ps->spans[side][f][0];
		ps->pas_d[side][f] = 8;
		ps->double_d[side][f] = 8;
		ps->block_d[side][f] = 8;
		ps->block_d2[side][f] = 8;
		ps->stop_d[side][f] = 8;
		ps->prot_d[side][f] = 8;
		ps->prot_p_d[side][f] = 8;
		ps->prot_p_p_d[side][f] = 0;
		ps->prot_p_c_d[side][f] = 0;
		ps->prot_dir_d[side][f] = 0;
		ps->outp[side][f] = 8;
		ps->outp_d[side][f] = 8;
		ps->issue_d[side][f] = 0;

// passer, distance to promotion
		if ((dir & ps->pass_end[side])) {
			ps->pas_d[side][f] = BitCount(dir) - 1 - dpush;
			ps->passer[side] |= normmark[from];
			assert((ps->pas_d[side][f] < 8) && (ps->pas_d[side][f] >= 0));
		} else {
// blocked by some pawn, how far ahead
			dir = ps->spans[side][f][2];
			if (dir & ps->path_stop[side] & (b->maps[PAWN])) {
				if (dir & ps->path_stop[side] & (b->maps[PAWN])
					& b->colormaps[side]) {
// doubled - my pawn blocks progress
					ps->doubled[side] |= normmark[from];
					ps->double_d[side][f] = BitCount(dir) - 1;
					assert(
						(ps->double_d[side][f] < 5)
							&& (ps->double_d[side][f] >= 0));
				} else {
// blocked - opposite pawn blocks progress
					ps->blocked[side] |= normmark[from];
					ps->block_d[side][f] = BitCount(dir) - 1;
					assert(
						(ps->block_d[side][f] < 5)
							&& (ps->block_d[side][f] >= 0));
				}
			}
			
// how many issues on the way to promotion
			dir = ps->spans[side][f][3];
			ps->issue_d[side][f] = BitCount(dir & b->maps[PAWN])
				+ BitCount(ps->half_att[opside][1] & dir)
				+ BitCount(ps->half_att[opside][0] & dir);
			if (ps->issue_d[side][f] > 7)
				ps->issue_d[side][f] = 7;

			dir = ps->spans[side][f][2];
			if (dir & ps->path_stop[side] & (b->maps[PAWN])) {
				if (dir & ps->path_stop[side] & (b->maps[PAWN])
					& b->colormaps[opside]) {
					ps->blocked2[side] |= normmark[from];
					tt1 = BitCount(dir);
					if ((((side == WHITE) && ((rank + tt1) == RANKi7))
						| ((side == BLACK) && ((rank - tt1) == RANKi2)))
						& (tt1 >= 3)) {
						tt1--;
					}
					ps->block_d2[side][f] = tt1 - 1;
					assert(
						(ps->block_d2[side][f] < 5)
							&& (ps->block_d2[side][f] >= 0));
				}
			}

			dir = ps->spans[side][f][0];
			if (!(dir & ps->path_stop[side] & (b->maps[PAWN]))) {
// stopped - opposite pawn attacks path to promotion, how far
				ps->stopped[side] |= normmark[from];
				ps->stop_d[side][f] = BitCount(dir) - 1;
			}
		}
// can I be directly protected? _surr is square around PAWN
		temp = (side == WHITE) ?
			(attack.pawn_surr[from]
				& (~(attack.uphalf[from] | attack.file[from]))) :
			(attack.pawn_surr[from]
				& (~(attack.downhalf[from] | attack.file[from])));

// I am directly protected
		if (temp & b->maps[PAWN] & b->colormaps[side]) {
			ps->prot_dir[side] |= normmark[from];
			ps->prot_dir_d[side][f] = BitCount(
				temp & b->maps[PAWN] & b->colormaps[side]);
		}
		if (temp & ps->paths[side]) {
// somebody from behind can reach me
			ps->prot_p_d[side][f] = 8;
			ps->prot_p_c[side][f] = EMPTYBITMAP;
			i = 0;
			n = ps->pawns[side][i];
			while (n != -1) {
				if (ps->spans[side][i][0] & temp) {
					x = getRank(n);
					r = side == WHITE ? rank - x : x - rank;
					if (r >= 2) {
						if ((dpush) && (r >= 3))
							r--;
						r -= 2;
						if (ps->prot_p_d[side][f] > r)
							ps->prot_p_d[side][f] = r;

// store who is protecting me - map of protectors of f
						ps->prot_p_c[side][f] |= normmark[n];

// store who I protect - map of pawns i protects
						ps->prot_p_p[side][i] |= normmark[from];
					}
				}
				n = ps->pawns[side][++i];
			}
			if (ps->prot_p_d[side][f] < 8) {
				ps->prot_p[side] |= normmark[from];
			}
			assert(
				((ps->prot_p_d[side][f] <= 3) && (ps->prot_p_d[side][f] >= 0))
					|| (ps->prot_p_d[side][f] == 8));
		}
		temp = 0;
		if (file > FILEiA)
			temp |= ((dir & ps->paths[side]) >> 1);
		if (file < FILEiH)
			temp |= ((dir & ps->paths[side]) << 1);
		if (temp & b->maps[PAWN] & b->colormaps[side]) {
// I can reach somebody
			ps->prot[side] |= normmark[from];
			ps->prot_d[side][f] = 8;
			t2 = temp & b->maps[PAWN] & b->colormaps[side];
			while (t2) {
				tt1 = LastOne(t2);
				tt2 = getRank(tt1);
				if (side == WHITE) {
					if ((tt2 - rank) < ps->prot_d[side][f]) {
						ps->prot_d[side][f] = (tt2 - rank);
						assert(
							(ps->prot_d[side][f] < 8)
								&& (ps->prot_d[side][f] >= 0));
					}
				} else {
					if ((rank - tt2) < ps->prot_d[side][f]) {
						ps->prot_d[side][f] = (rank - tt2);
						if (!((ps->prot_d[side][f] < 8)
							&& (ps->prot_d[side][f] >= 0))) {
							assert(
								(ps->prot_d[side][f] < 8)
									&& (ps->prot_d[side][f] >= 0));
						}
					}
				}
				ClrLO(t2);
			}
		}

// i cannot be protected and cannot progress to promotion, so backward
		if ((((ps->prot_dir[side] | ps->prot[side] | ps->prot_p[side])
			& normmark[from]) == 0)
			&& (normmark[from] & (ps->blocked[side] | ps->stopped[side]))) {
			ps->back[side] |= normmark[from];
		}
// isolated
		if (file > 0) {
			if ((attack.rays[A1 + file - 1][A8 + file - 1]
				& (b->maps[PAWN] & b->colormaps[side])) == 0) {
				ps->half_isol[side][0] |= normmark[from];
			}
		}
		if (file < 7) {
			if ((attack.rays[A1 + file + 1][A8 + file + 1]
				& (b->maps[PAWN] & b->colormaps[side])) == 0) {
				ps->half_isol[side][1] |= normmark[from];
			}
		}
		from = ps->pawns[side][++f];
	}
	f = 0;
	from = ps->pawns[side][f];
	while (from != -1) {
		ps->prot_p_c_d[side][f] = BitCount(ps->prot_p_c[side][f]);
		ps->prot_p_p_d[side][f] = BitCount(ps->prot_p_p[side][f]);
		from = ps->pawns[side][++f];
	}
	
	return 0;
}

/*
 * doubled
 * isolated
 * 1st defense line
 * 2nd defense line	
 * PAWN se pocita pro dve situace je soucasti / neni soucasti stitu
 * zaroven jsou bonusy / penalty ktere se aplikuji jen kdyz jsou napr HEAVY OP na sachovnici
 *
 * heavy 		=> stit a, h, m, bez
 * non heavy 	=> stit a, h, m, bez
 */

/* shopt computed as change to sh 
 *
 */

int analyze_pawn_shield_singleN(board const *b, attack_model const *a, PawnStore *ps, int side, int pawn, int from, int sh, int shopt, personality const *p, BITVAR shlt,  stacker *st)
{
	BITVAR x, fst, sec, n2;
	int l, opside, f, fn, fn2;

	if (side == WHITE) {
		opside = BLACK;
		fst = RANK2;
		sec = RANK3;
	} else {
		opside = WHITE;
		fst = RANK7;
		sec = RANK6;
	}
	
	x = normmark[from];

// if simple_EVAL then only material and PSQ are used - KDE????
	if (p->simple_EVAL != 1) {
		if (x & (~(fst | sec))) {
//			L0("Shield\n");
			ps->t_sc[side][pawn][sh].sqr_b += (p->pshelter_out_penalty[MG]);
			ps->t_sc[side][pawn][sh].sqr_e += (p->pshelter_out_penalty[EG]);
#ifdef TUNING
			ADD_STACKER(st,pshelter_out_penalty[MG], 1, sh, side);
			ADD_STACKER(st,pshelter_out_penalty[EG], 1, sh, side);
#endif
		} else {
			l = BitCount(x & ps->half_isol[side][0])
				+ BitCount(x & ps->half_isol[side][1]);
			ps->t_sc[side][pawn][sh].sqr_b += (p->pshelter_isol_penalty[MG] * l)
				/ 2;
			ps->t_sc[side][pawn][sh].sqr_e += (p->pshelter_isol_penalty[EG] * l)
				/ 2;

#ifdef TUNING
			ADD_STACKER(st, pshelter_isol_penalty[MG], l/2, sh, side)
			ADD_STACKER(st, pshelter_isol_penalty[EG], l/2, sh, side)
#endif

// doubled pawn - one pawn in shelter other anywhere, might apply only when both pawns are in shelter area????
			if ((x & ps->doubled[side])) {
				ps->t_sc[side][pawn][sh].sqr_b +=
					(p->pshelter_double_penalty[MG]);
				ps->t_sc[side][pawn][sh].sqr_e +=
					(p->pshelter_double_penalty[EG]);
#ifdef TUNING
			ADD_STACKER(st, pshelter_double_penalty[MG], 1, sh, side);
			ADD_STACKER(st, pshelter_double_penalty[EG], 1, sh, side);
#endif
			}
			if (x & fst) {
				ps->t_sc[side][pawn][sh].sqr_b += (p->pshelter_prim_bonus[MG]);
				ps->t_sc[side][pawn][sh].sqr_e += (p->pshelter_prim_bonus[EG]);
#ifdef TUNING
			ADD_STACKER(st, pshelter_prim_bonus[MG], 1, sh, side);
			ADD_STACKER(st, pshelter_prim_bonus[EG], 1, sh, side);
#endif
			}
			if (x & sec) {
				ps->t_sc[side][pawn][sh].sqr_b += (p->pshelter_sec_bonus[MG]);
				ps->t_sc[side][pawn][sh].sqr_e += (p->pshelter_sec_bonus[EG]);
#ifdef TUNING
			ADD_STACKER(st, pshelter_sec_bonus[MG], 1, sh, side);
			ADD_STACKER(st, pshelter_sec_bonus[EG], 1, sh, side);
#endif
			}
// directly protected
			if (ps->prot_dir[side] & x) {
				ps->t_sc[side][pawn][sh].sqr_b +=
					p->pshelter_dir_protect[MG][side][ps->prot_dir_d[side][pawn]];
				ps->t_sc[side][pawn][sh].sqr_e +=
					p->pshelter_dir_protect[EG][side][ps->prot_dir_d[side][pawn]];
#ifdef TUNING
			ADD_STACKER(st, pshelter_dir_protect[MG][side][ps->prot_dir_d[side][pawn]], 1, sh, side);
			ADD_STACKER(st, pshelter_dir_protect[EG][side][ps->prot_dir_d[side][pawn]], 1, sh, side);
#endif
			}

// blocked - enemy pawn approaching
			if (ps->blocked2[side] & x) {
				ps->t_sc[side][pawn][sh].sqr_b +=
					p->pshelter_blocked_penalty[MG][side][ps->block_d2[side][pawn]];
				ps->t_sc[side][pawn][sh].sqr_e +=
					p->pshelter_blocked_penalty[EG][side][ps->block_d2[side][pawn]];
#ifdef TUNING
			ADD_STACKER(st, pshelter_blocked_penalty[MG][side][ps->block_d2[side][pawn]], 1, sh, side);
			ADD_STACKER(st, pshelter_blocked_penalty[EG][side][ps->block_d2[side][pawn]], 1, sh, side);
#endif
			}
// stopped - enemy pawn approaching 
			if (ps->stopped[side] & x) {
				ps->t_sc[side][pawn][sh].sqr_b +=
					p->pshelter_stopped_penalty[MG][side][ps->stop_d[side][pawn]];
				ps->t_sc[side][pawn][sh].sqr_e +=
					p->pshelter_stopped_penalty[EG][side][ps->stop_d[side][pawn]];
#ifdef TUNING
			ADD_STACKER(st, pshelter_stopped_penalty[MG][side][ps->stop_d[side][pawn]], 1, sh, side);
			ADD_STACKER(st, pshelter_stopped_penalty[EG][side][ps->stop_d[side][pawn]], 1, sh, side);
#endif
			}

			if (x & (~ps->not_pawns_file[side])
				& (ps->not_pawns_file[opside])) {
				ps->t_sc[side][pawn][shopt].sqr_b +=
					(p->pshelter_hopen_penalty[MG]);
				ps->t_sc[side][pawn][shopt].sqr_e +=
					(p->pshelter_hopen_penalty[EG]);
#ifdef TUNING
			ADD_STACKER(st, pshelter_hopen_penalty[MG], 1, shopt, side);
			ADD_STACKER(st, pshelter_hopen_penalty[EG], 1, shopt, side);
#endif
			}
		}
		
//		ps->t_sc[side][pawn][shopt].sqr_b += ps->t_sc[side][pawn][sh].sqr_b;
//		ps->t_sc[side][pawn][shopt].sqr_e += ps->t_sc[side][pawn][sh].sqr_e;

// if at shelter position, unapply normal PST
		if (x & ((fst | sec))) {
//			ps->t_sc[side][pawn][sh].sqr_b -=p->piecetosquare[MG][side][PAWN][from];
//			ps->t_sc[side][pawn][sh].sqr_e -=p->piecetosquare[EG][side][PAWN][from];
#ifdef TUNING
//			if(side==WHITE) {
//			ADD_STACKER(st, piecetosquare[MG][side][PAWN][from], -1, sh, side);
//			ADD_STACKER(st, piecetosquare[EG][side][PAWN][from], -1, sh, side);
//			} else
//			{
//			ADD_STACKER(st, piecetosquare[MG][side][PAWN][from], -1, sh, side);
//			ADD_STACKER(st, piecetosquare[EG][side][PAWN][from], -1, sh, side);
//			}
#endif
		}
	}
	return 0;
}

int analyze_pawn_shield_stub(board const *b, attack_model const *a, PawnStore *ps, int side, int pawn, int from, personality const *p, stacker *st)
{
BITVAR x;
	x = normmark[from];
	if (x & ps->shelter_p[side][0])
		analyze_pawn_shield_singleN(b, a, ps, side, pawn, from, SHa, SHah,
			p, ps->shelter_p[side][0], st);
	if (x & ps->shelter_p[side][1])
		analyze_pawn_shield_singleN(b, a, ps, side, pawn, from, SHh, SHhh,
			p, ps->shelter_p[side][1], st);
	if (x & ps->shelter_p[side][2])
		analyze_pawn_shield_singleN(b, a, ps, side, pawn, from, SHm, SHmh,
			p, ps->shelter_p[side][2], st);
return 0;
}

// analyze one potential variant of pawn shield
int analyze_pawn_shield_globN(board const *b, attack_model const *a, PawnStore *ps, int side, BITVAR mask, BITVAR pwns, int sh, int shopt, personality const *p, stacker *st)
{
	int count, opside, f, from, ppc, ppp, y, rank, r;
	BITVAR fst, sec, pawns, protectors, prots, tmp;
	int pr_stck[8], pr_stck_idx=-1;
	int score[2],i;

	opside = Flip(side);
	if (side == WHITE) {
		opside = BLACK;
		fst = RANK2;
		sec = RANK3;
	} else {
		opside = WHITE;
		fst = RANK7;
		sec = RANK6;
	}
	if ((ps->not_pawns_file[side] & ps->not_pawns_file[opside]) & mask) {
		count =
			BitCount(
				ps->not_pawns_file[side] & ps->not_pawns_file[opside] & mask
					& RANK2);
		ps->score[side][shopt].sqr_b += (p->pshelter_open_penalty[MG] * count);
		ps->score[side][shopt].sqr_e += (p->pshelter_open_penalty[EG] * count);
#ifdef TUNING
		ADD_STACKER(st, pshelter_open_penalty[MG], count, shopt, side);
		ADD_STACKER(st, pshelter_open_penalty[EG], count, shopt, side);
#endif
	}

// get pawn in shelter & exclude out of shelter ones

	pawns=pwns&(fst|sec);

// get ones that are protectors
// store who is protecting me - map of protectors of f
//		ps->prot_p_c[side][f] |= normmark[n];

// store who I protect - map of pawns i protects
//		ps->prot_p_p[side][i] |= normmark[from];

//		ps->prot_p_c_d[side][f] = BitCount(ps->prot_p_c[side][f]);
//		ps->prot_p_p_d[side][f] = BitCount(ps->prot_p_p[side][f]);

//		p->pawn_pot_protect[EG][side][ps->prot_p_d[side][f]];
//		p->pawn_protect_count[MG][side][ps->prot_p_c_d[side][f]];
//		p->pawn_prot_over_penalty[MG][side][ps->prot_p_p_d[side][f]];

// build map of non shelter pawns protected by shelter ones
	protectors=0;
	f=0;
	score[MG]=score[EG]=0;
	from=ps->pawns[side][f];
	while (from != -1) {
		if((ps->prot_p_c[side][f] & pawns)&&(normmark[from]&&(~pawns))) {
			pr_stck[++pr_stck_idx]=f;
			protectors|=(ps->prot_p_c[side][f] & pawns);
		}
		from=ps->pawns[side][++f];
	}
	
// replay those protected and fix score for num of protectors etc...
	for(f=0;f<=pr_stck_idx;f++){
		i=pr_stck[f];
		from=ps->pawns[side][i];

		ps->t_sc[side][i][sh].sqr_b -=p->pawn_pot_protect[MG][side][ps->prot_p_d[side][i]];
		ps->t_sc[side][i][sh].sqr_e -=p->pawn_pot_protect[EG][side][ps->prot_p_d[side][i]];

#ifdef TUNING
		ADD_STACKER(st, pawn_pot_protect[MG][side][ps->prot_p_d[side][i]], -1, sh, side);
		ADD_STACKER(st, pawn_pot_protect[EG][side][ps->prot_p_d[side][i]], -1, sh, side);
#endif

		tmp=ps->prot_p_c[side][i]&(~pawns);
		ppc=BitCount(tmp);
		
		ps->t_sc[side][i][sh].sqr_b -=p->pawn_protect_count[MG][side][ps->prot_p_c_d[side][i]];
		ps->t_sc[side][i][sh].sqr_e -=p->pawn_protect_count[EG][side][ps->prot_p_c_d[side][i]];
		
#ifdef TUNING
		ADD_STACKER(st, pawn_protect_count[MG][side][ps->prot_p_c_d[side][i]], -1, sh, side);
		ADD_STACKER(st, pawn_protect_count[EG][side][ps->prot_p_c_d[side][i]], -1, sh, side);
#endif

		if(ppc>0) {
			ps->t_sc[side][i][sh].sqr_b +=p->pawn_protect_count[MG][side][ppc];
			ps->t_sc[side][i][sh].sqr_e +=p->pawn_protect_count[EG][side][ppc];
#ifdef TUNING
			ADD_STACKER(st, pawn_protect_count[MG][side][ppc], 1, sh, side);
			ADD_STACKER(st, pawn_protect_count[EG][side][ppc], 1, sh, side);
#endif
// protector distance
			assert(ppc==1);
			y=getRank(LastOne(tmp));
			rank=getRank(from);
			r = side == WHITE ? rank - y : y - rank;
			if(r >= 2) r--;
			r--;
			ps->t_sc[side][i][sh].sqr_b +=p->pawn_pot_protect[MG][side][r];
			ps->t_sc[side][i][sh].sqr_e +=p->pawn_pot_protect[EG][side][r];

#ifdef TUNING
			ADD_STACKER(st, pawn_pot_protect[MG][side][r], 1, sh, side);
			ADD_STACKER(st, pawn_pot_protect[EG][side][r], 1, sh, side);
#endif
		}
	}
	
// fix over protect
//		p->pawn_prot_over_penalty[MG][side][ps->prot_p_p_d[side][f]];

	f=0;
	from=ps->pawns[side][f];
		while (from != -1) {
			if(normmark[from]&protectors) {
				ps->t_sc[side][f][sh].sqr_b -=p->pawn_prot_over_penalty[MG][side][ps->prot_p_p_d[side][f]];
				ps->t_sc[side][f][sh].sqr_e -=p->pawn_prot_over_penalty[EG][side][ps->prot_p_p_d[side][f]];
#ifdef TUNING
				ADD_STACKER(st, pawn_prot_over_penalty[MG][side][ps->prot_p_p_d[side][f]], -1, sh, side);
				ADD_STACKER(st, pawn_prot_over_penalty[EG][side][ps->prot_p_p_d[side][f]], -1, sh, side);
#endif
				tmp=ps->prot_p_p[side][f]&pawns;
				ppc=BitCount(tmp);
				if(tmp>0) {
					ps->t_sc[side][f][sh].sqr_b +=p->pawn_prot_over_penalty[MG][side][ps->prot_p_p_d[side][ppc]];
					ps->t_sc[side][f][sh].sqr_e +=p->pawn_prot_over_penalty[EG][side][ps->prot_p_p_d[side][ppc]];
#ifdef TUNING
					ADD_STACKER(st, pawn_prot_over_penalty[MG][side][ppc], 1, sh, side);
					ADD_STACKER(st, pawn_prot_over_penalty[EG][side][ppc], 1, sh, side);
#endif
				};
			}
			from=ps->pawns[side][++f];
		}

	return 0;
}

/*
 * analyze shelter variants with or without heavy opposition
 */

// BAs absolute, all relative to BAs

int analyze_pawn_shieldN(board const *b, attack_model const *a, PawnStore *ps, personality const *p, stacker *st)
{
	int f;
	int side;

	// analyze shelters not related to individual pawn
	for (side = 0; side <= 1; side++) {
		analyze_pawn_shield_globN(b, a, ps, side, SHELTERA, ps->shelter_p[side][0], SHa, SHah, p, st);
		analyze_pawn_shield_globN(b, a, ps, side, SHELTERH, ps->shelter_p[side][1], SHh, SHhh, p, st);
		analyze_pawn_shield_globN(b, a, ps, side, SHELTERM, ps->shelter_p[side][2], SHm, SHmh, p, st);
	}

	return 0;}

/*
 * Precompute various possible scenarios, their use depends on king position, heavy pieces availability etc
 * Prepare two basic scenarios BAs - no heavy opposition and no pawn shelter
 *
 * BAs is absolute, others are relative to these bases
 * base variant is used at eval_pawn, proper shelter is added in eval_king
 */

int pre_evaluate_pawns(board const *b, attack_model const *a, PawnStore *ps, personality const *p, stacker *st)
{
	int f, ff, from;
	int side, opside;
	BITVAR msk;
	BITVAR x;

	for (side = 0; side <= 1; side++) {
		opside = Flip(side);
		f = 0;
		from = ps->pawns[side][f];
		while (from != -1) {
			x = normmark[from];
			
// here we trigger single pawn shelter analysis
			analyze_pawn_shield_stub(b, a, ps, side, f, from, p, st);
// PSQ
			ps->t_sc[side][f][BAs].sqr_b +=
				p->piecetosquare[MG][side][PAWN][from];
			ps->t_sc[side][f][BAs].sqr_e +=
				p->piecetosquare[EG][side][PAWN][from];
#ifdef TUNING
			if(side==WHITE) {
			ADD_STACKER(st, piecetosquare[MG][side][PAWN][from], 1, BAs, side)
			ADD_STACKER(st, piecetosquare[EG][side][PAWN][from], 1, BAs, side)
			} else
			{
			ADD_STACKER(st, piecetosquare[MG][side][PAWN][from], 1, BAs, side)
			ADD_STACKER(st, piecetosquare[EG][side][PAWN][from], 1, BAs, side)
			}
#endif

// if simple_EVAL then only material and PSQ are used
			if (p->simple_EVAL != 1) {
// isolated
				if ((ps->half_isol[side][0] | ps->half_isol[side][1]) & x) {
					if (ps->half_isol[side][0] & x) {
						ps->t_sc[side][f][BAs].sqr_b += p->isolated_penalty[MG];
						ps->t_sc[side][f][BAs].sqr_e += p->isolated_penalty[EG];

#ifdef TUNING
		ADD_STACKER(st, isolated_penalty[MG], 1, BAs, side)
		ADD_STACKER(st, isolated_penalty[EG], 1, BAs, side)
#endif

//???? unaply for shelter
					  if ((x & ps->not_pawns_file[opside])) {
						ps->t_sc[side][f][HEa].sqr_b +=
							p->pawn_iso_onopen_penalty[MG];
						ps->t_sc[side][f][HEa].sqr_e +=
							p->pawn_iso_onopen_penalty[EG];

#ifdef TUNING
		ADD_STACKER(st, pawn_iso_onopen_penalty[MG], 1, HEa, side)
		ADD_STACKER(st, pawn_iso_onopen_penalty[EG], 1, HEa, side)
#endif

					  }
					}
					if (ps->half_isol[side][1] & x) {
						ps->t_sc[side][f][BAs].sqr_b += p->isolated_penalty[MG];
						ps->t_sc[side][f][BAs].sqr_e += p->isolated_penalty[EG];
#ifdef TUNING
		ADD_STACKER(st, isolated_penalty[MG], 1, BAs, side)
		ADD_STACKER(st, isolated_penalty[EG], 1, BAs, side)
#endif
					  if ((x & ps->not_pawns_file[opside])) {
						ps->t_sc[side][f][HEa].sqr_b +=
							p->pawn_iso_onopen_penalty[MG];
						ps->t_sc[side][f][HEa].sqr_e +=
							p->pawn_iso_onopen_penalty[EG];
#ifdef TUNING
		ADD_STACKER(st, pawn_iso_onopen_penalty[MG], 1, HEa, side)
		ADD_STACKER(st, pawn_iso_onopen_penalty[EG], 1, HEa, side)
#endif
					  }
					}
					if (x & CENTEREXBITMAP) {
						ps->t_sc[side][f][BAs].sqr_b +=
							p->pawn_iso_center_penalty[MG];
						ps->t_sc[side][f][BAs].sqr_e +=
							p->pawn_iso_center_penalty[EG];
#ifdef TUNING
		ADD_STACKER(st, pawn_iso_center_penalty[MG], 1, BAs, side)
		ADD_STACKER(st, pawn_iso_center_penalty[EG], 1, BAs, side)
#endif
					}
				}
// blocked
				if (ps->blocked[side] & x) {
					ps->t_sc[side][f][BAs].sqr_b +=
						p->pawn_blocked_penalty[MG][side][ps->block_d[side][f]];
					ps->t_sc[side][f][BAs].sqr_e +=
						p->pawn_blocked_penalty[EG][side][ps->block_d[side][f]];

#ifdef TUNING
		ADD_STACKER(st, pawn_blocked_penalty[MG][side][ps->block_d[side][f]], 1, BAs, side)
		ADD_STACKER(st, pawn_blocked_penalty[EG][side][ps->block_d[side][f]], 1, BAs, side)
#endif

				}
// stopped
				if (ps->stopped[side] & x) {
					ps->t_sc[side][f][BAs].sqr_b +=
						p->pawn_stopped_penalty[MG][side][ps->stop_d[side][f]];
					ps->t_sc[side][f][BAs].sqr_e +=
						p->pawn_stopped_penalty[EG][side][ps->stop_d[side][f]];

#ifdef TUNING
		ADD_STACKER(st, pawn_stopped_penalty[MG][side][ps->stop_d[side][f]], 1, BAs, side)
		ADD_STACKER(st, pawn_stopped_penalty[MG][side][ps->stop_d[side][f]], 1, BAs, side)
#endif

				}
// doubled
				if (ps->doubled[side] & x) {
					ps->t_sc[side][f][BAs].sqr_b +=
						p->doubled_n_penalty[MG][side][ps->double_d[side][f]];
					ps->t_sc[side][f][BAs].sqr_e +=
						p->doubled_n_penalty[EG][side][ps->double_d[side][f]];

#ifdef TUNING
		ADD_STACKER(st, doubled_n_penalty[MG][side][ps->double_d[side][f]], 1, BAs, side)
		ADD_STACKER(st, doubled_n_penalty[EG][side][ps->double_d[side][f]], 1, BAs, side)
#endif

				}
// protected
//				if(ps->prot[side]&x){
//					ps->t_sc[side][f][BAs].sqr_b+=p->pawn_n_protect[0][side][ps->prot_d[side][f]];
//					ps->t_sc[side][f][BAs].sqr_e+=p->pawn_n_protect[1][side][ps->prot_d[side][f]];
//				}
				if (ps->prot_p[side] & x) {
					ps->t_sc[side][f][BAs].sqr_b +=
						p->pawn_pot_protect[MG][side][ps->prot_p_d[side][f]];
					ps->t_sc[side][f][BAs].sqr_e +=
						p->pawn_pot_protect[EG][side][ps->prot_p_d[side][f]];

#ifdef TUNING
		ADD_STACKER(st, pawn_pot_protect[MG][side][ps->prot_p_d[side][f]], 1, BAs, side)
		ADD_STACKER(st, pawn_pot_protect[EG][side][ps->prot_p_d[side][f]], 1, BAs, side)
#endif
				}

// honor number of potential protectors
				ps->t_sc[side][f][BAs].sqr_b +=
					p->pawn_protect_count[MG][side][ps->prot_p_c_d[side][f]];
				ps->t_sc[side][f][BAs].sqr_e +=
					p->pawn_protect_count[EG][side][ps->prot_p_c_d[side][f]];

#ifdef TUNING
		ADD_STACKER(st, pawn_protect_count[MG][side][ps->prot_p_c_d[side][f]], 1, BAs, side)
		ADD_STACKER(st, pawn_protect_count[EG][side][ps->prot_p_c_d[side][f]], 1, BAs, side)
#endif

// penalize overloading - one protects too much pawns
				ps->t_sc[side][f][BAs].sqr_b +=
					p->pawn_prot_over_penalty[MG][side][ps->prot_p_p_d[side][f]];
				ps->t_sc[side][f][BAs].sqr_e +=
					p->pawn_prot_over_penalty[EG][side][ps->prot_p_p_d[side][f]];
#ifdef TUNING
		ADD_STACKER(st, pawn_prot_over_penalty[MG][side][ps->prot_p_p_d[side][f]], 1, BAs, side)
		ADD_STACKER(st, pawn_prot_over_penalty[EG][side][ps->prot_p_p_d[side][f]], 1, BAs, side)
#endif
// penalize number of issues pawn has 
				ps->t_sc[side][f][BAs].sqr_b +=
					p->pawn_issues_penalty[MG][side][ps->issue_d[side][f]];
				ps->t_sc[side][f][BAs].sqr_e +=
					p->pawn_issues_penalty[EG][side][ps->issue_d[side][f]];
#ifdef TUNING
		ADD_STACKER(st, pawn_issues_penalty[MG][side][ps->issue_d[side][f]], 1, BAs, side)
		ADD_STACKER(st, pawn_issues_penalty[EG][side][ps->issue_d[side][f]], 1, BAs, side)
#endif

// directly protected
				if (ps->prot_dir[side] & x) {
					ps->t_sc[side][f][BAs].sqr_b +=
						p->pawn_dir_protect[MG][side][ps->prot_dir_d[side][f]];
					ps->t_sc[side][f][BAs].sqr_e +=
						p->pawn_dir_protect[EG][side][ps->prot_dir_d[side][f]];
#ifdef TUNING
		ADD_STACKER(st, pawn_dir_protect[MG][side][ps->prot_dir_d[side][f]], 1, BAs, side)
		ADD_STACKER(st, pawn_dir_protect[MG][side][ps->prot_dir_d[side][f]], 1, BAs, side)
#endif
				}
// backward,ie unprotected, not able to promote, not completely isolated
				if (ps->back[side]
					& (ps->blocked[side] | ps->stopped[side] | ps->doubled[side])
					& (~(ps->half_isol[side][0] & ps->half_isol[side][1]))
					& x) {
					ps->t_sc[side][f][BAs].sqr_b += p->backward_penalty[MG];
					ps->t_sc[side][f][BAs].sqr_e += p->backward_penalty[EG];
#ifdef TUNING
		ADD_STACKER(st, backward_penalty[MG], 1, BAs, side)
		ADD_STACKER(st, backward_penalty[MG], 1, BAs, side)
#endif
				}
// potential passer ?
				if (ps->pas_d[side][f] < 8) {
					ps->t_sc[side][f][BAs].sqr_b +=
						p->passer_bonus[MG][side][ps->pas_d[side][f]];
					ps->t_sc[side][f][BAs].sqr_e +=
						p->passer_bonus[EG][side][ps->pas_d[side][f]];
#ifdef TUNING
		ADD_STACKER(st, passer_bonus[MG][side][ps->pas_d[side][f]], 1, BAs, side)
		ADD_STACKER(st, passer_bonus[EG][side][ps->pas_d[side][f]], 1, BAs, side)
#endif
				}
// weak...
				if ((ps->back[side] | ps->blocked[side] | ps->stopped[side]
					| ps->doubled[side]) & x) {
// in center				
					if (x & CENTEREXBITMAP) {
						ps->t_sc[side][f][BAs].sqr_b +=
							p->pawn_weak_center_penalty[MG];
						ps->t_sc[side][f][BAs].sqr_e +=
							p->pawn_weak_center_penalty[EG];
#ifdef TUNING
		ADD_STACKER(st, pawn_weak_center_penalty[MG], 1, BAs, side)
		ADD_STACKER(st, pawn_weak_center_penalty[EG], 1, BAs, side)
#endif
					}
					if ((x & ps->not_pawns_file[opside])) {
						ps->t_sc[side][f][HEa].sqr_b +=
							p->pawn_weak_onopen_penalty[MG];
						ps->t_sc[side][f][HEa].sqr_e +=
							p->pawn_weak_onopen_penalty[EG];
#ifdef TUNING
		ADD_STACKER(st, pawn_weak_onopen_penalty[MG], 1, HEa, side)
		ADD_STACKER(st, pawn_weak_onopen_penalty[EG], 1, HEa, side)
#endif
					}
				}
// fix material value
				if (x & (FILEA | FILEH)) {
					ps->t_sc[side][f][BAs].sqr_b += p->pawn_ah_penalty[MG];
					ps->t_sc[side][f][BAs].sqr_e += p->pawn_ah_penalty[EG];
#ifdef TUNING
		ADD_STACKER(st, pawn_ah_penalty[MG], 1, BAs, side)
		ADD_STACKER(st, pawn_ah_penalty[EG], 1, BAs, side)
#endif
				}
// mobility, but related to PAWNS only, other pieces are treated like non existant
				msk = p->mobility_protect == 1 ?
				FULLBITMAP : ~b->colormaps[side];
				ff = BitCount(a->pa_mo[side] & attack.pawn_move[side][from]
						& (~b->maps[PAWN]))
					+ BitCount(a->pa_at[side] & attack.pawn_att[side][from] & msk);
				ps->t_sc[side][f][BAs].sqr_b += p->mob_val[MG][side][PAWN][0] * ff;
				ps->t_sc[side][f][BAs].sqr_e += p->mob_val[EG][side][PAWN][0] * ff;
#ifdef TUNING
		ADD_STACKER(st, mob_val[MG][side][PAWN][0], ff, BAs, side)
		ADD_STACKER(st, mob_val[EG][side][PAWN][0], ff, BAs, side)
#endif

#if 0
		ADD_STACKER(st, , 1, BAs, side)
		ADD_STACKER(st, , 1, BAs, side)
#endif

			}
			from = ps->pawns[side][++f];
		}

// up to here basics are filled, incl HEa and SHxx variants
// all relative to BAs
	}
	return 0;
}

/*
 * evaluation should be called in quiet situation, so no attacks should be on the board
 * but in the case... if pawn is attacking pawn there should be some penalty 
 * or some other mechanism like not counting bonuses for attacked pawns
 */

int premake_pawn_model(board const *b, attack_model const *a, hashPawnEntry **hhh, personality const *p, stacker *st)
{
	int f, ff, file, n, i, from, sq_file[8], f1, f2;
	int tt, tt1, tt2, side;
	BITVAR ss1, ss2;
	BITVAR temp, x;
	PawnStore *ps;

	hashPawnEntry *h2, *hash;

	hash = *hhh;
	hash->key = b->pawnkey;
	hash->map = b->maps[PAWN];

	h2 = (b->hps != NULL) ? retrievePawnHash(b->hps, hash, b->stats) : NULL;
	if (h2 == NULL) {
		ps = &(hash->value);
		// attacks halves
		ps->half_att[WHITE][1] = (((b->maps[PAWN] & b->colormaps[WHITE])
			& (~(FILEH | RANK8))) << 9);
		ps->half_att[WHITE][0] = (((b->maps[PAWN] & b->colormaps[WHITE])
			& (~(FILEA | RANK8))) << 7);
		ps->half_att[BLACK][0] = (((b->maps[PAWN] & b->colormaps[BLACK])
			& (~(FILEH | RANK1))) >> 7);
		ps->half_att[BLACK][1] = (((b->maps[PAWN] & b->colormaps[BLACK])
			& (~(FILEA | RANK1))) >> 9);

		//double attacked
		ps->double_att[WHITE] = ps->half_att[WHITE][0] & ps->half_att[WHITE][1];
		ps->double_att[BLACK] = ps->half_att[BLACK][0] & ps->half_att[BLACK][1];

		// single attacked
		ps->odd_att[WHITE] = ps->half_att[WHITE][0] ^ ps->half_att[WHITE][1];
		ps->odd_att[BLACK] = ps->half_att[BLACK][0] ^ ps->half_att[BLACK][1];

		// squares properly defended
		ps->safe_att[WHITE] = (ps->double_att[WHITE]
			| ~(ps->half_att[BLACK][0] | ps->half_att[BLACK][1])
			| (ps->odd_att[WHITE] & ~ps->double_att[BLACK]));
		ps->safe_att[BLACK] = (ps->double_att[BLACK]
			| ~(ps->half_att[WHITE][0] | ps->half_att[WHITE][1])
			| (ps->odd_att[BLACK] & ~ps->double_att[WHITE]));

		// paths including stops
		ps->path_stop[WHITE] = FillNorth(b->maps[PAWN] & b->colormaps[WHITE],
			ps->safe_att[WHITE] & ~b->maps[PAWN],
			b->maps[PAWN] & b->colormaps[WHITE]);
		ps->path_stop[BLACK] = FillSouth(b->maps[PAWN] & b->colormaps[BLACK],
			ps->safe_att[BLACK] & ~b->maps[PAWN],
			b->maps[PAWN] & b->colormaps[BLACK]);

		// is path up to promotion square?
		ps->pass_end[WHITE] = ps->path_stop[WHITE] & attack.rank[A8];
		ps->pass_end[BLACK] = ps->path_stop[BLACK] & attack.rank[A1];

		// safe paths
		ps->paths[WHITE] = ((ps->path_stop[WHITE] >> 8)
			& (~(b->maps[PAWN] & b->colormaps[WHITE]))) | ps->pass_end[WHITE];
		ps->paths[BLACK] = ((ps->path_stop[BLACK] << 8)
			& (~(b->maps[PAWN] & b->colormaps[BLACK]))) | ps->pass_end[BLACK];
		// stops only
		ps->path_stop2[WHITE] = ps->path_stop[WHITE] ^ ps->paths[WHITE];
		ps->path_stop2[BLACK] = ps->path_stop[BLACK] ^ ps->paths[BLACK];
		/*
		 * holes/outpost (in enemy pawns) - squares covered by my pawns only
		 * but not reachable by enemy pawns - for my minor pieces. In center or opponent half of board.
		 */
		// squares attacked by my pawns only
		ps->one_side[WHITE] = ((ps->half_att[WHITE][0] | ps->half_att[WHITE][1])
			& (~(ps->half_att[BLACK][0] | ps->half_att[BLACK][1])));
		ps->one_side[BLACK] = ((ps->half_att[BLACK][0] | ps->half_att[BLACK][1])
			& (~(ps->half_att[WHITE][0] | ps->half_att[WHITE][1])));

		// pawn attacks from hole/outpost for analysing opponent pawn reachability
		ps->one_s_att[WHITE][1] = (((ps->one_side[WHITE]) & (~(FILEH | RANK8)))
			<< 9);
		ps->one_s_att[WHITE][0] = (((ps->one_side[WHITE]) & (~(FILEA | RANK8)))
			<< 7);
		ps->one_s_att[BLACK][0] = (((ps->one_side[BLACK]) & (~(FILEH | RANK1)))
			>> 7);
		ps->one_s_att[BLACK][1] = (((ps->one_side[BLACK]) & (~(FILEA | RANK1)))
			>> 9);

// prepare bitmaps of potential shelter pawns - even out of shelter ones
		ps->shelter_p[WHITE][0] = ps->shelter_p[BLACK][0] = 0;
		ps->shelter_p[WHITE][1] = ps->shelter_p[BLACK][1] = 0;
		ps->shelter_p[WHITE][2] = ps->shelter_p[BLACK][2] = 0;

// front & back spans
		for (f = 0; f < 8; f++) {
			ps->spans[WHITE][f][0] = ps->spans[BLACK][f][0] =
				ps->spans[WHITE][f][1] = ps->spans[BLACK][f][1] =
				EMPTYBITMAP;
		}
// iterate pawns by files, serialize
		f1 = f2 = 0;
		for (file = 0; file < 8; file++) {
			temp = attack.file[A1 + file];
			x = b->maps[PAWN] & b->colormaps[WHITE] & temp;
			i = 0;
			while (x) {
				n = LastOne(x);
				ps->pawns[WHITE][f1] = n;
				sq_file[i++] = n;
				f1++;
				ClrLO(x);
			}
			x = b->maps[PAWN] & b->colormaps[BLACK] & temp;
			while (x) {
				n = LastOne(x);
				ps->pawns[BLACK][f2] = n;
				sq_file[i++] = n;
				f2++;
				ClrLO(x);
			}

// sort pawns on file
// i has number of pawns on file
			for (n = i; n > 1; n--) {
				for (f = 1; f < n; f++) {
					if (getRank(sq_file[f]) < getRank(sq_file[f - 1])) {
						tt = sq_file[f - 1];
						sq_file[f - 1] = sq_file[f];
						sq_file[f] = tt;
					}
				}
			}
			if (i > 0) {
				// get pawns on file and assign them spans
				for (f = 0; f < i; f++) {
					tt = sq_file[f];
					if (f == 0)
						tt1 = getPos(file, 0);
					else
						tt1 = sq_file[f - 1];
					if (f == (i - 1))
						tt2 = getPos(file, 7);
					else
						tt2 = sq_file[f + 1];
					ss1 = attack.rays[tt][tt2] & (~normmark[tt]);
					ss2 = attack.rays[tt][tt1] & (~normmark[tt]);
					ff = 0;
					if (normmark[tt] & b->colormaps[WHITE]) {
						while ((ps->pawns[WHITE][ff] != tt)) {
							ff++;
						}
						assert(ps->pawns[WHITE][ff] == tt);
// store "unsafe" front span
						ps->spans[WHITE][ff][2] = ss1;
// cut the front span short if path is not safe
						ps->spans[WHITE][ff][0] = ss1 & ps->path_stop[WHITE];
						ps->spans[WHITE][ff][1] = ss2;
						ps->spans[WHITE][ff][3] = attack.dirs[tt][0]
							& (~normmark[tt]);
					} else {
						while ((ps->pawns[BLACK][ff] != tt)) {
							ff++;
						}
						assert(ps->pawns[BLACK][ff] == tt);
						ps->spans[BLACK][ff][2] = ss2;
						ps->spans[BLACK][ff][0] = ss2 & ps->path_stop[BLACK];
						ps->spans[BLACK][ff][1] = ss1;
						ps->spans[BLACK][ff][3] = attack.dirs[tt][4]
							& (~normmark[tt]);
					}
				}
			}
			/*
			 * ps->pawns[side][idx] contains pawn on the board
			 * ps->spans[side][idx][0] contains bitmap of frontspan of relevant pawn, it includes stop square, doesnt include pawn itself
			 * from stop square we can conclude reason - my pawn, opposide pawn, no piece - either we terminate at promotion row
			 * or the square is attacked by other pawn
			 * ps->spans[side][idx][1] is backspan - contains either pawn behind or first row
			 *
			 */
			ps->pawns[WHITE][f1] = -1;
			ps->pawns[BLACK][f2] = -1;
		}
		ps->stopped[WHITE] = ps->passer[WHITE] = ps->blocked[WHITE] =
			ps->blocked2[WHITE] = ps->isolated[WHITE] = ps->doubled[WHITE] =
				ps->back[WHITE] =
				EMPTYBITMAP;
		ps->stopped[BLACK] = ps->passer[BLACK] = ps->blocked[BLACK] =
			ps->blocked2[BLACK] = ps->isolated[BLACK] = ps->doubled[BLACK] =
				ps->back[BLACK] =
				EMPTYBITMAP;

		ps->half_isol[WHITE][0] = ps->half_isol[WHITE][1] =
			ps->half_isol[BLACK][0] = ps->half_isol[BLACK][1] =
			EMPTYBITMAP;
		ps->prot[WHITE] = ps->prot[BLACK] = ps->prot_p[WHITE] =
			ps->prot_p[BLACK] = ps->prot_dir[WHITE] = ps->prot_dir[BLACK] =
			EMPTYBITMAP;
		ps->not_pawns_file[WHITE] = ps->not_pawns_file[BLACK] =
		FULLBITMAP;

// setup SHELTER
		for (side = 0; side <= 1; side++) {
			f = 0;
			from = ps->pawns[side][f];
			while (from != -1) {
				ps->pawns_b[side][f] = normmark[from];
				if (ps->spans[side][f][1]
					& (~(b->maps[PAWN] & b->colormaps[side]))) {
					file = getFile(from);
					if ((file <= FILEiF) && (file >= FILEiD))
						ps->shelter_p[side][2] |= normmark[from];
					if (file <= FILEiC)
						ps->shelter_p[side][0] |= normmark[from];
					if (file >= FILEiF)
						ps->shelter_p[side][1] |= normmark[from];
				}
				from = ps->pawns[side][++f];
			}
		}

// analyze individual pawns
		analyze_pawn(b, a, ps, WHITE, p);
		analyze_pawn(b, a, ps, BLACK, p);

/* 
 * two basic models of PAWN evaluation
 * BAs - no PAWN shield, no Heavy pieces evaluated, HEa - heavy opposition no PAWN shield
 *
 * variants SHa, SHh, SHm - PAWN shield is considered for Left, Right, Middle position of king
 * variants SHah, SHhh, SHmm - evaluate PAWN shield with Heavy opposition
 */

		for (side = 0; side <= 1; side++) {
			for (f = 0; f < 8; f++) {
				ps->t_sc[side][f][BAs].sqr_b = ps->t_sc[side][f][BAs].sqr_e =
				ps->t_sc[side][f][HEa].sqr_b = ps->t_sc[side][f][HEa].sqr_e =
				ps->t_sc[side][f][SHa].sqr_b = ps->t_sc[side][f][SHa].sqr_e =
				ps->t_sc[side][f][SHh].sqr_b = ps->t_sc[side][f][SHh].sqr_e =
				ps->t_sc[side][f][SHm].sqr_b = ps->t_sc[side][f][SHm].sqr_e =
				ps->t_sc[side][f][SHah].sqr_b = ps->t_sc[side][f][SHah].sqr_e =
				ps->t_sc[side][f][SHhh].sqr_b = ps->t_sc[side][f][SHhh].sqr_e =
				ps->t_sc[side][f][SHmh].sqr_b = ps->t_sc[side][f][SHmh].sqr_e =
					0;
			}
			ps->score[side][BAs].sqr_b = ps->score[side][BAs].sqr_e =
			ps->score[side][HEa].sqr_b = ps->score[side][HEa].sqr_e =
			ps->score[side][SHa].sqr_b = ps->score[side][SHa].sqr_e =
			ps->score[side][SHh].sqr_b = ps->score[side][SHh].sqr_e =
			ps->score[side][SHm].sqr_b = ps->score[side][SHm].sqr_e =
			ps->score[side][SHah].sqr_b = ps->score[side][SHah].sqr_e =
			ps->score[side][SHhh].sqr_b = ps->score[side][SHhh].sqr_e =
			ps->score[side][SHmh].sqr_b = ps->score[side][SHmh].sqr_e =
				0;
		}
		ps->pot_sh[WHITE] = ps->pot_sh[BLACK] = 0;

		// compute scores that are only pawn related
		pre_evaluate_pawns(b, a, ps, p, st);
// all done - relative to BAs

		// evaluate & score shelter
		if (p->use_pawn_shelter != 0) 
			analyze_pawn_shieldN(b, a, ps, p, st);

/*
 * sum scores of PAWNS for each variant
 * base variants summary
 */

	int vars[] = { BAs, HEa, SHa, SHh, SHm, SHah, SHhh, SHmh, -1 };

// sum each variant
		for (side = 0; side <= 1; side++) {
			f = 0;
			from = ps->pawns[side][f];
			while (from != -1) {
				ff = 0;
				while (vars[ff] != -1) {
//					L0("P:%d, s:%d, v: %d=%d:%d\n", f, side, vars[ff], ps->t_sc[side][f][vars[ff]].sqr_b, ps->t_sc[side][f][vars[ff]].sqr_b);
					ps->score[side][vars[ff]].sqr_b += ps->t_sc[side][f][vars[ff]].sqr_b;
					ps->score[side][vars[ff]].sqr_e += ps->t_sc[side][f][vars[ff]].sqr_e;
					ff++;
				}
				from = ps->pawns[side][++f];
			}
		}

		assert(
			(ps->score[0][BAs].sqr_b < 100000)
				&& (ps->score[0][BAs].sqr_b > -100000));
		if ((b->hps != NULL)) {
			*hhh = storePawnHash(b->hps, hash, b->stats);
		}
	} else {
		*hhh = h2;
		ps = &(h2->value);
	}
	assert(
		(ps->score[0][BAs].sqr_b < 100000)
			&& (ps->score[0][BAs].sqr_b > -100000));
	return 0;
}

/*
 * Vygenerujeme vsechny co utoci na krale
 * vygenerujeme vsechny PINy - tedy ty kteri blokuji utok na krale
 * vygenerujeme vsechny RAYe utoku na krale
 */

int eval_king_checks_ext(board const *b, king_eval *ke, personality const *p, int side, int from)
{
	BITVAR cr2, di2, c2, d2, c, d, c3, d3, c2s, d2s;

	int ff, o;
	BITVAR epbmp;

	o = Flip(side);
	epbmp = (b->ep != -1) ? attack.ep_mask[b->ep] : 0;
	ke->ep_block = 0;

// find potential attackers - get rays, and check existence of them
	cr2 = di2 = 0;
// vert/horiz rays
	c = ke->cr_all_ray = attack.maps[ROOK][from];
// vert/horiz attackers
	c2 = c2s = c & (b->maps[ROOK] | b->maps[QUEEN]) & (b->colormaps[o]);
// diag rays
	d = ke->di_all_ray = attack.maps[BISHOP][from];
// diag attackers
	d2 = d2s = d & (b->maps[BISHOP] | b->maps[QUEEN]) & (b->colormaps[o]);

// if it can hit king, find nearest piece, blocker?
// rook/queen
	ke->cr_pins = ke->cr_attackers = ke->cr_att_ray = 0;

// iterate attackers
	while (c2) {
		ff = LastOne(c2);
// get line between square and attacker
		cr2 = attack.rays_int[from][ff];
// check if there is piece in that line, that blocks the attack
		c3 = cr2 & b->norm;
		if ((c3 & c2s) == 0) {
// determine status
			switch (BitCount(c3)) {
// just 1 means pin
			case 1:
				ke->cr_pins |= c3;
				break;
// 0 means attacked
			case 0:
				ke->cr_attackers |= normmark[ff];
				ke->cr_att_ray |= attack.rays_dir[ff][from];
				break;
			case 2:
// check ep pin, see below
				if (epbmp
					&& ((c3 & (epbmp | normmark[b->ep]) & b->maps[PAWN]) == c3))
					ke->ep_block = c3;
				break;
			default:
				break;
			}
		}
		ClrLO(c2);
	}

	/*
	 * check for ep pin situation - white king on 5th rank, white pawn on the same rank pinned with horizontal attack
	 * and black pawn moved two squares from 7th to 5th. In such case white pawn cannot do ep capture...
	 * pawn was pinned before doublepush, but now is not classified as such
	 */

// bishop/queen
	ke->di_pins = ke->di_attackers = ke->di_att_ray = 0;

	while (d2) {
		ff = LastOne(d2);
		di2 = attack.rays_int[from][ff];
		d3 = di2 & b->norm;
		if ((d3 & d2s) == 0) {
			switch (BitCount(d3)) {
			case 1:
				ke->di_pins |= d3;
				break;
			case 0:
				ke->di_attackers |= normmark[ff];
				ke->di_att_ray |= attack.rays_dir[ff][from];
				break;
			}
		}
		ClrLO(d2);
	}

// incorporate knights
	ke->kn_pot_att_pos = attack.maps[KNIGHT][from];
	ke->kn_attackers = ke->kn_pot_att_pos & b->maps[KNIGHT] & b->colormaps[o];
//incorporate pawns
	ke->pn_pot_att_pos = attack.pawn_att[side][from];
	ke->pn_attackers = ke->pn_pot_att_pos & b->maps[PAWN] & b->colormaps[o];
	ke->attackers = ke->cr_attackers | ke->di_attackers | ke->kn_attackers
		| ke->pn_attackers;

	return 0;
}

int eval_ind_attacks(const board *const b, king_eval *ke, personality *p, int side, int from)
{
	BITVAR cr2, di2, c2, d2, c, d, c3, d3, coo, doo, bl_ray;

	int ff, o;

	o = (side == 0) ? BLACK : WHITE;
//	epbmp= (b->ep!=-1) ? attack.ep_mask[b->ep] : 0;
	ke->ep_block = 0;

// find potential attackers - get rays, and check existence of them
	cr2 = di2 = 0;
// vert/horiz rays
	c = ke->cr_blocker_ray = ke->cr_all_ray = attack.maps[ROOK][from];
// vert/horiz blockers
	c2 = c
		& (((b->maps[BISHOP] | b->maps[KNIGHT] | b->maps[PAWN])
			& (b->colormaps[o])) | (b->norm & b->colormaps[side]));
// diag rays
	d = ke->di_blocker_ray = ke->di_all_ray = attack.maps[BISHOP][from];
// diag blockers
	d2 = d
		& (((b->maps[ROOK] | b->maps[KNIGHT] | b->maps[PAWN])
			& (b->colormaps[o])) | (b->norm & b->colormaps[side]));

	coo = c & (b->maps[BISHOP] | b->maps[KNIGHT] | b->maps[PAWN])
		& (b->colormaps[o]);
	doo = d & (b->maps[ROOK] | b->maps[KNIGHT] | b->maps[PAWN])
		& (b->colormaps[o]);

// rook/queen
	ke->cr_blocks = ke->cr_attackers = ke->cr_att_ray = 0;

// iterate endpoints
	while (c2) {
		ff = LastOne(c2);
// get line to endpoint
		cr2 = attack.rays_int[from][ff];
// check if there is piece in that line, that blocks the attack
		c3 = cr2 & b->norm;
		if (BitCount(c3) == 0) {
			ke->cr_blocks |= (c3 & coo);
			bl_ray = (attack.rays_dir[from][ff] ^ attack.rays[from][ff])
				^ FULLBITMAP;
			ke->cr_blocker_ray &= (bl_ray);
		}
		ClrLO(c2);
	}

// bishop/queen
	ke->di_blocks = ke->di_attackers = ke->di_att_ray = 0;

	while (d2) {
		ff = LastOne(d2);
		di2 = attack.rays_int[from][ff];
		d3 = di2 & b->norm;
		if (BitCount(d3) == 0) {
			ke->di_blocks |= (d3 & doo);
			bl_ray = (attack.rays_dir[from][ff] ^ attack.rays[from][ff])
				^ FULLBITMAP;
			ke->di_blocker_ray &= (bl_ray);
		}
		ClrLO(d2);
	}

// incorporate knights
	ke->kn_pot_att_pos = attack.maps[KNIGHT][from];
//incorporate pawns
	ke->pn_pot_att_pos = attack.pawn_att[side][from];

// blocker rays contain squares from king can be attacked by particular type of piece
// blocks contains opside pieces that might block opside attack and moving that away might cause attack

	return 0;
}

// eval king check builds PINS bitmap and attacker bitmap including attacks rays against king position
// ext version does it for any position
// oth version removes temporarily king before building bitmaps
// full version builds also blocker_rays - between blocker and position

int eval_king_checks(board const *b, king_eval *ke, personality const *p, int side)
{
	int from;
	from = b->king[side];
	eval_king_checks_ext(b, ke, p, side, from);
	return 0;
}

int eval_king_checks_oth(board *b, king_eval *ke, personality *p, int side, int from)
{
	int oldk;
	
	oldk = b->king[side];
// clear KING position	
	ClearAll(oldk, side, KING, b);
	eval_king_checks_ext(b, ke, p, side, from);
// restore old king position
	SetAll(oldk, side, KING, b);
	return 0;
}

// att_ray - mezi utocnikem az za krale, bez utocnika
// blocker_ray - mezi blockerem a kralem, vcetne blockera

int eval_king_checks_all(board *b, attack_model *a)
{
	eval_king_checks(b, &(a->ke[WHITE]), NULL, WHITE);
	eval_king_checks(b, &(a->ke[BLACK]), NULL, BLACK);
	return 0;
}

int is_draw(board *b, attack_model *a, personality *p)
{
	int ret, i, count;

	if ((b->mindex_validity == 1)
		&& (p->mat_info[b->mindex].info[b->side] <= 1)&&(p->mat_info[b->mindex].info[Flip(b->side)]==0))
		return 1;

	/*
	 * The fifty-move rule - if in the previous fifty moves by each side
	 * no pawn has moved and no capture has been made
	 * a draw may be claimed by either player.
	 * Here again, the draw is not automatic and must be claimed if the player wants the draw.
	 * If the player whose turn it is to move has made only 49 such moves,
	 * he may write his next move on the scoresheet and claim a draw.
	 * As with the threefold repetition, the right to claim the draw is forfeited
	 * if it is not used on that move, but the opportunity may occur again
	 */

	ret = 0;
	if ((b->move - b->rule50move) >= 101) {
		return 4;
	}
	count = 0;
	i = b->move;

	/*
	 * threefold repetition testing
	 * a position is considered identical to another if
	 * the same player is on move
	 * the same types of pieces of the same colors occupy the same squares
	 * the same moves are available to each player; in particular, each player has the same castling and en passant capturing rights.
	 */

// na i musi matchnout vzdy!
	while ((count < 3) && (i >= b->rule50move) && (i >= b->move_start)) {
		if (b->positions[i - b->move_start] == b->key) {
			DEB_3(if(b->posnorm[i-b->move_start]!=b->norm) printf("Error: Not matching position to hash!\n");)
			count++;
			if ((count == 2) && (i > b->move_ply_start)) {
				ret = 2;
			}
		}
		i -= 2;
	}
	if (count >= 3) {
		ret = 3;
	}
	return ret;
}

int meval_value_c(int pw, int pb, int nw, int nb, int bwl, int bwd, int bbl, int bbd, int rw, int rb, int qw, int qb, struct materi *t)
{
	t->m[WHITE][PAWN] = pw;
	t->m[WHITE][KNIGHT] = nw;
	t->m[WHITE][BISHOP] = bwl + bwd;
	t->m[WHITE][ROOK] = rw;
	t->m[WHITE][QUEEN] = qw;

	t->m[WHITE][LBISHOP] = bwl;
	t->m[WHITE][DBISHOP] = bwd;
	t->m[WHITE][LIGHT] = t->m[WHITE][KNIGHT] + t->m[WHITE][BISHOP];
	t->m[WHITE][HEAVY] = t->m[WHITE][ROOK] + t->m[WHITE][QUEEN];
	t->m[WHITE][PIECES] = t->m[WHITE][LIGHT] + t->m[WHITE][HEAVY];
	t->m[WHITE][TPIECES] = t->m[WHITE][PIECES] + t->m[WHITE][PAWN];

	t->m[BLACK][PAWN] = pb;
	t->m[BLACK][KNIGHT] = nb;
	t->m[BLACK][BISHOP] = bbl + bbd;
	t->m[BLACK][ROOK] = rb;
	t->m[BLACK][QUEEN] = qb;

	t->m[BLACK][LBISHOP] = bbl;
	t->m[BLACK][DBISHOP] = bbd;
	t->m[BLACK][LIGHT] = t->m[BLACK][KNIGHT] + t->m[BLACK][BISHOP];
	t->m[BLACK][HEAVY] = t->m[BLACK][ROOK] + t->m[BLACK][QUEEN];
	t->m[BLACK][PIECES] = t->m[BLACK][LIGHT] + t->m[BLACK][HEAVY];
	t->m[BLACK][TPIECES] = t->m[BLACK][PIECES] + t->m[BLACK][PAWN];
	return 1;
}

/*
 * DRAW: no pawns, num of pieces in total < 7, MAT(stronger side)-MAT(weaker side) < MAT(bishop) 
 * scaling when leading side has less than 2 pawns
 */
int mat_setup(int p[2], int n[2], int bl[2], int bd[2], int r[2], int q[2], struct materi *t)
{
	int values[] = { 1000, 3500, 3500, 5000, 9750, 0 };
	int i, op;
	int mp;
	int b[2], nn, bb, rr, qq, pp;
	int mt[2], m2[2];

	uint8_t tun[2];
	
	meval_value_c(p[0], p[1], n[0], n[1], bl[0], bd[0], bl[1], bd[1], r[0],
		r[1], q[0], q[1], t);
	for (i = 0; i < 2; i++) {
		b[i] = bl[i] + bd[i];
		m2[i] = n[i] * values[1] + b[i] * values[2] + r[i] * values[3]
			+ q[i] * values[4];
		mt[i] = p[i] * values[0] + m2[i];
	}
	tun[0] = tun[1] = 128;

	for (i = 0; i <= 1; i++) {
		op = i == 0 ? 1 : 0;

// material wise I'm ahead
		pp = t->m[i][PAWN];
		if ((mt[i] >= mt[op]) && (pp < 2)) {
			if (pp == 1) {
				if (t->m[op][PIECES] > 0) {
					nn = t->m[op][KNIGHT];
					bb = t->m[op][BISHOP];
					rr = t->m[op][ROOK];
					qq = t->m[op][QUEEN];
					if (nn > 0) {
						nn--;
						pp--;
					} else if (bb > 0) {
						bb--;
						pp--;
					} else if (rr > 0) {
						rr--;
						pp--;
					} else if (qq > 0) {
						qq--;
						pp--;
					}
					m2[op] = nn * values[1] + bb * values[2] + rr * values[3]
						+ qq * values[4];
				}
			}
			mp = 1;
			if (t->m[i][PIECES] <= 2) {
				if (t->m[i][KNIGHT] >= 2)
					mp = 0;
				if ((m2[i] - m2[op]) <= values[2])
					mp = 0;
			}
			if (!mp) {
				tun[i] = t->m[i][PAWN] == 0 ? 8 : 16;
			} else {
				if ((t->m[i][PAWN] == 0) && (t->m[op][PIECES] > 0))
					tun[i] = 64;
			}
		}
	}
	t->info[0] = tun[0];
	t->info[1] = tun[1];
	return 0;
}


int mat_setup2(int pw, int pb, int nw, int nb, int bwl, int bwd, int bbl, int bbd, int rw, int rb, int qw, int qb, struct materi *tt)
{
	int v[] = { 100, 325, 325, 500, 975, 0 };
	uint8_t imul[] = { 0, 4, 8, 16, 64, 128 };
	int i, op;
	int mp, pd;
	int nn, bb, rr, qq, pp;
	int mvtot[2], mv[2];
	struct materi t2, *t;
	t=&t2;

	uint8_t tun[2];
	
	meval_value_c(pw, pb, nw, nb, bwl, bwd, bbl, bbd, rw, rb, qw, qb, tt);
	for (i = 0; i < 2; i++) {
		mv[i] = tt->m[i][KNIGHT]*v[KNIGHT] + tt->m[i][BISHOP]*v[BISHOP]
		 + tt->m[i][ROOK]*v[ROOK] + tt->m[i][QUEEN]*v[QUEEN];
		mvtot[i] = tt->m[i][PAWN]*v[PAWN] + mv[i];
	}
//	L0("Org: %d-%d %d-%d %d-%d %d-%d %d-%d %d-%d=%d:%d\n",qw, qb, rw, rb, bwl, bwd, bbl, bbd, nw, nb, pw, pb, tt->info[0], tt->info[1]);
	tun[0] = tun[1] = 128;

	pd=0;
	for (i = 0; i <= 1; i++) {
		op = i == 0 ? 1 : 0;

// scaling is triggered when side ahead in material
		if (((mvtot[i] - mvtot[op])>=0)&&(tt->m[i][PAWN]<=1)) {
			mp=4;
			pd++;
			meval_value_c(pw, pb, nw, nb, bwl, bwd, bbl, bbd, rw, rb, qw, qb, t);
// discount last pawn of i
				if ((t->m[i][PAWN] == 1)&&(t->m[op][PIECES] > 0)) {
					if(t->m[op][KNIGHT]) t->m[op][KNIGHT]--;
					if(t->m[op][LBISHOP]) t->m[op][LBISHOP]--;
					if(t->m[op][DBISHOP]) t->m[op][DBISHOP]--;
					if(t->m[op][ROOK]) t->m[op][ROOK]--;

					t->m[op][BISHOP] = t->m[op][LBISHOP] + t->m[op][DBISHOP];
					t->m[op][LIGHT] = t->m[op][KNIGHT] + t->m[op][BISHOP];
					t->m[op][HEAVY] = t->m[op][ROOK] + t->m[op][QUEEN];
					t->m[op][PIECES] = t->m[op][LIGHT] + t->m[op][HEAVY];
					t->m[op][TPIECES] = t->m[op][PIECES] + t->m[op][PAWN];
					
					if(t->m[op][PIECES]<tt->m[op][PIECES]) t->m[i][PAWN]--;
					mv[op] = t->m[op][KNIGHT]*v[KNIGHT] + t->m[op][BISHOP]*v[BISHOP]
					 + t->m[op][ROOK]*v[ROOK] + t->m[op][QUEEN]*v[QUEEN];
					mvtot[i] = mv[i];
					mvtot[op] = mv[op];
				}

				if(t->m[i][PAWN] == 0){
					if(t->m[i][HEAVY]==0 && t->m[i][LIGHT]<=3 && t->m[i][LIGHT]>=1) {
						pd++;
						if(t->m[i][LIGHT]==3) {
							mp=1;
							if(t->m[op][PIECES]<=2) {
								if(t->m[op][HEAVY]==1 && t->m[op][PIECES]==1) 
									mp= (t->m[i][BISHOP]<2 || t->m[i][PIECES]>=3) ? 1 : 3;
								else if(t->m[op][LIGHT]<=1 && t->m[op][PIECES]==t->m[op][LIGHT]) mp= t->m[op][PIECES] ? 3:4;
								else if(t->m[op][LIGHT]==2 && t->m[op][PIECES]==2) mp= (t->m[op][KNIGHT]>=1 && t->m[i][BISHOP]==2) ? 4 : 1;
							}
						} else if(t->m[i][LIGHT]==2) {
							mp=1;
							if(t->m[op][PIECES]==1) {
								if(t->m[op][LIGHT]==1 && t->m[i][BISHOP]>0) mp= t->m[i][KNIGHT]>0 ? 2 : t->m[op][KNIGHT]>0 ? 3 : 1;
							} else {
								if(t->m[op][PIECES]==0) mp= (t->m[i][KNIGHT]<2) ? 4: t->m[op][PAWN]>0 ? 2 : 0;
							}
						} else {
						if(t->m[i][LIGHT]==1) mp=0;
						}
					} else if(t->m[i][HEAVY]>=1 && t->m[i][HEAVY]<=3) {
						pd++;
						mp=4;
						if(!t->m[op][PIECES]) mp=5;
						else if(t->m[i][PIECES]==3) {
							if(t->m[i][HEAVY]==1 && t->m[op][HEAVY]==1 ) {
								if(t->m[op][LIGHT]>=0 && t->m[i][ROOK]==t->m[op][ROOK])
									mp= (t->m[i][ROOK]||t->m[op][LIGHT]==0) ? 4 : (t->m[op][KNIGHT] && t->m[i][BISHOP]>1) ? 3:1;
								else if(t->m[i][ROOK]) mp= t->m[op][QUEEN] ? 1 : t->m[op][KNIGHT]>=2 ? 3 : 4;
							} else if(t->m[i][ROOK]==2 && t->m[i][LIGHT] && t->m[op][HEAVY]>=1) {
								if(t->m[op][QUEEN]==1 && t->m[op][HEAVY]==1 && t->m[op][PIECES]==1) mp=3;
								else if(t->m[op][ROOK]==2 && t->m[op][HEAVY]==2 && t->m[op][PIECES]==2) 
									mp= t->m[i][BISHOP]>=1 ? 4 : 1;
							}
						} else if(t->m[i][PIECES]<=2) {
							if(t->m[i][HEAVY]==1 && t->m[i][LIGHT]==1) {
								if(t->m[i][ROOK]) {
								  if(t->m[op][ROOK]) { if(t->m[op][PIECES]<=2) mp= (t->m[i][KNIGHT]||t->m[op][PIECES]==2) ? 1 : 2; }
								  else if(t->m[op][PIECES]==2 && t->m[op][LIGHT]==2) {
									if(t->m[op][BISHOP]==2) mp=1;
									else if(t->m[i][KNIGHT]>=1) mp=2;
									else mp= t->m[i][KNIGHT]>1 ? 3 : t->m[i][LBISHOP]==t->m[op][LBISHOP] ? 1 : 3;
								  }
								} else if(t->m[i][QUEEN]) {
									if(t->m[op][PIECES]==1 && t->m[op][QUEEN]) { mp= t->m[i][BISHOP] ? 1 : 3;
									}
									else if(t->m[op][PIECES]==2 && t->m[op][ROOK]==2) mp= t->m[i][BISHOP] ? 3 : 1;
									else if(t->m[op][KNIGHT]==1 && t->m[op][ROOK]==1 && t->m[op][PIECES]==2) mp= t->m[i][BISHOP] ? 2 : 1;
									else if(t->m[op][LIGHT]>=1 && t->m[op][ROOK]==1) {
									  if(t->m[op][PIECES]==2 && t->m[op][BISHOP]==1) mp=4;
									  else mp= (t->m[i][KNIGHT] && t->m[op][KNIGHT]==1 && t->m[op][BISHOP]==1) ? 3 : 1;
									}
								  }
								} else if(t->m[i][HEAVY]<=2) {
									if(mv[i]-mv[op] > 400) {
										mp= t->m[op][PIECES] ? 4 : 5;
									}
									else if(mv[i]-mv[op] == 350) mp=3;
									else if(mv[i]-mv[op] == 325) mp= t->m[op][KNIGHT]==2 ? 1 : t->m[op][BISHOP]==2 ? 3 : 4;
									else if(mv[i]-mv[op] < 325) mp= t->m[op][QUEEN] ? 2 : 1;
								}
							}
						} else { mp= mv[i]>0 ? 4 : 0; pd++; }
				}
			if(t->m[i][PAWN]!=tt->m[i][PAWN]) mp=Max(3, mp+1);
			mp=Min(5, mp);
			tun[i]=imul[mp];
			if(tun[i]==0) tun[op]=0;
		} else {
			if(tt->m[i][TPIECES]==0 || (tt->m[i][TPIECES]==1 && tt->m[i][LIGHT]==1)) tun[i]=imul[0];
			else if((tt->m[i][TPIECES]==2 && tt->m[i][KNIGHT]==2)) tun[i]=imul[1];
		}
	}
//	L0("---: %d-%d %d-%d %d-%d %d-%d %d-%d=\t%d:%d\n",qw, qb, rw, rb, bwl+bwd, bbl+bbd, nw, nb, pw, pb, tt->info[0], tt->info[1]);
	tt->info[0] = tun[0];
	tt->info[1] = tun[1];
//	if(pd) L0("NNN: %d-%d %d-%d %d-%d %d-%d %d-%d=\t%d:%d\n",qw, qb, rw, rb, bwl+bwd, bbl+bbd, nw, nb, pw, pb, tt->info[0], tt->info[1]);
	return 0;
}

int mat_info(struct materi *info)
{
	int f;
	for (f = 0; f < 419999; f++) {
		info[f].info[0] = info[f].info[1] = 128;
	}
// certain values known draw

	int m;
	int p[2], n[2], bl[2], bd[2], r[2], q[2];

	for (p[1] = 0; p[1] < 9; p[1]++) {
		for (p[0] = 0; p[0] < 9; p[0]++) {
			for (r[1] = 0; r[1] < 3; r[1]++) {
				for (r[0] = 0; r[0] < 3; r[0]++) {
					for (bd[1] = 0; bd[1] < 2; bd[1]++) {
						for (bl[1] = 0; bl[1] < 2; bl[1]++) {
							for (bd[0] = 0; bd[0] < 2; bd[0]++) {
								for (bl[0] = 0; bl[0] < 2; bl[0]++) {
									for (n[1] = 0; n[1] < 3; n[1]++) {
										for (n[0] = 0; n[0] < 3; n[0]++) {
											for (q[1] = 0; q[1] < 2; q[1]++) {
												for (q[0] = 0; q[0] < 2;
														q[0]++) {
													m = MATidx(p[0], p[1], n[0],
														n[1], bl[0], bd[0],
														bl[1], bd[1], r[0],
														r[1], q[0], q[1]);
													mat_setup(p, n, bl, bd, r,
														q, &(info[m]));
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
	return 0;
}

int mat_faze(uint8_t *faze)
{
	int pw, pb, nw, nb, bwl, bwd, bbl, bbd, rw, rb, qw, qb, f;
	int i, tot, m;
	int fz, q;
	int vaha[] = { 0, 6, 6, 9, 18 };
	int nc[] = { 16, 4, 4, 4, 2 };
	for (f = 0; f < 419999; f++) {
		faze[f] = 0;
	}
	tot = nc[PAWN] * vaha[PAWN] + nc[KNIGHT] * vaha[KNIGHT]
		+ nc[BISHOP] * vaha[BISHOP] + nc[ROOK] * vaha[ROOK]
		+ nc[QUEEN] * vaha[QUEEN];
	for (qb = 0; qb < 2; qb++) {
		for (qw = 0; qw < 2; qw++) {
			for (rb = 0; rb < 3; rb++) {
				for (rw = 0; rw < 3; rw++) {
					for (bbd = 0; bbd < 2; bbd++) {
						for (bbl = 0; bbl < 2; bbl++) {
							for (bwd = 0; bwd < 2; bwd++) {
								for (bwl = 0; bwl < 2; bwl++) {
									for (nb = 0; nb < 3; nb++) {
										for (nw = 0; nw < 3; nw++) {
											for (pb = 0; pb < 9; pb++) {
												for (pw = 0; pw < 9; pw++) {
													m = MATidx(pw, pb, nw, nb,
														bwl, bwd, bbl, bbd, rw,
														rb, qw, qb);
													i = (pb + pw) * vaha[PAWN];
													i += (nw + nb)
														* vaha[KNIGHT];
													i += (bbd + bbl + bwd + bwl)
														* vaha[BISHOP];
													i += (rw + rb) * vaha[ROOK];
													i += (qw + qb)
														* vaha[QUEEN];
													q = Min(i, tot);
													fz = q * 255 / tot;
													assert(faze[m] == 0);
													faze[m] = (uint8_t) fz
														& 255;
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}

	return 0;
}

int meval_value(int pw, int pb, int nw, int nb, int bwl, int bwd, int bbl, int bbd, int rw, int rb, int qw, int qb, meval_t *t, personality const *p, int stage)
{
	int w, b, pp, scw, scb;
	w = pw * p->Values[stage][0] + nw * p->Values[stage][1]
		+ (bwl + bwd) * p->Values[stage][2] + rw * p->Values[stage][3]
		+ qw * p->Values[stage][4];
	b = pb * p->Values[stage][0] + nb * p->Values[stage][1]
		+ (bbl + bbd) * p->Values[stage][2] + rb * p->Values[stage][3]
		+ qb * p->Values[stage][4];

	pp = pw + pb;
	t->mat = (w - b);
	t->mat_w = w;

	t->mat_o[WHITE] = 4 * pw + 12 * nw + 12 * (bwl + bwd) + 20 * rw + 39 * qw;
	t->mat_o[BLACK] = 4 * pb + 12 * nb + 12 * (bbl + bbd) + 20 * rb + 39 * qb;

#if 1
	scw = p->dvalues[ROOK][pp] * rw + p->dvalues[KNIGHT][pp] * nw
		+ p->dvalues[QUEEN][pp] * qw + p->dvalues[BISHOP][pp] * bwl;
	scb = p->dvalues[ROOK][pp] * rb + p->dvalues[KNIGHT][pp] * nb
		+ p->dvalues[QUEEN][pp] * qb + p->dvalues[BISHOP][pp] * bbl;
	
	t->mat += (scw - scb);
	t->mat_w += scw;
	
#endif

	return 0;
}

int meval_table_gen(meval_t *t, personality *p, int stage)
{
	int pw, pb, nw, nb, bwl, bwd, bbl, bbd, rw, rb, qw, qb, f;
	int m;

	MATIdxIncW[PAWN] = PW_MI;
	MATIdxIncW[KNIGHT] = NW_MI;
	MATIdxIncW[BISHOP] = BWL_MI;
	MATIdxIncW[DBISHOP] = BWD_MI;
	MATIdxIncW[ROOK] = RW_MI;
	MATIdxIncW[QUEEN] = QW_MI;

	MATIdxIncB[PAWN] = PB_MI;
	MATIdxIncB[KNIGHT] = NB_MI;
	MATIdxIncB[BISHOP] = BBL_MI;
	MATIdxIncB[DBISHOP] = BBD_MI;
	MATIdxIncB[ROOK] = RB_MI;
	MATIdxIncB[QUEEN] = QB_MI;

	MATincW2[PAWN] = PW_MI2;
	MATincB2[PAWN] = PB_MI2;
	MATincW2[KNIGHT] = NW_MI2;
	MATincB2[KNIGHT] = NB_MI2;
	MATincW2[ROOK] = RW_MI2;
	MATincB2[ROOK] = RB_MI2;
	MATincW2[QUEEN] = QW_MI2;
	MATincB2[QUEEN] = QB_MI2;
	MATincW2[BISHOP] = BWL_MI2;
	MATincB2[BISHOP] = BBL_MI2;
	MATincW2[DBISHOP] = BWD_MI2;
	MATincB2[DBISHOP] = BBD_MI2;
	
// clear
	for (f = 0; f < 419999; f++) {
		t[f].mat = 0;
		t[f].mat_w = 0;
		t[f].mat_o[WHITE] = 0;
		t[f].mat_o[BLACK] = 0;
	}
	for (qb = 0; qb < 2; qb++) {
		for (qw = 0; qw < 2; qw++) {
			for (rb = 0; rb < 3; rb++) {
				for (rw = 0; rw < 3; rw++) {
					for (bbd = 0; bbd < 2; bbd++) {
						for (bbl = 0; bbl < 2; bbl++) {
							for (bwd = 0; bwd < 2; bwd++) {
								for (bwl = 0; bwl < 2; bwl++) {
									for (nb = 0; nb < 3; nb++) {
										for (nw = 0; nw < 3; nw++) {
											for (pb = 0; pb < 9; pb++) {
												for (pw = 0; pw < 9; pw++) {
													m = MATidx(pw, pb, nw, nb,
														bwl, bwd, bbl, bbd, rw,
														rb, qw, qb);
													meval_value(pw, pb, nw, nb,
														bwl, bwd, bbl, bbd, rw,
														rb, qw, qb, t + m, p,
														stage);
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
	return 0;
}


int meval_t_gen(personality *p)
{
	int pw, pb, nw, nb, bwl, bwd, bbl, bbd, rw, rb, qw, qb, f;
	int m;
	meval_t *tm, *te;
	uint8_t *fz;
	struct materi *info;

	info=p->mat_info;
	tm=p->mat;
	te=p->mate_e;
	fz=p->mat_faze;

	
	MATIdxIncW[PAWN] = PW_MI;
	MATIdxIncW[KNIGHT] = NW_MI;
	MATIdxIncW[BISHOP] = BWL_MI;
	MATIdxIncW[DBISHOP] = BWD_MI;
	MATIdxIncW[ROOK] = RW_MI;
	MATIdxIncW[QUEEN] = QW_MI;

	MATIdxIncB[PAWN] = PB_MI;
	MATIdxIncB[KNIGHT] = NB_MI;
	MATIdxIncB[BISHOP] = BBL_MI;
	MATIdxIncB[DBISHOP] = BBD_MI;
	MATIdxIncB[ROOK] = RB_MI;
	MATIdxIncB[QUEEN] = QB_MI;

	MATincW2[PAWN] = PW_MI2;
	MATincB2[PAWN] = PB_MI2;
	MATincW2[KNIGHT] = NW_MI2;
	MATincB2[KNIGHT] = NB_MI2;
	MATincW2[ROOK] = RW_MI2;
	MATincB2[ROOK] = RB_MI2;
	MATincW2[QUEEN] = QW_MI2;
	MATincB2[QUEEN] = QB_MI2;
	MATincW2[BISHOP] = BWL_MI2;
	MATincB2[BISHOP] = BBL_MI2;
	MATincW2[DBISHOP] = BWD_MI2;
	MATincB2[DBISHOP] = BBD_MI2;
	
// clear
	for (f = 0; f < 419999; f++) {
		tm[f].mat = 0;
		tm[f].mat_w = 0;
		tm[f].mat_o[WHITE] = 0;
		tm[f].mat_o[BLACK] = 0;
		te[f].mat = 0;
		te[f].mat_w = 0;
		te[f].mat_o[WHITE] = 0;
		te[f].mat_o[BLACK] = 0;
		fz[f] = 0;
		info[f].info[0] = info[f].info[1] = 128;
	}
	for (pb = 0; pb < 9; pb++) {
		for (pw = 0; pw < 9; pw++) {
			for (qb = 0; qb < 2; qb++) {
				for (qw = 0; qw < 2; qw++) {
					for (rb = 0; rb < 3; rb++) {
						for (rw = 0; rw < 3; rw++) {
							for (bbd = 0; bbd < 2; bbd++) {
								for (bbl = 0; bbl < 2; bbl++) {
									for (bwd = 0; bwd < 2; bwd++) {
										for (bwl = 0; bwl < 2; bwl++) {
											for (nb = 0; nb < 3; nb++) {
												for (nw = 0; nw < 3; nw++) {
													m = MATidx(pw, pb, nw, nb,
														bwl, bwd, bbl, bbd, rw,
														rb, qw, qb);
													meval_value(pw, pb, nw, nb,
														bwl, bwd, bbl, bbd, rw,
														rb, qw, qb, tm + m, p,
														0);
													meval_value(pw, pb, nw, nb,
														bwl, bwd, bbl, bbd, rw,
														rb, qw, qb, te + m, p,
														1);
													fz[m]=t_phase(pw+pb, nw+nb, bwd+bwl+bbd+bbl, rw+rb, qw+qb);
													mat_setup2(pw, pb, nw, nb, bwl, bwd, bbl, bbd, rw, rb, qw, qb, info+m);
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
	return 0;
}


/*
	meval_table_gen(p->mat, p, 0);
	meval_table_gen(p->mate_e, p, 1);
	mat_info(p->mat_info);
	mat_faze(p->mat_faze);
	MVVLVA_gen((p->LVAcap), p->Values);
*/


int get_material_eval(board const *b, personality const *p, int *mb, int *me, int *wb, int *we, stacker *st)
{
	int stage;
	int pw, pb, nw, nb, bwl, bwd, bbl, bbd, rw, rb, qw, qb;
	int pp, scb, scw;

	meval_t t;
	if (b->mindex_validity == 1) {
		*mb = p->mat[b->mindex].mat;
		*me = p->mate_e[b->mindex].mat;
		*wb = p->mat[b->mindex].mat_w;
		*we = p->mate_e[b->mindex].mat_w;
	} else {
		collect_material_from_board(b, &pw, &pb, &nw, &nb, &bwl, &bwd, &bbl,
			&bbd, &rw, &rb, &qw, &qb);

		stage = 0;
		meval_value(pw, pb, nw, nb, bwl, bwd, bbl, bbd, rw, rb, qw, qb, &t, p,
			stage);
		*mb = t.mat;
		*wb = t.mat_w;

		stage = 1;
		meval_value(pw, pb, nw, nb, bwl, bwd, bbl, bbd, rw, rb, qw, qb, &t, p,
			stage);
		*me = t.mat;
		*we = t.mat_w;
		
		pp = pw + pb;
		scw = p->dvalues[ROOK][pp] * rw + p->dvalues[KNIGHT][pp] * nw
			+ p->dvalues[QUEEN][pp] * qw + p->dvalues[BISHOP][pp] * (bwl + bwd);
		scb = p->dvalues[ROOK][pp] * rb + p->dvalues[KNIGHT][pp] * nb
			+ p->dvalues[QUEEN][pp] * qb + p->dvalues[BISHOP][pp] * (bbl + bbd);
		(*mb) += (scw - scb);
		(*me) += (scw - scb);
		(*wb) += (scw);
		(*we) += (scw);

#ifdef TUNING
		if(rw) { ADD_STACKER(st, dvalues[ROOK][pp], rw, BAs, WHITE); 
			ADD_STACKER(st, Values[MG][ROOK], rw, BAs, WHITE)
			ADD_STACKER(st, Values[EG][ROOK], rw, BAs, WHITE)
			}
		if(qw) { ADD_STACKER(st, dvalues[QUEEN][pp], qw, BAs, WHITE); 
			ADD_STACKER(st, Values[MG][QUEEN], qw, BAs, WHITE)
			ADD_STACKER(st, Values[EG][QUEEN], qw, BAs, WHITE)
			}
		if(nw) { ADD_STACKER(st, dvalues[KNIGHT][pp], nw, BAs, WHITE); 
			ADD_STACKER(st, Values[MG][KNIGHT], nw, BAs, WHITE)
			ADD_STACKER(st, Values[EG][KNIGHT], nw, BAs, WHITE)
			}
		if(bwl+bwd) { ADD_STACKER(st, dvalues[BISHOP][pp], bwl+bwd, BAs, WHITE); 
			ADD_STACKER(st, Values[MG][BISHOP], bwl+bwd, BAs, WHITE)
			ADD_STACKER(st, Values[EG][BISHOP], bwl+bwd, BAs, WHITE)
			}
		if(pw) {
			ADD_STACKER(st, Values[MG][PAWN], pw, BAs, WHITE)
			ADD_STACKER(st, Values[EG][PAWN], pw, BAs, WHITE)
			}
		if(rb) { ADD_STACKER(st, dvalues[ROOK][pp], rb, BAs, BLACK); 
			ADD_STACKER(st, Values[MG][ROOK], rb, BAs, BLACK)
			ADD_STACKER(st, Values[EG][ROOK], rb, BAs, BLACK)
			}
		if(qb) { ADD_STACKER(st, dvalues[QUEEN][pp], qb, BAs, BLACK);
			ADD_STACKER(st, Values[MG][QUEEN], qb, BAs, BLACK)
			ADD_STACKER(st, Values[EG][QUEEN], qb, BAs, BLACK)
			}
		if(nb) { ADD_STACKER(st, dvalues[KNIGHT][pp], nb, BAs, BLACK); 
			ADD_STACKER(st, Values[MG][KNIGHT], nb, BAs, BLACK)
			ADD_STACKER(st, Values[EG][KNIGHT], nb, BAs, BLACK)
			}
		if(bbl+bbd) { ADD_STACKER(st, dvalues[BISHOP][pp], bbl+bbd, BAs, BLACK); 
			ADD_STACKER(st, Values[MG][BISHOP], bbl+bbd, BAs, BLACK)
			ADD_STACKER(st, Values[EG][BISHOP], bbl+bbd, BAs, BLACK)
			}
		if(pb) {
			ADD_STACKER(st, Values[MG][PAWN], pb, BAs, BLACK)
			ADD_STACKER(st, Values[EG][PAWN], pb, BAs, BLACK)
			}
#endif
	}
	return 2;
}

int get_material_eval_f(board *b, personality *p)
{
	int score;
	int me, mb, we, wb;
	int phase = eval_phase(b, p);
	stacker st;

	get_material_eval(b, p, &mb, &me, &wb, &we, &st);
	score = (mb * phase + me * (255 - phase)) / 255;
	return score;
}

int eval_bishop(board const *b, attack_model *a, PawnStore const *ps, int side, personality const *p, stacker *st)
{
	int piece;
	int from;
	int f;

	piece = (side == WHITE) ? BISHOP : BISHOP | BLACKPIECE;
	for (f = a->pos_c[piece]; f >= 0; f--) {
		from = a->pos_m[piece][f];
		a->sc.side[side].mobi_b += a->me[from].pos_mob_tot_b;
		a->sc.side[side].mobi_e += a->me[from].pos_mob_tot_e;
		a->sq[from].sqr_b = p->piecetosquare[MG][side][BISHOP][from];
		a->sq[from].sqr_e = p->piecetosquare[EG][side][BISHOP][from];
		a->scc[from].sqr_b=0;
		a->scc[from].sqr_e=0;
		a->sc.side[side].sqr_b += a->sq[from].sqr_b;
		a->sc.side[side].sqr_e += a->sq[from].sqr_e;
#ifdef TUNING
		if(side==WHITE) {
		ADD_STACKER(st, piecetosquare[MG][side][BISHOP][from], 1, BAs, side)
		ADD_STACKER(st, piecetosquare[EG][side][BISHOP][from], 1, BAs, side)
		} else
		{
		ADD_STACKER(st, piecetosquare[MG][side][BISHOP][from], 1, BAs, side)
		ADD_STACKER(st, piecetosquare[EG][side][BISHOP][from], 1, BAs, side)
		}
		
#endif
	}
	return 0;
}

int eval_knight(board const *b, attack_model *a, PawnStore const *ps, int side, personality const *p, stacker *st)
{
	int piece;
	int from;
	int f;

	piece = (side == WHITE) ? KNIGHT : KNIGHT | BLACKPIECE;
	for (f = a->pos_c[piece]; f >= 0; f--) {
		from = a->pos_m[piece][f];
		a->sc.side[side].mobi_b += a->me[from].pos_mob_tot_b;
		a->sc.side[side].mobi_e += a->me[from].pos_mob_tot_e;
		a->sq[from].sqr_b = p->piecetosquare[MG][side][KNIGHT][from];
		a->sq[from].sqr_e = p->piecetosquare[EG][side][KNIGHT][from];
		a->scc[from].sqr_b=0;
		a->scc[from].sqr_e=0;
		a->sc.side[side].sqr_b += a->sq[from].sqr_b;
		a->sc.side[side].sqr_e += a->sq[from].sqr_e;
#ifdef TUNING
		if(side==WHITE) {
		ADD_STACKER(st, piecetosquare[MG][side][KNIGHT][from], 1, BAs, side)
		ADD_STACKER(st, piecetosquare[EG][side][KNIGHT][from], 1, BAs, side)
		} else
		{
		ADD_STACKER(st, piecetosquare[MG][side][KNIGHT][from], 1, BAs, side)
		ADD_STACKER(st, piecetosquare[EG][side][KNIGHT][from], 1, BAs, side)
		}
		
#endif
	}
	return 0;
}

int eval_queen(board const *b, attack_model *a, PawnStore const *ps, int side, personality const *p, stacker *st)
{
	int piece;
	int from;
	int f;

	piece = (side == WHITE) ? QUEEN : QUEEN | BLACKPIECE;
	for (f = a->pos_c[piece]; f >= 0; f--) {
		from = a->pos_m[piece][f];
		a->sc.side[side].mobi_b += a->me[from].pos_mob_tot_b;
		a->sc.side[side].mobi_e += a->me[from].pos_mob_tot_e;
		a->sq[from].sqr_b = p->piecetosquare[MG][side][QUEEN][from];
		a->sq[from].sqr_e = p->piecetosquare[EG][side][QUEEN][from];
		a->scc[from].sqr_b=0;
		a->scc[from].sqr_e=0;
		a->sc.side[side].sqr_b += a->sq[from].sqr_b;
		a->sc.side[side].sqr_e += a->sq[from].sqr_e;
#ifdef TUNING
		if(side==WHITE) {
		ADD_STACKER(st, piecetosquare[MG][side][QUEEN][from], 1, BAs, side)
		ADD_STACKER(st, piecetosquare[EG][side][QUEEN][from], 1, BAs, side)
		} else
		{
		ADD_STACKER(st, piecetosquare[MG][side][QUEEN][from], 1, BAs, side)
		ADD_STACKER(st, piecetosquare[EG][side][QUEEN][from], 1, BAs, side)
		}
		
#endif
	}
	return 0;
}

int eval_rook(board const *b, attack_model *a, PawnStore const *ps, int side, personality const *p, stacker *st)
{
	int piece;
	int from;
	int srank;
	int z, f;
	int opside;
	BITVAR n;

	if (side == WHITE) {
		piece = ROOK;
		srank = 6;
		opside = BLACK;
	} else {
		piece = ROOK | BLACKPIECE;
		srank = 1;
		opside = WHITE;
	}
	for (f = a->pos_c[piece]; f >= 0; f--) {
		from = a->pos_m[piece][f];
		a->sc.side[side].mobi_b += a->me[from].pos_mob_tot_b;
		a->sc.side[side].mobi_e += a->me[from].pos_mob_tot_e;
		a->sq[from].sqr_b = p->piecetosquare[MG][side][ROOK][from];
		a->sq[from].sqr_e = p->piecetosquare[EG][side][ROOK][from];
		a->scc[from].sqr_b=0;
		a->scc[from].sqr_e=0;
		a->sc.side[side].sqr_b += a->sq[from].sqr_b;
		a->sc.side[side].sqr_e += a->sq[from].sqr_e;
//		LOGGER_0("PSQ sq:%o, piece:%d, side:%d, val:%d:%d\n", from, ROOK, side, p->piecetosquare[MG][side][ROOK][from], p->piecetosquare[EG][side][ROOK][from]);
#ifdef TUNING
		if(side==WHITE) {
		ADD_STACKER(st, piecetosquare[MG][side][ROOK][from], 1, BAs, side)
		ADD_STACKER(st, piecetosquare[EG][side][ROOK][from], 1, BAs, side)
		} else
		{
		ADD_STACKER(st, piecetosquare[MG][side][ROOK][from], 1, BAs, side)
		ADD_STACKER(st, piecetosquare[EG][side][ROOK][from], 1, BAs, side)
		}
		
#endif

		z = getRank(from);
		if (z == srank) {
			a->specs[side][ROOK].sqr_b += p->rook_on_seventh[MG];
			a->specs[side][ROOK].sqr_e += p->rook_on_seventh[EG];
			a->scc[from].sqr_b += p->rook_on_seventh[MG];
			a->scc[from].sqr_e += p->rook_on_seventh[EG];

#ifdef TUNING
		ADD_STACKER(st, rook_on_seventh[MG], 1, BAs, side)
		ADD_STACKER(st, rook_on_seventh[EG], 1, BAs, side)
#endif
		}

		n = attack.file[from];
		if (n & ps->not_pawns_file[side] & ps->not_pawns_file[opside]) {
			a->specs[side][ROOK].sqr_b += p->rook_on_open[MG];
			a->specs[side][ROOK].sqr_e += p->rook_on_open[EG];
			a->scc[from].sqr_b += p->rook_on_open[MG];
			a->scc[from].sqr_e += p->rook_on_open[EG];
#ifdef TUNING
		ADD_STACKER(st, rook_on_open[MG], 1, BAs, side)
		ADD_STACKER(st, rook_on_open[EG], 1, BAs, side)
#endif
		} else if (n & ps->not_pawns_file[side]
			& (~ps->not_pawns_file[opside])) {
			a->specs[side][ROOK].sqr_b += p->rook_on_semiopen[MG];
			a->specs[side][ROOK].sqr_e += p->rook_on_semiopen[EG];
			a->scc[from].sqr_b += p->rook_on_semiopen[MG];
			a->scc[from].sqr_e += p->rook_on_semiopen[EG];
#ifdef TUNING
			ADD_STACKER(st, rook_on_semiopen[MG], 1, BAs, side)
			ADD_STACKER(st, rook_on_semiopen[EG], 1, BAs, side)
#endif
		}
		a->sc.side[side].specs_b += a->specs[side][ROOK].sqr_b;
		a->sc.side[side].specs_e += a->specs[side][ROOK].sqr_e;
	}
	return 0;
}

int eval_pawn(board const *b, attack_model *a, PawnStore const *ps, int side, personality const *p, stacker *st)
{
	int heavy_op;

/*
 * add stuff related to other pieces esp heavy opp pieces
 * at present pawn model depends on availability opponents heavy pieces
 */

#ifdef TUNING
		st->heavy[side]=0;
		st->variant[side]=BAs;
#endif


	a->sc.side[side].sqr_b += ps->score[side][BAs].sqr_b;
	a->sc.side[side].sqr_e += ps->score[side][BAs].sqr_e;

	heavy_op = ((((b->maps[ROOK] | b->maps[QUEEN]) & b->colormaps[Flip(side)])!=0)
		&& (p->use_heavy_material!=0));

	if (heavy_op) {
		a->sc.side[side].sqr_b += ps->score[side][HEa].sqr_b;
		a->sc.side[side].sqr_e += ps->score[side][HEa].sqr_e;
#ifdef TUNING
			st->heavy[side]=1;
			st->variant[side]=HEa;
#endif
	}

	return 0;
}

int eval_king2(board const *b, attack_model *a, PawnStore const *ps, int side, personality const *p, stacker *st)
{
	int from, m;
	int heavy_op, vr;
	BITVAR mv;

	a->specs[side][KING].sqr_b = 0;
	a->specs[side][KING].sqr_e = 0;
	from = b->king[side];
	a->scc[from].sqr_b=0;
	a->scc[from].sqr_e=0;

#if 1
	int opmat_o=128, mat_o_tot, sl, row;
	mat_o_tot=128;
	if(b->mindex_validity==1) {
		mat_o_tot=p->mat[MAXMAT_IDX].mat_o[WHITE]+p->mat[MAXMAT_IDX].mat_o[BLACK];
		opmat_o=p->mat[b->mindex].mat_o[Flip(side)];
	}
#endif

// king mobility, spocitame vsechna pole kam muj kral muze (tj. krome vlastnich figurek a poli na ktere utoci nepratelsky kral
// a poli ktera jsou napadena cizi figurou
	mv = (attack.maps[KING][from]) & (~b->colormaps[side])
		& (~attack.maps[KING][b->king[Flip(side)]]);
	mv = mv & (~a->att_by_side[Flip(side)]) & (~a->ke[side].cr_att_ray)
		& (~a->ke[side].di_att_ray);

	m = a->me[from].pos_att_tot = BitCount(mv);
// king square mobility
	a->me[from].pos_mob_tot_b = p->mob_val[MG][side][KING][m];
	a->me[from].pos_mob_tot_e = p->mob_val[EG][side][KING][m];
// king square PST
	a->sq[from].sqr_b = p->piecetosquare[MG][side][KING][from];
	a->sq[from].sqr_e = p->piecetosquare[EG][side][KING][from];

#ifdef TUNING
		ADD_STACKER(st, mob_val[MG][side][KING][m], 1, BAs, side)
		ADD_STACKER(st, mob_val[EG][side][KING][m], 1, BAs, side)
//!!!!!!!!!!!!!!!!!
//		if(side==WHITE) {
		ADD_STACKER(st, piecetosquare[MG][side][KING][from], 1, BAs, side)
		ADD_STACKER(st, piecetosquare[EG][side][KING][from], 1, BAs, side)
//		} else
//		{
//		ADD_STACKER(st, piecetosquare[MG][side][KING][from], 1, BAs, side)
//		ADD_STACKER(st, piecetosquare[EG][side][KING][from], 1, BAs, side)
//		}
#endif

	heavy_op = ((((b->maps[ROOK] | b->maps[QUEEN]) & b->colormaps[Flip(side)])!=0)
		&& (p->use_heavy_material!=0));

/*
 * consider quality of pawn shelter with regard to placement of king and opposing material, especially heavy 
 * eventually rescale 
 */

#if 0
// evalute shelter
	if(p->use_pawn_shelter!=0) {
	sl=getFile(from);
	row=getRank(from);
	if(((side==WHITE)&&(row==RANKi1))||((side==BLACK)&&(row==RANKi7))) {
	  if(!heavy_op) {
// add KING specials for the side
		if((sl>=FILEiD)&&(sl<=FILEiF)) {
			a->specs[side][KING].sqr_b+=ps->score[side][SHm].sqr_b*opmat_o/mat_o_tot;
			a->specs[side][KING].sqr_e+=ps->score[side][SHm].sqr_e*opmat_o/mat_o_tot;
		} else if(sl<=FILEiC) {
			a->specs[side][KING].sqr_b+=ps->score[side][SHa].sqr_b*opmat_o/mat_o_tot;
			a->specs[side][KING].sqr_e+=ps->score[side][SHa].sqr_e*opmat_o/mat_o_tot;
		} else if(sl>=FILEiF) {
			a->specs[side][KING].sqr_b+=ps->score[side][SHh].sqr_b*opmat_o/mat_o_tot;
			a->specs[side][KING].sqr_e+=ps->score[side][SHh].sqr_e*opmat_o/mat_o_tot;
		}
	  } else {
		if((sl>=FILEiD)&&(sl<=FILEiF)) {
			a->specs[side][KING].sqr_b+=ps->score[side][SHmh].sqr_b*opmat_o/mat_o_tot;
			a->specs[side][KING].sqr_e+=ps->score[side][SHmh].sqr_e*opmat_o/mat_o_tot;
		} else if(sl<=FILEiC) {
			a->specs[side][KING].sqr_b+=ps->score[side][SHah].sqr_b*opmat_o/mat_o_tot;
			a->specs[side][KING].sqr_e+=ps->score[side][SHah].sqr_e*opmat_o/mat_o_tot;
		} else if(sl>=FILEiF) {
			a->specs[side][KING].sqr_b+=ps->score[side][SHhh].sqr_b*opmat_o/mat_o_tot;
			a->specs[side][KING].sqr_e+=ps->score[side][SHhh].sqr_e*opmat_o/mat_o_tot;
		}
	  }
	  
	}
	}
#endif

#if 1
// evalute shelter
	vr=BAs;
	if((p->use_pawn_shelter!=0)) {
		sl=getFile(from);
		row=getRank(from);
		// king on base line
		if(((side==WHITE)&&(row==0))||((side==BLACK)&&(row==7))) {
// heavy opposition?
		  if(!heavy_op) {
			if((sl>=FILEiD)&&(sl<FILEiF)) {
				vr=SHm;
			} else if(sl<=FILEiC) {
				vr=SHa;
			} else if(sl>=FILEiF) {
				vr=SHh;
			}
		  } else {
			if((sl>=FILEiD)&&(sl<FILEiF)) {
				vr=SHmh;
			} else if(sl<=FILEiC) {
				vr=SHah;
			} else if(sl>=FILEiF) {
				vr=SHhh;
			}
		  }
		}
	}

	if(vr!=BAs) {
		a->specs[side][KING].sqr_b+=ps->score[side][vr].sqr_b;
		a->specs[side][KING].sqr_e+=ps->score[side][vr].sqr_e;
		a->scc[from].sqr_b+=ps->score[side][vr].sqr_b;
		a->scc[from].sqr_e+=ps->score[side][vr].sqr_e;

#ifdef TUNING
		st->variant[side]=vr;
#endif
	}

//	L0("KING var:%d  eval side:%d, %d:%d\n", vr, side, ps->score[side][vr].sqr_b, ps->score[side][vr].sqr_e);
//	L0("KING var:BA  eval side:%d, %d:%d\n", side, ps->score[side][BAs].sqr_b, ps->score[side][BAs].sqr_e);
//	L0("KING var:SHa eval side:%d, %d:%d\n", side, ps->score[side][SHa].sqr_b, ps->score[side][SHa].sqr_e);
//	L0("KING var:SHh eval side:%d, %d:%d\n", side, ps->score[side][SHh].sqr_b, ps->score[side][SHh].sqr_e);
//	L0("KING var:SHm eval side:%d, %d:%d\n", side, ps->score[side][SHm].sqr_b, ps->score[side][SHm].sqr_e);
#endif

// add king mobility to side mobility score
	a->sc.side[side].mobi_b += a->me[from].pos_mob_tot_b;
	a->sc.side[side].mobi_e += a->me[from].pos_mob_tot_e;
// add KING PST to side PST bonuses
	a->sc.side[side].sqr_b += a->sq[from].sqr_b;
	a->sc.side[side].sqr_e += a->sq[from].sqr_e;
// add KING specials to side specials
	a->sc.side[side].specs_b += a->specs[side][KING].sqr_b;
	a->sc.side[side].specs_e += a->specs[side][KING].sqr_e;
	return 0;
}

int eval_inter_bishop(board const *b, attack_model *a, PawnStore const *ps, int side, personality const *p, stacker *st)
{
	if ((GT_M(b, p, side, DBISHOP, 0) > 0)
		&& (GT_M(b, p, side, LBISHOP, 0) > 0)) {
		a->sc.side[side].specs_b += p->bishopboth[MG];
		a->sc.side[side].specs_e += p->bishopboth[EG];
#ifdef TUNING
		ADD_STACKER(st, bishopboth[MG], 1, BAs, side)
		ADD_STACKER(st, bishopboth[EG], 1, BAs, side)
#endif
	}
	return 0;
}

int eval_inter_rook(board const *b, attack_model *a, PawnStore const *ps, int side, personality const *p, stacker *st)
{
	if (GT_M(b, p, side, ROOK, 1) > 1) {
		a->sc.side[side].specs_b += p->rookpair[MG];
		a->sc.side[side].specs_e += p->rookpair[EG];
#ifdef TUNING
		ADD_STACKER(st, rookpair[MG], 1, BAs, side)
		ADD_STACKER(st, rookpair[EG], 1, BAs, side)
#endif
	}
	return 0;
}

int eval_inter_knight(board const *b, attack_model *a, PawnStore const *ps, int side, personality const *p, stacker *st)
{
	if (GT_M(b, p, side, KNIGHT, 1) > 1) {
		a->sc.side[side].specs_b += p->knightpair[MG];
		a->sc.side[side].specs_e += p->knightpair[EG];
#ifdef TUNING
		ADD_STACKER(st, knightpair[MG], 1, BAs, side)
		ADD_STACKER(st, knightpair[EG], 1, BAs, side)
#endif
	}
	return 0;
}

/*
 * hodnoceni dle
 * material
 * pieceSquare
 * struktura pescu
 * speciality jednotlivych figur
 * mobilita
 * boj o stred
 * vzajemne propojeni
 * chycene figury
 * bezpecnost krale
 */

// pawn attacks - what squares are attacked by pawns of relevant side
// make model
// make pawn model
// eval_king
// WHITE POV!
int eval_x(board const *b, attack_model *a, personality const *p, stacker *st)
{
	int temp_b, temp_e;
	
	a->phase = eval_phase(b, p);
// setup pawn attacks
	PawnStore *ps;

	a->hpep = &(a->hpe);
	/*
	 * pawn attacks and moves require cr_pins, di_pins setup
	 */

// initialize
	a->sc.side[0].mobi_b = 0;
	a->sc.side[0].mobi_e = 0;
	a->sc.side[0].sqr_b = 0;
	a->sc.side[0].sqr_e = 0;
	a->sc.side[0].specs_b = 0;
	a->sc.side[0].specs_e = 0;
	a->sc.side[1].mobi_b = 0;
	a->sc.side[1].mobi_e = 0;
	a->sc.side[1].sqr_b = 0;
	a->sc.side[1].sqr_e = 0;
	a->sc.side[1].specs_b = 0;
	a->sc.side[1].specs_e = 0;

#ifdef TUNING
	st->variant[WHITE]=st->variant[BLACK]=BAs;
#endif

	a->specs[WHITE][ROOK].sqr_b = a->specs[BLACK][ROOK].sqr_b = 0;
	a->specs[WHITE][ROOK].sqr_e = a->specs[BLACK][ROOK].sqr_e = 0;
	a->specs[WHITE][BISHOP].sqr_b = a->specs[BLACK][BISHOP].sqr_b = 0;
	a->specs[WHITE][BISHOP].sqr_e = a->specs[BLACK][BISHOP].sqr_e = 0;
	a->specs[WHITE][KNIGHT].sqr_b = a->specs[BLACK][KNIGHT].sqr_b = 0;
	a->specs[WHITE][KNIGHT].sqr_e = a->specs[BLACK][KNIGHT].sqr_e = 0;
	a->specs[WHITE][QUEEN].sqr_b = a->specs[BLACK][QUEEN].sqr_b = 0;
	a->specs[WHITE][QUEEN].sqr_e = a->specs[BLACK][QUEEN].sqr_e = 0;
	a->specs[WHITE][KING].sqr_b = a->specs[BLACK][KING].sqr_b = 0;
	a->specs[WHITE][KING].sqr_e = a->specs[BLACK][KING].sqr_e = 0;
	a->specs[WHITE][PAWN].sqr_b = a->specs[BLACK][PAWN].sqr_b = 0;
	a->specs[WHITE][PAWN].sqr_e = a->specs[BLACK][PAWN].sqr_e = 0;

// build attack model + calculate mobility
	make_mobility_modelN2(b, a, p, st);

// build pawn mode + pawn cache + evaluate + pre compute pawn king shield + pawn mobility
	premake_pawn_model(b, a, &(a->hpep), p, st);
	ps = &(a->hpep->value);

// compute material	
	get_material_eval(b, p, &a->sc.material, &a->sc.material_e,
		&a->sc.material_b_w, &a->sc.material_e_w, st);
// evaluate individual pieces + PST + piece special feature + features related to piece-pawn interaction
	eval_bishop(b, a, ps, WHITE, p, st);
	eval_bishop(b, a, ps, BLACK, p, st);
	eval_knight(b, a, ps, WHITE, p, st);
	eval_knight(b, a, ps, BLACK, p, st);
	eval_queen(b, a, ps, WHITE, p, st);
	eval_queen(b, a, ps, BLACK, p, st);
	eval_rook(b, a, ps, WHITE, p, st);
	eval_rook(b, a, ps, BLACK, p, st);
	eval_pawn(b, a, ps, WHITE, p, st);
	eval_pawn(b, a, ps, BLACK, p, st);

// evaluate king 
	eval_king2(b, a, ps, WHITE, p, st);
	eval_king2(b, a, ps, BLACK, p, st);

// evaluate inter pieces features or global features
	eval_inter_bishop(b, a, ps, WHITE, p, st);
	eval_inter_bishop(b, a, ps, BLACK, p, st);
	eval_inter_knight(b, a, ps, WHITE, p, st);
	eval_inter_knight(b, a, ps, BLACK, p, st);
	eval_inter_rook(b, a, ps, WHITE, p, st);
	eval_inter_rook(b, a, ps, BLACK, p, st);

// side to move tempo bonus
	if (b->side == WHITE) {
		temp_b = p->move_tempo[MG];
		temp_e = p->move_tempo[EG];
	} else {
		temp_b = -p->move_tempo[MG];
		temp_e = -p->move_tempo[EG];
	}

	if (p->simple_EVAL == 1) {
// simplified eval - Material and PST only
		a->sc.score_b_w = a->sc.side[0].sqr_b + a->sc.material_b_w;
		a->sc.score_b_b = a->sc.side[1].sqr_b + a->sc.material_b_w
			- a->sc.material;
		a->sc.score_e_w = a->sc.side[0].sqr_e + a->sc.material_e_w;
		a->sc.score_e_b = a->sc.side[1].sqr_e + a->sc.material_e_w
			- a->sc.material_e;
		a->sc.score_b = a->sc.score_b_w - a->sc.score_b_b + temp_b;
		a->sc.score_e = a->sc.score_e_w - a->sc.score_e_b + temp_e;
		a->sc.score_nsc = a->sc.score_b * a->phase
			+ a->sc.score_e * (255 - a->phase);
	} else {
#if 1
		a->sc.score_b_w = a->sc.side[0].mobi_b + a->sc.side[0].sqr_b
			+ a->sc.side[0].specs_b + a->sc.material_b_w;
		a->sc.score_b_b = a->sc.side[1].mobi_b + a->sc.side[1].sqr_b
			+ a->sc.side[1].specs_b + a->sc.material_b_w - a->sc.material;
		a->sc.score_e_w = a->sc.side[0].mobi_e + a->sc.side[0].sqr_e
			+ a->sc.side[0].specs_e + a->sc.material_e_w;
		a->sc.score_e_b = a->sc.side[1].mobi_e + a->sc.side[1].sqr_e
			+ a->sc.side[1].specs_e + a->sc.material_e_w - a->sc.material_e;
		
		a->sc.score_b = a->sc.score_b_w - a->sc.score_b_b + temp_b;
		a->sc.score_e = a->sc.score_e_w - a->sc.score_e_b + temp_e;
		a->sc.score_nsc = a->sc.score_b * a->phase
			+ a->sc.score_e * (255 - a->phase);
#endif
	}
	return a->sc.score_nsc;
}

void eval_lnk(board const *b, attack_model *a, int piece, int side, int pp)
{
	BITVAR x;
	int f;
	x = (b->maps[piece] & b->colormaps[side]);
	for (f = 0; f < 8; f++) {
		if (!x)
			break;
		a->pos_m[pp][f] = LastOne(x);
		ClrLO(x);
	}
	a->pos_c[pp] = f - 1;
}

void eval_lnk2(board const *b, attack_model *a){
int si,pi,i,bp,pp;
BITVAR x;

	for(si=0;si<2;si++) {
		for(pi=KNIGHT;pi<ER_PIECE;pi++) {
			pp=pi+si*BLACKPIECE;
			i=-1;
			x=b->maps[pi]&b->colormaps[si];
			while(x) {
				a->pos_m[pp][++i] = LastOne(x);
				ClrLO(x);
			}
			a->pos_c[pp] = i;
		}
	}
return;
}

void eval_lnks(board const *b, attack_model *a){
int si,pi,i,bp,pp, po;
BITVAR x,n;

	a->pos_c[PAWN]=a->pos_c[KNIGHT]=a->pos_c[BISHOP]=a->pos_c[ROOK]=a->pos_c[QUEEN]=a->pos_c[KING]=-1;
	a->pos_c[PAWN+BLACKPIECE]=a->pos_c[KNIGHT+BLACKPIECE]=a->pos_c[BISHOP+BLACKPIECE]
		=a->pos_c[ROOK+BLACKPIECE]=a->pos_c[QUEEN+BLACKPIECE]=a->pos_c[KING+BLACKPIECE]
		=a->pos_c[ER_PIECE]=-1;
	x=b->norm;
//	for(i=0;i<64;i++){
	while(x) {
		i=LastOne(x);
		pi=b->pieces[i];
		a->pos_m[pi][++(a->pos_c[pi])]=i;
		ClrLO(x);
	}
return;
}

int eval(board const *b, attack_model *a, personality const *p, stacker *st)
{
	long score;
	int f, sqb, sqe, ch;

//	eval_lnks(b, a);

#ifdef TUNING
	REINIT_STACKER(st)
#endif
	
	eval_x(b, a, p, st);
// here the stacker has all features recognised, in BAs scenario, other variants are on top of BAs

	a->sc.scaling = 129;
	
// scaling
	score = a->sc.score_nsc + p->eval_BIAS + p->eval_BIAS_e;
	if (b->mindex_validity == 1) a->sc.scaling = (p->mat_info[b->mindex].info[b->side]);
//	L0("Valid\n");
	score = (score * a->sc.scaling) / 128;
	a->sc.complete = score / 255;

#if 0
			printBoardNice(b);
//			LOGGER_0("mat %d, mob %d, mob %d, sqr %d, sqr %d, spc %d, spc %d\n", a->sc.material,a->sc.side[0].mobi_b, a->sc.side[1].mobi_b, a->sc.side[0].sqr_b, a->sc.side[1].sqr_b, a->sc.side[0].specs_b, a->sc.side[1].specs_b );
//			LOGGER_0("mat %d, mob %d, mob %d, sqr %d, sqr %d, spc %d, spc %d\n", a->sc.material_e,a->sc.side[0].mobi_e,a->sc.side[1].mobi_e,a->sc.side[0].sqr_e, a->sc.side[1].sqr_e, a->sc.side[0].specs_e, a->sc.side[1].specs_e );
			LOGGER_0("score %d, nsc %d, scale %d, phase %d, score_b %d, score_e %d\n", a->sc.complete / 255,  a->sc.score_nsc, a->sc.scaling, a->phase, a->sc.score_b, a->sc.score_e);
			eval_dump(b, a, b->pers);
#endif

#if 0
	sqb=sqe=0;
	for(f=A1;f<=H8; f++) {
		if(b->pieces[f]!=ER_PIECE) {
		  ch=b->pieces[f]&PIECEMASK;
		  if(b->pieces[f]&BLACKPIECE) {
			sqb-=p->piecetosquare[MG][0][ch][f];
			sqe-=p->piecetosquare[EG][0][ch][f];
			LOGGER_0("PSQ sq:%o, piece:%d, side:%d, val:%d:%d\n", f, ch, BLACK, p->piecetosquare[MG][0][ch][f], p->piecetosquare[EG][0][ch][f]);
		  }
		  else {
			sqb+=p->piecetosquare[MG][0][ch][f];
			sqe+=p->piecetosquare[EG][0][ch][f];
			LOGGER_0("PSQ sq:%o, piece:%d, side:%d, val:%d:%d\n", f, ch, WHITE, p->piecetosquare[MG][0][ch][f], p->piecetosquare[EG][0][ch][f]);
		  }
		}
	}
	LOGGER_0("TUNE score xxxx, sb %d, se %d\n", sqb, sqe);
	LOGGER_0("NORM score %d, mb %d, me %d, sb %d, se %d\n", a->sc.complete, a->sc.material, a->sc.material_e, a->sc.side[0].sqr_b-a->sc.side[1].sqr_b, a->sc.side[0].sqr_e-a->sc.side[1].sqr_e);
	
	L0("\n");
#endif

	return a->sc.complete;
}

int lazyEval(board const *b, attack_model *a, int alfa, int beta, int side, int ply, int depth, personality const *p, int *fullrun)
{
	int scr, sc4, sc3, sc2;
	int mb, me, wb, we;
	stacker st;

	a->phase = eval_phase(b, p);
	*fullrun = 0;
	
	get_material_eval(b, p, &mb, &me, &wb, &we, &st);
	sc4 = (mb * a->phase + me * (255 - a->phase)) / 255;
	
	sc3 = (b->psq_b * a->phase + b->psq_e * (255 - a->phase)) / 255;
	sc2 = sc3 + sc4;
	
	if ((((sc2 + p->lazy_eval_cutoff) < alfa)
		|| (sc2 > (beta + p->lazy_eval_cutoff)))) {
		scr = sc2;
//		LOGGER_0("score %d, mat %d, psq %d, alfa %d, beta %d, cutoff %d\n", sc2, sc4, sc3, alfa, beta, p->lazy_eval_cutoff);
	} else {
		*fullrun = 1;
		a->att_by_side[side] = KingAvoidSQ(b, a, side);
		eval_king_checks(b, &(a->ke[Flip(b->side)]), NULL, Flip(b->side));
//		simple_pre_movegen_n2(b, a, Flip(side));
//		simple_pre_movegen_n2(b, a, side);
		eval(b, a, b->pers, &st);
		scr = a->sc.complete;
//		LOGGER_0("score CMP %d:%d, mat %d, psq %d, alfa %d, beta %d, cutoff %d\n", scr, sc2, sc4, sc3, alfa, beta, p->lazy_eval_cutoff);
	}
//	LOGGER_0("LAZY score %d, mb %d, me %d, sb %d, se %d\n", sc2, mb, me, b->psq_b, b->psq_e);
	
	if (side == WHITE)
		return scr;
	else
		return 0 - scr;
}

int SEE(board *b, MOVESTORE m)
{
	int fr, to, side, d, attacker, piece;
	int gain[32];
	BITVAR ignore, bto, ppromote;

	ignore = FULLBITMAP;
	fr = UnPackFrom(m);
	to = UnPackTo(m);
	bto = normmark[to];
	ppromote = (RANK1 | RANK8) & bto;
	side = (b->pieces[fr] & BLACKPIECE) != 0;
	d = 0;
	if (bto & b->norm) {
		piece = b->pieces[to] & PIECEMASK;
		gain[d] = b->pers->Values[1][piece];
	} else
		gain[d] = 0;
	attacker = fr;
	while (attacker != -1) {
		d++;
		piece = b->pieces[attacker] & PIECEMASK;
		gain[d] =
				((ppromote) && (piece == PAWN)) ?
					-gain[d - 1] + b->pers->Values[EG][QUEEN]
						- b->pers->Values[EG][PAWN] :
					-gain[d - 1] + b->pers->Values[EG][piece];
		side = Flip(side);
		ignore ^= normmark[attacker];
		attacker = GetLVA_to(b, to, side, ignore);
	}
	while (--d)
		gain[d - 1] = -Max(-gain[d - 1], gain[d]);
	return gain[0];
}

int SEEx(board *b, MOVESTORE m)
{
	int gain[32];
	int fr, to, side, d, attacker, piece;
	BITVAR ignore, bto, ppromote;

	ignore = FULLBITMAP;
	fr = UnPackFrom(m);
	to = UnPackTo(m);
	bto = normmark[to];
	ppromote = (RANK1 | RANK8) & bto;
	side = (b->pieces[fr] & BLACKPIECE) != 0;
	d = 0;
	if (bto & b->norm) {
		piece = b->pieces[to] & PIECEMASK;
		gain[d] = b->pers->Values[EG][piece];
	} else
		gain[d] = 0;
	attacker = fr;
	while (attacker != -1) {
		d++;
		piece = b->pieces[attacker] & PIECEMASK;
		gain[d] =
				((ppromote) && (piece == PAWN)) ?
					-gain[d - 1] + b->pers->Values[EG][QUEEN]
						- b->pers->Values[EG][PAWN] :
					-gain[d - 1] + b->pers->Values[EG][piece];
		if (Max(-gain[d-1], gain[d]) < 0)
			break;
		side = Flip(side);
		ignore ^= normmark[attacker];
		attacker = GetLVA_to(b, to, side, ignore);
	}
	while (--d)
		gain[d - 1] = -Max(-gain[d - 1], gain[d]);
	return gain[0];
}

/*
 * SEE after piece moved to to
 */

/*
 x R n P
 x 5 3 1

 x -x+5 x-5+3 -x+2+1


 */

int SEE0(board *b, int to, int side, int val)
{
	int d, attacker, piece;
	int gain[32];
	BITVAR ignore, bto, ppromote;

	ignore = FULLBITMAP;
	bto = normmark[to];
	ppromote = (RANK1 | RANK8) & bto;
	side = Flip(side);
	d = 0;
	piece = b->pieces[to] & PIECEMASK;
	gain[d++] = val;
	gain[d] = -val + b->pers->Values[EG][piece];
	attacker = GetLVA_to(b, to, side, ignore);
	while (attacker != -1) {
		piece = b->pieces[attacker] & PIECEMASK;
		d++;
		gain[d] =
				((ppromote) && (piece == PAWN)) ?
					-gain[d - 1] + b->pers->Values[EG][QUEEN]
						- b->pers->Values[EG][PAWN] :
					-gain[d - 1] + b->pers->Values[EG][piece];
		if (Max(-gain[d-1], gain[d]) < 0)
			break;
		side = Flip(side);
		ignore ^= normmark[attacker];
		attacker = GetLVA_to(b, to, side, ignore);
	}
	while (--d)
		gain[d - 1] = -Max(-gain[d - 1], gain[d]);
	return gain[0];
}

// [side][piece] 
BITVAR getMatKey(unsigned char m[][2 * ER_PIECE])
{
	BITVAR k;
	k = 0;
	k ^= randomTable[WHITE][A2 + m[WHITE][PAWN]][PAWN];
	k ^= randomTable[WHITE][A2 + m[WHITE][KNIGHT]][KNIGHT];
	k ^= randomTable[WHITE][A2 + m[WHITE][BISHOP] - m[WHITE][DBISHOP]][BISHOP];
	k ^= randomTable[WHITE][A4 + m[WHITE][DBISHOP]][BISHOP];
	k ^= randomTable[WHITE][A2 + m[WHITE][ROOK]][ROOK];
	k ^= randomTable[WHITE][A2 + m[WHITE][QUEEN]][QUEEN];

	k ^= randomTable[BLACK][A2 + m[BLACK][PAWN]][PAWN];
	k ^= randomTable[BLACK][A2 + m[BLACK][KNIGHT]][KNIGHT];
	k ^= randomTable[BLACK][A2 + m[BLACK][BISHOP] - m[BLACK][DBISHOP]][BISHOP];
	k ^= randomTable[BLACK][A4 + m[BLACK][DBISHOP]][BISHOP];
	k ^= randomTable[BLACK][A2 + m[BLACK][ROOK]][ROOK];
	k ^= randomTable[BLACK][A2 + m[BLACK][QUEEN]][QUEEN];
	return k;
}

/*
 * check whether piece counts are within limits imposed by normal board setup
 * normally this can be changed with promotion, reasonable promotion is another queen
 * so without force parameter just check queen count. Force on mindex_validity==0
 * trigger full rescan
 */

int check_mindex_validity(board *b, int force)
{

	int bwl, bwd, bbl, bbd;
	int qw, qb, rw, rb, nw, nb, pw, pb;

	if ((force == 1) || (b->mindex_validity == 0)) {
		b->mindex_validity = 0;
//		L0("check\n");
		collect_material_from_board(b, &pw, &pb, &nw, &nb, &bwl, &bwd, &bbl,
			&bbd, &rw, &rb, &qw, &qb);
		if ((qw > 1) || (qb > 1) || (nw > 2) || (nb > 2) || (bwd > 1)
			|| (bwl > 1) || (bbl > 1) || (bbd > 1) || (rw > 2) || (rb > 2)
			|| (pw > 8) || (pb > 8))
			return 0;
		b->mindex = MATidx(pw, pb, nw, nb, bwl, bwd, bbl, bbd, rw, rb, qw, qb);
//		L0("checked\n");
		b->mindex_validity = 1;
	} else {
		b->mindex_validity = 0;
		qw = BitCount(b->maps[QUEEN] & b->colormaps[WHITE]);
		qb = BitCount(b->maps[QUEEN] & b->colormaps[BLACK]);
		if ((qw > 1) || (qb > 1))
			return 0;
//		L0("checked2\n");
		b->mindex_validity = 1;
	}
	return 1;
}

// move ordering is to get the fastest beta cutoff
int MVVLVA_gen(int table[ER_PIECE + 2][ER_PIECE+1], _values Values)
{
	int v[ER_PIECE];
	int vic, att;
	v[PAWN] = P_OR;
	v[KNIGHT] = N_OR;
	v[BISHOP] = B_OR;
	v[ROOK] = R_OR;
	v[QUEEN] = Q_OR;
	v[KING] = K_OR_M;
	for (vic = PAWN; vic < ER_PIECE; vic++) {
		for (att = PAWN; att < ER_PIECE; att++) {
// all values inserted are positive!
			if (vic == att) {
				table[att][vic] = A_OR + (7 * v[att] - v[att]) * 2;
			} else if (vic > att) {
				table[att][vic] = A_OR + (7 * v[vic] - v[att]) * 2;
			} else if (vic < att) {
				table[att][vic] = A_OR2 + (7 * v[vic] - v[att]) * 2;
			}
		}
	}
#if 1
// lines for capture+promotion
// to queen
	for (vic = PAWN; vic < ER_PIECE; vic++) {
		att = PAWN;
		table[KING + 1][vic] = A_OR + (7 * v[vic] - v[PAWN] + v[QUEEN]) * 2;
	}
// to knight
	for (vic = PAWN; vic < ER_PIECE; vic++) {
		att = PAWN;
		table[KING + 2][vic] = A_OR + (7 * v[vic] - v[PAWN] + v[KNIGHT]) * 2;
	}
	table[PAWN][ER_PIECE]	=MV_OR+P_OR;
	table[KNIGHT][ER_PIECE]	=MV_OR+N_OR;
	table[BISHOP][ER_PIECE]	=MV_OR+B_OR;
	table[ROOK][ER_PIECE]	=MV_OR+R_OR;
	table[QUEEN][ER_PIECE]	=MV_OR+Q_OR;
	table[KING][ER_PIECE]	=MV_OR+K_OR;
#endif

	return 0;
}

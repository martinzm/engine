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

uint8_t eval_phase(board const *b, personality const *p)
{
	int i, i1, i2, i3, i4, i5, tot, q;
	int vaha[] = { 0, 5, 5, 11, 22 };
	int nc[] = { 16, 4, 4, 4, 2 };

	int faze;

// 255 -- pure beginning, 0 -- total ending
	if (b->mindex_validity == 1) {
		faze = (int) p->mat_faze[b->mindex] & 0xff;
	} else {
		tot = nc[PAWN] * vaha[PAWN] + nc[KNIGHT] * vaha[KNIGHT]
			+ nc[BISHOP] * vaha[BISHOP] + nc[ROOK] * vaha[ROOK]
			+ nc[QUEEN] * vaha[QUEEN];
		i1 = BitCount(b->maps[PAWN]) * vaha[PAWN];
		i2 = BitCount(b->maps[KNIGHT]) * vaha[KNIGHT];
		i3 = BitCount(b->maps[BISHOP]) * vaha[BISHOP];
		i4 = BitCount(b->maps[ROOK]) * vaha[ROOK];
		i5 = BitCount(b->maps[QUEEN]) * vaha[QUEEN];
		i = i1 + i2 + i3 + i4 + i5;
		q = Min(i, tot);
		faze = (uint8_t) q * 255 / (tot);
	}
	return (uint8_t) faze & 255;
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

#define MAKEMOBx(piece, side, pp) \
	for(f=a->pos_c[pp];f>=0;f--) { \
		from=a->pos_m[pp][f]; \
		q=a->mvs[from]; \
		a->att_by_side[side]|=q; \
		m=a->me[from].pos_att_tot=BitCount(q & togo[side]); \
		m2=BitCount(q & togo[side] & unsafe[side]); \
		a->me[from].pos_mob_tot_b=p->mob_val[MG][side][piece][m-m2]; \
		a->me[from].pos_mob_tot_e=p->mob_val[EG][side][piece][m-m2]; \
		if(p->mobility_unsafe==1) { \
			a->me[from].pos_mob_tot_b+=p->mob_uns[MG][side][piece][m2]; \
			a->me[from].pos_mob_tot_e+=p->mob_uns[EG][side][piece][m2]; \
		} \
	} 

#ifdef TUNING
#define MAKEMOB(piece, side, pp, st) \
	for(f=a->pos_c[pp];f>=0;f--) { \
		from=a->pos_m[pp][f]; \
		q=a->mvs[from]; \
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
		} \
	} 
	
#else
#define MAKEMOB(piece, side, pp, st) \
	for(f=a->pos_c[pp];f>=0;f--) { \
		from=a->pos_m[pp][f]; \
		q=a->mvs[from]; \
		m=a->me[from].pos_att_tot=BitCount(q & togo[side]); \
		m2=BitCount(q & togo[side] & unsafe[side]); \
		a->me[from].pos_mob_tot_b=p->mob_val[MG][side][piece][m-m2]; \
		a->me[from].pos_mob_tot_e=p->mob_val[EG][side][piece][m-m2]; \
		if(p->mobility_unsafe==1) { \
			a->me[from].pos_mob_tot_b+=p->mob_uns[MG][side][piece][m2]; \
			a->me[from].pos_mob_tot_e+=p->mob_uns[EG][side][piece][m2]; \
		} \
	} 
	
#endif

int make_mobility_modelN(board const *b, attack_model *a, personality const *p, stacker *st)
{
	int from, pp, m, m2, pc, f, side;
	BITVAR x, q, togo[2], unsafe[2];

// distribute to pawn pre eval
// a->pa_at - pawn attacks for side
	a->pa_at[WHITE] = a->pa_at[BLACK] = a->pa_mo[WHITE] = a->pa_mo[BLACK] = 0;

// compute pawn mobility + pawn attacks/moves
	int pt[2] = { PAWN, PAWN | BLACKPIECE };
	for (side = WHITE; side <= BLACK; side++) {
		pp = pt[side];
		x = (b->maps[PAWN] & b->colormaps[side]);
		for (f = 0; f < 8; f++) {
			if (!x)
				break;
			pc = a->pos_m[pp][f] = LastOne(x);
			q = attack.pawn_att[side][pc];
			a->pa_at[side] |= q;
			a->pa_mo[side] |= a->mvs[pc] & (~q);
			ClrLO(x);
		}
		a->pos_c[pp] = f - 1;
	}

// do not count pawn attacked and squares with side pieces by default
	togo[WHITE] = ~(b->colormaps[WHITE] | a->pa_at[BLACK]);
	togo[BLACK] = ~(b->colormaps[BLACK] | a->pa_at[WHITE]);
	unsafe[WHITE] = a->pa_at[BLACK];
	unsafe[BLACK] = a->pa_at[WHITE];

// count moves to squares with my own pieces, protection
	if (p->mobility_protect == 1) {
		togo[WHITE] |= (b->colormaps[WHITE] & ~unsafe[WHITE]);
		togo[BLACK] |= (b->colormaps[BLACK] & ~unsafe[BLACK]);
	}
// count moves to squares attacked (by opp pawns)
	if (p->mobility_unsafe == 1) {
		togo[WHITE] |= (unsafe[WHITE] & ~b->norm);
		togo[BLACK] |= (unsafe[BLACK] & ~b->norm);
	}
	if ((p->mobility_unsafe == 1) && (p->mobility_protect == 1)) {
		togo[WHITE] |= (unsafe[WHITE] & b->colormaps[WHITE]);
		togo[BLACK] |= (unsafe[BLACK] & b->colormaps[BLACK]);
	}

	MAKEMOB(QUEEN, WHITE, QUEEN, st)
	MAKEMOB(QUEEN, BLACK, QUEEN+BLACKPIECE, st)
	MAKEMOB(ROOK, WHITE, ROOK, st)
	MAKEMOB(ROOK, BLACK, ROOK+BLACKPIECE, st)
	MAKEMOB(BISHOP, WHITE, BISHOP, st)
	MAKEMOB(BISHOP, BLACK, BISHOP+BLACKPIECE, st)
	MAKEMOB(KNIGHT, WHITE, KNIGHT, st)
	MAKEMOB(KNIGHT, BLACK, KNIGHT+BLACKPIECE, st)
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

// shelter opposing pawn approaching, how far
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
	// reset score for PAWNs in shelter to 0, all is relative to BAs
	ps->t_sc[side][pawn][sh].sqr_b = 0;
	ps->t_sc[side][pawn][sh].sqr_e = 0;
	ps->t_sc[side][pawn][shopt].sqr_b = 0;
	ps->t_sc[side][pawn][shopt].sqr_e = 0;

// if simple_EVAL then only material and PSQ are used
	if (p->simple_EVAL != 1) {
		if (x & (~(fst | sec))) {
			ps->t_sc[side][pawn][sh].sqr_b += (p->pshelter_out_penalty[MG]);
			ps->t_sc[side][pawn][sh].sqr_e += (p->pshelter_out_penalty[EG]);
#ifdef TUNING
			ADD_STACKER(st,pshelter_out_penalty[MG], 1, sh, side);
			ADD_STACKER(st,pshelter_out_penalty[EG], 1, sh, side);
//			uu[side][sh].p.pshelter_out_penalty[MG]++;
//			uu[side][sh].p.pshelter_out_penalty[EG]++;
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
//			uu[side][sh].p.pshelter_sec_bonus[MG]++;
//			uu[side][sh].p.pshelter_sec_bonus[EG]++;
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

// fixes for pawns protected by shelter pawns
			// koho chranim
			if (ps->prot_p_p[side][pawn]) {
				f = 0;
				while (ps->pawns[side][f] != -1) {
					if (ps->prot_p_p[side][pawn] & ps->pawns_b[side][f]) {
						n2 = ps->prot_p_c[side][f] & shlt;
// num of protectors belonging to shelter
						fn2 = BitCount(n2);
// protectors total
						fn = BitCount(ps->prot_p_c[side][f]);
						assert(fn2 > 0);
						if (fn == fn2) {

// only protected by shelter pawns
							ps->t_sc[side][pawn][sh].sqr_b -=
								p->pawn_pot_protect[MG][side][ps->prot_p_d[side][f]];
							ps->t_sc[side][pawn][sh].sqr_e -=
								p->pawn_pot_protect[EG][side][ps->prot_p_d[side][f]];
							ps->t_sc[side][pawn][sh].sqr_b -=
								p->pawn_protect_count[MG][side][fn];
							ps->t_sc[side][pawn][sh].sqr_e -=
								p->pawn_protect_count[EG][side][fn];

#ifdef TUNING
//			uu[side][sh].p.pawn_pot_protect[MG][side][ps->prot_p_d[side][f]]--;
//			uu[side][sh].p.pawn_pot_protect[EG][side][ps->prot_p_d[side][f]]--;
//			uu[side][sh].p.pawn_protect_count[MG][side][fn]--;
//			uu[side][sh].p.pawn_protect_count[EG][side][fn]--;
			ADD_STACKER(st, pawn_pot_protect[MG][side][ps->prot_p_d[side][f]], -1, sh, side);
			ADD_STACKER(st, pawn_pot_protect[EG][side][ps->prot_p_d[side][f]], -1, sh, side);
			ADD_STACKER(st, pawn_protect_count[MG][side][fn], -1, sh, side);
			ADD_STACKER(st, pawn_protect_count[EG][side][fn], -1, sh, side);
#endif

						} else {
							ps->t_sc[side][pawn][sh].sqr_b -=
								p->pawn_protect_count[MG][side][fn];
							ps->t_sc[side][pawn][sh].sqr_e -=
								p->pawn_protect_count[EG][side][fn];
							ps->t_sc[side][pawn][sh].sqr_b +=
								p->pawn_protect_count[MG][side][fn - fn2];
							ps->t_sc[side][pawn][sh].sqr_e +=
								p->pawn_protect_count[EG][side][fn - fn2];

#ifdef TUNING
			ADD_STACKER(st, pawn_protect_count[MG][side][fn], -1, sh, side);
			ADD_STACKER(st, pawn_protect_count[EG][side][fn], -1, sh, side);
			ADD_STACKER(st, pawn_protect_count[MG][side][fn - fn2], -1, sh, side);
			ADD_STACKER(st, pawn_protect_count[EG][side][fn - fn2], -1, sh, side);
#endif

						}
					}
					f++;
				}
			}
		}
		
//		ps->t_sc[side][pawn][shopt].sqr_b += ps->t_sc[side][pawn][sh].sqr_b;
//		ps->t_sc[side][pawn][shopt].sqr_e += ps->t_sc[side][pawn][sh].sqr_e;

// if at shelter position, unapply normal PST
		if (x & ((fst | sec))) {
			ps->t_sc[side][pawn][sh].sqr_b -=p->piecetosquare[MG][side][PAWN][from];
			ps->t_sc[side][pawn][sh].sqr_e -=p->piecetosquare[EG][side][PAWN][from];
#ifdef TUNING
			ADD_STACKER(st, piecetosquare[MG][side][PAWN][from], -1, sh, side);
			ADD_STACKER(st, piecetosquare[EG][side][PAWN][from], -1, sh, side);
#endif
		}
	}
	return 0;
}

int analyze_pawn_shield_globN(board const *b, attack_model const *a, PawnStore *ps, int side, BITVAR mask, int sh, int shopt, personality const *p, stacker *st)
{
	int count, opside;

	opside = Flip(side);
	if ((ps->not_pawns_file[side] & ps->not_pawns_file[opside]) & mask) {
		count =
			BitCount(
				ps->not_pawns_file[side] & ps->not_pawns_file[opside] & mask
					& RANK2);
		ps->score[side][shopt].sqr_b += (p->pshelter_open_penalty[MG] * count);
		ps->score[side][shopt].sqr_e += (p->pshelter_open_penalty[EG] * count);
#ifdef TUNING
//		uu[side][shopt].p.pshelter_open_penalty[MG]+=count;
//		uu[side][shopt].p.pshelter_open_penalty[EG]+=count;
		ADD_STACKER(st, pshelter_open_penalty[MG], count, shopt, side);
		ADD_STACKER(st, pshelter_open_penalty[EG], count, shopt, side);
#endif
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

/*
 * analyze various shelter options with or without heavy opposition
 */

// BAs, HEa are absolute, HEa Shelter variants are relative to Shelter variant, that are relative to BAs

int analyze_pawn_shieldN(board const *b, attack_model const *a, PawnStore *ps, personality const *p, stacker *st)
{
	int f;
	int side;

	// analyze shelters not related to individual pawn
	for (side = 0; side <= 1; side++) {
		analyze_pawn_shield_globN(b, a, ps, side, SHELTERA, SHa, SHah, p, st);
		analyze_pawn_shield_globN(b, a, ps, side, SHELTERH, SHh, SHhh, p, st);
		analyze_pawn_shield_globN(b, a, ps, side, SHELTERM, SHm, SHmh, p, st);
	}

	return 0;}

/*
 * Precompute various possible scenarios, their use depends on king position, heavy pieces availability etc
 * Prepare two basic scenarios BAs - no heavy opposition and no pawn shelter
 *
 * BAs is absolute, HEa is computed as relative to BAs - ie changes needed to get from BAs to HEa
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
			ps->t_sc[side][f][BAs].sqr_b =
				p->piecetosquare[MG][side][PAWN][from];
			ps->t_sc[side][f][BAs].sqr_e =
				p->piecetosquare[EG][side][PAWN][from];
#ifdef TUNING
			ADD_STACKER(st, piecetosquare[MG][side][PAWN][from], 1, BAs, side)
			ADD_STACKER(st, piecetosquare[EG][side][PAWN][from], 1, BAs, side)
#endif

// if simple_EVAL then only material and PSQ are used
			if (p->simple_EVAL != 1) {
// check if pawn might belong to some shelter a evaluate such variant

// isolated
				if ((ps->half_isol[side][0] | ps->half_isol[side][1]) & x) {
					if (ps->half_isol[side][0] & x) {
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
//		uu[side][HEa].p.pawn_iso_onopen_penalty[MG]++;
//		uu[side][HEa].p.pawn_iso_onopen_penalty[EG]++;
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
// on open file, heavy pieces related!!!
// if index is not BAs then we store at other index difference to BAs
					if ((x & ps->not_pawns_file[opside])) {
						assert(ps->t_sc[side][f][HEa].sqr_b == 0);
						assert(ps->t_sc[side][f][HEa].sqr_e == 0);
						
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
				ff = BitCount(
					a->pa_mo[side] & attack.pawn_move[side][from]
						& (~b->maps[PAWN]))
					+ BitCount(
						a->pa_at[side] & attack.pawn_att[side][from] & msk);
				ps->t_sc[side][f][BAs].sqr_b += p->mob_val[MG][side][PAWN][0]
					* ff;
				ps->t_sc[side][f][BAs].sqr_e += p->mob_val[EG][side][PAWN][0]
					* ff;
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
				ps->t_sc[side][f][BAs].sqr_b =
					ps->t_sc[side][f][BAs].sqr_e =
						ps->t_sc[side][f][HEa].sqr_b =
							ps->t_sc[side][f][HEa].sqr_e =
								ps->t_sc[side][f][SHa].sqr_b =
									ps->t_sc[side][f][SHa].sqr_e =
										ps->t_sc[side][f][SHh].sqr_b =
											ps->t_sc[side][f][SHh].sqr_e =
												ps->t_sc[side][f][SHm].sqr_b =
													ps->t_sc[side][f][SHm].sqr_e =
														ps->t_sc[side][f][SHah].sqr_b =
															ps->t_sc[side][f][SHah].sqr_e =
																ps->t_sc[side][f][SHhh].sqr_b =
																	ps->t_sc[side][f][SHhh].sqr_e =
																		ps->t_sc[side][f][SHmh].sqr_b =
																			ps->t_sc[side][f][SHmh].sqr_e =
																				0;
			}
			ps->score[side][BAs].sqr_b =
				ps->score[side][BAs].sqr_e =
					ps->score[side][HEa].sqr_b =
						ps->score[side][HEa].sqr_e =
							ps->score[side][SHa].sqr_b =
								ps->score[side][SHa].sqr_e =
									ps->score[side][SHh].sqr_b =
										ps->score[side][SHh].sqr_e =
											ps->score[side][SHm].sqr_b =
												ps->score[side][SHm].sqr_e =
													ps->score[side][SHah].sqr_b =
														ps->score[side][SHah].sqr_e =
															ps->score[side][SHhh].sqr_b =
																ps->score[side][SHhh].sqr_e =
																	ps->score[side][SHmh].sqr_b =
																		ps->score[side][SHmh].sqr_e =
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
					ps->score[side][ff].sqr_b += ps->t_sc[side][f][ff].sqr_b;
					ps->score[side][ff].sqr_e += ps->t_sc[side][f][ff].sqr_e;
					ff++;
				}
				from = ps->pawns[side][++f];
			}
		}

//propagate BAs into all variants
		for (side = 0; side <= 1; side++) {
			f = 0;
			from = ps->pawns[side][f];
			while (from != -1) {
				ff = 1;
				while (vars[ff] != -1) {
					ps->score[side][ff].sqr_b += ps->score[side][BAs].sqr_b;
					ps->score[side][ff].sqr_e += ps->score[side][BAs].sqr_e;
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

int eval_ind_attacks(board *b, king_eval *ke, personality *p, int side, int from)
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

int eval_king_checks_n_full(board *b, king_eval *ke, personality *p, int side)
{
	BITVAR d10, d20, r0, blk0, atk0, atk20, ar0, br0;
	BITVAR d11, d21, r1, blk1, atk1, atk21, ar1, br1;
	BITVAR d12, d22, r2, blk2, atk2, atk22, ar2, br2;
	BITVAR d13, d23, r3, blk3, atk3, atk23, ar3, br3;
	BITVAR r4, ar4, br4;
	BITVAR r5, ar5, br5;
	BITVAR r6, ar6, br6;
	BITVAR r7, ar7, br7;

	BITVAR di_att, di_block, cr_att, cr_block;
	int pos, o;

	pos = b->king[side];
	o = (side == 0) ? BLACK : WHITE;

	ke->ep_block = 0;
	ke->cr_pins = ke->cr_attackers = ke->cr_att_ray = 0;
	ke->di_pins = ke->di_attackers = ke->di_att_ray = 0;

// diagonal attackers
	di_att = b->colormaps[o] & (b->maps[QUEEN] | b->maps[BISHOP]);
// diagonal blockers
	di_block = b->norm & (~(di_att | (b->maps[KING] & b->colormaps[side])));
// to right up

	get45Rvector2(b->r45R, pos, &d11, &d21);
	r1 = attack.dirs[pos][1];
	r5 = attack.dirs[pos][5];
	atk1 = d11 & di_att;
	blk1 = d11 & di_block;
	atk21 = (d21 & (~d11)) & di_att;
	ar1 = atk1 & r1;
	br1 = blk1 & r1;
	ar5 = atk1 & r5;
	br5 = blk1 & r5;

	ke->di_attackers |= atk1;
	ke->di_att_ray |= (ar1 != 0) * (d11 & (~ar1));
	ke->di_pins |= (((atk21 & r1) != 0) & (br1 != 0)) * br1;

	ke->di_att_ray |= (ar5 != 0) * (d11 & (~ar5));
	ke->di_pins |= (((atk21 & r5) != 0) & (br5 != 0)) * br5;
	
	get45Lvector2(b->r45L, pos, &d13, &d23);
	r3 = attack.dirs[pos][3];
	r7 = attack.dirs[pos][7];
	atk3 = d13 & di_att;
	blk3 = d13 & di_block;
	atk23 = (d23 & (~d13)) & di_att;
	ar3 = atk3 & r3;
	br3 = blk3 & r3;
	ar7 = atk3 & r7;
	br7 = blk3 & r7;

	ke->di_attackers |= atk3;
	ke->di_att_ray |= (ar3 != 0) * (d13 & (~ar3));
	ke->di_pins |= (((atk23 & r3) != 0) & (br3 != 0)) * br3;

	ke->di_att_ray |= (ar7 != 0) * (d13 & (~ar7));
	ke->di_pins |= (((atk23 & r7) != 0) & (br7 != 0)) * br7;
	
// normal attackers
	cr_att = b->colormaps[o] & (b->maps[QUEEN] | b->maps[ROOK]);
// normal blocks
	cr_block = b->norm & (~(cr_att | (b->maps[KING] & b->colormaps[side])));

	getnormvector2(b->norm, pos, &d12, &d22);
	r2 = attack.dirs[pos][2];
	r6 = attack.dirs[pos][6];
	atk2 = d12 & cr_att;
	blk2 = d12 & cr_block;
	atk22 = (d22 & (~d12)) & cr_att;
	ar2 = atk2 & r2;
	br2 = blk2 & r2;
	ar6 = atk2 & r6;
	br6 = blk2 & r6;

	ke->cr_attackers |= atk2;
	ke->cr_att_ray |= (ar2 != 0) * (d12 & (~ar2));
	ke->cr_pins |= (((atk22 & r2) != 0) & (br2 != 0)) * br2;

	ke->cr_att_ray |= (ar6 != 0) * (d12 & (~ar6));
	ke->cr_pins |= (((atk22 & r6) != 0) & (br6 != 0)) * br6;
	
// pokud je 1 utocnik, tak ulozit utocnika a cestu do att_ray
// pokud je 1 prazdno, tak ulozit cestu do blocker_ray
// pokud je 1 blocker, tak ulozit cestu do blocker_ray
// pokud je 1 blocker a 2 utocnik, tak cestu do blocker_ray a blocker do PIN

	get90Rvector2(b->r90R, pos, &d10, &d20);
	r0 = attack.dirs[pos][0];
	r4 = attack.dirs[pos][4];
	atk0 = d10 & cr_att;
	blk0 = d10 & cr_block;
	atk20 = (d20 & (~d10)) & cr_att;
	ar0 = atk0 & r0;
	br0 = blk0 & r0;
	ar4 = atk0 & r4;
	br4 = blk0 & r4;

	ke->cr_attackers |= atk0;
	ke->cr_att_ray |= (ar0 != 0) * (d10 & (~ar0));
	ke->cr_pins |= (((atk20 & r0) != 0) & (br0 != 0)) * br0;

	ke->cr_att_ray |= (ar4 != 0) * (d10 & (~ar4));
	ke->cr_pins |= (((atk20 & r4) != 0) & (br4 != 0)) * br4;
	
// generating quiet check moves depends on di_blocker_ray and cr_blocker_ray containing all squares leading to king

// incorporate knights
	ke->kn_pot_att_pos = attack.maps[KNIGHT][pos];
	ke->kn_attackers = ke->kn_pot_att_pos & b->maps[KNIGHT] & b->colormaps[o];
//incorporate pawns
	ke->pn_pot_att_pos = attack.pawn_att[side][pos];
	ke->pn_attackers = ke->pn_pot_att_pos & b->maps[PAWN] & b->colormaps[o];
	ke->attackers = ke->cr_attackers | ke->di_attackers | ke->kn_attackers
		| ke->pn_attackers;

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
		&& (p->mat_info[b->mindex].info[b->side] == 0))
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
		if ((mt[i] > mt[op]) && (pp < 2)) {
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
		t->info[0] = tun[0];
		t->info[1] = tun[1];
	}
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

	for (q[1] = 0; q[1] < 2; q[1]++) {
		for (q[0] = 0; q[0] < 2; q[0]++) {
			for (r[1] = 0; r[1] < 3; r[1]++) {
				for (r[0] = 0; r[0] < 3; r[0]++) {
					for (bd[1] = 0; bd[1] < 2; bd[1]++) {
						for (bl[1] = 0; bl[1] < 2; bl[1]++) {
							for (bd[0] = 0; bd[0] < 2; bd[0]++) {
								for (bl[0] = 0; bl[0] < 2; bl[0]++) {
									for (n[1] = 0; n[1] < 3; n[1]++) {
										for (n[0] = 0; n[0] < 3; n[0]++) {
											for (p[1] = 0; p[1] < 9; p[1]++) {
												for (p[0] = 0; p[0] < 9;
														p[0]++) {
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
		if(rw) { ADD_STACKER(st, dvalues[ROOK][pp], rb, BAs, BLACK); 
			ADD_STACKER(st, Values[MG][ROOK], rb, BAs, BLACK)
			ADD_STACKER(st, Values[EG][ROOK], rb, BAs, BLACK)
			}
		if(qw) { ADD_STACKER(st, dvalues[QUEEN][pp], qb, BAs, BLACK); 
			ADD_STACKER(st, Values[MG][QUEEN], qb, BAs, BLACK)
			ADD_STACKER(st, Values[EG][QUEEN], qb, BAs, BLACK)
			}
		if(nw) { ADD_STACKER(st, dvalues[KNIGHT][pp], nb, BAs, BLACK); 
			ADD_STACKER(st, Values[MG][KNIGHT], nb, BAs, BLACK)
			ADD_STACKER(st, Values[EG][KNIGHT], nb, BAs, BLACK)
			}
		if(bwl+bwd) { ADD_STACKER(st, dvalues[BISHOP][pp], bbl+bbd, BAs, BLACK); 
			ADD_STACKER(st, Values[MG][BISHOP], bbl+bbd, BAs, BLACK)
			ADD_STACKER(st, Values[EG][BISHOP], bbl+bbd, BAs, BLACK)
			}
		if(pw) {
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
		a->sc.side[side].sqr_b += a->sq[from].sqr_b;
		a->sc.side[side].sqr_e += a->sq[from].sqr_e;
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
		a->sc.side[side].sqr_b += a->sq[from].sqr_b;
		a->sc.side[side].sqr_e += a->sq[from].sqr_e;
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
		a->sc.side[side].sqr_b += a->sq[from].sqr_b;
		a->sc.side[side].sqr_e += a->sq[from].sqr_e;
#ifdef TUNING
		ADD_STACKER(st, piecetosquare[MG][side][QUEEN][from], 1, BAs, side)
		ADD_STACKER(st, piecetosquare[EG][side][QUEEN][from], 1, BAs, side)
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
		a->sc.side[side].sqr_b += a->sq[from].sqr_b;
		a->sc.side[side].sqr_e += a->sq[from].sqr_e;

#ifdef TUNING
		ADD_STACKER(st, piecetosquare[MG][side][QUEEN][from], 1, BAs, side)
		ADD_STACKER(st, piecetosquare[EG][side][QUEEN][from], 1, BAs, side)
#endif

		z = getRank(from);
		if (z == srank) {
			a->specs[side][ROOK].sqr_b += p->rook_on_seventh[MG];
			a->specs[side][ROOK].sqr_e += p->rook_on_seventh[EG];
#ifdef TUNING
		ADD_STACKER(st, rook_on_seventh[MG], 1, BAs, side)
		ADD_STACKER(st, rook_on_seventh[EG], 1, BAs, side)
#endif
		}

		n = attack.file[from];
		if (n & ps->not_pawns_file[side] & ps->not_pawns_file[opside]) {
			a->specs[side][ROOK].sqr_b += p->rook_on_open[MG];
			a->specs[side][ROOK].sqr_e += p->rook_on_open[EG];
#ifdef TUNING
		ADD_STACKER(st, rook_on_open[MG], 1, BAs, side)
		ADD_STACKER(st, rook_on_open[EG], 1, BAs, side)
#endif
		} else if (n & ps->not_pawns_file[side]
			& (~ps->not_pawns_file[opside])) {
			a->specs[side][ROOK].sqr_b += p->rook_on_semiopen[MG];
			a->specs[side][ROOK].sqr_e += p->rook_on_semiopen[EG];
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

	if(p->use_heavy_material!=0)
		heavy_op = (b->maps[ROOK] | b->maps[QUEEN]) & b->colormaps[Flip(side)];
	else 
		heavy_op = 0;
		
	if (heavy_op) {
		a->sc.side[side].sqr_b += ps->score[side][HEa].sqr_b;
		a->sc.side[side].sqr_e += ps->score[side][HEa].sqr_e;
	} else {
		a->sc.side[side].sqr_b += ps->score[side][BAs].sqr_b;
		a->sc.side[side].sqr_e += ps->score[side][BAs].sqr_e;
	}
	return 0;
}

int eval_king2(board const *b, attack_model *a, PawnStore const *ps, int side, personality const *p, stacker *st)
{
	int from, m;
	int heavy_op;
	BITVAR mv;

	a->specs[side][KING].sqr_b = 0;
	a->specs[side][KING].sqr_e = 0;
	from = b->king[side];

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
		ADD_STACKER(st, piecetosquare[MG][side][KING][from], 1, BAs, side)
		ADD_STACKER(st, piecetosquare[EG][side][KING][from], 1, BAs, side)
#endif

	if(p->use_heavy_material!=0)
		heavy_op = (b->maps[ROOK] | b->maps[QUEEN]) & b->colormaps[Flip(side)];
	else
		heavy_op = 0;

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

#if 0
// evalute shelter
	sl=getFile(from);
	row=getRank(from);
	if(((side==WHITE)&&(row==0))||((side==BLACK)&&(row==7))) {
	  if(!heavy_op) {
// add KING specials for the side
		if((sl>=FILEiD)&&(sl<=FILEiF)) {
			a->specs[side][KING].sqr_b+=ps->score[side][SHm].sqr_b;
			a->specs[side][KING].sqr_e+=ps->score[side][SHm].sqr_e;
		} else if(sl<=FILEiC) {
			a->specs[side][KING].sqr_b+=ps->score[side][SHa].sqr_b;
			a->specs[side][KING].sqr_e+=ps->score[side][SHa].sqr_e;
		} else if(sl>=FILEiF) {
			a->specs[side][KING].sqr_b+=ps->score[side][SHh].sqr_b;
			a->specs[side][KING].sqr_e+=ps->score[side][SHh].sqr_e;
		}
	  } else {
		if((sl>=FILEiD)&&(sl<=FILEiF)) {
			a->specs[side][KING].sqr_b+=ps->score[side][SHmh].sqr_b;
			a->specs[side][KING].sqr_e+=ps->score[side][SHmh].sqr_e;
		} else if(sl<=FILEiC) {
			a->specs[side][KING].sqr_b+=ps->score[side][SHah].sqr_b;
			a->specs[side][KING].sqr_e+=ps->score[side][SHah].sqr_e;
		} else if(sl>=FILEiF) {
			a->specs[side][KING].sqr_b+=ps->score[side][SHhh].sqr_b;
			a->specs[side][KING].sqr_e+=ps->score[side][SHhh].sqr_e;
		}
	  }
	}
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
	make_mobility_modelN(b, a, p, st);

// build pawn mode + pawn cache + evaluate + pre compute pawn king shield
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

// just testing 

int eval(board const *b, attack_model *a, personality const *p, stacker *st)
{
	long score;
	int f;

	for (f = ER_PIECE; f >= PAWN; f--) {
		a->pos_c[f] = -1;
		a->pos_c[f | BLACKPIECE] = -1;
	}
	eval_lnk(b, a, ROOK, WHITE, ROOK);
	eval_lnk(b, a, ROOK, BLACK, ROOK + BLACKPIECE);
	eval_lnk(b, a, KNIGHT, WHITE, KNIGHT);
	eval_lnk(b, a, KNIGHT, BLACK, KNIGHT + BLACKPIECE);
	eval_lnk(b, a, BISHOP, WHITE, BISHOP);
	eval_lnk(b, a, BISHOP, BLACK, BISHOP + BLACKPIECE);
	eval_lnk(b, a, QUEEN, WHITE, QUEEN);
	eval_lnk(b, a, QUEEN, BLACK, QUEEN + BLACKPIECE);

#ifdef TUNING
	REINIT_STACKER(st)
#endif
	
	eval_x(b, a, p, st);
// here the stacker has all features recognised, in BAs scenario, other variants are on top of BAs

	a->sc.scaling = 128;
	
// scaling
	score = a->sc.score_nsc + p->eval_BIAS + p->eval_BIAS_e;
	if ((b->mindex_validity == 1)
		&& (((b->side == WHITE) && (score > 0))
			|| ((b->side == BLACK) && (score < 0)))) {
		a->sc.scaling = (p->mat_info[b->mindex].info[b->side]);
	}
	score = (score * a->sc.scaling) / 128;
	a->sc.complete = score / 255;

#if 0
			LOGGER_0("mat %d, mob %d, mob %d, sqr %d, sqr %d, spc %d, spc %d\n", a->sc.material,a->sc.side[0].mobi_b, a->sc.side[1].mobi_b, a->sc.side[0].sqr_b, a->sc.side[1].sqr_b, a->sc.side[0].specs_b, a->sc.side[1].specs_b );
			LOGGER_0("mat %d, mob %d, mob %d, sqr %d, sqr %d, spc %d, spc %d\n", a->sc.material_e,a->sc.side[0].mobi_e,a->sc.side[1].mobi_e,a->sc.side[0].sqr_e, a->sc.side[1].sqr_e, a->sc.side[0].specs_e, a->sc.side[1].specs_e );
			LOGGER_0("score %d, phase %d, score_b %d, score_e %d\n", a->sc.complete / 255, a->phase, a->sc.score_b, a->sc.score_e);
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
	if (((sc2 + p->lazy_eval_cutoff) < alfa)
		|| (sc2 > (beta + p->lazy_eval_cutoff))) {
		scr = sc2;
		LOGGER_4("score %d, mat %d, psq %d, alfa %d, beta %d, cutoff %d\n", sc2, sc4, sc3, alfa, beta, p->lazy_eval_cutoff);
	} else {
		*fullrun = 1;
		a->att_by_side[side] = KingAvoidSQ(b, a, side);
		eval_king_checks(b, &(a->ke[Flip(b->side)]), NULL, Flip(b->side));
		simple_pre_movegen_n2(b, a, Flip(side));
		simple_pre_movegen_n2(b, a, side);
		eval(b, a, b->pers, &st);
		scr = a->sc.complete;
	}
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
		collect_material_from_board(b, &pw, &pb, &nw, &nb, &bwl, &bwd, &bbl,
			&bbd, &rw, &rb, &qw, &qb);
		if ((qw > 1) || (qb > 1) || (nw > 2) || (nb > 2) || (bwd > 1)
			|| (bwl > 1) || (bbl > 1) || (bbd > 1) || (rw > 2) || (rb > 2)
			|| (pw > 8) || (pb > 8))
			return 0;
		b->mindex = MATidx(pw, pb, nw, nb, bwl, bwd, bbl, bbd, rw, rb, qw, qb);
		b->mindex_validity = 1;
	} else {
		b->mindex_validity = 0;
		qw = BitCount(b->maps[QUEEN] & b->colormaps[WHITE]);
		qb = BitCount(b->maps[QUEEN] & b->colormaps[BLACK]);
		if ((qw > 1) || (qb > 1))
			return 0;
		b->mindex_validity = 1;
	}
	return 1;
}

// move ordering is to get the fastest beta cutoff
int MVVLVA_gen(int table[ER_PIECE + 2][ER_PIECE], _values Values)
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
#endif

	return 0;
}

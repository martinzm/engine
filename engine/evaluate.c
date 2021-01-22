/*
 *
 * 
 *
 */
 

#include <stdlib.h>
#include <string.h>
#include "evaluate.h"
#include "movgen.h"
#include "attacks.h"
#include "bitmap.h"
#include "pers.h"
#include "utils.h"
#include "globals.h"
#include "assert.h"

uint8_t eval_phase(board *b, personality *p){
int i,i1,i2,i3,i4,i5, tot, faze, fz2, q;
int vaha[]={0,6,6,9,18};
int nc[]={16,4,4,4,2};

int bb, wb, be, we, stage;
int pw, pb, nw, nb, bwl, bwd, bbl, bbd, rw, rb, qw, qb;

// 255 -- pure beginning, 0 -- total ending
	if(b->mindex_validity==1) {
		faze=p->mat_faze[b->mindex];
	}
	else {
		tot=nc[PAWN]*vaha[PAWN]+nc[KNIGHT]*vaha[KNIGHT]+nc[BISHOP]*vaha[BISHOP]+nc[ROOK]*vaha[ROOK]+nc[QUEEN]*vaha[QUEEN];
		i1=BitCount(b->maps[PAWN])		*vaha[PAWN];
		i2=BitCount(b->maps[KNIGHT])	*vaha[KNIGHT];
		i3=BitCount(b->maps[BISHOP])	*vaha[BISHOP];
		i4=BitCount(b->maps[ROOK])		*vaha[ROOK];
		i5=BitCount(b->maps[QUEEN])		*vaha[QUEEN];
		i=i1+i2+i3+i4+i5;
		q=Min(i, tot);
		faze=q*255/tot;
	}
return (uint8_t)faze;
}



/*
 * vygenerujeme bitmapy moznych tahu pro N, B, R, Q dane strany
 */

int simple_pre_movegen(board *b, attack_model *a, int side)
{
int f, from, pp, st, en, add;
BITVAR x, q;

	if(side==BLACK) {
		st=ER_PIECE|BLACKPIECE;
		en=PAWN|BLACKPIECE;
		add=BLACKPIECE;
	} else {
		add=0;
		st=ER_PIECE;
		en=PAWN;
	}
	for(f=st;f>=en;f--) {
		a->pos_c[f]=-1;
	}
	q=0;

// rook
	x = (b->maps[ROOK]&b->colormaps[side]);
	pp=ROOK+add;
	while (x) {
		from = LastOne(x);
		a->pos_c[pp]++;
		a->pos_m[pp][a->pos_c[pp]]=from;
		q|=a->mvs[from] = (RookAttacks(b, from));
		ClrLO(x);
	}
// bishop
	x = (b->maps[BISHOP]&b->colormaps[side]);
	pp=BISHOP+add;
	while (x) {
		from = LastOne(x);
		a->pos_c[pp]++;
		a->pos_m[pp][a->pos_c[pp]]=from;
		q|=a->mvs[from] = (BishopAttacks(b, from));
		ClrLO(x);
	}
// knights
	x = (b->maps[KNIGHT]&b->colormaps[side]);
	pp=KNIGHT+add;
	while (x) {
		from = LastOne(x);
		a->pos_c[pp]++;
		a->pos_m[pp][a->pos_c[pp]]=from;
		q|=a->mvs[from]  = (attack.maps[KNIGHT][from]);
		ClrLO(x);
	}
// queen
	x = (b->maps[QUEEN]&b->colormaps[side]);
	pp=QUEEN+add;
	while (x) {
		from = LastOne(x);
		a->pos_c[pp]++;
		a->pos_m[pp][a->pos_c[pp]]=from;
		q|=a->mvs[from] = (QueenAttacks(b, from));
		ClrLO(x);
	}
// utoky pescu
	if(side==WHITE) a->pa_at[WHITE]=WhitePawnAttacks(b, a, &(a->pa_at_mo[WHITE]));
	else a->pa_at[BLACK]=BlackPawnAttacks(b, a, &(a->pa_at_mo[BLACK]));

	a->att_by_side[side]=q|a->pa_at[side];
return 0;
}

int PSQSearch(int from, int to, int piece, int side, int phase, personality *p)
{
int res, be, en;

uint8_t ph;
//	ph=(uint8_t) phase;
	be=p->piecetosquare[0][side][piece][to]-p->piecetosquare[0][side][piece][from];
	en=p->piecetosquare[1][side][piece][to]-p->piecetosquare[1][side][piece][from];
//	res=(en+be)/20;
	res=((be*phase+en*(255-phase))/255)/2;
//	score=mb*phase+me*(255-phase);
//	return score / 255;
#if defined (DEBUG4)
{
// PSQ analyzer
	if((res>=20)||(res<=-20)) {
		LOGGER_1("PSQS: %d, en:%d, be:%d\n",res, en, be);
	}
}
#endif
//	res=0;
	if(res>=499) res=499;
	else if(res<=-499) res=-499;
	res=0;
return res;
}


/* mobility = free safe squares
 * protect = free safe + friendly squares
 * unsafe = free safe + free but attacked by enemy pawns
 * mobility_protect==1 pocitam do poli pro mobilitu i moje figury
 * mobility_unsafe==1 pocitam i pole napadena nepratelskymi pesci
 *
 * builds attacks and evaluates mobility of pieces
 * other aspects of evaluation are handled in evaluate_PIECE functions
 *
 */

int make_mobility_model(board *b, attack_model *a, personality *p)
{
int from, pp, m, m2, s, z;
BITVAR x, q, v, n, a1[2], avoid[2], unsafe[2];

	avoid[WHITE]=~(b->norm|a->pa_at[BLACK]);
	avoid[BLACK]=~(b->norm|a->pa_at[WHITE]);
	unsafe[WHITE]=a->pa_at[BLACK];
	unsafe[BLACK]=a->pa_at[WHITE];

	if(p->mobility_protect==1) {
		avoid[WHITE]|=(b->colormaps[WHITE] & ~unsafe[WHITE]);
		avoid[BLACK]|=(b->colormaps[BLACK] & ~unsafe[BLACK]);
	}
	if(p->mobility_unsafe==1) {
		avoid[WHITE]|=(unsafe[WHITE] & ~b->colormaps[WHITE]);
		avoid[BLACK]|=(unsafe[BLACK] & ~b->colormaps[BLACK]);
	}
	if((p->mobility_unsafe==1)&&(p->mobility_protect==1)) {
		avoid[WHITE]|=~b->colormaps[BLACK];
		avoid[BLACK]|=~b->colormaps[WHITE];
	}

// rook
	x = (b->maps[ROOK]);
	while (x) {
		from = LastOne(x);
		pp=b->pieces[from];
		s=(pp&BLACKPIECE)!=0;
		a->pos_c[pp]++;
		a->pos_m[pp][a->pos_c[pp]]=from;
		q=a->mvs[from] = (RookAttacks(b, from));
		a->att_by_side[s]|=q;
		m=a->me[from].pos_att_tot=BitCount(q & avoid[s]);
		m2=BitCount(q & avoid[s] & unsafe[s]);
		a->me[from].pos_mob_tot_b=p->mob_val[0][s][ROOK][m-m2];
		a->me[from].pos_mob_tot_e=p->mob_val[1][s][ROOK][m-m2];
		if(p->mobility_unsafe==1) {
			a->me[from].pos_mob_tot_b+=p->mob_uns[0][s][ROOK][m2];
			a->me[from].pos_mob_tot_e+=p->mob_uns[1][s][ROOK][m2];
		}
		ClrLO(x);
	}
// bishop
	x = (b->maps[BISHOP]);
	while (x) {
		from = LastOne(x);
		pp=b->pieces[from];
		s=(pp&BLACKPIECE)>>3;
		a->pos_c[pp]++;
		a->pos_m[pp][a->pos_c[pp]]=from;
		q=a->mvs[from] = (BishopAttacks(b, from));
		a->att_by_side[s]|=q;
		m=a->me[from].pos_att_tot=BitCount(q & avoid[s]);
		m2=BitCount(q & avoid[s] & unsafe[s]);
		a->me[from].pos_mob_tot_b=p->mob_val[0][s][BISHOP][m-m2];
		a->me[from].pos_mob_tot_e=p->mob_val[1][s][BISHOP][m-m2];
		if(p->mobility_unsafe==1) {
			a->me[from].pos_mob_tot_b+=p->mob_uns[0][s][BISHOP][m2];
			a->me[from].pos_mob_tot_e+=p->mob_uns[1][s][BISHOP][m2];
		}
		ClrLO(x);
	}
// knights
	x = (b->maps[KNIGHT]);
	while (x) {
		from = LastOne(x);
		pp=b->pieces[from];
		s=(pp&BLACKPIECE)>>3;
		a->pos_c[pp]++;
		a->pos_m[pp][a->pos_c[pp]]=from;
		q=a->mvs[from]  = (attack.maps[KNIGHT][from]);
		a->att_by_side[s]|=q;
		m=a->me[from].pos_att_tot=BitCount(q & avoid[s]);
		m2=BitCount(q & avoid[s] & unsafe[s]);
		a->me[from].pos_mob_tot_b=p->mob_val[0][s][KNIGHT][m-m2];
		a->me[from].pos_mob_tot_e=p->mob_val[1][s][KNIGHT][m-m2];
		if(p->mobility_unsafe==1) {
			a->me[from].pos_mob_tot_b+=p->mob_uns[0][s][KNIGHT][m2];
			a->me[from].pos_mob_tot_e+=p->mob_uns[1][s][KNIGHT][m2];
		}
		ClrLO(x);
	}
// queen
	x = (b->maps[QUEEN]);
	while (x) {
		from = LastOne(x);
		pp=b->pieces[from];
		s=(pp&BLACKPIECE)>>3;
		a->pos_c[pp]++;
		a->pos_m[pp][a->pos_c[pp]]=from;
		q=a->mvs[from] = (QueenAttacks(b, from));
		a->att_by_side[s]|=q;
		m=a->me[from].pos_att_tot=BitCount(q & avoid[s]);
		m2=BitCount(q & avoid[s] & unsafe[s]);
		a->me[from].pos_mob_tot_b=p->mob_val[0][s][QUEEN][m-m2];
		a->me[from].pos_mob_tot_e=p->mob_val[1][s][QUEEN][m-m2];
		if(p->mobility_unsafe==1) {
			a->me[from].pos_mob_tot_b+=p->mob_uns[0][s][QUEEN][m2];
			a->me[from].pos_mob_tot_e+=p->mob_uns[1][s][QUEEN][m2];
		}
		ClrLO(x);
	}
return 0;
}

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

int analyze_pawn(board *b, attack_model *a, PawnStore *ps, int side, personality *p) {
int opside;

BITVAR dir;
BITVAR temp, t2;
int file, rank, tt1, tt2, from, f;

	opside = (side == WHITE) ? BLACK : WHITE;

// iterate pawns
	f=0;
	from=ps->pawns[side][f];
	while(from!=-1) {
		file=getFile(from);
		rank=getRank(from);
// pawns on the file
		ps->not_pawns_file[side]&=(~attack.file[from]);
		dir=ps->spans[side][f][0];
// passer
		ps->pas_d[side][f]=8;
		ps->double_d[side][f]=8;
		ps->block_d[side][f]=8;
		ps->stop_d[side][f]=8;
		ps->prot_d[side][f]=8;
		ps->prot_p_d[side][f]=8;
		ps->prot_dir_d[side][f]=0;
		ps->outp[side][f]=8;
		ps->outp_d[side][f]=8;

		if((dir&ps->pass_end[side])) {
			ps->pas_d[side][f]=BitCount(dir)-1;
			ps->passer[side]|=normmark[from];
			assert((ps->pas_d[side][f]<8) &&(ps->pas_d[side][f]>=0));
		} else {
			if(dir & ps->path_stop[side]&(b->maps[PAWN])) {
				if(dir & ps->path_stop[side]&(b->maps[PAWN])&b->colormaps[side]) {
// doubled
					ps->doubled[side]|=normmark[from];
					ps->double_d[side][f]=BitCount(dir)-1;
					assert((ps->double_d[side][f]<8) &&(ps->double_d[side][f]>=0));
				} else {
// blocked
					ps->blocked[side]|=normmark[from];
					ps->block_d[side][f]=BitCount(dir)-1;
					assert((ps->block_d[side][f]<8) &&(ps->block_d[side][f]>=0));
				}
			} else 	if(dir & ps->path_stop[side]&(ps->half_att[opside][0]|ps->half_att[opside][1])) {
// stopped
					ps->stopped[side]|=normmark[from];
					ps->stop_d[side][f]=BitCount(dir)-1;
					assert((ps->stop_d[side][f]<8) &&(ps->stop_d[side][f]>=0));
				}
		}
// can I be protected?
		temp= (side == WHITE) ? (attack.pawn_surr[from]&(~(attack.uphalf[from]|attack.file[from]))) :
				 (attack.pawn_surr[from]&(~(attack.downhalf[from]|attack.file[from])));

		if((temp&b->maps[PAWN]&b->colormaps[side])==0) {
			if(temp&ps->paths[side]) {
// somebody from behing can reach me
				ps->prot[side]|=normmark[from];
				t2=temp&ps->paths[side];
				while(t2) {
					tt1=LastOne(t2);
					tt2=getRank(tt1);
					if(side==WHITE) {
						if((rank-tt2)<ps->prot_p_d[side][f]) {
							ps->prot_p_d[side][f]=(rank-tt2);
							assert((ps->prot_p_d[side][f]<8) &&(ps->prot_p_d[side][f]>=0));
						}
					} else {
						if((tt2-rank)<ps->prot_p_d[side][f]) {
							ps->prot_p_d[side][f]=(tt2-rank);
							assert((ps->prot_p_d[side][f]<8) &&(ps->prot_p_d[side][f]>=0));
						}
					}
					ClrLO(t2);
				}
				assert((ps->prot_p_d[side][f]<8) &&(ps->prot_p_d[side][f]>=0));
			}
			temp=0;
			if(file>0) temp|=((dir & ps->paths[side])>>1);
			if(file<7) temp|=((dir & ps->paths[side])<<1);
			if(temp&ps->paths[side]&b->maps[PAWN]&b->colormaps[WHITE]) {
// I can reach somebody	
				ps->prot[side]|=normmark[from];
				t2=temp&ps->paths[side]&b->maps[PAWN]&b->colormaps[WHITE];
				while(t2) {
					tt1=LastOne(t2);
					tt2=getRank(tt1);
					if((tt2-rank)<ps->prot_d[side][f]) {
						ps->prot_d[side][f]=(tt2-rank);
						assert((ps->prot_d[side][f]<8) &&(ps->prot_d[side][f]>=0));
					}
					ClrLO(t2);
				}
				
				assert((ps->prot_d[side][f]<8) &&(ps->prot_d[side][f]>=0));
				
			} else {
// i cannot be protected, so backward
//			    ps->back[side]|=normmark[from];
			}
		} else {
// I am 
			ps->prot[side]|=normmark[from];
			ps->prot_d[side][f]=0;
			ps->prot_dir_d[side][f]=BitCount(temp&b->maps[PAWN]&b->colormaps[side]);
			assert((ps->prot_dir_d[side][f]<8) &&(ps->prot_dir_d[side][f]>=0));
		}
// isolated
		if(file>0) {
			if((attack.rays[A1+file-1][A8+file-1] & (b->maps[PAWN]&b->colormaps[side]))==0) {
			    ps->half_isol[side][0]|=normmark[from];
			}
		}
		if(file<7) {
			if((attack.rays[A1+file+1][A8+file+1] & (b->maps[PAWN]&b->colormaps[side]))==0) {
			    ps->half_isol[side][1]|=normmark[from];
			}
		}
		f++;
		from=ps->pawns[side][f];
	}

// if one_s_att square can be reached, in how many pawn moves?

	return 0;
}


// doubled
// isolated
// 
// open file
// half open file
// 1st defense line
// 2nd defense line	

int analyze_pawn_shield_single(board *b, attack_model *a, PawnStore *ps, int side, BITVAR mask, int *beg, int *end, personality *p){
BITVAR x;
int l, opside;

	opside = (side == WHITE) ? BLACK : WHITE;
	*beg=0;
	*end=0;
	x=(b->maps[PAWN]&b->colormaps[side]&mask);
	l=BitCount(x&ps->half_isol[side][0])+BitCount(ps->half_isol[side][1]);
	(*beg)+=(p->pshelter_isol_penalty[0]*l);
	(*end)+=(p->pshelter_isol_penalty[1]*l);

	l=BitCount(x&ps->doubled[side]);
	(*beg)+=(p->pshelter_double_penalty[0]*l);
	(*end)+=(p->pshelter_double_penalty[1]*l);

	l=BitCount(mask&ps->not_pawns_file[side]&ps->not_pawns_file[opside])/2;
	(*beg)+=(p->pshelter_open_penalty[0]*l);
	(*end)+=(p->pshelter_open_penalty[1]*l);

	l=BitCount(mask&ps->not_pawns_file[side]&(~ps->not_pawns_file[opside]))/2;
	(*beg)+=(p->pshelter_hopen_penalty[0]*l);
	(*end)+=(p->pshelter_hopen_penalty[1]*l);

	l=BitCount(x&(RANK2 | RANK7));
	(*beg)+=(p->pshelter_prim_bonus[0]*l);
	(*end)+=(p->pshelter_prim_bonus[1]*l);

	l=BitCount(x&(RANK3 | RANK6));
	(*beg)+=(p->pshelter_sec_bonus[0]*l);
	(*end)+=(p->pshelter_sec_bonus[1]*l);
return 0;
}

int analyze_pawn_shield(board *b, attack_model *a, PawnStore *ps, personality *p) {
int f;
int i,l;
BITVAR x;
BITVAR mask[]= { SHELTERA2, SHELTERH2, SHELTERM2, SHELTERA7, SHELTERH7, SHELTERM7 };
	analyze_pawn_shield_single(b, a, ps, WHITE, SHELTERA2, &(ps->shelter_a[WHITE].sqr_b), &(ps->shelter_a[WHITE].sqr_e), p);
	analyze_pawn_shield_single(b, a, ps, WHITE, SHELTERH2, &(ps->shelter_h[WHITE].sqr_b), &(ps->shelter_h[WHITE].sqr_e), p);
	analyze_pawn_shield_single(b, a, ps, WHITE, SHELTERM2, &(ps->shelter_m[WHITE].sqr_b), &(ps->shelter_h[WHITE].sqr_e), p);
	analyze_pawn_shield_single(b, a, ps, BLACK, SHELTERA7, &(ps->shelter_a[BLACK].sqr_b), &(ps->shelter_a[BLACK].sqr_e), p);
	analyze_pawn_shield_single(b, a, ps, BLACK, SHELTERH7, &(ps->shelter_h[BLACK].sqr_b), &(ps->shelter_h[BLACK].sqr_e), p);
	analyze_pawn_shield_single(b, a, ps, BLACK, SHELTERM7, &(ps->shelter_m[BLACK].sqr_b), &(ps->shelter_h[BLACK].sqr_e), p);			
return 0;
}

int pre_evaluate_pawns(board *b, attack_model *a, PawnStore *ps, personality *p)
{
int f, ff, file, n, i, from, to, rank, sq_file[8];
int tt, tt1, tt2, side, opside;
BITVAR ss1, ss2, dir, ppp;
BITVAR temp, t2, x;

	ps->score[WHITE].sqr_b=0;
	ps->score[WHITE].sqr_e=0;
	ps->score[BLACK].sqr_b=0;
	ps->score[BLACK].sqr_e=0;

	for(f=0;f<8;f++) {
		ps->t_sc[WHITE][f].sqr_b=0;
		ps->t_sc[WHITE][f].sqr_e=0;
		ps->t_sc[BLACK][f].sqr_b=0;
		ps->t_sc[BLACK][f].sqr_e=0;
	}

	for(side=0;side<=1;side++) {
		opside = (side==0) ? BLACK : WHITE;
		f=0;
		from=ps->pawns[side][f];
		while(from!=-1) {
			x=normmark[from];
// PSQ
			ps->t_sc[side][f].sqr_b=p->piecetosquare[0][side][PAWN][from];
			ps->t_sc[side][f].sqr_e=p->piecetosquare[1][side][PAWN][from];

// if simple_EVAL then only material and PSQ are used
			if(p->simple_EVAL==0) {
// isolated
				if((ps->half_isol[side][0] || (ps->half_isol[side][1]))&x) {
					if(ps->half_isol[side][0]&x) {
						ps->t_sc[side][f].sqr_b+=p->isolated_penalty[0];
						ps->t_sc[side][f].sqr_e+=p->isolated_penalty[1];
					}
					if(ps->half_isol[side][1]&x) {
						ps->t_sc[side][f].sqr_b+=p->isolated_penalty[0];
						ps->t_sc[side][f].sqr_e+=p->isolated_penalty[1];
					}
					if(x&CENTEREXBITMAP) {
						ps->t_sc[side][f].sqr_b+=p->pawn_iso_center_penalty[0];
						ps->t_sc[side][f].sqr_e+=p->pawn_iso_center_penalty[1];
					}
					if(x&ps->not_pawns_file[opside]) {
						ps->t_sc[side][f].sqr_b+=p->pawn_iso_center_penalty[0];
						ps->t_sc[side][f].sqr_e+=p->pawn_iso_center_penalty[1];
					}
				}
// backward
				if(ps->back[side]&x) {
					ps->t_sc[side][f].sqr_b+=p->backward_penalty[0];
					ps->t_sc[side][f].sqr_e+=p->backward_penalty[1];
				}
// blocked
				if(ps->block_d[side][f]<8) {
					ps->t_sc[side][f].sqr_b+=p->pawn_blocked_penalty[0][side][ps->block_d[side][f]];
					ps->t_sc[side][f].sqr_e+=p->pawn_blocked_penalty[1][side][ps->block_d[side][f]];
				}
// stopped
				if(ps->stop_d[side][f]<8) {
					ps->t_sc[side][f].sqr_b+=p->pawn_stopped_penalty[0][side][ps->stop_d[side][f]];
					ps->t_sc[side][f].sqr_e+=p->pawn_stopped_penalty[1][side][ps->stop_d[side][f]];
				}
// doubled
				if(ps->double_d[side][f]<8) {
					ps->t_sc[side][f].sqr_b+=p->doubled_n_penalty[0][side][ps->double_d[side][f]];
					ps->t_sc[side][f].sqr_e+=p->doubled_n_penalty[1][side][ps->double_d[side][f]];
				}
// protected
				if(ps->prot_d[side][f]<8) {
					ps->t_sc[side][f].sqr_b+=p->pawn_n_protect[0][side][ps->prot_d[side][f]];
					ps->t_sc[side][f].sqr_e+=p->pawn_n_protect[1][side][ps->prot_d[side][f]];
				}
				if(ps->prot_p_d[side][f]<8) {
					ps->t_sc[side][f].sqr_b+=p->pawn_pot_protect[0][side][ps->prot_p_d[side][f]];
					ps->t_sc[side][f].sqr_e+=p->pawn_pot_protect[1][side][ps->prot_p_d[side][f]];
				}
// directly protected
				ps->t_sc[side][f].sqr_b+=p->pawn_dir_protect[0][side][ps->prot_dir_d[side][f]];
				ps->t_sc[side][f].sqr_e+=p->pawn_dir_protect[1][side][ps->prot_dir_d[side][f]];
// potential passer ?
				if(ps->pas_d[side][f]<8) {
					ps->t_sc[side][f].sqr_b+=p->passer_bonus[0][side][ps->pas_d[side][f]];
					ps->t_sc[side][f].sqr_e+=p->passer_bonus[1][side][ps->pas_d[side][f]];
				}
// weak in center
				if(x&CENTEREXBITMAP) {
					ps->t_sc[side][f].sqr_b+=p->pawn_weak_center_penalty[0];
					ps->t_sc[side][f].sqr_e+=p->pawn_weak_center_penalty[1];
				}
// fix material value
				if(x&(FILEA|FILEH)) {
					ps->t_sc[side][f].sqr_b+=p->pawn_ah_penalty[0];
					ps->t_sc[side][f].sqr_e+=p->pawn_ah_penalty[1];
				}
// weak on open file ?
				if(x&ps->back[side]&ps->not_pawns_file[opside]) {
					ps->t_sc[side][f].sqr_b+=p->pawn_weak_onopen_penalty[0];
					ps->t_sc[side][f].sqr_e+=p->pawn_weak_onopen_penalty[1];
				}
// mobility
				ff=BitCount(a->pa_mo[side])+BitCount(a->pa_at[side]);
				ps->t_sc[side][f].sqr_b+=p->mob_val[0][side][PAWN][0]*ff;
				ps->t_sc[side][f].sqr_e+=p->mob_val[1][side][PAWN][1]*ff;
			}
/*
 *		if pawn is under attack from pawn we should react
 *		- not count any bonuses?
 *		- count half of them?
 *		- nothing?
 */			
//			if((x&ps->safe_att[WHITE])==0) {
//					ps->score[side].sqr_b+=(ps->t_sc[side][f].sqr_b/2);
//					ps->score[side].sqr_e+=(ps->t_sc[side][f].sqr_e/2);
//			} else {
					ps->score[side].sqr_b+=ps->t_sc[side][f].sqr_b;
					ps->score[side].sqr_e+=ps->t_sc[side][f].sqr_e;
//			}

			f++;
			from=ps->pawns[side][f];
		}
	}
	return 0;
}

/*
 * evaluation should be called in quiet situation, so no attacks should be on the board
 * but in the case... if pawn is attacking pawn there should be some penalty 
 * or some other mechanism like not counting bonuses for attacked pawns
 */

int premake_pawn_model(board *b, attack_model *a, PawnStore *ps, personality *p) {

int f, ff, file, n, i, from, to, rank, sq_file[8], f1, f2;
int tt, tt1, tt2, side, opside;
BITVAR ss1, ss2, dir, ppp;
BITVAR temp, t2, x;

hashPawnEntry hash, h2;
int hret;

//PawnStore psx, *ps;
//	ps=&psx;

	hash.key=b->pawnkey;
	hash.map=b->maps[PAWN];
	hret=-1;

	if(b->hps!=NULL) hret=retrievePawnHash(b->hps, &hash, b->stats);
	if(hret!=1) {

		// attacks halves
		ps->half_att[WHITE][1]=(((b->maps[PAWN]&b->colormaps[WHITE])&(~(FILEH | RANK8)))<<9);
		ps->half_att[WHITE][0]=(((b->maps[PAWN]&b->colormaps[WHITE])&(~(FILEA | RANK8)))<<7);
		ps->half_att[BLACK][0]=(((b->maps[PAWN]&b->colormaps[BLACK])&(~(FILEH | RANK1)))>>7);
		ps->half_att[BLACK][1]=(((b->maps[PAWN]&b->colormaps[BLACK])&(~(FILEA | RANK1)))>>9);

		//double attacked
		ps->double_att[WHITE]=ps->half_att[WHITE][0] & ps->half_att[WHITE][1];
		ps->double_att[BLACK]=ps->half_att[BLACK][0] & ps->half_att[BLACK][1];

		// single attacked
		ps->odd_att[WHITE]=ps->half_att[WHITE][0] ^ ps->half_att[WHITE][1];
		ps->odd_att[BLACK]=ps->half_att[BLACK][0] ^ ps->half_att[BLACK][1];

		// squares properly defended
		ps->safe_att[WHITE]=(ps->double_att[WHITE] | ~(ps->half_att[BLACK][0]|ps->half_att[BLACK][1]) | (ps->odd_att[WHITE] & ~ps->double_att[BLACK]));
		ps->safe_att[BLACK]=(ps->double_att[BLACK] | ~(ps->half_att[WHITE][0]|ps->half_att[WHITE][1]) | (ps->odd_att[BLACK] & ~ps->double_att[WHITE]));

		// safe paths
		ps->paths[WHITE]=FillNorth(b->maps[PAWN]&b->colormaps[WHITE], ps->safe_att[WHITE]& ~b->maps[PAWN], 0);
		ps->paths[BLACK]=FillSouth(b->maps[PAWN]&b->colormaps[BLACK], ps->safe_att[BLACK]& ~b->maps[PAWN], 0);

		// paths including stops
		ps->path_stop[WHITE]=ps->paths[WHITE]|(ps->paths[WHITE])<<8;
		ps->path_stop[BLACK]=ps->paths[BLACK]|(ps->paths[BLACK])>>8;
		// stops only
		ps->path_stop2[WHITE]=ps->path_stop[WHITE]^ps->paths[WHITE];
		ps->path_stop2[BLACK]=ps->path_stop[BLACK]^ps->paths[BLACK];

		// is path up to promotion square?
		ps->pass_end[WHITE]=ps->paths[WHITE] & attack.rank[A8];
		ps->pass_end[BLACK]=ps->paths[BLACK] & attack.rank[A1];

		/*
		 * holes/outpost (in enemy pawns) - squares covered by my pawns only
		 * but not reachable by enemy pawns - for my minor pieces. In center or opponent half of board.
		 */
		// squares attacked by my pawns only
		ps->one_side[WHITE] = ((ps->half_att[WHITE][0]|ps->half_att[WHITE][1])&(~(ps->half_att[BLACK][0]|ps->half_att[BLACK][1])));
		ps->one_side[BLACK] = ((ps->half_att[BLACK][0]|ps->half_att[BLACK][1])&(~(ps->half_att[WHITE][0]|ps->half_att[WHITE][1])));

		// pawn attacks from hole/outpost for analysing opponent pawn reachability
		ps->one_s_att[WHITE][1]=(((ps->one_side[WHITE])&(~(FILEH | RANK8)))<<9);
		ps->one_s_att[WHITE][0]=(((ps->one_side[WHITE])&(~(FILEA | RANK8)))<<7);
		ps->one_s_att[BLACK][0]=(((ps->one_side[BLACK])&(~(FILEH | RANK1)))>>7);
		ps->one_s_att[BLACK][1]=(((ps->one_side[BLACK])&(~(FILEA | RANK1)))>>9);

		// front & back spans
		for(f=0;f<8;f++) {
			ps->spans[WHITE][f][0]=ps->spans[BLACK][f][0]=ps->spans[WHITE][f][1]=ps->spans[BLACK][f][1]=EMPTYBITMAP;
		}

		// iterate pawns by files, serialize
		f1=f2=0;
		for(file=0;file<8;file++) {
			temp=attack.file[A1+file];
			x = b->maps[PAWN]&b->colormaps[WHITE]&temp;
			i=0;
			while (x) {
				n=LastOne(x);
				ps->pawns[WHITE][f1]=n;
				sq_file[i++]=n;
				f1++;
				ClrLO(x);
			}
			x = b->maps[PAWN]&b->colormaps[BLACK]&temp;
			while (x) {
				n=LastOne(x);
				ps->pawns[BLACK][f2]=n;
				sq_file[i++]=n;
				f2++;
				ClrLO(x);
			}

			// sort pawns on file
			// i has number of pawns on file
			for(n=i;n>1;n--) {
				for(f=1;f<n;f++) {
					if(getRank(sq_file[f])<getRank(sq_file[f-1])) {
						tt=sq_file[f-1];
						sq_file[f-1]=sq_file[f];
						sq_file[f]=tt;
					}
				}
			}
			if(i>0) {
				// get pawns on file and assign them spans
				for(f=0;f<i;f++){
					tt=sq_file[f];
					if(f==0) tt1=getPos(file,0); else tt1=sq_file[f-1];
					if(f==(i-1)) tt2=getPos(file,7); else tt2=sq_file[f+1];
					ss1=attack.rays[tt][tt2]&(~normmark[tt]);
					ss2=attack.rays[tt][tt1]&(~normmark[tt]);
					ff=0;
					if(normmark[tt]&b->colormaps[WHITE]) {
						while((ps->pawns[WHITE][ff]!=tt)) {
							ff++;
						}
						assert(ps->pawns[WHITE][ff]==tt);
						ps->spans[WHITE][ff][0]=ss1;
						ps->spans[WHITE][ff][1]=ss2;
					} else {
						while((ps->pawns[BLACK][ff]!=tt)) {
							ff++;
						}
						assert(ps->pawns[BLACK][ff]==tt);
						ps->spans[BLACK][ff][0]=ss2;
						ps->spans[BLACK][ff][1]=ss1;
					}
				}
			}
			ps->pawns[WHITE][f1]=-1;
			ps->pawns[BLACK][f2]=-1;
		}

		ps->stopped[WHITE]=ps->passer[WHITE]=ps->blocked[WHITE]=ps->isolated[WHITE]=ps->doubled[WHITE]=ps->back[WHITE]=EMPTYBITMAP;
		ps->stopped[BLACK]=ps->passer[BLACK]=ps->blocked[BLACK]=ps->isolated[BLACK]=ps->doubled[BLACK]=ps->back[BLACK]=EMPTYBITMAP;

		ps->half_isol[WHITE][0]=ps->half_isol[WHITE][1]=ps->half_isol[BLACK][0]=ps->half_isol[BLACK][1]=EMPTYBITMAP;
		ps->prot[WHITE]=ps->prot[BLACK]=ps->prot_p[WHITE]=ps->prot_p[BLACK]=0;
		ps->not_pawns_file[WHITE]=ps->not_pawns_file[BLACK]=FULLBITMAP;

		analyze_pawn(b, a, ps, WHITE, p);
		analyze_pawn(b, a, ps, BLACK, p);

		// compute scores that are only pawn related
		pre_evaluate_pawns(b, a, ps, p);
		analyze_pawn_shield(b, a, ps, p);
		hash.value=*ps;
		if((b->hps!=NULL)&&(hret!=1)) storePawnHash(b->hps, &hash, b->stats);
	} else {
		*ps=hash.value;
	}
	return 0;
}

/*
 * passer_bonus - pawn cannot be stopped by opposite pawns up to promotion
 * doubled_penalty - more pawns on a file
 * pawn_blocked_penalty - opposite pawn stands in way
 * pawn_stopped_penalty - on the way square is attacked by more opposite pawns than by own pawns
 * pawn_protect - pawn has protection from other pawns
 * pawn_weak_onopen_penalty - unprotected pawn on open file 
 * pawn_weak_center_penalty - unprotected pawn at center files
 * isolated_penalty - no own pawns on surrounding files
 * backward_penalty - no longer defensible by own pawns and stop is attacked by opposite pawns
 * backward_fix_penalty - 
 * pawn_ah_penalty - AH files material fix
 * mob_val - num of moves available (capture+move)
 */
 
/*
 * Vygenerujeme vsechny co utoci na krale
 * vygenerujeme vsechny PINy - tedy ty kteri blokuji utok na krale
 * vygenerujeme vsechny RAYe utoku na krale
 */

int eval_king_checks(board *b, king_eval *ke, personality *p, int side)
{
BITVAR cr2, di2, c2, d2, c, d, c3, d3, ob, c2s, d2s, c3s, bl_ray;

int from, ff, o, ee;
BITVAR pp;

		from=b->king[side];

		o= (side==0) ? BLACK:WHITE;

// find potential attackers - get rays, and check existence of them
		cr2=di2=0;
// vert/horiz rays
		c =ke->cr_all_ray = attack.maps[ROOK][from];
// vert/horiz attackers
		c2=c2s=c & (b->maps[ROOK]|b->maps[QUEEN])&(b->colormaps[o]);
// diag rays
		d =ke->di_all_ray = attack.maps[BISHOP][from];
// diag attackers
		d2=d2s=d & (b->maps[BISHOP]|b->maps[QUEEN])&(b->colormaps[o]);

// if it can hit king, find nearest piece, blocker?
// pins might contain false as there can be other piece between pin and distant attacker
// rook/queen
		ke->cr_pins = 0;
		ke->cr_attackers = 0;
		ke->cr_blocks = 0;
		ke->cr_att_ray = 0;
		ke->cr_blocker_ray = 0;
		
// iterate attackers
			while(c2) {
				ff = LastOne(c2);
// get line between square and attacker
				cr2=attack.rays_int[from][ff];
// check if there is piece in that line, that blocks the attack
				c3=cr2 & b->norm;
				if((c3 & c2s)==0) {
// determine status
					switch (BitCount(c3)) {
// just 1 means pin
					case 1:
						ee = LastOne(c3);
						ke->blocker_ray[ee]=(cr2|normmark[ff]);
						ke->cr_pins |=c3;
						bl_ray=attack.rays_int[from][ee]|normmark[ee];
						ke->cr_blocker_ray|=(bl_ray);
						ke->cr_blocks|=normmark[ee];
						break;
// 0 means attacked
					case 0:
						ke->cr_attackers |= normmark[ff];
						ke->cr_att_ray|=attack.rays_dir[ff][from];
						break;
					case 2:
// 2 means no attack no pin, with one exception
// pawn can be subject of e.p. In that case 2 pawns is just one blocker
						pp=c3&b->maps[PAWN]&attack.rank[from];
// obe figury jsou pesci?
						if((!(pp^c3)) && (b->ep!=-1)) {
						BITVAR aa;
							aa=(attack.ep_mask[b->ep])&b->colormaps[o];
							ob=(c3 & normmark[b->ep])&b->colormaps[side];
							if((aa!=0)&&(ob!=0)) {
								ke->cr_pins |=c3;
								ke->cr_blocker_ray|=(cr2|normmark[ff]);
							}
						}
						break;
					default:
// more than 2 means no attack no pin
						break;
					}
				}
				ClrLO(c2);
		}

// bishop/queen
		ke->di_pins = 0;
		ke->di_attackers = 0;
		ke->di_blocks = 0;
		ke->di_att_ray = 0;
		ke->di_blocker_ray = 0;
			while(d2) {
				ff = LastOne(d2);
				di2=attack.rays_int[from][ff];
				d3=di2 & b->norm;
				if((d3 & d2s)==0) {
					switch (BitCount(d3)) {
					case 1:
						ee = LastOne(d3);
						ke->blocker_ray[ee]=(di2|normmark[ff]);
						ke->di_pins |=d3;
//???
						bl_ray=attack.rays_int[from][ee]|normmark[ee];
						ke->di_blocker_ray|=(bl_ray);
						ke->di_blocks|=normmark[ee];
						break;
					case 0:
						ke->di_attackers |= normmark[ff];
						ke->di_att_ray|=attack.rays_dir[ff][from];
						break;
					}
				}
				ClrLO(d2);
			}

#if 1
		c2=(((BOARDEDGEF&c&attack.rank[from])|(BOARDEDGER&c&attack.file[from]))&(~b->norm));
		if(c2){
			while(c2) {
				ff = LastOne(c2);
				cr2=attack.rays_int[from][ff];
				c3=cr2 & b->norm;
				if(c3==0) {
					ke->cr_blocker_ray|=(cr2|normmark[ff]);
				}
				ClrLO(c2);
			}
		}
		d2=(BOARDEDGE&d&(~b->norm));
		if(d2){
			while(d2) {
				ff = LastOne(d2);
				di2=attack.rays_int[from][ff];
				d3=di2 & b->norm;
				if(d3==0) {
					ke->di_blocker_ray|=(di2|normmark[ff]);
				}
				ClrLO(d2);
			}
		}

#endif
// generating quiet check moves depends on di_blocker_ray and cr_blocker_ray containing all squares leading to king
// which now is not for completely empty path from king to edge of board

// incorporate knights
		ke->kn_pot_att_pos=attack.maps[KNIGHT][from];
		ke->kn_attackers=ke->kn_pot_att_pos & b->maps[KNIGHT] & b->colormaps[o];
//incorporate pawns
		ke->pn_pot_att_pos=attack.pawn_att[side][from];
		ke->pn_attackers=ke->pn_pot_att_pos & b->maps[PAWN] & b->colormaps[o];
		ke->attackers=ke->cr_attackers | ke->di_attackers | ke->kn_attackers | ke->pn_attackers;
	return 0;
}

int eval_king_checks_all(board *b, attack_model *a)
{
	eval_king_checks(b, &(a->ke[WHITE]), NULL, WHITE);
	eval_king_checks(b, &(a->ke[BLACK]), NULL, BLACK);
return 0;
}

int isDrawBy50x(board * b) {
	return 0;
}

int is_draw(board *b, attack_model *a, personality *p)
{
int ret,i, count;

	if((b->mindex_validity==1) && (p->mat_info[b->mindex][b->side]==INSUFF)) return 1;


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

	ret=0;

	if((b->move-b->rule50move)>=101) {
		return 4;
	}
	
	count=0;
	i=b->move;

/*
 * threefold repetition testing
 * a position is considered identical to another if
 * the same player is on move
 * the same types of pieces of the same colors occupy the same squares
 * the same moves are available to each player; in particular, each player has the same castling and en passant capturing rights.
 */

//	i-=2;
// na i musi matchnout vzdy!
	while((count<3)&&(i>=b->rule50move)&&(i>=b->move_start)) {
		if(b->positions[i-b->move_start]==b->key) {
			DEB_3(if(b->posnorm[i-b->move_start]!=b->norm)	printf("Error: Not matching position to hash!\n"));
			count++;
			if((count==2)&&(i>b->move_ply_start)) {
				ret=2;
			}
		}
		i-=2;
	}
	if(count>=3) {
		ret=3;
	}
	return ret;
}

/*
 * DRAW: no pawns, num of pieces in total < 7, MAT(stronger side)-MAT(weaker side) < MAT(bishop) 
 *
 * scaling when leading side has less than 2 pawns
 */


int mat_setup(int p[2], int n[2], int bl[2], int bd[2], int r[2], int q[2], int tun[2])
{
int values[]={1000, 3500, 3500, 5000, 9750, 0};
int i, op;
int pieces, mm;
int pc[2], b[2];
int min[2], maj[2], m[2];
	
	pieces=0;
	for(i=0;i<2;i++) {
		m[i]=p[i]*values[0]+n[i]*values[0]+(bl[i]+bd[i])*values[2]+r[i]*values[3]+q[i]*values[4];
		pieces+=q[i]+r[i]+bd[i]+bl[i]+n[i];
		b[i]=bl[i]+bd[i];
		min[i]=n[i]+b[i];
		maj[i]=q[i]+r[i];
		pc[i]=min[i]+maj[i];
	}
	mm=m[0]-m[1];
	tun[0]=tun[1]=NO_INFO;

	for(i=0;i<=1;i++) {
	  op = i == 0 ? 1 : 0;
	
		if(m[i]>=m[op]) {
			if(p[i]==0) {
				if(((pc[i]==1)&&(min[i]==1))||(pc[i]==0)) {
					tun[i]=INSUFF;
				} else if((pc[i]==2)&&(n[i]==2)&&(m[op]==0)&&(pc[op]==0)) {
					tun[i]=INSUFF;
				} else if ((pc[i]==1)&&(r[i]==1)&&(min[op]==1)&&(pc[op]==1)) {
					tun[i]=DIV2;
				} else if ((pc[i]==2)&&(r[i]==1)&&(min[i]==1)&&(r[op]==1)&&(pc[op]==1)) {
					tun[i]=DIV2;
				} 
				  else if(((m[i]-m[op])<=values[1])&&((pc[i]==2)&&(b[i]==2)&&(n[op]==1)&&(pc[op]==1))) {
					tun[i]=DIV4;
				} else if(((m[i]-m[op])<=values[1])&&((min[op]<=3)&&(r[op]<=2)&&(q[op]<=1))) {
					tun[i]=DIV8;
				} else if(((m[i]-m[op])>values[1])&&(m[op]>0)) {
					tun[op]=DIV2;
				}
			} else if(p[i]==1) {
				if((pc[i]==1)&&(min[i]==1)&&((pc[op]==1)&&(min[op]==1))) {
					tun[i]=DIV4;
				} else if((pc[i]==2)&&(n[i]==2)&&((pc[op]==1)&&(p[op]==0))) {
					tun[i]=DIV4;
				} else if(((m[i]-m[op])<=values[1])&&((min[op]<=4)&&(r[op]<=2)&&(q[op]<=1))) {
					tun[i]=DIV2;
				}
			}
		}	
	}
return 0;
}


int mat_info(int8_t info[][2])
{
int f;
	for(f=0;f<419999;f++) {
			info[f][0]=info[f][1]=NO_INFO;
	}
// certain values known draw
//	m=MATidx(pw,pb,nw,nb,bwl,bwd,bbl,bbd,rw,rb,qw,qb);
// pw,pb,nw,nb,bwl,bwd,bbl,bbd,rw,rb,qw,qb, TYPE

int i,m;
int p[2], n[2], bl[2], bd[2], r[2], q[2], pp, tun[2];


	for(q[1]=0;q[1]<2;q[1]++) {
		for(q[0]=0;q[0]<2;q[0]++) {
			for(r[1]=0;r[1]<3;r[1]++) {
				for(r[0]=0;r[0]<3;r[0]++) {
					for(bd[1]=0;bd[1]<2;bd[1]++) {
						for(bl[1]=0;bl[1]<2;bl[1]++) {
							for(bd[0]=0;bd[0]<2;bd[0]++) {
								for(bl[0]=0;bl[0]<2;bl[0]++) {
									for(n[1]=0;n[1]<3;n[1]++) {
										for(n[0]=0;n[0]<3;n[0]++) {
											for(p[1]=0;p[1]<9;p[1]++) {
												for(p[0]=0;p[0]<9;p[0]++) {
													m=MATidx(p[0],p[1],n[0],n[1],bl[0],bd[0],bl[1],bd[1],r[0],r[1],q[0],q[1]);
													mat_setup(p, n, bl, bd, r, q, tun);
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
int i, tot, q, m;
uint8_t fz;
int vaha[]={0,6,6,9,18};
int nc[]={16,4,4,4,2};
// clear
	for(f=0;f<419999;f++) {
			faze[f]=0;
	}
	tot=nc[PAWN]*vaha[PAWN]+nc[KNIGHT]*vaha[KNIGHT]+nc[BISHOP]*vaha[BISHOP]+nc[ROOK]*vaha[ROOK]+nc[QUEEN]*vaha[QUEEN];
	for(qb=0;qb<2;qb++) {
		for(qw=0;qw<2;qw++) {
			for(rb=0;rb<3;rb++) {
				for(rw=0;rw<3;rw++) {
					for(bbd=0;bbd<2;bbd++) {
						for(bbl=0;bbl<2;bbl++) {
							for(bwd=0;bwd<2;bwd++) {
								for(bwl=0;bwl<2;bwl++) {
									for(nb=0;nb<3;nb++) {
										for(nw=0;nw<3;nw++) {
											for(pb=0;pb<9;pb++) {
												for(pw=0;pw<9;pw++) 
												{
													m=MATidx(pw,pb,nw,nb,bwl,bwd,bbl,bbd,rw,rb,qw,qb);
													i =(pb+pw)*vaha[PAWN];
													i+=(nw+nb)*vaha[KNIGHT];
													i+=(bbd+bbl+bwd+bwl)*vaha[BISHOP];
													i+=(rw+rb)*vaha[ROOK];
													i+=(qw+qb)*vaha[QUEEN];
													q=Min(i, tot);
													fz=q*255/tot;
													assert(faze[m]==0);
													faze[m]=fz;
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

int meval_table_gen(meval_t *t, personality *p, int stage){
int pw, pb, nw, nb, bwl, bwd, bbl, bbd, rw, rb, qw, qb, f;
int m, w, b;
float mcount;
/*
	milipawns
	jeden P ma hodnotu 1000
	to dava nejlepsi materialove skore kolem 41000 za normalnich okolnosti. V extremu asi 102000. 
	Rezerva na bonusy 3x.
	tj. 123000, resp. 306000. 
	jako MATESCORE dam 0x50000 -- 327680
*/

	MATIdxIncW[PAWN]=PW_MI;
	MATIdxIncB[PAWN]=PB_MI;
	MATIdxIncW[KNIGHT]=NW_MI;
	MATIdxIncB[KNIGHT]=NB_MI;
	MATIdxIncW[ROOK]=RW_MI;
	MATIdxIncB[ROOK]=RB_MI;
	MATIdxIncW[QUEEN]=QW_MI;
	MATIdxIncB[QUEEN]=QB_MI;
	MATIdxIncW[BISHOP]=BWL_MI;
	MATIdxIncB[BISHOP]=BBL_MI;
	MATIdxIncW[BISHOP+ER_PIECE]=BWD_MI;
	MATIdxIncB[BISHOP+ER_PIECE]=BBD_MI;

	MATincW2[PAWN]=PW_MI2;
	MATincB2[PAWN]=PB_MI2;
	MATincW2[KNIGHT]=NW_MI2;
	MATincB2[KNIGHT]=NB_MI2;
	MATincW2[ROOK]=RW_MI2;
	MATincB2[ROOK]=RB_MI2;
	MATincW2[QUEEN]=QW_MI2;
	MATincB2[QUEEN]=QB_MI2;
	MATincW2[BISHOP]=BWL_MI2;
	MATincB2[BISHOP]=BBL_MI2;
	MATincW2[BISHOP+ER_PIECE]=BWD_MI2;
	MATincB2[BISHOP+ER_PIECE]=BBD_MI2;

	
// clear
	for(f=0;f<419999;f++) {
			t[f].mat=0;
//			t[f].info=NO_INFO;
	}
	for(qb=0;qb<2;qb++) {
		for(qw=0;qw<2;qw++) {
			for(rb=0;rb<3;rb++) {
				for(rw=0;rw<3;rw++) {
					for(bbd=0;bbd<2;bbd++) {
						for(bbl=0;bbl<2;bbl++) {
							for(bwd=0;bwd<2;bwd++) {
								for(bwl=0;bwl<2;bwl++) {
									for(nb=0;nb<3;nb++) {
										for(nw=0;nw<3;nw++) {
											for(pb=0;pb<9;pb++) {
												for(pw=0;pw<9;pw++) {
													m=MATidx(pw,pb,nw,nb,bwl,bwd,bbl,bbd,rw,rb,qw,qb);
													w=pw*p->Values[stage][0]+nw*p->Values[stage][1]+(bwl+bwd)*p->Values[stage][2]+rw*p->Values[stage][3]+qw*p->Values[stage][4];
													b=pb*p->Values[stage][0]+nb*p->Values[stage][1]+(bbl+bbd)*p->Values[stage][2]+rb*p->Values[stage][3]+qb*p->Values[stage][4];
// tune rooks and knight based on pawns at board

#if 1
														w+=nw*(pw-5)*p->rook_to_pawn[stage]/2;
														w+=rw*(5-pw)*p->rook_to_pawn[stage];
														b+=nb*(pb-5)*p->rook_to_pawn[stage]/2;
														b+=rb*(5-pb)*p->rook_to_pawn[stage];
#else
													if((pb+pw)>=13) {
														w-=(qw*p->Values[stage][4]*0.1);
														b-=(qb*p->Values[stage][4]*0.1);
														w-=(rw*p->Values[stage][3]*0.15);
														b-=(rb*p->Values[stage][3]*0.15);
														w+=(nw*p->Values[stage][1]*0.1);
														b+=(nb*p->Values[stage][1]*0.1);
													} else if((pb+pw)>=9) {
														w-=(qw*p->Values[stage][4]*0.05);
														b-=(qb*p->Values[stage][4]*0.05);
														w-=(rw*p->Values[stage][3]*0.1);
														b-=(rb*p->Values[stage][3]*0.1);
														w+=((bwl+bwd)*p->Values[stage][2]*0.05);
														b+=((bbl+bbd)*p->Values[stage][2]*0.05);
														w+=(nw*p->Values[stage][1]*0.1);
														b+=(nb*p->Values[stage][1]*0.1);
													} else if((pb+pw)>=5) {
														w+=(qw*p->Values[stage][4]*0.2);
														b+=(qb*p->Values[stage][4]*0.2);
														w+=(rw*p->Values[stage][3]*0.1);
														b+=(rb*p->Values[stage][3]*0.1);
														w+=((bwl+bwd)*p->Values[stage][2]*0.15);
														b+=((bbl+bbd)*p->Values[stage][2]*0.15);
														w-=(nw*p->Values[stage][1]*0.1);
														b-=(nb*p->Values[stage][1]*0.1);
													} else {
														w+=(qw*p->Values[stage][4]*0.30);
														b+=(qb*p->Values[stage][4]*0.30);
														w+=(rw*p->Values[stage][3]*0.1);
														b+=(rb*p->Values[stage][3]*0.1);
														w+=((bwl+bwd)*p->Values[stage][2]*0.2);
														b+=((bbl+bbd)*p->Values[stage][2]*0.2);
														w-=(nw*p->Values[stage][1]*0.15);
														b-=(nb*p->Values[stage][1]*0.15);
													}
													mcount=pw+nw*3+(bwl+bwd)*3+rw*4.5f+qw*9+pb+nb*3+(bbl+bbd)*3+rb*4.5f+qb*9;
													if(mcount>=45) {
													} else if(mcount>=30) {
														w+=(pw*p->Values[stage][0]*0.05);
														b+=(pb*p->Values[stage][0]*0.05);
													} else if(mcount>=15) {
														w+=(pw*p->Values[stage][0]*0.10);
														b+=(pb*p->Values[stage][0]*0.10);
													} else {
														w+=(pw*p->Values[stage][0]*0.15);
														b+=(pb*p->Values[stage][0]*0.15);
													}
#endif

// tune bishop pair
													if((bwl>=1)&&(bwd>=1)) w+=p->bishopboth[stage];
													if((bbl>=1)&&(bbd>=1)) b+=p->bishopboth[stage];
// zohlednit materialove nerovnovahy !!!
													if(t[m].mat!=0)
														printf("poplach %d %d!!!!\n", m, t[m].mat);
													t[m].mat=(w-b);
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

int get_material_eval(board *b, personality *p, int *mb, int *me){
int bb, wb, be, we, stage;
int pw, pb, nw, nb, bwl, bwd, bbl, bbd, rw, rb, qw, qb;
	if(b->mindex_validity==1) {
		*mb = p->mat[b->mindex].mat;
		*me= p->mate_e[b->mindex].mat;
		return 1;
	} else {
		bwd=b->material[WHITE][BISHOP+ER_PIECE];
		bbd=b->material[BLACK][BISHOP+ER_PIECE];
		bwl=b->material[WHITE][BISHOP]-bwd;
		bbl=b->material[BLACK][BISHOP]-bbd;
		pw=b->material[WHITE][PAWN];
		pb=b->material[BLACK][PAWN];
		nw=b->material[WHITE][KNIGHT];
		nb=b->material[BLACK][KNIGHT];
		rw=b->material[WHITE][ROOK];
		rb=b->material[BLACK][ROOK];
		qw=b->material[WHITE][QUEEN];
		qb=b->material[BLACK][QUEEN];
		stage=0;
		wb=pw*p->Values[stage][0]+nw*p->Values[stage][1]+(bwl+bwd)*p->Values[stage][2]+rw*p->Values[stage][3]+qw*p->Values[stage][4];
		bb=pb*p->Values[stage][0]+nb*p->Values[stage][1]+(bbl+bbd)*p->Values[stage][2]+rb*p->Values[stage][3]+qb*p->Values[stage][4];
		stage=1;
		we=pw*p->Values[stage][0]+nw*p->Values[stage][1]+(bwl+bwd)*p->Values[stage][2]+rw*p->Values[stage][3]+qw*p->Values[stage][4];
		be=pb*p->Values[stage][0]+nb*p->Values[stage][1]+(bbl+bbd)*p->Values[stage][2]+rb*p->Values[stage][3]+qb*p->Values[stage][4];
		*mb=wb-bb;
		*me=we-be;
	}
return 2;
}

int get_material_eval_f(board *b, personality *p){
int score;
int me,mb;
int phase = eval_phase(b, p);

	get_material_eval(b, p, &mb, &me);
	score=mb*phase+me*(255-phase);
	return score / 255;
}

int eval_bishop(board *b, attack_model *a, PawnStore *ps, int side, personality *p){
int piece;
int from;
int f;

	piece = (side == WHITE) ? BISHOP : BISHOP|BLACKPIECE;
	for (f = a->pos_c[piece]; f >= 0; f--) {
		from = a->pos_m[piece][f];
		a->sc.side[side].mobi_b += a->me[from].pos_mob_tot_b;
		a->sc.side[side].mobi_e += a->me[from].pos_mob_tot_e;
		a->sq[from].sqr_b=p->piecetosquare[0][side][BISHOP][from];
		a->sq[from].sqr_e=p->piecetosquare[1][side][BISHOP][from];
		a->sc.side[side].sqr_b += a->sq[from].sqr_b;
		a->sc.side[side].sqr_e += a->sq[from].sqr_e;
		
	}
	return 0;
}

int eval_knight(board *b, attack_model *a, PawnStore *ps, int side, personality *p){
int piece;
int from;
int f;

	piece = (side == WHITE) ? KNIGHT : KNIGHT|BLACKPIECE;
	for (f = a->pos_c[piece]; f >= 0; f--) {
		from = a->pos_m[piece][f];
		a->sc.side[side].mobi_b += a->me[from].pos_mob_tot_b;
		a->sc.side[side].mobi_e += a->me[from].pos_mob_tot_e;
		a->sq[from].sqr_b=p->piecetosquare[0][side][KNIGHT][from];
		a->sq[from].sqr_e=p->piecetosquare[1][side][KNIGHT][from];
		a->sc.side[side].sqr_b += a->sq[from].sqr_b;
		a->sc.side[side].sqr_e += a->sq[from].sqr_e;

	}
	return 0;
}

int eval_queen(board *b, attack_model *a, PawnStore *ps, int side, personality *p){
int piece;
int from;
int f;

	piece = (side == WHITE) ? QUEEN : QUEEN|BLACKPIECE;
	for (f = a->pos_c[piece]; f >= 0; f--) {
		from = a->pos_m[piece][f];
		a->sc.side[side].mobi_b += a->me[from].pos_mob_tot_b;
		a->sc.side[side].mobi_e += a->me[from].pos_mob_tot_e;
		a->sq[from].sqr_b=p->piecetosquare[0][side][QUEEN][from];
		a->sq[from].sqr_e=p->piecetosquare[1][side][QUEEN][from];
		a->sc.side[side].sqr_b += a->sq[from].sqr_b;
		a->sc.side[side].sqr_e += a->sq[from].sqr_e;

	}
	return 0;
}

int eval_rook(board *b, attack_model *a, PawnStore *ps, int side, personality *p){
int piece;
int from;
int srank;
int z,f;
int opside;
BITVAR v,n;

	if(side == WHITE) {
		piece= ROOK;
		srank= 6;
		opside=BLACK;
	} else {
		piece= ROOK|BLACKPIECE;
		srank= 1;
		opside=WHITE;
	}
	for (f = a->pos_c[piece]; f >= 0; f--) {
		from = a->pos_m[piece][f];

		a->sc.side[side].mobi_b += a->me[from].pos_mob_tot_b;
		a->sc.side[side].mobi_e += a->me[from].pos_mob_tot_e;
		a->sq[from].sqr_b=p->piecetosquare[0][side][ROOK][from];
		a->sq[from].sqr_e=p->piecetosquare[1][side][ROOK][from];
		a->sc.side[side].sqr_b += a->sq[from].sqr_b;
		a->sc.side[side].sqr_e += a->sq[from].sqr_e;

//???		
//		a->sc.side[side].specs_b+=a->specs[side][ROOK].sqr_b;
//		a->sc.side[side].specs_e+=a->specs[side][ROOK].sqr_e;
		
		z=getRank(from);
		if(z==srank) {
			a->specs[side][ROOK].sqr_b+=p->rook_on_seventh[0];
			a->specs[side][ROOK].sqr_e+=p->rook_on_seventh[1];
		}

		n=attack.file[from];
		if(n&ps->not_pawns_file[side]&ps->not_pawns_file[opside]){
			a->specs[side][ROOK].sqr_b+=p->rook_on_open[0];
			a->specs[side][ROOK].sqr_e+=p->rook_on_open[1];
		} else if(n&ps->not_pawns_file[side]&(~ps->not_pawns_file[opside])) {
				a->specs[side][ROOK].sqr_b+=p->rook_on_semiopen[0];
				a->specs[side][ROOK].sqr_e+=p->rook_on_semiopen[1];
		}
	}
	return 0;
}

int eval_pawn(board *b, attack_model *a, PawnStore *ps, int side, personality *p){

	a->sc.side[side].sqr_b +=ps->score[side].sqr_b;
	a->sc.side[side].sqr_e +=ps->score[side].sqr_e;
	return 0;
}

int eval_king2(board *b, attack_model *a, PawnStore *ps, int side, personality *p){
int from, m, to, sl;
BITVAR mv;

	a->specs[side][KING].sqr_b=0;
	a->specs[side][KING].sqr_e=0;
	from=b->king[side];

// king mobility, spocitame vsechna pole kam muj kral muze (tj. krome vlastnich figurek a poli na ktere utoci nepratelsky kral
// a poli ktera jsou napadena cizi figurou
	mv = (attack.maps[KING][from]) & (~b->colormaps[side]) & (~attack.maps[KING][b->king[side^1]]);
	mv = mv & (~a->att_by_side[side^1]) & (~a->ke[side].cr_att_ray) & (~a->ke[side].di_att_ray);

	m=a->me[from].pos_att_tot=BitCount(mv);
	a->me[from].pos_mob_tot_b=p->mob_val[0][side][KING][m];
	a->me[from].pos_mob_tot_e=p->mob_val[1][side][KING][m];
	a->sq[from].sqr_b=p->piecetosquare[0][side][KING][from];
	a->sq[from].sqr_e=p->piecetosquare[1][side][KING][from];

// evalute shelter

	sl=getFile(from);
	if(sl<=2) {
		a->specs[side][KING].sqr_b+=ps->shelter_a[side].sqr_b;
		a->specs[side][KING].sqr_e+=ps->shelter_a[side].sqr_e;
	} else if(sl>=5) {
		a->specs[side][KING].sqr_b+=ps->shelter_h[side].sqr_b;
		a->specs[side][KING].sqr_e+=ps->shelter_h[side].sqr_e;
	} else {
		a->specs[side][KING].sqr_b+=ps->shelter_m[side].sqr_b;
		a->specs[side][KING].sqr_e+=ps->shelter_m[side].sqr_e;
	}

	a->sc.side[side].mobi_b += a->me[from].pos_mob_tot_b;
	a->sc.side[side].mobi_e += a->me[from].pos_mob_tot_e;
	a->sc.side[side].sqr_b += a->sq[from].sqr_b;
	a->sc.side[side].sqr_e += a->sq[from].sqr_e;
//	a->sc.side[side].specs_b +=a->specs[side][KING].sqr_b;
//	a->sc.side[side].specs_e +=a->specs[side][KING].sqr_e;
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
int eval_x(board* b, attack_model *a, personality* p) {
int f, from;
int score, score_b, score_e;
PawnStore pps, *ps;
	
	ps=&pps;

	a->phase = eval_phase(b, p);
// setup pawn attacks

	for(f=(ER_PIECE|BLACKPIECE);f>=0;f--) {
		a->pos_c[f]=-1;
	}

	a->att_by_side[WHITE]=a->pa_at[WHITE]=WhitePawnAttacks(b, a, &(a->pa_at_mo[WHITE]));
	a->att_by_side[BLACK]=a->pa_at[BLACK]=BlackPawnAttacks(b, a, &(a->pa_at_mo[BLACK]));
	a->pa_mo[WHITE]=WhitePawnMoves(b, a);
	a->pa_mo[BLACK]=BlackPawnMoves(b, a);
// bez ep!

// initialize
	a->sc.side[0].mobi_b = 0;
	a->sc.side[0].mobi_e = 0;
	a->sc.side[0].sqr_b = 0;
	a->sc.side[0].sqr_e = 0;
	a->sc.side[0].specs_b = 0;
	a->sc.side[0].specs_e = 0;;
	a->sc.side[1].mobi_b = 0;
	a->sc.side[1].mobi_e = 0;
	a->sc.side[1].sqr_b = 0;
	a->sc.side[1].sqr_e = 0;
	a->sc.side[1].specs_b = 0;
	a->sc.side[1].specs_e = 0;;
	
	a->specs[WHITE][ROOK].sqr_b=a->specs[BLACK][ROOK].sqr_b=0;
	a->specs[WHITE][ROOK].sqr_e=a->specs[BLACK][ROOK].sqr_e=0;
	a->specs[WHITE][BISHOP].sqr_b=a->specs[BLACK][BISHOP].sqr_b=0;
	a->specs[WHITE][BISHOP].sqr_e=a->specs[BLACK][BISHOP].sqr_e=0;
	a->specs[WHITE][KNIGHT].sqr_b=a->specs[BLACK][KNIGHT].sqr_b=0;
	a->specs[WHITE][KNIGHT].sqr_e=a->specs[BLACK][KNIGHT].sqr_e=0;
	a->specs[WHITE][QUEEN].sqr_b=a->specs[BLACK][QUEEN].sqr_b=0;
	a->specs[WHITE][QUEEN].sqr_e=a->specs[BLACK][QUEEN].sqr_e=0;
	a->specs[WHITE][KING].sqr_b=a->specs[BLACK][KING].sqr_b=0;
	a->specs[WHITE][KING].sqr_e=a->specs[BLACK][KING].sqr_e=0;
	a->specs[WHITE][PAWN].sqr_b=a->specs[BLACK][PAWN].sqr_b=0;
	a->specs[WHITE][PAWN].sqr_e=a->specs[BLACK][PAWN].sqr_e=0;

// build attack model + calculate mobility
	make_mobility_model(b, a, p);
	
// build pawn mode + pawn cache	+ evaluate + pre comupte pawn king shield
//	make_pawn_model(b, a, p);
	premake_pawn_model(b, a, ps, p);
	
// compute material	
	get_material_eval(b, p, &a->sc.material, &a->sc.material_e);

// evaluate individual pieces + PST + piece special feature + features related to piece-pawn interaction
	eval_bishop(b, a, ps, WHITE, p);
	eval_bishop(b, a, ps, BLACK, p);
	eval_knight(b, a, ps, WHITE, p);
	eval_knight(b, a, ps, BLACK, p);
	eval_queen(b, a, ps, WHITE, p);
	eval_queen(b, a, ps, BLACK, p);
	eval_rook(b, a, ps, WHITE, p);
	eval_rook(b, a, ps, BLACK, p);
	eval_pawn(b, a, ps, WHITE, p);
	eval_pawn(b, a, ps, BLACK, p);

// evaluate king 
	eval_king2(b, a, ps, WHITE, p);
	eval_king2(b, a, ps, BLACK, p);

// evaluate inter pieces features or global features

//all evaluations are in milipawns 
// phase is in range 0 - 255. 255 being total opening, 0 total ending

	if(p->simple_EVAL==1) {
// simplified eval
//		score_b=a->sc.material+(a->sc.side[0].sqr_b - a->sc.side[1].sqr_b);
//		score_e=a->sc.material_e+(a->sc.side[0].sqr_e - a->sc.side[1].sqr_e);
		score_b=a->sc.material;
		score_e=a->sc.material_e;
		score=score_b*a->phase+score_e*(255-a->phase);
	} else {
#if 1
		score_b=a->sc.material+(a->sc.side[0].mobi_b - a->sc.side[1].mobi_b)+(a->sc.side[0].sqr_b - a->sc.side[1].sqr_b)+(a->sc.side[0].specs_b-a->sc.side[1].specs_b );
		score_e=a->sc.material_e +(a->sc.side[0].mobi_e - a->sc.side[1].mobi_e)+(a->sc.side[0].sqr_e - a->sc.side[1].sqr_e)+(a->sc.side[0].specs_e-a->sc.side[1].specs_e );
		score=score_b*a->phase+score_e*(255-a->phase);
#endif
	
/*	
	score = a->phase * (a->sc.material) + (256 - a->phase) * (a->sc.material_e);
	score += a->phase * (a->sc.side[0].mobi_b - a->sc.side[1].mobi_b) 
			+ (256 - a->phase) * (a->sc.side[0].mobi_e - a->sc.side[1].mobi_e);
	score += a->phase * (a->sc.side[0].sqr_b - a->sc.side[1].sqr_b)
			+ (256 - a->phase) * (a->sc.side[0].sqr_e - a->sc.side[1].sqr_e);
			
*/

	a->sc.scaling=(p->mat_info[b->mindex][b->side]);
	score += p->eval_BIAS;
#ifndef TUNING	
		if((b->mindex_validity==1)&&(((b->side==WHITE)&&(score>0))||((b->side==BLACK)&&(score<0)))) {
			switch(p->mat_info[b->mindex][b->side]) {
			case NO_INFO:
				break;
			case INSUFF:
				score=0;
				break;
			case UNLIKELY:
				score/=16;
				break;
			case DIV2:
				score/=2;
				break;
			case DIV4:
				score/=4;
				break;
			case DIV8:
				score/=8;
				break;
			default:
				break;
			}
		}
#endif		
	}
#if 0
//	if((score>100000*256) || (score< -100000*256)) {
		LOGGER_0("mat %d, mob %d, mob %d, sqr %d, sqr %d, spc %d, spc %d\n", a->sc.material,a->sc.side[0].mobi_b, a->sc.side[1].mobi_b, a->sc.side[0].sqr_b, a->sc.side[1].sqr_b, a->sc.side[0].specs_b, a->sc.side[1].specs_b );
		LOGGER_0("mat %d, mob %d, mob %d, sqr %d, sqr %d, spc %d, spc %d\n", a->sc.material_e,a->sc.side[0].mobi_e,a->sc.side[1].mobi_e,a->sc.side[0].sqr_e, a->sc.side[1].sqr_e, a->sc.side[0].specs_e, a->sc.side[1].specs_e );
		score=score_b*a->phase+score_e*(255-a->phase);
		LOGGER_0("score %d, phase %d, score_b %d, score_e %d\n", score / 255, a->phase, score_b, score_e);
//	}
#endif
//		score=score_b*a->phase+score_e*(255-a->phase);
	a->sc.complete = score / 255;
	return a->sc.complete;
}

int eval(board* b, attack_model *a, personality* p) {
attack_model a2;
int i;
//	a2=*a;
//	i=p->simple_EVAL;
//	p->simple_EVAL=1;
	eval_x(b, a, p);
//	p->simple_EVAL=0;
//	eval_x(b,&a2, p);
//	p->simple_EVAL=i;
//	if(a->sc.complete!=a2.sc.complete) {
//		LOGGER_0("SIMPLE Score %d, FULL score %d\n", a->sc.complete, a2.sc.complete);	
//	}
	return a->sc.complete;
}


//
//
//
//  Pxp, PxP, PxP, RxB, BxR, QxB, QxQ
// G:1 ,  0 ,  1 ,  0 ,  5 ,  -2,  12, -2 
//   1  -1,  1 , -5,   5 , -12,
//  Pxp, BxP, RxP  ?xR
//  1,   0,   3,   2,

int SEE(board * b, int m) {
int fr, to, side,d;
int gain[32];
BITVAR ignore;
int attacker;

	ignore=FULLBITMAP;
	fr=UnPackFrom(m);
	to=UnPackTo(m);
	side=(b->pieces[fr]&BLACKPIECE)!=0;
	d=0;
	gain[d]=b->pers->Values[0][b->pieces[to]&PIECEMASK];
	attacker=fr;
	while (attacker!=-1) {
		d++;
		gain[d]=-gain[d-1]+b->pers->Values[0][b->pieces[attacker]&PIECEMASK];
		if(Max(-gain[d-1], gain[d]) < 0) break;
		side^=1;
		ignore^=normmark[attacker];
		attacker=GetLVA_to(b, to, side, ignore);
//		ignore^=normmark[attacker];
	}
	while(--d) {
		gain[d-1]= -Max(-gain[d-1], gain[d]);
	}
	return gain[0];
}

int SEE_0(board * b, int move) {
int to, side,d;
int gain[32];
BITVAR ignore;
int attacker;

	to=UnPackTo(move);
	ignore=FULLBITMAP;
	printBoardNice(b);
	side=((b->pieces[to]&BLACKPIECE)!=0)? BLACK:WHITE;
	d=0;
//	gain[d]=b->pers->Values[0][b->pieces[to]&PIECEMASK];
	gain[d]=0;
	attacker=to;
//	attacker=GetLVA_to(b, to, side, ignore);
	while (attacker!=-1) {
		d++;
		gain[d]=-gain[d-1]+b->pers->Values[0][b->pieces[attacker]&PIECEMASK];
		if(Max(-gain[d-1], gain[d]) < 0) break;
		side^=1;
		ignore^=normmark[attacker];
		attacker=GetLVA_to(b, to, side, ignore);
//		ignore^=normmark[attacker];
	}
	while(--d) {
		gain[d-1]= -Max(-gain[d-1], gain[d]);
	}
	return gain[0];
}

int copyAttModel(attack_model *source, attack_model *dest){
	memcpy(dest, source, sizeof(attack_model));
return 0;
}

// [side][piece] 
BITVAR getMatKey(unsigned char m[][2*ER_PIECE]){
BITVAR k;
	k=0;
	k^=randomTable[WHITE][A2+m[WHITE][PAWN]][PAWN];
	k^=randomTable[WHITE][A2+m[WHITE][KNIGHT]][KNIGHT];
	k^=randomTable[WHITE][A2+m[WHITE][BISHOP]-m[WHITE][ER_PIECE+BISHOP]][BISHOP];
	k^=randomTable[WHITE][A4+m[WHITE][ER_PIECE+BISHOP]][BISHOP];
	k^=randomTable[WHITE][A2+m[WHITE][ROOK]][ROOK];
	k^=randomTable[WHITE][A2+m[WHITE][QUEEN]][QUEEN];

	k^=randomTable[BLACK][A2+m[BLACK][PAWN]][PAWN];
	k^=randomTable[BLACK][A2+m[BLACK][KNIGHT]][KNIGHT];
	k^=randomTable[BLACK][A2+m[BLACK][BISHOP]-m[BLACK][ER_PIECE+BISHOP]][BISHOP];
	k^=randomTable[BLACK][A4+m[BLACK][ER_PIECE+BISHOP]][BISHOP];
	k^=randomTable[BLACK][A2+m[BLACK][ROOK]][ROOK];
	k^=randomTable[BLACK][A2+m[BLACK][QUEEN]][QUEEN];
return k;
}
 
int check_mindex_validity(board *b, int force) {

int bwl, bwd, bbl, bbd;

	if((force==1)||(b->mindex_validity==0)) {
		b->mindex_validity=0;
		bwd=b->material[WHITE][BISHOP+ER_PIECE];
		bbd=b->material[BLACK][BISHOP+ER_PIECE];
		bwl=b->material[WHITE][BISHOP]-bwd;
		bbl=b->material[BLACK][BISHOP]-bbd;

		if((b->material[WHITE][QUEEN]>1) || (b->material[BLACK][QUEEN]>1) \
			|| (b->material[WHITE][KNIGHT]>2) || (b->material[BLACK][KNIGHT]>2) \
			|| (bwd>1) || (bwl>1) || (bbl>1) || (bbd>1) \
			|| (b->material[WHITE][ROOK]>2) || (b->material[BLACK][ROOK]>2) \
			|| (b->material[WHITE][PAWN]>8) || (b->material[BLACK][PAWN]>8)) return 0; 
		
		b->mindex=MATidx(b->material[WHITE][PAWN],b->material[BLACK][PAWN],b->material[WHITE][KNIGHT], \
			b->material[BLACK][KNIGHT],bwl,bwd,bbl,bbd,b->material[WHITE][ROOK],b->material[BLACK][ROOK], \
			b->material[WHITE][QUEEN],b->material[BLACK][QUEEN]);
		b->mindex_validity=1;
	}
return 1;
}

/* attacker, victim
int LVAcap[ER_PIECE][ER_PIECE] = { 
	{ A_OR_N+P_OR,        A_OR+16*N_OR-P_OR,  A_OR+16*B_OR-P_OR,  A_OR+16*R_OR-P_OR,  A_OR+16*Q_OR-P_OR,  A_OR+16*K_OR-P_OR },
	{ A_OR2+16*P_OR-N_OR, A_OR_N+N_OR,        A_OR+16*B_OR-N_OR,  A_OR+16*R_OR-N_OR,  A_OR+16*Q_OR-N_OR,  A_OR+16*K_OR-N_OR },
	{ A_OR2+16*P_OR-B_OR, A_OR2+16*N_OR-B_OR, A_OR_N+B_OR,        A_OR+16*R_OR-B_OR,  A_OR+16*Q_OR-B_OR,  A_OR+16*K_OR-B_OR },
	{ A_OR2+16*P_OR-R_OR, A_OR2+16*N_OR-R_OR, A_OR2+16*B_OR-R_OR, A_OR_N+R_OR,        A_OR+16*Q_OR-R_OR,  A_OR+16*K_OR-R_OR },
	{ A_OR2+16*P_OR-Q_OR, A_OR2+16*N_OR-Q_OR, A_OR2+16*B_OR-Q_OR, A_OR2+16*R_OR-Q_OR, A_OR_N+Q_OR,        A_OR+16*K_OR-Q_OR },
	{ A_OR2+16*P_OR-K_OR, A_OR2+16*N_OR-K_OR, A_OR2+16*B_OR-K_OR, A_OR2+16*R_OR-K_OR, A_OR2+16*Q_OR-K_OR, A_OR_N+K_OR }
};
*/

// for 10 spacing
// losing A_OR2+ 100-740 
// normal A_OR_N+ 10-60
// winn	 A_OR+ 310-790 (950)

// hash
// killers

/*
 * for 20 spacing
 *
 * losing A_OR2+ 6*20-120=0 , 6*100-120=480
 * normal A_OR_N+ 6*20 - 20= 100, 6*100 - 100=500
 * winn A_OR+ 6*40-20=220, 6*100-20=580 (6*120-20=700)
 */

/*
 * for adaptive spacing
 * 10, 30, 35, 50, 97, 110
 * losing A_OR2+ 11*10-110=0 , 11*100-110=990
 * normal A_OR_N+ 11*10 - 10= 100, 11*110 - 110=1100
 * winn A_OR+ 11*30-10=320, 11*97-10=1057 (11*110-10=1200)
 */



// move ordering is to get the fastest beta cutoff

int MVVLVA_gen(int table[ER_PIECE+2][ER_PIECE], _values Values)
{
int v[ER_PIECE];
int vic, att;
	v[PAWN]=P_OR;
	v[KNIGHT]=K_OR;
	v[BISHOP]=B_OR;
	v[ROOK]=R_OR;
	v[QUEEN]=Q_OR;
	v[KING]=K_OR_M;
	for(vic=PAWN;vic<ER_PIECE;vic++) {
		for(att=PAWN;att<ER_PIECE;att++) {
// all values inserted are positive!
			if(vic==att) {
				table[att][vic]=(A_OR_N+20*v[att]-v[att]);
			} else if(vic>att) {
				table[att][vic]=(A_OR+20*v[vic]-v[att]);
			} else if(vic<att) {
				table[att][vic]=(A_OR2+10*v[vic]-v[att]);
			}
		}
	}
// lines for capture+promotion
// to queen
	for(vic=PAWN;vic<ER_PIECE;vic++) {
		att=PAWN;
		table[KING+1][vic]=(A_OR+20*v[vic]-v[PAWN]+v[QUEEN]);
//		table[KING+1][vic]=A_CA_PROM_Q+vic;
	}
// to knight
	for(vic=PAWN;vic<ER_PIECE;vic++) {
		att=PAWN;
		table[KING+2][vic]=(A_OR+20*v[vic]-v[PAWN]+v[QUEEN]);
//		table[KING+2][vic]=A_CA_PROM_N+vic;
	}

return 0;
}

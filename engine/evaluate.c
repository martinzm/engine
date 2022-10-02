
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
int i,i1,i2,i3,i4,i5, tot, fz2, q;
int vaha[]={0,5,5,11,22};
int nc[]={16,4,4,4,2};

int faze;

int bb, wb, be, we, stage;
int pw, pb, nw, nb, bwl, bwd, bbl, bbd, rw, rb, qw, qb;

// 255 -- pure beginning, 0 -- total ending
	if(b->mindex_validity==1) {
		faze=(int) p->mat_faze[b->mindex]&0xff;
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
		faze= (uint8_t) q*255/(tot);
	}
return (uint8_t)faze & 255;
}


int simple_pre_movegen_clear(board *b, attack_model *a)
{
int f;
	for(f=0;f<64;f++) a->mvs[f]=FULLBITMAP;
return 0;
}
/*
 * vygenerujeme bitmapy moznych tahu pro N, B, R, Q dane strany
 */

int simple_pre_movegen(board *b, attack_model *a, int side)
{
int f, from, pp, st, en, add;
BITVAR x, q, set3;

	if(side==BLACK) {
		st=ER_PIECE|BLACKPIECE;
		en=PAWN|BLACKPIECE;
		add=BLACKPIECE;
	} else {
		add=0;
		st=ER_PIECE;
		en=PAWN;
	}
	q=0;

// rook
	x = (b->maps[ROOK]&b->colormaps[side]);
	pp=ROOK+add;
	a->pos_c[pp]=0;
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
	a->pos_c[pp]=0;
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
	a->pos_c[pp]=0;
	while (x) {
		from = LastOne(x);
		a->pos_c[pp]++;
		a->pos_m[pp][a->pos_c[pp]]=from;
		q|=a->mvs[from] = (attack.maps[KNIGHT][from]);
		ClrLO(x);
	}
// queen
	x = (b->maps[QUEEN]&b->colormaps[side]);
	pp=QUEEN+add;
	a->pos_c[pp]=0;
	while (x) {
		from = LastOne(x);
		a->pos_c[pp]++;
		a->pos_m[pp][a->pos_c[pp]]=from;
		q|=a->mvs[from] = (QueenAttacks(b, from));
		ClrLO(x);
	}
// utoky pescu
	if(side==WHITE) {
		set3 = b->colormaps[WHITE]&b->maps[PAWN];
		a->pa_at[WHITE] = (((set3 << 9) &0xfefefefefefefefe ) | ((set3 << 7) &0x7f7f7f7f7f7f7f7f ));
	} else {
		set3 = b->colormaps[BLACK]&b->maps[PAWN];
		a->pa_at[BLACK]= (((set3 >> 7) &0xfefefefefefefefe ) | ((set3 >> 9) &0x7f7f7f7f7f7f7f7f ));
	}

	a->att_by_side[side]=q|a->pa_at[side];
return 0;
}

int simple_pre_movegen_n(board *b, attack_model *a, int side)
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
	q=0;

// rook
	x = (b->maps[ROOK]&b->colormaps[side]);
	while (x) {
		from = LastOne(x);
		q|=a->mvs[from] = (RookAttacks(b, from));
		ClrLO(x);
	}
// bishop
	x = (b->maps[BISHOP]&b->colormaps[side]);
	while (x) {
		from = LastOne(x);
		q|=a->mvs[from] = (BishopAttacks(b, from));
		ClrLO(x);
	}
// knights
	x = (b->maps[KNIGHT]&b->colormaps[side]);
	while (x) {
		from = LastOne(x);
		q|=a->mvs[from] = (attack.maps[KNIGHT][from]);
		ClrLO(x);
	}
// queen
	x = (b->maps[QUEEN]&b->colormaps[side]);
	while (x) {
		from = LastOne(x);
		q|=a->mvs[from] = (QueenAttacks(b, from));
		ClrLO(x);
	}
// utoky pescu
	if(side==WHITE) a->pa_at[WHITE]=WhitePawnAttacks(b, a);
	else a->pa_at[BLACK]=BlackPawnAttacks(b, a);

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
	res=((be*phase+en*(255-phase))/255);

#if defined (DEBUG4)
{
// PSQ analyzer
	if((res>=20)||(res<=-20)) {
		LOGGER_1("PSQS: %d, en:%d, be:%d\n",res, en, be);
	}
}
#endif
//	res=0;
//	if(res>=499) res=499;
//	else if(res<=-499) res=-499;
return res/10;
//return 0;
}


/* mobility = free safe squares (from PAWN attacks)
 * protect = free safe + friendly squares
 * unsafe = free safe + free but attacked by enemy pawns
 * mobility_protect==1 pocitam do poli pro mobilitu i moje figury
 * mobility_unsafe==1 pocitam i pole napadena nepratelskymi pesci
 *
 * builds attacks and evaluates mobility of pieces
 * other aspects of evaluation are handled in evaluate_PIECE functions
 *
 */

// requires cleared pieces counters/structures, pawn moves generated
int make_mobility_model(board *b, attack_model *a, personality *p)
{
int from, pp, m, m2, s, z;
BITVAR x, q, v, n, a1[2], togo[2], unsafe[2];

	togo[WHITE]=~(b->norm|a->pa_at[BLACK]);
	togo[BLACK]=~(b->norm|a->pa_at[WHITE]);
	unsafe[WHITE]=a->pa_at[BLACK];
	unsafe[BLACK]=a->pa_at[WHITE];

	if(p->mobility_protect==1) {
		togo[WHITE]|=(b->colormaps[WHITE] & ~unsafe[WHITE]);
		togo[BLACK]|=(b->colormaps[BLACK] & ~unsafe[BLACK]);
	}
	if(p->mobility_unsafe==1) {
		togo[WHITE]|=(unsafe[WHITE] & ~b->norm);
		togo[BLACK]|=(unsafe[BLACK] & ~b->norm);
	}
	if((p->mobility_unsafe==1)&&(p->mobility_protect==1)) {
		togo[WHITE]|=(unsafe[WHITE] & b->colormaps[WHITE]);
		togo[BLACK]|=(unsafe[BLACK] & b->colormaps[BLACK]);
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
		m=a->me[from].pos_att_tot=BitCount(q & togo[s]);
		m2=BitCount(q & togo[s] & unsafe[s]);
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
		m=a->me[from].pos_att_tot=BitCount(q & togo[s]);
		m2=BitCount(q & togo[s] & unsafe[s]);
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
		m=a->me[from].pos_att_tot=BitCount(q & togo[s]);
		m2=BitCount(q & togo[s] & unsafe[s]);
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
		m=a->me[from].pos_att_tot=BitCount(q & togo[s]);
		m2=BitCount(q & togo[s] & unsafe[s]);
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
 * mobility model can be built from simple_pre_movegen result 
 * 
 */

#define MAKEMOB(piece, side, pp) \
	for(f=a->pos_c[pp];f>=0;f--) { \
		from=a->pos_m[pp][f]; \
		q=a->mvs[from]; \
		a->att_by_side[side]|=q; \
		m=a->me[from].pos_att_tot=BitCount(q & togo[side]); \
		m2=BitCount(q & togo[side] & unsafe[side]); \
		a->me[from].pos_mob_tot_b=p->mob_val[0][side][piece][m-m2]; \
		a->me[from].pos_mob_tot_e=p->mob_val[1][side][piece][m-m2]; \
		if(p->mobility_unsafe==1) { \
			a->me[from].pos_mob_tot_b+=p->mob_uns[0][side][piece][m2]; \
			a->me[from].pos_mob_tot_e+=p->mob_uns[1][side][piece][m2]; \
		} \
	} 

int make_mobility_modelN(board *b, attack_model *a, personality *p){
int from, pp, m, m2, s, z, pc, f, side;
BITVAR x, q, v, n, a1[2], togo[2], unsafe[2], msk;


// distribute to pawn pre eval
// a->pa_at - pawn attacks for side
	a->pa_at[WHITE]=a->pa_at[BLACK]=a->pa_mo[WHITE]=a->pa_mo[BLACK]=0;

// compute pawn mobility + pawn attacks/moves
	
	int pt[2]= { PAWN, PAWN|BLACKPIECE };
	for(side=WHITE;side<=BLACK;side++) {
		msk = p->mobility_protect==1 ? FULLBITMAP : b->colormaps[Flip(side)];
		pp=pt[side];
		a->pos_c[pp]=-1;
		x = (b->maps[PAWN]&b->colormaps[side]);
		while (x) {
			a->pos_c[pp]++;
			pc=a->pos_m[pp][a->pos_c[pp]]=LastOne(x);
			q=attack.pawn_att[side][pc];
			a->pa_at[side]|=q;
			a->pa_mo[side]|=a->mvs[pc]&(~q);
			ClrLO(x);
		}
	a->att_by_side[side]=a->pa_at[side];
	}

// do not count pawn attacked and squares with side pieces by default
	togo[WHITE]=~(b->colormaps[WHITE]|a->pa_at[BLACK]);
	togo[BLACK]=~(b->colormaps[BLACK]|a->pa_at[WHITE]);
	unsafe[WHITE]=a->pa_at[BLACK];
	unsafe[BLACK]=a->pa_at[WHITE];

// count moves to squares with my own pieces, protection
	if(p->mobility_protect==1) {
		togo[WHITE]|=(b->colormaps[WHITE] & ~unsafe[WHITE]);
		togo[BLACK]|=(b->colormaps[BLACK] & ~unsafe[BLACK]);
	}
// count moves to squares attacked (by opp pawns)
	if(p->mobility_unsafe==1) {
		togo[WHITE]|=(unsafe[WHITE] & ~b->norm);
		togo[BLACK]|=(unsafe[BLACK] & ~b->norm);
	}
	if((p->mobility_unsafe==1)&&(p->mobility_protect==1)) {
		togo[WHITE]|=(unsafe[WHITE] & b->colormaps[WHITE]);
		togo[BLACK]|=(unsafe[BLACK] & b->colormaps[BLACK]);
	}

	MAKEMOB(QUEEN, WHITE, QUEEN)
	MAKEMOB(QUEEN, BLACK, QUEEN+BLACKPIECE)
	MAKEMOB(ROOK, WHITE, ROOK)
	MAKEMOB(ROOK, BLACK, ROOK+BLACKPIECE)
	MAKEMOB(BISHOP, WHITE, BISHOP)
	MAKEMOB(BISHOP, BLACK, BISHOP+BLACKPIECE)
	MAKEMOB(KNIGHT, WHITE, KNIGHT)
	MAKEMOB(KNIGHT, BLACK, KNIGHT+BLACKPIECE)

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
BITVAR temp, t2, piece;
int file, rank, tt1, tt2, from, f, i, n, x, r;

	opside = Flip(side);

	for(f=0;f<8;f++) {
		ps->prot_p_p[side][f]=EMPTYBITMAP;
		ps->prot_p_c[side][f]=EMPTYBITMAP;
	}
	
// iterate pawns
	f=0;
	from=ps->pawns[side][f];
	while(from!=-1) {
		file=getFile(from);
		rank=getRank(from);

		ps->not_pawns_file[side]&=(~attack.file[from]);
		
// span is path from pawn to next pawn before me [][][0] or behind me [][][1], or to edge of board
		dir=ps->spans[side][f][0];
		ps->pas_d[side][f]=8;
		ps->double_d[side][f]=8;
		ps->block_d[side][f]=8;
		ps->block_d2[side][f]=8;
		ps->stop_d[side][f]=8;
		ps->prot_d[side][f]=8;
		ps->prot_p_d[side][f]=8;
		ps->prot_p_p_d[side][f]=0;
		ps->prot_p_c_d[side][f]=0;
		ps->prot_dir_d[side][f]=0;
		ps->outp[side][f]=8;
		ps->outp_d[side][f]=8;
		ps->issue_d[side][f]=0;

		if((dir&ps->pass_end[side])) {
			ps->pas_d[side][f]=BitCount(dir)-1;
			ps->passer[side]|=normmark[from];
			assert((ps->pas_d[side][f]<8) &&(ps->pas_d[side][f]>=0));
		} else {
			dir=ps->spans[side][f][0];
			if(dir & ps->path_stop[side]&(b->maps[PAWN])) {
				if(dir & ps->path_stop[side]&(b->maps[PAWN])&b->colormaps[side]) {
// doubled - my pawn blocks progress
					ps->doubled[side]|=normmark[from];
					ps->double_d[side][f]=BitCount(dir)-1;
					assert((ps->double_d[side][f]<5) && (ps->double_d[side][f]>=0));
				} else {
// blocked - opposite pawn blocks progress
					ps->blocked[side]|=normmark[from];
					ps->block_d[side][f]=BitCount(dir)-1;
					assert((ps->block_d[side][f]<8) &&(ps->block_d[side][f]>=0));
				}
			}
			
			ps->issue_d[side][f]=BitCount(dir&b->maps[PAWN])
			  +BitCount(ps->half_att[opside][1])+BitCount(ps->half_att[opside][0]);
			if(ps->issue_d[side][f]>7) ps->issue_d[side][f]=7;


			dir=ps->spans[side][f][2];
			if(dir & ps->path_stop[side]&(b->maps[PAWN])) {
				if(dir & ps->path_stop[side]&(b->maps[PAWN])&b->colormaps[opside]) {
					ps->blocked2[side]|=normmark[from];
					ps->block_d2[side][f]=BitCount(dir)-1;
					assert((ps->block_d2[side][f]<8) &&(ps->block_d2[side][f]>=0));
				}
			}

			dir=ps->spans[side][f][0];
			if(dir & ps->path_stop[side]&(ps->half_att[opside][0]|ps->half_att[opside][1])) {
// stopped - opposite pawn attacks path to promotion
				ps->stopped[side]|=normmark[from];
				ps->stop_d[side][f]=BitCount(dir)-1;
//				assert((ps->stop_d[side][f]<8) &&(ps->stop_d[side][f]>=0));
			}
		}
// can I be directly protected? _surr is square around PAWN
		temp= (side == WHITE) ? (attack.pawn_surr[from]&(~(attack.uphalf[from]|attack.file[from]))) :
				 (attack.pawn_surr[from]&(~(attack.downhalf[from]|attack.file[from])));

// I am directly protected
		if(temp&b->maps[PAWN]&b->colormaps[side]) {
			ps->prot_dir[side]|=normmark[from];
			ps->prot_dir_d[side][f]=BitCount(temp&b->maps[PAWN]&b->colormaps[side]);
		}
		if(temp&ps->paths[side]) {
		
// somebody from behind can reach me
			ps->prot_p[side]|=normmark[from];
			ps->prot_p_d[side][f]=8;
			ps->prot_p_c[side][f]=EMPTYBITMAP;
			i=0;
			n=ps->pawns[side][i];
			while(n!=-1) {
				if(ps->spans[side][i][0]&temp) {
// store who is protecting me - map of f protectors
					ps->prot_p_c[side][f]|=normmark[n];

// store who I protect - map of pawns i protects
					ps->prot_p_p[side][i]|=normmark[from];
					x=getRank(n);
					r= side==WHITE ? rank-x : x-rank;
					if((side == WHITE)&&(x==1)&&(r>3)) r--;
					if((side == BLACK)&&(x==6)&&(r>3)) r--;
					r = r >= 2 ? r-2 : 8;
					if(ps->prot_p_d[side][f]>r) ps->prot_p_d[side][f]=r;
				}
				n=ps->pawns[side][++i];
			}
			assert(((ps->prot_p_d[side][f]<=2) &&(ps->prot_p_d[side][f]>=0))||(ps->prot_p_d[side][f]==8));
		}
		temp=0;
		if(file>FILEiA) temp|=((dir & ps->paths[side])>>1);
		if(file<FILEiH) temp|=((dir & ps->paths[side])<<1);
		
		if(temp&b->maps[PAWN]&b->colormaps[side]) {
// I can reach somebody
			ps->prot[side]|=normmark[from];
			ps->prot_d[side][f]=8;
			t2=temp&b->maps[PAWN]&b->colormaps[side];
			while(t2) {
				tt1=LastOne(t2);
				tt2=getRank(tt1);
				if(side==WHITE) {
					if((tt2-rank)<ps->prot_d[side][f]) {
						ps->prot_d[side][f]=(tt2-rank);
						assert((ps->prot_d[side][f]<8) &&(ps->prot_d[side][f]>=0));
					}
				} else {
					if((rank-tt2)<ps->prot_d[side][f]) {
						ps->prot_d[side][f]=(rank-tt2);
						if(!((ps->prot_d[side][f]<8) &&(ps->prot_d[side][f]>=0))) {
							assert((ps->prot_d[side][f]<8) &&(ps->prot_d[side][f]>=0));
						}
					}
				}
				ClrLO(t2);
			}
		}
// i cannot be protected and cannot progress to promotion, so backward
		if((((ps->prot_dir[side]|ps->prot[side]|ps->prot_p[side])&normmark[from])==0)
			&&(normmark[from]&(ps->blocked[side]|ps->stopped[side]))){
				ps->back[side]|=normmark[from];
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
		from=ps->pawns[side][++f];
	}

	f=0;
	from=ps->pawns[side][f];
	while(from!=-1) {
		ps->prot_p_c_d[side][f]=BitCount(ps->prot_p_c[side][f]);
		ps->prot_p_p_d[side][f]=BitCount(ps->prot_p_p[side][f]);
		from=ps->pawns[side][++f];
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
 
 
int analyze_pawn_shield_single(board *b, attack_model *a, PawnStore *ps, int side, BITVAR mask, sqr_eval *norm, sqr_eval *opts, personality *p){
BITVAR x, fst, sec;
int l, opside;

	if(side==WHITE) {
		opside=BLACK;
		fst=RANK2;
		sec=RANK3;
	} else {
		opside=WHITE;
		fst=RANK7;
		sec=RANK6;
	}
	
	x=(b->maps[PAWN]&b->colormaps[side]&mask);
	
	norm->sqr_b=norm->sqr_e=0;
	opts->sqr_b=opts->sqr_e=0;
	
	l=BitCount(x&ps->half_isol[side][0])+BitCount(x&ps->half_isol[side][1]);
	norm->sqr_b+=(p->pshelter_isol_penalty[0]*l) / 2;
	norm->sqr_e+=(p->pshelter_isol_penalty[1]*l) / 2 ;

	l=BitCount(x&ps->doubled[side]);
	norm->sqr_b+=(p->pshelter_double_penalty[0]*l);
	norm->sqr_e+=(p->pshelter_double_penalty[1]*l);

	l=BitCount(mask&ps->not_pawns_file[side]&ps->not_pawns_file[opside]);
	opts->sqr_b+=(p->pshelter_open_penalty[0]*l);
	opts->sqr_e+=(p->pshelter_open_penalty[1]*l);

	l=BitCount(mask&(~ps->not_pawns_file[side])&(ps->not_pawns_file[opside]));
	opts->sqr_b+=(p->pshelter_hopen_penalty[0]*l);
	opts->sqr_e+=(p->pshelter_hopen_penalty[1]*l);

	l=BitCount(x&fst);
	norm->sqr_b+=(p->pshelter_prim_bonus[0]*l);
	norm->sqr_e+=(p->pshelter_prim_bonus[1]*l);

	l=BitCount(x&sec);
	norm->sqr_b+=(p->pshelter_sec_bonus[0]*l);
	norm->sqr_e+=(p->pshelter_sec_bonus[1]*l);
	
	// get other pawns that should be part of the shield
	// is shield pawn advanced from shield
	l=BitCount(x&(~(fst|sec)));
	norm->sqr_b+=p->pshelter_out_penalty[0];
	norm->sqr_e+=p->pshelter_out_penalty[1];
return 0;
}

int analyze_pawn_shield(board *b, attack_model *a, PawnStore *ps, personality *p) {
int f;
int i,l;
BITVAR x;
BITVAR mask[]= { SHELTERA2, SHELTERH2, SHELTERM2, SHELTERA7, SHELTERH7, SHELTERM7 };
	analyze_pawn_shield_single(b, a, ps, WHITE, ps->shelter_p[WHITE][0], &(ps->shelter_a[WHITE]), &(ps->sh_opts[WHITE][0]), p);
	analyze_pawn_shield_single(b, a, ps, WHITE, ps->shelter_p[WHITE][1], &(ps->shelter_h[WHITE]), &(ps->sh_opts[WHITE][1]), p);
	analyze_pawn_shield_single(b, a, ps, WHITE, ps->shelter_p[WHITE][2], &(ps->shelter_m[WHITE]), &(ps->sh_opts[WHITE][2]), p);
	analyze_pawn_shield_single(b, a, ps, BLACK, ps->shelter_p[BLACK][0], &(ps->shelter_a[BLACK]), &(ps->sh_opts[BLACK][0]), p);
	analyze_pawn_shield_single(b, a, ps, BLACK, ps->shelter_p[BLACK][1], &(ps->shelter_h[BLACK]), &(ps->sh_opts[BLACK][1]), p);
	analyze_pawn_shield_single(b, a, ps, BLACK, ps->shelter_p[BLACK][2], &(ps->shelter_m[BLACK]), &(ps->sh_opts[BLACK][2]), p);
// fix middle shelter
	ps->shelter_m[WHITE].sqr_b*=3;
	ps->shelter_m[WHITE].sqr_e*=3;
	ps->shelter_m[BLACK].sqr_b*=3;
	ps->shelter_m[BLACK].sqr_e*=3;
	ps->shelter_m[WHITE].sqr_b/=4;
	ps->shelter_m[WHITE].sqr_e/=4;
	ps->shelter_m[BLACK].sqr_b/=4;
	ps->shelter_m[BLACK].sqr_e/=4;
return 0;
}

int analyze_pawn_shield_singleN(board *b, attack_model *a, PawnStore *ps, int side, int pawn, int from, int sh, int shopt, personality *p, BITVAR shlt){
BITVAR x, fst, sec, msk, n, n2;
int l, opside, ff, f, fn, fn2;

	if(side==WHITE) {
		opside=BLACK;
		fst=RANK2;
		sec=RANK3;
	} else {
		opside=WHITE;
		fst=RANK7;
		sec=RANK6;
	}
	
	x=normmark[from];
	// reset score for PAWNs in shelter to 0, all is relative to BAs
	ps->t_sc[side][pawn][sh].sqr_b=0;
	ps->t_sc[side][pawn][sh].sqr_e=0;
	ps->t_sc[side][pawn][shopt].sqr_b=0;
	ps->t_sc[side][pawn][shopt].sqr_e=0;

// PSQ - for shelter pawn ??
//	ps->t_sc[side][pawn][sh].sqr_b=+p->piecetosquare[0][side][PAWN][from];
//	ps->t_sc[side][pawn][sh].sqr_e=+p->piecetosquare[1][side][PAWN][from];

// if simple_EVAL then only material and PSQ are used
	if(p->simple_EVAL!=1) {
		if(x&(~(fst|sec))){
			ps->t_sc[side][pawn][sh].sqr_b+=(p->pshelter_out_penalty[0]);
			ps->t_sc[side][pawn][sh].sqr_e+=(p->pshelter_out_penalty[1]);
		} else {
			l=BitCount(x&ps->half_isol[side][0])+BitCount(x&ps->half_isol[side][1]);
			ps->t_sc[side][pawn][sh].sqr_b+=(p->pshelter_isol_penalty[0]*l) / 2;
			ps->t_sc[side][pawn][sh].sqr_e+=(p->pshelter_isol_penalty[1]*l) / 2 ;
			if((x&ps->doubled[side])){
				ps->t_sc[side][pawn][sh].sqr_b+=(p->pshelter_double_penalty[0]);
				ps->t_sc[side][pawn][sh].sqr_e+=(p->pshelter_double_penalty[1]);
			}
			if(x&fst){
				ps->t_sc[side][pawn][sh].sqr_b+=(p->pshelter_prim_bonus[0]);
				ps->t_sc[side][pawn][sh].sqr_e+=(p->pshelter_prim_bonus[1]);
			}
			if(x&sec){
				ps->t_sc[side][pawn][sh].sqr_b+=(p->pshelter_sec_bonus[0]);
				ps->t_sc[side][pawn][sh].sqr_e+=(p->pshelter_sec_bonus[1]);
			}
// directly protected
			if(ps->prot_dir[side]&x){
				ps->t_sc[side][pawn][sh].sqr_b+=p->pshelter_dir_protect[0][side][ps->prot_dir_d[side][pawn]];
				ps->t_sc[side][pawn][sh].sqr_e+=p->pshelter_dir_protect[1][side][ps->prot_dir_d[side][pawn]];
			}

// blocked - enemy pawn approaching
			if(ps->blocked2[side]&x) {
				ps->t_sc[side][pawn][sh].sqr_b+=p->pshelter_blocked_penalty[0][side][ps->block_d2[side][pawn]];
				ps->t_sc[side][pawn][sh].sqr_e+=p->pshelter_blocked_penalty[1][side][ps->block_d2[side][pawn]];
			}
// stopped - enemy pawn approaching 
			if(ps->stopped[side]&x) {
				ps->t_sc[side][pawn][sh].sqr_b+=p->pshelter_stopped_penalty[0][side][ps->stop_d[side][pawn]];
				ps->t_sc[side][pawn][sh].sqr_e+=p->pshelter_stopped_penalty[1][side][ps->stop_d[side][pawn]];
			}

// if mobility should be considered in shelter
//			msk = p->mobility_protect==1 ? FULLBITMAP : ~b->colormaps[side];
//			ff=BitCount(a->pa_mo[side]&attack.pawn_move[side][from]&(~b->maps[PAWN]))
//			  +BitCount(a->pa_at[side]&attack.pawn_att[side][from]&msk);
//			ps->t_sc[side][pawn][sh].sqr_b+=p->mob_val[0][side][PAWN][0]*ff;
//			ps->t_sc[side][pawn][sh].sqr_e+=p->mob_val[1][side][PAWN][0]*ff;

			if(x&(~ps->not_pawns_file[side])&(ps->not_pawns_file[opside])){
				ps->t_sc[side][pawn][shopt].sqr_b+=(p->pshelter_hopen_penalty[0]);
				ps->t_sc[side][pawn][shopt].sqr_e+=(p->pshelter_hopen_penalty[1]);
			}
		

// fixes for pawns protected by shelter pawns
		// koho chranim
			if(ps->prot_p_p[side][pawn]) {
				f=0;
				n=EMPTYBITMAP;
				while(ps->pawns[side][f]!=-1) {
					if(ps->prot_p_p[side][pawn]&ps->pawns_b[side][f]) {
						n2=ps->prot_p_c[side][f]&shlt;
// num of protectors belonging to shelter
						fn2=BitCount(n2);
// protectors total
						fn=BitCount(ps->prot_p_c[side][f]);
						assert(fn2>0);
						if(fn==fn2) {
					
// only protected by shelter pawns
							ps->t_sc[side][pawn][sh].sqr_b-=p->pawn_pot_protect[0][side][ps->prot_p_d[side][f]];
							ps->t_sc[side][pawn][sh].sqr_e-=p->pawn_pot_protect[1][side][ps->prot_p_d[side][f]];
							ps->t_sc[side][pawn][sh].sqr_b-=p->pawn_protect_count[0][side][fn];
							ps->t_sc[side][pawn][sh].sqr_e-=p->pawn_protect_count[1][side][fn];
						} else {
							ps->t_sc[side][pawn][sh].sqr_b-=p->pawn_protect_count[0][side][fn];
							ps->t_sc[side][pawn][sh].sqr_e-=p->pawn_protect_count[1][side][fn];
							ps->t_sc[side][pawn][sh].sqr_b+=p->pawn_protect_count[0][side][fn-fn2];
							ps->t_sc[side][pawn][sh].sqr_e+=p->pawn_protect_count[1][side][fn-fn2];
						}
					}
					f++;
				}
			}
		}
		
		ps->t_sc[side][pawn][shopt].sqr_b+=ps->t_sc[side][pawn][sh].sqr_b;
		ps->t_sc[side][pawn][shopt].sqr_e+=ps->t_sc[side][pawn][sh].sqr_e;
		
		if(x&((fst|sec))){
			ps->t_sc[side][pawn][sh].sqr_b-=ps->t_sc[side][pawn][BAs].sqr_b;
			ps->t_sc[side][pawn][sh].sqr_e-=ps->t_sc[side][pawn][BAs].sqr_e;
			ps->t_sc[side][pawn][shopt].sqr_b-=ps->t_sc[side][pawn][HEa].sqr_b;
			ps->t_sc[side][pawn][shopt].sqr_e-=ps->t_sc[side][pawn][HEa].sqr_e;
		}
	}
	
return 0;
}

int analyze_pawn_shield_globN(board *b, attack_model *a, PawnStore *ps, int side, BITVAR mask, int sh, int shopt, personality *p){
int count, opside;

char b2[9][9];
char b3[9][256];
char *bb[9];

		opside=Flip(side);

#if 0
		mask2init(b2);
		mask2init2(b3, bb);

		printmask2(ps->not_pawns_file[opside], b2, "NPop");
		mask2add(bb, b2);
		printmask2(ps->not_pawns_file[side], b2, "NPss");
		mask2add(bb, b2);
		printmask2(mask, b2, "mask");
		mask2add(bb, b2);
		printmask2(ps->not_pawns_file[side]&ps->not_pawns_file[opside]&mask&RANK2, b2, "res");
		mask2add(bb, b2);
		mask2print(bb);
		
#endif

		if((ps->not_pawns_file[side]&ps->not_pawns_file[opside])&mask){
			count=BitCount(ps->not_pawns_file[side]&ps->not_pawns_file[opside]&mask&RANK2);
			ps->score[side][shopt].sqr_b+=(p->pshelter_open_penalty[0]*count);
			ps->score[side][shopt].sqr_e+=(p->pshelter_open_penalty[1]*count);
//			LOGGER_0("Hx\n");
		}
return 0;
}

int analyze_pawn_shieldN(board *b, attack_model *a, PawnStore *ps, personality *p) {
int f,ff;
int i,l;
BITVAR x;
int side, from, opside, idx;

int vars[]= { SHa, SHh, SHm, SHah, SHhh, SHmh, -1 };

		for(side=0;side<=1;side++) {
			opside = Flip(side);
			f=0;
			from=ps->pawns[side][f];
			while(from!=-1) {
				x=normmark[from];
				if(x&ps->shelter_p[side][0]) analyze_pawn_shield_singleN(b, a, ps, side, f, from, SHa, SHah, p, ps->shelter_p[side][0]);
				if(x&ps->shelter_p[side][1]) analyze_pawn_shield_singleN(b, a, ps, side, f, from, SHh, SHhh, p, ps->shelter_p[side][1]);
				if(x&ps->shelter_p[side][2]) analyze_pawn_shield_singleN(b, a, ps, side, f, from, SHm, SHmh, p, ps->shelter_p[side][2]);
				ff=0;
				while(vars[ff]!=-1) {
					idx=vars[ff];
					ps->score[side][idx].sqr_b+=ps->t_sc[side][f][idx].sqr_b;
					ps->score[side][idx].sqr_e+=ps->t_sc[side][f][idx].sqr_e;
					ff++;
				}
				from=ps->pawns[side][++f];
			}
		}
		// analyze shelters not related to individual pawn
		for(side=0;side<=1;side++) {
			analyze_pawn_shield_globN(b, a, ps, side, SHELTERA, SHa, SHah, p);
			analyze_pawn_shield_globN(b, a, ps, side, SHELTERH, SHh, SHhh, p);
			analyze_pawn_shield_globN(b, a, ps, side, SHELTERM, SHm, SHmh, p);
		}
		// rescale m shelter
#if 0
		for(side=0;side<=1;side++) {
			ff=0;
			while(vars[ff]!=-1) {
				idx=vars[ff];
				ps->score[side][idx].sqr_b*=3;
				ps->score[side][idx].sqr_e*=3;
				ps->score[side][idx].sqr_b/=4;
				ps->score[side][idx].sqr_e/=4;
				ff++;
			}
		}
#endif

return 0;
}

/*
 * Precompute various possible scenarios, their use depends on king position, heavy pieces availability etc
 * NoSH, SHa, SHh, SHm in variants Heavy a NHe
 */

int pre_evaluate_pawns(board *b, attack_model *a, PawnStore *ps, personality *p)
{
int f, ff, file, n, i, from, to, rank, sq_file[8], l;
int tt, tt1, tt2, side, opside, rew_b, rew_e;
BITVAR ss1, ss2, dir, ppp, msk;
BITVAR temp, t2, x, heavy_op, SHRANK;

	for(side=0;side<=1;side++) {
		opside = Flip(side);
		f=0;
		from=ps->pawns[side][f];
		while(from!=-1) {
			x=normmark[from];
// PSQ
			ps->t_sc[side][f][BAs].sqr_b=p->piecetosquare[0][side][PAWN][from];
			ps->t_sc[side][f][BAs].sqr_e=p->piecetosquare[1][side][PAWN][from];

// if simple_EVAL then only material and PSQ are used
			if(p->simple_EVAL!=1) {
// isolated
				if((ps->half_isol[side][0] | ps->half_isol[side][1])&x) {
					if(ps->half_isol[side][0]&x) {
						ps->t_sc[side][f][BAs].sqr_b+=p->isolated_penalty[0];
						ps->t_sc[side][f][BAs].sqr_e+=p->isolated_penalty[1];
					}
					if(ps->half_isol[side][1]&x) {
						ps->t_sc[side][f][BAs].sqr_b+=p->isolated_penalty[0];
						ps->t_sc[side][f][BAs].sqr_e+=p->isolated_penalty[1];
					}
					if(x&CENTEREXBITMAP) {
						ps->t_sc[side][f][BAs].sqr_b+=p->pawn_iso_center_penalty[0];
						ps->t_sc[side][f][BAs].sqr_e+=p->pawn_iso_center_penalty[1];
					}
				}
// blocked
				if(ps->blocked[side]&x) {
					ps->t_sc[side][f][BAs].sqr_b+=p->pawn_blocked_penalty[0][side][ps->block_d[side][f]];
					ps->t_sc[side][f][BAs].sqr_e+=p->pawn_blocked_penalty[1][side][ps->block_d[side][f]];
				}
// stopped
				if(ps->stopped[side]&x) {
					ps->t_sc[side][f][BAs].sqr_b+=p->pawn_stopped_penalty[0][side][ps->stop_d[side][f]];
					ps->t_sc[side][f][BAs].sqr_e+=p->pawn_stopped_penalty[1][side][ps->stop_d[side][f]];
				}
// doubled
				if(ps->doubled[side]&x){
					ps->t_sc[side][f][BAs].sqr_b+=p->doubled_n_penalty[0][side][ps->double_d[side][f]];
					ps->t_sc[side][f][BAs].sqr_e+=p->doubled_n_penalty[1][side][ps->double_d[side][f]];
				}
// protected
//				if(ps->prot[side]&x){
//					ps->t_sc[side][f][BAs].sqr_b+=p->pawn_n_protect[0][side][ps->prot_d[side][f]];
//					ps->t_sc[side][f][BAs].sqr_e+=p->pawn_n_protect[1][side][ps->prot_d[side][f]];
//				}
				if(ps->prot_p[side]&x){
					ps->t_sc[side][f][BAs].sqr_b+=p->pawn_pot_protect[0][side][ps->prot_p_d[side][f]];
					ps->t_sc[side][f][BAs].sqr_e+=p->pawn_pot_protect[1][side][ps->prot_p_d[side][f]];
				}

// honor number of potential protectors
				ps->t_sc[side][f][BAs].sqr_b+=p->pawn_protect_count[0][side][ps->prot_p_c_d[side][f]];
				ps->t_sc[side][f][BAs].sqr_e+=p->pawn_protect_count[1][side][ps->prot_p_c_d[side][f]];
// penalize overloading - one protects too much pawns
				ps->t_sc[side][f][BAs].sqr_b+=p->pawn_prot_over_penalty[0][side][ps->prot_p_p_d[side][f]];
				ps->t_sc[side][f][BAs].sqr_e+=p->pawn_prot_over_penalty[1][side][ps->prot_p_p_d[side][f]];
// penalize number of issues pawn has 
				ps->t_sc[side][f][BAs].sqr_b+=p->pawn_issues_penalty[0][side][ps->issue_d[side][f]];
				ps->t_sc[side][f][BAs].sqr_e+=p->pawn_issues_penalty[1][side][ps->issue_d[side][f]];

// directly protected
				if(ps->prot_dir[side]&x){
					ps->t_sc[side][f][BAs].sqr_b+=p->pawn_dir_protect[0][side][ps->prot_dir_d[side][f]];
					ps->t_sc[side][f][BAs].sqr_e+=p->pawn_dir_protect[1][side][ps->prot_dir_d[side][f]];
				}
// backward,ie unprotected, not able to promote, not completely isolated
				if(ps->back[side]&(ps->blocked[side]|ps->stopped[side]|ps->doubled[side])&(~(ps->half_isol[side][0]&ps->half_isol[side][1]))&x) {
					ps->t_sc[side][f][BAs].sqr_b+=p->backward_penalty[0];
					ps->t_sc[side][f][BAs].sqr_e+=p->backward_penalty[1];
				}
// potential passer ?
				if(ps->pas_d[side][f]<8) {
					ps->t_sc[side][f][BAs].sqr_b+=p->passer_bonus[0][side][ps->pas_d[side][f]];
					ps->t_sc[side][f][BAs].sqr_e+=p->passer_bonus[1][side][ps->pas_d[side][f]];
				}
// weak...
				if((ps->back[side]|ps->blocked[side]|ps->stopped[side]|ps->doubled[side])&x) {
// in center				
					if(x&CENTEREXBITMAP) {
						ps->t_sc[side][f][BAs].sqr_b+=p->pawn_weak_center_penalty[0];
						ps->t_sc[side][f][BAs].sqr_e+=p->pawn_weak_center_penalty[1];
					}
// on open file, heavy pieces related!!!
// if index is not BAs then we store at other index difference to BAs
					if((x&ps->not_pawns_file[opside])) {
						ps->t_sc[side][f][HEa].sqr_b+=p->pawn_weak_onopen_penalty[0];
						ps->t_sc[side][f][HEa].sqr_e+=p->pawn_weak_onopen_penalty[1];
//						LOGGER_0("MM\n");
					}
				}
// fix material value
				if(x&(FILEA|FILEH)) {
					ps->t_sc[side][f][BAs].sqr_b+=p->pawn_ah_penalty[0];
					ps->t_sc[side][f][BAs].sqr_e+=p->pawn_ah_penalty[1];
				}
// mobility, but related to PAWNS only, other pieces are treated like non existant
				msk = p->mobility_protect==1 ? FULLBITMAP : ~b->colormaps[side];
				ff=BitCount(a->pa_mo[side]&attack.pawn_move[side][from]&(~b->maps[PAWN]))
				  +BitCount(a->pa_at[side]&attack.pawn_att[side][from]&msk);
				ps->t_sc[side][f][BAs].sqr_b+=p->mob_val[0][side][PAWN][0]*ff;
				ps->t_sc[side][f][BAs].sqr_e+=p->mob_val[1][side][PAWN][0]*ff;
			}
			// make HEavy absolute
			ps->t_sc[side][f][HEa].sqr_b+=ps->t_sc[side][f][BAs].sqr_b;
			ps->t_sc[side][f][HEa].sqr_e+=ps->t_sc[side][f][BAs].sqr_e;
			from=ps->pawns[side][++f];
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
BITVAR temp, t2, x, spt;

hashPawnEntry hash, h2;
int hret;

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

		// paths including stops
		ps->path_stop[WHITE]=FillNorth(b->maps[PAWN]&b->colormaps[WHITE], ps->safe_att[WHITE]& ~b->maps[PAWN], b->maps[PAWN]&b->colormaps[WHITE]);
		ps->path_stop[BLACK]=FillSouth(b->maps[PAWN]&b->colormaps[BLACK], ps->safe_att[BLACK]& ~b->maps[PAWN], b->maps[PAWN]&b->colormaps[BLACK]);

		// is path up to promotion square?
		ps->pass_end[WHITE]=ps->path_stop[WHITE] & attack.rank[A8];
		ps->pass_end[BLACK]=ps->path_stop[BLACK] & attack.rank[A1];

		// safe paths
		ps->paths[WHITE]=((ps->path_stop[WHITE]>>8)&(~(b->maps[PAWN]&b->colormaps[WHITE])))|ps->pass_end[WHITE];
		ps->paths[BLACK]=((ps->path_stop[BLACK]<<8)&(~(b->maps[PAWN]&b->colormaps[BLACK])))|ps->pass_end[BLACK];
		// stops only
		ps->path_stop2[WHITE]=ps->path_stop[WHITE]^ps->paths[WHITE];
		ps->path_stop2[BLACK]=ps->path_stop[BLACK]^ps->paths[BLACK];

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

// prepare bitmaps of potential shelter pawns - even out of shelter ones
		ps->shelter_p[WHITE][0]=ps->shelter_p[BLACK][0]=0;
		ps->shelter_p[WHITE][1]=ps->shelter_p[BLACK][1]=0;
		ps->shelter_p[WHITE][2]=ps->shelter_p[BLACK][2]=0;

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
// store "unsafe" front span
						ps->spans[WHITE][ff][2]=ss1;
// cut the front span short if path is not safe
						ps->spans[WHITE][ff][0]=ss1&ps->path_stop[WHITE];
						ps->spans[WHITE][ff][1]=ss2;
					} else {
						while((ps->pawns[BLACK][ff]!=tt)) {
							ff++;
						}
						assert(ps->pawns[BLACK][ff]==tt);
						ps->spans[BLACK][ff][2]=ss2;
						ps->spans[BLACK][ff][0]=ss2&ps->path_stop[BLACK];
						ps->spans[BLACK][ff][1]=ss1;
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
			ps->pawns[WHITE][f1]=-1;
			ps->pawns[BLACK][f2]=-1;
		}

		ps->stopped[WHITE]=ps->passer[WHITE]=ps->blocked[WHITE]=ps->blocked2[WHITE]=ps->isolated[WHITE]=ps->doubled[WHITE]=ps->back[WHITE]=EMPTYBITMAP;
		ps->stopped[BLACK]=ps->passer[BLACK]=ps->blocked[BLACK]=ps->blocked2[BLACK]=ps->isolated[BLACK]=ps->doubled[BLACK]=ps->back[BLACK]=EMPTYBITMAP;

		ps->half_isol[WHITE][0]=ps->half_isol[WHITE][1]=ps->half_isol[BLACK][0]=ps->half_isol[BLACK][1]=EMPTYBITMAP;
		ps->prot[WHITE]=ps->prot[BLACK]=ps->prot_p[WHITE]=ps->prot_p[BLACK]=ps->prot_dir[WHITE]=ps->prot_dir[BLACK]=EMPTYBITMAP;
		ps->not_pawns_file[WHITE]=ps->not_pawns_file[BLACK]=FULLBITMAP;

#if 0
char zb2[9][9];
char zb3[9][256];
char *zbb[9];
#endif

// setup SHELTER
		for(side=0;side<=1;side++) {
			opside = (side==0) ? BLACK : WHITE;
			f=0;
			from=ps->pawns[side][f];
			while(from!=-1) {
				ps->pawns_b[side][f]=normmark[from];
				if(ps->spans[side][f][1]&(~(b->maps[PAWN]&b->colormaps[side]))){
					file=getFile(from);
					if((file<=FILEiF)&&(file>=FILEiD)) ps->shelter_p[side][2]|=normmark[from];
					if(file<=FILEiC) ps->shelter_p[side][0]|=normmark[from];
					if(file>=FILEiF) ps->shelter_p[side][1]|=normmark[from];
				}
				from=ps->pawns[side][++f];
			}

#if 0
		mask2init(zb2);
		mask2init2(zb3, zbb);

		printmask2(ps->shelter_p[WHITE][0], zb2, "WSa");
		mask2add(zbb, zb2);
		printmask2(ps->shelter_p[WHITE][1], zb2, "WSh");
		mask2add(zbb, zb2);
		printmask2(ps->shelter_p[WHITE][2], zb2, "WSm");
		mask2add(zbb, zb2);
		mask2print(zbb);

		mask2init(zb2);
		mask2init2(zb3, zbb);

		printmask2(ps->shelter_p[BLACK][0], zb2, "BSa");
		mask2add(zbb, zb2);
		printmask2(ps->shelter_p[BLACK][1], zb2, "BSh");
		mask2add(zbb, zb2);
		printmask2(ps->shelter_p[BLACK][2], zb2, "BSm");
		mask2add(zbb, zb2);
		mask2print(zbb);
		
#endif

#if 0
			ps->shelter_p[WHITE][0]|=(SHELTERA2);
			ps->shelter_p[BLACK][0]|=(SHELTERA7);
			ps->shelter_p[WHITE][1]|=(SHELTERH2);
			ps->shelter_p[BLACK][1]|=(SHELTERH7);
			ps->shelter_p[WHITE][2]|=(SHELTERM2);
			ps->shelter_p[BLACK][2]|=(SHELTERM7);
#endif
		}

// analyze individual pawns
		analyze_pawn(b, a, ps, WHITE, p);
		analyze_pawn(b, a, ps, BLACK, p);

// clear scores for individual base variants and shelters 
		for(ff=0;ff<ER_VAR;ff++) {
		  for(f=0;f<8;f++) {
			ps->t_sc[WHITE][f][ff].sqr_b=0;
			ps->t_sc[WHITE][f][ff].sqr_e=0;
			ps->t_sc[BLACK][f][ff].sqr_b=0;
			ps->t_sc[BLACK][f][ff].sqr_e=0;
		  }
		  ps->score[WHITE][ff].sqr_b=0;
		  ps->score[WHITE][ff].sqr_e=0;
		  ps->score[BLACK][ff].sqr_b=0;
		  ps->score[BLACK][ff].sqr_e=0;
		}

		ps->pot_sh[WHITE]=ps->pot_sh[BLACK]=0;

		// compute scores that are only pawn related
		pre_evaluate_pawns(b, a, ps, p);

		// evaluate & score shelter
		analyze_pawn_shieldN(b, a, ps, p);

// base variants summary
int vars[]= { BAs, HEa, -1 };
		for(side=0;side<=1;side++) {
			opside = Flip(side);
			f=0;
			from=ps->pawns[side][f];
			while(from!=-1) {
				ff=0;
				while(vars[ff]!=-1) {
					ps->score[side][ff].sqr_b+=ps->t_sc[side][f][ff].sqr_b;
					ps->score[side][ff].sqr_e+=ps->t_sc[side][f][ff].sqr_e;
					ff++;
				}
				from=ps->pawns[side][++f];
			}
		}

		hash.value=*ps;
		if((b->hps!=NULL)&&(hret!=1)) storePawnHash(b->hps, &hash, b->stats);
	} else {
		*ps=hash.value;
	}
	return 0;
}

/*
 * Vygenerujeme vsechny co utoci na krale
 * vygenerujeme vsechny PINy - tedy ty kteri blokuji utok na krale
 * vygenerujeme vsechny RAYe utoku na krale
 */

int eval_king_checks_ext(board *b, king_eval *ke, personality *p, int side, int from)
{
BITVAR cr2, di2, c2, d2, c, d, c3, d3, ob, c2s, d2s, c3s, bl_ray;

int ff, o, ee;
BITVAR pp,aa, cr_temp2, di_temp2, epbmp;

//	assert((from>=0)&&(from<64));
	o= (side==0) ? BLACK:WHITE;
	epbmp= (b->ep!=-1) ? attack.ep_mask[b->ep] : 0;
	ke->ep_block=0;

// find potential attackers - get rays, and check existence of them
		cr2=di2=0;
// vert/horiz rays
		c=ke->cr_all_ray = attack.maps[ROOK][from];
// vert/horiz attackers
		c2=c2s=c & (b->maps[ROOK]|b->maps[QUEEN])&(b->colormaps[o]);
// diag rays
		d =ke->di_all_ray = attack.maps[BISHOP][from];
// diag attackers
		d2=d2s=d & (b->maps[BISHOP]|b->maps[QUEEN])&(b->colormaps[o]);

// if it can hit king, find nearest piece, blocker?
// rook/queen
		ke->cr_pins = ke->cr_attackers = ke->cr_att_ray = 0;
		
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
//						ee = LastOne(c3);
						ke->cr_pins |=c3;
						break;
// 0 means attacked
					case 0:
						ke->cr_attackers |= normmark[ff];
						ke->cr_att_ray|=attack.rays_dir[ff][from];
						break;
					case 2:
// check ep pin, see below
						if(epbmp&&((c3&(epbmp|normmark[b->ep])&b->maps[PAWN])==c3)) ke->ep_block=c3;
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
		
			while(d2) {
				ff = LastOne(d2);
				di2=attack.rays_int[from][ff];
				d3=di2 & b->norm;
				if((d3 & d2s)==0) {
					switch (BitCount(d3)) {
					case 1:
						ee = LastOne(d3);
						ke->di_pins |=d3;
						break;
					case 0:
						ke->di_attackers |= normmark[ff];
						ke->di_att_ray|=attack.rays_dir[ff][from];
						break;
					}
				}
				ClrLO(d2);
			}

// incorporate knights
		ke->kn_pot_att_pos=attack.maps[KNIGHT][from];
		ke->kn_attackers=ke->kn_pot_att_pos & b->maps[KNIGHT] & b->colormaps[o];
//incorporate pawns
		ke->pn_pot_att_pos=attack.pawn_att[side][from];
		ke->pn_attackers=ke->pn_pot_att_pos & b->maps[PAWN] & b->colormaps[o];
		ke->attackers=ke->cr_attackers | ke->di_attackers | ke->kn_attackers | ke->pn_attackers;

	return 0;
}

int eval_ind_attacks(board *b, king_eval *ke, personality *p, int side, int from)
{
BITVAR cr2, di2, c2, d2, c, d, c3, d3, coo, doo, bl_ray;

int ff, o, ee;
BITVAR pp,aa, cr_temp2, di_temp2, epbmp;

//	assert((from>=0)&&(from<64));
	o= (side==0) ? BLACK:WHITE;
	epbmp= (b->ep!=-1) ? attack.ep_mask[b->ep] : 0;
	ke->ep_block=0;

// find potential attackers - get rays, and check existence of them
		cr2=di2=0;
// vert/horiz rays
		c=ke->cr_blocker_ray=ke->cr_all_ray = attack.maps[ROOK][from];
// vert/horiz blockers
		c2=c & (((b->maps[BISHOP]|b->maps[KNIGHT]|b->maps[PAWN])&(b->colormaps[o]))|(b->norm&b->colormaps[side]));
// diag rays
		d=ke->di_blocker_ray=ke->di_all_ray = attack.maps[BISHOP][from];
// diag blockers
		d2=d & (((b->maps[ROOK]|b->maps[KNIGHT]|b->maps[PAWN])&(b->colormaps[o]))|(b->norm&b->colormaps[side]));

	coo=c & (b->maps[BISHOP]|b->maps[KNIGHT]|b->maps[PAWN])&(b->colormaps[o]);
	doo=d & (b->maps[ROOK]|b->maps[KNIGHT]|b->maps[PAWN])&(b->colormaps[o]);

// rook/queen
		ke->cr_blocks = ke->cr_attackers = ke->cr_att_ray = 0;
		
// iterate endpoints
			while(c2) {
				ff = LastOne(c2);
// get line to endpoint
				cr2=attack.rays_int[from][ff];
// check if there is piece in that line, that blocks the attack
				c3=cr2 & b->norm;
				if(BitCount(c3)==0) {
						ke->cr_blocks |= (c3 & coo);
						bl_ray=(attack.rays_dir[from][ff] ^ attack.rays[from][ff]) ^ FULLBITMAP;
						ke->cr_blocker_ray&=(bl_ray);
				}
				ClrLO(c2);
		}
		
// bishop/queen
		ke->di_blocks = ke->di_attackers = ke->di_att_ray = 0;
		
			while(d2) {
				ff = LastOne(d2);
				di2=attack.rays_int[from][ff];
				d3=di2 & b->norm;
				if(BitCount(d3)==0) {
					ke->di_blocks |=(d3 & doo);
					bl_ray=(attack.rays_dir[from][ff] ^ attack.rays[from][ff]) ^ FULLBITMAP;
					ke->di_blocker_ray&=(bl_ray);
				}
				ClrLO(d2);
			}

// incorporate knights
		ke->kn_pot_att_pos=attack.maps[KNIGHT][from];
//incorporate pawns
		ke->pn_pot_att_pos=attack.pawn_att[side][from];

// blocker rays contain squares from king can be attacked by particular type of piece
// blocks contains opside pieces that might block opside attack and moving that away might cause attack

	return 0;
}

// eval king check builds PINS bitmap and attacker bitmap including attacks rays against king position
// ext version does it for any position
// oth version removes temporarily king before building bitmaps
// full version builds also blocker_rays - between blocker and position

int eval_king_checks(board *b, king_eval *ke, personality *p, int side) {
int from;
	from=b->king[side];
//	assert((from>=0)&&(from<64));
	eval_king_checks_ext(b, ke, p, side, from);
return 0;
}

int eval_king_checks_oth(board *b, king_eval *ke, personality *p, int side, int from) {
int oldk;
	
	oldk=b->king[side];
//	assert((oldk>=0)&&(oldk<64));
// clear KING position	
	ClearAll(oldk, side, KING, b);
	eval_king_checks_ext(b, ke, p, side, from);
// restore old king position
	SetAll(oldk, side, KING, b);
return 0;
}

int eval_king_checks_full(board *b, king_eval *ke, personality *p, int side)
{
BITVAR cr2, di2, c2, d2, c, d, c3, d3, ob, c2s, d2s, c3s, bl_ray;

int from, ff, o, ee;
BITVAR pp,aa, cr_temp2, di_temp2;

		from=b->king[side];
//		assert((from>=0)&&(from<64));

		o= (side==0) ? BLACK:WHITE;

//		ke->ep_block=0;
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
		ke->cr_pins = ke->cr_attackers = ke->cr_att_ray = ke->cr_blocker_ray = 0;
		
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
						ke->cr_pins |=c3;
						bl_ray=attack.rays_int[from][ee]|normmark[ee];
						ke->cr_blocker_ray|=(bl_ray);
						break;
// 0 means attacked
					case 0:
						ke->cr_attackers |= normmark[ff];
						ke->cr_att_ray|=attack.rays_dir[ff][from];
						break;
					case 2:
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
						ke->di_pins |=d3;
//???
						bl_ray=attack.rays_int[from][ee]|normmark[ee];
						ke->di_blocker_ray|=(bl_ray);
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

int eval_king_checks_n_full(board *b, king_eval *ke, personality *p, int side)
{
//BITVAR d1, d2, d1r, r, blk, atk, atkx;
BITVAR d10, d20, r0, blk0, atk0, atk20, ar0, br0;
BITVAR d11, d21, r1, blk1, atk1, atk21, ar1, br1;
BITVAR d12, d22, r2, blk2, atk2, atk22, ar2, br2;
BITVAR d13, d23, r3, blk3, atk3, atk23, ar3, br3;
BITVAR d14,      r4, blk4, atk4, atk24, ar4, br4;
BITVAR d15,      r5, blk5, atk5, atk25, ar5, br5;
BITVAR d16,      r6, blk6, atk6, atk26, ar6, br6;
BITVAR d17,      r7, blk7, atk7, atk27, ar7, br7;

BITVAR di_att, di_block, cr_att, cr_block;
int pos, o, opp;

		pos=b->king[side];
//		assert((pos>=0)&&(pos<64));
		o= (side==0) ? BLACK:WHITE;

		ke->ep_block=0;
		ke->cr_pins = ke->cr_attackers = ke->cr_att_ray = 0;
		ke->di_pins = ke->di_attackers = ke->di_att_ray = 0;

// diagonal attackers
	di_att = b->colormaps[o] & (b->maps[QUEEN]|b->maps[BISHOP]);
// diagonal blockers
	di_block = b->norm&(~(di_att|(b->maps[KING]&b->colormaps[side])));
// to right up

	get45Rvector2(b->r45R, pos, &d11, &d21);
	r1 = attack.dirs[pos][1];
	r5 = attack.dirs[pos][5];
	atk1 = d11 & di_att;
	blk1 = d11 & di_block;
	atk21 = (d21&(~d11)) & di_att;
	ar1 = atk1 & r1;
	br1 = blk1 & r1;
	ar5 = atk1 & r5;
	br5 = blk1 & r5;

	ke->di_attackers |= atk1;
	ke->di_att_ray |= (ar1!=0)*(d11 & (~ar1));
//	ke->di_blocker_ray |= (ar1==0)*(d11 & r1);
	ke->di_pins |= (((atk21 & r1)!=0)&(br1!=0)) *br1;
	
	ke->di_att_ray |= (ar5!=0)*(d11 & (~ar5));
//	ke->di_blocker_ray |= (ar5==0)*(d11 & r5);
	ke->di_pins |= (((atk21 & r5)!=0)&(br5!=0)) *br5;

	get45Lvector2(b->r45L, pos, &d13, &d23);
	r3 = attack.dirs[pos][3];
	r7 = attack.dirs[pos][7];
	atk3 = d13 & di_att;
	blk3 = d13 & di_block;
	atk23 = (d23&(~d13)) & di_att;
	ar3 = atk3 & r3;
	br3 = blk3 & r3;
	ar7 = atk3 & r7;
	br7 = blk3 & r7;

	ke->di_attackers |= atk3;
	ke->di_att_ray |= (ar3!=0)*(d13 & (~ar3));
//	ke->di_blocker_ray |= (ar3==0)*(d13 & r3);
	ke->di_pins |= (((atk23 & r3)!=0)&(br3!=0)) *br3;
	
	ke->di_att_ray |= (ar7!=0)*(d13 & (~ar7));
//	ke->di_blocker_ray |= (ar7==0)*(d13 & r7);
	ke->di_pins |= (((atk23 & r7)!=0)&(br7!=0)) *br7;

// normal attackers
	cr_att = b->colormaps[o] & (b->maps[QUEEN]|b->maps[ROOK]);
// normal blocks
	cr_block = b->norm&(~(cr_att|(b->maps[KING]&b->colormaps[side])));

	getnormvector2(b->norm, pos, &d12, &d22);
	r2 = attack.dirs[pos][2];
	r6 = attack.dirs[pos][6];
	atk2 = d12 & cr_att;
	blk2 = d12 & cr_block;
	atk22 = (d22&(~d12)) & cr_att;
	ar2 = atk2 & r2;
	br2 = blk2 & r2;
	ar6 = atk2 & r6;
	br6 = blk2 & r6;

	ke->cr_attackers |= atk2;
	ke->cr_att_ray |= (ar2!=0)*(d12 & (~ar2));
//	ke->cr_blocker_ray |= (ar2==0)*(d12 & r2);
	ke->cr_pins |= (((atk22 & r2)!=0)&(br2!=0)) *br2;
	
	ke->cr_att_ray |= (ar6!=0)*(d12 & (~ar6));
//	ke->cr_blocker_ray |= (ar6==0)*(d12 & r6);
	ke->cr_pins |= (((atk22 & r6)!=0)&(br6!=0)) *br6;

// pokud je 1 utocnik, tak ulozit utocnika a cestu do att_ray
// pokud je 1 prazdno, tak ulozit cestu do blocker_ray
// pokud je 1 blocker, tak ulozit cestu do blocker_ray
// pokud je 1 blocker a 2 utocnik, tak cestu do blocker_ray a blocker do PIN

	get90Rvector2(b->r90R, pos, &d10, &d20);
	r0 = attack.dirs[pos][0];
	r4 = attack.dirs[pos][4];
	atk0 = d10 & cr_att;
	blk0 = d10 & cr_block;
	atk20 = (d20&(~d10)) & cr_att;
	ar0 = atk0 & r0;
	br0 = blk0 & r0;
	ar4 = atk0 & r4;
	br4 = blk0 & r4;

	ke->cr_attackers |= atk0;
	ke->cr_att_ray |= (ar0!=0)*(d10 & (~ar0));
//	ke->cr_blocker_ray |= (ar0==0)*(d10 & r0);
	ke->cr_pins |= (((atk20 & r0)!=0)&(br0!=0)) *br0;
	
	ke->cr_att_ray |= (ar4!=0)*(d10 & (~ar4));
//	ke->cr_blocker_ray |= (ar4==0)*(d10 & r4);
	ke->cr_pins |= (((atk20 & r4)!=0)&(br4!=0)) *br4;

// generating quiet check moves depends on di_blocker_ray and cr_blocker_ray containing all squares leading to king

// incorporate knights
		ke->kn_pot_att_pos=attack.maps[KNIGHT][pos];
		ke->kn_attackers=ke->kn_pot_att_pos & b->maps[KNIGHT] & b->colormaps[o];
//incorporate pawns
		ke->pn_pot_att_pos=attack.pawn_att[side][pos];
		ke->pn_attackers=ke->pn_pot_att_pos & b->maps[PAWN] & b->colormaps[o];
		ke->attackers=ke->cr_attackers | ke->di_attackers | ke->kn_attackers | ke->pn_attackers;

	return 0;
}

// att_ray - mezi utocnikem az za krale, bez utocnika
// blocker_ray - mezi blockerem a kralem, vcetne blockera
 
int eval_king_checks_n(board *b, king_eval *ke, personality *p, int side)
{
//BITVAR d1, d2, d1r, r, blk, atk, atkx;
BITVAR d10, d20, r0, blk0, atk0, atk20, ar0, br0;
BITVAR d11, d21, r1, blk1, atk1, atk21, ar1, br1;
BITVAR d12, d22, r2, blk2, atk2, atk22, ar2, br2;
BITVAR d13, d23, r3, blk3, atk3, atk23, ar3, br3;
BITVAR d14,      r4, blk4, atk4, atk24, ar4, br4;
BITVAR d15,      r5, blk5, atk5, atk25, ar5, br5;
BITVAR d16,      r6, blk6, atk6, atk26, ar6, br6;
BITVAR d17,      r7, blk7, atk7, atk27, ar7, br7;

BITVAR di_att, di_block, cr_att, cr_block;
int pos, o, opp;

		pos=b->king[side];
//		assert((pos>=0)&&(pos<64));
		o= (side==0) ? BLACK:WHITE;

		ke->ep_block=0;
		ke->cr_pins = ke->cr_attackers = ke->cr_att_ray = ke->cr_blocker_ray = 0;
		ke->di_pins = ke->di_attackers = ke->di_att_ray = ke->di_blocker_ray = 0;

// diagonal attackers
	di_att = b->colormaps[o] & (b->maps[QUEEN]|b->maps[BISHOP]);
// diagonal blockers
	di_block = b->norm&(~(di_att|(b->maps[KING]&b->colormaps[side])));
// to right up

	get45Rvector2(b->r45R, pos, &d11, &d21);
	r1 = attack.dirs[pos][1];
	r5 = attack.dirs[pos][5];
	atk1 = d11 & di_att;
	blk1 = d11 & di_block;
	atk21 = (d21&(~d11)) & di_att;
	ar1 = atk1 & r1;
	br1 = blk1 & r1;
	ar5 = atk1 & r5;
	br5 = blk1 & r5;

	ke->di_attackers |= atk1;
	ke->di_att_ray |= (ar1!=0)*(d11 & (~ar1));
//	ke->di_blocker_ray |= (ar1==0)*(d11 & r1);
	ke->di_pins |= (((atk21 & r1)!=0)&(br1!=0)) *br1;
	
	ke->di_att_ray |= (ar5!=0)*(d11 & (~ar5));
//	ke->di_blocker_ray |= (ar5==0)*(d11 & r5);
	ke->di_pins |= (((atk21 & r5)!=0)&(br5!=0)) *br5;

	get45Lvector2(b->r45L, pos, &d13, &d23);
	r3 = attack.dirs[pos][3];
	r7 = attack.dirs[pos][7];
	atk3 = d13 & di_att;
	blk3 = d13 & di_block;
	atk23 = (d23&(~d13)) & di_att;
	ar3 = atk3 & r3;
	br3 = blk3 & r3;
	ar7 = atk3 & r7;
	br7 = blk3 & r7;

	ke->di_attackers |= atk3;
	ke->di_att_ray |= (ar3!=0)*(d13 & (~ar3));
//	ke->di_blocker_ray |= (ar3==0)*(d13 & r3);
	ke->di_pins |= (((atk23 & r3)!=0)&(br3!=0)) *br3;
	
	ke->di_att_ray |= (ar7!=0)*(d13 & (~ar7));
//	ke->di_blocker_ray |= (ar7==0)*(d13 & r7);
	ke->di_pins |= (((atk23 & r7)!=0)&(br7!=0)) *br7;

// normal attackers
	cr_att = b->colormaps[o] & (b->maps[QUEEN]|b->maps[ROOK]);
// normal blocks
	cr_block = b->norm&(~(cr_att|(b->maps[KING]&b->colormaps[side])));

	getnormvector2(b->norm, pos, &d12, &d22);
	r2 = attack.dirs[pos][2];
	r6 = attack.dirs[pos][6];
	atk2 = d12 & cr_att;
	blk2 = d12 & cr_block;
	atk22 = (d22&(~d12)) & cr_att;
	ar2 = atk2 & r2;
	br2 = blk2 & r2;
	ar6 = atk2 & r6;
	br6 = blk2 & r6;

	ke->cr_attackers |= atk2;
	ke->cr_att_ray |= (ar2!=0)*(d12 & (~ar2));
//	ke->cr_blocker_ray |= (ar2==0)*(d12 & r2);
	ke->cr_pins |= (((atk22 & r2)!=0)&(br2!=0)) *br2;
	
	ke->cr_att_ray |= (ar6!=0)*(d12 & (~ar6));
//	ke->cr_blocker_ray |= (ar6==0)*(d12 & r6);
	ke->cr_pins |= (((atk22 & r6)!=0)&(br6!=0)) *br6;

// pokud je 1 utocnik, tak ulozit utocnika a cestu do att_ray
// pokud je 1 prazdno, tak ulozit cestu do blocker_ray
// pokud je 1 blocker, tak ulozit cestu do blocker_ray
// pokud je 1 blocker a 2 utocnik, tak cestu do blocker_ray a blocker do PIN

	get90Rvector2(b->r90R, pos, &d10, &d20);
	r0 = attack.dirs[pos][0];
	r4 = attack.dirs[pos][4];
	atk0 = d10 & cr_att;
	blk0 = d10 & cr_block;
	atk20 = (d20&(~d10)) & cr_att;
	ar0 = atk0 & r0;
	br0 = blk0 & r0;
	ar4 = atk0 & r4;
	br4 = blk0 & r4;

	ke->cr_attackers |= atk0;
	ke->cr_att_ray |= (ar0!=0)*(d10 & (~ar0));
//	ke->cr_blocker_ray |= (ar0==0)*(d10 & r0);
	ke->cr_pins |= (((atk20 & r0)!=0)&(br0!=0)) *br0;
	
	ke->cr_att_ray |= (ar4!=0)*(d10 & (~ar4));
//	ke->cr_blocker_ray |= (ar4==0)*(d10 & r4);
	ke->cr_pins |= (((atk20 & r4)!=0)&(br4!=0)) *br4;

// generating quiet check moves depends on di_blocker_ray and cr_blocker_ray containing all squares leading to king

// incorporate knights
		ke->kn_pot_att_pos=attack.maps[KNIGHT][pos];
		ke->kn_attackers=ke->kn_pot_att_pos & b->maps[KNIGHT] & b->colormaps[o];
//incorporate pawns
		ke->pn_pot_att_pos=attack.pawn_att[side][pos];
		ke->pn_attackers=ke->pn_pot_att_pos & b->maps[PAWN] & b->colormaps[o];
		ke->attackers=ke->cr_attackers | ke->di_attackers | ke->kn_attackers | ke->pn_attackers;

	return 0;
}

int eval_king_checks_all(board *b, attack_model *a)
{
//	eval_king_checks_n(b, (&W), NULL, WHITE);
//	eval_king_checks_n(b, (&B), NULL, BLACK);
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

	if((b->mindex_validity==1) && (p->mat_info[b->mindex][b->side]==0)) return 1;


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
			DEB_3(if(b->posnorm[i-b->move_start]!=b->norm)	printf("Error: Not matching position to hash!\n");)
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

int mat_setup(int px[2], int nx[2], int blx[2], int bdx[2], int rx[2], int qx[2], uint8_t tun[2])
{
int values[]={1000, 3500, 3500, 5000, 9750, 0};
int i, op;
int pieces, mm, mp, mx[2];
int pc[2], b[2];
int min[2], maj[2], m[2], m2[2];
int p[2], n[2], bl[2], bd[2], r[2], q[2];
	
	pieces=0;
	for(i=0;i<2;i++) {
		p[i]=px[i];
		n[i]=nx[i];
		bl[i]=blx[i];
		bd[i]=bdx[i];
		r[i]=rx[i];
		q[i]=qx[i];
		
		m[i]=p[i]*values[0]+n[i]*values[0]+(bl[i]+bd[i])*values[2]+r[i]*values[3]+q[i]*values[4];
		m2[i]=n[i]*values[0]+(bl[i]+bd[i])*values[2]+r[i]*values[3]+q[i]*values[4];
		pieces+=q[i]+r[i]+bd[i]+bl[i]+n[i];
		b[i]=bl[i]+bd[i];
		min[i]=n[i]+b[i];
		maj[i]=q[i]+r[i];
		pc[i]=min[i]+maj[i];
	}
//	LOGGER_0("W %d %d %d %d %d %d, B %d %d %d %d %d %d\n", p[0], n[0], bl[0], bd[0], r[0], q[0], p[1], n[1], bl[1], bd[1], r[1], q[1]);
	mm=m[0]-m[1];
	tun[0]=tun[1]=255;

	for(i=0;i<=1;i++) {
	  op = i == 0 ? 1 : 0;

// material wise I'm ahead
#if 0
		if(m[i]>=m[op]) {
// I have no pawns
			if(p[i]==0) {
				if(((pc[i]==1)&&(min[i]==1))||(pc[i]==0)) {
					tun[i]=0;
				} else if((pc[i]==2)&&(n[i]==2)&&(m[op]==0)&&(pc[op]==0)) {
					tun[i]=0;
				} else if ((pc[i]==1)&&(r[i]==1)&&(min[op]==1)&&(pc[op]==1)) {
					tun[i]=128;
				} else if ((pc[i]==2)&&(r[i]==1)&&(min[i]==1)&&(r[op]==1)&&(pc[op]==1)) {
					tun[i]=128;
				} 
				  else if(((m[i]-m[op])<=values[1])&&((pc[i]==2)&&(b[i]==2)&&(n[op]==1)&&(pc[op]==1))) {
					tun[i]=64;
				} else if(((m[i]-m[op])<=values[1])&&((min[op]<=3)&&(r[op]<=2)&&(q[op]<=1))) {
					tun[i]=32;
				} else if(((m[i]-m[op])>values[1])&&(m[op]>0)) {
					tun[op]=128;
				}
			} else if(p[i]==1) {
				if((pc[i]==1)&&(min[i]==1)&&((pc[op]==1)&&(min[op]==1))) {
					tun[i]=64;
				} else if((pc[i]==2)&&(n[i]==2)&&((pc[op]==1)&&(p[op]==0))) {
					tun[i]=DIV4;
				} else if(((m[i]-m[op])<=values[1])&&((min[op]<=4)&&(r[op]<=2)&&(q[op]<=1))) {
					tun[i]=128;
				}
			}
		}	
#endif
// material wise I'm ahead
		if((m[i]>m[op])&&(p[i]<2)) {
			if(p[i]==1) {
				if(pc[op]>0) {
					if(bl[op]>0) bl[op]--;
					else if(bd[op]>0) bd[op]--;
					else if(r[op]>0) r[op]--;
					else if(q[op]>0) q[op]--;
					m2[op]=n[op]*values[0]+(bl[op]+bd[op])*values[2]+r[op]*values[3]+q[op]*values[4];
				}
			}
			mp=pc[i] > 2 ? 1 : n[i]>=2 ? 0 : (m2[i]-m2[op])>values[2] ? 1 : 0 ;
			if(!mp) { tun[i] = p[i]==0 ? 16 : 32; }
			else { if ((p[i]==0) && (m2[op]>0)) tun[i]=128; }
			  
		}	
	}
return 0;
}

int mat_info(uint8_t info[][2])
{
int f;
	for(f=0;f<419999;f++) {
			info[f][0]=info[f][1]=255;
	}
// certain values known draw
//	m=MATidx(pw,pb,nw,nb,bwl,bwd,bbl,bbd,rw,rb,qw,qb);
// pw,pb,nw,nb,bwl,bwd,bbl,bbd,rw,rb,qw,qb, TYPE

int i,m;
int p[2], n[2], bl[2], bd[2], r[2], q[2], pp;
uint8_t tun[2];

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
													info[m][0]=tun[0];
													info[m][1]=tun[1];
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
													faze[m]=(uint8_t) fz & 255;
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

int meval_value(int pw, int pb, int nw, int nb, int bwl, int bwd, int bbl, int bbd, int rw, int rb, int qw, int qb, meval_t *t, personality *p, int stage)
{
int w, b, pp, scw, scb;
	w=pw*p->Values[stage][0]+nw*p->Values[stage][1]+(bwl+bwd)*p->Values[stage][2]+rw*p->Values[stage][3]+qw*p->Values[stage][4];
	b=pb*p->Values[stage][0]+nb*p->Values[stage][1]+(bbl+bbd)*p->Values[stage][2]+rb*p->Values[stage][3]+qb*p->Values[stage][4];

	pp=pw+pb;
	t->mat=(w-b);
	t->mat_w=w;

#if 1
	scw=p->dvalues[ROOK][pp]*rw+p->dvalues[KNIGHT][pp]*nw+p->dvalues[QUEEN][pp]*qw+p->dvalues[BISHOP][pp]*bwl;
	scb=p->dvalues[ROOK][pp]*rb+p->dvalues[KNIGHT][pp]*nb+p->dvalues[QUEEN][pp]*qb+p->dvalues[BISHOP][pp]*bbl;
	
	t->mat+=(scw-scb);
	t->mat_w+=scw;
	
#endif

return 0;
}

int meval_table_gen(meval_t *t, personality *p, int stage){
int pw, pb, nw, nb, bwl, bwd, bbl, bbd, rw, rb, qw, qb, f;
int m, w, b;
float mcount;

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
			t[f].mat_w=0;
			t[f].mat_o[WHITE]=0;
			t[f].mat_o[BLACK]=0;
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
													meval_value(pw,pb,nw,nb,bwl,bwd,bbl,bbd,rw,rb,qw,qb, t+m,p, stage);
//													LOGGER_0("pw %d,pb %d,nw %d,nb %d,bwl %d,bwd %d,bbl %d,bbd %d,rw %d,rb %d,qw %d,qb %d, m %x, stage %d, mat %d, mat_w %d\n",pw,pb,nw,nb,bwl,bwd,bbl,bbd,rw,rb,qw,qb, m, stage, t[m].mat, t[m].mat_w);
													t[m].mat_o[WHITE]=4*pw+12*nw+12*(bwl+bwd)+20*rw+39*qw;
													t[m].mat_o[BLACK]=4*pb+12*nb+12*(bbl+bbd)+20*rb+39*qb;
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

int get_material_eval(board *b, personality *p, int *mb, int *me, int *wb, int *we){
int stage, m, r;
int pw, pb, nw, nb, bwl, bwd, bbl, bbd, rw, rb, qw, qb;
int pp, nn, bb, rr, qq, scb, scw;

meval_t t, te;
	if(b->mindex_validity==1) {
//		assert((b->mindex>=0)&&(b->mindex<419999));
		*mb = p->mat[b->mindex].mat;
		*me = p->mate_e[b->mindex].mat;
		*wb = p->mat[b->mindex].mat_w;
		*we = p->mate_e[b->mindex].mat_w;
//		return 1;
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
		meval_value(pw,pb,nw,nb,bwl,bwd,bbl,bbd,rw,rb,qw,qb, &t,p, stage);
		*mb=t.mat;
		*wb=t.mat_w;
		
		stage=1;
		meval_value(pw,pb,nw,nb,bwl,bwd,bbl,bbd,rw,rb,qw,qb, &t,p, stage);
		*me=t.mat;
		*we=t.mat_w;
	
		bwl=b->material[WHITE][BISHOP];
		bbl=b->material[BLACK][BISHOP];
		pw=b->material[WHITE][PAWN];
		pb=b->material[BLACK][PAWN];
		nw=b->material[WHITE][KNIGHT];
		nb=b->material[BLACK][KNIGHT];
		rw=b->material[WHITE][ROOK];
		rb=b->material[BLACK][ROOK];
		qw=b->material[WHITE][QUEEN];
		qb=b->material[BLACK][QUEEN];

		pp=pw+pb;
		scw=p->dvalues[ROOK][pp]*rw+p->dvalues[KNIGHT][pp]*nw+p->dvalues[QUEEN][pp]*qw+p->dvalues[BISHOP][pp]*bwl;
		scb=p->dvalues[ROOK][pp]*rb+p->dvalues[KNIGHT][pp]*nb+p->dvalues[QUEEN][pp]*qb+p->dvalues[BISHOP][pp]*bbl;
		(*mb)+=(scw-scb);
		(*me)+=(scw-scb);
		(*wb)+=(scw);
		(*we)+=(scw);
	}

return 2;
}

int get_material_eval_f(board *b, personality *p){
int score, sc2;
int me,mb, we, wb;
int phase = eval_phase(b, p);


//	b->mindex_validity=0;
	get_material_eval(b, p, &mb, &me, &wb, &we);
	score=(mb*phase+me*(255-phase))/255;

	return score;
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
		a->sc.side[side].specs_b+=a->specs[side][ROOK].sqr_b;
		a->sc.side[side].specs_e+=a->specs[side][ROOK].sqr_e;
	}
	return 0;
}

int eval_pawn(board *b, attack_model *a, PawnStore *ps, int side, personality *p){
int piece, heavy_op;
int from;
int srank;
int z,f;
int opside;
BITVAR v,n;

	piece = (side == WHITE) ? PAWN : PAWN|BLACKPIECE;
// add stuff related to other pieces esp heavy opp pieces
	heavy_op=(b->maps[ROOK]|b->maps[QUEEN])&b->colormaps[Flip(side)];
//	heavy_op=0;
	if(heavy_op) {
		a->sc.side[side].sqr_b +=ps->score[side][HEa].sqr_b;
		a->sc.side[side].sqr_e +=ps->score[side][HEa].sqr_e;
	} else {
		a->sc.side[side].sqr_b +=ps->score[side][BAs].sqr_b;
		a->sc.side[side].sqr_e +=ps->score[side][BAs].sqr_e;
	}
	return 0;
}

int eval_king2(board *b, attack_model *a, PawnStore *ps, int side, personality *p){
int from, m, to, sl, row;
int heavy_op;
BITVAR mv;

int opmat_o, mat_o_tot;

	a->specs[side][KING].sqr_b=0;
	a->specs[side][KING].sqr_e=0;
	from=b->king[side];
	opmat_o=mat_o_tot=128;

#if 1
	if(b->mindex_validity==1) {
		mat_o_tot=p->mat[MAXMAT_IDX].mat_o[WHITE]+p->mat[MAXMAT_IDX].mat_o[BLACK];
		opmat_o=p->mat[b->mindex].mat_o[Flip(side)];
	}
#endif

// king mobility, spocitame vsechna pole kam muj kral muze (tj. krome vlastnich figurek a poli na ktere utoci nepratelsky kral
// a poli ktera jsou napadena cizi figurou
	mv = (attack.maps[KING][from]) & (~b->colormaps[side]) & (~attack.maps[KING][b->king[side^1]]);
	mv = mv & (~a->att_by_side[side^1]) & (~a->ke[side].cr_att_ray) & (~a->ke[side].di_att_ray);

	m=a->me[from].pos_att_tot=BitCount(mv);
// king square mobility
	a->me[from].pos_mob_tot_b=p->mob_val[0][side][KING][m];
	a->me[from].pos_mob_tot_e=p->mob_val[1][side][KING][m];
// king square PST
	a->sq[from].sqr_b=p->piecetosquare[0][side][KING][from];
	a->sq[from].sqr_e=p->piecetosquare[1][side][KING][from];

	heavy_op=(b->maps[ROOK]|b->maps[QUEEN])&b->colormaps[Flip(side)];
//	heavy_op=0;

#if 1
// evalute shelter
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
	a->sc.side[side].specs_b +=a->specs[side][KING].sqr_b;
	a->sc.side[side].specs_e +=a->specs[side][KING].sqr_e;
	return 0;
}

int eval_inter_bishop(board *b, attack_model *a, PawnStore *ps, int side, personality *p){
int piece;
int from;
int f;

	if((b->material[side][BISHOP+ER_PIECE]>0)&&(b->material[side][BISHOP]>0)) {
		a->sc.side[side].specs_b+=p->bishopboth[0];
		a->sc.side[side].specs_e+=p->bishopboth[1];
	}
	return 0;
}

int eval_inter_rook(board *b, attack_model *a, PawnStore *ps, int side, personality *p){
int piece;
int from;
int f;

	if(b->material[side][ROOK]>1) {
		a->sc.side[side].specs_b+=p->rookpair[0];
		a->sc.side[side].specs_e+=p->rookpair[1];
	}
	return 0;
}

int eval_inter_knight(board *b, attack_model *a, PawnStore *ps, int side, personality *p){
int piece;
int from;
int f;

	if(b->material[side][KNIGHT]>1) {
		a->sc.side[side].specs_b+=p->knightpair[0];
		a->sc.side[side].specs_e+=p->knightpair[1];
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
int eval_x(board* b, attack_model *a, personality* p) {
int f, from, temp_b, temp_e;
int score_b, score_e, wb, we;
PawnStore pps, *ps;
attack_model ATT;
	
//	ps=&pps;
	ps=&(a->pps);

	a->phase = eval_phase(b, p);
// setup pawn attacks

/*
 *  pawn attacks and moves require cr_pins, di_pins setup
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
	make_mobility_modelN(b, a, p);

// build pawn mode + pawn cache + evaluate + pre comupte pawn king shield
	premake_pawn_model(b, a, ps, p);

// compute material	
	get_material_eval(b, p, &a->sc.material, &a->sc.material_e, &a->sc.material_b_w, &a->sc.material_e_w);
//LOGGER_0("MAT b_tot:%d, e_tot:%d, b_w:%d, e_w:%d\n", a->sc.material, a->sc.material_e, a->sc.material_b_w, a->sc.material_e_w);
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
	eval_inter_bishop(b, a, ps, WHITE, p);
	eval_inter_bishop(b, a, ps, BLACK, p);
	eval_inter_knight(b, a, ps, WHITE, p);
	eval_inter_knight(b, a, ps, BLACK, p);
	eval_inter_rook(b, a, ps, WHITE, p);
	eval_inter_rook(b, a, ps, BLACK, p);

// side to move tempo bonus
	if(b->side==WHITE) {
		temp_b=p->move_tempo[0];
		temp_e=p->move_tempo[1];
	} else {
		temp_b=-p->move_tempo[0];
		temp_e=-p->move_tempo[1];
	}

	if(p->simple_EVAL==1) {
// simplified eval - Material and PST only
//		LOGGER_0("simple EVAL %d %d\n", a->sc.side[0].sqr_b, a->sc.material_b_w);
		a->sc.score_b_w=a->sc.side[0].sqr_b+a->sc.material_b_w;
		a->sc.score_b_b=a->sc.side[1].sqr_b+a->sc.material_b_w-a->sc.material;
		a->sc.score_e_w=a->sc.side[0].sqr_e+a->sc.material_e_w;
		a->sc.score_e_b=a->sc.side[1].sqr_e+a->sc.material_e_w-a->sc.material_e;
		a->sc.score_b=a->sc.score_b_w-a->sc.score_b_b+temp_b;
		a->sc.score_e=a->sc.score_e_w-a->sc.score_e_b+temp_e;
		a->sc.score_nsc=a->sc.score_b*a->phase+a->sc.score_e*(255-a->phase);
	} else {
#if 1
		a->sc.score_b_w=a->sc.side[0].mobi_b+a->sc.side[0].sqr_b+a->sc.side[0].specs_b+a->sc.material_b_w;
		a->sc.score_b_b=a->sc.side[1].mobi_b+a->sc.side[1].sqr_b+a->sc.side[1].specs_b+a->sc.material_b_w-a->sc.material;
		a->sc.score_e_w=a->sc.side[0].mobi_e+a->sc.side[0].sqr_e+a->sc.side[0].specs_e+a->sc.material_e_w;
		a->sc.score_e_b=a->sc.side[1].mobi_e+a->sc.side[1].sqr_e+a->sc.side[1].specs_e+a->sc.material_e_w-a->sc.material_e;
		a->sc.score_b=a->sc.score_b_w-a->sc.score_b_b+temp_b;
		a->sc.score_e=a->sc.score_e_w-a->sc.score_e_b+temp_e;
		a->sc.score_nsc=a->sc.score_b*a->phase+a->sc.score_e*(255-a->phase);
#endif
	}
	return a->sc.score_nsc;
}

void eval_lnk(board* b, attack_model *a, int piece, int side, int pp) {
BITVAR x;
	x = (b->maps[piece]&b->colormaps[side]);
	a->pos_c[pp] = -1;
	while (x) {
		a->pos_c[pp]++;
		a->pos_m[pp][a->pos_c[pp]]=LastOne(x);
		ClrLO(x);
	}
}

// just testing 
int mpsq_eval(board* b, attack_model *a, personality* p) {
attack_model aaa, *aa;
int score;
		aa=&aaa;
		aa->phase = eval_phase(b, p);
		aa->sc.score_b_w=a->sc.side[0].sqr_b+a->sc.material_b_w;
		aa->sc.score_b_b=a->sc.side[1].sqr_b+a->sc.material_b_w-a->sc.material;
		aa->sc.score_e_w=a->sc.side[0].sqr_e+a->sc.material_e_w;
		aa->sc.score_e_b=a->sc.side[1].sqr_e+a->sc.material_e_w-a->sc.material_e;
		aa->sc.score_b=aa->sc.score_b_w-aa->sc.score_b_b;
		aa->sc.score_e=aa->sc.score_e_w-aa->sc.score_e_b;
		aa->sc.score_nsc=aa->sc.score_b*aa->phase+a->sc.score_e*(255-aa->phase);
		
		aa->sc.scaling=256;
		if((b->mindex_validity==1)&&(((b->side==WHITE)&&(aa->sc.score_nsc>0))||((b->side==BLACK)&&(aa->sc.score_nsc<0)))) {
		aa->sc.scaling=(p->mat_info[b->mindex][b->side]);
		}
	score=aa->sc.score_nsc*aa->sc.scaling/255;
	aa->sc.complete = score / 255;
return aa->sc.score_nsc / 255;
}

int psq_eval(board *b, attack_model *a, personality *p)
{
BITVAR x;
int square;
int side;
int piece;
int val;

	a->sc.side[0].sqr_b=0;
	a->sc.side[1].sqr_b=0;
	a->sc.side[0].sqr_e=0;
	a->sc.side[1].sqr_e=0;
	x=b->norm;
	while(x) {
		square=LastOne(x);
		piece=b->pieces[square]&PIECEMASK;
		side = (b->pieces[square]&BLACKPIECE)!=0 ? BLACK : WHITE;
		a->sq[square].sqr_b=p->piecetosquare[0][side][piece][square];
		a->sq[square].sqr_e=p->piecetosquare[1][side][piece][square];
			a->sc.side[side].sqr_b+=a->sq[square].sqr_b;
			a->sc.side[side].sqr_e+=a->sq[square].sqr_e;
		ClrLO(x);
	}
	a->phase=eval_phase(b,p);
	a->sc.scaling=255;
	b->psq_b=a->sc.side[WHITE].sqr_b-a->sc.side[BLACK].sqr_b;
	b->psq_e=a->sc.side[WHITE].sqr_e-a->sc.side[BLACK].sqr_e;
	val=(b->psq_b)*a->phase+(b->psq_e)*(255-a->phase);
	return val/255;
}

int eval(board* b, attack_model *a, personality* p) {
long score;
int8_t vi;
int f;
//attack_model ATT;
int tmp;

	for(f=ER_PIECE;f>=PAWN;f--) {
		a->pos_c[f]=-1;
		a->pos_c[f|BLACKPIECE]=-1;
	}

	eval_lnk(b, a, ROOK, WHITE, ROOK);
	eval_lnk(b, a, ROOK, BLACK, ROOK+BLACKPIECE);
	eval_lnk(b, a, KNIGHT, WHITE, KNIGHT);
	eval_lnk(b, a, KNIGHT, BLACK, KNIGHT+BLACKPIECE);
	eval_lnk(b, a, BISHOP, WHITE, BISHOP);
	eval_lnk(b, a, BISHOP, BLACK, BISHOP+BLACKPIECE);
	eval_lnk(b, a, QUEEN, WHITE, QUEEN);
	eval_lnk(b, a, QUEEN, BLACK, QUEEN+BLACKPIECE);
	
	eval_x(b, a, p);

	a->sc.scaling=255;
	
// scaling
	score = a->sc.score_nsc+p->eval_BIAS+p->eval_BIAS_e;
	if((b->mindex_validity==1)&&(((b->side==WHITE)&&(score>0))||((b->side==BLACK)&&(score<0)))) {
		a->sc.scaling=(p->mat_info[b->mindex][b->side]);
	}
	score=(score*a->sc.scaling)/255;
	a->sc.complete = score / 255;

#if 0
			LOGGER_0("mat %d, mob %d, mob %d, sqr %d, sqr %d, spc %d, spc %d\n", a->sc.material,a->sc.side[0].mobi_b, a->sc.side[1].mobi_b, a->sc.side[0].sqr_b, a->sc.side[1].sqr_b, a->sc.side[0].specs_b, a->sc.side[1].specs_b );
			LOGGER_0("mat %d, mob %d, mob %d, sqr %d, sqr %d, spc %d, spc %d\n", a->sc.material_e,a->sc.side[0].mobi_e,a->sc.side[1].mobi_e,a->sc.side[0].sqr_e, a->sc.side[1].sqr_e, a->sc.side[0].specs_e, a->sc.side[1].specs_e );
			LOGGER_0("score %d, phase %d, score_b %d, score_e %d\n", a->sc.complete / 255, a->phase, a->sc.score_b, a->sc.score_e);
#endif

	return a->sc.complete;
}

int lazyEval(board* b, attack_model *a, int alfa, int beta, int side, int ply, int depth, personality* p, int *fullrun){
int scr, sc4, sc3, sc2;
int mb, me, wb, we;

	a->phase = eval_phase(b, p);
	*fullrun=0;
	
	get_material_eval(b, p, &mb, &me, &wb, &we);
	sc4=(mb*a->phase+me*(255-a->phase))/255;
	
//	LOGGER_0("EVAL SC4 %d, phase %d, PsqB %d, PsqE %d\n", sc4, a->phase, b->psq_b, b->psq_e);
	sc3=(b->psq_b*a->phase+b->psq_e*(255-a->phase))/255;
	sc2=sc3+sc4;
	if(((sc2+p->lazy_eval_cutoff) < alfa)||(sc2>(beta+p->lazy_eval_cutoff))) {
		scr= sc2;
		LOGGER_4("score %d, mat %d, psq %d, alfa %d, beta %d, cutoff %d\n", sc2, sc4, sc3, alfa, beta, p->lazy_eval_cutoff);
	}
	else {
		*fullrun=1;
		a->att_by_side[side]=KingAvoidSQ(b, a, side);
		eval_king_checks(b, &(a->ke[Flip(b->side)]), NULL, Flip(b->side));
		simple_pre_movegen_n2(b, a, Flip(side));
		simple_pre_movegen_n2(b, a, side);
		eval(b, a, b->pers);
		scr= a->sc.complete;
	}

//	sc32=psq_eval(b, att, b->pers);
//LOGGER_0("Score Ru: %d, Cp: %d, DIF %d, MaF %d, PsR %d, PsC %d, DIF %d\n", scrN, scr, scrN-scr, sc4, sc3, sc32, sc3-sc32);

	if(side==WHITE) return scr; else return 0-scr;
}

//
//
//
//  Pxp, PxP, PxP, RxB, BxR, QxB, QxQ
// G:1 ,  0 ,  1 ,  0 ,  5 ,  -2,  12, -2 
//   1  -1,  1 , -5,   5 , -12,
//  Pxp, BxP, RxP  ?xR
//  1,   0,   3,   2,
//#define PackMove(from,to,prom,spec)  ((((from)&63) + (((to)&63) << 6) + (((prom)&7) << 12))|((spec)&(CHECKFLAG)))

int SEE(board * b, MOVESTORE m) {
int fr, to, side,d;
int gain[32];
BITVAR ignore, bto, ppromote;
int attacker;
int piece;
int king, kside;

	ignore=FULLBITMAP;
	fr=UnPackFrom(m);
	to=UnPackTo(m);
	bto=normmark[to];
	ppromote=(RANK1|RANK8)&bto;
	side=(b->pieces[fr]&BLACKPIECE)!=0;
	d=0;
	piece=b->pieces[to]&PIECEMASK;
	gain[d]= ((piece>=PAWN)&&(piece<KING)) ? b->pers->Values[0][piece] : 1000000;
	attacker=fr;
	while (attacker!=-1) {
		d++;
		piece= b->pieces[attacker]&PIECEMASK;
		gain[d]=-gain[d-1]+b->pers->Values[0][piece];
		if((ppromote) && (piece==PAWN)) gain[d]=-gain[d-1]+b->pers->Values[0][QUEEN]-b->pers->Values[0][PAWN];
		if(piece==KING) {
// king must be the last attacker
			side^=1;
			ignore^=normmark[attacker];
			attacker=GetLVA_to(b, to, side, ignore);
			if(attacker!=-1) gain[d]=-gain[d-1]+1000000;
//			break;
			continue;
		}
		if(Max(-gain[d-1], gain[d]) < 0) break;
		side^=1;
		ignore^=normmark[attacker];
		attacker=GetLVA_to(b, to, side, ignore);
	}
	while(--d) {
		gain[d-1]= -Max(-gain[d-1], gain[d]);
	}
	return gain[0];
}

/*
 * SEE after piece moved to to
 */


int SEE0(board *b, int to, int side, int val) {
int fr, d;
int gain[32];
BITVAR ignore, bto, ppromote;
int attacker;
int piece;
int king, kside;

	ignore=FULLBITMAP;
	bto=normmark[to];
	ppromote=(RANK1|RANK8)&bto;
//	side=(b->pieces[to]&BLACKPIECE)==0;
	side^=1;
	d=0;
//	printBoardNice(b);
//	LOGGER_0("ATT to %o, side %d\n", to, side);
	piece=b->pieces[to]&PIECEMASK;
	gain[d++]=val;
	gain[d]= ((piece>=PAWN)&&(piece<KING)) ? -val+b->pers->Values[0][piece] : b->pers->Values[0][piece] ;
	attacker=GetLVA_to(b, to, side, ignore);
	while (attacker!=-1) {
		d++;
//		LOGGER_0("ATT %o\n", attacker);
		piece= b->pieces[attacker]&PIECEMASK;
		gain[d]=-gain[d-1]+b->pers->Values[0][piece];
//		LOGGER_0("G %d:%d\n", d, gain[d]);
		if((ppromote) && (piece==PAWN)) gain[d]=-gain[d-1]+b->pers->Values[0][QUEEN]-b->pers->Values[0][PAWN];
		if(piece==KING) {
// king must be the last attacker
			side^=1;
			ignore^=normmark[attacker];
			attacker=GetLVA_to(b, to, side, ignore);
//			if(attacker!=-1) d--;
			if(attacker!=-1) gain[d]=1000000;
//			break;
			continue;
		}
//		if(Max(-gain[d-1], gain[d]) < 0) break;
		side^=1;
		ignore^=normmark[attacker];
		attacker=GetLVA_to(b, to, side, ignore);
	}
//	if(d<1) return 0;
	while(--d) {
//		LOGGER_0("Go d-1 %d:%d, d:%d\n", d-1, gain[d-1],gain[d]);
		gain[d-1]= -Max(-gain[d-1], gain[d]);
//		LOGGER_0("Gn d-1 %d:%d\n", d-1, gain[d-1]);
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
		
//		LOGGER_0("wp %d,bp %d,wn %d,bn %d, bwl %d, bwd %d, bbl %d, bbd %d, wr %d, br %d, wq %d, bq %d\n", b->material[WHITE][PAWN],b->material[BLACK][PAWN],b->material[WHITE][KNIGHT], \
//			b->material[BLACK][KNIGHT],bwl,bwd,bbl,bbd,b->material[WHITE][ROOK],b->material[BLACK][ROOK], \
//			b->material[WHITE][QUEEN],b->material[BLACK][QUEEN]);
		b->mindex=MATidx(b->material[WHITE][PAWN],b->material[BLACK][PAWN],b->material[WHITE][KNIGHT], \
			b->material[BLACK][KNIGHT],bwl,bwd,bbl,bbd,b->material[WHITE][ROOK],b->material[BLACK][ROOK], \
			b->material[WHITE][QUEEN],b->material[BLACK][QUEEN]);
		b->mindex_validity=1;
	}
return 1;
}

// move ordering is to get the fastest beta cutoff
int MVVLVA_gen(int table[ER_PIECE+2][ER_PIECE], _values Values)
{
int v[ER_PIECE];
int vic, att;
	v[PAWN]=P_OR;
	v[KNIGHT]=N_OR;
	v[BISHOP]=B_OR;
	v[ROOK]=R_OR;
	v[QUEEN]=Q_OR;
	v[KING]=K_OR_M;
	for(vic=PAWN;vic<ER_PIECE;vic++) {
		for(att=PAWN;att<ER_PIECE;att++) {
// all values inserted are positive!
			if(vic==att) {
				table[att][vic]=A_OR+(7*v[att]-v[att])*2;
			} else if(vic>att) {
				table[att][vic]=A_OR+(7*v[vic]-v[att])*2;
			} else if(vic<att) {
				table[att][vic]=A_OR2+(7*v[vic]-v[att])*2;
			}
		}
	}
#if 1
// lines for capture+promotion
// to queen
	for(vic=PAWN;vic<ER_PIECE;vic++) {
		att=PAWN;
		table[KING+1][vic]=A_OR+(7*v[vic]-v[PAWN]+v[QUEEN])*2;
	}
// to knight
	for(vic=PAWN;vic<ER_PIECE;vic++) {
		att=PAWN;
		table[KING+2][vic]=A_OR+(7*v[vic]-v[PAWN]+v[KNIGHT])*2;
	}
#endif

return 0;
}

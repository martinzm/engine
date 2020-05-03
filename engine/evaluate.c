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
//	a->att_by_side[side]=0;
	q=0;

// rook
	x = (b->maps[ROOK]&b->colormaps[side]);
	pp=ROOK+add;
	while (x) {
		from = LastOne(x);
//		pp=b->pieces[from];
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
//		pp=b->pieces[from];
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
//		pp=b->pieces[from];
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
//		pp=b->pieces[from];
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
 */

int make_model(board *b, attack_model *a, personality *p)
{
int from, pp, m, m2, s, z;
BITVAR x, q, v, n, a1[2], avoid[2], unsafe[2];

//	printBoardNice(b);
//	boardCheck(b);

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
	a->specs[0][ROOK].sqr_b=a->specs[1][ROOK].sqr_b=0;
	a->specs[0][ROOK].sqr_e=a->specs[1][ROOK].sqr_e=0;
	while (x) {
		from = LastOne(x);
		pp=b->pieces[from];
		s=(pp&BLACKPIECE)!=0;
		a->pos_c[pp]++;
		a->pos_m[pp][a->pos_c[pp]]=from;
		q=a->mvs[from] = (RookAttacks(b, from));
		a->att_by_side[s]|=q;
		m=a->me[from].pos_att_tot=BitCount(q & avoid[s]);
//		a->me[from].mob_count=BitCount(q & ~b->colormaps[s]);
		m2=BitCount(q & avoid[s] & unsafe[s]);
//		LOGGER_0("ROOK only EVAL: m %d, m2 %d, s %d, value\n", m, m2, s);
//		LOGGER_0("ROOK only EVAL: m %d, m2 %d, s %d, value %d\n", m, m2, s, p->mob_val[1][s][ROOK][0]);
		a->me[from].pos_mob_tot_b=p->mob_val[0][s][ROOK][m-m2];
		a->me[from].pos_mob_tot_e=p->mob_val[1][s][ROOK][m-m2];
		if(p->mobility_unsafe==1) {
			a->me[from].pos_mob_tot_b+=p->mob_uns[0][s][ROOK][m2];
			a->me[from].pos_mob_tot_e+=p->mob_uns[1][s][ROOK][m2];
		}
		a->sq[from].sqr_b=p->piecetosquare[0][s][ROOK][from];
		a->sq[from].sqr_e=p->piecetosquare[1][s][ROOK][from];
		z=getRank(from);
		if(((s==WHITE)&&(z==6))||((s==BLACK)&&(z==1))) {
			a->specs[s][ROOK].sqr_b+=p->rook_on_seventh[0];
			a->specs[s][ROOK].sqr_e+=p->rook_on_seventh[1];
		}

		n=attack.file[from];
		v = (s==0) ? n&b->maps[PAWN]&attack.uphalf[from] : n&b->maps[PAWN]&attack.downhalf[from];
#if 1
		if(v==0) {
			a->specs[s][ROOK].sqr_b+=p->rook_on_open[0];
			a->specs[s][ROOK].sqr_e+=p->rook_on_open[1];
		}

		else if((v&b->colormaps[s])==0) {
				a->specs[s][ROOK].sqr_b+=p->rook_on_semiopen[0];
				a->specs[s][ROOK].sqr_e+=p->rook_on_semiopen[1];
		}
#endif
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
//		a->me[from].mob_count=BitCount(q & ~b->colormaps[s]);
		a->att_by_side[s]|=q;
		m=a->me[from].pos_att_tot=BitCount(q & avoid[s]);
		m2=BitCount(q & avoid[s] & unsafe[s]);
		a->me[from].pos_mob_tot_b=p->mob_val[0][s][BISHOP][m-m2];
		a->me[from].pos_mob_tot_e=p->mob_val[1][s][BISHOP][m-m2];
		if(p->mobility_unsafe==1) {
			a->me[from].pos_mob_tot_b+=p->mob_uns[0][s][BISHOP][m2];
			a->me[from].pos_mob_tot_e+=p->mob_uns[1][s][BISHOP][m2];
		}
		a->sq[from].sqr_b=p->piecetosquare[0][s][BISHOP][from];
		a->sq[from].sqr_e=p->piecetosquare[1][s][BISHOP][from];
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
//		m=a->me[from].mob_count=BitCount(q & ~b->colormaps[s]);
		a->att_by_side[s]|=q;
		m=a->me[from].pos_att_tot=BitCount(q & avoid[s]);
		m2=BitCount(q & avoid[s] & unsafe[s]);
		a->me[from].pos_mob_tot_b=p->mob_val[0][s][KNIGHT][m-m2];
		a->me[from].pos_mob_tot_e=p->mob_val[1][s][KNIGHT][m-m2];
		if(p->mobility_unsafe==1) {
			a->me[from].pos_mob_tot_b+=p->mob_uns[0][s][KNIGHT][m2];
			a->me[from].pos_mob_tot_e+=p->mob_uns[1][s][KNIGHT][m2];
		}
		a->sq[from].sqr_b=p->piecetosquare[0][s][KNIGHT][from];
		a->sq[from].sqr_e=p->piecetosquare[1][s][KNIGHT][from];
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
//		a->me[from].mob_count=BitCount(q & ~b->colormaps[s]);
		a->att_by_side[s]|=q;
		m=a->me[from].pos_att_tot=BitCount(q & avoid[s]);
		m2=BitCount(q & avoid[s] & unsafe[s]);
//		printf("QUEEN eval %d %d, %d\n",BitCount(q), m, m2);
		a->me[from].pos_mob_tot_b=p->mob_val[0][s][QUEEN][m-m2];
		a->me[from].pos_mob_tot_e=p->mob_val[1][s][QUEEN][m-m2];
		if(p->mobility_unsafe==1) {
			a->me[from].pos_mob_tot_b+=p->mob_uns[0][s][QUEEN][m2];
			a->me[from].pos_mob_tot_e+=p->mob_uns[1][s][QUEEN][m2];
		}
		a->sq[from].sqr_b=p->piecetosquare[0][s][QUEEN][from];
		a->sq[from].sqr_e=p->piecetosquare[1][s][QUEEN][from];
		ClrLO(x);
	}
//	boardCheck(b);
return 0;
}

int make_pawn_model(board *b, attack_model *a, personality *p) {

int from, pp, s, cc;
BITVAR x, n, ob, sb, bc, dd, from_b, w_max, b_max, b1, b2, w1, w2, fin[2], xx, x_f[2], x_ff[2], x_p[2], t, nt, ntt, z, prot, tx[2];
int pathlen, fin_b, fin_e, count;

hashPawnEntry hash;

	a->specs[0][PAWN].sqr_b=0;
	a->specs[0][PAWN].sqr_e=0;
	a->specs[1][PAWN].sqr_b=0;
	a->specs[1][PAWN].sqr_e=0;
	
	hash.key=b->pawnkey;
	hash.map=b->maps[PAWN];
//	if(b->hps!=NULL) retrievePawnHash(b->hps, &hash, b->stats);

	a->pos_c[PAWN]=-1;
	a->pos_c[PAWN|BLACKPIECE]=-1;
	x = tx[0] = b->maps[PAWN]&b->colormaps[WHITE];
	fin_b=fin_e=0;

// blocked - normalni pesec
// doubled - blokuje mne vlastni pesec
// passed - zadny nepratelsky pesec nemuze p sebrat a ten muze dojit az do damy
// weak - pesec ktereho neni mozno branit vlastnimi pesci
	// isolated - nema po stranach vlastni pesce
	// backward - weak jenz po ceste muze byt sebran nepratelskym pescem

	w_max=(b->maps[PAWN])|((a->pa_at[BLACK])&(~a->pa_at[WHITE]));
	b_max=(b->maps[PAWN])|((a->pa_at[WHITE])&(~a->pa_at[BLACK]));
	
	x_f[WHITE]=FillNorth(b->maps[PAWN]&b->colormaps[WHITE],~w_max, 0);
	x_f[BLACK]=FillSouth(b->maps[PAWN]&b->colormaps[BLACK],~b_max, 0);
	x_ff[WHITE]=(x_f[WHITE]|(b->maps[PAWN]&b->colormaps[WHITE]))<<8;
	x_ff[BLACK]=(x_f[BLACK]|(b->maps[PAWN]&b->colormaps[BLACK]))>>8;
	x_p[WHITE]=x_f[WHITE] & attack.rank[A8];
	x_p[BLACK]=x_f[BLACK] & attack.rank[A1];
	
// x_f path to stop
// x_ff path to stop including stop
// x_p which files reached promotion rank?

	w1=(x_f[WHITE] &(~FILEH))<<9;
	w2=(x_f[WHITE] &(~FILEA))<<7;
	b1=(x_f[BLACK] &(~FILEH))>>7;
	b2=(x_f[BLACK] &(~FILEA))>>9;

// kteri pesci mohou byt chraneni?
	fin[WHITE] = (b->maps[PAWN]&b->colormaps[WHITE]) & (w1|w2);
	fin[BLACK] = (b->maps[PAWN]&b->colormaps[BLACK]) & (b1|b2);

	pp=PAWN;
	for(s=WHITE;s<=BLACK;s++) {
		x = sb = b->maps[PAWN]&b->colormaps[s];
//		pp=PAWN;
//		if(s==BLACK) pp=PAWN|BLACKPIECE;

		while (x) {
/*
 * reasons for stop
 * promotion true in x_p
 * pawn enemy/mine x_ff 
 * attacked square
 */
			from = LastOne(x);
			a->pos_c[pp]++;
			a->pos_m[pp][a->pos_c[pp]]=from;
			a->sq[from].sqr_b=p->piecetosquare[0][s][PAWN][from];
			a->sq[from].sqr_e=p->piecetosquare[1][s][PAWN][from];
			a->me[from].pos_mob_tot_b = a->me[from].pos_mob_tot_e = a->me[from].pos_att_tot=0;
// disabling pawn scoring
			if(p->simple_EVAL==0) {
				t = x_f[s];
				n = attack.passed_p[s][from]; // forward span
				dd = attack.file[from];
				z=dd&n; // path to promotion from from
				nt=z&t; // is path to stop the same as to path to promotion?
				from_b=normmark[from];
// z contains path to promotion square
// t contains path forward to stop point
				if(((nt)==z)){
					pathlen=BitCount(nt);
// max index 0-5, path is minimum 1 square, 6 at max
					a->sq[from].sqr_b+=p->passer_bonus[0][s][pathlen-1];
					a->sq[from].sqr_e+=p->passer_bonus[1][s][pathlen-1];
					fin_b+=p->passer_bonus[0][s][pathlen-1];
					fin_e=p->passer_bonus[1][s][pathlen-1];
					fin[s]|=(from_b);
				} else {
// get blocker - could be pawn or just attack from pawn
					ntt=x_ff[s]&z;
				//blockers
				// pawns in my way?
					if(ntt &b->maps[PAWN]) {
					// blocked...
						if(ntt & sb) {
						// doubled?
							a->sq[from].sqr_b+=p->doubled_penalty[0];
							a->sq[from].sqr_e+=p->doubled_penalty[1];
							fin_b+=p->doubled_penalty[0];
							fin_e+=p->doubled_penalty[1];
						} else {
						// blocked by opposite pawn
						// how far is blocker? 0 to 4 squares
							pathlen=BitCount(ntt)-1;
							a->sq[from].sqr_b+=p->pawn_blocked_penalty[0][s][pathlen];
							a->sq[from].sqr_e+=p->pawn_blocked_penalty[1][s][pathlen];
							fin_b+=p->pawn_blocked_penalty[0][s][pathlen];
							fin_e+=p->pawn_blocked_penalty[1][s][pathlen];
						}
					} else {
// stop square attacked without proper pawn protection from my side
// blocked by path forward attacked... 0 to 4 squares
						pathlen=BitCount(ntt)-1;
						a->sq[from].sqr_b+=p->pawn_stopped_penalty[0][s][pathlen];
						a->sq[from].sqr_e+=p->pawn_stopped_penalty[1][s][pathlen];
						fin_b+=p->pawn_stopped_penalty[0][s][pathlen];
						fin_e+=p->pawn_stopped_penalty[1][s][pathlen];
					}
// have pawn protection?
//					n = attack.isolated_p[from];
//					prot=n&(attack.rank[from]|attack.pawn_att[s^1][from]|attack.pawn_att[s][from]|);
					prot=attack.pawn_surr[from];
					prot&=sb;
					if(prot) {
						cc=BitCount(prot);
						a->sq[from].sqr_b+=p->pawn_protect[0]*cc;
						a->sq[from].sqr_e+=p->pawn_protect[1]*cc;
						fin_b+=p->pawn_protect[0]*cc;
						fin_e+=p->pawn_protect[1]*cc;
					} else {
// weak
// on half open file/in centre
BITVAR xxx;
						xxx= (s==0) ? attack.uphalf[from] : attack.downhalf[from];
						xxx&=attack.rank[from];
						xxx&=(b->maps[PAWN]&b->colormaps[s^1]);
						if(!xxx) {
								a->sq[from].sqr_b+=p->pawn_weak_onopen_penalty[0];
								a->sq[from].sqr_e+=p->pawn_weak_onopen_penalty[1];
								fin_b+=p->pawn_weak_onopen_penalty[0];
								fin_e+=p->pawn_weak_onopen_penalty[1];
						}
int fff;
						fff=getFile(from);
						if((fff>=2)&&(fff<=5)){
								a->sq[from].sqr_b+=p->pawn_weak_center_penalty[0];
								a->sq[from].sqr_e+=p->pawn_weak_center_penalty[1];
								fin_b+=p->pawn_weak_center_penalty[0];
								fin_e+=p->pawn_weak_center_penalty[1];
						}

// muzu byt chranen pesci zezadu?
//isolated? !!!!!
						bc=n&sb;
						if(!bc) {
							a->sq[from].sqr_b+=p->isolated_penalty[0];
							a->sq[from].sqr_e+=p->isolated_penalty[1];
							fin_b+=p->isolated_penalty[0];
							fin_e+=p->isolated_penalty[1];
//								fin[s]|=(from_b); //???
						} else {
//backward
							if(!(from_b&fin[s])) {
								a->sq[from].sqr_b+=p->backward_penalty[0];
								a->sq[from].sqr_e+=p->backward_penalty[1];
								fin_b+=p->backward_penalty[0];
								fin_e+=p->backward_penalty[1];
// can it be fixed? resp. muzu se dostat k nekomu kdo mne muze chranit?
								xx=(((x_f[s]& dd & (~FILEA))>>1) | ((x_f[s]& dd & (~FILEH))<<1));
								if(xx&sb) {
// it can,
									a->sq[from].sqr_b-=p->backward_penalty[0];
									a->sq[from].sqr_e-=p->backward_penalty[1];
									a->sq[from].sqr_b+=p->backward_fix_penalty[0];
									a->sq[from].sqr_e+=p->backward_fix_penalty[1];
									fin_b-=p->backward_penalty[0];
									fin_e-=p->backward_penalty[1];
									fin_b+=p->backward_fix_penalty[0];
									fin_e+=p->backward_fix_penalty[1];
								}
							}
						}
					}
				}

// fix material value
				if(from_b&(FILEA|FILEH)) {
					a->sq[from].sqr_b+=p->pawn_ah_penalty[0];
					a->sq[from].sqr_e+=p->pawn_ah_penalty[1];
					fin_b+=p->pawn_ah_penalty[0];
					fin_e+=p->pawn_ah_penalty[1];
				}
			}
			ClrLO(x);
		}
		count=BitCount(a->pa_mo[s])+BitCount(a->pa_at[s]);
		a->specs[s][PAWN].sqr_b=p->mob_val[0][s][PAWN][0]*count;
		a->specs[s][PAWN].sqr_e=p->mob_val[1][s][PAWN][1]*count;
		
//		LOGGER_2("%d; %d,%d\n", count, p->mob_val[0][s][PAWN][0]*count, p->mob_val[1][s][PAWN][1]*count );
//		printf("%d; %d,%d\n", count, p->mob_val[0][s][PAWN][0]*count, p->mob_val[1][s][PAWN][1]*count );
		
		pp=PAWN|BLACKPIECE;
	}
//	if(b->hps!=NULL) storePawnHash(b->hps, &hash, b->stats);
	return 0;
}

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

//		x = (b->maps[KING]) & b->colormaps[side];
//		from = LastOne(x);

		from=b->king[side];

//		s=side;
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
						ke->cr_att_ray|=cr2;
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
//							ke->cr_pins |=ob;
//							ke->cr_blocker_piece |=c3;
								ke->cr_pins |=c3;
//							ee = LastOne(c3);
								ke->cr_blocker_ray|=(cr2|normmark[ff]);
//							ke->cr_blocker_ray=(rays_int[from][ee]|normmark[ff]);
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
						ke->di_att_ray|=di2;
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
//		ke->kn_attackers=attack.maps[KNIGHT][from] & b->maps[KNIGHT] & b->colormaps[o];
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

int eval_w_sh_pawn2(board *b, attack_model *a, personality *p, BITVAR wdef, BITVAR watt, int file, int *eb, int *ee)
{
BITVAR f, bb, ww;
int wp=7, bp=7;
// 0 - 6, 0 - in front of king, 6 nothing at all in front of king
	f=attack.file[A1+file];
	bb=watt&f;
	ww=wdef&f;

//	printmask(f, "W_f");
//	printmask(bb, "W_BB");
//	printmask(ww, "W_WW");

	if(bb&b->maps[PAWN]&b->colormaps[BLACK]) bp=BitCount(bb);
	if(ww&b->maps[PAWN]&b->colormaps[WHITE]) wp=BitCount(ww);
	if(bp<wp) wp=7;
//	assert((bp<=7)&&(bp>=1));
//	assert((wp<=7)&&(wp>=1));
	*eb=p->king_s_pdef[0][WHITE][wp-1]+p->king_s_patt[0][WHITE][bp-1];
	*ee=p->king_s_pdef[1][WHITE][wp-1]+p->king_s_patt[1][WHITE][bp-1];
	return 0;
}

int eval_b_sh_pawn2(board *b, attack_model *a, personality *p, BITVAR bdef, BITVAR batt, int file, int *eb, int *ee)
{
BITVAR f, bb, ww;
int wp=7, bp=7;
	f=attack.file[A1+file];
	ww=batt&f;
	bb=bdef&f;
	if(ww&b->maps[PAWN]&b->colormaps[WHITE]) wp=BitCount(ww);
	if(bb&b->maps[PAWN]&b->colormaps[BLACK]) bp=BitCount(bb);
	if(wp<bp) bp=7;
//	assert((bp<=7)&&(bp>=1));
//	assert((wp<=7)&&(wp>=1));
	*eb=p->king_s_pdef[0][BLACK][bp-1]+p->king_s_patt[0][BLACK][wp-1];
	*ee=p->king_s_pdef[1][BLACK][bp-1]+p->king_s_patt[1][BLACK][wp-1];
	return 0;
}

int eval_king(board *b, attack_model *a, personality *p)
{
// zatim pouze pins a incheck
BITVAR x, q, mv;
int from, pp, s, m, to, ws, bs, r, r1_b, r1_e, r2_b, r2_e, rb, re, wr, br;
BITVAR  w_oppos, b_oppos, w_my, b_my;

	a->specs[0][KING].sqr_b=a->specs[1][KING].sqr_b=0;
	a->specs[0][KING].sqr_e=a->specs[1][KING].sqr_e=0;

	x = (b->maps[KING]);
	while (x) {
		from = LastOne(x);
		pp=b->pieces[from];
		s=(pp&BLACKPIECE)>>3;
//		eval_king_checks(b, &(a->ke[s]), p, s);
//		eval_king_checks_all(b, a);
		q=0;

// king mobility, spocitame vsechna pole kam muj kral muze (tj. krome vlastnich figurek a poli na ktere utoci nepratelsky kral
// a poli ktera jsou napadena cizi figurou
		mv = (attack.maps[KING][from]) & (~b->colormaps[s]) & (~attack.maps[KING][b->king[s^1]]);
		while (mv) {
			to = LastOne(mv);
			if(!AttackedTo_B(b, to, s)) {
				q|=normmark[to];
			}
			ClrLO(mv);
		}
		m=a->me[from].pos_att_tot=BitCount(q);
		a->me[from].pos_mob_tot_b=p->mob_val[0][s][KING][m];
		a->me[from].pos_mob_tot_e=p->mob_val[1][s][KING][m];
		a->sq[from].sqr_b=p->piecetosquare[0][s][KING][from];
		a->sq[from].sqr_e=p->piecetosquare[1][s][KING][from];
		ClrLO(x);
	}
// evaluate shelter
// left/right just consider pawns on three outer files
// when in center
// x_oppos - nejblizsi utocici pesec
// x_my nejblizsi branici pesec

	ws=getFile(b->king[WHITE]);
	bs=getFile(b->king[BLACK]);
	
	w_oppos=FillNorth(attack.rank[b->king[WHITE]],~(b->maps[PAWN]&b->colormaps[BLACK]), 0);
	w_oppos|= ((w_oppos|attack.rank[b->king[WHITE]])<<8);
	b_oppos=FillSouth(attack.rank[b->king[BLACK]], ~(b->maps[PAWN]&b->colormaps[WHITE]), 0);
	b_oppos|= ((b_oppos|attack.rank[b->king[BLACK]])>>8);
	w_my=FillNorth(attack.rank[b->king[WHITE]],~(b->maps[PAWN]&b->colormaps[WHITE]), 0);
	w_my|= ((w_my|attack.rank[b->king[WHITE]])<<8);
	b_my=FillSouth(attack.rank[b->king[BLACK]], ~(b->maps[PAWN]&b->colormaps[BLACK]), 0);
	b_my|= ((b_my|attack.rank[b->king[BLACK]])>>8);

//	printmask(b->maps[PAWN]&b->colormaps[WHITE], "WPAWN");
//	printmask(b->maps[PAWN]&b->colormaps[BLACK], "BPAWN");
//	printmask(w_oppos, "W_O");
//	printmask(w_my, "W_M");
//	printmask(b_oppos, "B_O");
//	printmask(b_my, "B_M");
//	LOGGER_0("pos %o, %o\n", b->king[WHITE], b->king[BLACK]);
	
	r1_b=r1_e=r2_b=r2_e=0;

// evaluate shelter only if king is on wings

	if((ws<=2) ||(ws>=5)) {
		if((ws-1)>=0){
			eval_w_sh_pawn2(b, a, p, w_my, w_oppos, ws-1, &rb, &re);
			r1_b+=rb;
			r1_e+=re;
		} else {
//				r1_b+=0;
//				r1_e+=0;
			r1_b+=p->king_s_pdef[0][WHITE][0]+p->king_s_patt[0][WHITE][6];
			r1_e+=p->king_s_pdef[1][WHITE][0]+p->king_s_patt[1][WHITE][6];
		}
		eval_w_sh_pawn2(b, a, p, w_my, w_oppos, ws, &rb, &re);
		r1_b+=rb;
		r1_e+=re;
		if((ws+1<8)) {
			eval_w_sh_pawn2(b, a, p, w_my, w_oppos, ws+1, &rb, &re);
			r1_b+=rb;
			r1_e+=re;
		} else {
			r1_b+=p->king_s_pdef[0][WHITE][0]+p->king_s_patt[0][WHITE][6];
			r1_e+=p->king_s_pdef[1][WHITE][0]+p->king_s_patt[1][WHITE][6];
		}
	}
	if((bs<=2)||(bs>=5)) {
		if((bs-1)>=0) {
			eval_b_sh_pawn2(b, a, p, b_my, b_oppos, bs-1, &rb, &re);
			r2_b+=rb;
			r2_e+=re;
		} else {
			r2_b+=p->king_s_pdef[0][BLACK][0]+p->king_s_patt[0][BLACK][6];
			r2_e+=p->king_s_pdef[1][BLACK][0]+p->king_s_patt[1][BLACK][6];
		}
		eval_b_sh_pawn2(b, a, p, b_my, b_oppos, bs, &rb, &re);
		r2_b+=rb;
		r2_e+=re;
		if((bs+1)<8) {
			eval_b_sh_pawn2(b, a, p, b_my, b_oppos, bs+1, &rb, &re);
			r2_b+=rb;
			r2_e+=re;
		} else {
			r2_b+=p->king_s_pdef[0][BLACK][0]+p->king_s_patt[0][BLACK][6];
			r2_e+=p->king_s_pdef[1][BLACK][0]+p->king_s_patt[1][BLACK][6];
		}
	}

#if 1
	a->specs[WHITE][KING].sqr_b=r1_b;
	a->specs[WHITE][KING].sqr_e=r1_e;
	a->specs[BLACK][KING].sqr_b=r2_b;
	a->specs[BLACK][KING].sqr_e=r2_e;
#endif

return 0;
}

int eval_king_shelter(board *b, attack_model *a, personality *p)
{
// zatim pouze pins a incheck
BITVAR x, q, mv;
int from, pp, s, m, to, ws, bs, r, r1_b, r1_e, r2_b, r2_e, rb, re, wr, br;
BITVAR  w_oppos, b_oppos, w_my, b_my;

// evaluate shelter
// left/right just consider pawns on three outer files
// when in center
// x_oppos - nejblizsi utocici pesec
// x_my nejblizsi branici pesec

	ws=getFile(b->king[WHITE]);
	bs=getFile(b->king[BLACK]);
	
	w_oppos=FillNorth(attack.rank[b->king[WHITE]],~(b->maps[PAWN]&b->colormaps[BLACK]), 0);
	w_oppos|= ((w_oppos|attack.rank[b->king[WHITE]])<<8);
	b_oppos=FillSouth(attack.rank[b->king[BLACK]], ~(b->maps[PAWN]&b->colormaps[WHITE]), 0);
	b_oppos|= ((b_oppos|attack.rank[b->king[BLACK]])>>8);
	w_my=FillNorth(attack.rank[b->king[WHITE]],~(b->maps[PAWN]&b->colormaps[WHITE]), 0);
	w_my|= ((w_my|attack.rank[b->king[WHITE]])<<8);
	b_my=FillSouth(attack.rank[b->king[BLACK]], ~(b->maps[PAWN]&b->colormaps[BLACK]), 0);
	b_my|= ((b_my|attack.rank[b->king[BLACK]])>>8);

	r1_b=r1_e=r2_b=r2_e=0;

// evaluate shelter only if king is on wings

	if((ws<=2) ||(ws>=5)) {
		if((ws-1)>=0){
			eval_w_sh_pawn2(b, a, p, w_my, w_oppos, ws-1, &rb, &re);
			r1_b+=rb;
			r1_e+=re;
		} else {
			r1_b+=p->king_s_pdef[0][WHITE][0]+p->king_s_patt[0][WHITE][6];
			r1_e+=p->king_s_pdef[1][WHITE][0]+p->king_s_patt[1][WHITE][6];
		}
		eval_w_sh_pawn2(b, a, p, w_my, w_oppos, ws, &rb, &re);
		r1_b+=rb;
		r1_e+=re;
		if((ws+1<8)) {
			eval_w_sh_pawn2(b, a, p, w_my, w_oppos, ws+1, &rb, &re);
			r1_b+=rb;
			r1_e+=re;
		} else {
			r1_b+=p->king_s_pdef[0][WHITE][0]+p->king_s_patt[0][WHITE][6];
			r1_e+=p->king_s_pdef[1][WHITE][0]+p->king_s_patt[1][WHITE][6];
		}
	}
	if((bs<=2)||(bs>=5)) {
		if((bs-1)>=0) {
			eval_b_sh_pawn2(b, a, p, b_my, b_oppos, bs-1, &rb, &re);
			r2_b+=rb;
			r2_e+=re;
		} else {
			r2_b+=p->king_s_pdef[0][BLACK][0]+p->king_s_patt[0][BLACK][6];
			r2_e+=p->king_s_pdef[1][BLACK][0]+p->king_s_patt[1][BLACK][6];
		}
		eval_b_sh_pawn2(b, a, p, b_my, b_oppos, bs, &rb, &re);
		r2_b+=rb;
		r2_e+=re;
		if((bs+1)<8) {
			eval_b_sh_pawn2(b, a, p, b_my, b_oppos, bs+1, &rb, &re);
			r2_b+=rb;
			r2_e+=re;
		} else {
			r2_b+=p->king_s_pdef[0][BLACK][0]+p->king_s_patt[0][BLACK][6];
			r2_e+=p->king_s_pdef[1][BLACK][0]+p->king_s_patt[1][BLACK][6];
		}
	}

#if 1
	a->specs[WHITE][KING].sqr_b=r1_b;
	a->specs[WHITE][KING].sqr_e=r1_e;
	a->specs[BLACK][KING].sqr_b=r2_b;
	a->specs[BLACK][KING].sqr_e=r2_e;
#endif

return 0;
}

int isDrawBy50x(board * b) {
	return 0;
}

int is_draw(board *b, attack_model *a, personality *p)
{
int ret,i, count;

	if((b->mindex_validity==1) && (p->mat_info[b->mindex]==INSUFF)) return 1;


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

int mat_info(int8_t *info)
{
int f;
	for(f=0;f<419999;f++) {
			info[f]=NO_INFO;
	}
// certain values known draw
//	m=MATidx(pw,pb,nw,nb,bwl,bwd,bbl,bbd,rw,rb,qw,qb);
// pw,pb,nw,nb,bwl,bwd,bbl,bbd,rw,rb,qw,qb, TYPE

int8_t CVL[][13]= {
// two knights
		{0,0,2,0,0,0,0,0,0,0,0,0,INSUFF},
		{0,0,0,2,0,0,0,0,0,0,0,0,INSUFF},
		{0,0,2,2,0,0,0,0,0,0,0,0,INSUFF},
		{0,0,0,0,2,0,0,0,0,0,0,0,UNLIKELY},
		{0,0,0,0,0,2,0,0,0,0,0,0,UNLIKELY},
		{0,0,0,0,0,0,2,0,0,0,0,0,UNLIKELY},
		{0,0,0,0,0,0,0,2,0,0,0,0,UNLIKELY}
    };

int values[]={1000, 3500, 3500, 5000, 9750, 0};
int i,m;
int pw, pb, nw, nb, bwl, bwd, bbl, bbd, rw, rb, qw, qb, p, mw, mb, mm, mmt, mwt, mbt;
int pwt, pbt, nwt, nbt, bwlt, bwdt, bblt, bbdt, rwt, rbt, qwt, qbt, pt, pt2;

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
//													if((pb==0)&&(pw==0)) {
														p=qb+qw+rb+rw+bbd+bbl+bwd+bwl+nb+nw;
														mw=pw*values[0]+nw*values[1]+(bwl+bwd)*values[2]+rw*values[3]+qw*values[4];
														mb=pb*values[0]+nb*values[1]+(bbl+bbd)*values[2]+rb*values[3]+qb*values[4];
// stronger side analysis
														mm=mw-mb;
														if(mm>0) {
															if(pw<2) {
																pwt=pw;
																pbt=pb;
																nwt=nw;
																nbt=nb;
																bwlt=bwl;
																bwdt=bwd;
																bblt=bbl;
																bbdt=bbd;
																rwt=rw;
																rbt=rb;
																qwt=qw;
																qbt=qb;
// ignore black pawns
																if(pw==1) {
// find least valuable enemy
																	if(nbt>0) nbt--;
																	else if(bblt>0) bblt--;
																	else if(bbdt>0) bbdt--;
																	else if(rbt>0) rbt--;
																	else if(qbt>0) qbt--;
																}
// material?
/*
  Mating potential of stronger side
  Yes leading more than 2 pieces
  No less then 350 cP ahead
  no if deficient pair (NN)
  
  scale /2 without P, (not against King + Pawns), 1/4 no potential, 1/16 no potential no pawns
  no scaling KKRKR, KKNKN
  
 */
																mwt=nwt*values[1]+(bwlt+bwdt)*values[2]+rwt*values[3]+qwt*values[4];
																mbt=nbt*values[1]+(bblt+bbdt)*values[2]+rbt*values[3]+qbt*values[4];
																pt=qw+rw+bwd+bwl+nw;
																pt2=qb+rb+bbd+bbl+nb;
																if(((mwt-mbt)>values[2])||(pt>=3)) {
																	if((pw==0)&&(pt2!=pb)) info[m]=DIV2;
																	else info[m]=NO_INFO;
																} else {
																	if(pw==0) info[m]=UNLIKELY;
																	else info[m]=DIV4;
																}
															}
														} else if(mm<0) {
															if(pb<2) {
																pwt=pw;
																pbt=pb;
																nwt=nw;
																nbt=nb;
																bwlt=bwl;
																bwdt=bwd;
																bblt=bbl;
																bbdt=bbd;
																rwt=rw;
																rbt=rb;
																qwt=qw;
																qbt=qb;
// ignore black pawns
																if(pb==1) {
// find least valuable enemy
																	if(nwt>0) nwt--;
																	else if(bwlt>0) bwlt--;
																	else if(bwdt>0) bwdt--;
																	else if(rwt>0) rwt--;
																	else if(qwt>0) qwt--;
																}
// material?
																mwt=nwt*values[1]+(bwlt+bwdt)*values[2]+rwt*values[3]+qwt*values[4];
																mbt=nbt*values[1]+(bblt+bbdt)*values[2]+rbt*values[3]+qbt*values[4];
																pt=qb+rb+bbd+bbl+nb;
																pt2=qw+rw+bwd+bwl+nw;
																if(((mbt-mwt)>values[2])||(pt>=3)) {
																	if((pb==0)&&(pt2!=pw)) info[m]=DIV2;
																	else info[m]=NO_INFO;
																} else {
																	if(pb==0) info[m]=UNLIKELY;
																	else info[m]=DIV4;
																}
															}
														}

//														if((p<7)&&(mm<values[2])&&(mm>-values[2])) {
//															info[m]=UNLIKELY;
//														}
//													}
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

	for(i=0;i<7;i++) {
		m=MATidx(CVL[i][0],CVL[i][1], CVL[i][2], CVL[i][3], CVL[i][4], CVL[i][5], CVL[i][6],
				CVL[i][7], CVL[i][8], CVL[i][9], CVL[i][10], CVL[i][11]);
		info[m]=CVL[i][12];
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
int eval(board* b, attack_model* a, personality* p) {
	int f, from;
	int score, score_b, score_e;
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

	make_model(b, a, p);
	make_pawn_model(b, a, p);
	eval_king(b, a, p);
	get_material_eval(b, p, &a->sc.material, &a->sc.material_e);
//	a->sc.material = p->mat[b->mindex].mat;
//	a->sc.material_e = p->mate_e[b->mindex].mat;
	// spocitat mobilitu + piece-square
	// spocitat mobilitu + piece-square
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

	for (f = a->pos_c[BISHOP]; f >= 0; f--) {
		from = a->pos_m[BISHOP][f];
		a->sc.side[0].mobi_b += a->me[from].pos_mob_tot_b;
		a->sc.side[0].mobi_e += a->me[from].pos_mob_tot_e;
		a->sc.side[0].sqr_b += a->sq[from].sqr_b;
		a->sc.side[0].sqr_e += a->sq[from].sqr_e;
	}
	for (f = a->pos_c[BISHOP | BLACKPIECE]; f >= 0; f--) {
		from = a->pos_m[BISHOP | BLACKPIECE][f];
		a->sc.side[1].mobi_b += a->me[from].pos_mob_tot_b;
		a->sc.side[1].mobi_e += a->me[from].pos_mob_tot_e;
		a->sc.side[1].sqr_b += a->sq[from].sqr_b;
		a->sc.side[1].sqr_e += a->sq[from].sqr_e;
	}
	for (f = a->pos_c[KNIGHT]; f >= 0; f--) {
		from = a->pos_m[KNIGHT][f];
		a->sc.side[0].mobi_b += a->me[from].pos_mob_tot_b;
		a->sc.side[0].mobi_e += a->me[from].pos_mob_tot_e;
		a->sc.side[0].sqr_b += a->sq[from].sqr_b;
		a->sc.side[0].sqr_e += a->sq[from].sqr_e;
	}
	for (f = a->pos_c[KNIGHT | BLACKPIECE]; f >= 0; f--) {
		from = a->pos_m[KNIGHT | BLACKPIECE][f];
		a->sc.side[1].mobi_b += a->me[from].pos_mob_tot_b;
		a->sc.side[1].mobi_e += a->me[from].pos_mob_tot_e;
		a->sc.side[1].sqr_b += a->sq[from].sqr_b;
		a->sc.side[1].sqr_e += a->sq[from].sqr_e;
	}
	for (f = a->pos_c[ROOK]; f >= 0; f--) {
		from = a->pos_m[ROOK][f];
		a->sc.side[0].mobi_b += a->me[from].pos_mob_tot_b;
		a->sc.side[0].mobi_e += a->me[from].pos_mob_tot_e;
		a->sc.side[0].sqr_b += a->sq[from].sqr_b;
		a->sc.side[0].sqr_e += a->sq[from].sqr_e;
	}
	a->sc.side[0].specs_b+=a->specs[0][ROOK].sqr_b;
	a->sc.side[0].specs_e+=a->specs[0][ROOK].sqr_e;

	for (f = a->pos_c[ROOK | BLACKPIECE]; f >= 0; f--) {
		from = a->pos_m[ROOK | BLACKPIECE][f];
		a->sc.side[1].mobi_b += a->me[from].pos_mob_tot_b;
		a->sc.side[1].mobi_e += a->me[from].pos_mob_tot_e;
		a->sc.side[1].sqr_b += a->sq[from].sqr_b;
		a->sc.side[1].sqr_e += a->sq[from].sqr_e;
	}
	a->sc.side[1].specs_b+=a->specs[1][ROOK].sqr_b;
	a->sc.side[1].specs_e+=a->specs[1][ROOK].sqr_e;

	for (f = a->pos_c[QUEEN]; f >= 0; f--) {
		from = a->pos_m[QUEEN][f];
		a->sc.side[0].mobi_b += a->me[from].pos_mob_tot_b;
		a->sc.side[0].mobi_e += a->me[from].pos_mob_tot_e;
		a->sc.side[0].sqr_b += a->sq[from].sqr_b;
		a->sc.side[0].sqr_e += a->sq[from].sqr_e;
	}
	for (f = a->pos_c[QUEEN | BLACKPIECE]; f >= 0; f--) {
		from = a->pos_m[QUEEN | BLACKPIECE][f];
		a->sc.side[1].mobi_b += a->me[from].pos_mob_tot_b;
		a->sc.side[1].mobi_e += a->me[from].pos_mob_tot_e;
		a->sc.side[1].sqr_b += a->sq[from].sqr_b;
		a->sc.side[1].sqr_e += a->sq[from].sqr_e;
	}
	for (f = a->pos_c[PAWN]; f >= 0; f--) {
		from = a->pos_m[PAWN][f];
		a->sc.side[0].sqr_b += a->sq[from].sqr_b;
		a->sc.side[0].sqr_e += a->sq[from].sqr_e;
	}
	a->sc.side[0].specs_b+=a->specs[0][PAWN].sqr_b;
	a->sc.side[0].specs_e+=a->specs[0][PAWN].sqr_e;

	for (f = a->pos_c[PAWN | BLACKPIECE]; f >= 0; f--) {
		from = a->pos_m[PAWN | BLACKPIECE][f];
		a->sc.side[1].sqr_b += a->sq[from].sqr_b;
		a->sc.side[1].sqr_e += a->sq[from].sqr_e;
	}
	a->sc.side[1].specs_b+=a->specs[1][PAWN].sqr_b;
	a->sc.side[1].specs_e+=a->specs[1][PAWN].sqr_e;

	from = b->king[WHITE];
		a->sc.side[0].mobi_b += a->me[from].pos_mob_tot_b;
		a->sc.side[0].mobi_e += a->me[from].pos_mob_tot_e;
		a->sc.side[0].sqr_b += a->sq[from].sqr_b;
		a->sc.side[0].sqr_e += a->sq[from].sqr_e;
		a->sc.side[0].specs_b +=a->specs[0][KING].sqr_b;
		a->sc.side[0].specs_e +=a->specs[0][KING].sqr_e;
	from = b->king[BLACK];
		a->sc.side[1].mobi_b += a->me[from].pos_mob_tot_b;
		a->sc.side[1].mobi_e += a->me[from].pos_mob_tot_e;
		a->sc.side[1].sqr_b += a->sq[from].sqr_b;
		a->sc.side[1].sqr_e += a->sq[from].sqr_e;
		a->sc.side[1].specs_b +=a->specs[1][KING].sqr_b;
		a->sc.side[1].specs_e +=a->specs[1][KING].sqr_e;


//all evaluations are in milipawns 
// phase is in range 0 - 255. 255 being total opening, 0 total ending

	if(p->simple_EVAL==1) {
// simplified eval
		score_b=a->sc.material+(a->sc.side[0].sqr_b - a->sc.side[1].sqr_b);
		score_e=a->sc.material_e+(a->sc.side[0].sqr_e - a->sc.side[1].sqr_e);
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

		if((b->mindex_validity==1)&&(((b->side==WHITE)&&(score>0))||((b->side==BLACK)&&(score<0)))) {
			switch(p->mat_info[b->mindex]) {
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
	}
	score += p->eval_BIAS;
#if 0
//	if((score>100000*256) || (score< -100000*256)) {
		printf("%d, %d, %d, %d, %d, %d, %d\n", a->sc.material,a->sc.side[0].mobi_b, a->sc.side[1].mobi_b, a->sc.side[0].sqr_b, a->sc.side[1].sqr_b, a->sc.side[0].specs_b, a->sc.side[1].specs_b );
		printf("%d, %d, %d, %d, %d, %d, %d\n", a->sc.material_e,a->sc.side[0].mobi_e,a->sc.side[1].mobi_e,a->sc.side[0].sqr_e, a->sc.side[1].sqr_e, a->sc.side[0].specs_e, a->sc.side[1].specs_e );
		score=score_b*a->phase+score_e*(255-a->phase);
		printf ("score %d, phase %d, score_b %d, score_e %d\n", score / 255, a->phase, score_b, score_e);
//	}
#endif
//		score=score_b*a->phase+score_e*(255-a->phase);
	a->sc.complete = score / 255;
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

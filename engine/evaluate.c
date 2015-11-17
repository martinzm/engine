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

int eval_phase(board *b){
int i;
// 256 -- pure beginning, 0 -- total ending
	i=3*BitCount(b->maps[PAWN]); 
	i+=10*BitCount(b->maps[KNIGHT]);
	i+=12*BitCount(b->maps[BISHOP]);
	i+=15*BitCount(b->maps[ROOK]);
	i+=30*BitCount(b->maps[QUEEN]);
	//
return i;
}

/*
 * pro mobilitu budeme pocitat pouze pole
 * - ktera nejsou napadena nepratelskymi pesci
 * - nejsou obsazena mymi figurami
 */

int make_model(board *b, attack_model *a, personality *p)
{
int f, from, pp, m, s;
BITVAR x, q;

// rook
	x = (b->maps[ROOK]);
	while (x) {
		from = LastOne(x);
		pp=b->pieces[from];
		s=(pp&BLACKPIECE)!=0;
		a->pos_c[pp]++;
		a->pos_m[pp][a->pos_c[pp]]=from;
		q=a->mvs[from] = (RookAttacks(b, from));
		m=a->me[from].pos_att_tot=BitCount(q & (~b->colormaps[s]) & (~a->pa_at[s^1]));
		a->me[from].pos_mob_tot_b=p->mob_val[0][s][ROOK][m];
		a->me[from].pos_mob_tot_e=p->mob_val[1][s][ROOK][m];
		a->sq[from].sqr_b=p->piecetosquare[0][s][ROOK][from];
		a->sq[from].sqr_e=p->piecetosquare[1][s][ROOK][from];
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
		m=a->me[from].pos_att_tot=BitCount(q & (~b->colormaps[s]) & (~a->pa_at[s^1]));
		a->me[from].pos_mob_tot_b=p->mob_val[0][s][BISHOP][m];
		a->me[from].pos_mob_tot_e=p->mob_val[1][s][BISHOP][m];
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
		m=a->me[from].pos_att_tot=BitCount(q & (~b->colormaps[s]) & (~a->pa_at[s^1]));
		a->me[from].pos_mob_tot_b=p->mob_val[0][s][KNIGHT][m];
		a->me[from].pos_mob_tot_e=p->mob_val[1][s][KNIGHT][m];
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
		m=a->me[from].pos_att_tot=BitCount(q & (~b->colormaps[s]) & (~a->pa_at[s^1]));
		a->me[from].pos_mob_tot_b=p->mob_val[0][s][QUEEN][m];
		a->me[from].pos_mob_tot_e=p->mob_val[1][s][QUEEN][m];
		a->sq[from].sqr_b=p->piecetosquare[0][s][QUEEN][from];
		a->sq[from].sqr_e=p->piecetosquare[1][s][QUEEN][from];
		ClrLO(x);
	}
return 0;
}

int make_pawn_model(board *b, attack_model *a, personality *p) {

int from, pp, s, fi, f;
BITVAR x, n, ob, sb, bc, dd, bsp, t;

//	wh = b->maps[PAWN]&b->colormaps[WHITE];
//	bl = b->maps[PAWN]&b->colormaps[BLACK];
	
	for(s=WHITE;s<=BLACK;s++) {
		x = b->maps[PAWN]&b->colormaps[s];
		while (x) {
			from = LastOne(x);
			pp=b->pieces[from];
			a->pos_c[pp]++;
			a->pos_m[pp][a->pos_c[pp]]=from;

			a->sq[from].sqr_b=p->piecetosquare[0][s][PAWN][from];
			a->sq[from].sqr_e=p->piecetosquare[1][s][PAWN][from];
			a->me[from].pos_mob_tot_b=0;
			a->me[from].pos_mob_tot_e=0;
			a->me[from].pos_att_tot=0;
			ClrLO(x);
		}
	}
// passed - zadny nepratelsky pesec nemuze p sebrat
// isolated - nema po stranach vlastni pesce
// weak - pesec ktery neni chranen a neni mozne ho chranit jinymi pesci a stop neni chranen pescem 
	
	for(s=WHITE;s<=BLACK;s++) {
		x = sb = b->maps[PAWN]&b->colormaps[s];
		ob = b->maps[PAWN]&b->colormaps[s^1];
		while (x) {
			from = LastOne(x);
			n = attack.passed_p[s][from];
			dd = attack.file[from];
// pawns in my way?
			if(n &dd &b->maps[PAWN]) {
// doubled? aplying penalty to second pawn only
				if(BitCount(n &dd &sb)==1) {
					a->sq[from].sqr_b+=p->doubled_penalty[0];
					a->sq[from].sqr_e+=p->doubled_penalty[1];
				}
			} else {
// could be passer
				if(!(n&ob)) {
					fi=(from>>3)&7;
					a->sq[from].sqr_b+=p->passer_bonus[0][s][fi];
					a->sq[from].sqr_e+=p->passer_bonus[1][s][fi];
				}
			}
			n = attack.isolated_p[from];
//isolated?
			if(!(bc=n&sb)) {
				a->sq[from].sqr_b+=p->isolated_penalty[0];
				a->sq[from].sqr_e+=p->isolated_penalty[1];
			} else {
//backward
				if(!(bc& attack.back_span_p[s][from])) {
					a->sq[from].sqr_b+=p->backward_penalty[0];
					a->sq[from].sqr_e+=p->backward_penalty[1];
// can it be fixed?
					bsp=a->pa_at[0]|a->pa_at[1]|(attack.passed_p[s][from]& dd &b->maps[PAWN]);
					t= (s == WHITE ? LastOne(bsp):FirstOne(bsp));
					if(!(t & (a->pa_at[s^1]|(attack.passed_p[s][from]&ob)))) {
// it can,
						a->sq[from].sqr_b-=p->backward_penalty[0];
						a->sq[from].sqr_e-=p->backward_penalty[1];
						a->sq[from].sqr_b+=p->backward_fix_penalty[0];
						a->sq[from].sqr_e+=p->backward_fix_penalty[1];
					}
				}
			}
			ClrLO(x);
		}
	}

// doubled - dva a vice za sebou, dalsi p pokuta
// candidate - je sance ze z nej bude passed
// backward  - je opozdeny
// stop je misto kam pesec stoupne po normalnim tahu (ne brani) - pawn push, 
// telestop jsou dalsi mista po ceste vpred (front span)

return 0;
}

//???
// teoreticky nepratelska vez muze blokovat utok strelce nebo damy po diagonale
// podobne strelec muze blokovat utok po sloupci nebo rade
int eval_king_quiet(board *b, king_eval *ke, personality *p, unsigned int side)
{
BITVAR cr2, di2, c2, d2, c, d, c3, d3;
int from, s, ff;
BITVAR x;

		x = (b->maps[KING]) & b->colormaps[side];
		from = LastOne(x);
		s=side;
//		o=s^1;

		c =ke->cr_all_ray;
		c2=c & (b->norm);
		d =ke->di_all_ray;
		d2=d & (b->norm);

#if 0
		if(c2|d2){
			printBoardNice(b);
			printmask(c,"C");
			printmask(c2,"C2");
			printmask(d,"D");
			printmask(d2,"D2");
		}
#endif

		ke->cr_blocker_piece = 0;
		ke->cr_blocker_ray = 0;
		if(c2){
			while(c2) {
				ff = LastOne(c2);
				cr2=rays[from][ff] & (nnormmark[from]);
				c3=cr2 & b->norm;
				if(c3==0) {
					ke->cr_blocker_piece |= normmark[ff];
					ke->cr_blocker_ray|=cr2;
				}
				ClrLO(c2);
			}
		}
		ke->di_blocker_piece = 0;
		ke->di_blocker_ray = 0;
		if(d2){
			while(d2) {
				ff = LastOne(d2);
				di2=rays[from][ff] & (nnormmark[from]);
				d3=di2 & b->norm;
				if(d3==0) {
					ke->di_blocker_piece |= normmark[ff];
					ke->di_blocker_ray|=di2;
				}
				ClrLO(d2);
			}
		}
// incorporate knights
		ke->kn_pot_att_pos=attack.maps[KNIGHT][from];
//incorporate pawns
		ke->pn_pot_att_pos=attack.pawn_att[s][from];

#if 0
		if(c3|d3){
			printmask(ke->cr_blocker_piece,"cr my piece");
			printmask(ke->di_blocker_piece,"di my piece");
			printmask(ke->kn_pot_att_pos,"kn pot attack");
			printmask(ke->pn_pot_att_pos,"pn pot attack");
			printmask(ke->cr_all_ray,"CR all rays");
			printmask(ke->cr_blocker_ray,"CR DEF rays");
			printmask(ke->di_all_ray,"DI all rays");
			printmask(ke->di_blocker_ray,"DI DEF rays");
		}
#endif

		

return 0;
}

int eval_king_checks(board *b, king_eval *ke, personality *p, unsigned int side)
{
BITVAR cr2, di2, c2, d2, c, d, c3, d3, ob;

int from, s, ff, o;
BITVAR x;

		x = (b->maps[KING]) & b->colormaps[side];
		from = LastOne(x);
		s=side;
		o=s^1;

// find potential attackers - get rays, and check existence of them
		cr2=di2=0;
// vert/horiz rays
		c =ke->cr_all_ray = attack.maps[ROOK][from];
// vert/horiz attackers
		c2=c & (b->maps[ROOK]|b->maps[QUEEN])&(b->colormaps[o]);
// diag rays
		d =ke->di_all_ray = attack.maps[BISHOP][from];
// diag attackers
		d2=d & (b->maps[BISHOP]|b->maps[QUEEN])&(b->colormaps[o]);

#if 0
		printBoardNice(b);
		printmask(c,"C");
		printmask(c2,"C2");
		printmask(d,"D");
		printmask(d2,"D2");
#endif

// if it can hit king, find nearest piece, blocker?
// pins might contain false as there can be other piece between pin and distant attacker
// rook/queen
		ke->cr_pins = 0;
		ke->cr_attackers = 0;
		ke->cr_att_ray = 0;
// iterate attackers
		if(c2){
			while(c2) {
				ff = LastOne(c2);
// get line between square and attacker
//				cr2=rays[from][ff] & (~normmark[ff]) & (~normmark[from]);
				cr2=rays_int[from][ff];
// check if there is piece in that line, that blocks the attack
				c3=cr2 & b->norm;

// determine status
				switch (BitCount(c3)) {
// just 1 means pin
				case 1:
					ke->cr_pins |=(c3 & b->colormaps[s]);
					break;
// 0 means attacked
				case 0:
					ke->cr_attackers |= normmark[ff];
					ke->cr_att_ray|=cr2;
					break;
				case 2:
// 2 means no attack no pin, with one exception
// pawn can be subject of e.p. In that case 2 pawns is just one blocker
					if((!((c3&b->maps[PAWN]&attack.rank[from])^c3)) && (b->ep!=-1)) {
						ob=c3 & nnormmark[b->ep];
						if(ob^c3) ke->cr_pins |=(c3 & b->colormaps[s]);
					}
					break;

				default:
// more than 2 means no attack no pin
					break;
				}
				ClrLO(c2);
			}
		}
// bishop/queen
		ke->di_pins = 0;
		ke->di_attackers = 0;
		ke->di_att_ray = 0;
		if(d2){
			while(d2) {
				ff = LastOne(d2);
//				di2=rays[from][ff] & (~normmark[ff]) & (~normmark[from]);
				di2=rays_int[from][ff];
				d3=di2 & b->norm;
				switch (BitCount(d3)) {
				case 1:
					ke->di_pins |=(d3 & b->colormaps[s]);
					break;
				case 0:
					ke->di_attackers |= normmark[ff];
					ke->di_att_ray|=di2;
					break;
				}
				ClrLO(d2);
			}
		}

// incorporate knights
		ke->kn_attackers=attack.maps[KNIGHT][from] & b->maps[KNIGHT] & b->colormaps[o];
//incorporate pawns
		ke->pn_attackers=attack.pawn_att[s][from] & b->maps[PAWN] & b->colormaps[o];
		ke->attackers=ke->cr_attackers | ke->di_attackers | ke->kn_attackers | ke->pn_attackers;

#if 0
		printmask(ke->cr_attackers,"cr attackers");
		printmask(ke->di_attackers,"di attackers");
		printmask(ke->kn_attackers,"kn attackers");
		printmask(ke->pn_attackers,"pn attackers");
		printmask(ke->attackers,"attackers");
		printmask(ke->cr_all_ray,"CR all rays");
		printmask(ke->cr_att_ray,"CR att rays");
		printmask(ke->di_all_ray,"DI all rays");
		printmask(ke->di_att_ray,"DI att rays");
		printmask(ke->cr_pins,"CR pins");
		printmask(ke->di_pins,"DI pins");
#endif

	return 0;
}

int eval_king_checks_all(board *b, attack_model *a)
{
	eval_king_checks(b, &(a->ke[WHITE]), NULL, WHITE);
	eval_king_checks(b, &(a->ke[BLACK]), NULL, BLACK);
return 0;
}

int eval_king(board *b, attack_model *a, personality *p)
{
// zatim pouze pins a incheck
BITVAR x, q, mv;
int from, pp, s, m, to;

	x = (b->maps[KING]);
	while (x) {
		from = LastOne(x);
		pp=b->pieces[from];
		s=(pp&BLACKPIECE)>>3;
		eval_king_checks(b, &(a->ke[s]), p, s);
		q=0;
		mv = (attack.maps[KING][from]) & (~b->colormaps[s]) & (~attack.maps[KING][b->king[s^1]]);
		while (mv) {
			to = LastOne(mv);
			if(!AttackedTo_A(b, to, s)) {
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
return 0;
}

int simple_pre_movegen(board *b, attack_model *a, unsigned int side)
{
int f, from, pp, m, s;
BITVAR x, q;

	for(f=(ER_PIECE+BLACKPIECE);f>=0;f--) {
		a->pos_c[f]=-1;
	}

// rook
	x = (b->maps[ROOK]&b->colormaps[side]);
	while (x) {
		from = LastOne(x);
		pp=b->pieces[from];
		a->pos_c[pp]++;
		a->pos_m[pp][a->pos_c[pp]]=from;
		q=a->mvs[from] = (RookAttacks(b, from));
		ClrLO(x);
	}
// bishop
	x = (b->maps[BISHOP]&b->colormaps[side]);
	while (x) {
		from = LastOne(x);
		pp=b->pieces[from];
		a->pos_c[pp]++;
		a->pos_m[pp][a->pos_c[pp]]=from;
		q=a->mvs[from] = (BishopAttacks(b, from));
		ClrLO(x);
	}
// knights
	x = (b->maps[KNIGHT]&b->colormaps[side]);
	while (x) {
		from = LastOne(x);
		pp=b->pieces[from];
		a->pos_c[pp]++;
		a->pos_m[pp][a->pos_c[pp]]=from;
		q=a->mvs[from]  = (attack.maps[KNIGHT][from]);
		ClrLO(x);
	}
// queen
	x = (b->maps[QUEEN]&b->colormaps[side]);
	while (x) {
		from = LastOne(x);
		pp=b->pieces[from];
		a->pos_c[pp]++;
		a->pos_m[pp][a->pos_c[pp]]=from;
		q=a->mvs[from] = (QueenAttacks(b, from));
		ClrLO(x);
	}
return 0;
}

int isDrawBy50x(board * b) {
	return 0;
}

int is_draw(board *b, attack_model *a, personality *p)
{
int ret,i, count;

	if(p->mat[b->mindex].info==INSUFF_MATERIAL) return 1;


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

/*
 * position[0] contain the last position with capture or pawn move...
 * and b->rule50move contains index where to write new position.
 * so rule50move must be 102 for 50 moves (both sides) to apply
 */

	if((b->move-b->rule50move)>=101) return 2;
	
	count=0;
	i=b->move;

/*
 * threefold repetition testing
 * a position is considered identical to another if
 * the same player is on move
 * the same types of pieces of the same colors occupy the same squares
 * the same moves are available to each player; in particular, each player has the same castling and en passant capturing rights.
 */

	i-=1;
	while(i>=0) {
		if(b->positions[i]==b->key) {
			if(b->posnorm[i]!=b->norm)	printf("Error: Not matching position to hash!\n");
			count++;
		}
		i-=2;
	}
	if(count>=3) ret=3;
	return ret;
}

int meval_table_gen(meval_t *t, personality *p, int stage){
int pw, pb, nw, nb, bwl, bwd, bbl, bbd, rw, rb, qw, qb, f;
int m, w, b;

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

// clear
	for(f=0;f<419999;f++) {
			t[f].mat=0;
			t[f].info=NO_INFO;
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
// tune pawn based
//													if(pw>5) 
													{
														w+=nw*(pw-5)*p->rook_to_pawn[stage]/2;
														w+=rw*(5-pw)*p->rook_to_pawn[stage];
													}
//													if(pb>5) 
													{
														b+=nw*(pb-5)*p->rook_to_pawn[stage]/2;
														b+=rw*(5-pb)*p->rook_to_pawn[stage];
													}
// tune bishop pair
													if((bwl+bwd)==2) w+=p->bishopboth[stage];
													if((bbl+bbd)==2) b+=p->bishopboth[stage];
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


// certain values known draw
//	m=MATidx(pw,pb,nw,nb,bwl,bwd,bbl,bbd,rw,rb,qw,qb);

int CVL[][12]= {
		{0,0,0,0,0,0,0,0,0,0,0,0}, // king

		{0,0,1,0,0,0,0,0,0,0,0,0}, // knights
		{0,0,0,1,0,0,0,0,0,0,0,0},
		{0,0,2,0,0,0,0,0,0,0,0,0},
		{0,0,0,2,0,0,0,0,0,0,0,0},
		{0,0,2,2,0,0,0,0,0,0,0,0},
		{0,0,0,0,1,0,0,0,0,0,0,0}, // bishops
		{0,0,0,0,0,1,0,0,0,0,0,0},
		{0,0,0,0,0,0,1,0,0,0,0,0}, // bishops
		{0,0,0,0,0,0,0,1,0,0,0,0},
		{0,0,0,0,1,0,1,0,0,0,0,0}, // bishops
		{0,0,0,0,0,1,0,1,0,0,0,0}, // bishops

};

// pw,pb,nw,nb,bwl,bwd,bbl,bbd,rw,rb,qw,qb
int CVL2[][12]= {
		{0,0,2,1,0,0,0,0,0,0,0,0},
		{0,0,1,2,0,0,0,0,0,0,0,0},
		{0,0,1,0,0,0,1,0,0,0,0,0}, // k + b
		{0,0,1,0,0,0,0,1,0,0,0,0},
		{0,0,0,1,1,0,0,0,0,0,0,0},
		{0,0,0,1,0,1,0,0,0,0,0,0},
		{0,0,2,0,0,0,1,0,0,0,0,0},
		{0,0,2,0,0,0,0,1,0,0,0,0},
		{0,0,0,2,1,0,0,0,0,0,0,0},
		{0,0,0,2,0,1,0,0,0,0,0,0},
		{0,0,0,0,0,0,1,0,1,0,0,0}, //R-B
		{0,0,0,0,0,0,0,1,1,0,0,0}, //R-B
		{0,0,0,0,1,0,0,0,0,1,0,0}, //R-B
		{0,0,0,0,0,1,0,0,0,1,0,0}, //R-B
		{0,0,0,1,0,0,0,0,1,0,0,0}, //R-N
		{0,0,1,0,0,0,0,0,0,1,0,0}, //R-N
		{0,0,0,0,1,0,0,0,1,1,0,0}, //RB-R
		{0,0,0,0,0,1,0,0,1,1,0,0}, //RB-R
		{0,0,0,0,0,0,1,0,1,1,0,0}, //RB-R
		{0,0,0,0,0,0,0,1,1,1,0,0}, //RB-R
		{0,0,1,0,0,0,0,0,1,1,0,0}, //RN-R
		{0,0,0,1,0,0,0,0,1,1,0,0}, //RN-R
		{0,0,0,0,1,0,0,0,0,0,1,1}, //QB-Q
		{0,0,0,0,0,1,0,0,0,0,1,1}, //QB-Q
		{0,0,0,0,0,0,1,0,0,0,1,1}, //QB-Q
		{0,0,0,0,0,0,0,1,0,0,1,1}, //QB-Q
		{0,0,1,0,0,0,0,0,0,0,1,1}, //QN-Q
		{0,0,0,1,0,0,0,0,0,0,1,1}, //QN-Q
		{0,0,1,0,1,0,1,0,0,0,0,0}, // bn-b
		{0,0,1,0,0,1,1,0,0,0,0,0}, // bn-b
		{0,0,1,0,1,0,0,1,0,0,0,0}, // bn-b
		{0,0,1,0,0,1,0,1,0,0,0,0}, // bn-b
		{0,0,0,1,1,0,1,0,0,0,0,0}, // bn-b
		{0,0,0,1,0,1,1,0,0,0,0,0}, // bn-b
		{0,0,0,1,1,0,0,1,0,0,0,0}, // bn-b
		{0,0,0,1,0,1,0,1,0,0,0,0}, // bn-b
		{0,0,1,1,1,0,0,0,0,0,0,0}, // bn-n
		{0,0,1,1,0,1,0,0,0,0,0,0}, // bn-n
		{0,0,1,1,0,0,1,0,0,0,0,0}, // bn-n
		{0,0,1,1,0,0,0,1,0,0,0,0}, // bn-n
		{0,0,1,1,1,0,0,0,0,0,0,0}, // bb-b
		{0,0,1,1,0,1,0,0,0,0,0,0}, // bb-b
		{0,0,1,0,1,1,0,0,0,0,0,0}, // bb-b
		{0,0,0,1,1,1,0,0,0,0,0,0}, // bb-b
		{0,0,0,2,0,0,0,0,1,0,0,0}, // 2m-R
		{0,0,0,1,0,0,1,0,1,0,0,0}, // 2m-R
		{0,0,0,1,0,0,0,1,1,0,0,0}, // 2m-R
		{0,0,0,0,0,0,1,1,1,0,0,0}, // 2m-Rw
		{0,0,2,0,0,0,0,0,0,1,0,0}, // 2m-R
		{0,0,1,0,1,0,0,0,0,1,0,0}, // 2m-R
		{0,0,1,0,0,1,0,0,0,1,0,0}, // 2m-R
		{0,0,0,0,1,1,0,0,0,1,0,0}, // 2m-Rb

};
int i;
	for(i=0;i<11;i++) {
		m=MATidx(CVL[i][0],CVL[i][1], CVL[i][2], CVL[i][3], CVL[i][4], CVL[i][5], CVL[i][6],
				CVL[i][7], CVL[i][8], CVL[i][9], CVL[i][10], CVL[i][11]);
		t[m].info=INSUFF_MATERIAL;
	}
	for(i=0;i<52;i++) {
		m=MATidx(CVL2[i][0],CVL2[i][1], CVL2[i][2], CVL2[i][3], CVL2[i][4], CVL2[i][5], CVL2[i][6],
				CVL2[i][7], CVL2[i][8], CVL2[i][9], CVL2[i][10], CVL2[i][11]);
		t[m].info=UNLIKELY;
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
 
int eval(board* b, attack_model* a, personality* p) {
	int f, from;
	int score;
//	a->phase = eval_phase(b);
// setup pawn attacks

	for(f=(ER_PIECE+BLACKPIECE);f>=0;f--) {
		a->pos_c[f]=-1;
	}
	a->pa_at[WHITE]=WhitePawnAttacks(b);
	a->pa_at[BLACK]=BlackPawnAttacks(b);
// bez ep!

	make_model(b, a, p);
	make_pawn_model(b, a, p);
	eval_king(b, a, p);
	a->sc.material = p->mat[b->mindex].mat;
	a->sc.material_e = p->mate_e[b->mindex].mat;
	// spocitat mobilitu + piece-square
	// spocitat mobilitu + piece-square
	a->sc.side[0].mobi_b = 0;
	a->sc.side[0].mobi_e = 0;
	a->sc.side[0].sqr_b = 0;
	a->sc.side[0].sqr_e = 0;
	a->sc.side[1].mobi_b = 0;
	a->sc.side[1].mobi_e = 0;
	a->sc.side[1].sqr_b = 0;
	a->sc.side[1].sqr_e = 0;
	for (f = a->pos_c[BISHOP]; f >= 0; f--) {
		from = a->pos_m[BISHOP][f];
		a->sc.side[0].mobi_b += a->me[from].pos_mob_tot_b;
		a->sc.side[0].mobi_e += a->me[from].pos_mob_tot_e;
		a->sc.side[0].sqr_b += a->sq[from].sqr_b;
		a->sc.side[0].sqr_e += a->sq[from].sqr_e;
	}
	for (f = a->pos_c[BISHOP + BLACKPIECE]; f >= 0; f--) {
		from = a->pos_m[BISHOP + BLACKPIECE][f];
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
	for (f = a->pos_c[KNIGHT + BLACKPIECE]; f >= 0; f--) {
		from = a->pos_m[KNIGHT + BLACKPIECE][f];
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
	for (f = a->pos_c[ROOK + BLACKPIECE]; f >= 0; f--) {
		from = a->pos_m[ROOK + BLACKPIECE][f];
		a->sc.side[1].mobi_b += a->me[from].pos_mob_tot_b;
		a->sc.side[1].mobi_e += a->me[from].pos_mob_tot_e;
		a->sc.side[1].sqr_b += a->sq[from].sqr_b;
		a->sc.side[1].sqr_e += a->sq[from].sqr_e;
	}
	for (f = a->pos_c[QUEEN]; f >= 0; f--) {
		from = a->pos_m[QUEEN][f];
		a->sc.side[0].mobi_b += a->me[from].pos_mob_tot_b;
		a->sc.side[0].mobi_e += a->me[from].pos_mob_tot_e;
		a->sc.side[0].sqr_b += a->sq[from].sqr_b;
		a->sc.side[0].sqr_e += a->sq[from].sqr_e;
	}
	for (f = a->pos_c[QUEEN + BLACKPIECE]; f >= 0; f--) {
		from = a->pos_m[QUEEN + BLACKPIECE][f];
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
	for (f = a->pos_c[PAWN + BLACKPIECE]; f >= 0; f--) {
		from = a->pos_m[PAWN + BLACKPIECE][f];
		a->sc.side[1].sqr_b += a->sq[from].sqr_b;
		a->sc.side[1].sqr_e += a->sq[from].sqr_e;
	}

	from = b->king[WHITE];
		a->sc.side[0].mobi_b += a->me[from].pos_mob_tot_b;
		a->sc.side[0].mobi_e += a->me[from].pos_mob_tot_e;
		a->sc.side[0].sqr_b += a->sq[from].sqr_b;
		a->sc.side[0].sqr_e += a->sq[from].sqr_e;

	from = b->king[BLACK];
		a->sc.side[1].mobi_b += a->me[from].pos_mob_tot_b;
		a->sc.side[1].mobi_e += a->me[from].pos_mob_tot_e;
		a->sc.side[1].sqr_b += a->sq[from].sqr_b;
		a->sc.side[1].sqr_e += a->sq[from].sqr_e;

		if(p->mat[b->mindex].info==UNLIKELY) {
			a->sc.material >>= 1;
			a->sc.material_e >>= 1;
		}

	score = a->phase * (a->sc.material) + (256 - a->phase) * (a->sc.material_e);
	score += a->phase * (a->sc.side[0].mobi_b - a->sc.side[1].mobi_b) 
			+ (256 - a->phase) * (a->sc.side[0].mobi_e - a->sc.side[1].mobi_e);
	score += a->phase * (a->sc.side[0].sqr_b - a->sc.side[1].sqr_b)
			+ (256 - a->phase) * (a->sc.side[0].sqr_e - a->sc.side[1].sqr_e);
			
	if(p->mat[b->mindex].info==INSUFF_MATERIAL) score=0;
	a->sc.complete = score / 256;

#if 0
	sprintf(b2,"Material: %d : %d ; phase: %d", a->sc.material, a->sc.material_e, a->phase);
	LOGGER_1("Eval:", b2, "\n");
	sprintf(b2,"Mobility: %d : %d ; %d : %d", a->sc.side[0].mobi_b, a->sc.side[1].mobi_b, a->sc.side[0].mobi_e, a->sc.side[1].mobi_e);
	LOGGER_1("Eval:", b2, "\n");
	sprintf(b2,"Squares: %d : %d ; %d : %d", a->sc.side[0].sqr_b, a->sc.side[1].sqr_b, a->sc.side[0].sqr_e, a->sc.side[1].sqr_e);
	LOGGER_1("Eval:", b2, "\n");
	sprintf(b2,"Totals: %d : %d", score, a->sc.complete);
	LOGGER_1("Eval:", b2, "\n");
#endif


	return a->sc.complete;
}

int SEE(board * b, attack_model *a, int *m) {
	return 0;
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
 
int setMatRec(cacheMat c, unsigned char m[][2*ER_PIECE], hashMat *s) {
	BITVAR gmat=getMatKey(m);
	BITVAR i = gmat&c.maskMat;
	c.cMat[i].info=s->info;
	c.cMat[i].mat[0]=s->mat[0];
	c.cMat[i].mat[1]=s->mat[1];
	c.cMat[i].key=gmat;

	c.cMat[i].material[WHITE][PAWN]=s->material[WHITE][PAWN];
	c.cMat[i].material[WHITE][KNIGHT]=s->material[WHITE][KNIGHT];
	c.cMat[i].material[WHITE][BISHOP]=s->material[WHITE][BISHOP];
	c.cMat[i].material[WHITE][BISHOP+ER_PIECE]=s->material[WHITE][BISHOP+ER_PIECE];
	c.cMat[i].material[WHITE][ROOK]=s->material[WHITE][ROOK];
	c.cMat[i].material[WHITE][QUEEN]=s->material[WHITE][QUEEN];
	c.cMat[i].material[BLACK][PAWN]=s->material[BLACK][PAWN];
	c.cMat[i].material[BLACK][KNIGHT]=s->material[BLACK][KNIGHT];
	c.cMat[i].material[BLACK][BISHOP]=s->material[BLACK][BISHOP];
	c.cMat[i].material[BLACK][BISHOP+ER_PIECE]=s->material[BLACK][BISHOP+ER_PIECE];
	c.cMat[i].material[BLACK][ROOK]=s->material[BLACK][ROOK];
	c.cMat[i].material[BLACK][QUEEN]=s->material[BLACK][QUEEN];

return 0;
}

int getMatRec(cacheMat c, unsigned char m[][2*ER_PIECE], hashMat *r){

	BITVAR gmat=getMatKey(m);
	BITVAR i = gmat&c.maskMat;

	if((c.cMat[i].key==-1)) return 2;
	r->info=c.cMat[i].info;
	r->mat[0]=c.cMat[i].mat[0];
	r->mat[1]=c.cMat[i].mat[1];
	r->material[WHITE][PAWN]=c.cMat[i].material[WHITE][PAWN];
	r->material[WHITE][KNIGHT]=c.cMat[i].material[WHITE][KNIGHT];
	r->material[WHITE][BISHOP]=c.cMat[i].material[WHITE][BISHOP];
	r->material[WHITE][BISHOP+ER_PIECE]=c.cMat[i].material[WHITE][BISHOP+ER_PIECE];
	r->material[WHITE][ROOK]=c.cMat[i].material[WHITE][ROOK];
	r->material[WHITE][QUEEN]=c.cMat[i].material[WHITE][QUEEN];
	r->material[BLACK][PAWN]=c.cMat[i].material[BLACK][PAWN];
	r->material[BLACK][KNIGHT]=c.cMat[i].material[BLACK][KNIGHT];
	r->material[BLACK][BISHOP]=c.cMat[i].material[BLACK][BISHOP];
	r->material[BLACK][BISHOP+ER_PIECE]=c.cMat[i].material[BLACK][BISHOP+ER_PIECE];
	r->material[BLACK][ROOK]=c.cMat[i].material[BLACK][ROOK];
	r->material[BLACK][QUEEN]=c.cMat[i].material[BLACK][QUEEN];
	if((c.cMat[i].key!=gmat)) {
		return 1;
	}
	
return 0;
}

int populateMatExceptions(personality *p, cacheMat c){
unsigned char pw, pb, nw, nb, bwl, bwd, bbl, bbd, rw, rb, qw, qb;
int m1, w1, b1, m2, w2, b2, u, ii, f;
unsigned char m[ER_SIDE][2*ER_PIECE];
BITVAR key;
hashMat r;
int run, ru2;

	run=ru2=0;

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
													u=0;
													w1=pw*p->Values[0][0]+nw*p->Values[0][1]+(bwl+bwd)*p->Values[0][2]+rw*p->Values[0][3]+qw*p->Values[0][4];
													b1=pb*p->Values[0][0]+nb*p->Values[0][1]+(bbl+bbd)*p->Values[0][2]+rb*p->Values[0][3]+qb*p->Values[0][4];
													w2=pw*p->Values[1][0]+nw*p->Values[1][1]+(bwl+bwd)*p->Values[1][2]+rw*p->Values[1][3]+qw*p->Values[1][4];
													b2=pb*p->Values[1][0]+nb*p->Values[1][1]+(bbl+bbd)*p->Values[1][2]+rb*p->Values[1][3]+qb*p->Values[1][4];
// tune pawn based
													if((pw>5)&&((nw>0)||(rw>0))) {
//														u=1;
														w1+=nw*(pw-5)*p->rook_to_pawn[0]/2;
														w1+=rw*(5-pw)*p->rook_to_pawn[0];
														w2+=nw*(pw-5)*p->rook_to_pawn[1]/2;
														w2+=rw*(5-pw)*p->rook_to_pawn[1];
													}
													if((pb>5)&&((nb>0)||(rb>0))) {
//														u=1;
														b1+=nw*(pb-5)*p->rook_to_pawn[0]/2;
														b1+=rw*(5-pb)*p->rook_to_pawn[0];
														b2+=nw*(pb-5)*p->rook_to_pawn[1]/2;
														b2+=rw*(5-pb)*p->rook_to_pawn[1];
													}
// tune bishop pair
													if((bwl+bwd)==2) {
//														u=1;
														w1+=p->bishopboth[0];
														w2+=p->bishopboth[1];
													}
													if((bbl+bbd)==2) {
//														u=1;
														b1+=p->bishopboth[0];
														b2+=p->bishopboth[1];
													}
													ru2++;
													if(u!=0) {
														run++;
														m[WHITE][PAWN]=pw;
														m[WHITE][KNIGHT]=nw;
														m[WHITE][BISHOP]=bwl+bwd;
														m[WHITE][BISHOP+ER_PIECE]=bwd;
														m[WHITE][ROOK]=rw;
														m[WHITE][QUEEN]=qw;

														m[BLACK][PAWN]=pb;
														m[BLACK][KNIGHT]=nb;
														m[BLACK][BISHOP]=bbl+bbd;
														m[BLACK][BISHOP+ER_PIECE]=bbd;
														m[BLACK][ROOK]=rb;
														m[BLACK][QUEEN]=qb;

														ii=getMatRec(c, m, &r);
														if(ii==1) {
															printf("Total %d, fixup %d\n", ru2, run);
															return 0;
														}
														r.mat[0]=w1-b1;
														r.mat[1]=w2-b2;
														r.info=NO_INFO;
														setMatRec(c, m, &r);
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
	}


// certain values known draw
//	m=MATidx(pw,pb,nw,nb,bwl,bwd,bbl,bbd,rw,rb,qw,qb);

int CVL[][12]= {
		{0,0,0,0,0,0,0,0,0,0,0,0}, // king

		{0,0,1,0,0,0,0,0,0,0,0,0}, // knights
		{0,0,0,1,0,0,0,0,0,0,0,0},
		{0,0,2,0,0,0,0,0,0,0,0,0},
		{0,0,0,2,0,0,0,0,0,0,0,0},
		{0,0,2,2,0,0,0,0,0,0,0,0},
		{0,0,0,0,1,0,0,0,0,0,0,0}, // bishops
		{0,0,0,0,0,1,0,0,0,0,0,0},
		{0,0,0,0,0,0,1,0,0,0,0,0}, // bishops
		{0,0,0,0,0,0,0,1,0,0,0,0},
		{0,0,0,0,1,0,1,0,0,0,0,0}, // bishops
		{0,0,0,0,0,1,0,1,0,0,0,0}, // bishops

};

// pw,pb,nw,nb,bwl,bwd,bbl,bbd,rw,rb,qw,qb
int CVL2[][12]= {
		{0,0,2,1,0,0,0,0,0,0,0,0},
		{0,0,1,2,0,0,0,0,0,0,0,0},
		{0,0,1,0,0,0,1,0,0,0,0,0}, // k + b
		{0,0,1,0,0,0,0,1,0,0,0,0},
		{0,0,0,1,1,0,0,0,0,0,0,0},
		{0,0,0,1,0,1,0,0,0,0,0,0},
		{0,0,2,0,0,0,1,0,0,0,0,0},
		{0,0,2,0,0,0,0,1,0,0,0,0},
		{0,0,0,2,1,0,0,0,0,0,0,0},
		{0,0,0,2,0,1,0,0,0,0,0,0},
		{0,0,0,0,0,0,1,0,1,0,0,0}, //R-B
		{0,0,0,0,0,0,0,1,1,0,0,0}, //R-B
		{0,0,0,0,1,0,0,0,0,1,0,0}, //R-B
		{0,0,0,0,0,1,0,0,0,1,0,0}, //R-B
		{0,0,0,1,0,0,0,0,1,0,0,0}, //R-N
		{0,0,1,0,0,0,0,0,0,1,0,0}, //R-N
		{0,0,0,0,1,0,0,0,1,1,0,0}, //RB-R
		{0,0,0,0,0,1,0,0,1,1,0,0}, //RB-R
		{0,0,0,0,0,0,1,0,1,1,0,0}, //RB-R
		{0,0,0,0,0,0,0,1,1,1,0,0}, //RB-R
		{0,0,1,0,0,0,0,0,1,1,0,0}, //RN-R
		{0,0,0,1,0,0,0,0,1,1,0,0}, //RN-R
		{0,0,0,0,1,0,0,0,0,0,1,1}, //QB-Q
		{0,0,0,0,0,1,0,0,0,0,1,1}, //QB-Q
		{0,0,0,0,0,0,1,0,0,0,1,1}, //QB-Q
		{0,0,0,0,0,0,0,1,0,0,1,1}, //QB-Q
		{0,0,1,0,0,0,0,0,0,0,1,1}, //QN-Q
		{0,0,0,1,0,0,0,0,0,0,1,1}, //QN-Q
		{0,0,1,0,1,0,1,0,0,0,0,0}, // bn-b
		{0,0,1,0,0,1,1,0,0,0,0,0}, // bn-b
		{0,0,1,0,1,0,0,1,0,0,0,0}, // bn-b
		{0,0,1,0,0,1,0,1,0,0,0,0}, // bn-b
		{0,0,0,1,1,0,1,0,0,0,0,0}, // bn-b
		{0,0,0,1,0,1,1,0,0,0,0,0}, // bn-b
		{0,0,0,1,1,0,0,1,0,0,0,0}, // bn-b
		{0,0,0,1,0,1,0,1,0,0,0,0}, // bn-b
		{0,0,1,1,1,0,0,0,0,0,0,0}, // bn-n
		{0,0,1,1,0,1,0,0,0,0,0,0}, // bn-n
		{0,0,1,1,0,0,1,0,0,0,0,0}, // bn-n
		{0,0,1,1,0,0,0,1,0,0,0,0}, // bn-n
		{0,0,1,1,1,0,0,0,0,0,0,0}, // bb-b
		{0,0,1,1,0,1,0,0,0,0,0,0}, // bb-b
		{0,0,1,0,1,1,0,0,0,0,0,0}, // bb-b
		{0,0,0,1,1,1,0,0,0,0,0,0}, // bb-b
		{0,0,0,2,0,0,0,0,1,0,0,0}, // 2m-R
		{0,0,0,1,0,0,1,0,1,0,0,0}, // 2m-R
		{0,0,0,1,0,0,0,1,1,0,0,0}, // 2m-R
		{0,0,0,0,0,0,1,1,1,0,0,0}, // 2m-Rw
		{0,0,2,0,0,0,0,0,0,1,0,0}, // 2m-R
		{0,0,1,0,1,0,0,0,0,1,0,0}, // 2m-R
		{0,0,1,0,0,1,0,0,0,1,0,0}, // 2m-R
		{0,0,0,0,1,1,0,0,0,1,0,0}, // 2m-Rb

};

int i;
	for(i=0;i<11;i++) {
//		m=MATidx(CVL[i][0],CVL[i][1], CVL[i][2], CVL[i][3], CVL[i][4], CVL[i][5], CVL[i][6],
//				CVL[i][7], CVL[i][8], CVL[i][9], CVL[i][10], CVL[i][11]);
//		t[m].info=INSUFF_MATERIAL;

		pw=CVL[i][0];
		pb=CVL[i][1];
		nw=CVL[i][2];
		nb=CVL[i][3];
		bwl=CVL[i][4];
		bwd=CVL[i][5];
		bbl=CVL[i][6];
		bbd=CVL[i][7];
		rw=CVL[i][8];
		rb=CVL[i][9];
		qw=CVL[i][10];
		qb=CVL[i][11];

		m[WHITE][PAWN]=pw;
		m[WHITE][KNIGHT]=nw;
		m[WHITE][BISHOP]=bwl+bwd;
		m[WHITE][BISHOP+ER_PIECE]=bwd;
		m[WHITE][ROOK]=rw;
		m[WHITE][QUEEN]=qw;

		m[BLACK][PAWN]=pb;
		m[BLACK][KNIGHT]=nb;
		m[BLACK][BISHOP]=bbl+bbd;
		m[BLACK][BISHOP+ER_PIECE]=bbd;
		m[BLACK][ROOK]=rb;
		m[BLACK][QUEEN]=qb;

		w1=pw*p->Values[0][0]+nw*p->Values[0][1]+(bwl+bwd)*p->Values[0][2]+rw*p->Values[0][3]+qw*p->Values[0][4];
		b1=pb*p->Values[0][0]+nb*p->Values[0][1]+(bbl+bbd)*p->Values[0][2]+rb*p->Values[0][3]+qb*p->Values[0][4];
		w2=pw*p->Values[1][0]+nw*p->Values[1][1]+(bwl+bwd)*p->Values[1][2]+rw*p->Values[1][3]+qw*p->Values[1][4];
		b2=pb*p->Values[1][0]+nb*p->Values[1][1]+(bbl+bbd)*p->Values[1][2]+rb*p->Values[1][3]+qb*p->Values[1][4];

		ii=getMatRec(c, m, &r);
		if(ii==1) {
			printf("WW Total %d, fixup %d\n", ru2, run);
		}
		if(ii!=0) {
			r.mat[0]=w1-b1;
			r.mat[1]=w2-b2;
		}
		r.info=INSUFF_MATERIAL;
		setMatRec(c, m, &r);
	}
	for(i=0;i<52;i++) {
//		m=MATidx(CVL2[i][0],CVL2[i][1], CVL2[i][2], CVL2[i][3], CVL2[i][4], CVL2[i][5], CVL2[i][6],
//				CVL2[i][7], CVL2[i][8], CVL2[i][9], CVL2[i][10], CVL2[i][11]);
//		t[m].info=UNLIKELY;
		pw=CVL2[i][0];
		pb=CVL2[i][1];
		nw=CVL2[i][2];
		nb=CVL2[i][3];
		bwl=CVL2[i][4];
		bwd=CVL2[i][5];
		bbl=CVL2[i][6];
		bbd=CVL2[i][7];
		rw=CVL2[i][8];
		rb=CVL2[i][9];
		qw=CVL2[i][10];
		qb=CVL2[i][11];

		m[WHITE][PAWN]=pw;
		m[WHITE][KNIGHT]=nw;
		m[WHITE][BISHOP]=bwl+bwd;
		m[WHITE][BISHOP+ER_PIECE]=bwd;
		m[WHITE][ROOK]=rw;
		m[WHITE][QUEEN]=qw;

		m[BLACK][PAWN]=pb;
		m[BLACK][KNIGHT]=nb;
		m[BLACK][BISHOP]=bbl+bbd;
		m[BLACK][BISHOP+ER_PIECE]=bbd;
		m[BLACK][ROOK]=rb;
		m[BLACK][QUEEN]=qb;

		w1=pw*p->Values[0][0]+nw*p->Values[0][1]+(bwl+bwd)*p->Values[0][2]+rw*p->Values[0][3]+qw*p->Values[0][4];
		b1=pb*p->Values[0][0]+nb*p->Values[0][1]+(bbl+bbd)*p->Values[0][2]+rb*p->Values[0][3]+qb*p->Values[0][4];
		w2=pw*p->Values[1][0]+nw*p->Values[1][1]+(bwl+bwd)*p->Values[1][2]+rw*p->Values[1][3]+qw*p->Values[1][4];
		b2=pb*p->Values[1][0]+nb*p->Values[1][1]+(bbl+bbd)*p->Values[1][2]+rb*p->Values[1][3]+qb*p->Values[1][4];

		ii=getMatRec(c, m, &r);
		if(ii==1) {
			printf("W2 Total %d, fixup %d\n", ru2, run);
		}
		if(ii!=0) {
			r.mat[0]=w1-b1;
			r.mat[1]=w2-b2;
		}
		r.info=UNLIKELY;
		setMatRec(c, m, &r);
	}
	return 1;
}

cacheMat buildMatCache(personality *p) {
cacheMat ret;
int f;
	ret.cMat=NULL;
	ret.maskMat=0x7F;
	do {
		if(ret.cMat!=NULL) free(ret.cMat);
		ret.maskMat<<=1;
		ret.maskMat++;
		ret.cMat=malloc((ret.maskMat+1)*sizeof(hashMat));
		for(f=ret.maskMat;f>=0;f--) {
			ret.cMat[f].key=-1;
		}
	} while(populateMatExceptions(p, ret)==0);
//	populateMatNormal(p, ret.cMat, ret.maskMat));
	return ret;
}

int deleteMatCache(cacheMat c){
	free(c.cMat);
	return 0;
}

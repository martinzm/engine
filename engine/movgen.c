#include "movgen.h"
#include "attacks.h"
#include "evaluate.h"
#include "generate.h"
#include "hash.h"
#include "defines.h"
#include "bitmap.h"
#include "tests.h"
#include "utils.h"
#include "globals.h"
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#define MOVE_TEST_SETUP BITVAR mv2=mv
#define MOVE_TEST(x) if(*(move-1)==x) { printf("Move from:%d to:%d triggered file:%s, line:%d\n", from, to, __FILE__, __LINE__ );printmask(mv2,"rook"); printboard(b);  dumpit(b, from); }

BITVAR pincount[ER_PIECE];
BITVAR pindrop[ER_PIECE];

int pininit() {
int i;
	for(i=0;i<ER_PIECE;i++) pindrop[i]=pincount[i]=0;
	return 0;
}

int pindump(){
int i;
	for(i=0;i<ER_PIECE;i++) {
		printf("PIN dump: piece:%d, count: %lu, dropped: %lu\n",i, pincount[i], pindrop[i]);
	}
	return 0;
}

void dumpit(board * b, int from)
{
BITVAR n,r,o,aw;
    n=getnormvector(b->norm, from);
    r=get90Rvector(b->r90R, from);
    o=r|n;
    aw=o&b->colormaps[WHITE];
//    ab=o&b->colormaps[BLACK];
    printmask(o,"ored");
    printmask(aw,"And White");
    printmask(aw,"And BLACK");

}

BITVAR isInCheck_Eval(board *b, attack_model *a, int side)
{
	return a->ke[side].attackers;
}

BITVAR generatePins_eval(board *b, attack_model *a, int side)
{
	return ((a->ke[side].cr_pins | a->ke[side].di_pins)&(b->colormaps[side]));
}

/*
 * generates pseudomove, not checking full validity, not doing promotion and castling properly.
 * just to make sure that king of side, which piece is moving, will not end up in check...
 */

BITVAR isInCheck_after_move(board *b, attack_model *a, int from, int to, int del)
{
int oldp, movp, delp, so, sm, sd, mon;
BITVAR ret;

//	printBoardNice(b);

	oldp=b->pieces[to];
	movp=b->pieces[from];

	so=(oldp&BLACKPIECE)>>3;
	sm=(movp&BLACKPIECE)>>3;

	oldp&=PIECEMASK;
	movp&=PIECEMASK;

	mon=b->king[sm];
// king moves?
	if(mon==from) mon=to;

	if(oldp!=ER_PIECE) ClearAll(to, so, oldp, b);
	ClearAll(from, sm, movp, b);
	SetAll(to, sm, movp, b);

	if(del!=-1) {
		delp=b->pieces[del];
		sd=(delp&BLACKPIECE)>>3;
		delp&=PIECEMASK;
		ClearAll(del, sd, delp, b);
	}

	ret=AttackedTo_A(b,mon, sm);

	if(del!=-1) {
		SetAll(del, sd, delp, b);
	}
	ClearAll(to, sm, movp, b);
	SetAll(from, sm, movp, b);
	if(oldp!=ER_PIECE) SetAll(to, so, oldp, b);
	return ret;
}

// pv > hash > winning/prom > Qprom > neutral > killer > castling > minor prom > non-cap (history heur) > losing
 
 //declare quiet move purely based on previous qorder assignment
int is_quiet_move(board *b, attack_model *a, move_entry *m){
 // predelat
long int x;
char buff[256];
	x=m->qorder;
	if(x>=HASH_OR) x-=HASH_OR;
//	if((x>=(A_OR_N)) || ((x>=(A_OR2)) && (x<=(A_OR2_MAX)))||((MV_BAD<=x)&&(x<=(MV_BAD_MAX)))) return 0;
	if(((x>=(MV_OR)) && (x<=(MV_HH_MAX))) || ((x>=(KILLER_OR))&&(x<=KILLER_OR_MAX)) || ((x==CS_Q_OR)||(x==CS_K_OR))) return 1;
 return 0;
}

// eval musi byt proveden! 
 
void generateCaptures(board * b, attack_model *a, move_entry ** m, int gen_u)
{
int from, to;
BITVAR x, mv, rank, piece, npins, block_ray;
move_entry * move;
int ep_add, pie;
unsigned char side, opside;
//king_eval ke;
personality *p;

		p=b->pers;
	
		move = *m;
		if(b->side == WHITE) {
			rank=RANK7;
			side=WHITE;
			opside=BLACK;
			ep_add=8;
		}
		else {
			rank=RANK2;
			opside=WHITE;
			side=BLACK;
			ep_add=-8;
		}
		npins=(~(a->ke[side].cr_pins | a->ke[side].di_pins));

		
// generate queens
		piece=b->maps[QUEEN]&(b->colormaps[side])&npins;
		while(piece) {
			from = LastOne(piece);
			mv=a->mvs[from] & (b->colormaps[opside]);
			while (mv) {
				to = LastOne(mv);
				move->move = PackMove(from, to, ER_PIECE, 0);
				move->qorder=move->real_score=b->pers->LVAcap[QUEEN][b->pieces[to]&PIECEMASK];
				move++;
				ClrLO(mv);
			}
			ClrLO(piece);
		}
		piece=b->maps[QUEEN]&(b->colormaps[side])&(~npins);
		while(piece) {
			from = LastOne(piece);
			mv=a->mvs[from] & (b->colormaps[opside]) & attack.rays_dir[b->king[side]][from];
			while (mv) {
				to = LastOne(mv);
				move->move = PackMove(from, to,  ER_PIECE, 0);
				move->qorder=move->real_score=b->pers->LVAcap[QUEEN][b->pieces[to]&PIECEMASK];
				move++;
				ClrLO(mv);
			}
			ClrLO(piece);
		}

// generate rooks 
// generate non pins only
		piece=b->maps[ROOK]&(b->colormaps[side])&npins;
		while(piece) {
			from = LastOne(piece);
			mv=a->mvs[from] & (b->colormaps[opside]);
			while (mv) {
				to = LastOne(mv);
				move->move = PackMove(from, to,  ER_PIECE, 0);
				move->qorder=move->real_score=b->pers->LVAcap[ROOK][b->pieces[to]&PIECEMASK];
				move++;
				ClrLO(mv);
			}
			ClrLO(piece);
		}
// generate pins only, limit moves to blocker line
		piece=b->maps[ROOK]&(b->colormaps[side])&(~npins);
		while(piece) {
			from = LastOne(piece);
			mv=a->mvs[from] & (b->colormaps[opside]) & attack.rays_dir[b->king[side]][from];
			while (mv) {
				to = LastOne(mv);
				move->move = PackMove(from, to,  ER_PIECE, 0);
				move->qorder=move->real_score=b->pers->LVAcap[ROOK][b->pieces[to]&PIECEMASK];
				move++;
				ClrLO(mv);
			}
			ClrLO(piece);
		}

// bishops
		piece=b->maps[BISHOP]&(b->colormaps[side])&npins;
		while(piece) {
			from = LastOne(piece);
			mv=a->mvs[from] & (b->colormaps[opside]);
			while (mv) {
				to = LastOne(mv);
				move->move = PackMove(from, to,  ER_PIECE, 0);
				move->qorder=move->real_score=b->pers->LVAcap[BISHOP][b->pieces[to]&PIECEMASK];
				move++;
				ClrLO(mv);
			}
			ClrLO(piece);
		}

		piece=b->maps[BISHOP]&(b->colormaps[side])&(~npins);
		while(piece) {
			from = LastOne(piece);
			mv=a->mvs[from] & (b->colormaps[opside]) & attack.rays_dir[b->king[side]][from];
			while (mv) {
				to = LastOne(mv);
				move->move = PackMove(from, to,  ER_PIECE, 0);
				move->qorder=move->real_score=b->pers->LVAcap[BISHOP][b->pieces[to]&PIECEMASK];
				move++;
				ClrLO(mv);
			}
			ClrLO(piece);
		}

// knights
		piece=b->maps[KNIGHT]&(b->colormaps[side])&npins;
		while(piece) {
			from = LastOne(piece);
			mv=a->mvs[from] & (b->colormaps[opside]);
			while (mv) {
				to = LastOne(mv);
				move->move = PackMove(from, to,  ER_PIECE, 0);
				move->qorder=move->real_score=b->pers->LVAcap[KNIGHT][b->pieces[to]&PIECEMASK];
				move++;
				ClrLO(mv);
			}
			ClrLO(piece);
		}
// no need to generate pinned knight move, cannot move in way of attack
// pawn promotions
		x = (b->maps[PAWN]) & (b->colormaps[side]) & rank & npins;
		while (x) {
			from = LastOne(x);
			mv = (attack.pawn_move[side][from]) & (~ b->norm);
			while (mv) {
				to = LastOne(mv);
				move->move = PackMove(from, to,  QUEEN, 0);
				move->qorder=move->real_score=A_QUEEN_PROM;
				move++;
				move->move = PackMove(from, to,  KNIGHT, 0);
				move->qorder=move->real_score=A_KNIGHT_PROM;
				move++;
// underpromotions
				if(gen_u!=0) {
					move->move = PackMove(from, to,  BISHOP, 0);
					move->qorder=move->real_score=A_MINOR_PROM+B_OR;
					move++;
					move->move = PackMove(from, to,  ROOK, 0);
					move->qorder=move->real_score=A_MINOR_PROM+R_OR;
					move++;
				}
				ClrLO(mv);
			}
			ClrLO(x);
	}

// pinned version
		x = (b->maps[PAWN]) & (b->colormaps[side]) & rank & (~npins);
		while (x) {
			from = LastOne(x);
			mv = (attack.pawn_move[side][from]) & (~ b->norm) & attack.rays_dir[b->king[side]][from];
			while (mv) {
				to = LastOne(mv);
				move->move = PackMove(from, to,  QUEEN, 0);
				move->qorder=move->real_score=A_QUEEN_PROM;
				move++;
				move->move = PackMove(from, to,  KNIGHT, 0);
				move->qorder=move->real_score=A_KNIGHT_PROM;
				move++;
// underpromotions
				if(gen_u!=0) {
					move->move = PackMove(from, to,  BISHOP, 0);
					move->qorder=move->real_score=A_MINOR_PROM+B_OR;
					move++;
					move->move = PackMove(from, to,  ROOK, 0);
					move->qorder=move->real_score=A_MINOR_PROM+R_OR;
					move++;
				}
				ClrLO(mv);
			}
			ClrLO(x);
	}

// pawn promotions after capture/attack
		x = (b->maps[PAWN]) & (b->colormaps[side]) & rank & npins;
		while (x) {
			from = LastOne(x);
			mv = (attack.pawn_att[side][from]) & (b->colormaps[opside]);
			while (mv) {
				to = LastOne(mv);
				move->move = PackMove(from, to,  QUEEN, 0);
				move->qorder=move->real_score=b->pers->LVAcap[KING+1][b->pieces[to]&PIECEMASK];
				move++;
				move->move = PackMove(from, to,  KNIGHT, 0);
				move->qorder=move->real_score=b->pers->LVAcap[KING+2][b->pieces[to]&PIECEMASK];
				move++;
//underpromotion
				if(gen_u!=0) {
					move->move = PackMove(from, to,  BISHOP, 0);
					move->qorder=move->real_score=A_OR2;
					move++;
					move->move = PackMove(from, to,  ROOK, 0);
					move->qorder=move->real_score=A_OR2;
					move++;
				}
				ClrLO(mv);
			}
			ClrLO(x);
		}
// pinned
		x = (b->maps[PAWN]) & (b->colormaps[side]) & rank & (~npins);
		while (x) {
			from = LastOne(x);
			mv = (attack.pawn_att[side][from]) & (b->colormaps[opside])& attack.rays_dir[b->king[side]][from];
			while (mv) {
				to = LastOne(mv);
				move->move = PackMove(from, to,  QUEEN, 0);
				move->qorder=move->real_score=b->pers->LVAcap[KING+1][b->pieces[to]&PIECEMASK];
				move++;
				move->move = PackMove(from, to,  KNIGHT, 0);
				move->qorder=move->real_score=b->pers->LVAcap[KING+2][b->pieces[to]&PIECEMASK];
				move++;
//underpromotion
				if(gen_u!=0) {
					move->move = PackMove(from, to,  BISHOP, 0);
					move->qorder=move->real_score=A_OR2;
					move++;
					move->move = PackMove(from, to,  ROOK, 0);
					move->qorder=move->real_score=A_OR2;
					move++;
				}
				ClrLO(mv);
			}
			ClrLO(x);
		}

// pawn attacks
		x = (b->maps[PAWN]) & (b->colormaps[side]) & (~rank) & npins;
		while (x) {
			from = LastOne(x);
			mv = (attack.pawn_att[side][from]) & (b->colormaps[opside]);
			while (mv) {
				to = LastOne(mv);
				move->move = PackMove(from, to,  ER_PIECE, 0);
				move->qorder=move->real_score=b->pers->LVAcap[PAWN][b->pieces[to]&PIECEMASK];
				move++;
				ClrLO(mv);
			}
			ClrLO(x);
		}
		x = (b->maps[PAWN]) & (b->colormaps[side]) & (~rank) & (~npins);
		while (x) {
			from = LastOne(x);
			mv = (attack.pawn_att[side][from]) & (b->colormaps[opside])& attack.rays_dir[b->king[side]][from];
			while (mv) {
				to = LastOne(mv);
				move->move = PackMove(from, to,  ER_PIECE, 0);
				move->qorder=move->real_score=b->pers->LVAcap[PAWN][b->pieces[to]&PIECEMASK];
				move++;
				ClrLO(mv);
			}
			ClrLO(x);
		}

// king 
		x = (b->maps[KING]) & (b->colormaps[side]);
		while (x) {
			from = LastOne(x);
			mv = (attack.maps[KING][from]) & (b->colormaps[opside]) & (~attack.maps[KING][b->king[opside]]);
/*
 * zavislost na opside analyze
 */			
			mv = mv & (~a->att_by_side[opside]);
			while (mv) {
				to = LastOne(mv);
					move->move = PackMove(from, to,  ER_PIECE, 0);
					move->qorder=move->real_score=b->pers->LVAcap[KING][b->pieces[to]&PIECEMASK];
					move++;
				ClrLO(mv);
			}
			ClrLO(x);
		}
// ep attacks 
		if(b->ep!=-1) {
			x = (attack.ep_mask[b->ep]) & (b->maps[PAWN]) & (b->colormaps[side]);
			while (x) {
				from = LastOne(x);
				to=b->ep+ep_add;
				ClearAll(b->ep, opside, PAWN, b);
				ClearAll(from, side, PAWN, b);
				SetAll(to, side, PAWN, b);
// pin?
				if(!AttackedTo_B(b, b->king[side], side)) {
					move->move = PackMove(from, to, PAWN, 0);
// b->ep has real position of DoublePush
// to is where capturing pawn will end
					LOGGER_4("GenerateCapture EP\n");
					move->qorder=move->real_score=b->pers->LVAcap[PAWN][PAWN];
					move++;
				}
				ClearAll(to, side, PAWN, b);
				SetAll(from, side, PAWN, b);
				SetAll(b->ep, opside, PAWN, b);
				ClrLO(x);
			}
		}
		*m=move;
}

void generateMoves(board * b, attack_model *a, move_entry ** m)
{
int from, to;
BITVAR x, mv, rank, brank, pmv, y, piece, npins, block_ray;
move_entry * move;
int orank, back, ff, pie;
unsigned char side, opside;
personality *p;

		p=b->pers;

		move = *m;
		if(b->side == WHITE) {
			rank=RANK7;
			side=WHITE;
			opside=BLACK;
			brank=RANK2;
			orank=0;
			back=0;
			ff=8;
		}
		else {
			rank=RANK2;
			opside=WHITE;
			side=BLACK;
			brank=RANK7;
			orank=56;
			back=16;
			ff=-8;
		}
		npins=(~(a->ke[side].cr_pins | a->ke[side].di_pins));

// knights
		piece=b->maps[KNIGHT]&(b->colormaps[side])&npins;
		while(piece) {
			from = LastOne(piece);
			mv=a->mvs[from]& (~b->norm);
			while (mv) {
				to = LastOne(mv);
				move->move = PackMove(from, to,  ER_PIECE, 0);
				move->qorder=move->real_score=MV_OR+N_OR;
				move++;
				ClrLO(mv);
			}
			ClrLO(piece);
		}
// no pinned knight moves!???

// bishops	
		piece=b->maps[BISHOP]&(b->colormaps[side])&(npins);
		while(piece) {
			from = LastOne(piece);
			mv=a->mvs[from]& (~b->norm);
			while (mv) {
				to = LastOne(mv);
				move->move = PackMove(from, to,  ER_PIECE, 0);
				move->qorder=move->real_score=MV_OR+B_OR;
				move++;
				ClrLO(mv);
			}
			ClrLO(piece);
		}
		piece=b->maps[BISHOP]&(b->colormaps[side])&(~npins);
		while(piece) {
			from = LastOne(piece);
			mv=a->mvs[from]& (~b->norm) & attack.rays_dir[b->king[side]][from];
			while (mv) {
				to = LastOne(mv);
				move->move = PackMove(from, to,  ER_PIECE, 0);
				move->qorder=move->real_score=MV_OR+B_OR;
				move++;
				ClrLO(mv);
			}
			ClrLO(piece);
		}
// generate rooks, non pins
		piece=b->maps[ROOK]&(b->colormaps[side])&npins;
		while(piece) {
			from = LastOne(piece);
			mv=a->mvs[from]& (~b->norm) ;
			while (mv) {
				to = LastOne(mv);
				move->move = PackMove(from, to,  ER_PIECE, 0);
				move->qorder=move->real_score=MV_OR+R_OR;
				move++;
				ClrLO(mv);
			}
			ClrLO(piece);
		}
//pins
		piece=b->maps[ROOK]&(b->colormaps[side])&(~npins);
		while(piece) {
			from = LastOne(piece);
			mv=a->mvs[from]& (~b->norm) & attack.rays_dir[b->king[side]][from];
			while (mv) {
				to = LastOne(mv);
				move->move = PackMove(from, to,  ER_PIECE, 0);
				move->qorder=move->real_score=MV_OR+R_OR;
				move++;
				ClrLO(mv);
			}
			ClrLO(piece);
		}
		
// generate queens, non pins
		piece=b->maps[QUEEN]&(b->colormaps[side])&npins;
		while(piece) {
			from = LastOne(piece);
			mv=a->mvs[from]& (~b->norm);
			while (mv) {
				to = LastOne(mv);
				move->move = PackMove(from, to,  ER_PIECE, 0);
				move->qorder=move->real_score=MV_OR+Q_OR;
				move++;
				ClrLO(mv);
			}
			ClrLO(piece);
		}
//pins
		piece=b->maps[QUEEN]&(b->colormaps[side])&(~npins);
		while(piece) {
			from = LastOne(piece);
			mv=a->mvs[from]& (~b->norm) & attack.rays_dir[b->king[side]][from];
			while (mv) {
				to = LastOne(mv);
				move->move = PackMove(from, to,  ER_PIECE, 0);
				move->qorder=move->real_score=MV_OR+Q_OR;
				move++;
				ClrLO(mv);
			}
			ClrLO(piece);
		}
		
// pawn moves
		x = (b->maps[PAWN]) & (b->colormaps[side]) & (~rank) & npins;
		y=((x<<8)>>back)& (~b->norm);
		while (y) {
			to = LastOne(y);
			from=to-ff;
			move->move = PackMove(from, to,  ER_PIECE, 0);
			move->qorder=move->real_score=MV_OR+P_OR;
			move++;
			ClrLO(y);
		}
		x = (b->maps[PAWN]) & (b->colormaps[side]) & (~rank) & (~npins);
		y=((x<<8)>>back)& (~b->norm);
		while (y) {
			to = LastOne(y);
			from=to-ff;
//????
			if((normmark[to])&(attack.rays_dir[b->king[side]][from])) {
				move->move = PackMove(from, to,  ER_PIECE, 0);
				move->qorder=move->real_score=MV_OR+P_OR;
				move++;
			}
			ClrLO(y);
		}

// pawn moves for base line - 2squares move needs handling
		x = (b->maps[PAWN]) & (b->colormaps[side]) & (brank) & npins;
		y=((x<<8)>>back)& (~b->norm);
		pmv=((y<<8)>>back)& (~b->norm);
		while(pmv){
			to=LastOne(pmv);
			from=to-ff-ff;
			move->move = PackMove(from, to,  ER_PIECE+1, 0);
			move->qorder=move->real_score=MV_OR+P_OR;
			move++;
			ClrLO(pmv);
		}
		x = (b->maps[PAWN]) & (b->colormaps[side]) & (brank) & (~npins);
		y=((x<<8)>>back)& (~b->norm);
		pmv=((y<<8)>>back)& (~b->norm);
		while(pmv){
			to=LastOne(pmv);
			from=to-ff-ff;
//????			
			if((normmark[to])&(attack.rays_dir[b->king[side]][from])) {
				move->move = PackMove(from, to,  ER_PIECE+1, 0);
				move->qorder=move->real_score=MV_OR+P_OR;
				move++;
			}
			ClrLO(pmv);
		}

// king
		x = (b->maps[KING]) & (b->colormaps[side]);
		while (x) {
			from = LastOne(x);
			mv = (attack.maps[KING][from]) & (~b->norm) & (~attack.maps[KING][b->king[opside]]);
			mv = mv & (~a->att_by_side[opside]);
			while (mv) {
				to = LastOne(mv);
				move->move = PackMove(from, to,  ER_PIECE, 0);
				move->qorder=move->real_score=MV_OR;
				move++;
				ClrLO(mv);
			}
			ClrLO(x);
		}
		
// castling check - je tady i moznost ze druhy kral blokuje nejake pole?
		if(b->castle[side] & QUEENSIDE) {
			BITVAR m1=attack.rays[C1+orank][E1+orank];
			BITVAR m2=attack.rays[B1+orank][D1+orank];
			BITVAR m3=attack.maps[KING][b->king[opside]];
			if((((a->att_by_side[opside]|m3)&m1))||((m2 & b->norm))) {
			} else {
				move->move = PackMove(E1+orank,C1+orank, KING, 0);
				move->qorder=move->real_score=CS_Q_OR;
				move++;
			}
		}
		if(b->castle[side] & KINGSIDE) {
			BITVAR m1=attack.rays[E1+orank][G1+orank];
			BITVAR m2=attack.rays[F1+orank][G1+orank];
			BITVAR m3=attack.maps[KING][b->king[opside]];

			if((((a->att_by_side[opside]|m3)&m1))||((m2 & b->norm))) {
			} else {
				move->move = PackMove(E1+orank,G1+orank, KING, 0);
				move->qorder=move->real_score=CS_K_OR;
				move++;
			}
		}
		*m=move;
}

int isQuietCheckMove(board * b, attack_model *a, move_entry *m)
{
	int movp;
	unsigned char from, to;
	BITVAR r;
	king_eval *ke;

	ke=&(a->ke[b->side^1]);

	from=UnPackFrom(m->move);
	to=UnPackTo(m->move);

	movp=b->pieces[from];
	movp&=PIECEMASK;
	r = 0;

	switch (movp) {
	case PAWN:
		r = ke->pn_pot_att_pos & normmark[to];
		break;
	case BISHOP:
		r = ke->di_blocker_ray & normmark[to];
		break;
	case KNIGHT:
		r = ke->kn_pot_att_pos & normmark[to];
		break;
	case ROOK:
		r= ke->cr_blocker_ray & normmark[to];
		break;
	case QUEEN:
		r = (ke->di_blocker_ray|ke->cr_blocker_ray) & normmark[to];
		break;
	default:
		break;
	}
	return (r!=0);
}

/*
 * tahy ktere vedou na policka, ktera jsou od nepratelskeho krale - krome pinned
 * tahy pinned ktere vedou na ^^ policka a neodkryvaji vlastniho krale
 * tahy figurami, ktere blokuji utok na nepratelskeho krale
 */

void generateQuietCheckMoves(board * b, attack_model *a, move_entry ** m)
{
int from, to;
BITVAR x, mv, rank, brank, pmv, y, npins, block_ray, piece;
move_entry * move;
int back, ff, pie;
unsigned char side, opside;
king_eval *ke;
personality *p;

		p=b->pers;

		move = *m;
		if(b->side == WHITE) {
			rank=RANK7;
			side=WHITE;
			opside=BLACK;
			brank=RANK2;
			back=0;
			ff=8;
		}
		else {
			rank=RANK2;
			opside=WHITE;
			side=BLACK;
			brank=RANK7;
			back=16;
			ff=-8;
		}

		npins=(~(a->ke[side].cr_pins | a->ke[side].di_pins));

		ke=&(a->ke[opside]);
		
// generate rooks &queens other way
		piece=b->maps[QUEEN]&(b->colormaps[side])&npins;
		while(piece) {
			from = LastOne(piece);
			mv=a->mvs[from]& ((ke->cr_blocker_ray)|(ke->di_blocker_ray))& (~b->norm);
			while (mv) {
				to = LastOne(mv);
				move->move = PackMove(from, to,  ER_PIECE, 0);
				move->qorder=move->real_score=MV_OR+Q_OR;
				move++;
				ClrLO(mv);
			}
			ClrLO(piece);
		}

// generate rooks &queens other way
		piece=b->maps[ROOK]&(b->colormaps[side])&npins;
		while(piece) {
			from = LastOne(piece);
			mv=a->mvs[from]& (ke->cr_blocker_ray)& (~b->norm);
			while (mv) {
				to = LastOne(mv);
				move->move = PackMove(from, to,  ER_PIECE, 0);
				move->qorder=move->real_score=MV_OR+R_OR;
				move++;
				ClrLO(mv);
			}
			ClrLO(piece);
		}
		
// bishops	
		piece=b->maps[BISHOP]&(b->colormaps[side])&npins;
		while(piece) {
			from = LastOne(piece);
			mv=a->mvs[from]& (ke->di_blocker_ray)& (~b->norm);
			while (mv) {
				to = LastOne(mv);
				move->move = PackMove(from, to,  ER_PIECE, 0);
				move->qorder=move->real_score=MV_OR+B_OR;
				move++;
				ClrLO(mv);
			}
			ClrLO(piece);
		}
		
// knights
		piece=b->maps[KNIGHT]&(b->colormaps[side])&npins;
		while(piece) {
			from = LastOne(piece);
			mv=a->mvs[from]& (ke->kn_pot_att_pos)& (~b->norm);
			while (mv) {
				to = LastOne(mv);
				move->move = PackMove(from, to,  ER_PIECE, 0);
				move->qorder=move->real_score=MV_OR+N_OR;
				move++;
				ClrLO(mv);
			}
			ClrLO(piece);
		}

// pawn moves
		x = (b->maps[PAWN]) & (b->colormaps[side]) & (~rank) & npins;
		y=((x<<8)>>back)& (~b->norm)&(ke->pn_pot_att_pos);
		while (y) {
			to = LastOne(y);
			move->move = PackMove((to-ff), to,  ER_PIECE, 0);
			move->qorder=move->real_score=MV_OR+P_OR;
			move++;
			ClrLO(y);
		}

		// pawn moves for base line - 2squares move needs handling
		x = (b->maps[PAWN]) & (b->colormaps[side]) & (brank) & npins;
		y=((x<<8)>>back)& (ke->pn_pot_att_pos)& (~b->norm);
		pmv=((y<<8)>>back)& (ke->pn_pot_att_pos)& (~b->norm);
		while(pmv){
			to=LastOne(pmv);
			move->move = PackMove(to-ff-ff, to,  ER_PIECE+1, 0);
			move->qorder=move->real_score=MV_OR+P_OR;
			move++;
			ClrLO(pmv);
		}
// castling check - je tady i moznost ze druhy kral blokuje nejake pole?
// teoreticky je mozne aby rosada davala sach, ale neimplementuju
		*m=move;
}

int kingCheck(board *b)
{
BITVAR x;
int from;
	
		x = (b->maps[KING]) & (b->colormaps[WHITE]);
		from = LastOne(x);
		if((x==0ULL)||(from!=b->king[WHITE])) {
			printf("%lld, %d=%lld %d WHITE\n", (unsigned long long) x,from, 1ULL<<from, b->king[WHITE]);
			return 0;
		} 
		x = (b->maps[KING]) & (b->colormaps[BLACK]);
		from = LastOne(x);
		if((x==0ULL)||(from!=b->king[BLACK])) {
			printf("%lld, %d=%lld %d BLACK\n", (unsigned long long) x,from, 1ULL<<from, b->king[BLACK]);
			return 0;
		} 
		return 1;
}

int boardCheck(board *b, char *name)
{
char bf[2048], b2[512];
int ret,f;
BITVAR bl, wh, no, pa, kn, bi, ro, qu, ki, key;
int blb, whb, pab, knb, bib, rob, qub, kib, matidx, pp, ppp;
//int nob;
	
		ret=1;
		if(b->colormaps[WHITE]&b->colormaps[BLACK]) {
			ret=0;
			LOGGER_1("ERR: %s, Black and white piece on the same square\n", name);
			printmask(b->colormaps[WHITE],"WHITE");
			printmask(b->colormaps[BLACK], "BLACK");
//			printBoardNice(b);
//			return 0;
//			abort();
		}
		key=getKey(b);
		if(b->key!=key) {
			ret=0;
			LOGGER_1("ERR: %s, Keys dont match, board key %lld, computed key %lld\n",name, (unsigned long long) b->key, (unsigned long long) key);
//			printBoardNice(b);
//			return 0;
//			abort();
		}
		matidx=computeMATIdx(b);
		if(b->mindex!=matidx) {
			ret=0;
			LOGGER_1("ERR: %s, Material indexes dont match, board mindex %d, computed mindex %d\n",name, b->mindex, matidx);
//			printBoardNice(b);
//			return 0;
//			abort();
		}
		
		for(f=0; f<64; f++) {
			wh=b->colormaps[WHITE] & normmark[f];
			bl=b->colormaps[BLACK] & normmark[f];
			no=b->norm & normmark[f];
			pa=b->maps[PAWN] & normmark[f];
			bi=b->maps[BISHOP] & normmark[f];
			kn=b->maps[KNIGHT] & normmark[f];
			ki=b->maps[KING] & normmark[f];
			qu=b->maps[QUEEN] & normmark[f];
			ro=b->maps[ROOK] & normmark[f];
			pp=b->pieces[f];

			blb=BitCount(bl);
			whb=BitCount(wh);
//			nob=BitCount(no);
			pab=BitCount(pa);
			bib=BitCount(bi);
			knb=BitCount(kn);
			kib=BitCount(ki);
			qub=BitCount(qu);
			rob=BitCount(ro);

			sprintf(bf,"Square %d: ",f);

// empty place
			if((pp==ER_PIECE ) || (no==0)) {
				if(pp!=ER_PIECE) {
					ret=0;
					strcat(bf,"emp: Piece NOT_E ");
				}
				if(no!=0) {
					ret=0;
					strcat(bf,"emp: NORM NOT_E ");
				}
				if((bl|wh)!=0) {
					ret=0;
					if(bl!=0) strcat(bf,"emp: Black NOT_E ");
					if(wh!=0) strcat(bf,"emp: White NOT_E ");
				}
				if ((pa|kn|bi|ro|qu|ki)!=0) {
					ret=0;
					if(pa!=0) strcat(bf,"emp: PAWN NOT_E ");
					if(kn!=0) strcat(bf,"emp: KNIGHT NOT_E ");
					if(bi!=0) strcat(bf,"emp: BISHOP NOT_E ");
					if(ro!=0) strcat(bf,"emp: ROOK NOT_E ");
					if(qu!=0) strcat(bf,"emp: QUEEN NOT_E ");
					if(ki!=0) strcat(bf,"emp: KING NOT_E ");
				}
			}
// non empty
			else {

				if(pp==ER_PIECE) {
					ret=0;
					strcat(bf,"nmp: Piece IS_E ");
				}
				if(no==0) {
					ret=0;
					strcat(bf,"nmp: NORM IS_E ");
				}
				if((bl|wh)==0) {
					ret=0;
					strcat(bf,"nmp: COLORs IS_E ");
				}
				if((blb+whb)>1) {
					ret=0;
					strcat(bf,"nmp: BOTH colors SET_UP ");
				}
				if(pp!=ER_PIECE) {
					if(pp&BLACKPIECE) {
						if(whb!=0){
							ret=0;
							strcat(bf,"nmp: BLACK piece WHITE map setup ");
						}
						if(blb==0){
							ret=0;
							strcat(bf,"nmp: BLACK piece BLACK map IS_E ");
						}
					} else {
						if(blb!=0){
							ret=0;
							strcat(bf,"nmp: WHITE piece BLACK map setup ");
						}
						if(whb==0){
							ret=0;
							strcat(bf,"nmp: WHITE piece WHITE map IS_E ");
						}
					}
					ppp=pp&PIECEMASK;
					switch(ppp) {
					case 0:
							ppp=pab;
							break;
					case 1:
							ppp=knb;
							break;
					case 2:
							ppp=bib;
							break;
					case 3:
							ppp=rob;
							break;
					case 4:
							ppp=qub;
							break;
					case 5:
							ppp=kib;
							break;
					default:
							ret=0;
							break;
					}
					if(ppp==0) {
						ret=0;
						sprintf(b2, "nmp: PIECE %d map not SETUP ", ppp);
						strcat(bf, b2);
						if((pab+knb+bib+rob+qub+kib)>0) {
							ret=0;
							sprintf(b2, "nmp: OTHER MAPs setup P,N,B,R,Q,K %d,%d,%d,%d,%d,%d ", pab, knb, bib, rob, qub, kib);
							strcat(bf, b2);
						}
					}
					if((pab+knb+bib+rob+qub+kib)>1) {
						ret=0;
						sprintf(b2, "nmp: MORE MAPs setup P,N,B,R,Q,K %d,%d,%d,%d,%d,%d ", pab, knb, bib, rob, qub, kib);
						strcat(bf, b2);
					}

				} else {
					if((pab+knb+bib+rob+qub+kib)>0) {
						ret=0;
						sprintf(b2, "nmp: Some MAPS setup P,N,B,R,Q,K %d,%d,%d,%d,%d,%d ", pab, knb, bib, rob, qub, kib);
						strcat(bf, b2);
					}
				}
			}

			if(pp>(ER_PIECE|BLACKPIECE)||(pp<0)) {
				ret=0;
				sprintf(b2, "Piece value error: %X ", pp);
				strcat(bf, b2);
			}

			if(ret==0) {
				LOGGER_1("ERR:%s, %s\n",name, bf);
				printBoardNice(b);
				printboard(b);
				return 0;
//				abort();
			}
		}
		return ret;
}

/*
		if(b->castle[side] & QUEENSIDE) {
			BITVAR m1=attack.rays[C1+orank][E1+orank];
			BITVAR m2=attack.rays[B1+orank][D1+orank];
			BITVAR m3=attack.maps[KING][b->king[opside]];
			if((((a->att_by_side[opside]|m3)&m1))||((m2 & b->norm))) {
			} else {
				move->move = PackMove(E1+orank,C1+orank, KING, 0);
				move->qorder=move->real_score=CS_Q_OR;
				move++;
			}
		}
		if(b->castle[side] & KINGSIDE) {
			BITVAR m1=attack.rays[E1+orank][G1+orank];
			BITVAR m2=attack.rays[F1+orank][G1+orank];
			BITVAR m3=attack.maps[KING][b->king[opside]];

			if((((a->att_by_side[opside]|m3)&m1))||((m2 & b->norm))) {
			} else {
				move->move = PackMove(E1+orank,G1+orank, KING, 0);
				move->qorder=move->real_score=CS_K_OR;
				move++;
			}
		}

 */


/*
 * Check if move can be valid
 * if at source is our side piece, if at destination is empty or other side piece
 * if there is free path between source and dest (sliders)
 */

int isMoveValid(board *b, MOVESTORE move, int side)
{
int8_t from, to, prom, movp, capp, opside, pside, tot, prank, pfile;
BITVAR bfrom, bto, m, ep;
int ret;
	ret=0;
	from=UnPackFrom(move);
	to=UnPackTo(move);
	prom=UnPackProm(move);
	movp=b->pieces[from];
	bfrom=normmark[from];
	if(side==BLACK) {
	  pside=BLACKPIECE;
	  opside=WHITE;
	  prank=7;
	} else {
	  pside=0;
	  opside=BLACK;
	  prank=0;
	}
	switch (prom) {
	  case KING:
// castling
			if((movp!=(KING|pside))||(from != getPos(E1,prank))) return 0;
			if((getPos(C1,prank))==to) { if(!(b->castle[side]&QUEENSIDE)) return 0; }
			else if((getPos(G1, prank))==to) { if(!(b->castle[side]&KINGSIDE)) return 0; }
			else return 0;
			return 1;
	  case PAWN:
// ep
			if(movp!=(PAWN|pside)) return 0;
			if(b->ep==-1) return 0;
			tot= side==WHITE ? getPos(getFile(b->ep), getRank(b->ep)+1) : getPos(getFile(b->ep), getRank(b->ep)-1);
			if(tot!=to) return 0;
			return 1;
	  case ER_PIECE+1:
// doublepush
			if(movp!=(PAWN|pside)) return 0;
			pfile=getFile(from);
			prank=getRank(from);
			prank= side==WHITE ? prank+16 : prank-16;
			tot=getPos(pfile, prank);
			if(tot!=to) return 1;
			if(b->pieces[tot]!=ER_PIECE) return 1;
			return 1;
	  case ER_PIECE:
// ordinary movement
	  default:
			break;
	}
	
	capp=b->pieces[to];
	bto=normmark[to];
	if(!(bfrom & b->colormaps[side])) return 0;
	if((bto & b->colormaps[side])) return 0;
	m=0;
	
	switch (movp&PIECEMASK) {
	  case BISHOP:
			m=attack.rays[from][to];
//			m=BishopAttacks(b, from);
			break;
	  case QUEEN:
			m=attack.rays[from][to];
//			m=QueenAttacks(b, from);
			break;
	  case ROOK:
			m=attack.rays[from][to];
//			m=RookAttacks(b, from);
			break;
	  case KING:
			m=attack.maps[KING][from];
			break;
	  case KNIGHT:
			m=attack.maps[KNIGHT][from];
			break;
	  case PAWN:
			m=attack.pawn_move[side][from];
			m|=attack.pawn_att[side][from];
			break;
	  default:
			break;
	}
	if(!(m & bto)) return 0;
	
return 1;
}

/*
  Make proposed move and update board information
  - key (hash)
  - bitboards
  - ep
  - sideToMove
  - rule50move
  - move 
  - castling
  - 
  - material[ER_SIDE][ER_PIECE] ???
  - mcount[ER_SIDE] ???
  - king[ER_SIDE] ??????
  - positions[102] ???
  - posnorm[102] ???
  - gamestage ???

  Store information for UNDO the move
  
*/

UNDO MakeMove(board *b, MOVESTORE move)
{
UNDO ret;
int8_t from;
int8_t to;
int8_t prom;
int8_t opside;
int8_t siderooks, opsiderooks;
int8_t oldp, movp, capp;
//int sidemask;
int * tmidx;
int * omidx;
int64_t *tmidx2, *omidx2, midx2;
int midx;

	if(b->side==WHITE) {
			opside=BLACK;
			siderooks=A1;
			opsiderooks=A8;
			tmidx = MATIdxIncW;
			omidx = MATIdxIncB;
			tmidx2 = MATincW2;
			omidx2 = MATincB2;
		} 
		else {
			opside=WHITE;
			siderooks=A8;
			opsiderooks=A1;
			tmidx = MATIdxIncB;
			omidx = MATIdxIncW;
			tmidx2 = MATincB2;
			omidx2 = MATincW2;
		}

	DEB_4(if(computeMATIdx(b)!=b->mindex) {
		printf("mindex problem");
		abort();
	})
	DEB_4({
		BITVAR pix = getPawnKey(b);
		if(pix!=b->pawnkey) {
			printf("#1 PawnHashKey problem! %lX:%lX\n",b->pawnkey, pix);
		}
	})

		ret.move=move;
		ret.side=b->side;
		ret.castle[WHITE]=b->castle[WHITE];
		ret.castle[BLACK]=b->castle[BLACK];
		ret.rule50move=b->rule50move;
		ret.ep=b->ep;
		ret.key=b->key;
		ret.pawnkey=b->pawnkey;
		ret.mindex_validity=b->mindex_validity;
		
		from=UnPackFrom(move);
		to=UnPackTo(move);
		prom=UnPackProm(move);
		capp=ret.captured=b->pieces[to]&PIECEMASK;
		movp=oldp=ret.old=ret.moved=b->pieces[from]&PIECEMASK;
		
/* change HASH:
   - remove ep - set to NO
*/
		if(ret.ep!=-1) {
			b->key^=epKey[ret.ep]; 
		}
		b->ep=-1;

		switch (prom) {
		case ER_PIECE+1:
		case ER_PIECE:
// normal move - no promotion
			if(capp!=ER_PIECE) {
// capture
				ClearAll(to, opside, capp , b);
				b->material[opside][capp]--; // opside material change
				b->rule50move=b->move;
				midx=omidx[capp];
				midx2=omidx2[capp];
// fix for dark bishop
				if(capp==BISHOP) {
					if(normmark[to] & BLACKBITMAP) {
						midx=omidx[BISHOP+ER_PIECE];
						midx2=omidx2[BISHOP+ER_PIECE];
						b->material[opside][BISHOP+ER_PIECE]--;
					}
				}
				else if(capp==PAWN) {
					b->pawnkey^=randomTable[opside][to][PAWN]; //pawnhash
				}
				b->mindex-=midx;
				b->mindex2-=midx2;
				b->key^=randomTable[opside][to][capp];

				if ((to==opsiderooks) && (capp==ROOK)){
/* remove castling opside */
					b->castle[opside] &=(~QUEENSIDE);
					if(b->castle[opside]!=ret.castle[opside])
						b->key^=castleKey[opside][QUEENSIDE];
				}
				else if ((to==(opsiderooks+7)) && (capp==ROOK)) {
					b->castle[opside] &=(~KINGSIDE);
					if(b->castle[opside]!=ret.castle[opside])
						b->key^=castleKey[opside][KINGSIDE];
				}
// check validity of mindex and ev. fix it
				check_mindex_validity(b, 0);
			}
// move part of move. both capture and noncapture
// pawn movement ?
			if(oldp==PAWN) {
				b->rule50move=b->move;
// was it 2 rows ?
				if(((to>from) ? to-from : from-to)==16) b->ep=to;
				b->pawnkey^=randomTable[b->side][from][PAWN]; //pawnhash
				b->pawnkey^=randomTable[b->side][to][PAWN]; //pawnhash
			}
// king moved
			if(oldp==KING) {
				b->castle[b->side]=NOCASTLE;
				b->king[b->side]=to;
				if(b->castle[b->side]!=ret.castle[b->side])
					b->key^=castleKey[b->side][ret.castle[b->side]];
//				b->pawnkey^=randomTable[b->side][from][KING]; //pawnhash
//				b->pawnkey^=randomTable[b->side][to][KING]; //pawnhash
			}
// move side screwed castle ?
// 	was the move from my corners ?
			if((from==siderooks)&& (oldp==ROOK)) {
				b->castle[b->side] &=(~QUEENSIDE);
				if(b->castle[b->side]!=ret.castle[b->side])
					b->key^=castleKey[b->side][QUEENSIDE];
			} else if ((from==(siderooks+7))&& (oldp==ROOK)) {
				b->castle[b->side] &=(~KINGSIDE);
				if(b->castle[b->side]!=ret.castle[b->side])
					b->key^=castleKey[b->side][KINGSIDE];
			} 
			break;
		case KING:
// moves are legal
// castling 
			b->king[b->side]=to;
			b->castle[b->side]=NOCASTLE;
			if(b->castle[b->side]!=ret.castle[b->side])
				b->key^=castleKey[b->side][ret.castle[b->side]];
			if(to>from) {
// kingside castling
				MoveFromTo(from+3, to-1, b->side, ROOK, b);
				b->key^=randomTable[b->side][from+3][ROOK]; //hash
				b->key^=randomTable[b->side][to-1][ROOK]; //hash
			}
			else {
				MoveFromTo(from-4, to+1, b->side, ROOK, b);
				b->key^=randomTable[b->side][from-4][ROOK]; //hash
				b->key^=randomTable[b->side][to+1][ROOK]; //hash
			}

//			b->pawnkey^=randomTable[b->side][from][KING]; //pawnhash
//			b->pawnkey^=randomTable[b->side][to][KING]; //pawnhash
			break;
		case PAWN:
// EP
			ClearAll(ret.ep, opside, PAWN , b);
			b->material[opside][PAWN]--; // opside material change
			b->mindex-=omidx[PAWN];
			b->mindex2-=omidx2[PAWN];

			b->key^=randomTable[opside][ret.ep][PAWN]; //hash
			b->rule50move=b->move;
			b->pawnkey^=randomTable[b->side][from][PAWN]; //pawnhash
			b->pawnkey^=randomTable[b->side][to][PAWN]; //pawnhash
			b->pawnkey^=randomTable[opside][ret.ep][PAWN]; //pawnhash
			break;
			
		default:
// promotion
			if(capp!=ER_PIECE) {
// promotion with capture
				b->key^=randomTable[opside][to][capp]; //hash
				ClearAll(to, opside, capp , b);
				b->material[opside][capp]--; // opside material change
				midx=omidx[capp];
				midx2=omidx2[capp];
// fix for dark bishop
				if(capp==BISHOP) {
					if(normmark[to] & BLACKBITMAP) {
						midx=omidx[BISHOP+ER_PIECE];
						midx2=omidx2[BISHOP+ER_PIECE];
						b->material[opside][BISHOP+ER_PIECE]--;
					}
				}
//				else if(capp==PAWN) {
//					b->pawnkey^=randomTable[opside][to][PAWN]; //pawnhash
//				}
				b->mindex-=midx;
				b->mindex2-=midx2;
				//# fix hash for castling
				if ((to==opsiderooks)&& (capp==ROOK)) {
					b->castle[opside] &=(~QUEENSIDE);
					if(b->castle[opside]!=ret.castle[opside])
						b->key^=castleKey[opside][QUEENSIDE];
				}
				else if ((to==(opsiderooks+7))&& (capp==ROOK)) {
					b->castle[opside] &=(~KINGSIDE);
					if(b->castle[opside]!=ret.castle[opside])
						b->key^=castleKey[opside][KINGSIDE];
				}
			}
			b->pawnkey^=randomTable[b->side][from][PAWN]; //pawnhash
			ret.moved=prom;
			movp=prom;
			b->rule50move=b->move;
			b->material[b->side][PAWN]--; // side material change - PAWN
			b->material[b->side][prom]++; // side material change
			b->mindex-=tmidx[PAWN];
			b->mindex2-=tmidx2[PAWN];
			midx=tmidx[prom];
			midx2=tmidx2[prom];
// fix for dark bishop
			if(prom==BISHOP)
				if(normmark[to] & BLACKBITMAP) {
					midx=tmidx[BISHOP+ER_PIECE];
					midx2=tmidx2[BISHOP+ER_PIECE];
					b->material[b->side][BISHOP+ER_PIECE]++;
				}
			b->mindex+=midx;
			b->mindex2+=midx2;
// check validity of mindex and ev. fix it
			check_mindex_validity(b, 1);
			break;
		}
		if(oldp!=movp) {
			ClearAll(from, b->side, oldp, b);
			SetAll(to, b->side, movp, b);
		}
		else
			MoveFromTo(from, to, b->side, oldp, b);
			
/* change HASH:
   - update target
   - restore source
   - set ep
   - change side
   - set castling to proper state
   - update 50key and 50position restoration info
*/
		b->key^=randomTable[b->side][from][oldp]; //hash
		b->key^=randomTable[b->side][to][movp]; //hash
		b->key^=sideKey; //hash

		if(b->ep!=-1) {
			b->key^=epKey[b->ep]; 
		}
//!!		ret.old50key=b->positions[b->rule50move];
//!!		ret.old50pos=b->posnorm[b->rule50move];

//		b->positions[b->rule50move]=b->key;
//		b->posnorm[b->rule50move]=b->norm;
		b->move++;
		b->positions[b->move-b->move_start]=b->key;
		b->posnorm[b->move-b->move_start]=b->norm;

//!!		b->rule50move++;
		b->side=opside;
		DEB_4(if(computeMATIdx(b)!=b->mindex) {
			printf("mindex problem");
			printBoardNice(b);
			abort();
		})
#if 0
int bwl, bbl, bwd, bbd;
		bwl=BitCount(b->maps[BISHOP]&(b->colormaps[WHITE]));
		bbl=BitCount(b->maps[BISHOP]&(b->colormaps[BLACK]));
		bwd=BitCount(b->maps[BISHOP]&(b->colormaps[WHITE])&BLACKBITMAP);
		bbd=BitCount(b->maps[BISHOP]&(b->colormaps[BLACK])&BLACKBITMAP);
		if((bwl!=b->material[WHITE][BISHOP])||(bbl!=b->material[BLACK][BISHOP])
			||(bwd!=b->material[WHITE][BISHOP+ER_PIECE])||(bbd!=b->material[BLACK][BISHOP+ER_PIECE])){
			LOGGER_0("BITC: bwl %d, bwd %d, bbl %d, bbd %d\n", bwl, bwd, bbl, bbd);
			LOGGER_0("MATS: bwl %d, bwd %d, bbl %d, bbd %d\n", b->material[WHITE][BISHOP], b->material[WHITE][BISHOP+ER_PIECE],
				bbl!=b->material[BLACK][BISHOP], bbd!=b->material[BLACK][BISHOP+ER_PIECE]);
			abort();
		}
#endif
	DEB_4({
		BITVAR pix = getPawnKey(b);
		if(pix!=b->pawnkey) {
			printf("#2 PawnHashKey problem! %lX:%lX\n",b->pawnkey, pix);
			b->pawnkey=pix;
		}
	})
//	printf("b1");
return ret;
}

UNDO MakeNullMove(board *b)
{
UNDO ret;
int8_t opside;
		
	if(b->side==WHITE) {
			opside=BLACK;
		} 
		else {
			opside=WHITE;
		}

	ret.move=NULL_MOVE;
	ret.side=b->side;
	ret.castle[WHITE]=b->castle[WHITE];
	ret.castle[BLACK]=b->castle[BLACK];
	ret.rule50move=b->rule50move;
	ret.ep=b->ep;
	ret.key=b->key;
	ret.pawnkey=b->pawnkey;
	ret.mindex_validity=b->mindex_validity;
	
	if(b->ep!=-1) b->key^=epKey[b->ep]; 
	b->ep=-1;
	b->key^=sideKey; //hash
	b->rule50move=b->move;
	b->move++;
	b->positions[b->move-b->move_start]=b->key;
	b->posnorm[b->move-b->move_start]=b->norm;
	b->side=opside;
return ret;
}

void UnMakeNullMove(board *b, UNDO u)
{
		b->ep=u.ep;
		b->move--;
		b->rule50move=u.rule50move;
		b->castle[WHITE]=u.castle[WHITE];
		b->castle[BLACK]=u.castle[BLACK];
		b->side=u.side;
		b->key=u.key;
		b->mindex_validity=u.mindex_validity;
}

void UnMakeMove(board *b, UNDO u)
{
int8_t from, to, prom;
int midx;
int64_t midx2, *xmidx2;
int * xmidx;

		from=UnPackFrom(u.move);
		to=UnPackTo(u.move);
		b->mindex_validity=u.mindex_validity;
		b->ep=u.ep;
		b->move--;
		b->rule50move=u.rule50move;
		b->castle[WHITE]=u.castle[WHITE];
		b->castle[BLACK]=u.castle[BLACK];

		if(u.moved!=u.old) {
			ClearAll(to, u.side, u.moved, b);
			SetAll(from, u.side, u.old, b);
		}
		else
			MoveFromTo(to, from, u.side, u.old, b); //moving actually backwards

		if(u.captured!=ER_PIECE) {
// ep is not recorded as capture!!!
			SetAll(to, b->side, u.captured, b);
			b->material[b->side][u.captured]++; // opside material change
			if(b->side == WHITE) {
				xmidx=MATIdxIncW;
				xmidx2=MATincW2;
			} else {
				xmidx=MATIdxIncB;
				xmidx2=MATincB2;
			}
			midx=xmidx[u.captured];
			midx2=xmidx2[u.captured];
			if(u.captured == BISHOP)
				if(normmark[to] & BLACKBITMAP) {
					midx=xmidx[BISHOP+ER_PIECE];
					midx2=xmidx2[BISHOP+ER_PIECE];
					b->material[b->side][BISHOP+ER_PIECE]++;
				}
			b->mindex+=midx;
			b->mindex2+=midx2;
		} else {
		}
		if(u.old==KING) b->king[u.side]=from;
		prom=UnPackProm(u.move);
		switch(prom) {
		case KING:
// castle ... just fix the rook position
			if(to>from) {
				MoveFromTo(to-1, from+3, u.side, ROOK, b);
			}
			else {
				MoveFromTo(to+1, from-4, u.side, ROOK, b);
			}
			break;
		case PAWN:
			SetAll(u.ep, b->side, PAWN, b);
			b->material[b->side][PAWN]++; // opside material change
			if(b->side == WHITE) {
				xmidx=MATIdxIncW;
				xmidx2=MATincW2;
			} else {
				xmidx=MATIdxIncB;
				xmidx2=MATincB2;
			}
			b->mindex+=xmidx[PAWN];
			b->mindex2+=xmidx2[PAWN];
			break;

		case ER_PIECE+1:
		case ER_PIECE:
			break;
		default:
			b->material[u.side][PAWN]++; // side material change
			b->material[u.side][u.moved]--; // side material change
			if(u.side == WHITE) {
				xmidx=MATIdxIncW;
				xmidx2=MATincW2;
			} else {
				xmidx=MATIdxIncB;
				xmidx2=MATincB2;
			}
			b->mindex+=xmidx[PAWN];
			b->mindex2+=xmidx2[PAWN];
			midx=xmidx[u.moved];
			midx2=xmidx2[u.moved];
			if(u.moved == BISHOP)
				if(normmark[to] & BLACKBITMAP) {
					midx=xmidx[BISHOP+ER_PIECE];
					midx2=xmidx2[BISHOP+ER_PIECE];
					b->material[u.side][BISHOP+ER_PIECE]--;
				}
			b->mindex-=midx;
			b->mindex2-=midx2;
			break;
		}
		b->side=u.side;
		b->key=u.key;
		b->pawnkey=u.pawnkey;

		DEB_4(if(computeMATIdx(b)!=b->mindex) {
			printf("mindex problem");
			abort();
		})
}


/*
 *  na konci generation tahu neni overeno ze tahy kralem neskonci v sachu a ze tah PIN figurou
 *   neuvede krale do sachu
 *  naproti tomu moznost rosady je overena
 */

/*
		Generate moves when King is in check
		o one attacker
			- king moves away
			- piece (including king) captures
			o attacker is sliding piece (bishop, rook queen)
				- piece blocks attacker
			o special case - after pawn double move king is in check, which can be resolved with EP...
		o more attackers
			- king moves away
			- king captures unprotected attackers
*/
			
void  generateInCheckMoves(board * b, attack_model *a, move_entry ** m)
{
BITVAR at2, at4, utc, mezi, pole, pd1, pd2;
	
int from, to, num;
BITVAR x, mv, rank, pmv, brank, npins, block_ray;
move_entry * move;
int ep_add, pie, f;
unsigned char side, opside;
//king_eval ke;
personality *p;

		p=b->pers;
	
		move = *m;
		if(b->side == WHITE) {
			rank=RANK7;
			side=WHITE;
			opside=BLACK;
			brank=RANK2;
			ep_add=8;
			pie=0;
		}
		else {
			rank=RANK2;
			opside=WHITE;
			side=BLACK;
			brank=RANK7;
			ep_add=-8;
			pie=BLACKPIECE;
		}

		npins=(~(a->ke[side].cr_pins | a->ke[side].di_pins));

		at4=a->ke[side].attackers;

/*
 * v teto situaci
 * kral bud muze uhnout mimo utok, nebo sebrat utocnika
 * jine figury mohou blokovat utok (stoupnout si mezi utocnika a krale) nebo vzit utocnika (je li jeden) - je treba dbat na PIN
 */


#if 0
		if(at!=at4) {
			printBoardNice(b);
			printmask(at,"OLD");
			printmask(at4,"NEW");
			printmask(a->ke[side].cr_attackers,"CR");
			printmask(a->ke[side].di_attackers,"DI");
			printmask(a->ke[side].kn_attackers,"KN");
			printmask(a->ke[side].pn_attackers,"PN");
		}
#endif
// count attackers
		num=BitCount(at4);
		from=0; //dummy!
		
// king moves + captures
//		printBoardNice(b);
		x = (b->maps[KING]) & (b->colormaps[side]);
		while (x) {
			from = LastOne(x);
			ClearAll(from, side, KING, b);
			mv = (attack.maps[KING][from])	& (~b->colormaps[side]) & (~attack.maps[KING][b->king[opside]]);
			mv = mv & (~a->att_by_side[opside]) & (~a->ke[side].cr_att_ray) & (~a->ke[side].di_att_ray);
//			mv = mv & (~a->att_by_side[opside]);
#if 0
			printmask(a->att_by_side[opside],"att by side");
			printmask(a->ke[side].cr_att_ray,"cr_att_ray");
			printmask(a->ke[side].di_att_ray,"di_att_ray");
			printmask((a->att_by_side[opside]|a->ke[side].cr_att_ray|a->ke[side].di_att_ray)^a->att_by_side[opside],"XOR");
#endif
			while (mv) {
				to = LastOne(mv);
//Fix!!!
// je to pro situaci kdy kral v sachu odstupuje od utocnika ve smeru utoku...

//				if(!AttackedTo_B(b, to, side)) {
					move->move = PackMove(from, to, ER_PIECE, 0);
					move->qorder=move->real_score=b->pers->LVAcap[KING][b->pieces[to]&PIECEMASK];
					move++;
//				}
				mv =ClrNorm(to,mv);
			}	
			x=ClrNorm(from,x);
			SetAll(from, side, KING, b);
		}
		
		at2=0;
		//at3=at4;
		utc=0;
		mezi=0;
		pole=0;

// sebrani utocnika a blokovani
		if(num==1) {
			utc=at4;
			to = LastOne(at4);
			if( ! a->ke[side].kn_attackers)
			{
				at2 = a->ke[side].cr_att_ray | a->ke[side].di_att_ray;
//				at3 = at4;
				at4 |= at2;
				mezi=attack.rays_int[from][to];
			} 
			pole=mezi|utc;

		for(f=a->pos_c[QUEEN|pie]; f>=0; f--) {
			from=a->pos_m[QUEEN|pie][f];
			mv=a->mvs[from] & pole;
			if(normmark[from]&(~npins)) mv&=attack.rays_dir[b->king[side]][from];
			while (mv) {
				to = LastOne(mv);
				move->move = PackMove(from, to,  ER_PIECE, 0);
				if(normmark[to]&utc) {
					move->qorder=move->real_score=b->pers->LVAcap[QUEEN][b->pieces[to]&PIECEMASK];
				} else {
					move->qorder=move->real_score=MV_OR+Q_OR;
				}
				move++;
				ClrLO(mv);
			}
		}
	// rooks				
		for(f=a->pos_c[ROOK|pie]; f>=0; f--) {
			from=a->pos_m[ROOK|pie][f];
			mv=a->mvs[from] & pole;
			if(normmark[from]&(~npins)) mv&=attack.rays_dir[b->king[side]][from];
			while (mv) {
				to = LastOne(mv);
				move->move = PackMove(from, to,  ER_PIECE, 0);
				if(normmark[to]&utc) {
					move->qorder=move->real_score=b->pers->LVAcap[ROOK][b->pieces[to]&PIECEMASK];
				} else {
					move->qorder=move->real_score=MV_OR+R_OR;
				}
				move++;
				ClrLO(mv);
			}
		}
	// bishops
		for(f=a->pos_c[BISHOP|pie]; f>=0; f--) {
			from=a->pos_m[BISHOP|pie][f];
			mv=a->mvs[from] & pole;
			if(normmark[from]&(~npins)) mv&=attack.rays_dir[b->king[side]][from];
			while (mv) {
				to = LastOne(mv);
				move->move = PackMove(from, to,  ER_PIECE, 0);
				if(normmark[to]&utc) {
					move->qorder=move->real_score=b->pers->LVAcap[BISHOP][b->pieces[to]&PIECEMASK];
				} else {
					move->qorder=move->real_score=MV_OR+B_OR;
				}
				move++;
				ClrLO(mv);
			}
		}
		// knights
		for(f=a->pos_c[KNIGHT|pie]; f>=0; f--) {
			from=a->pos_m[KNIGHT|pie][f];
			mv=a->mvs[from] & pole;
			if(normmark[from]&(~npins)) mv&=attack.rays_dir[b->king[side]][from];
			while (mv) {
				to = LastOne(mv);
				move->move = PackMove(from, to,  ER_PIECE, 0);
				if(normmark[to]&utc) {
					move->qorder=move->real_score=b->pers->LVAcap[KNIGHT][b->pieces[to]&PIECEMASK];
				} else {
					move->qorder=move->real_score=MV_OR+N_OR;
				}
				move++;
				ClrLO(mv);
			}
		}
//FIXME cele pesce predelat a vsude stejne!!!
// pawn promotions extra with capture or move in
			x = (b->maps[PAWN]) & (b->colormaps[side]) & rank;
			while (x) {
				from = LastOne(x);
				mv = (attack.pawn_att[side][from] & utc)|(attack.pawn_move[side][from] & mezi & ~b->norm);
				if(normmark[from]&(~npins)) mv&=attack.rays_dir[b->king[side]][from];
				while (mv) {
					to = LastOne(mv);
					if(normmark[to]&utc) {
						move->move = PackMove(from, to,  QUEEN, 0);
						move->qorder=move->real_score=b->pers->LVAcap[KING+1][b->pieces[to]&PIECEMASK];
						move++;
						move->move = PackMove(from, to,  KNIGHT, 0);
						move->qorder=move->real_score=b->pers->LVAcap[KING+2][b->pieces[to]&PIECEMASK];
						move++;
						move->move = PackMove(from, to,  BISHOP, 0);
						move->qorder=move->real_score=A_OR2;
						move++;
						move->move = PackMove(from, to,  ROOK, 0);
						move->qorder=move->real_score=A_OR2;
						move++;
					} else {
						move->move = PackMove(from, to,  QUEEN, 0);
						move->qorder=move->real_score=A_QUEEN_PROM;
						move++;
						move->move = PackMove(from, to,  KNIGHT, 0);
						move->qorder=move->real_score=A_KNIGHT_PROM;
						move++;
						move->move = PackMove(from, to,  BISHOP, 0);
						move->qorder=move->real_score=A_MINOR_PROM+B_OR;
						move++;
						move->move = PackMove(from, to,  ROOK, 0);
						move->qorder=move->real_score=A_MINOR_PROM+R_OR;
						move++;
					}
					mv =ClrNorm(to,mv);
				}
				x=ClrNorm(from,x);
			}
// pawn attacks & moves in
			x = (b->maps[PAWN]) & (b->colormaps[side]) & (~rank) & (~brank);
			while (x) {
				from = LastOne(x);
				mv = (attack.pawn_att[side][from] & utc)|(attack.pawn_move[side][from] & mezi & ~b->norm);
				if(normmark[from]&(~npins)) mv&=attack.rays_dir[b->king[side]][from];
				while (mv) {
					to = LastOne(mv);
					move->move = PackMove(from, to, ER_PIECE, 0);
					if(normmark[to]&utc) {
						move->qorder=move->real_score=b->pers->LVAcap[PAWN][b->pieces[to]&PIECEMASK];
					} else {
						move->qorder=move->real_score=MV_OR+P_OR;
					}
					move++;
					mv =ClrNorm(to,mv);
				}
				x=ClrNorm(from,x);
			}
// predelat!!!!
			x = (b->maps[PAWN]) & (b->colormaps[side]) & (~rank) & (brank);
			if(x>0){
				if(side==WHITE) {
					pd1= (x<<8) & (~b->norm);
					pd2= (pd1<<8) & (~b->norm);
					pmv= (pd1|pd2) & mezi;
				}
				else{
					pd1= (x>>8) & (~b->norm);
					pd2= (pd1>>8) & (~b->norm);
					pmv= (pd1|pd2) & mezi;
				}
				while (x) {
					from = LastOne(x);
					mv = (attack.pawn_att[side][from] & utc)|(attack.pawn_move[side][from]&pmv);
					if(normmark[from]&(~npins)) mv&=attack.rays_dir[b->king[side]][from];
					while (mv) {
						to = LastOne(mv);
						if(((b->side==WHITE) ? to-from : from-to)==16) {
							move->move = PackMove(from, to, ER_PIECE+1, 0);
						}
						else move->move = PackMove(from, to, ER_PIECE, 0);
						if(normmark[to]&utc) {
							move->qorder=move->real_score=b->pers->LVAcap[PAWN][b->pieces[to]&PIECEMASK];
						} else {
							move->qorder=move->real_score=MV_OR+P_OR;
						}
						move++;
						mv =ClrNorm(to,mv);
					}
					x=ClrNorm(from,x);
				}
			}
		}
// ep attacks ???????????????????
		if(b->ep!=-1) {
// check if that pawn attacks the king
			if ((utc & normmark[b->ep])!=0) {
// can I take it with other pawn?
				x = (attack.ep_mask[b->ep]) & (b->maps[PAWN]) & (b->colormaps[side]) & npins;
				while (x) {
					from = LastOne(x);
					to=b->ep+ep_add;
					move->move = PackMove(from, to, PAWN, 0);
					LOGGER_4("GenerateInCheck EP\n");
					move->qorder=move->real_score=b->pers->LVAcap[PAWN][PAWN];
					move++;
					x=ClrNorm(from,x);
				}
			}
		}
		*m=move;
}

int alternateMovGen(board * b, MOVESTORE *filter){

//fixme all!!!
int i,f,n, tc,cc,t,th, sp,pr, op, f1, f2, t1, t2, pm, rr, opside, piece, ff;
int8_t t2t;
move_entry mm[300], *m;
attack_model *a, aa;
char b2[512], b3[512];

	m = mm;
	a=&aa;
// is side to move in check ?

	opside = (b->side == WHITE) ? BLACK:WHITE;
	eval_king_checks_all(b, a);
	a->phase=eval_phase(b, b->pers);
	simple_pre_movegen(b, a, b->side);
	simple_pre_movegen(b, a, opside);

	if(isInCheck_Eval(b, a, b->side)!=0) generateInCheckMoves(b, a, &m);
	else {
		generateCaptures(b, a, &m, 1);
		generateMoves(b, a, &m);
	}
	n=i=tc=0;
	if(b->side==1) pm=BLACKPIECE; else pm=0;
// fix filter
/*
 * prom field
 * 		PAWN means EP
 * 		KING means Castling
 *		ER_PIECE+1 means DoublePush
 * 		 fix the prom field!
 */
	while((filter[n]!=0)){
		tc=(int)(m-mm);
		cc = 0;

		th=filter[n];
		t=UnPackPPos(th);
		f2=UnPackFrom(th);
		t2=UnPackTo(th);
		piece=b->pieces[f2];
// if filter is castling, we have to normalize to E1-G1 (as it could be written as E1-H1 as well)
		switch(piece&PIECEMASK) {
		case KING:
			if((f2==(E1+b->side*56)) && ((t2==(A1+b->side*56))||(t2==(C1+b->side*56)))) {
				t2=(C1+b->side*56);
				th=PackMove(f2, t2, KING, 0);
				t=UnPackPPos(th);
			} else if((f2==(E1+b->side*56)) && ((t2==(H1+b->side*56))||(t2==(G1+b->side*56)))) {
				t2=(G1+b->side*56);
				th=PackMove(f2, t2, KING, 0);
				t=UnPackPPos(th);
			}
		  break;
		case PAWN:
// test for EP
// b->ep points to target if EP is available
		  t2t=t2;
		  LOGGER_4("t2t %x, %x\n", t2t, b->ep);
		  if(b->side==WHITE) t2t-=8; else t2t+=8;
		  LOGGER_4("t2t %x, %x\n", t2t, b->ep);
		LOGGER_4("ALT EP %d: EP test for from %x to %x/%x %x, ep %x\n", n, f2, getFile(t2), getFile(t2t), t, b->ep);
		  if((b->ep!=-1)&&(b->ep==t2t) && (getFile(t2)==getFile(t2t))){
			th=PackMove(f2, t2, PAWN, 0);
		  }
		  else {
			  ff = (b->side==WHITE) ? t2-f2:f2-t2;
			  if(ff==16) th=PackMove(f2, t2, ER_PIECE+1, 0);
//			  else th=PackMove(f2, t2, ER_PIECE, 0);
		  }
		  break;
		case ER_PIECE:
			printBoardNice(b);
			sprintfMove(b, *filter, b2);
			LOGGER_0("no piece at FROM %s\n",b2);
			dump_moves(b, mm, tc, 1, NULL);
			abort();
		  break;
		default:
		}
		filter[n]=th;
		sprintfMove(b, th, b2);
		while((cc<tc)) {
			DEB_4(sprintfMove(b, mm[cc].move, b3);)
			LOGGER_4("%d:%d Filter %s vs %s, %x vs %x ", n, cc, b2, b3, th, mm[cc].move);
			if((mm[cc].move&(~CHECKFLAG))==th) {
			  NLOGGER_4("equal\n");
			  mm[i++].move=mm[cc].move;
			} else NLOGGER_4(" ne\n");
			cc++;
		}
		n++;
	}
	DEB_4(
	if(i!=1) {
		printBoardNice(b);
		sprintfMove(b, *filter, b2);
		LOGGER_0("INFO3: move problem, %d, move %s, m-mm %ld, tc %d, cc %d\n",i,b2, m-mm, tc, cc);
		dump_moves(b, mm, tc, 1, NULL);
	})
	mm[i].move=0;
	f=0;
	while(mm[f].move!=0) {
		filter[f]=mm[f].move;
		f++;
	}
	return f;
}

int getNSortedBadCap(board *b, move_entry *n, int total, int start){
int f, seex;

//dump_moves(b, n, total, 1, "BadCap sort Start\n");

	for(f=start;f<total;f++) {
		if(((n[f].qorder>=A_OR2)&&(n[f].qorder<=A_OR2_MAX))){
			if(n[f].qorder & 1) n[f].qorder = n[f].qorder+A_OR-A_OR2;
			else {
				seex=SEE(b,n[f].move);
				n[f].qorder= seex>0 ? n[f].qorder+A_OR-A_OR2 : n[f].qorder+MV_BAD-A_OR2;
			}
		}
	}
//	dump_moves(b, n, total, 1, "BadCap sort End\n");
return 0;
}

//+PSQSearch(from, to, KNIGHT, side, a->phase, p)

int getNSortedNonCap(board *b, move_entry *n, int total, int start){
int f, seex, count, max, q, tmp1, tmp2, count2;
int ord[300], prio[300], ord2[300], prio2[300];
int fromPos, ToPos, side, piece;
uint8_t phase;

// get HH info for relevant moves
//	dump_moves(b, n, total, 1, "NONCap sort Start!!\n");
	count=count2=0;
	phase=eval_phase(b, b->pers);
	fromPos=UnPackFrom(n[start].move);
	side=((b->pieces[fromPos]&BLACKPIECE)==0) ? WHITE : BLACK;
	for(f=start;f<total;f++) {
		if(((n[f].qorder>=MV_OR)&&(n[f].qorder<=MV_OR_MAX))){
			ord[count]=f;
			fromPos=UnPackFrom(n[f].move);
			ToPos=UnPackTo(n[f].move);
			piece=b->pieces[fromPos]&PIECEMASK;
			prio[count]=checkHHTable(b->hht, side, piece, ToPos);
			if(prio[count]>0) count++; else {
				ord2[count2]=f;
				prio2[count2]=PSQSearch(fromPos, ToPos, piece, side, phase, b->pers);
//				prio2[count2]=100-f;
				count2++;
			}
		}
	}
//	LOGGER_0("HHcount %d, count2 %d\n", count, count2 );
	// sort HH values
	for(f=0;f<(count-1);f++) {
		max=q=f;
		q++;
		for(;q<(count);q++) {
			if(prio[max]<prio[q]) max=q;
		}
		if(max!=f) {
			tmp1=prio[f];
			tmp2=ord[f];
			prio[f]=prio[max];
			ord[f]=ord[max];
			prio[max]=tmp1;
			ord[max]=tmp2;
		}
	}	
	// sort PST values
	for(f=0;f<(count2-1);f++) {
		max=q=f;
		q++;
		for(;q<(count2);q++) {
			if(prio2[max]<prio2[q]) max=q;
		}
		if(max!=f) {
			tmp1=prio2[f];
			tmp2=ord2[f];
			prio2[f]=prio2[max];
			ord2[f]=ord2[max];
			prio2[max]=tmp1;
			ord2[max]=tmp2;
		}
	}	
	// HH is first PST after 
	for(f=0;f<count2;f++){
		ord[f+count]=ord2[f];
		prio[f+count]=prio2[f];
	}
	
	max=MV_HH_MAX;
	for(f=0;f<count;f++) {
		n[ord[f]].real_score=prio[f];
		n[ord[f]].qorder=max--;
	}
	max=MV_HH_MAX-100;
	for(f=0;f<count2;f++) {
		n[ord2[f]].real_score=prio2[f];
		n[ord2[f]].qorder=max--;
	}
//	assert(total==(count+count2));
//	dump_moves(b, n, total, 1, "NONCap sort END!!\n");
return 0;
}

/*
 * Sorts moves, they should be stored in order generated
 * it should resort captures to MVVLVA 
 * "bad" captures are rechecked via SEE - triggered in sorting
 * noncaptures sorted with HHeuristics - triggered in sorting
 */

int getNSorted(board *b, move_entry *n, int total, int start, int count, int *state){
int f, q, max, scant,r, i1, i2;
char bx2[256];
	move_entry move;
	
	count+=start;
	if(count>total) count=total;
	
	for(f=start;f<count;f++) {
		if(((n[f].qorder>=A_OR2)&&(n[f].qorder<=A_OR2_MAX))) getNSortedBadCap(b, n, total, f);
		else if(((n[f].qorder>=MV_OR)&&(n[f].qorder<=MV_OR_MAX))) getNSortedNonCap(b, n, total, f);
	}
	for(f=start;f<count;f++) {
		max=q=f;
		q++;
		for(;q<(total);q++) {
// check if second is special		
			if(n[max].qorder<n[q].qorder) {
				max=q;
			}
		}
		if(max!=f) {
			move=n[f];
			n[f]=n[max];
			n[max]=move;
		}
	}
	r=f;
	
#if defined (DEBUG4)
	for(f=1;f<count;f++) {
		if(n[f].qorder>n[f-1].qorder) {
			LOGGER_0("Total %d. Start %d, Count %d\n", total, start, count);
			LOGGER_0("Sort problem!!!");
			dump_moves(b, n, total, 1, "Sort Problem\n");
			LOGGER_0("position %d\n", f-1);
			
		}
	}
#endif	
	return r;
}

int getQNSorted(board *b, move_entry *n, int total, int start, int count){
	int f, q, max;
	move_entry move;
		count+=start;
	if(count>total) count=total;
	count--;
	// do the actual sorting
	for(f=start;f<=count;f++) {
		max=q=f;
		q++;
		for(;q<(total);q++) {
			if(n[max].qorder<n[q].qorder) max=q;
		}
		if(max!=f) {
			move=n[f];
			n[f]=n[max];
			n[max]=move;
		}
	}
	return f;
}

/*
		sort moves and filter out invalid castling,
		left in check moves
*/

int xMoveList_Legal(board *b, attack_model *a, int  h, move_entry *n, int count, int ply, int sort)
{
int f, c;
int64_t sc;
int from, to, prom, del;
unsigned char pfrom;

BITVAR x;

	c=0;
	sc=0;

	a->pins = generatePins_eval(b, a, b->side);

	for(f=0;f<count;f++) {
		
		from=UnPackFrom(n[f].move);
		to=UnPackTo(n[f].move);
		pfrom=b->pieces[from]&PIECEMASK;

		if(a->pins & (normmark[UnPackFrom(n[f].move)])) {
				del=-1;
				prom=UnPackProm(n[f].move);
				if((pfrom==PAWN)&(prom==PAWN)) {
					del=b->ep;
				}
				x=isInCheck_after_move(b, a, from, to, del);
				pincount[pfrom]++;
				if(x!=0) {
					pindrop[pfrom]++;
					printBoardNice(b);
					printmask(a->ke[WHITE].cr_pins,"cr pins white");
					printmask(a->ke[WHITE].di_pins,"di pins white");
					printmask(a->ke[BLACK].cr_pins,"cr pins black");
					printmask(a->ke[BLACK].di_pins,"di pins black");
					continue;
				}
		}

		n[c]=n[f];
		sc+=n[c].qorder;
		c++;
	}
return c;
}

void SelectBest(move_cont *mv)
{
move_entry *t, a1, a2;
//	t=mv->lastp-1;
	for(t=mv->lastp-1;t>(mv->next+1); t--) {
	  if(t->qorder> (t-1)->qorder) {
		a1=*t;
		a2=*(t-1);
		*(t-1)=a1;
		*t=a2;
	  }
	}
}

void ScoreNormal(board *b, move_cont *mv, int side){
move_entry *t;
int fromPos, ToPos, piece;
	for(t=mv->lastp-1;t>mv->next; t--) {
			fromPos=UnPackFrom(t->move);
			ToPos=UnPackTo(t->move);
			piece=b->pieces[fromPos]&PIECEMASK;
			t->qorder=checkHHTable(b->hht, side, piece, ToPos);
	}
//	LOGGER_0("HHcount %d, count2 %d\n", count, count2 );
}

int getNextMove(board *b, attack_model *a, move_cont *mv, int ply, int side, int incheck, move_entry **mm){
MOVESTORE pot;
int r;
	switch (mv->phase) {
	case INIT:
	// setup everything
		mv->lastp=mv->next=mv->move;
		mv->badp=mv->bad;
		mv->exclp=mv->excl;
		mv->count=0;
		mv->phase=PVLINE;
// previous PV move
	case PVLINE:
		mv->phase=GENERATE_CAPTURES;
	case GENERATE_CAPTURES:
		mv->phase=CAPTURES;
		
		mv->lastp=mv->next;
		if(incheck==1) {
				generateInCheckMoves(b, a, &(mv->lastp));
				goto rest_moves;
			}
		else generateCaptures(b, a, &(mv->lastp), 1);
		
	case CAPTURES:
		while(mv->next<mv->lastp) {
			if(mv->next==(mv->lastp-1)) mv->phase=KILLER1;
			SelectBest(mv);
			if(((mv->next->qorder<A_OR2_MAX)&&(mv->next->qorder>A_OR2)) && (SEE(b, mv->next->move)<0)) {
				*(mv->badp)=*(mv->next);
				mv->next++;
				continue;
			}
			*mm=mv->next;
			mv->next++;
			return ++mv->count;
		}
	case KILLER1:
		mv->phase=KILLER2;
		r = get_killer_move(b->kmove, ply, 0, &pot);
		if(r && isMoveValid(b,pot, side)) {
			mv->next->move=pot;
			*mm=mv->next;
			mv->next++;
			return ++mv->count;
		}
	case KILLER2:
		mv->phase=KILLER3;
		r = get_killer_move(b->kmove, ply, 1, &pot);
		if(r && isMoveValid(b,pot, side)) {
			mv->next->move=pot;
			*mm=mv->next;
			mv->next++;
			return ++mv->count;
		}
	case KILLER3:
		mv->phase=KILLER4;
		if(ply>2) {
			r = get_killer_move(b->kmove, ply-2, 0, &pot);
			if(r && isMoveValid(b,pot, side)) {
				mv->next->move=pot;
				*mm=mv->next;
				mv->next++;
				return ++mv->count;
			}
		}
	case KILLER4:
		mv->phase=GENERATE_NORMAL;
		if(ply>2) {
			r = get_killer_move(b->kmove, ply-2, 1, &pot);
			if(r && isMoveValid(b,pot, side)) {
				mv->next->move=pot;
				*mm=mv->next;
				mv->next++;
				return ++mv->count;
			}
		}
	case GENERATE_NORMAL:
		generateMoves(b, a, &(mv->lastp));
		// get HH values and sort
rest_moves:
		mv->phase=NORMAL;
	case NORMAL:
		while(mv->next<mv->lastp) {
			SelectBest(mv);
			if(mv->next==(mv->lastp-1)) mv->phase=OTHER;
			*mm=mv->next;
			mv->next++;
			return ++mv->count;
		}
		mv->phase=OTHER;
		mv->next=mv->bad;
	case OTHER:
// bad captures
		while(mv->next<mv->badp) {
			SelectBest(mv);
			if(mv->next==(mv->lastp-1)) mv->phase=OTHER;
			*mm=mv->next;
			mv->next++;
			return ++mv->count;
		}

	default:
	}
return 0;
}

int sortMoveListNew_Init(board *b, attack_model *a, move_cont *mv) {
	mv->phase=0;
	return 0;
}

int sortMoveList_Init(board *b, attack_model *a, int  h, move_entry *n, int count, int ply, int sort)
{
int c, q, sc;

int i, f, scant;

	q=count;
	if((h!=DRAW_M)&&(h!=NA_MOVE)) {
		for(q=0;q<count;q++) {
			if((n[q].move==h)) {
				n[q].qorder+=HASH_OR;
				break;
			}
		}
	}
	if((b->pers->use_killer>=1)) {
		for(q=0;q<count;q++) {
			i=check_killer_move(b->kmove, ply, n[q].move, b->stats);
			if((i>0)&&(n[q].qorder<KILLER_OR)) {
				n[q].qorder=(unsigned int)(KILLER_OR_MAX-i);
			}
		}
	}

#if 0	
	scant=count;
	for(f=0;f<scant;f++) {
		if(((n[f].qorder>=A_OR2)&&(n[f].qorder<=A_OR2_MAX))){
			getNSortedBadCap(b, n, count, f);
		} else if(((n[f].qorder>=MV_OR)&&(n[f].qorder<=MV_OR_MAX))){
			getNSortedNonCap(b, n, count, f);
		}
	}
#endif

//	getNSortedBadCap(b, n, count, 0);
//	getNSortedNonCap(b, n, count, 0);
return count;
}

/*
 * Degrade second move in row with the same piece, scale by phase; maximal effect in beginning of the game
 * doesnt affect promotions, ep, rochade
 */

int gradeMoveInRow(board *b, attack_model *a, MOVESTORE square, move_entry *n, int count)
{
int c, q, sc;

int i,s,p, val, min;
	s=UnPackTo(square);
	p=UnPackProm(square);

	for(q=0;q<count;q++) {
		if((UnPackFrom(n[q].move)==s)&&(p==ER_PIECE)) {
		
			min=0;
			val=n[q].qorder;
			if((val>=A_OR)&&(val<A_OR_MAX)) min=A_OR;
			else if((val>=A_OR_N)&&(val<A_OR_N_MAX)) min=A_OR_N;
			else if((val>=A_OR2)&&(val<A_OR2_MAX)) min=A_OR2;
			else if((val>=MV_BAD)&&(val<MV_BAD_MAX)) min=MV_BAD;
			else if((val>=MV_OR)&&(val<MV_OR_MAX)) min=MV_OR;
		
			n[q].qorder=min+(val-min)*a->phase/255;
			break;
		}
	}
return count;
}

int sortMoveList_QInit(board *b, attack_model *a, int  h, move_entry *n, int count, int ply, int sort)
{
int f, c, q;
int64_t sc;
//	c=0;
//	sc=0;
//	c=count;

//	for(f=0;f<count;f++) {
//		if(n[f].qorder>=(A_OR2)&&(n[f].qorder<=(A_OR2+16*Q_OR))) continue;
//		n[c]=n[f];
//		sc+=n[c].qorder;
//		c++;
//	}

	if((h!=DRAW_M)&&(h!=NA_MOVE)) {
		for(q=0;q<count;q++) {
			if(n[q].move==h) {
				n[q].qorder+=HASH_OR;
				break;
			}
		}
	}
//	getNSorted(b, n, c, 0, sort);

return count;
}

void sprintfMoveSimple(MOVESTORE m, char *buf){
	int from, to, prom;
	char b2[100];

	switch (m&(~CHECKFLAG)) {
		case DRAW_M:
				sprintf(buf," Draw ");
				return;
		case MATE_M:
				sprintf(buf,"# ");
				return;
		case NA_MOVE:
				sprintf(buf," N/A ");
				return;
		case NULL_MOVE:
				sprintf(buf," NULL ");
				return;
		case WAS_HASH_MOVE:
				sprintf(buf," WAS_HASH ");
				return;
		case BETA_CUT:
				sprintf(buf," WAS_BETA_CUT ");
				return;
		case ALL_NODE:
				sprintf(buf," WAS_ALL_NODE ");
				return;
		case ERR_NODE:
				sprintf(buf," ERR_NODE ");
				return;
	}

	from=UnPackFrom(m&(~CHECKFLAG));
	to=UnPackTo(m);
	sprintf(buf,"%s%s", SQUARES_ASC[from], SQUARES_ASC[to]);
	prom=UnPackProm(m);
	if(prom!=ER_PIECE) {
			if(prom==QUEEN) sprintf(b2, "q");
			else if(prom==KNIGHT) sprintf(b2, "n");
			else if(prom==ROOK) sprintf(b2, "r");
			else if(prom==BISHOP) sprintf(b2, "b");
			else sprintf(b2," ");
			strcat(buf,b2);
	}
//	if(m&CHECKFLAG) {
//		sprintf(b2,"+");
//		strcat(buf,b2);
//	}
}

/*
 * SAN move d4, Qa6xb7#, fxg1=Q+,
 * rozliseni pokud vice figur stejneho typu muze na stejne misto
 * 1. pocatecni sloupec hned za oznaceni figury
 * 2. pocatecni radek hned za oznaceni figury
 * 3. pocatecni souracnice za oznaceni figury
 *
 * oznaceni P pro pesce se neuvadi
 * zapis brani pescem obsahuje pocatecni sloupec
 */


// print move, board has already been updated
void sprintfMove(board *b, MOVESTORE m, char * buf)
{
int from, to, prom, spec, cap, side, cs, cr, tt, mate;

unsigned char pto, pfrom;
char b2[512], b3[512];
int ep_add;
BITVAR aa;
		
		ep_add= b->side == WHITE ? +8 : -8;
		buf[0]='\0';
		from=UnPackFrom(m);
		to=UnPackTo(m);
		prom=UnPackProm(m);
//		spec=UnPackSpec(m);
		pfrom=b->pieces[from]&PIECEMASK;
		pto=b->pieces[to]&PIECEMASK;
		side=(b->pieces[from]&BLACKPIECE)==0 ? WHITE : BLACK;
//		check=0;
//		mate=0;
		cr=cs=-1;
		if((pfrom==PAWN) && (to==(b->ep+ep_add))) pto=PAWN;
		switch (m&(~CHECKFLAG)) {
			case DRAW_M:
					strcat(buf," Draw ");
					return;
			case MATE_M:
					strcat(buf,"# ");
					return;
			case NA_MOVE:
					strcat(buf," N/A ");
					return;
			case NULL_MOVE:
					strcat(buf," NULL ");
					return;
			case WAS_HASH_MOVE:
					strcat(buf," WAS_HASH ");
					return;
			case BETA_CUT:
					strcat(buf," WAS_BETA_CUT ");
					return;
			case ALL_NODE:
					strcat(buf," WAS_ALL_NODE ");
					return;
			case ERR_NODE:
					strcat(buf," ERR_NODE ");
					return;
		}
//to
		sprintf(b2,"%s",SQUARES_ASC[to]);
		sprintf(buf,"%s",b2);
		cap=0;
//capture
		if(pto!=ER_PIECE) {
			sprintf(b2,"x%s", buf);
			sprintf(buf, "%s", b2);
			cap=1;
		}
// who is moving?
// who is attacking this destination
			aa=0;
			switch (pfrom) {
				case BISHOP:
							aa=BishopAttacks(b, to);
							aa= aa & b->maps[pfrom];
							aa=(aa & (b->colormaps[side]));
								sprintf(b2,"B");
							break;
				case QUEEN:
							aa=QueenAttacks(b, to);
							aa= aa & b->maps[pfrom];
							aa=(aa & (b->colormaps[side]));
								sprintf(b2,"Q");
							break;
				case KNIGHT:
							aa=attack.maps[KNIGHT][to];
							aa= aa & b->maps[pfrom];
							aa=(aa & (b->colormaps[side]));
								sprintf(b2,"N");
							break;
				case ROOK:
							aa=RookAttacks(b, to);
							aa= aa & b->maps[pfrom];
							aa=(aa & (b->colormaps[side]));
								sprintf(b2,"R");
							break;
				case PAWN:
							b2[0]='\0';
							if(cap) {
//								sprintf(b2,"P");
								aa=((attack.pawn_att[WHITE][to] & b->maps[PAWN] & (b->colormaps[BLACK])) |
										(attack.pawn_att[BLACK][to] & b->maps[PAWN] & (b->colormaps[WHITE])));
//								printmask(aa,"7");
								aa&=b->colormaps[side];
//								printmask(aa,"8");
							} else {
								aa=normmark[from];
							}
							break;
				case KING:
//FIXME				
							aa=normmark[from];
								sprintf(b2,"K");
							break;
				default:
							sprintf(b2,"Unk");
							printf("ERROR unknown piece %d\n", pfrom);
			}
// provereni zdali je vic figur stejneho typu ktere mohou na cilove pole

			if(BitCount(aa)>1) {
				while(aa) {
					tt=LastOne(aa);
					ClrLO(aa);
					if(tt!=from) {
// cim lze rozlisit?
						if((from&7) == (tt&7)) cs=-1;
						if(((from>>8)&7) == ((tt>>8)&7)) cr=-1;
					}
				}
			} else if(BitCount(aa)==1) {
				cs=(from&7);
				cr=((from>>8)&7);
				if((cap==1)&&(pfrom==PAWN)) cr=-1;
			}

// buf brani+destinace, b2 figura
// cs, cr unikatnost; -1 nelze uzit samostatne
// b3 pro identifikaci source
			b3[0]='\0';

			if((cr==-1)&&(cs==-1)) {
				sprintf(b3,"%c%d", (from&7)+'a', ((from>>3)&7)+1);
			} else {
				if(cr==-1) {
					sprintf(b3,"%c", (from&7)+'a');
				} else {
					if(cs==-1) sprintf(b3,"%d", ((from>>3)&7)+1);
//					else b3[0]='\0';
				}
			}

// poskladame vystup do buf
		strcat(b2, b3);
		strcat(b2, buf);
		strcpy(buf, b2);

		if(prom==ER_PIECE) {
		} else {
			if(pfrom == KING) {
				if(from > to) sprintf(b2, "O-O-O"); else sprintf(b2, "O-O");
				sprintf(buf,"%s",b2);
			} else if(pfrom==PAWN) {
				if(prom==PAWN) {
//						strcat(buf," e.p.");
				} else {
					if(prom==QUEEN) {
						strcat(buf, "=Q");
//						i=1;
					}
					
					else if(prom==KNIGHT) strcat(buf, "=N");
					else if(prom==BISHOP) strcat(buf, "=B");
					else if(prom==ROOK) strcat(buf, "=R");
				}
			} else  {
				printf("ERROR unknown special move %d\n", pfrom);
			}
		}
		if(m&CHECKFLAG) {
			strcat(buf,"+");
		}

#if 0
		if(i==1) {
			printBoardNice(b);
		}
#endif
}

void printfMove(board *b, MOVESTORE m)
{
char buf[2048];
	sprintfMove(b, m, buf);
	LOGGER_4("I_MOV: %s\n",buf);
}

void log_divider(char *s)
{
	if(s!=NULL) { LOGGER_1("****: %s\n",s); }
	else { LOGGER_1("****:\n"); }
}

void dump_moves(board *b, move_entry * m, int count, int ply, char *cmt){
char b2[2048];
int i;

	LOGGER_0("MOV_DUMP: * Start *\n");
	if(cmt!=NULL) LOGGER_0("MOV_DUMP: Comments %s\n",cmt);
	for(i=0;i<count;i++) {
		sprintfMove(b, m->move, b2);
		LOGGER_0("MOV_DUMP: ply:%d, %d: %s %d, %d, %X\n",ply, i, b2, m->qorder, m->real_score, m->move);
		m++;
	}
	LOGGER_0("MOV_DUMP: ** END **\n");
}

void printBoardNice(board *b)
{
int f,n;
int pw,pb,nw,nb,bwl,bwd,bbl,bbd,rw,rb,qw,qb;
char buff[1024];
char x,ep[3];
char row[8];
    if(b->ep!=-1) {
    	sprintf(ep,"%c%c",b->ep%8+'A', b->ep/8+'1');
    } else ep[0]='\0';
	LOGGER_0("Move %d, Side to Move %s, e.p. %s, CastleW:%i B:%i, HashKey 0x%016llX, MIdx:%d\n",b->move/2, (b->side==0) ? "White":"Black", ep, b->castle[WHITE], b->castle[BLACK], (unsigned long long) b->key, b->mindex );
	x=' ';
	for(f=7;f>=0;f--) {
		for(n=0;n<8;n++) {
			
			switch(b->pieces[f*8+n]) {
				case ER_PIECE :	x=' ';
								break;
				case BISHOP :	x='B';
								break;
				case KNIGHT :	x='N';
								break;
				case PAWN  :	x='P';
								break;
				case QUEEN :	x='Q';
								break;
				case KING :		x='K';
								break;
				case ROOK :		x='R';
								break;
								
				case BISHOP|BLACKPIECE :	x='b';
								break;
				case KNIGHT|BLACKPIECE :	x='n';
								break;
				case PAWN|BLACKPIECE   :	x='p';
								break;
				case QUEEN|BLACKPIECE  :	x='q';
								break;
				case KING|BLACKPIECE   :	x='k';
								break;
				case ROOK|BLACKPIECE   :	x='r';
								break;
			}
			row[n]=x;
		}
//		LOGGER_0("  +-----+-----+-----+-----+-----+-----+-----+-----+\n");
		LOGGER_0("  +---+---+---+---+---+---+---+---+\n");
		LOGGER_0("%c | %c | %c | %c | %c | %c | %c | %c | %c |\n",f+'1',row[0],row[1],row[2],row[3],row[4],row[5],row[6],row[7]);
	}
//	LOGGER_0("  +-----+-----+-----+-----+-----+-----+-----+-----+\n");
	LOGGER_0("  +---+---+---+---+---+---+---+---+\n");
	LOGGER_0("    A   B   C   D   E   F   G   H  \n");
//	LOGGER_0("%s\n",buff);
	writeEPD_FEN(b, buff, 0,"");
	LOGGER_0("%s\n",buff);
	
	
//#define MATidx(pw,pb,nw,nb,bwl,bwd,bbl,bbd,rw,rb,qw,qb) (pw*PW_MI+PB_MI*pb+NW_MI*nw+NB_MI*nb+BWL_MI*bwl+BBL_MI*bbl+BWD_MI*bwd+BBD_MI*bbd+QW_MI*qw+QB_MI*qb+RW_MI*rw+RB_MI*rb)
	pw=(b->mindex%PB_MI)/PW_MI;
	pb=(b->mindex%XX_MI)/PB_MI;

	nw=(b->mindex%NB_MI)/NW_MI;
	nb=(b->mindex%BWL_MI)/NB_MI;
	bwl=(b->mindex%BBL_MI)/BWL_MI;
	bbl=(b->mindex%BWD_MI)/BBL_MI;
	bwd=(b->mindex%BBD_MI)/BWD_MI;
	bbd=(b->mindex%RW_MI)/BBD_MI;
	rw=(b->mindex%RB_MI)/RW_MI;
	rb=(b->mindex%QW_MI)/RB_MI;
	qw=(b->mindex%QB_MI)/QW_MI;
	qb=(b->mindex%PW_MI)/QB_MI;
	LOGGER_0("%d, %d, %d, %d, %d, %d\n", pw,nw,bwl,bwd,rw,qw);
	LOGGER_0("%d, %d, %d, %d, %d, %d\n", pb,nb,bbl,bbd,rb,qb);
}

void printBoardEval_PSQ(board *b, attack_model *a)
{
int f,n;
//char x;
int row_b[8], row_e[8], bx, e, from;

// PSQ table
//		a->sq[from].sqr_b=p->piecetosquare[0][s][ROOK][from];
//		a->sq[from].sqr_e=p->piecetosquare[1][s][ROOK][from];

	LOGGER_1("PieceSquare info\n");
	for(f=7;f>=0;f--) {
		for(n=0;n<8;n++) {
			from=f*8+n;
			bx=e=0;
			switch(b->pieces[from]) {
				case ER_PIECE :
//								bx=0;
//								e=0;
								break;
				case KING :		
						bx=a->sq[from].sqr_b;
						e=a->sq[from].sqr_e;
								break;
				case KING|BLACKPIECE   :
						bx=a->sq[from].sqr_b;
						e=a->sq[from].sqr_e;
								break;
				case BISHOP :
				case KNIGHT :
				case PAWN  :
				case QUEEN :
				case ROOK :
						bx=a->sq[from].sqr_b;
						e=a->sq[from].sqr_e;
								break;
				case BISHOP|BLACKPIECE :
				case KNIGHT|BLACKPIECE :
				case PAWN|BLACKPIECE   :
				case QUEEN|BLACKPIECE  :
				case ROOK|BLACKPIECE   :
						bx=a->sq[from].sqr_b;
						e=a->sq[from].sqr_e;
								break;
			}
			row_b[n]=bx;
			row_e[n]=e;
		}
		LOGGER_1("  +-----+-----+-----+-----+-----+-----+-----+-----+\n");
		LOGGER_1("%c |%5d|%5d|%5d|%5d|%5d|%5d|%5d|%5d|\n",f+'1',row_b[0],row_b[1],row_b[2],row_b[3],row_b[4],row_b[5],row_b[6],row_b[7]);
		LOGGER_1("%c |%5d|%5d|%5d|%5d|%5d|%5d|%5d|%5d|\n",f+'1',row_e[0],row_e[1],row_e[2],row_e[3],row_e[4],row_e[5],row_e[6],row_e[7]);
	}
	LOGGER_1("  +-----+-----+-----+-----+-----+-----+-----+-----+\n");
	LOGGER_1("     A     B     C     D     E     F     G     H\n");
}

void printBoardEval_MOB(board *b, attack_model *a)
{
int f,n;
//char x;
int row_b[8], row_e[8], bx, e, from;

// MOB table
//		a->me[from].pos_mob_tot_b=p->mob_val[0][s][ROOK][m];
//		a->me[from].pos_mob_tot_e=p->mob_val[1][s][ROOK][m];

	LOGGER_1("Mobility info\n");
	for(f=7;f>=0;f--) {
		for(n=0;n<8;n++) {
			from=f*8+n;
			bx=e=0;
			switch(b->pieces[from]) {
				case ER_PIECE :
								bx=0;
								e=0;
								break;
				case KING :		
						bx=a->me[from].pos_mob_tot_b;
						e=a->me[from].pos_mob_tot_e;
								break;
				case KING|BLACKPIECE   :
						bx=a->me[from].pos_mob_tot_b;
						e=a->me[from].pos_mob_tot_e;
								break;
				case BISHOP :
				case KNIGHT :
				case PAWN  :
				case QUEEN :
				case ROOK :
						bx=a->me[from].pos_mob_tot_b;
						e=a->me[from].pos_mob_tot_e;
								break;
				case BISHOP|BLACKPIECE :
				case KNIGHT|BLACKPIECE :
				case PAWN|BLACKPIECE   :
				case QUEEN|BLACKPIECE  :
				case ROOK|BLACKPIECE   :
						bx=a->me[from].pos_mob_tot_b;
						e=a->me[from].pos_mob_tot_e;
								break;
			}
			row_b[n]=bx;
			row_e[n]=e;
		}
		LOGGER_1("  +-----+-----+-----+-----+-----+-----+-----+-----+\n");
		LOGGER_1("%c |%5d|%5d|%5d|%5d|%5d|%5d|%5d|%5d|\n",f+'1',row_b[0],row_b[1],row_b[2],row_b[3],row_b[4],row_b[5],row_b[6],row_b[7]);
		LOGGER_1("%c |%5d|%5d|%5d|%5d|%5d|%5d|%5d|%5d|\n",f+'1',row_e[0],row_e[1],row_e[2],row_e[3],row_e[4],row_e[5],row_e[6],row_e[7]);
	}
	LOGGER_1("  +-----+-----+-----+-----+-----+-----+-----+-----+\n");
	LOGGER_1("     A     B     C     D     E     F     G     H\n");
	LOGGER_1("   ");
}

int printScoreExt(attack_model *a)
{

	LOGGER_1("SC: \tWhite\t\t\tBlack\n");
	LOGGER_1("SC: SQR\t%d/%d\t\t\t%d/%d\n", a->sc.side[0].sqr_b,a->sc.side[0].sqr_e, a->sc.side[1].sqr_b,a->sc.side[1].sqr_e);
	LOGGER_1("SC: MOB\t%d/%d\t\t%d/%d\n", a->sc.side[0].mobi_b,a->sc.side[0].mobi_e, a->sc.side[1].mobi_b,a->sc.side[1].mobi_e);
	LOGGER_1("SC: \tPhase\tMaterial\tScore\n");
	LOGGER_1("SC: \t%d\t%d\t\t%d\n", a->phase, a->sc.material, a->sc.complete);
	return 0;
}

int compareBoardSilent(board *source, board *dest){
int i, ret;

	ret=0;
	if(dest->key!=source->key) { ret=1; goto konec; }
	if(dest->norm!=source->norm) { ret=2; goto konec; }
	if(dest->r45L!=source->r45L) { ret=1; goto konec; }
	if(dest->r45R!=source->r45R) { ret=4; goto konec; }
	if(dest->r90R!=source->r90R) { ret=5; goto konec; }
	if(dest->rule50move!=source->rule50move) { ret=6; goto konec; }
	if(dest->side!=source->side) { ret=7; goto konec; }
	if(dest->ep!=source->ep) { ret=8; goto konec; }
	for(i=WHITE;i<ER_SIDE;i++) if(dest->castle[i]!=source->castle[i]) { ret=9; goto konec; }
	for(i=0;i<6;i++) if(dest->maps[i]!=source->maps[i]) { ret=10; goto konec; }
	for(i=0;i<2;i++) if(dest->colormaps[i]!=source->colormaps[i]) { ret=11; goto konec; }
	for(i=0;i<ER_PIECE;i++) if(dest->material[WHITE][i]!=source->material[WHITE][i]) { ret=12; goto konec; }
	for(i=0;i<ER_PIECE;i++) if(dest->material[BLACK][i]!=source->material[BLACK][i]) { ret=13; goto konec; }
	for(i=0;i<64;i++) if(dest->pieces[i]!=source->pieces[i]) { ret=14; goto konec; }
	if(dest->mindex!=source->mindex)  { ret=15; goto konec; }
	if(dest->mindex2!=source->mindex2)  { ret=16; goto konec; }

konec:
	if(ret!=0) {
		printf("XXX");
	}
return ret;
}

int copyStats(struct _statistics *source, struct _statistics *dest){
	*dest=*source;
return 0;
}

int copyBoard(board *source, board *dest){

#if 1
	memcpy(dest, source, sizeof(board));
	copyStats(source->stats, dest->stats);
//	copyPers(source->pers,dest->pers);
	dest->pers=source->pers;

#else
int i;
	*dest=*source;

	for(i=0;i<6;i++) dest->maps[i]=source->maps[i];
	for(i=0;i<2;i++) dest->colormaps[i]=source->colormaps[i];
	dest->norm=source->norm;
	dest->r90R=source->r90R;
	dest->r45L=source->r45L;
	dest->r45R=source->r45R;
	for(i=0;i<64;i++) dest->pieces[i]=source->pieces[i];
	for(i=0;i<ER_PIECE;i++) dest->material[WHITE][i]=source->material[WHITE][i];
	for(i=0;i<ER_PIECE;i++) dest->material[BLACK][i]=source->material[BLACK][i];
	dest->mindex=source->mindex;
	dest->ep=source->ep;
	dest->side=source->side;
	for(i=WHITE;i<ER_SIDE;i++) dest->castle[i]=source->castle[i];
	for(i=0;i<2;i++) dest->king[i]=source->king[i];
	dest->move=source->move;
	dest->rule50move=source->rule50move;
	dest->key=source->key;
	for(i=0;i<102;i++) dest->positions[i]=source->positions[i];
	for(i=0;i<102;i++) dest->posnorm[i]=source->posnorm[i];
	dest->gamestage=source->gamestage;

	dest->time_start=source->time_start;
	dest->nodes_mask=source->nodes_mask;
	dest->time_move=source->time_move;
	dest->time_crit=source->time_crit;
	
	copyStats(&source->stats, &dest->stats);
//	struct _ui_opt uci_options;

	dest->bestmove=source->bestmove;
	dest->bestscore=source->bestscore;
	
	copyPers(source->pers,dest->pers);
#endif

return 0;
}


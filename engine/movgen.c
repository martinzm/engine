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
#include "search.h"
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#define MOVE_TEST_SETUP BITVAR mv2=mv
#define MOVE_TEST(x) if(*(move-1)==x) { printf("Move from:%d to:%d triggered file:%s, line:%d\n", from, to, __FILE__, __LINE__ );printmask(mv2,"rook"); printboard(b);  dumpit(b, from); }

BITVAR isInCheck_Eval(board *b, attack_model *a, int side)
{
	return a->ke[side].attackers;
}

// pv > hash > winning/prom > Qprom > neutral > killer > castling > minor prom > non-cap (history heur) > losing
 //declare quiet move purely based on previous qorder assignment
int is_quiet_move(board *b, attack_model *a, move_entry *m){
int to;

	to=UnPackTo(m->move);
	if(b->pieces[to]==ER_PIECE) return 1;
 return 0;
}

/*
 * Serialize moves from bitmaps, capture type of moves available at board for side
 */
 
void generateCapturesN(board * b, const attack_model *a, move_entry ** m, int gen_u)
{
int from, to;
BITVAR mv, rank, piece, epbmp;
move_entry * move;
int ep_add;
unsigned char side, opside;



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

// generate queens
		piece=b->maps[QUEEN]&(b->colormaps[side]);
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

// generate rooks 
		piece=b->maps[ROOK]&(b->colormaps[side]);
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

// bishops
		piece=b->maps[BISHOP]&(b->colormaps[side]);
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
// knights
		piece=b->maps[KNIGHT]&(b->colormaps[side]);
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

// pawn promotions
		piece = (b->maps[PAWN]) & (b->colormaps[side]) & rank;
		while (piece) {
			from = LastOne(piece);
			mv=a->mvs[from] & (~b->norm);
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
			ClrLO(piece);
	}

// pawn promotions after capture/attack
		piece = (b->maps[PAWN]) & (b->colormaps[side]) & rank;
		while (piece){
			from = LastOne(piece);
			mv=a->mvs[from] & (b->colormaps[opside]);
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
			ClrLO(piece);
		}

	if(b->ep!=-1) epbmp=attack.ep_mask[b->ep]&b->maps[PAWN]&b->colormaps[side];
	else epbmp=0;
	
// pawn attacks
		piece = (b->maps[PAWN]) & (b->colormaps[side]) & (~rank) & (~epbmp);
		while (piece) {
			from = LastOne(piece);
			mv = a->mvs[from] & (b->colormaps[opside]);
			while (mv) {
				to = LastOne(mv);
				move->move = PackMove(from, to,  ER_PIECE, 0);
				move->qorder=move->real_score=b->pers->LVAcap[PAWN][b->pieces[to]&PIECEMASK];
				move++;
				ClrLO(mv);
			}
			ClrLO(piece);
		}

// pawn attacks
		piece = (b->maps[PAWN]) & (b->colormaps[side]) & (~rank)&(epbmp);
		while (piece) {
			from = LastOne(piece);
			mv = a->mvs[from] & (b->colormaps[opside]);
			while (mv) {
				to = LastOne(mv);
				if((to==b->ep)) {
					move->move = PackMove(from, to+ep_add, PAWN, 0);
					move->qorder=move->real_score=b->pers->LVAcap[PAWN][PAWN];
				} else {
					move->move = PackMove(from, to,  ER_PIECE, 0);
					move->qorder=move->real_score=b->pers->LVAcap[PAWN][b->pieces[to]&PIECEMASK];
				}
				move++;
				ClrLO(mv);
			}
			ClrLO(piece);
		}

// king 
	from = b->king[side];
	mv = (a->mvs[from] & (b->colormaps[opside]));
	while (mv) {
		to = LastOne(mv);
		move->move = PackMove(from, to,  ER_PIECE, 0);
		move->qorder=move->real_score=b->pers->LVAcap[KING][b->pieces[to]&PIECEMASK];
		move++;
	ClrLO(mv);
	}
	*m=move;
}

#define GETMOVES(MAP, FUNC) x=MAP;while (x){from=LastOne(x);a->mvs[from] = (FUNC(b, from));ClrLO(x);}
#define GETMOVEK(MAP, PIECE) x=MAP;while (x){from=LastOne(x);a->mvs[from] = (attack.maps[PIECE][from]);ClrLO(x);}
#define GETMOVESP(MAP, FUNC) x=MAP;while (x){from=LastOne(x);a->mvs[from] = (FUNC(b, from) & attack.rays_dir[b->king[side]][from] );ClrLO(x);}
#define GETMOVEKP(MAP, PIECE) x=MAP;while (x){from=LastOne(x);a->mvs[from] = (attack.maps[PIECE][from]) & attack.rays_dir[b->king[side]][from] ;ClrLO(x);}

#define GETMOVESx(MAP, FUNC) x=MAP;while (x){from=LastOne(x);a->mvs[from] = (FUNC(b, from)&(~b->colormaps[side]));ClrLO(x);}
#define GETMOVEKx(MAP, PIECE) x=MAP;while (x){from=LastOne(x);a->mvs[from] = (attack.maps[PIECE][from]&(~b->colormaps[side]));ClrLO(x);}
#define GETMOVESPx(MAP, FUNC) x=MAP;while (x){from=LastOne(x);a->mvs[from] = (FUNC(b, from) & attack.rays_dir[b->king[side]][from]&(~b->colormaps[side]) );ClrLO(x);}
#define GETMOVEKPx(MAP, PIECE) x=MAP;while (x){from=LastOne(x);a->mvs[from] = (attack.maps[PIECE][from]) & attack.rays_dir[b->king[side]][from]&(~b->colormaps[side]) ;ClrLO(x);}

#define GETMOVE2(MAP, FUNC) x=MAP;\
  while (x){ \
  from=LastOne(x); \
  kpin = (normmark[from]&pins) ? attack.rays_dir[b->king[side]][from] : FULLBITMAP;\
  a->mvs[from] = (FUNC(b, from)&(~b->colormaps[side])&kpin);\
  ClrLO(x); }

/*
 * Generates moves bitmaps for all types of moves available at board for side
 * excludes moves to own pieces (ie protection)!
 */

#define GETMOVESu(MAP, FUNC, BMP, POS, DRV) x=MAP;\
  DRV=-1;\
  while (x){\
	DRV++;\
	POS[DRV]=LastOne(x);\
	BMP[DRV] = (FUNC(b, from));\
	ClrLO(x);\
  }

#define xT2(P,S) t2[P]=b->maps[P]&b->colormaps[S];
#define xPI(P) pi[P]=t2[P]&pins;
#define xNP(P) np[P]=pi[P]^t2[P];

typedef struct _run_in {
	int st;
	int en;
	int add;
	int opside;
	int orank;
} run_in;

run_in RR[] = {
		{ ER_PIECE, PAWN, 0, BLACK, 0},
		{ ER_PIECE|BLACKPIECE, PAWN|BLACKPIECE, BLACKPIECE, WHITE, 56}
};

int simple_pre_movegen_n2(board const *b, attack_model *a, int side)
{
int from, epn, opside, orank;
BITVAR x, pins, epbmp, tmp, kpin, nmf, t2[ER_PIECE];
BITVAR np[ER_PIECE+1];
BITVAR pi[ER_PIECE+1];
	if(side==BLACK) {
		opside=WHITE;
		orank=56;
	} else {
		opside=BLACK;
		orank=0;
	}

	epbmp= (b->ep!=-1 && (a->ke[side].ep_block==0)) ? attack.ep_mask[b->ep]&b->maps[PAWN]&b->colormaps[side] : 0;
	pins=((a->ke[side].cr_pins | a->ke[side].di_pins));

	xT2(PAWN, side)
	xT2(KNIGHT, side)
	xT2(BISHOP, side)
	xT2(ROOK, side)
	xT2(QUEEN, side)
	xT2(KING, side)
	xPI(PAWN)
	xPI(KNIGHT)
	xPI(BISHOP)
	xPI(ROOK)
	xPI(QUEEN)
	xPI(KING)
	xNP(PAWN)
	xNP(KNIGHT)
	xNP(BISHOP)
	xNP(ROOK)
	xNP(QUEEN)
	xNP(KING)

	GETMOVES(np[QUEEN], QueenAttacks); //!!
	GETMOVES(np[ROOK], RookAttacks); //!!
	GETMOVES(np[BISHOP], BishopAttacks);
	GETMOVEK(np[KNIGHT], KNIGHT);

	GETMOVESP(pi[QUEEN], QueenAttacks); //!!
	GETMOVESP(pi[ROOK], RookAttacks); //!!
	GETMOVESP(pi[BISHOP], BishopAttacks);
	GETMOVEKP(pi[KNIGHT], KNIGHT);

	x=np[PAWN]&b->colormaps[side];
	switch(side) {
		case WHITE:
			while (x){
				from=LastOne(x);
				nmf=normmark[from];
				a->mvs[from] = attack.pawn_att[side][from]&b->norm;
				tmp= (nmf<<8)&(~b->norm);
				if(tmp && (nmf&RANK2)) tmp|= (tmp<<8)&(~b->norm); //!!
				a->mvs[from]|=tmp;
				ClrLO(x);
			}
			
			break;
		case BLACK:
			while (x){
				from=LastOne(x);
				nmf=normmark[from];
				a->mvs[from] = attack.pawn_att[side][from]&b->norm;
				tmp= (nmf>>8)&(~b->norm);
				if(tmp && (nmf&RANK7)) tmp|= (tmp>>8)&(~b->norm);
				a->mvs[from]|=tmp;
				ClrLO(x);
			}
			break;
		default:
		  break;
	}

	x=pi[PAWN]&b->colormaps[side];
	switch(side) { //!!
		case WHITE:
			while (x){
				from=LastOne(x);
				nmf=normmark[from];
				kpin = attack.rays_dir[b->king[side]][from];
				a->mvs[from] = attack.pawn_att[side][from]&(b->norm) & kpin;
				tmp= (nmf<<8)&(~b->norm) & kpin;
				if(tmp && (nmf&RANK2)) tmp|= (tmp<<8)&(~b->norm);
				a->mvs[from]|=tmp;
				ClrLO(x);
			}
			break;
		case BLACK:
			while (x){
				from=LastOne(x);
				nmf=normmark[from];
				kpin = attack.rays_dir[b->king[side]][from];
				a->mvs[from] = attack.pawn_att[side][from]&(b->norm) & kpin;
				tmp= (nmf>>8)&(~b->norm) & kpin;
				if(tmp && (nmf&RANK7)) tmp|= (tmp>>8)&(~b->norm);
				a->mvs[from]|=tmp;
				ClrLO(x);
			}
		  break;
		default:
		  break;
	}

// ep
	x=b->maps[PAWN]&epbmp&b->colormaps[side];
	while(x) {
		epn = side==WHITE ? 1:-1;
		from=LastOne(x);
		nmf=normmark[from];
		kpin = (nmf&pins) ? attack.rays_dir[b->king[side]][from] : FULLBITMAP;
		if(normmark[getPos(getFile(b->ep), getRank(b->ep)+epn)] & kpin) a->mvs[from] |= normmark[b->ep];
		ClrLO(x);
	}

// king 
// !!!!! att_by_side - opside !!!!!
	from = b->king[side];
	a->mvs[from] = (attack.maps[KING][from]) & (~attack.maps[KING][b->king[opside]])&(~a->att_by_side[opside]);
/*
 * Incorporate castling
 */
	if(b->castle[side] & QUEENSIDE) {
		if((attack.rays[C1+orank][E1+orank]&((a->att_by_side[opside]|attack.maps[KING][b->king[opside]])))==0
		  && ((attack.rays[B1+orank][D1+orank]&b->norm)==0)) a->mvs[from]|=normmark[C1+orank];
	}
	if(b->castle[side] & KINGSIDE) {
		if((attack.rays[E1+orank][G1+orank]&(a->att_by_side[opside]|attack.maps[KING][b->king[opside]]))==0 
		  && ((attack.rays[F1+orank][G1+orank]&b->norm)==0)) a->mvs[from]|=normmark[G1+orank];
	}

return 0;
}

/*
 * requires check evaluation
 * pins cannot move
 * king can move to unattacked place
 * if only one attacker
 *	it can be captured with king or nonPIN piece
 *	nonPIN piece can stand into ray from attacker to king and become PINned
 */

/*
 * Generates moves bitmaps for inCheck types of moves available at board for side
 */

int simple_pre_movegen_n2check(board *b, attack_model *a, int side)
{
int f, from, opside, attacker;
BITVAR x, pins, epbmp, tmp, attack_ray, tmp3;

BITVAR np[ER_PIECE+1];

	if(side==BLACK) {
		opside=WHITE;
	} else {
		opside=BLACK;
	}
	epbmp= (b->ep!=-1 && (a->ke[side].ep_block==0)) ? attack.ep_mask[b->ep]&b->maps[PAWN]&b->colormaps[side] : 0;
	attacker=0;
	pins=((a->ke[side].cr_pins | a->ke[side].di_pins));
	
	if(BitCount(a->ke[side].attackers)==1) {
		attacker=LastOne(a->ke[side].attackers);
		attack_ray=(attack.rays_int[b->king[side]][attacker]|normmark[attacker]);
		tmp3=0;
		for(f=0;f<ER_PIECE;f++) {
			np[f]=b->maps[f]&b->colormaps[side]&(~pins);
			tmp3|=np[f];
		}
		tmp3&=~normmark[b->king[side]];
		GETMOVES(np[ROOK], RookAttacks);
		GETMOVES(np[BISHOP], BishopAttacks);
		GETMOVES(np[QUEEN], QueenAttacks);
		GETMOVEK(np[KNIGHT], KNIGHT);

// pawn 
		x=np[PAWN];
		while (x){
			from=LastOne(x);
			a->mvs[from] = attack.pawn_att[side][from]&(b->colormaps[opside]);
			if (side==WHITE) {
				tmp= (normmark[from]<<8)&(~b->norm);
				if(tmp && (normmark[from]&RANK2))
					tmp|= (tmp<<8)&(~b->norm);
			} else {
				tmp= (normmark[from]>>8)&(~b->norm);
				if(tmp && (normmark[from]&RANK7)) 
					tmp|= (tmp>>8)&(~b->norm);
			}
			tmp&=(~b->colormaps[side]);
			a->mvs[from]|=tmp;
			ClrLO(x);
		}

// ep
		x=np[PAWN]&epbmp;
		while(x) {
			from=LastOne(x);
			a->mvs[from] |= normmark[b->ep];
			ClrLO(x);
		}
		x=tmp3;
		while(x) {
			from=LastOne(x);
			a->mvs[from] &= attack_ray;
			ClrLO(x);
		}
	} else for(from=0;from<64;from++) a->mvs[from]=0;
// king 
	from = b->king[side];
	a->mvs[from] = (attack.maps[KING][from]) & (~attack.maps[KING][b->king[opside]])
	  &(~a->att_by_side[opside])&(~a->ke[side].cr_att_ray) & (~a->ke[side].di_att_ray) & (~b->colormaps[side]);
	  
return 0;
}

/*
 * Serialize moves from bitmaps for all types of moves available at board for side
 */

void generateMovesN(board * b, const attack_model *a, move_entry ** m)
{
int from, to;
BITVAR mv, rank, brank, piece, bran2;
move_entry * move;
int orank, ff;
unsigned char side;


		move = *m;
		if(b->side == WHITE) {
			rank=RANK7;
			side=WHITE;
			brank=RANK2;
			bran2=RANK4;
			orank=0;
			ff=8;
		}
		else {
			rank=RANK2;
			side=BLACK;
			brank=RANK7;
			bran2=RANK5;
			orank=56;
			ff=-8;
		}

// knights
		piece=b->maps[KNIGHT]&(b->colormaps[side]);
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

// bishops	
		piece=b->maps[BISHOP]&(b->colormaps[side]);
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
// generate rooks
		piece=b->maps[ROOK]&(b->colormaps[side]);
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
// generate queens
		piece=b->maps[QUEEN]&(b->colormaps[side]);
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

// pawn moves
		piece = (b->maps[PAWN]) & (b->colormaps[side]) & (~rank) & (~brank);
		while (piece) {
			from = LastOne(piece);
			mv=a->mvs[from]&(~b->norm);
			while (mv) {
				to = LastOne(mv);
				move->move = PackMove(from, to,  ER_PIECE, 0);
				move->qorder=move->real_score=MV_OR+P_OR;
				move++;
				ClrLO(mv);
			}
			ClrLO(piece);
		}
		piece = (b->maps[PAWN]) & (b->colormaps[side]) & (~rank) & (brank);
		while (piece) {
			from = LastOne(piece);
			mv=a->mvs[from]&(~b->norm);
			while (mv) {
				to = LastOne(mv);
				// doublepush has to be recognised
				if((normmark[to]&bran2)) {
					move->move = PackMove(from, to,  ER_PIECE+1, 0);
					move->qorder=move->real_score=MV_OR+P_OR+1;
				} else {
					move->move = PackMove(from, to,  ER_PIECE, 0);
					move->qorder=move->real_score=MV_OR+P_OR;
				}
				move++;
				ClrLO(mv);
			}
			ClrLO(piece);
		}

// king
			from = b->king[side];
			mv = (a->mvs[from]) & (~b->norm);
			while (mv) {
				to = LastOne(mv);
				ff = getFile(to);
// must mark castling move as such. Identify castling moves
				if(((b->castle[side] & QUEENSIDE)&&(ff==C1))) {
					move->move = PackMove(E1+orank,C1+orank, KING, 0);
					move->qorder=move->real_score=CS_Q_OR;
				} else if(((b->castle[side] & KINGSIDE)&&(ff==G1))) {
					move->move = PackMove(E1+orank,G1+orank, KING, 0);
					move->qorder=move->real_score=CS_K_OR;
				} else {
					move->move = PackMove(from, to,  ER_PIECE, 0);
					move->qorder=move->real_score=MV_OR;
				}
				move++;
				ClrLO(mv);
			}
	*m=move;
}

/*
 * tahy ktere vedou na policka, ktera jsou od nepratelskeho krale - krome pinned
 * tahy pinned ktere vedou na ^^ policka a neodkryvaji vlastniho krale
 * tahy figurami, ktere blokuji utok na nepratelskeho krale
 * taky je mozno tahnout vlastnim figurami, ktere blokuji utok na nepratelskeho krale
 */

/*
 * Serialize moves from bitmaps, for quiet/NON capture checking types of moves available at board for side
 */

void generateQuietCheckMovesN(board * b, const attack_model *a, move_entry ** m)
{
int from, to;
BITVAR mv, rank, brank, pins, piece, bran2;
move_entry * move;

unsigned char side, opside;
king_eval kee, *ke;


		move = *m;
		if(b->side == WHITE) {
			rank=RANK7;
			side=WHITE;
			opside=BLACK;
			brank=RANK2;
			bran2=RANK4;
		}
		else {
			rank=RANK2;
			opside=WHITE;
			side=BLACK;
			brank=RANK7;
			bran2=RANK5;
		}

		pins=((a->ke[side].cr_pins | a->ke[side].di_pins));
		ke=&kee;
		eval_ind_attacks(b, ke, NULL, opside, b->king[opside]);

		piece=b->maps[QUEEN]&(b->colormaps[side]);
		while(piece) {
			from = LastOne(piece);
// insert blocker array attacks
			mv=a->mvs[from]& ((ke->cr_blocker_ray)|(ke->di_blocker_ray));
// insert move away moves
			if(piece & (ke->di_blocks|ke->cr_blocks))
				mv|=(a->mvs[from] & (~attack.rays_dir[b->king[opside]][from]));
			mv&=(~b->norm);
			while (mv) {
				to = LastOne(mv);
				move->move = PackMove(from, to,  ER_PIECE, 0);
				move->qorder=move->real_score=MV_OR+Q_OR;
				move++;
				ClrLO(mv);
			}
			ClrLO(piece);
		}

		piece=b->maps[ROOK]&(b->colormaps[side]);
		while(piece) {
			from = LastOne(piece);
			mv=a->mvs[from]& (ke->cr_blocker_ray);
			if(piece & ke->cr_blocks)
				mv|=(a->mvs[from] & (~attack.rays_dir[b->king[opside]][from]));
			mv&=(~b->norm);
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
		piece=b->maps[BISHOP]&(b->colormaps[side]);
		while(piece) {
			from = LastOne(piece);
			mv=a->mvs[from]& (ke->di_blocker_ray);
			if(piece & ke->di_blocks)
				mv|=(a->mvs[from] & (~attack.rays_dir[b->king[opside]][from]));
			mv&=(~b->norm);
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
		piece=b->maps[KNIGHT]&(b->colormaps[side]) & (~pins);
		while(piece) {
			from = LastOne(piece);
			mv=a->mvs[from]& (ke->kn_pot_att_pos);
			if(piece & (ke->di_blocks|ke->cr_blocks))
				mv|=(a->mvs[from]);
			mv&=(~b->norm);
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
		piece = (b->maps[PAWN]) & (b->colormaps[side]) & (~rank) &(~brank);
		while (piece) {
			from = LastOne(piece);
			mv=a->mvs[from]& (ke->pn_pot_att_pos);
			if(piece & (ke->di_blocks|ke->cr_blocks))
				mv|=(a->mvs[from] & (~attack.rays_dir[b->king[opside]][from]));
			mv&=(~b->norm);
			while (mv) {
				to = LastOne(mv);
				move->move = PackMove(from, to,  ER_PIECE, 0);
				move->qorder=move->real_score=MV_OR+P_OR;
				move++;
				ClrLO(mv);
			}
			ClrLO(piece);
		}

		piece = (b->maps[PAWN]) & (b->colormaps[side]) & (~rank) &(brank);
		while (piece) {
			from = LastOne(piece);
			mv=a->mvs[from]& (ke->pn_pot_att_pos);
			if(piece & (ke->di_blocks|ke->cr_blocks)) 
				mv|=(a->mvs[from] & (~attack.rays_dir[b->king[opside]][from]));
			mv&=(~b->norm);
			while (mv) {
				to = LastOne(mv);
			// doublepush has to be recognised
				if((normmark[to]&bran2)) {
					move->move = PackMove(from, to,  ER_PIECE+1, 0);
					move->qorder=move->real_score=MV_OR+P_OR+1;
				} else {
					move->move = PackMove(from, to,  ER_PIECE, 0);
					move->qorder=move->real_score=MV_OR+P_OR;
				}
				move++;
				ClrLO(mv);
			}
			ClrLO(piece);
		}

// king
			from = b->king[side];
			if(normmark[from] & (ke->di_blocks|ke->cr_blocks)) {
				mv=(a->mvs[from] & (~attack.rays_dir[b->king[opside]][from]));
				mv&=(~b->norm);
				while (mv) {
				to = LastOne(mv);
					move->move = PackMove(from, to,  ER_PIECE, 0);
					move->qorder=move->real_score=MV_OR;
					move++;
					ClrLO(mv);
				}
			}
	*m=move;
}



/*
 * Check if move can be valid
 * if at source is our side piece, if at destination is empty or other side piece
 * if there is free path between source and dest (sliders)
 *
 * requires eval_king_checks for pins
 */

int isMoveValid(board *b, MOVESTORE move, const attack_model *a, int side, tree_store *tree)
{
int from, to, prom, movp, opside, pside, tot, prank, pfile;
BITVAR bfrom, bto, m, path, path2, npins;

king_eval kee;



	from=UnPackFrom(move);
	movp=b->pieces[from];
	if((movp&PIECEMASK)==ER_PIECE) return 0;
	bfrom=normmark[from];
	if(!(bfrom & b->colormaps[side])) return 0;
	to=UnPackTo(move);
	if(from==to) return 0;
	prom=UnPackProm(move);
	bto=normmark[to];
	if((bto & b->colormaps[side])) return 0;
	if(bto & b->maps[KING]) return 0;
	if(side==BLACK) {
	  pside=BLACKPIECE;
	  opside=WHITE;
	  prank=7;
	} else {
	  pside=0;
	  opside=BLACK;
	  prank=0;
	}
	// handle special moves
	switch (prom) {
	  case KING:
// castling
			if((movp!=(KING&PIECEMASK))||(from != getPos(E1,prank))) return 0;
			if((getPos(C1,prank))==to) { if(!(b->castle[side]&QUEENSIDE)) return 0; 
				else {
				  path=attack.rays_int[from][getPos(A1,prank)];
				  path2=attack.rays[from][getPos(C1,prank)];
				}
			}
			else if((getPos(G1, prank))==to) { if(!(b->castle[side]&KINGSIDE)) return 0; 
				else {
				  path=attack.rays_int[from][getPos(H1,prank)];
				  path2=attack.rays[from][getPos(G1,prank)];
				}
			}
			else return 0;
			if(path&b->norm) return 0;
			if(path2&(a->att_by_side[opside] | a->att_by_side[opside]|attack.maps[KING][b->king[opside]])) return 0;
			return 1;
	  case PAWN:
// ep
			if(movp!=(PAWN|pside)) return 0;
			if(b->ep==-1) return 0;
			tot= side==WHITE ? getPos(getFile(b->ep), getRank(b->ep)+1) : getPos(getFile(b->ep), getRank(b->ep)-1);
			if(tot!=to) return 0;
			if((!(normmark[b->ep]&b->maps[PAWN]))||(!(normmark[b->ep]&b->colormaps[opside]))||(b->norm & normmark[to])) return 0;
			if(a->ke[side].ep_block&bfrom) return 0;
			npins=((a->ke[side].cr_pins | a->ke[side].di_pins)&bfrom);
			if(npins) if(!(attack.rays_dir[b->king[side]][from] & bto)) return 0;
			return 1;
			break;
		
	  case ER_PIECE+1:
// doublepush
			pfile=getFile(from);
			prank= side==WHITE ? getRank(from)+2 : getRank(from)-2;
			tot=getPos(pfile, prank);
			if(tot!=to) return 0;
			path=attack.rays[from][to]&(~bfrom);
			if(path&b->norm) return 0;
//			return 1;
			break;
	  case ER_PIECE:
// ordinary movement
			tot=getRank(to);
			if((movp==(PAWN|pside))&&(((side==WHITE)&&(tot==7))||((side==BLACK)&&(tot==0)))) return 0;
			break;
// pawn promotion ie for prom == KNIGHT, BISHOP, ROOK, QUEEN
	  default:
			if(movp!=(PAWN|pside)) return 0;
			tot=getRank(to);
			if(((side==WHITE)&&(tot!=7))||((side==BLACK)&&(tot!=0))) return 0;
			break;
	}
	m=0;
	
	switch (movp&PIECEMASK) {
	  case BISHOP:
			path=attack.rays[from][to];
			m=attack.maps[BISHOP][from];
			break;
	  case QUEEN:
			path=attack.rays[from][to];
			m=attack.maps[BISHOP][from]|attack.maps[ROOK][from];
			break;
	  case ROOK:
			path=attack.rays[from][to];
			m=attack.maps[ROOK][from];
			break;
	  case KING:
			m=attack.maps[KING][from];
			path=attack.rays[from][to];
			eval_king_checks_oth(b, &kee, NULL, side, to);
			if(((kee.attackers)&(~bto))!=0) return 0;
			m &= ~attack.maps[KING][b->king[opside]];
			break;
	  case KNIGHT:
			m=attack.maps[KNIGHT][from];
			path=0;
			break;
	  case PAWN:
			m=attack.pawn_move[side][from]&(~b->norm);
			m|=(attack.pawn_att[side][from]&(b->colormaps[opside]));
			path=attack.rays[from][to];
			break;
	  default:
			return 0;
			break;
	}
	if(!(m & bto)) return 0;
	
	if(path & (~(bfrom|bto))&b->norm) return 0;
// handle pins
	npins=((a->ke[side].cr_pins | a->ke[side].di_pins)&bfrom);
	if(npins) {
		m &= attack.rays_dir[b->king[side]][from];
		if(!(m & bto)) return 0;
	}

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
int8_t siderooks, opsiderooks, kingbase;
int8_t oldp, movp, capp;
int * tmidx;
int * omidx;

int midx;
int sidx, oidx;
int rookf, rookt;
personality *p;
int vcheck=0;

	if(b->side==WHITE) {
			opside=BLACK;
			siderooks=A1;
			opsiderooks=A8;
			kingbase=E1;
			tmidx = MATIdxIncW;
			omidx = MATIdxIncB;
			sidx=1;
			oidx=-1;
		} 
		else {
			opside=WHITE;
			siderooks=A8;
			opsiderooks=A1;
			kingbase=E8;
			tmidx = MATIdxIncB;
			omidx = MATIdxIncW;
			sidx=-1;
			oidx=1;
		}

		ret.move=move;
		ret.side=b->side;
		ret.castle[WHITE]=b->castle[WHITE];
		ret.castle[BLACK]=b->castle[BLACK];
		ret.rule50move=b->rule50move;
		ret.ep=b->ep;
		ret.key=b->key;
		ret.pawnkey=b->pawnkey;
		ret.mindex_validity=b->mindex_validity;
		ret.psq_b=b->psq_b;
		ret.psq_e=b->psq_e;
		
		p=b->pers;
		
		from=UnPackFrom(move);
		to=UnPackTo(move);
		prom=UnPackProm(move);
		capp=ret.captured=b->pieces[to]&PIECEMASK;
		movp=oldp=ret.old=ret.moved=b->pieces[from]&PIECEMASK;

DEB_4(
		if(movp==PAWN) {
			to_f=getRank(to);
			if(((to_f==0)||(to_f==7))&&((prom<KNIGHT)||(prom>QUEEN))) {
				printBoardNice(b);
				sprintfMoveSimple(move, b2);
				LOGGER_0("XX failed move %s, prom: %d\n",b2, prom);
				ret.move=NA_MOVE;
				assert(0);
				return ret;
			}
		}
		if((capp==KING)) {
			printBoardNice(b);
			sprintfMoveSimple(move, b2);
			LOGGER_0("failed move %s\n",b2);
			printboard(b);
			ret.move=NA_MOVE;
			return ret;
		}
)

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
				b->rule50move=b->move;
				midx=omidx[capp];

// psq is not side relative
// so white is positive, black negative

				b->psq_b-=(oidx*p->piecetosquare[0][opside][capp][to]);
				b->psq_e-=(oidx*p->piecetosquare[1][opside][capp][to]);
				
// fix for dark bishop
				if(capp==BISHOP) {
					if(normmark[to] & BLACKBITMAP) {
						midx=omidx[DBISHOP];
					}
				}
				else if(capp==PAWN) {
					b->pawnkey^=randomTable[opside][to][PAWN]; //pawnhash
				}
				b->mindex-=midx;
				b->key^=randomTable[opside][to][capp];

				if ((to==opsiderooks) && (capp==ROOK)&&(b->castle[opside]!=NOCASTLE)){
/* remove castling opside */
					b->castle[opside] &=(~QUEENSIDE);
					if(b->castle[opside]!=ret.castle[opside])
						b->key^=castleKey[opside][QUEENSIDE];
				}
				else if ((to==(opsiderooks+7)) && (capp==ROOK)&&(b->castle[opside]!=NOCASTLE)) {
					b->castle[opside] &=(~KINGSIDE);
					if(b->castle[opside]!=ret.castle[opside])
						b->key^=castleKey[opside][KINGSIDE];
				}
				if(b->mindex_validity==0) vcheck=1;
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
			if((oldp==KING)) {
				b->king[b->side]=to;
				if((from==kingbase)&&(b->castle[b->side]!=NOCASTLE)){
					b->castle[b->side]=NOCASTLE;
					b->key^=castleKey[b->side][ret.castle[b->side]];
				}
			}
// move side screwed castle ?
// 	was the move from my corners ?
			if((from==siderooks) && (oldp==ROOK)&&(b->castle[b->side]!=NOCASTLE)) {
				b->castle[b->side] &=(~QUEENSIDE);
				if(b->castle[b->side]!=ret.castle[b->side])
					b->key^=castleKey[b->side][QUEENSIDE];
			} else if ((from==(siderooks+7))&& (oldp==ROOK)&&(b->castle[b->side]!=NOCASTLE)) {
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
				rookf=from+3;
				rookt=to-1;
			} else {
				rookf=from-4;
				rookt=to+1;
			}
// update rook movement
			MoveFromTo(rookf, rookt, b->side, ROOK, b);
			b->key^=randomTable[b->side][rookf][ROOK]; //hash
			b->key^=randomTable[b->side][rookt][ROOK]; //hash

			b->psq_b-=(sidx*p->piecetosquare[0][b->side][ROOK][rookf]);
			b->psq_e-=(sidx*p->piecetosquare[1][b->side][ROOK][rookf]);
			b->psq_b+=(sidx*p->piecetosquare[0][b->side][ROOK][rookt]);
			b->psq_e+=(sidx*p->piecetosquare[1][b->side][ROOK][rookt]);

			break;
		case PAWN:
// EP
			ClearAll(ret.ep, opside, PAWN , b);
			b->mindex-=omidx[PAWN];

// update pawn captured
			b->psq_b-=(oidx*p->piecetosquare[0][opside][PAWN][ret.ep]);
			b->psq_e-=(oidx*p->piecetosquare[1][opside][PAWN][ret.ep]);

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
				ClearAll(to, opside, capp, b);
// remove captured piece
				b->psq_b-=(oidx*p->piecetosquare[0][opside][capp][to]);
				b->psq_e-=(oidx*p->piecetosquare[1][opside][capp][to]);
				midx=omidx[capp];

// fix for dark bishop
				if(capp==BISHOP) {
					if(normmark[to] & BLACKBITMAP) {
						midx=omidx[DBISHOP];
					}
				}
				b->mindex-=midx;
//# fix hash for castling
				if ((to==opsiderooks)&& (capp==ROOK)&&(b->castle[opside]!=NOCASTLE)) {
					b->castle[opside] &=(~QUEENSIDE);
					if(b->castle[opside]!=ret.castle[opside])
						b->key^=castleKey[opside][QUEENSIDE];
				}
				else if ((to==(opsiderooks+7))&& (capp==ROOK)&&(b->castle[b->side]!=NOCASTLE)) {
					b->castle[opside] &=(~KINGSIDE);
					if(b->castle[opside]!=ret.castle[opside])
						b->key^=castleKey[opside][KINGSIDE];
				}
			}
			b->pawnkey^=randomTable[b->side][from][PAWN]; //pawnhash
			ret.moved=prom;
			movp=prom;
			b->rule50move=b->move;
// remove PAWN, add promoted piece
			b->psq_b-=(sidx*p->piecetosquare[0][b->side][PAWN][from]);
			b->psq_e-=(sidx*p->piecetosquare[1][b->side][PAWN][from]);
			b->psq_b+=(sidx*p->piecetosquare[0][b->side][prom][from]);
			b->psq_e+=(sidx*p->piecetosquare[1][b->side][prom][from]);

			b->mindex-=tmidx[PAWN];
			midx=tmidx[prom];
// fix for dark bishop
			if(prom==BISHOP)
				if(normmark[to] & BLACKBITMAP) {
					midx=tmidx[DBISHOP];
				}
			b->mindex+=midx;
// check validity of mindex and ev. fix it
			if(b->mindex_validity!=0) vcheck=1;;
			break;
		}
		if(oldp!=movp) {
			ClearAll(from, b->side, oldp, b);
			SetAll(to, b->side, movp, b);
		}
		else MoveFromTo(from, to, b->side, oldp, b);
			
/* change HASH:
   - update target
   - restore source
   - set ep
   - change side
   - set castling to proper state
   - update 50key and 50position restoration info
*/

		b->psq_b-=(sidx*p->piecetosquare[0][b->side][movp][from]);
		b->psq_e-=(sidx*p->piecetosquare[1][b->side][movp][from]);
		b->psq_b+=(sidx*p->piecetosquare[0][b->side][movp][to]);
		b->psq_e+=(sidx*p->piecetosquare[1][b->side][movp][to]);

		b->key^=randomTable[b->side][from][oldp];
		b->key^=randomTable[b->side][to][movp];
		b->key^=sideKey;
		if(b->ep!=-1) {
			b->key^=epKey[b->ep]; 
		}

		if(vcheck) check_mindex_validity(b, 1);
		b->move++;
		b->positions[b->move-b->move_start]=b->key;
		b->posnorm[b->move-b->move_start]=b->norm;
		b->side=opside;
return ret;
}

UNDO MakeNullMove(board *b)
{
UNDO ret;
int8_t opside;
		
	opside= (b->side==WHITE) ? BLACK:WHITE;

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
			if(b->side == WHITE) {
				xmidx=MATIdxIncW;
			} else {
				xmidx=MATIdxIncB;
			}
			midx=xmidx[u.captured];
			if(u.captured == BISHOP)
				if(normmark[to] & BLACKBITMAP) {
					midx=xmidx[DBISHOP];
				}
			b->mindex+=midx;
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
			if(b->side == WHITE) {
				xmidx=MATIdxIncW;
			} else {
				xmidx=MATIdxIncB;
			}
			b->mindex+=xmidx[PAWN];
			break;

		case ER_PIECE+1:
		case ER_PIECE:
			break;
		default:
			if(u.side == WHITE) {
				xmidx=MATIdxIncW;
			} else {
				xmidx=MATIdxIncB;
			}
			b->mindex+=xmidx[PAWN];
			midx=xmidx[u.moved];
			if(u.moved == BISHOP)
				if(normmark[to] & BLACKBITMAP) {
					midx=xmidx[DBISHOP];
				}
			b->mindex-=midx;
			break;
		}
		b->side=u.side;
		b->key=u.key;
		b->pawnkey=u.pawnkey;
		b->psq_b=u.psq_b;
		b->psq_e=u.psq_e;
}
			
void  generateInCheckMovesN(board * b, const attack_model *a, move_entry ** m, int gen_u)
{
int from, to, ff, orank;
BITVAR mv, rank, brank, bran2, piece, epbmp, pins;
move_entry * move;
int ep_add;
unsigned char side, opside;



		move = *m;
		if(b->side == WHITE) {
			rank=RANK7;
			side=WHITE;
			opside=BLACK;
			brank=RANK2;
			bran2=RANK4;
			orank=0;
			ff=8;
			ep_add=8;
		}
		else {
			rank=RANK2;
			opside=WHITE;
			side=BLACK;
			brank=RANK7;
			bran2=RANK5;
			orank=56;
			ff=-8;
			ep_add=-8;
		}

	pins=((a->ke[side].cr_pins | a->ke[side].di_pins));
		
// generate queens
		piece=b->maps[QUEEN]&(b->colormaps[side])&(~pins);
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

// generate rooks 
		piece=b->maps[ROOK]&(b->colormaps[side])&(~pins);
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

// bishops
		piece=b->maps[BISHOP]&(b->colormaps[side])&(~pins);
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

// knights
		piece=b->maps[KNIGHT]&(b->colormaps[side])&(~pins);
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

// pawn promotions
		piece = (b->maps[PAWN]) & (b->colormaps[side]) & rank &(~pins);
		while (piece) {
			from = LastOne(piece);
			mv=a->mvs[from] & (~b->norm);
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
			ClrLO(piece);
	}

// pawn promotions after capture/attack
		piece = (b->maps[PAWN]) & (b->colormaps[side]) & rank &(~pins);
		while (piece){
			from = LastOne(piece);
			mv=a->mvs[from] & (b->colormaps[opside]);
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
			ClrLO(piece);
		}

	if(b->ep!=-1) epbmp=attack.ep_mask[b->ep]&b->maps[PAWN]&b->colormaps[side];
	else epbmp=0;
	
// pawn attacks
		piece = (b->maps[PAWN]) & (b->colormaps[side]) & (~rank) & (~epbmp) &(~pins);
		while (piece) {
			from = LastOne(piece);
			mv = a->mvs[from] & (b->colormaps[opside]);
			while (mv) {
				to = LastOne(mv);
				move->move = PackMove(from, to,  ER_PIECE, 0);
				move->qorder=move->real_score=b->pers->LVAcap[PAWN][b->pieces[to]&PIECEMASK];
				move++;
				ClrLO(mv);
			}
			ClrLO(piece);
		}

// pawn attacks
		piece = (b->maps[PAWN]) & (b->colormaps[side]) & (~rank)&(epbmp)&(~pins);
		while (piece) {
			from = LastOne(piece);
			mv = a->mvs[from] & (b->colormaps[opside]);
			while (mv) {
				to = LastOne(mv);
//				LOGGER_0("from %o to %o ep %o\n", from, to, b->ep);
				if((to==b->ep)) {
					move->move = PackMove(from, to+ep_add, PAWN, 0);
					move->qorder=move->real_score=b->pers->LVAcap[PAWN][PAWN];
				} else {
					move->move = PackMove(from, to,  ER_PIECE, 0);
					move->qorder=move->real_score=b->pers->LVAcap[PAWN][b->pieces[to]&PIECEMASK];
				}
				move++;
				ClrLO(mv);
			}
			ClrLO(piece);
		}

// king 
	from = b->king[side];
	mv = (a->mvs[from] & (b->colormaps[opside]));
	while (mv) {
		to = LastOne(mv);
		move->move = PackMove(from, to,  ER_PIECE, 0);
		move->qorder=move->real_score=b->pers->LVAcap[KING][b->pieces[to]&PIECEMASK];
		move++;
	ClrLO(mv);
	}

// knights
		piece=b->maps[KNIGHT]&(b->colormaps[side])&(~pins);
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

// bishops	
		piece=b->maps[BISHOP]&(b->colormaps[side])&(~pins);
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
// generate rooks
		piece=b->maps[ROOK]&(b->colormaps[side])&(~pins);
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
// generate queens
		piece=b->maps[QUEEN]&(b->colormaps[side])&(~pins);
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

// pawn moves
		piece = (b->maps[PAWN]) & (b->colormaps[side]) & (~rank) &(~brank)&(~pins);
		while (piece) {
			from = LastOne(piece);
			mv=a->mvs[from]&(~b->norm);
			while (mv) {
				to = LastOne(mv);
				// doublepush has to be recognised
				move->move = PackMove(from, to,  ER_PIECE, 0);
				move->qorder=move->real_score=MV_OR+P_OR;
				move++;
				ClrLO(mv);
			}
			ClrLO(piece);
		}
		piece = (b->maps[PAWN]) & (b->colormaps[side]) & (~rank) &(brank)&(~pins);
		while (piece) {
			from = LastOne(piece);
			mv=a->mvs[from]&(~b->norm);
			while (mv) {
				to = LastOne(mv);
				// doublepush has to be recognised
				if((mv&bran2)) {
					move->move = PackMove(from, to,  ER_PIECE+1, 0);
					move->qorder=move->real_score=MV_OR+P_OR+1;
				} else {
					move->move = PackMove(from, to,  ER_PIECE, 0);
					move->qorder=move->real_score=MV_OR+P_OR;
				}
				move++;
				ClrLO(mv);
			}
			ClrLO(piece);
		}

// king
			from = b->king[side];
			mv = (a->mvs[from]) & (~b->norm);
			while (mv) {
				to = LastOne(mv);
				ff = getFile(to);
// must mark castling move as such. Identify castling moves
				if(((b->castle[side] & QUEENSIDE)&&(ff==C1))) {
					move->move = PackMove(E1+orank,C1+orank, KING, 0);
					move->qorder=move->real_score=CS_Q_OR;
				} else if(((b->castle[side] & KINGSIDE)&&(ff==G1))) {
					move->move = PackMove(E1+orank,G1+orank, KING, 0);
					move->qorder=move->real_score=CS_K_OR;
				} else {
					move->move = PackMove(from, to,  ER_PIECE, 0);
					move->qorder=move->real_score=MV_OR;
				}
				move++;
				ClrLO(mv);
			}
	*m=move;
}

int alternateMovGen(board * b, MOVESTORE *filter){

//fixme all!!!
int i,f,n, tc,cc,th, f2, t2, piece, ff;
int t2t;
move_entry mm[300], *m;
attack_model *a, aa;
char b2[512], b3[512];

	m = mm;
	a=&aa;
// is side to move in check ?

	a->phase=eval_phase(b, b->pers);

	a->att_by_side[WHITE]=KingAvoidSQ(b, a, WHITE);
	a->att_by_side[BLACK]=KingAvoidSQ(b, a, BLACK);

	eval_king_checks_all (b, a);
	if(isInCheck_Eval(b, a, b->side)!=0) {
		simple_pre_movegen_n2check(b, a, b->side);
		generateInCheckMovesN(b, a, &m, 1);
	}
	else {
		simple_pre_movegen_n2(b, a, b->side);
		generateCapturesN(b, a, &m, 1);
		generateMovesN(b, a, &m);
	}
	n=i=0;
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

		th=filter[n];
		f2=UnPackFrom(th);
		t2=UnPackTo(th);
		piece=b->pieces[f2];
// if filter is castling, we have to normalize to E1-G1 (as it could be written as E1-H1 as well)
		switch(piece&PIECEMASK) {
		case KING:
			if((f2==(E1+b->side*56)) && ((t2==(A1+b->side*56))||(t2==(C1+b->side*56)))) {
				t2=(C1+b->side*56);
				th=PackMove(f2, t2, KING, 0);
			} else if((f2==(E1+b->side*56)) && ((t2==(H1+b->side*56))||(t2==(G1+b->side*56)))) {
				t2=(G1+b->side*56);
				th=PackMove(f2, t2, KING, 0);
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
		  }
		  break;
		case ER_PIECE:
			printBoardNice(b);
			sprintfMove(b, *filter, b2);
			LOGGER_0("no piece at FROM %s\n",b2);
			dump_moves(b, mm, tc, 1, NULL);
			abort();
		  break;
		}
		
		cc=0;
		i=0;
		filter[n]=th&0xFFF;
		sprintfMoveSimple(th&0xFFF, b2);
		while((cc<tc)) {
			if((mm[cc].move&(~CHECKFLAG))==th) {
			  mm[i++].move=mm[cc].move;
			}
			cc++;
		}
		if(i!=1) {
			cc=0;
			while((cc<tc)) {
				DEB_1(sprintfMoveSimple(mm[cc].move, b3);)
				LOGGER_1("%d:%d Filter %s vs %s, %x vs %o ", n, cc, b2, b3, th, mm[cc].move);
				if((mm[cc].move&(~CHECKFLAG))==th) {
				  NLOGGER_1("equal\n");
				  mm[i++].move=mm[cc].move;
				} else NLOGGER_1(" ne\n");
				cc++;
			}
		}
		n++;
	}
	mm[i].move=0;
	f=0;
	while(mm[f].move!=0) {
		filter[f]=mm[f].move;
		f++;
	}
	return f;
}

//+PSQSearch(from, to, KNIGHT, side, a->phase, p)

/*
 * Sorts moves, they should be stored in order generated
 * it should resort captures to MVVLVA 
 * "bad" captures are rechecked via SEE - triggered in sorting
 * noncaptures sorted with HHeuristics - triggered in sorting
 */

// mv->lastp-1 - posledni element
// mv->next - prvni element

void SelectBestO(move_cont *mv)
{
move_entry *t, a1, *j;
	t=mv->next+1;
	while(t<mv->lastp) {
	  a1=*t;
	  j=t-1;
	  while((j>=mv->next) && (j->qorder < a1.qorder)) {
			*(j+1) = *j;
			j--;
	  }
	  *(j+1)=a1;
	  t++;
	}
}

// mv->next points to move to be selected/played
// mv->lastp points behind generated moves

void SelectBest(move_cont *mv)
{
move_entry *t, a1;
	
	for(t=mv->lastp-1; t>(mv->next); t--) {
	  if(t->qorder > (t-1)->qorder) {
		a1=*(t-1);
		*(t-1)=*t;
		*t=a1;
	  }
	}
}

void ScoreNormal(board *b, move_cont *mv, int side){
move_entry *t;
int fromPos, ToPos, piece, opside, dist;

	opside = side == WHITE ? BLACK : WHITE;
	for(t=mv->lastp-1;t>mv->next; t--) {
			fromPos=UnPackFrom(t->move);
			ToPos=UnPackTo(t->move);
			piece=b->pieces[fromPos]&PIECEMASK;
			t->qorder=checkHHTable(b->hht, side, piece, ToPos)+MV_OR;
// assign priority based on distance to enemy king or promotion
#if 1
			if(piece==PAWN) {
				dist= side == WHITE ? 7-getRank(ToPos) : getRank(ToPos);
			} else {
				dist=attack.distance[ToPos][b->king[opside]];
			}
#endif
			if(t->qorder==MV_OR) t->qorder=MV_OR+8-dist;
	}
}

int ExcludeMove(move_cont *mv, MOVESTORE mm){
move_entry *t;

	t=mv->excl;
	while(t<mv->exclp) {
		if(t->move==mm) {
			return 1;
		}
		t++;
	}
return 0;
}

void invalidDump(board *b, MOVESTORE m, int side){
char bb[256];
  printBoardNice(b);
  sprintfMoveSimple(m, bb);
  LOGGER_1("failed move %s\n",bb);
  printboard(b);
return;
}

int getNextMove(board *b, const attack_model *a, move_cont *mv, int ply, int side, int incheck, move_entry **mm, tree_store *tree){

MOVESTORE pot;
int r;
	switch (mv->phase) {
	case INIT:
	// setup everything
		mv->lastp=mv->move;
		mv->next=mv->lastp;
		mv->badp=mv->bad;
		mv->exclp=mv->excl;
		mv->count=0;
		mv->phase=PVLINE;
// previous PV move
	case PVLINE:
		mv->phase=HASHMOVE;
	case HASHMOVE:
		mv->phase=GENERATE_CAPTURES;
		if((mv->hash.move!=DRAW_M)&&(b->hs!=NULL) &&
		  isMoveValid(b, mv->hash.move, a, side, tree) && (!ExcludeMove(mv, mv->hash.move))) {
				mv->lastp->move=mv->hash.move;
				*mm=mv->lastp;
				*(mv->exclp)=*(mv->lastp);
				mv->lastp++;
				mv->exclp++;
				mv->next=mv->lastp;
				mv->actph=HASHMOVE;
				return ++mv->count;
		} else {
		}
	case GENERATE_CAPTURES:
		mv->phase=CAPTUREA;
		mv->next=mv->lastp;
		if(incheck==1) {
				generateInCheckMovesN(b, a, &(mv->lastp), 1);
				goto rest_moves;
		}
		generateCapturesN(b, a, &(mv->lastp), 1);
		mv->tcnt=0;
		mv->actph=CAPTUREA;
	case CAPTUREA:
		while((mv->next<mv->lastp)&&(mv->tcnt>0)) {
			mv->tcnt--;
			if(mv->tcnt==0) mv->phase=SORT_CAPTURES;
			SelectBest(mv);
			if(((mv->next->qorder<A_OR2_MAX)&&(mv->next->qorder>A_OR2)) && (SEEx(b, mv->next->move)<0)) {
				*(mv->badp)=*(mv->next);
				mv->badp++;
				mv->next++;
				continue;
			}
			*mm=mv->next;
			mv->next++;
			return ++mv->count;
		}
	case SORT_CAPTURES:
		SelectBestO(mv);
		mv->actph=CAPTURES;
		mv->phase=CAPTURES;
	case CAPTURES:
		while(mv->next<mv->lastp) {
			if(mv->next==(mv->lastp-1)) mv->phase=KILLER1;
			if(((mv->next->qorder<A_OR2_MAX)&&(mv->next->qorder>A_OR2)) && (SEEx(b, mv->next->move)<0)) {
				*(mv->badp)=*(mv->next);
				mv->badp++;
				mv->next++;
				continue;
			}
			*mm=mv->next;
			mv->next++;
			return ++mv->count;
		}
		mv->phase=KILLER1;
	case KILLER1:
		mv->phase=KILLER2;
		if((b->pers->use_killer>=1)) {
			r = get_killer_move(b->kmove, ply, 0, &pot);
			if (r && isMoveValid(b, pot, a, side, tree) && (!ExcludeMove(mv, pot))) {
				mv->lastp->move=pot;
				*mm=mv->lastp;
				*(mv->exclp)=*(mv->lastp);
				mv->exclp++;
				mv->lastp++;
				mv->next=mv->lastp;
				mv->actph=KILLER1;
				return ++mv->count;
			}
		}
	case KILLER2:
		mv->phase=KILLER3;
		if((b->pers->use_killer>=1)) {
			r = get_killer_move(b->kmove, ply, 1, &pot);
			if(r && isMoveValid(b,pot, a, side, tree) && (!ExcludeMove(mv, pot))) {
				mv->lastp->move=pot;
				*mm=mv->lastp;
				*(mv->exclp)=*(mv->lastp);
				mv->exclp++;
				mv->lastp++;
				mv->next=mv->lastp;
				mv->actph=KILLER2;
				return ++mv->count;
			}
		}
	case KILLER3:
		mv->phase=KILLER4;
		if((b->pers->use_killer>=1)) {
			if(ply>2) {
				r = get_killer_move(b->kmove, ply-2, 0, &pot);
				if(r && isMoveValid(b,pot, a, side, tree) && (!ExcludeMove(mv, pot))) {
					mv->lastp->move=pot;
					*mm=mv->lastp;
					*(mv->exclp)=*(mv->lastp);
					mv->exclp++;
					mv->lastp++;
					mv->next=mv->lastp;
					mv->actph=KILLER3;
					return ++mv->count;
				}
			}
		}
	case KILLER4:
		mv->phase=GENERATE_NORMAL;
//		mv->phase=OTHER_SET;
		if((b->pers->use_killer>=1)) {
			if(ply>2) {
				r = get_killer_move(b->kmove, ply-2, 1, &pot);
				if(r && isMoveValid(b,pot, a, side, tree) && (!ExcludeMove(mv, pot))) {
					mv->lastp->move=pot;
					*mm=mv->lastp;
					*(mv->exclp)=*(mv->lastp);
					mv->exclp++;
					mv->lastp++;
					mv->next=mv->lastp;
					mv->actph=KILLER4;
					return ++mv->count;
				}
			}
		}
	case GENERATE_NORMAL:
		mv->next=mv->lastp;
		generateMovesN(b, a, &(mv->lastp));
		// get HH values and sort
		ScoreNormal(b, mv, side);
		SelectBestO(mv);
rest_moves:
		mv->phase=NORMAL;
		mv->actph=NORMAL;
	case NORMAL:
		while(mv->next<mv->lastp) {
			if(ExcludeMove(mv, mv->next->move)) {
				mv->next++;
				continue;
			}
			*mm=mv->next;
			mv->next++;
			return ++mv->count;
		}
		mv->phase=OTHER_SET;
	case OTHER_SET:
		mv->phase=OTHER;
		mv->next=mv->bad;
	case OTHER:
		while(mv->next<mv->badp) {
			if(ExcludeMove(mv, mv->next->move)) {
				mv->next++;
				continue;
			}
			if(mv->next==(mv->lastp-1)) mv->phase=DONE;
			*mm=mv->next;
			mv->next++;
			mv->actph=OTHER;
			return ++mv->count;
		}
	case DONE:
		break;
//	default:
	}
return 0;
}

int getNextCheckin(board *b, const attack_model *a, move_cont *mv, int ply, int side, int incheck, move_entry **mm, tree_store *tree){
	switch (mv->phase) {
	case INIT:
	// setup everything
		mv->lastp=mv->move;
		mv->next=mv->lastp;
		mv->badp=mv->bad;
		mv->exclp=mv->excl;
		mv->count=0;
		mv->phase=PVLINE;
// previous PV move
	case PVLINE:
		mv->phase=GENERATE_NORMAL;
	case GENERATE_NORMAL:
		generateQuietCheckMovesN(b, a, &(mv->lastp));
		mv->actph=NORMAL;
		mv->phase=NORMAL;
	case NORMAL:
		while(mv->next<mv->lastp) {
			if((SEEx(b, mv->next->move)<0)) {
				mv->next++;
				continue;
			}
			*mm=mv->next;
			mv->next++;
			return ++mv->count;
		}
		mv->phase=OTHER;
		mv->next=mv->bad;
	case OTHER:
			mv->phase=DONE;
	case DONE:
		break;
//	default:
	}
return 0;
}

int getNextCap(board *b, const attack_model *a, move_cont *mv, int ply, int side, int incheck, move_entry **mm, tree_store *tree){
	switch (mv->phase) {
	case INIT:
	// setup everything
		mv->lastp=mv->move;
		mv->badp=mv->bad;
		mv->exclp=mv->excl;
		mv->count=0;
		mv->phase=PVLINE;
		mv->lpcheck=BitCount((b->maps[BISHOP]|b->maps[ROOK]|b->maps[QUEEN]|b->maps[PAWN])&b->colormaps[Flip(side)])!=1;

// previous PV move
	case PVLINE:
		mv->phase=GENERATE_CAPTURES;
	case GENERATE_CAPTURES:
		mv->phase=CAPTUREA;
		mv->next=mv->lastp;
		generateCapturesN(b, a, &(mv->lastp), 0);
		mv->tcnt=0;
		mv->actph=CAPTUREA;
	case CAPTUREA:
		while((mv->next<mv->lastp)&&(mv->tcnt>0)) {
			mv->tcnt--;
			if(mv->tcnt==0) mv->phase=SORT_CAPTURES;
			SelectBest(mv);
			if(((mv->next->qorder<A_OR2_MAX)&&(mv->next->qorder>A_OR2)) && (SEEx(b, mv->next->move)<0)&&mv->lpcheck) {
				mv->next++;
				continue;
			}
			*mm=mv->next;
			mv->next++;
			return ++mv->count;
		}
	case SORT_CAPTURES:
		SelectBestO(mv);
		mv->actph=CAPTURES;
		mv->phase=CAPTURES;
	case CAPTURES:
		while(mv->next<mv->lastp) {
			if(mv->next==(mv->lastp-1)) mv->phase=DONE;
			if(((mv->next->qorder<A_OR2_MAX)&&(mv->next->qorder>A_OR2)) && (SEEx(b, mv->next->move)<0)&&mv->lpcheck) {
				mv->next++;
				continue;
			}
			*mm=mv->next;
			mv->next++;
			return ++mv->count;
		}
		mv->phase=DONE;
	case DONE:
	    break;
	}
return 0;
}

int sortMoveListNew_Init(board *b, attack_model *a, move_cont *mv) {
	mv->phase=INIT;
	mv->hash.move=DRAW_M;
	return 0;
}

/*
 * Degrade second move in row with the same piece, scale by phase; maximal effect in beginning of the game
 * doesnt affect promotions, ep, rochade
 */

int gradeMoveInRow(board *b, attack_model *a, MOVESTORE square, move_entry *n, int count)
{
int q;

int s,p, min;
long int val;
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
	b2[0]='\0';
	if(prom!=ER_PIECE) {
			if(prom==QUEEN) sprintf(b2, "q");
			else if(prom==KNIGHT) sprintf(b2, "n");
			else if(prom==ROOK) sprintf(b2, "r");
			else if(prom==BISHOP) sprintf(b2, "b");
			strcat(buf,b2);
	}
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
 *
 * [MovingPiece][from][x]TO[promotion][+]
 *
 */
/*
 * moves encoding
 * prom to
 * 		- KING, means rochade (rochade can be encoded without it, but this helps)
 * 		- PAWN, means EP
 * 		- ER_PIECE, normal movement
 * 		- ER_PIECE+1, doublepush (can be encoded without it, but it helps)
 */

//
void sprintfMove(board *b, MOVESTORE m, char * buf)
{
int from, to, prom, cap, side, mate;

unsigned char pto, pfrom;
char b2[512], b3[512];
int ep_add;
BITVAR aa;
		
		ep_add= b->side == WHITE ? +8 : -8;
		buf[0]='\0';
		from=UnPackFrom(m);
		to=UnPackTo(m);
		prom=UnPackProm(m);
		mate=0;
		pfrom=b->pieces[from]&PIECEMASK;
		pto=b->pieces[to]&PIECEMASK;
		side=(b->pieces[from]&BLACKPIECE)==0 ? WHITE : BLACK;
		if((pfrom==PAWN) && (to==(b->ep+ep_add))) pto=PAWN;
		switch (m&(~CHECKFLAG)) {
			case DRAW_M:
					strcat(buf," Draw ");
					return;
			case MATE_M:
					strcat(buf,"# ");
					mate=1;
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
								aa=((attack.pawn_att[WHITE][to] & b->maps[PAWN] & (b->colormaps[BLACK])) |
										(attack.pawn_att[BLACK][to] & b->maps[PAWN] & (b->colormaps[WHITE])));
								aa&=b->colormaps[side];
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

			 b3[0]='\0';
			if((BitCount(aa)>1)||((cap==1)&&(pfrom==PAWN))) {
				if(BitCount(attack.file[from]&aa)==1) {
					sprintf(b3,"%c", getFile(from)+'a');
// file is enough
				} else if(BitCount(attack.rank[from]&aa)==1) {
// rank is enough
					sprintf(b3,"%c", getRank(from)+'1');
				} else {
// file&rank are needed
					sprintf(b3,"%c%c", getFile(from)+'a', getRank(from)+'1');
				}
			}
// poskladame vystup do buf
		strcat(b2, b3);
		strcat(b2, buf);
		strcpy(buf, b2);

			if((pfrom == KING)&&(prom==KING)) {
				if(from > to) sprintf(b2, "O-O-O"); else sprintf(b2, "O-O");
				sprintf(buf,"%s",b2);
			} else if((pfrom==PAWN)) {
				if(prom==QUEEN) strcat(buf, "=Q");
				else if(prom==KNIGHT) strcat(buf, "=N");
				else if(prom==BISHOP) strcat(buf, "=B");
				else if(prom==ROOK) strcat(buf, "=R");
				}
		if((m&CHECKFLAG)&&(mate!=1)) {
			strcat(buf,"+");
		}
}


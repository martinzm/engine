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

int movescheck(board *b, move_entry *m, move_entry *end) {
int to;
	while(m<end) {
		to=UnPackTo(m->move);
		if((b->pieces[to]&PIECEMASK)==KING) {
			printBoardNice(b);
char bb[256];
			sprintfMoveSimple(m->move, bb);
			LOGGER_0("failed move %s\n",bb);
			printboard(b);
			return 0;
		}
	m++;
	}
	return 1;
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
int to;
char buff[256];
//	x=m->qorder;
//	if(x>=HASH_OR) x-=HASH_OR;
//	if((x>=(A_OR_N)) || ((x>=(A_OR2)) && (x<=(A_OR2_MAX)))||((MV_BAD<=x)&&(x<=(MV_BAD_MAX)))) return 0;
//	if(((x>=(MV_OR)) && (x<=(MV_HH_MAX))) || ((x>=(KILLER_OR))&&(x<=KILLER_OR_MAX)) || ((x==CS_Q_OR)||(x==CS_K_OR))) return 1;
	to=UnPackTo(m->move);
	if(b->pieces[to]==ER_PIECE) return 1;
 return 0;
}

// eval musi byt proveden! 

void generateCaptures(board * b, const attack_model *a, move_entry ** m, int gen_u)
{
int from, to;
BITVAR x, mv, rank, piece, npins, block_ray;
move_entry * move;
int ep_add, pie;
unsigned char side, opside;
//king_eval ke;
personality *p;
char b2[256];

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
 * zavislost na opside analyze. simple_pre_movegen must be done for att_by_side
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
// pin? ???
				if(!AttackedTo_B(b, b->king[b->side], b->side)) {
					move->move = PackMove(from, to, PAWN, 0);
// b->ep has real position of DoublePush
// to is where capturing pawn will end
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


/*
 * Serialize moves from bitmaps, capture type of moves available at board for side
 */
 
void generateCapturesN(board * b, const attack_model *a, move_entry ** m, int gen_u)
{
int from, to;
BITVAR x, mv, rank, piece, npins, block_ray, brank, epbmp;
move_entry * move;
int ep_add, pie;
unsigned char side, opside;
//king_eval ke;
personality *p;
char b2[256];

		p=b->pers;
	
		move = *m;
		if(b->side == WHITE) {
			rank=RANK7;
			side=WHITE;
			opside=BLACK;
			ep_add=8;
			brank=RANK2;
		}
		else {
			rank=RANK2;
			opside=WHITE;
			side=BLACK;
			ep_add=-8;
			brank=RANK7;
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
//#define xNP(P) np[P]=t2[P]&(~pins);
#define xNP(P) np[P]=pi[P]^t2[P];

int simple_pre_movegen_n2(board const *b, attack_model *a, int side)
{
int f, from, pp, st, en, epn, add, opside, orank;
BITVAR x, q, pins, epbmp, tmp, kpin, nmf, t2[ER_PIECE];

BITVAR un[16];
int pos[16];
int drv;

BITVAR np[ER_PIECE+1];
BITVAR pi[ER_PIECE+1];
	if(side==BLACK) {
		st=ER_PIECE|BLACKPIECE;
		en=PAWN|BLACKPIECE;
		add=BLACKPIECE;
		opside=WHITE;
		orank=56;
	} else {
		add=0;
		st=ER_PIECE;
		en=PAWN;
		opside=BLACK;
		orank=0;
	}

	epbmp= (b->ep!=-1 && (a->ke[side].ep_block==0)) ? attack.ep_mask[b->ep]&b->maps[PAWN]&b->colormaps[side] : 0;
	q=0;
	pins=((a->ke[side].cr_pins | a->ke[side].di_pins));

#if 0
	for(f=PAWN;f<ER_PIECE;f++) {
		t2[f]=b->maps[f]&b->colormaps[side];
		pi[f]=t2[f]&pins;
		np[f]=t2[f]&(~pins);
	}
#else

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
#endif

	GETMOVES(np[QUEEN], QueenAttacks);
	GETMOVES(np[ROOK], RookAttacks);
	GETMOVES(np[BISHOP], BishopAttacks);
	GETMOVEK(np[KNIGHT], KNIGHT);

	GETMOVESP(pi[QUEEN], QueenAttacks);
	GETMOVESP(pi[ROOK], RookAttacks);
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
				if(tmp && (nmf&RANK2)) tmp|= (tmp<<8)&(~b->norm);
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
	switch(side) {
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
int f, from, pp, st, en, add, opside, orank, attacker;
BITVAR x, q, pins, epbmp, tmp, attack_ray, tmp2, tmp3;

BITVAR np[ER_PIECE+1];
BITVAR pi[ER_PIECE+1];

char b2[256];

	if(side==BLACK) {
		st=ER_PIECE|BLACKPIECE;
		en=PAWN|BLACKPIECE;
		add=BLACKPIECE;
		opside=WHITE;
		orank=56;
	} else {
		add=0;
		st=ER_PIECE;
		en=PAWN;
		opside=BLACK;
		orank=0;
	}
	epbmp= (b->ep!=-1 && (a->ke[side].ep_block==0)) ? attack.ep_mask[b->ep]&b->maps[PAWN]&b->colormaps[side] : 0;
	tmp2=attack_ray=q=0;
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

void generateMoves(board * b, const attack_model *a, move_entry ** m)
{
int from, to;
BITVAR x, mv, rank, brank, pmv, y, piece, npins, block_ray;
move_entry * move;
int orank, back, ff, pie;
unsigned char side, opside;
personality *p;
char b2[256];

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
//????			- make sure that pinned pawn stays in direction of attack
			if((normmark[to])&(attack.rays_dir[b->king[side]][from])) {
				move->move = PackMove(from, to,  ER_PIECE+1, 0);
				move->qorder=move->real_score=MV_OR+P_OR+1;
				move++;
			}
			ClrLO(pmv);
		}

// king
		x = (b->maps[KING]) & (b->colormaps[side]);
		while (x) {
			from = LastOne(x);
//			printmask(attack.maps[KING][from], "king map");
//			printmask(~b->norm, "~b->norm");
//			printmask(~attack.maps[KING][b->king[opside]], "~attack.maps[KING][b->king[opside]]");
//			printmask(~a->att_by_side[opside], "~a->att_by_side[opside]");
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

/*
 * Serialize moves from bitmaps for all types of moves available at board for side
 */

void generateMovesN(board * b, const attack_model *a, move_entry ** m)
{
int from, to;
BITVAR x, mv, rank, brank, pmv, y, piece, npins, block_ray, bran2;
move_entry * move;
int orank, back, ff, pie, f2;
unsigned char side, opside;
personality *p;
char b2[256];

		p=b->pers;

		move = *m;
		if(b->side == WHITE) {
			rank=RANK7;
			side=WHITE;
			opside=BLACK;
			brank=RANK2;
			bran2=RANK4;
			orank=0;
			back=0;
			ff=8;
		}
		else {
			rank=RANK2;
			opside=WHITE;
			side=BLACK;
			brank=RANK7;
			bran2=RANK5;
			orank=56;
			back=16;
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

void generateQuietCheckMoves(board * b, const attack_model *a, move_entry ** m)
{
int from, to;
BITVAR x, mv, rank, brank, pmv, y, npins, block_ray, piece;
move_entry * move;
int back, ff, pie;
unsigned char side, opside;
king_eval kee, *ke;

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

		ke=&kee;
		eval_king_checks_full(b, ke, NULL, opside);
		
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
BITVAR x, mv, rank, brank, pmv, y, pins, block_ray, piece, bran2;
move_entry * move;
int back, ff, pie;
unsigned char side, opside;
king_eval kee, *ke;

personality *p;

		p=b->pers;

	
		move = *m;
		if(b->side == WHITE) {
			rank=RANK7;
			side=WHITE;
			opside=BLACK;
			brank=RANK2;
			bran2=RANK4;
			back=0;
			ff=8;
		}
		else {
			rank=RANK2;
			opside=WHITE;
			side=BLACK;
			brank=RANK7;
			bran2=RANK5;
			back=16;
			ff=-8;
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
				ff = getFile(to);
					move->move = PackMove(from, to,  ER_PIECE, 0);
					move->qorder=move->real_score=MV_OR;
					move++;
					ClrLO(mv);
				}
			}
	*m=move;
}

int kingCheck(board *b)
{
BITVAR x;
int from, c1,c2;
	
		x = (b->maps[KING]) & (b->colormaps[WHITE]);
		c1 = BitCount(x);
		if(!c1) {
			LOGGER_0("Missing kings\n");
			return 0;
		}
		from = LastOne(x);
		if((x==0ULL)||(from!=b->king[WHITE])||(c1!=1)) {
			LOGGER_0("%lld, %d=%lld %o, count %d WHITE\n", (unsigned long long) x,from, 1ULL<<from, b->king[WHITE], c1);
			return 0;
		} 
		x = (b->maps[KING]) & (b->colormaps[BLACK]);
		c2 = BitCount(x);
		from = LastOne(x);
		if((x==0ULL)||(from!=b->king[BLACK])||(c2!=1)) {
			LOGGER_0("%lld, %o=%lld, %o, count %d BLACK\n", (unsigned long long) x,from, 1ULL<<from, b->king[BLACK], c2);
			return 0;
		} 
		x=attack.maps[KING][b->king[WHITE]];
		if(x&normmark[b->king[BLACK]]) {
			printmask(x, "x");
			printmask(attack.maps[KING][b->king[BLACK]],"BLACK");
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
int bwd, bbd, bwl, bbl, pw, pb, bbl2, bbd2, bwl2, bwd2, nb, nw, rb, rw, qb, qw;
//int nob;
	
		if(kingCheck(b)==0) {
			printBoardNice(b);
			LOGGER_0("king error!\n");
		}
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
			LOGGER_1("ERR: %s, Keys dont match, board key %llX, computed key %llX\n",name, (unsigned long long) b->key, (unsigned long long) key);
//			printBoardNice(b);
//			return 0;
//			abort();
		}
		matidx=computeMATIdx(b);
		if(b->mindex!=matidx) {
			ret=0;
			LOGGER_1("ERR: %s, Material indexes dont match, board mindex %X, computed mindex %X\n",name, b->mindex, matidx);
//			printBoardNice(b);
//			return 0;
//			abort();
		}

#if 0		
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
//				return 0;
//				abort();
			}
		}
#endif
		
// material checks
#if 0
		pp=0;
		bwd=b->material[WHITE][DBISHOP];
		bbd=b->material[BLACK][DBISHOP];
		bwl=b->material[WHITE][BISHOP]-bwd;
		bbl=b->material[BLACK][BISHOP]-bbd;
		pw=BitCount(b->maps[PAWN]&b->colormaps[WHITE]);
		pb=BitCount(b->maps[PAWN]&b->colormaps[BLACK]);
		nw=BitCount(b->maps[KNIGHT]&b->colormaps[WHITE]);
		nb=BitCount(b->maps[KNIGHT]&b->colormaps[BLACK]);

		bwl2=BitCount(b->maps[BISHOP]&b->colormaps[WHITE]&WHITEBITMAP);
		bbl2=BitCount(b->maps[BISHOP]&b->colormaps[BLACK]&WHITEBITMAP);
		bwd2=BitCount(b->maps[BISHOP]&b->colormaps[WHITE])-bwl2;
		bbd2=BitCount(b->maps[BISHOP]&b->colormaps[BLACK])-bbl2;

		rw=BitCount(b->maps[ROOK]&b->colormaps[WHITE]);
		rb=BitCount(b->maps[ROOK]&b->colormaps[BLACK]);
		qw=BitCount(b->maps[QUEEN]&b->colormaps[WHITE]);
		qb=BitCount(b->maps[QUEEN]&b->colormaps[BLACK]);

		if(pw!=b->material[WHITE][PAWN]) pp++;
		if(pb!=b->material[BLACK][PAWN]) pp++;
		if(nw!=b->material[WHITE][KNIGHT]) pp++;
		if(nb!=b->material[BLACK][KNIGHT]) pp++;

		if(bwd!=bwd2) pp++;
		if(bbd!=bbd2) pp++;
		if(bwl!=bwl2) pp++;
		if(bbl!=bbl2) pp++;

		if(rw!=b->material[WHITE][ROOK]) pp++;
		if(rb!=b->material[BLACK][ROOK]) pp++;
		if(qw!=b->material[WHITE][QUEEN]) pp++;
		if(qb!=b->material[BLACK][QUEEN]) pp++;

		if(pp>0) {
			printBoardNice(b);
			printboard(b);
			if(pw!=b->material[WHITE][PAWN]) LOGGER_0("boardcheck WP problem mat %d: board %d\n", b->material[WHITE][PAWN], pw);
			if(pb!=b->material[BLACK][PAWN]) LOGGER_0("boardcheck BP problem mat %d: board %d\n", b->material[BLACK][PAWN], pb);
			if(nw!=b->material[WHITE][KNIGHT]) LOGGER_0("boardcheck WN problem mat %d: board %d\n", b->material[WHITE][KNIGHT], nw);
			if(nb!=b->material[BLACK][KNIGHT]) LOGGER_0("boardcheck BN problem mat %d: board %d\n", b->material[BLACK][KNIGHT], nb);

			if(bwd!=bwd2) LOGGER_0("boardcheck WB problem mat %d: board %d\n", bwd2, bwd);
			if(bbd!=bbd2) LOGGER_0("boardcheck BB problem mat %d: board %d\n", bbd2, bbd);
			if(bwl!=bwl2) LOGGER_0("boardcheck WBL problem mat %d: board %d\n", bwl2, bwl);
			if(bbl!=bbl2) LOGGER_0("boardcheck BBL problem mat %d: board %d\n", bbl2, bbl);

			if(rw!=b->material[WHITE][ROOK]) LOGGER_0("boardcheck WR problem mat %d: board %d\n", b->material[WHITE][ROOK], rw);
			if(rb!=b->material[BLACK][ROOK]) LOGGER_0("boardcheck BR problem mat %d: board %d\n", b->material[BLACK][ROOK], rb);
			if(qw!=b->material[WHITE][QUEEN]) LOGGER_0("boardcheck WQ problem mat %d: board %d\n", b->material[WHITE][QUEEN], qw);
			if(qb!=b->material[BLACK][QUEEN]) LOGGER_0("boardcheck BQ problem mat %d: board %d\n", b->material[BLACK][QUEEN], qb);
			return 0;
		}
#endif
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
 *
 * requires eval_king_checks for pins
 */

int isMoveValid(board *b, MOVESTORE move, const attack_model *a, int side, tree_store *tree)
{
int from, to, prom, movp, capp, opside, pside, tot, prank, pfile;
BITVAR bfrom, bto, m, ep, path, path2, npins;
char b2[256];
king_eval kee, *ke;

int ret;

	ret=0;
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
//			LOGGER_0("Move validation ep tot %o, to %o, pin %d\n", tot, to, npins!=0);
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
//			if((b->trace!=0)&&(movp!=(PAWN|pside))) LOGGER_0("FTPM (movp!=(PAWN|pside) hit\n", from, to, prom, movp);
			if(movp!=(PAWN|pside)) return 0;
			tot=getRank(to);
//			if((b->trace!=0)&&(((side==WHITE)&&(tot!=7))||((side==BLACK)&&(tot!=0)))) {
//			LOGGER_0("FTPM TOT %o, to %o, side %d,  err2\n", tot, to, side);
//			
//			}
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
#if 0
	if((b->trace!=0)) {
	printmask(m, "m");
	printmask(bto, "bto");
	printmask(bfrom, "bfrm");
	printmask(path, "path");
	}
#endif
	
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
//int sidemask;
int * tmidx;
int * omidx;
int64_t *tmidx2, *omidx2, midx2;
int midx;
char b2[256];
int tmp, tmp2, tmp3;
int sidx, oidx, to_f;

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
//				printPV_simple_act(b,(tree_store*) b->td, 99, b->side, NULL, NULL);
				ret.move=NA_MOVE;
				assert(0);
				return ret;
			}
		}
		if((capp==KING)) {
			printBoardNice(b);
			sprintfMoveSimple(move, b2);
			LOGGER_0("failed move %s\n",b2);
//			printPV_simple_act(b,(tree_store*) b->td, 99, b->side, NULL, NULL);
			printboard(b);
			ret.move=NA_MOVE;
			return ret;
//			assert(0);
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
//				b->material[opside][capp]--; // opside material change
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
//						b->material[opside][DBISHOP]--;
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
//			if((oldp==KING)&&(from==kingbase)&&(b->castle[b->side]!=NOCASTLE)) {
			if((oldp==KING)) {
				b->king[b->side]=to;
				if((from==kingbase)&&(b->castle[b->side]!=NOCASTLE)){
					b->castle[b->side]=NOCASTLE;
//				if(b->castle[b->side]!=ret.castle[b->side])
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
//			b->material[opside][PAWN]--; // opside material change
			b->mindex-=omidx[PAWN];

// update pawn captured
			b->psq_b-=(oidx*p->piecetosquare[0][opside][PAWN][ret.ep]);
			b->psq_e-=(oidx*p->piecetosquare[1][opside][PAWN][ret.ep]);

			b->key^=randomTable[opside][ret.ep][PAWN]; //hash
			b->rule50move=b->move;
			b->pawnkey^=randomTable[b->side][from][PAWN]; //pawnhash
			b->pawnkey^=randomTable[b->side][to][PAWN]; //pawnhash
			b->pawnkey^=randomTable[opside][ret.ep][PAWN]; //pawnhash
//			check_mindex_validity(b, 1);
			break;
		default:
// promotion
			if(capp!=ER_PIECE) {
// promotion with capture
				b->key^=randomTable[opside][to][capp]; //hash
				ClearAll(to, opside, capp, b);
//				b->material[opside][capp]--; // opside material change
// remove captured piece
				b->psq_b-=(oidx*p->piecetosquare[0][opside][capp][to]);
				b->psq_e-=(oidx*p->piecetosquare[1][opside][capp][to]);
				midx=omidx[capp];

// fix for dark bishop
				if(capp==BISHOP) {
					if(normmark[to] & BLACKBITMAP) {
						midx=omidx[DBISHOP];
//						b->material[opside][DBISHOP]--;
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
//			b->material[b->side][PAWN]--; // side material change - PAWN
//			b->material[b->side][prom]++; // side material change
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
//					b->material[b->side][DBISHOP]++;
				}
			b->mindex+=midx;
//			LOGGER_0("Prom %d, midx %d, mindex %lld\n", prom, midx, b->mindex);
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
int64_t midx2, *xmidx2;
int * xmidx;

char b2[256];

//		sprintfMoveSimple(u.move, b2);
//		LOGGER_0("UnMakeMove move %s\n",b2);

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
//			b->material[b->side][u.captured]++; // opside material change
			if(b->side == WHITE) {
				xmidx=MATIdxIncW;
			} else {
				xmidx=MATIdxIncB;
			}
			midx=xmidx[u.captured];
			if(u.captured == BISHOP)
				if(normmark[to] & BLACKBITMAP) {
					midx=xmidx[DBISHOP];
//					b->material[b->side][DBISHOP]++;
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
//			b->material[b->side][PAWN]++; // opside material change
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
//			b->material[u.side][PAWN]++; // side material change
//			b->material[u.side][u.moved]--; // side material change
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
//					b->material[u.side][DBISHOP]--;
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
			
void  generateInCheckMoves(board * b, const attack_model *a, move_entry ** m)
{
BITVAR at2, at4, utc, mezi, pole, pd1, pd2;
	
int from, to, num;
BITVAR x, mv, rank, pmv, brank, npins, block_ray;
move_entry * move;
int ep_add, pie, f;
unsigned char side, opside;
king_eval ke;
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

// count attackers
		num=BitCount(at4);
		from=0; //dummy!
		
// king moves + captures
		x = (b->maps[KING]) & (b->colormaps[side]);
			from = LastOne(x);
			ClearAll(from, side, KING, b);
			mv = (attack.maps[KING][from])	& (~b->colormaps[side]) & (~attack.maps[KING][b->king[opside]]);
			mv = mv & (~a->att_by_side[opside]) & (~a->ke[side].cr_att_ray) & (~a->ke[side].di_att_ray);
//!!! a->att_by_side[opside] is not sometimes initialized !!!
			while (mv) {
				to = LastOne(mv);
						move->move = PackMove(from, to, ER_PIECE, 0);
						move->qorder=move->real_score= ((b->pieces[to]&PIECEMASK)!=ER_PIECE) ?
							b->pers->LVAcap[KING][b->pieces[to]&PIECEMASK] : MV_OR;
						move++;
//				mv =ClrNorm(to,mv);
						ClrLO(mv);
			}	
			SetAll(from, side, KING, b);
		
		at2=0;
		utc=0;
		mezi=0;
		pole=0;

// sebrani utocnika a blokovani
		if(num==1) {
			utc=at4;
			to = LastOne(at4);
				at2 = a->ke[side].cr_att_ray | a->ke[side].di_att_ray;
				at4 |= at2;
				mezi=attack.rays_int[from][to];
			pole=mezi|utc;

		x=b->maps[QUEEN]&(b->colormaps[side]);
		while(x) {
			from = LastOne(x);
			mv=a->mvs[from] & pole;
			if(normmark[from]&(~npins)) mv&=attack.rays_dir[b->king[side]][from];
			while (mv) {
				to = LastOne(mv);
				move->move = PackMove(from, to,  ER_PIECE, 0);
				move->qorder=move->real_score= (normmark[to]&utc) ? b->pers->LVAcap[QUEEN][b->pieces[to]&PIECEMASK] : MV_OR+Q_OR;
				move++;
				ClrLO(mv);
			}
			ClrLO(x);
		}
// rooks
		x=b->maps[ROOK]&(b->colormaps[side]);
		while(x) {
			from = LastOne(x);
			mv=a->mvs[from] & pole;
			if(normmark[from]&(~npins)) mv&=attack.rays_dir[b->king[side]][from];
			while (mv) {
				to = LastOne(mv);
				move->move = PackMove(from, to,  ER_PIECE, 0);
				move->qorder=move->real_score= (normmark[to]&utc) ? b->pers->LVAcap[ROOK][b->pieces[to]&PIECEMASK] : MV_OR+R_OR;
				move++;
				ClrLO(mv);
			}
			ClrLO(x);
		}
// bishops
		x=b->maps[BISHOP]&(b->colormaps[side]);
		while(x) {
			from = LastOne(x);
			mv=a->mvs[from] & pole;
			if(normmark[from]&(~npins)) mv&=attack.rays_dir[b->king[side]][from];
			while (mv) {
				to = LastOne(mv);
				move->move = PackMove(from, to,  ER_PIECE, 0);
				move->qorder=move->real_score= (normmark[to]&utc) ? b->pers->LVAcap[BISHOP][b->pieces[to]&PIECEMASK] : MV_OR+B_OR;
				move++;
				ClrLO(mv);
			}
			ClrLO(x);
		}
// knights
		x=b->maps[KNIGHT]&(b->colormaps[side]);
		while(x) {
			from = LastOne(x);
			mv=a->mvs[from] & pole;
			if(normmark[from]&(~npins)) mv&=attack.rays_dir[b->king[side]][from];
			while (mv) {
				to = LastOne(mv);
				move->move = PackMove(from, to,  ER_PIECE, 0);
				move->qorder=move->real_score= (normmark[to]&utc) ? b->pers->LVAcap[KNIGHT][b->pieces[to]&PIECEMASK] : MV_OR+N_OR;
				move++;
				ClrLO(mv);
			}
			ClrLO(x);
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
					move->qorder=move->real_score=b->pers->LVAcap[PAWN][PAWN];
					move++;
					x=ClrNorm(from,x);
				}
			}
		}
		*m=move;
}

void  generateInCheckMovesN(board * b, const attack_model *a, move_entry ** m, int gen_u)
{
BITVAR at2, at4, utc, mezi, pole, pd1, pd2;
	
int from, to, num, ff, back, orank;
BITVAR x, mv, rank, pmv, brank, npins, block_ray, bran2, piece, epbmp, pins;
move_entry * move;
int ep_add, pie, f;
unsigned char side, opside;
king_eval ke;
personality *p;

		p=b->pers;
		move = *m;
		if(b->side == WHITE) {
			rank=RANK7;
			side=WHITE;
			opside=BLACK;
			brank=RANK2;
			bran2=RANK4;
			orank=0;
			back=0;
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
			back=16;
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
int i,f,n, tc,cc,t,th, sp,pr, op, f1, f2, t1, t2, pm, rr, opside, piece, ff;
int t2t;
move_entry mm[300], *m;
attack_model *a, aa;
char b2[512], b3[512];

	m = mm;
	a=&aa;
// is side to move in check ?

	opside = (b->side == WHITE) ? BLACK:WHITE;
	a->phase=eval_phase(b, b->pers);

	a->att_by_side[WHITE]=KingAvoidSQ(b, a, WHITE);
	a->att_by_side[BLACK]=KingAvoidSQ(b, a, BLACK);

	eval_king_checks_all (b, a);

//	simple_pre_movegen_n2(b, a, WHITE);
//	simple_pre_movegen_n2(b, a, BLACK);

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
//		default:
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
				DEB_0(sprintfMoveSimple(mm[cc].move, b3);)
				LOGGER_0("%d:%d Filter %s vs %s, %x vs %o ", n, cc, b2, b3, th, mm[cc].move);
				if((mm[cc].move&(~CHECKFLAG))==th) {
				  NLOGGER_0("equal\n");
				  mm[i++].move=mm[cc].move;
				} else NLOGGER_0(" ne\n");
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
void SelectBestX(move_cont *mv)
{
move_entry *t, a1, a2, *t2;
//	return;
	t2=mv->next;
	for(t=mv->lastp-1;t>(mv->next+1); t--)
	  if(t->qorder > t2->qorder) t2=t;
		a1=*(mv->next);
		a2=*t2;
		*t2=a1;
		*(mv->next)=a2;
}

void SelectBest(move_cont *mv)
{
move_entry *t, a1, a2, *t2;
//	return;
	
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
int fromPos, ToPos, piece, opside, dist, phase;
//	return;
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
//			if(t->qorder==MV_OR) t->qorder=MV_OR+PSQSearch(fromPos, ToPos, piece, side, eval_phase(b, b->pers), b->pers);
	}
}

//prio2[count2]=PSQSearch(fromPos, ToPos, piece, side, phase, b->pers);


int ExcludeMove(move_cont *mv, MOVESTORE mm){
move_entry *t;
int i;
char b2[256];

//	return 0;
	t=mv->excl;
	while(t<mv->exclp) {
//		sprintfMoveSimple(t->move, b2);
//		LOGGER_0("excluded move %s\n",b2);
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
  LOGGER_0("failed move %s\n",bb);
  printboard(b);
return;
}

int getNextMove(board *b, const attack_model *a, move_cont *mv, int ply, int side, int incheck, move_entry **mm, tree_store *tree){
move_entry *t;
MOVESTORE pot;
int r;
char b2[256];

king_eval ke1, ke2;

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
				
//				sprintfMoveSimple(mv->hash.move, b2);
//				LOGGER_0("HASH move offered %s\n",b2);
				mv->actph=HASHMOVE;
				return ++mv->count;
		} else {
//				LOGGER_0("hash order problem %o\n", mv->hash.move);
		}
	case GENERATE_CAPTURES:
		mv->phase=CAPTUREA;
		mv->next=mv->lastp;
		if(incheck==1) {
				generateInCheckMovesN(b, a, &(mv->lastp), 1);
//				LOGGER_0("InCheck\n");
//				dump_moves(b, mv->move, mv->lastp-mv->move, ply, NULL);
//				SelectBestO(mv);
				goto rest_moves;
		}
		generateCapturesN(b, a, &(mv->lastp), 1);
//				LOGGER_0("Captures\n");
//			dump_moves(b, mv->move, mv->lastp-mv->move, ply, NULL);
		
		mv->tcnt=0;
		mv->actph=CAPTUREA;
	case CAPTUREA:
		while((mv->next<mv->lastp)&&(mv->tcnt>0)) {
			mv->tcnt--;
			if(mv->tcnt==0) mv->phase=SORT_CAPTURES;
			SelectBest(mv);
			if(((mv->next->qorder<A_OR2_MAX)&&(mv->next->qorder>A_OR2)) && (SEE(b, mv->next->move)<0)) {
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
			if(((mv->next->qorder<A_OR2_MAX)&&(mv->next->qorder>A_OR2)) && (SEE(b, mv->next->move)<0)) {
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
//		LOGGER_0("NormalMoves\n");
//		dump_moves(b, mv->next, mv->lastp-mv->next, ply, NULL);
		// get HH values and sort
		ScoreNormal(b, mv, side);
		SelectBestO(mv);
rest_moves:
		mv->phase=NORMAL;
		mv->actph=NORMAL;
	case NORMAL:
		while(mv->next<mv->lastp) {
//			SelectBest(mv);
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
move_entry *t;
MOVESTORE pot;
int r;
char b2[256];

king_eval ke1, ke2;

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
//		LOGGER_0("QuietChecking\n");
//		printBoardNice(b);
//		dump_moves(b, mv->next, mv->lastp-mv->next, ply, NULL);
// get HH values and sort
//		ScoreNormal(b, mv, side);
//		SelectBestO(mv);
rest_moves:
		mv->actph=NORMAL;
		mv->phase=NORMAL;
	case NORMAL:
		while(mv->next<mv->lastp) {
			if((SEE(b, mv->next->move)<0)) {
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
move_entry *t;
MOVESTORE pot;
int r;
char b2[256];

king_eval ke1, ke2;

	switch (mv->phase) {
	case INIT:
	// setup everything
		mv->lastp=mv->move;
		mv->badp=mv->bad;
		mv->exclp=mv->excl;
		mv->count=0;
		mv->phase=PVLINE;
//		mv->lpcheck=((GT_M(b, b->pers, Flip(side), TPIECES, 0)>1));
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
			if(((mv->next->qorder<A_OR2_MAX)&&(mv->next->qorder>A_OR2)) && (SEE(b, mv->next->move)<0)&&mv->lpcheck) {
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
			if(((mv->next->qorder<A_OR2_MAX)&&(mv->next->qorder>A_OR2)) && (SEE(b, mv->next->move)<0)&&mv->lpcheck) {
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
//	default:
	}
return 0;
}

int sortMoveListNew_Init(board *b, attack_model *a, move_cont *mv) {
	mv->phase=INIT;
	mv->hash.move=DRAW_M;
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

int i,s,p, min;
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
	b2[0]='\0';
	if(prom!=ER_PIECE) {
			if(prom==QUEEN) sprintf(b2, "q");
			else if(prom==KNIGHT) sprintf(b2, "n");
			else if(prom==ROOK) sprintf(b2, "r");
			else if(prom==BISHOP) sprintf(b2, "b");
//			else sprintf(b2," ");
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
int from, to, prom, spec, cap, side, cs, cr, tt, mate;

unsigned char pto, pfrom;
char b2[512], b3[512];
int ep_add;
BITVAR aa, map;
		
		ep_add= b->side == WHITE ? +8 : -8;
		buf[0]='\0';
		from=UnPackFrom(m);
		to=UnPackTo(m);
		prom=UnPackProm(m);
//		spec=UnPackSpec(m);
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
//				else if(prom==ER_PIECE+1) ;
//				else if(prom==PAWN) ;
				}

		if((m&CHECKFLAG)&&(mate!=1)) {
			strcat(buf,"+");
		}
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
	
//	return;
	
	LOGGER_0("MOV_DUMP: * Start *\n");
	if(cmt!=NULL) LOGGER_0("MOV_DUMP: Comments %s\n",cmt);
	for(i=0;i<count;i++) {
		sprintfMove(b, m->move, b2);
		LOGGER_0("%*d, MVD , %d: %s %d, %d, %X\n",2+ply, ply, i, b2, m->qorder, m->real_score, m->move);
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
	bwl=(b->mindex%BWD_MI)/BWL_MI;
	bwd=(b->mindex%BBL_MI)/BWD_MI;
	bbl=(b->mindex%BBD_MI)/BBL_MI;
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
//	for(i=0;i<ER_PIECE;i++) if(dest->material[WHITE][i]!=source->material[WHITE][i]) { ret=12; goto konec; }
//	for(i=0;i<ER_PIECE;i++) if(dest->material[BLACK][i]!=source->material[BLACK][i]) { ret=13; goto konec; }
	for(i=0;i<64;i++) if(dest->pieces[i]!=source->pieces[i]) { ret=14; goto konec; }
	if(dest->mindex!=source->mindex)  { ret=15; goto konec; }

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


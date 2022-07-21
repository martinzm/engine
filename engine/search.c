#include "defines.h"
#include "ui.h"
#include "search.h"
#include "evaluate.h"
#include "movgen.h"
#include "generate.h"
#include "attacks.h"
#include "sys/time.h"
#include "hash.h"
#include "utils.h"
#include "pers.h"
#include "openings.h"
#include "globals.h"
#include <stdlib.h>
#include <limits.h>
#include <assert.h>

#define MOVES_RET_MAX 64
#define moves_ret_update(x) if(x<MOVES_RET_MAX-1) moves_ret[x]++; else  moves_ret[MOVES_RET_MAX-2]++

int oldPVcheck;

int DEPPLY=30;

int inPV;
unsigned long long COUNT;

#if 1
int TRIG;
#endif

/*
 * tree->tree[ply][absdep].
 * absdep = ply + depth
 * tree[ply][ply+0] contains info about bestmove from level at ply
 * tree[ply][ply+1] first move analyzed
 * tree[ply][ply+2] second move analyzed 
 */

void store_PV_tree(tree_store * tree, tree_line * pv )
{
	int f;
	pv->score=tree->score=tree->tree_board.bestscore=tree->tree[0][0].score;
	tree->tree_board.bestmove=tree->tree[0][0].move;
//	copyBoard(&tree->tree_board, &pv->tree_board) ;
	
	for(f=0;f<=MAXPLY;f++) {
		pv->line[f]=tree->tree[0][f];
	}
}

void restore_PV_tree(tree_line * pv, tree_store * tree )
{
	int f;
//	copyBoard(&(pv->tree_board),&(tree->tree_board));
	for(f=0;f<=MAXPLY;f++) {
		tree->tree[0][f]=pv->line[f];
	}
}

void copyTree(tree_store * tree, int level)
{
	int f;
	if(level>MAXPLY) {
		LOGGER_0("Error Depth: %d\n", level);
		abort();
	}

	for(f=level+1;f<=MAXPLY;f++) {
		tree->tree[level][f]=tree->tree[level+1][f];
	}
}

void installHashPV(tree_line * pv, board *b, int depth, struct _statistics *s)
{
hashEntry h;
UNDO u[MAXPLY+1];
	int f, l;
	// !!!!
//	depth=999;
// neulozime uplne posledni pozici ???
	f=0;
	l=1;

	while((f<depth) && (l!=0)) {
		l=0;
		switch(pv->line[f].move) {
		case DRAW_M:
		case NA_MOVE:
		case WAS_HASH_MOVE:
		case NULL_MOVE:
		case ALL_NODE:
		case BETA_CUT:
		case MATE_M:
		case ERR_NODE:
			break;
		default:
			h.key=b->key;
			h.map=b->norm;
			h.value=pv->line[f].score;
			h.bestmove=pv->line[f].move;
			storePVHash(b->hs, &h,f, s);


DEB_4(
int from;
int oldp;
		from=UnPackFrom(pv->line[f].move);
		oldp=b->pieces[from]&PIECEMASK;
		
		if((oldp>KING)||(oldp<PAWN)) {
	char buf[256];
			LOGGER_0("Step66 error\n");
			printBoardNice(b);
			printboard(b);
			sprintfMoveSimple(pv->line[f].move, buf);
			LOGGER_0("while making move %s\n", buf);
			LOGGER_0("From %d, old %d\n", from, oldp );
			abort();
		}
)

			u[f]=MakeMove(b, pv->line[f].move);
			l=1;
			break;
		}
		f++;
	}
	if(l==0) f--;
	f--;
	while(f>=0) {
		UnMakeMove(b, u[f]);
		f--;
	}
}

void clearPV(tree_store * tree) {
	int f;
	for(f=0;f<=MAXPLY;f++) {
		tree->tree[0][f].move=NA_MOVE;
		tree->tree[f][f].move=NA_MOVE;
	}
}

/* 
 * PV is always closed with NA_MOVE or some other special move 
 */

 /*
  * Triangular storage for PV
  * tree[ply][ply] contains bestmove at ply
  * tree[ply][ply+N] should contain bestmove N plies deeper for PV from bestmove at ply
  * tree[0][0+..] contains PV from root
  */
 
void sprintfPV(tree_store * tree, int depth, char *buff)
{
	UNDO u[MAXPLY+1];
	int f, s, mi, ply, l;
	char b2[1024];

	buff[0]='\0';
	// !!!!
	depth=MAXPLY+1;
	l=1;
	f=0;
	while((f<=depth)&&(l!=0)) {
		l=0;
		switch(tree->tree[0][f].move&(~CHECKFLAG)) {
		case DRAW_M:
		case NA_MOVE:
		case WAS_HASH_MOVE:
		case ALL_NODE:
		case BETA_CUT:
		case MATE_M:
		case ERR_NODE:
//			sprintfMove(&(tree->tree_board), tree->tree[0][f].move, b2);
//			strcat(buff, b2);
//			strcat(buff," ");
//			f=depth+1;
			break;
		case NULL_MOVE:
		default:
			sprintfMove(&(tree->tree_board), tree->tree[0][f].move, b2);
			strcat(buff, b2);
//			strcat(buff," ");
			if(tree->tree[0][f+1].move!=MATE_M) strcat(buff," ");
			u[f]=MakeMove(&(tree->tree_board),tree->tree[0][f].move);
			l=1;
			break;
		}
		f++;
	}
	if(l==0) f--;
	f--;
	while(f>=0) {
		UnMakeMove(&(tree->tree_board), u[f]);
		f--;
	}

	if(isMATE(tree->tree[0][0].score))  {
		ply=GetMATEDist(tree->tree[0][0].score);
		if (ply==0) mi=1;
		else {
// tree->tree!!!		
			mi= tree->tree_board.side ==WHITE ? (ply+1)/2 : (ply/2)+1;
		}
	} else mi=-1;

	if(mi==-1) sprintf(b2,"EVAL:%d", tree->tree[0][0].score); else {
		if(isMATE(tree->tree[0][0].score)<0) mi=0-mi;
		sprintf (b2,"MATE in:%d", mi);
	}
	strcat(buff, b2);
}

void printPV(tree_store * tree, int depth)
{
char buff[1024];

	buff[0]='\0';
// !!!!
//	printBoardNice(&tree->tree_board);
	sprintfPV(tree, depth, buff);
	LOGGER_3("BeLine: %s\n", buff);

}

void printPV_simple(board *b, tree_store * tree, int depth, int side, struct _statistics * s, struct _statistics * s2)
{
int f, mi, xdepth, ply;
char buff[1024], b2[1024];
unsigned long long int tno;

	buff[0]='\0';
	xdepth=depth;
// !!!!
	xdepth=MAXPLY+1;
	for(f=0; f<=xdepth; f++) {
		switch(tree->tree[0][f].move&(~CHECKFLAG)) {
			case DRAW_M:
			case NA_MOVE:
			case WAS_HASH_MOVE:
			case ALL_NODE:
			case BETA_CUT:
			case MATE_M:
				f=xdepth+1;
				break;
			case NULL_MOVE:
			default:
				sprintfMoveSimple(tree->tree[0][f].move, b2);
				strcat(buff, b2);
				strcat(buff," ");
			break;
		}
	}
/*
 DIST 0 - netazeno
 DIST 1 - prvni tahl
 DIST 2 - tahli oba
 tah je pocitan od bileho

 zacne cerny a jsou dva pultahy ==> dva tahy?

 zacal bily, tah
 0	0

 1	1
 2	1
 3	2
 4	2
 5	3

 (ply+1)/2

 zacal cerny, tah
 0	0

 1	1
 2	2
 3	2
 4	3
 5	3

(ply+2)/2 ; ply!=0

 *
 */

	if(isMATE(tree->tree[0][0].score))  {
		ply=GetMATEDist(tree->tree[0][0].score);
		if (ply==0) mi=1;
		else {
			mi= tree->tree_board.side ==WHITE ? (ply+1)/2 : (ply/2)+1;
		}
	} else mi=-1;

	tno=readClock()-b->run.time_start;
	
	if(mi==-1) {
		sprintf(b2,"info depth %d seldepth %d nodes %lld score cp %d time %lld pv ", depth, s2->depth_max, s2->positionsvisited+s2->qposvisited, tree->tree[0][0].score/10, tno);
		strcat(b2,buff);
	}
	else {
		if(isMATE(tree->tree[0][0].score)<0) mi=0-mi;
		sprintf (b2,"info depth %d seldepth %d nodes %lld score mate %d time %lld pv ", depth, s2->depth_max, s2->positionsvisited+s2->qposvisited, mi, tno);
		strcat(b2,buff);
	}
	tell_to_engine(b2);
	LOGGER_1("%s\n",b2);
	// LOGGER!!!
}

void printPV_simple_act(board *b, tree_store * tree, int depth, int side, struct _statistics * s, struct _statistics * s2)
{
int f, mi, xdepth, ply;
char buff[1024], b2[1024];
unsigned long long int tno;

	strcpy(buff, "Line: ");
	xdepth=depth;
// !!!!
	xdepth=MAXPLY+1;
	for(f=0; f<=xdepth; f++) {
		switch(tree->tree[f][f].move&(~CHECKFLAG)) {
			case DRAW_M:
			case NA_MOVE:
			case WAS_HASH_MOVE:
			case ALL_NODE:
			case BETA_CUT:
			case MATE_M:
				f=xdepth+1;
				break;
			case NULL_MOVE:
			default:
				sprintfMoveSimple(tree->tree[f][f].move, b2);
				strcat(buff, b2);
				if(tree->tree[f][f].move&CHECKFLAG) strcat(buff,"+ "); else strcat(buff," ");
				break;
		}
	}
	LOGGER_0("%s\n",buff);
}

// called inside search
int update_status(board *b){
	long long int tnow, slack, tpsd, nrun, npsd;
	long long int xx, trun;
//	LOGGER_0("Nodes at check %d, mask %d, crit %d\n",b->stats->nodes, b->run.nodes_mask, b->run.time_crit);
	if(b->uci_options->nodes>0) {
		if (b->stats->positionsvisited >= b->uci_options->nodes) engine_stop=2;
		return 0;
	}
	if(b->run.time_crit==0) return 0;
//tnow milisekundy
// movetime je v milisekundach
//
	tnow=readClock();
  	xx=(tnow-b->run.time_start)+1;

	if ((b->run.time_crit <= xx)){
		LOGGER_2("INFO: Time out loop - time_move CRIT, move: %d, crit: %d, mdif %lld, cdif %lld,  %llu, %llu\n", b->run.time_move,b->run.time_crit,xx-b->run.time_move,xx-b->run.time_crit, b->run.time_start, tnow);
		engine_stop=3;
		return 0;
	}
	
#if 0
	if ( b->run.time_move  <= xx ){
		LOGGER_2("INFO: Time out loop - time_move NORM, move: %d, crit: %d, mdif %lld, cdif %lld,  %llu, %llu\n", b->run.time_move,b->run.time_crit,xx-b->run.time_move,b->run.time_crit-xx, b->run.time_start, tnow);
		engine_stop=4;
		return 0;
	}
#endif
	// modify check counter
	tpsd=tnow-b->run.iter_start+1;
	npsd=b->stats->nodes-b->run.nodes_at_iter_start+1;
	nrun=(b->run.time_move-xx)*npsd/(tpsd+1);
	if(nrun<1) {
		engine_stop=5;
		return 0;
	}
	LOGGER_4("infos tpsd %lld, npsd %lld, nrun %lld\n", tpsd, npsd, nrun);
		while(((b->run.nodes_mask+1)*4)<nrun){
			b->run.nodes_mask*=2;
			b->run.nodes_mask++;
		}
		while((b->run.nodes_mask+1)>nrun) {
			b->run.nodes_mask/=2;
		}
		b->run.nodes_mask|=7;
	LOGGER_4("nodes_mask NEW: %lld\n", b->run.nodes_mask);
return 0;
}

// called after iteration
int search_finished(board *b){

unsigned long long tnow, tpsd, npsd;
unsigned long long trun, nrun, xx;

	if (engine_stop) {
		return 9999;
	}

// moznosti ukonceni hledani
// pokud ponder nebo infinite, tak hledame dal
	if((b->uci_options->infinite==1)||(b->uci_options->ponder==1)) {
		return 0;
	}
// mate in X
// depth
// nodes

	if(b->uci_options->nodes>0) {
		if (b->stats->positionsvisited >= b->uci_options->nodes) return 1;
		else return 0;
	}

// time per move
	if(b->run.time_crit==0) {
		return 0;
	}

	tnow=readClock();
	tpsd=tnow-b->run.iter_start+1;
	npsd=b->stats->nodes-b->run.nodes_at_iter_start+1;

	trun=(tnow-b->run.time_start);
	xx=(b->run.time_crit-trun);
	if(b->uci_options->movetime>0) {
		if (b->run.time_crit <= trun) {
			LOGGER_2("Time out - movetime, %d, %llu, %llu, %lld\n", b->uci_options->movetime, b->run.time_start, tnow, (tnow-b->run.time_start));
			return 2;
		}
	} else if ((b->run.time_crit>0)) {
		if (b->run.time_crit <= trun){
			LOGGER_2("Time out CRIT - time_move, %d, %llu, %llu, %lld\n", b->run.time_crit, b->run.time_start, tnow, (tnow-b->run.time_start));
			return 3;
		} else {
			// konzerva
			if(b->uci_options->movestogo==1) return 0;
			if((3*tpsd)>(b->run.time_crit)||((100*xx)<(60*b->run.time_move))) {
				LOGGER_2("Time out RUN - plan: %lld, crit: %lld, iter: %lld, left: %llu, elaps: %lld\n", b->run.time_move, b->run.time_crit, tpsd, xx, (tnow-b->run.time_start));
				return 33;
			}
		}
	}
	
	b->run.iter_start=tnow;
	b->run.nodes_at_iter_start=b->stats->nodes;
	return 0;
}
/*
 * no check
 * hash failed low
 * has no null
 * mat < beta
 * depth >= 2
 * other piece then pawn for side to move
 * 
 */

int can_do_NullMove(board *b, attack_model *a, int alfa, int beta, int depth, int ply, int side){
int pieces, pw;
int sc;

// side to move has only pawns
//	pieces=BitCount(b->colormaps[b->side]&(~b->maps[PAWN]));
//	if(pieces<=2) return 0;

// only few pieces left on the desk
//	pw=BitCount(b->colormaps[b->side]&(b->maps[PAWN]));
	pieces=6*b->material[side][QUEEN]+6*b->material[side][ROOK]+6*b->material[side][KNIGHT]+6*b->material[side][BISHOP]+0*b->material[side][PAWN];
	if(pieces<6) return 0;
	
	return 1;
}

/*
 *  do not reduce when
 *  - remaining depth is too low
 *  - in PVS
 *  - inCheck
 *  - good or neutral capture + promotions
 *  - move gives check (only quiet move others covered by above)
 *  -
 *  funkce je volana po make_move, takze side je strana co udelala tah
 *  b->side je strana na tahu
 */
/*
 * DEPTH klesa do 0, ply roste
 */
 
 /*
  * LMR doesnt reduce captures, hashmove, killers, non captures with good history
  * checks not reduced normally
  */
int can_do_LMR(board *b, attack_model *a, int alfa, int beta, int depth, int ply, int side, move_entry *move)
{
BITVAR inch2;
int8_t from, movp, ToPos;
int prio;

// zakazani LMR - 9999
//	if((depth<b->pers->LMR_remain_depth)) return 0;
//||(move->qorder>=A_OR_N)
	if(((move->qorder>=KILLER_OR)&&(move->qorder<=KILLER_OR_MAX))||(move->qorder==A_QUEEN_PROM)||(move->qorder==A_KNIGHT_PROM)) return 0;
	from=UnPackFrom(move->move);
	movp=b->pieces[from&PIECEMASK];
	if(movp==PAWN) return 0;
	ToPos=UnPackTo(move->move);
	prio=checkHHTable(b->hht, side, movp, ToPos);
	if(prio>0) return 0;
return 1;
}

int position_quality(board *b, attack_model *a, int alfa, int beta, int depth, int ply, int side)
{
BITVAR inch2, x;
int from, movp, p1, p2, pa ,opside;
int q1,q2,cc, mincc, quality, stage;

int threshold[3][4] = { { 1, 2, 2, 4}, { 2, 3, 3, 6}, { 3, 4, 4, 8}};

	quality = 0;
	opside = (side == WHITE) ? BLACK : WHITE;

	p1=b->material[side][KNIGHT]*6+b->material[side][BISHOP]*6+b->material[side][ROOK]*9+b->material[side][QUEEN]*18;
//	p2=b->material[opside][KNIGHT]*6+b->material[opside][BISHOP]*6+b->material[opside][ROOK]*9+b->material[opside][QUEEN]*18;
	pa=p1;
	
	if(pa>30) return 1;
	if(pa>=25) stage=0;
	else if(pa>=20) stage=1;
	else if(pa>=10) stage=2;
	else return 1;
	
// get mobility of side to move for pieces 
// rook
	x = (b->maps[QUEEN]&b->colormaps[side]);
	while (x) {
		from = LastOne(x);
		cc = BitCount(QueenAttacks(b, from)&(~b->norm));
		if(cc< threshold[stage][3]) goto FIN;
		ClrLO(x);
	}
	x = (b->maps[ROOK]&b->colormaps[side]);
	while (x) {
		from = LastOne(x);
		cc = BitCount(RookAttacks(b, from)&(~b->norm));
		if(cc< threshold[stage][2]) goto FIN;
		ClrLO(x);
	}
	x = (b->maps[BISHOP]&b->colormaps[side]);
	while (x) {
		from = LastOne(x);
		cc = BitCount(BishopAttacks(b, from)&(~b->norm));
		if(cc< threshold[stage][1]) goto FIN;
		ClrLO(x);
	}
	x = (b->maps[KNIGHT]&b->colormaps[side]);
	while (x) {
		from = LastOne(x);
		cc = BitCount(KnightAttacks(b, from)&(~b->norm));
		if(cc< threshold[stage][0]) goto FIN;
		ClrLO(x);
	}
//	LOGGER_0("Finish\n");
	quality = 1;
FIN:
return quality;
}

/*
 * Quiescence looks for quiet positions, ie where no checks, no captures etc take place
 *
 */

int QuiesceCheck(board *b, int alfa, int beta, int depth, int ply, int side, tree_store * tree, int checks, const attack_model *tolev)
{
	attack_model *att, ATT;
	move_entry move[300];
	MOVESTORE  bestmove, hashmove;
	hashEntry hash;
	int val,cc, fr, to;
	int depth_idx, sc_need, sc_mat, scf_n;

	move_entry *m, *n;
	int opside;
	int legalmoves, incheck, talfa, tbeta, gmr;
	int best, scr;
	int movlen;
	int tc, isPV, isPVcount;

	int psort;
	int see_res, see_int;
	UNDO u;

	b->stats->nodes++;
	b->stats->qposvisited++;
	if(!(b->stats->nodes & b->run.nodes_mask)){
		update_status(b);
		if(engine_stop!=0) return 0-iINFINITY+1;
	}

	att=&ATT; 
#if 1
	if (is_draw(b, att, b->pers)>0) {
		tree->tree[ply][ply].move=DRAW_M;
		return 0;
	}
#endif

	att->ke[b->side]=tolev->ke[b->side];
	eval(b, att, b->pers);

	scr= (side==WHITE) ? att->sc.complete : 0-att->sc.complete;

	bestmove=tree->tree[ply][ply].move=NA_MOVE;
	if(ply>=MAXPLY) return scr;

	val=best=scr;
	opside = (side == WHITE) ? BLACK : WHITE;
	talfa=alfa;
	tbeta=beta;
	incheck=1;
	isPV= (alfa != (beta-1));
	cc = 0;
	legalmoves=0;
	isPVcount=0;

	m = n = move;
	hashmove=DRAW_M;
	
	simple_pre_movegen(b, att, b->side);

	if(b->stats->depth_max<ply) b->stats->depth_max=ply;
	generateInCheckMovesN(b, att, &m, 0);
	tc=sortMoveList_QInit(b, att, hashmove, move,(int)(m-n), depth, 1 );
	psort=6;
//	getQNSorted(b, move, tc, 0, psort);

	b->stats->qpossiblemoves+=(unsigned int)tc;
	
	while ((cc<tc)&&(engine_stop==0)) {
		b->stats->qmovestested++;
		tree->tree[ply][ply].move=move[cc].move;
		u=MakeMove(b, move[cc].move);
		eval_king_checks(b, &(att->ke[b->side]), NULL, b->side);
		if(isInCheck_Eval(b ,att, b->side)) tree->tree[ply][ply].move = move[cc].move|=CHECKFLAG;
			val = -Quiesce(b, -tbeta, -talfa, depth-1,  ply+1, opside, tree, checks-1, att);
// engine stop protection?
		UnMakeMove(b, u);
		if(engine_stop==0) {
			move[cc].real_score=val;
			legalmoves++;
			if(val>alfa) isPVcount++;
			if(val>best) {
				best=val;
				bestmove=move[cc].move;
				if(val > talfa) {
					talfa=val;
					if(val >= tbeta) {
						if(cc==0) b->stats->qfirstcutoffs++;
						b->stats->qcutoffs++;
						tree->tree[ply][ply+1].move=BETA_CUT;
						break;
					} else copyTree(tree, ply);
				}
			}
			cc++;
		}
	}
// engine stop protection?
	if(engine_stop!=0) goto ESTOP;
	if(legalmoves==0) {
		gmr=GenerateMATESCORE(ply);
		best=0-gmr;
		bestmove=MATE_M;
	}

// restore best
	tree->tree[ply][ply].move=bestmove;
	tree->tree[ply][ply].score=best;

	if(best>=beta) b->stats->failhigh++;
	else {
		if(best<=alfa){
			b->stats->faillow++;
			tree->tree[ply][ply+1].move=ALL_NODE;
		} else b->stats->failnorm++;
  }

	hash.key=b->key;
	hash.depth=(int16_t)depth;
	hash.map=b->norm;
	hash.value=best;
	hash.bestmove=bestmove;
#if 1
	if(best>=beta) {
		hash.scoretype=FAILHIGH_SC;
		if((b->pers->use_ttable==1)&&(depth>0)) storeHash(b->hs, &hash, side, ply, depth, b->stats);
	} else {
		if(best>alfa){
			hash.scoretype=EXACT_SC;
			if((b->pers->use_ttable==1)&&(depth>0)) storeHash(b->hs, &hash, side, ply, depth, b->stats);
		}
	}
#endif
ESTOP:
	return best;
}

int QuiesceCheckN(board *b, int talfa, int tbeta, int depth, int ply, int side, tree_store * tree, int checks, const attack_model *tolev)
{
	move_cont mvs;
	attack_model ATT, *att;

	move_entry *m, mdum = { MATE_M, 0, 0-GenerateMATESCORE(ply) }, *mb;
	int opside = Flip(side);

	UNDO u;
	DEB_4( char b2[256];)

	att=&ATT;
	mb=&mdum;
	tree->tree[ply][ply+1].move=NA_MOVE;
	tree->tree[ply][ply].move=NA_MOVE;
	tree->tree[ply+1][ply+1].move=NA_MOVE;
	
// eval_king_check of side is done on level above, we just copying it
	att->ke[side]=tolev->ke[side];

// generating attacks from opposite to be sure not to move King to check
	att->att_by_side[opside]=KingAvoidSQ(b, att, opside);

	simple_pre_movegen_n2check(b, att, b->side);

	LOGGER_4("%*d, *C , QCQC, amove ch:X, depth %d, talfa %d, tbeta %d, best %d\n", 2+ply, ply, depth, talfa, tbeta, mb->real_score);

	sortMoveListNew_Init(b, att, &mvs);
	while ((getNextMove(b, att, &mvs, ply, side, 1, &m, tree)!=0)&&(engine_stop==0)) {

		tree->tree[ply][ply].move=m->move;
		u=MakeMove(b, m->move);
			
		eval_king_checks(b, &(att->ke[b->side]), NULL, b->side);
		if(isInCheck_Eval(b ,att, b->side))
		  tree->tree[ply][ply].move|=CHECKFLAG;

DEB_4(
		sprintfMoveSimple(m->move, b2);
		LOGGER_0("%*d, +C , %s, amove Xch:%d, depth %d, talfa %d, tbeta %d, best %d, actph %d\n", 2+ply, ply, b2, 0, depth, talfa, tbeta, mb->real_score, mvs.actph);
)

		m->real_score = -QuiesceNew(b, -tbeta, -talfa, depth-1,  ply+1, opside, tree, checks-1, att);
		tree->tree[ply][ply+1].move=NA_MOVE;
		UnMakeMove(b, u);
		LOGGER_4("%*d, -C , %s, amove ch:%d, depth %d, talfa %d, tbeta %d, best %d, val %d\n", 2+ply, ply, b2, 0, depth, talfa, tbeta, mb->real_score, m->real_score);
		if(m->real_score>=tbeta) {
			if(m==mvs.move) b->stats->qfirstcutoffs++;
			b->stats->qcutoffs++;
			b->stats->failhigh++;
			mb=m;
			goto ESTOP;
		}
		if(m->real_score>mb->real_score) {
			mb=m;
			if(mb->real_score>talfa) {
				talfa=mb->real_score;
				copyTree(tree, ply);
			}
		}
	}

// restore best
	tree->tree[ply][ply].score=mb->real_score;
	tree->tree[ply][ply].move=mb->move;
	if(mvs.count==0) goto ESTOP;

		if(mb->real_score<=talfa){
			b->stats->faillow++;
			tree->tree[ply][ply+1].move=ALL_NODE;
		} else b->stats->failnorm++;

ESTOP:
	b->stats->qmovestested+=mvs.count;
	b->stats->qpossiblemoves+=((mvs.lastp-mvs.move));
	return mb->real_score;
}

int Quiesce(board *b, int alfa, int beta, int depth, int ply, int side, tree_store * tree, int checks, const attack_model *tolev)
{
int bonus[] = { 00, 00, 000, 00, 000, 00, 000, 000, 000, 000 };

	attack_model *att, ATT;
	move_entry move[300];
	MOVESTORE  bestmove, hashmove;
	hashEntry  hash;
	int val,cc, fr, to, frf, tof;
	int depth_idx, sc_need, sc_mat, scf_n;
	move_cont mvs;

	move_entry *m, *n;
	int opside;
	int legalmoves, incheck, talfa, tbeta, gmr;
	int best, scr;
	int movlen;
	int tc;

	int psort;
	int see_res, see_int, see_mar;
	int ddeb,gcheck;
	char bx2[256];
	UNDO u;

//	oldPVcheck=2;
	assert(tree->tree[ply-1][ply-1].move!=NA_MOVE);

	b->stats->nodes++;
	b->stats->qposvisited++;
	
	if(!(b->stats->nodes & b->run.nodes_mask)){
		update_status(b);
		if(engine_stop!=0) {
			return 0-iINFINITY;
		}
	}
	
	att=&ATT;
#if 1
	if (is_draw(b, att, b->pers)>0) {
		tree->tree[ply][ply].move=DRAW_M;
		return 0;
	}
#endif
	
	opside = (side == WHITE) ? BLACK : WHITE;
	gmr=GenerateMATESCORE(ply);
#if 1
	// mate distance pruning
		if((gmr) <= alfa) return alfa;
		if(-gmr >= beta) return beta;
#endif
	
	att->ke[side]=tolev->ke[side];
//	att->ke[opside]=tolev->ke[opside];
// eval needs ...

	eval(b, att, b->pers);
// eval updates phase
	scr= (side==WHITE) ? att->sc.complete : 0-att->sc.complete;

	tree->tree[ply][ply].move=NA_MOVE;
	tree->tree[ply+1][ply+1].move=NA_MOVE;
	tree->tree[ply][ply+1].move=NA_MOVE;

	if(b->pers->use_quiesce==0) return scr;
	if(ply>=MAXPLY) return scr;
	if(ply>((ply+depth)*b->pers->quiesce_depth_limit_multi)) return scr;

	val=best=scr;

	talfa=alfa;
	tbeta=beta;
	// is side to move in check ?
	if(isInCheck_Eval(b, att, side)!=0)
		return QuiesceCheck(b, alfa, beta, depth, ply, side, tree, checks, att);
	else {
		incheck=0;
		if(scr>=beta) return scr;
		if(scr>talfa) talfa=scr;
	}

// talfa je bud scr (pro scr>alfa) nebo alfa
// tj talfa-scr je kolik musim minimalne udelat na zvyseni alfa

	bestmove=NA_MOVE;
	legalmoves=0;
	if(b->stats->depth_max<ply) b->stats->depth_max=ply;

	m = n = move;

/*
 * Quiescence should consider
 * hash move
 * good / neutral captures
 * promotions
 * checking moves - generate quiet moves giving check
 * all moves when in check
 *
 */

	hashmove=DRAW_M;
	generateCaptures(b, att, &m, 0);
	tc=sortMoveList_QInit(b, att, hashmove, move,(int)(m-n), depth, 1 );
	psort=3;
	getQNSorted(b, move, tc, 0, psort);

	cc = 0;
	b->stats->qpossiblemoves+=(unsigned int)tc;
	
	while (((cc<tc))&&(engine_stop==0)) {

// check SEE
//		if((move[cc].qorder<=(A_OR2_MAX))&&(move[cc].qorder>=A_OR2)||(move[cc].qorder>=MV_BAD)&&(move[cc].qorder<MV_BAD_MAX)) {
		if((move[cc].qorder<=(A_OR2_MAX))&&(move[cc].qorder>=A_OR2)) {
			see_res=SEE(b, move[cc].move);
			b->stats->qSEE_tests++;
			if(see_res<0) b->stats->qSEE_cuts++;
		} else if((move[cc].qorder>=MV_BAD)&&(move[cc].qorder<MV_BAD_MAX)) see_res=-1;
		  else {
			tof=b->pieces[UnPackTo(move[cc].move) & PIECEMASK];
			to=((tof>=0)&&(tof<6)) ? b->pers->Values[1][tof] : 0;
			see_res=to;
			b->stats->qSEE_tests++;
		}
		if((see_res>=0)) {
			b->stats->qmovestested++;
			tree->tree[ply][ply].move=move[cc].move;
			u=MakeMove(b, move[cc].move);
			eval_king_checks(b, &(att->ke[b->side]), NULL, b->side);
			if(isInCheck_Eval(b ,att, b->side)) move[cc].move=tree->tree[ply][ply].move|=CHECKFLAG;
			val = -Quiesce(b, -tbeta, -talfa, depth-1,  ply+1, opside, tree, checks-1, att);
// engine stop protection?
			UnMakeMove(b, u);
			if(engine_stop==0) {
				move[cc].real_score=val;
				legalmoves++;
				if(val>best) {
					best=val;
					if(val > talfa) {
						talfa=val;
						if(val >= tbeta) {
							if(cc==0) b->stats->qfirstcutoffs++;
							b->stats->qcutoffs++;
							tree->tree[ply][ply+1].move=BETA_CUT;
							break;
						} else copyTree(tree, ply);
					}
				}
			}
		}
//		psort--;
		cc++;
	}

// generate checks

#if 1
	if((incheck==0) && (checks>0) && (val<tbeta)&&(engine_stop==0)&&(legalmoves==0)) {
		n=m;
		generateQuietCheckMoves(b, att, &m);
		tc=sortMoveList_QInit(b, att, hashmove, n, (int)(m-n), depth, 1 );
		psort=3;
		getQNSorted(b, n, tc, 0, psort);

		cc = 0;
		b->stats->qpossiblemoves+=(unsigned int)tc;

		while ((cc<tc)&&(engine_stop==0)) {
			see_res=SEE(b, n[cc].move);
			b->stats->qSEE_tests++;
			if(see_res<0) b->stats->qSEE_cuts++;
			else {
				b->stats->qmovestested++;
				tree->tree[ply][ply].move=n[cc].move;
				u=MakeMove(b, n[cc].move);
				eval_king_checks(b, &(att->ke[b->side]), NULL, b->side);
				tree->tree[ply][ply].move=(n[cc].move|=CHECKFLAG);
					val = -QuiesceCheck(b, -tbeta, -talfa, depth-1,  ply+1, opside, tree, checks-1, att);
					b->stats->zerototal++;
// engine stop protection?
				UnMakeMove(b, u);
				if(engine_stop==0){
					n[cc].real_score=val;
					legalmoves++;
					if(val>best) {
						best=val;
						bestmove=n[cc].move;
						if(val > talfa) {
							talfa=val;
							if(val >= tbeta) {
								if(cc==0) b->stats->qfirstcutoffs++;
								b->stats->qcutoffs++;
								tree->tree[ply][ply+1].move=BETA_CUT;
								break;
							} else copyTree(tree, ply);
						}
					}
				}
			}
		cc++;
		}
	}
#endif
	if(engine_stop!=0) goto ESTOP;
// restore best
	tree->tree[ply][ply].move=bestmove;
	tree->tree[ply][ply].score=best;

	if(best>=beta) b->stats->failhigh++;
	else {
		if(best<=alfa){
			b->stats->faillow++;
			tree->tree[ply][ply+1].move=ALL_NODE;
		} else b->stats->failnorm++;
	}

	hash.key=b->key;
	hash.depth=(int16_t)depth;
	hash.map=b->norm;
	hash.value=best;
	hash.bestmove=bestmove;
#if 1
	if(best>=beta) {
		hash.scoretype=FAILHIGH_SC;
		if((b->pers->use_ttable==1)&&(depth>0)) storeHash(b->hs, &hash, side, ply, depth, b->stats);
	} else {
		if(best<=alfa){
		} else {
			hash.scoretype=EXACT_SC;
			if((b->pers->use_ttable==1)&&(depth>0)) {
				storeHash(b->hs, &hash, side, ply, depth, b->stats);
//				storeExactPV(b->hs, b->key, b->norm, tree, ply);
			}
		}
	}
#endif	
ESTOP:
  	return best;
}

int QuiesceNew(board *b, int alfa, int beta, int depth, int ply, int side, tree_store * tree, int checks, const attack_model *tolev)
{
	move_cont mvs;
	move_entry *m, mdum = { MATE_M, 0, 0-GenerateMATESCORE(ply) }, *mb;
	attack_model *att, ATT;
	BITVAR tmp;

	int opside, scr, f;
	int incheck, talfa, tbeta, gmr, aftermcheck;
	UNDO u;
	DEB_4( char b2[256]; )
	
	b->stats->nodes++;
	b->stats->qposvisited++;
	tree->tree[ply][ply+1].move=NA_MOVE;
	tree->tree[ply][ply].move=NA_MOVE;
	tree->tree[ply+1][ply+1].move=NA_MOVE;
	
	LOGGER_4("%*d, *Q , EEEE, amove ch:X, depth %d, alfa %d, beta %d\n", 2+ply, ply, depth, alfa, beta);
	
	if(!(b->stats->nodes & b->run.nodes_mask)){
		update_status(b);
		if(engine_stop!=0) {
			return 0;
		}
	}
	if(ply>=MAXPLY-1) return beta;
	if(b->stats->depth_max<ply) b->stats->depth_max=ply;
	

	opside = Flip(side);
	scr=gmr=-mdum.real_score;
	
	// mate distance pruning
	if((gmr) <= alfa) return alfa;
	if(-gmr >= beta) return beta;
	
	mb=&mdum;
	att=&ATT;
	
	att->ke[side]=tolev->ke[side];
	att->att_by_side[opside]=KingAvoidSQ(b, att, opside);

	scr=lazyEval(b, att, alfa, beta, side, ply, depth, b->pers);

	if(scr>=beta) return scr;
	mb->real_score=scr;
	talfa = scr> alfa ? scr : alfa;
	
	if((b->pers->use_quiesce==0) || (ply>=MAXPLY) ||
		(ply>((b->depth_run*(b->pers->quiesce_depth_limit_multi+10))/10))) {
		tree->tree[ply][ply].move=NA_MOVE;
		return scr;
	}
	
	if(checks>0)
		if (is_draw(b, att, b->pers)>0) {
			tree->tree[ply][ply].move=DRAW_M;
			tree->tree[ply][ply].move=NA_MOVE;
			return 0;
		}


	incheck = (UnPackCheck(tree->tree[ply-1][ply-1].move)!=0);
	if((incheck)&&(checks>0)) return QuiesceCheckN(b, alfa, beta, depth, ply, side, tree, checks, tolev);
	
	tbeta=beta;
//	if(incheck) simple_pre_movegen_n2check(b, att, b->side);
//	else simple_pre_movegen_n2(b, att, b->side);
	simple_pre_movegen_n2(b, att, b->side);
	
// check for king capture & for incheck solution
// verify we have some moves available while in check, find if any move hits other king
	tmp=0;
	for(f=A1;f<ER_SQUARE;f++) tmp|=att->mvs[f];
	if(tmp&normmark[b->king[opside]]) {
		LOGGER_4("%*d, *Q , KING, amove ch:X, depth %d, talfa %d, tbeta %d\n", 2+ply, ply, depth, talfa, tbeta);
		return gmr;
	}
//I have no move to avoid mate. Why beta works best, -gmr would be logical return value?
	if(incheck) {
		simple_pre_movegen_n2check(b, att, b->side);
		tmp=0;
		for(f=A1;f<ER_SQUARE;f++) tmp|=att->mvs[f];
		if(tmp==0) return -gmr;
	}

	LOGGER_4("%*d, *Q , QQQQ, amove ch:X, depth %d, talfa %d, tbeta %d, best %d\n", 2+ply, ply, depth, talfa, tbeta, mb->real_score);
	
	sortMoveListNew_Init(b, att, &mvs);
	while ((getNextCap(b, att, &mvs, ply, side, incheck, &m, tree)!=0)&&(engine_stop==0)) {

		tree->tree[ply][ply].move=m->move;

		u=MakeMove(b, m->move);
		eval_king_checks(b, &(att->ke[b->side]), NULL, b->side);
		if(isInCheck_Eval(b, att, b->side)) {
			tree->tree[ply][ply].move|=CHECKFLAG;
			aftermcheck=1;
		}else aftermcheck=0;

DEB_4(
		sprintfMoveSimple(m->move, b2);
		LOGGER_0("%*d, +Q , %s, amove ch:%d, depth %d, talfa %d, tbeta %d, best %d\n", 2+ply, ply, b2, aftermcheck, depth, talfa, tbeta, mb->real_score);
)

		if((checks<=0)||(aftermcheck==0)) {
			m->real_score = -QuiesceNew(b, -tbeta, -talfa, depth-1,  ply+1, opside, tree, checks-1, att);
//			  tree->tree[ply+1][ply+1].move=ALL_NODE;
		}
		else {
			if(incheck==0) {
				m->real_score = -QuiesceCheckN(b, -tbeta, -talfa, depth-1,  ply+1, opside, tree, checks-1, att);
			}
			else {
			  m->real_score=scr;
			}
		}

		UnMakeMove(b, u);

		LOGGER_4("%*d, -Q , %s, amove ch:%d, depth %d, talfa %d, tbeta %d, best %d, val %d\n", 2+ply, ply, b2, aftermcheck, depth, talfa, tbeta, mb->real_score, m->real_score);
		if(m->real_score>=tbeta) {
			if(m==mvs.move) b->stats->qfirstcutoffs++;
			b->stats->qcutoffs++;
			b->stats->failhigh++;
			mb=m;
			goto ESTOP;
		}
		if(m->real_score>mb->real_score) {
			mb=m;
			if(mb->real_score>talfa) {
				talfa=mb->real_score;
				copyTree(tree, ply);
			}
		}
	}

#if 1
// generate checks
	if((checks>0) && (mb->real_score<=talfa)&&(engine_stop==0)&&(incheck==0)) {

		b->stats->qmovestested+=mvs.count;
		sortMoveListNew_Init(b, att, &mvs);
		while ((getNextCheckin(b, att, &mvs, ply, side, incheck, &m, tree)!=0)&&(engine_stop==0)) {

			tree->tree[ply][ply].move=m->move;
			u=MakeMove(b, m->move);

DEB_4(
			sprintfMoveSimple(m->move, b2);
			LOGGER_0("%*d, +G , %s, amove ch:%d, depth %d, talfa %d, tbeta %d, best %d\n", 2+ply, ply, b2, 1, depth, talfa, tbeta, mb->real_score);
)

			eval_king_checks(b, &(att->ke[b->side]), NULL, b->side);
			tree->tree[ply][ply].move|=CHECKFLAG;
			tree->tree[ply][ply+1].move=NA_MOVE;
			m->real_score = -QuiesceCheckN(b, -tbeta, -talfa, depth-1,  ply+1, opside, tree, checks-1, att);
			UnMakeMove(b, u);

// compensation for QuiesceCheckN where we do not count nodes
			b->stats->nodes++;
			b->stats->qposvisited++;
			if(!(b->stats->nodes & b->run.nodes_mask)){
				update_status(b);
				if(engine_stop!=0) {
					return 0;
				}
			}

			LOGGER_4("%*d, -G , %s, amove ch:%d, depth %d, talfa %d, tbeta %d, best %d, val %d\n", 2+ply, ply, b2, 1, depth, talfa, tbeta, mb->real_score, m->real_score);
			if(m->real_score>=tbeta) {
				b->stats->qcutoffs++;
				b->stats->failhigh++;
				mb=m;
				goto ESTOP;
			}
			if(m->real_score>mb->real_score) {
				mb=m;
				if(mb->real_score>talfa) {
					talfa=mb->real_score;
					copyTree(tree, ply);
				}
			}
		}
	}
#endif

	if(engine_stop!=0) {
		mb->real_score=0;
		goto ESTOP;
	}

// restore best
	tree->tree[ply][ply].score=mb->real_score;
	tree->tree[ply][ply].move=mb->move;

	if(mb->real_score<=alfa){
		b->stats->faillow++;
		tree->tree[ply][ply+1].move=ALL_NODE;
	} else b->stats->failnorm++;

ESTOP:
	b->stats->qmovestested+=mvs.count;
	b->stats->qpossiblemoves+=((mvs.lastp-mvs.move));
	return mb->real_score;
}

int SearchMove(board *b, int talfa, int tbeta, int ttbeta, int depth, int ply, int extend, int reduce, int side, tree_store * tree, int nulls, attack_model *att)
{
int val, ext;
int isPV;
int opside= side == WHITE ? BLACK : WHITE;

	isPV= (talfa != (ttbeta-1));
	b->stats->zerototal+=(1-isPV);
	ext=depth-reduce+extend-1;
	val=0;
	if(((ext >= 0)&&(ply<MAXPLY))) val = -AlphaBeta(b, -(ttbeta), -talfa, ext,  ply+1, opside, tree, nulls, att);
	else val = -Quiesce(b, -(ttbeta), -talfa, ext,  ply+1, opside, tree, b->pers->quiesce_check_depth_limit, att);
	if((val>talfa && val < tbeta && ttbeta<tbeta) && (engine_stop==0)) {
		b->stats->zerorerun++;
		if(ext >= 0) val = -AlphaBeta(b, -tbeta, -talfa, ext+reduce,  ply+1, opside, tree, nulls, att);
		else val = -Quiesce(b, -tbeta, -talfa, ext+reduce,  ply+1, opside, tree, b->pers->quiesce_check_depth_limit, att);
		if(val<=talfa) b->stats->fhflcount++;
		if(reduce>0) b->stats->lmrrerun++;
	}
return val;
}

int SearchMoveNew(board *b, int talfa, int tbeta, int ttbeta, int depth, int ply, int extend, int reduce, int side, tree_store * tree, int nulls, const attack_model *att)
{
int val, ext;
int isPV;
int opside= Flip(side);

	isPV= (talfa != (ttbeta-1));
	b->stats->zerototal+=(1-isPV);
	ext=depth-reduce+extend-1;
	val=0;
	if(((ext > 0)&&(ply<MAXPLY))) {
		val = -ABNew(b, -(ttbeta), -talfa, ext,  ply+1, opside, tree, nulls, att);
		if((val>talfa)&&(reduce>extend)) val = -ABNew(b, -(ttbeta), -talfa, depth+extend-1,  ply+1, opside, tree, nulls, att);
	}
	else val = -QuiesceNew(b, -(ttbeta), -talfa, ext,  ply+1, opside, tree, b->pers->quiesce_check_depth_limit, att);
	if((val>talfa && val < tbeta && ttbeta<tbeta) && (engine_stop==0)) {
		ext=depth+extend-1;
		b->stats->zerorerun++;
		if(ext > 0) val = -ABNew(b, -tbeta, -talfa, ext,  ply+1, opside, tree, nulls, att);
		else val = -QuiesceNew(b, -tbeta, -talfa, ext,  ply+1, opside, tree, b->pers->quiesce_check_depth_limit, att);
		if(val<=talfa) b->stats->fhflcount++;
		if(reduce>0) b->stats->lmrrerun++;
	}
return val;
}

int BoardTrigger(char *m, board *b){
	LOGGER_0("Trigger %s board corrupted\n", m);
	printBoardNice(b);
	printboard(b);
	abort();
}

int NBoardTrigger(char *m, board *b){
	LOGGER_0("Trigger %s\n", m);
return 0;
}

#define BCHECK(i,b) if(b->colormaps[WHITE]&b->colormaps[BLACK]) BoardTrigger(i,b); else { LOGGER_0("Ps %d: %s\n", ply, i);}
#define NBCHECK(i,b) if(!(b->colormaps[WHITE]&b->colormaps[BLACK])) NBoardTrigger(i,b);

/*
 * FailSoft - patricne se upravuje AlfaBeta okno, ale vraci se vypocitana hodnota i kdyz je mimo okno
 * FailHard - upravuje se AlfaBeta okno a vraci se vypocitana hodnota nebo hranice, pokud je hodnota mimo okno
 
 * terminologie
 * PV-node - score je uvnitr hranic, vracena hodnota je exaktni, score S je mezi A a B
 * Cut-nodes / fail-high node - proveden beta-cutoff, plati S>=B, cili S neni exaktnim ohodnocenim,
 * ale je spodni hranici hledaneho ohodnoceni
 * All-nodes / fail-low node - nic nezlepsilo A, score S<=A, S je horni hranici hledaneho
 *
 * v hash typ
 * 0 - N/A
 * 1 - AllNodes - znamena (vsechny moznosti prolezeny) a tohle je maximum, a v hledani to neprekrocilo alfa
 * 2 - Exact - presne cislo a v danem hledani se to trefilo mezi alfa - beta
 * 3 - BCutoff - v danem hledani tohle prekrocilo beta, realna hodnota muze byt jeste vyssi
 *
 * FAILLOW_SC znamena, ze v dane pozici nic neprekrocilo ALFA, byly vyhodnoceny vsechny moznosti a dana hodnota
 * je Horni hranici Score dane pozice, ktere muze byt nizsi, tedy UPPER BOUND, node je AllNodes/fail-low
 * plati pouze v prvni iteraci 
 *
 * FAILHIGH_SC znamena, ze v dane pozici doslo k Beta-CutOff (prekroceni BETA),
 * uvedena hodnota je Spodni hranici Score pro danou pozici, ktere muze byt vyssi, tedy LOWER BOUND, node je Cut-node/failhigh
 *
 * upravovat Alfa - hodnota ktere urcite mohu dosahnout
 * upravovat Beta - hodnota kterou kdyz prekrocim tak si o uroven vyse tah vedouci do teto pozice nevyberou
 * udrzovat aktualni hodnotu nezavisle na A a B
 
 * - best - zatim nejvyssi hodnota
 * - bestmove - odpovidajici tah
 * - val - hodnota prave spocitaneho tahu
 */

int AlphaBeta(board *b, int alfa, int beta, int depth, int ply, int side, tree_store * tree, int nulls, attack_model *tolev)
// depth - jak hluboko mam jit, 0 znamena pouze evaluaci pozice, zadne dalsi pultahy
// ply - jak jsem hluboko, 0 jsem v root pozici
{
	int tc,cc, xcc, sc, qual;
	move_entry move[300];
	MOVESTORE bestmove, hashmove;
	move_entry *m, *n;
	move_cont mvs;
	int opside, isPV, isPVcount;
	int val, legalmoves, incheck, best, talfa, tbeta, ttbeta, gmr, aftermovecheck, valn, cutn, incheck2;
	int reduce, extend, ext;
	int reduce_o, extend_o;
	int see_res;
	struct _statistics s, r;
	unsigned long long nodes_stat, null_stat;
	hashEntry hash;
	char b2[256];

	int psort;
	int sortstate=0;
	UNDO u;
	attack_model *att, ATT;

	assert(tree->tree[ply-1][ply-1].move!=NA_MOVE);

	b->stats->nodes++;
	b->stats->positionsvisited++;
	
	if(b->pers->negamax==0) {
	// nechceme AB search, ale klasicky minimax
		alfa=0-iINFINITY;
		beta=iINFINITY;
	}

	isPV= (alfa != (beta-1));
	isPVcount=0;
	best=0-iINFINITY+2;
	bestmove=NA_MOVE;

	if(!(b->stats->nodes & b->run.nodes_mask)){
		update_status(b);
		if(engine_stop!=0) {
			goto ABFINISH;
		}
	}

	opside = (side == WHITE) ? BLACK : WHITE;

	CopySearchCnt(&s, b->stats);
// inicializuj zvazovany tah na NA
	tree->tree[ply][ply].move=NA_MOVE;
	tree->tree[ply+1][ply+1].move=NA_MOVE;
	tree->tree[ply][ply+1].move=NA_MOVE;
	
	att=&ATT;
	att->phase=eval_phase(b,b->pers);

	att->ke[b->side]=tolev->ke[b->side];
	
	if (is_draw(b, att, b->pers)>0) {
		tree->tree[ply][ply].move=DRAW_M;
		best=0;
		if(best<=alfa) b->stats->faillow++;
		else if(best>=beta) b->stats->failhigh++;
		else b->stats->failnorm++;
		goto ABFINISH;
	}

	gmr=GenerateMATESCORE(ply);

DEB_3(
	// is opposite side in check ?
	eval_king_checks(b, &(att->ke[opside]), NULL, opside);
	if(isInCheck_Eval(b, att, opside)!=0) {
		tree->tree[ply][ply].move=MATE_M;
		LOGGER_1("ERR: Opside in check3!\n");
		printBoardNice(b);
		printPV(tree,ply);
		best=gmr;
		goto ABFINISH;
	}
)
 
	
// mate distance pruning

	talfa=alfa;
	tbeta=beta;
#if 1
	switch(isMATE(alfa)) {
	case -1:
		if((0-gmr)>=tbeta) {
			best= 0-gmr;
			b->stats->failhigh++;
			goto ABFINISH;
		}
		if(talfa<(0-gmr)) talfa=0-gmr;
		break;
	case 1:
		if((gmr-1)<=talfa) {
			best= gmr-1;
			b->stats->faillow++;
			goto ABFINISH;
		}
		if((gmr)<tbeta) tbeta=gmr;
		break;
	default:
		break;
	}

#endif 
	hashmove=DRAW_M;
// time to check hash table
// TT CUT off?
	if(b->pers->use_ttable==1) {
		hash.key=b->key;
		hash.map=b->norm;
		hash.scoretype=NO_NULL;
		if(retrieveHash(b->hs, &hash, side, ply, depth, b->pers->use_ttable_prev, b->stats)!=0) {
			hashmove=hash.bestmove;
			if((hashmove==NULL_MOVE)||(isMoveValid(b, hashmove, att, side, tree))) {
/*
 * FAILLOW_SC - uz jsem vsechny tahy nekdy prosel a nedostal jsem se pres uvedenou hodnotu - vice to nemuze nikdy byt
 * FAILHIGH_SC - minimalne toto skore dana pozice ma
 * EXACT_SC - presne toto skore ma pozice
 */
				if((hash.depth>=depth)) {
					if((hash.scoretype!=FAILLOW_SC)&&(hash.value>=tbeta)) {
						b->stats->failhigh++;
						b->stats->failhashhigh++;
						tree->tree[ply][ply].move=hash.bestmove;
						tree->tree[ply][ply].score=hash.value;
						best=hash.value;
						goto ABFINISH;
					}
					if((hash.scoretype!=FAILHIGH_SC)&&(hash.value<=talfa)){
						b->stats->faillow++;
						b->stats->failhashlow++;
						tree->tree[ply][ply].move=hash.bestmove;
						tree->tree[ply][ply].score=hash.value;
						best=hash.value;
						goto ABFINISH;
					}
					if(hash.scoretype==EXACT_SC) {
						b->stats->failhashnorm++;
						if(b->pers->use_hash) {
							tree->tree[ply][ply].move=hash.bestmove;
							tree->tree[ply][ply].score=hash.value;
							best=hash.value;
							restoreExactPV(b->hs, b->key, b->norm, ply, tree);
							copyTree(tree, ply);
							b->stats->failnorm++;
							goto ABFINISH;
						}
					}
				} else {
// not enough depth
				if((b->pers->NMP_allowed>0) && (hash.scoretype!=FAILHIGH_SC)&&(hash.depth>= (depth - b->pers->NMP_reduction - 1))
					&& (hash.value<beta)) nulls=0;
				}
			} else {
//			  printBoardNice(b);
//			  sprintfMoveSimple(hashmove, b2);
//			  LOGGER_0("Invalid hash move %s\n", b2);
			}
		} else {
// no TT hit
		}
	}

	incheck = (UnPackCheck(tree->tree[ply-1][ply-1].move)!=0);
	reduce_o=extend_o=cutn=valn=val=0;

// null move PRUNING / REDUCING
	if((nulls>0) && (isPV==0) && (b->pers->NMP_allowed>0) && (incheck==0) && (can_do_NullMove(b, att, talfa, tbeta, depth, ply, side)!=0)&&(depth>=b->pers->NMP_min_depth)) {
		tree->tree[ply][ply].move=NULL_MOVE;
		u=MakeNullMove(b);
		eval_king_checks(b, &(att->ke[b->side]), NULL, b->side);
		b->stats->NMP_tries++;
		reduce=b->pers->NMP_reduction;
		ext=depth-reduce-1;
// save stats, to get info how many nodes were visited due to NULL move...
		nodes_stat=b->stats->nodes;
		null_stat=b->stats->u_nullnodes;
		if((ext)>=0) val = -AlphaBeta(b, -tbeta, -tbeta+1, ext, ply+1, opside, tree, nulls-1, att);
		else val = -Quiesce(b, -tbeta, -tbeta+1, ext,  ply+1, opside, tree, b->pers->quiesce_check_depth_limit, att);

// update null nodes statistics
		UnMakeNullMove(b, u);
// engine stop protection?
		if(engine_stop!=0) goto ABFINISH;
		b->stats->u_nullnodes=null_stat+(b->stats->nodes-nodes_stat);
		if(val>=tbeta) {
			tree->tree[ply][ply].score=val;
			b->stats->NMP_cuts++;
			hash.key=b->key;
			hash.depth=(int16_t)depth;
			hash.map=b->norm;
			hash.value=tbeta;
			hash.bestmove=NULL_MOVE;
			hash.scoretype=FAILHIGH_SC;
			if((b->pers->use_ttable==1)&&(engine_stop==0)) storeHash(b->hs, &hash, side, ply, depth, b->stats);
			if(b->pers->NMP_search_reduction==0) {
				best=val;
				b->stats->failhigh++;
				goto ABFINISH;
			} else if(b->pers->NMP_search_reduction==-1) {
				reduce_o=0;
				hashmove=DRAW_M;
			} else reduce_o=b->pers->NMP_search_reduction;
			cutn=1;
			valn=val;
		}
	} else if((nulls<=0) && (b->pers->NMP_allowed>0)) nulls=b->pers->NMP_allowed;

// generate bitmaps for movegen
	simple_pre_movegen(b, att, b->side);
	simple_pre_movegen(b, att, opside);
	
		
	if(hashmove==DRAW_M) {
// no hash, if we are deep enough and not in zero window, try IID
// IID, vypnout - 9999
		if((depth>=b->pers->IID_remain_depth) && (isPV)&&(b->pers->use_ttable==1)) {
			val = AlphaBeta(b, talfa, tbeta, depth-2,  ply, side, tree, nulls, att);
			if(engine_stop!=0) goto ABFINISH;
				// still no hash?, try everything!
				// engine_stop protection?
			if(val < talfa) {
				val = AlphaBeta(b, -iINFINITY, tbeta, depth-2,  ply, side, tree, nulls, att);
				if(engine_stop!=0) goto ABFINISH;
			}
			// engine_stop protection
			if(retrieveHash(b->hs, &hash, side, ply, depth, b->pers->use_ttable_prev, b->stats)!=0)
				hashmove=hash.bestmove;
			else hashmove=DRAW_M;
		}
	}

// try to judge on position and reduce / quit move searching
// sort of forward pruning / forward reducing

	if((incheck!=1)&&(b->pers->quality_search_reduction!=0)&&(ply>4)) {
		qual=position_quality(b, att, talfa, tbeta, depth, ply, side);
		b->stats->position_quality_tests++;
		if(qual==0) {
			b->stats->position_quality_cutoffs++;
			if(b->pers->quality_search_reduction==-1) {
					tree->tree[ply][ply].move=FAILLOW_SC;
					tree->tree[ply][ply].score=-iINFINITY;
					return alfa;
			} else reduce_o+=b->pers->quality_search_reduction;
		}
	}
		
	tree->tree[ply][ply].move=NA_MOVE;

// generate moves

	legalmoves=0;
	m = n = move;
	if(incheck==1) {
		generateInCheckMoves(b, att, &m);
		tc=sortMoveList_Init(b, att, hashmove, move, (int)(m-n), depth, 1 );
	} else {
		generateCaptures(b, att, &m, 1);
		generateMoves(b, att, &m);
		tc=sortMoveList_Init(b, att, hashmove, move, (int)(m-n), depth, 1 );
	}
	b->stats->poswithmove++;
	if(tc==1) extend_o++;
	cc = 0;
	b->stats->possiblemoves+=(unsigned int)tc;

// main loop

	psort=0;
	while ((cc<tc)&&(engine_stop==0)) {
		extend=extend_o;
		reduce=reduce_o;
		if(psort==0) {
			psort=2;
			getNSorted(b, move, tc, cc, psort, &sortstate);
		}
		b->stats->movestested++;
		tree->tree[ply][ply].move=move[cc].move;
		u=MakeMove(b, move[cc].move);

// vloz tah ktery aktualne zvazujeme - na vystupu z funkce je potreba nastavit na BESTMOVE!!!
// proto abychom v kazdem okamziku meli konzistetni prave pocitanou variantu od roota

// setup what to do with the move played

// is side to move in check, remember it and extend depth by one
		eval_king_checks(b, &(att->ke[b->side]), NULL, b->side);
		if(isInCheck_Eval(b ,att, b->side)) {
			extend+=b->pers->check_extension;
			move[cc].move|=CHECKFLAG;
			tree->tree[ply][ply].move|=CHECKFLAG;
			aftermovecheck=1;
		} else aftermovecheck=0;

// setup window
	ttbeta = ((isPVcount<b->pers->PVS_full_moves)&&isPV) ? tbeta : talfa+1;
// setup reductions
  if(cc>=b->pers->LMR_start_move && 
	(b->pers->LMR_reduction>0) &&
	(depth>=b->pers->LMR_remain_depth) && 
	(incheck==0) && (aftermovecheck==0) &&
	(extend==extend_o) && 
	can_do_LMR(b, att, talfa, ttbeta, depth, ply, side, &(move[cc]))) {
		if(cc>=b->pers->LMR_prog_start_move) reduce += div(depth, b->pers->LMR_prog_mod).quot;
		reduce +=b->pers->LMR_reduction;
		b->stats->lmrtotal++;
  }

//		ext=depth-reduce+extend-1;
		val=SearchMove(b, talfa, tbeta, ttbeta, depth, ply, extend, reduce, side, tree, nulls, att);
		UnMakeMove(b, u);
		
		if(engine_stop!=0) goto ABFINISH;
		move[cc].real_score=val;
		legalmoves++;

		if(val>alfa) isPVcount++;
		if((val>best)) {
			best=val;
			bestmove=move[cc].move;
			if(val > talfa) {
				talfa=val;
				if(val >= tbeta) {
// cutoff
					if(cc==0) b->stats->firstcutoffs++;
					b->stats->cutoffs++;
// record killer
					if((b->pers->use_killer>=1)&&(is_quiet_move(b, att, &(move[cc])))) {
						update_killer_move(b->kmove, ply, move[cc].move, b->stats);
// update history tables
						updateHHTable(b, b->hht, move, cc, side, depth, ply);
					}
					tree->tree[ply][ply+1].move=BETA_CUT;
					break;
				} else copyTree(tree, ply);
			}
		}
		psort--;
		cc++;
	}
	
	if(legalmoves==0) {
		if(incheck==0) {
			best=0;
			bestmove=DRAW_M;
		}	else 	{
			// I was mated! So best is big negative number...
			best=0-gmr;
			bestmove=MATE_M;
		}
		goto ABFINISH;
	}
	if(bestmove==NA_MOVE) best=alfa;
	
	assert( best > -iINFINITY );
	
	tree->tree[ply][ply].move=bestmove;
	tree->tree[ply][ply].score=best;

		// update stats & store Hash

	hash.key=b->key;
	hash.depth=(int16_t)depth;
	hash.map=b->norm;
	hash.value=best;
	hash.bestmove=bestmove;
	if(best>=beta) {
		b->stats->failhigh++;
		hash.scoretype=FAILHIGH_SC;
		if((b->pers->use_ttable==1)&&(depth>0)) storeHash(b->hs, &hash, side, ply, depth, b->stats);
	} else {
		if(best<=alfa){
			b->stats->faillow++;
			hash.scoretype=FAILLOW_SC;
			if((b->pers->use_ttable==1)&&(depth>0)) storeHash(b->hs, &hash, side, ply, depth, b->stats);
// proc je PV ukoncovana tady???
//			tree->tree[ply][ply+1].move=WAS_HASH_MOVE;
		} else {
			b->stats->failnorm++;
			hash.scoretype=EXACT_SC;
//			if((b->pers->use_ttable==1)&&(b->pers->use_hash==1)&&(depth>0)&&(engine_stop==0)) {
			if((b->pers->use_ttable==1)&&(depth>0)) {
				storeHash(b->hs, &hash, side, ply, depth, b->stats);
// and store PV from this position
				storeExactPV(b->hs, b->key, b->norm, tree, ply);
			}
		}
	}
	assert((cutn==0) ? 1:(valn>=beta));
ABFINISH:
	DecSearchCnt(b->stats, &s, &r);
	return best; //!!!
}

/*********
 *
 *
 */


int ABNew(board *b, int alfa, int beta, int depth, int ply, int side, tree_store * tree, int nulls, const attack_model *tolev)
// depth - jak hluboko mam jit, 0 znamena pouze evaluaci pozice, zadne dalsi pultahy
// ply - jak jsem hluboko, 0 jsem v root pozici
{
	int qual;
	move_entry *m, mdum = { MATE_M, 0, 0-GenerateMATESCORE(ply) }, *mb, mt;
	MOVESTORE hashmove;
	move_cont mvs;
	int opside;
	int isPV = (alfa != (beta-1));
	int pval, sval;
	int incheck, talfa, tbeta, ttbeta, gmr, aftermovecheck;
	int reduce, extend, ext;
	int reduce_o, extend_o;
//	struct _statistics s, r;
	unsigned long long nodes_stat, null_stat;
	hashEntry hash;
	DEB_4( char b2[256];)
	int tmp;

	UNDO u;
	attack_model *att, ATT;

//	assert(tree->tree[ply-1][ply-1].move!=NA_MOVE);
	b->stats->nodes++;
	b->stats->positionsvisited++;
	tree->tree[ply][ply].move=NA_MOVE;
	
	mb=&mdum;
	if(!(b->stats->nodes & b->run.nodes_mask)){
		update_status(b);
		if(engine_stop!=0) {
			return 0;
//			goto ABFINISH2;
		}
	}
	LOGGER_4("ABNew d:%d, p:%d, nodes: %d, alfa %d, beta %d\n", depth,ply, b->stats->nodes, alfa, beta);

//	gmr=GenerateMATESCORE(ply);
// mate distance pruning
	gmr=-mb->real_score;
 
// mate distance pruning
	if(gmr <= alfa) {
//		mb->real_score= alfa;
		b->stats->faillow++;
		return alfa;
//		goto ABFINISH2;
	}
	if(-gmr >= beta) {
//		mb->real_score= beta;
		b->stats->failhigh++;
		return beta;
//		goto ABFINISH2;
	}

	if(b->pers->negamax==0) {
	// nechceme AB search, ale klasicky minimax
		alfa=0-iINFINITY;
		beta=iINFINITY;
	}

	opside = Flip(side);;
	att=&ATT;

	att->ke[b->side]=tolev->ke[b->side];
	att->att_by_side[opside]=KingAvoidSQ(b, att, opside);

//	CopySearchCnt(&s, b->stats);
	if (is_draw(b, att, b->pers)>0) {
		mb->move=tree->tree[ply][ply].move=DRAW_M;;
		mb->real_score=0;
		if(mb->real_score<=alfa) b->stats->faillow++;
		else if(mb->real_score>=beta) b->stats->failhigh++;
		else b->stats->failnorm++;
		goto ABFINISH2;
	}

// inicializuj zvazovany tah na NA
	tree->tree[ply][ply].move=NA_MOVE;
	tree->tree[ply+1][ply+1].move=NA_MOVE;
	tree->tree[ply][ply+1].move=NA_MOVE;

	
	talfa=alfa;
	tbeta=beta;
	if(tbeta>gmr) tbeta=gmr;
	if(talfa<-gmr) talfa=-gmr;

	mt.move=DRAW_M;

int hresult;
// time to check hash table
// TT CUT off?
	if(b->pers->use_ttable==1) {
		hash.key=b->key;
		hash.map=b->norm;
		hash.scoretype=NO_NULL;
		hresult=0;
		hresult=retrieveHash(b->hs, &hash, side, ply, depth, b->pers->use_ttable_prev, b->stats);
		if(hresult!=0) {
			mt.real_score=hash.value;
			mt.move=hash.bestmove;
			if((mt.move==NULL_MOVE)||(isMoveValid(b, mt.move, att, side, tree))) {
				if((hash.depth>=depth)) {
					tree->tree[ply][ply].move=hash.bestmove;
					tree->tree[ply][ply].score=hash.value;
					if((hash.scoretype!=FAILHIGH_SC)&&(hash.value<=talfa)){
						b->stats->faillow++;
						b->stats->failhashlow++;
						mb=&mt;
						goto ABFINISH2;
					} else
					if((hash.scoretype!=FAILLOW_SC)&&(hash.value>=tbeta)) {
						b->stats->failhigh++;
						b->stats->failhashhigh++;
						mb=&mt;
						goto ABFINISH2;
					} else 
					if(hash.scoretype==EXACT_SC){
						b->stats->failhashnorm++;
						if(b->pers->use_hash) {
							restoreExactPV(b->hs, b->key, b->norm, ply, tree);
							copyTree(tree, ply);
							b->stats->failnorm++;
							mb=&mt;
							goto ABFINISH2;
						} else {
							mt.real_score=mdum.real_score;
						}
					}
				} else {
// not enough depth
				if((b->pers->NMP_allowed>0) && (hash.scoretype!=FAILHIGH_SC)&&(hash.depth>= (depth - b->pers->NMP_reduction - 1))
					&& (hash.value<beta)) nulls=0;
					mt.move=DRAW_M;
				} 
			} else mt.move=DRAW_M;
		}
	}

	incheck = (UnPackCheck(tree->tree[ply-1][ply-1].move)!=0);
	reduce_o=extend_o=0;

	aftermovecheck=0;
// null move PRUNING / REDUCING
	if((nulls>0) && (isPV==0) && (b->pers->NMP_allowed>0) && (incheck==0) && (can_do_NullMove(b, att, talfa, tbeta, depth, ply, side)!=0)&&(depth>=b->pers->NMP_min_depth)) {
		tree->tree[ply][ply].move=NULL_MOVE;
		u=MakeNullMove(b);

		eval_king_checks(b, &(att->ke[b->side]), NULL, b->side);
		LOGGER_4("%*d, +S , NULL, amove ch:%d, depth %d, talfa %d, tbeta %d, best %d\n", 2+ply, ply, aftermovecheck, depth, talfa, tbeta, mb->real_score);
		
		b->stats->NMP_tries++;
		reduce=b->pers->NMP_reduction;
		ext=depth-reduce-1;
// save stats, to get info how many nodes were visited due to NULL move...
		nodes_stat=b->stats->nodes;
		null_stat=b->stats->u_nullnodes;
		if(ext>0) {
				  LOGGER_4("%*d, *S , NULL, AB, alfa %d, beta %d, ext %d, ply %d, nulls %d\n", 2+ply, ply, -tbeta, -tbeta+1, ext, ply+1, nulls-1);
			mt.real_score = -ABNew(b, -tbeta, -tbeta+1, ext, ply+1, opside, tree, nulls-1, att);
		}
		else {
				  LOGGER_4("%*d, *S , NULL, Q, alfa %d, beta %d, ext %d, ply %d, checks %d\n", 2+ply, ply, -tbeta, -tbeta+1, ext, ply+1, b->pers->quiesce_check_depth_limit);
			mt.real_score = -QuiesceNew(b, -tbeta, -tbeta+1, ext, ply+1, opside, tree, b->pers->quiesce_check_depth_limit, att);
		}

// update null nodes statistics
		UnMakeNullMove(b, u);
		LOGGER_4("%*d, -S , NULL, amove ch:%d, depth %d, talfa %d, tbeta %d, best %d, val %d\n", 2+ply, ply, aftermovecheck, depth, talfa, tbeta, mb->real_score, mt.real_score);

		
// engine stop protection?
		if(engine_stop!=0) goto ABFINISH2;
		b->stats->u_nullnodes=null_stat+(b->stats->nodes-nodes_stat);
		if(mt.real_score>=tbeta) {
			b->stats->NMP_cuts++;
			hash.key=b->key;
			hash.depth=(int16_t)depth;
			hash.map=b->norm;
			hash.value=mt.real_score;
			hash.bestmove=NULL_MOVE;
			hash.scoretype=FAILHIGH_SC;
			if((b->pers->use_ttable==1)&&(engine_stop==0)) storeHash(b->hs, &hash, side, ply, ext, b->stats);
			if(b->pers->NMP_search_reduction==0) {
				b->stats->failhigh++;
				mb=&mt;
				goto ABFINISH2;
			} else if(b->pers->NMP_search_reduction==-1) {
				reduce_o=0;
				mt.move=DRAW_M;
			} else reduce_o=b->pers->NMP_search_reduction;
		}
	} else if((nulls<=0) && (b->pers->NMP_allowed>0)) nulls=b->pers->NMP_allowed;

//	eval_king_checks(b, &(att->ke[side]), NULL, side);
// generate bitmaps for movegen

	if(incheck) simple_pre_movegen_n2check(b, att, b->side);
	else simple_pre_movegen_n2(b, att, b->side);

#if 1
	if(mt.move==DRAW_M) {
// no hash, if we are deep enough and not in zero window, try IID
		if((depth>=b->pers->IID_remain_depth) && (isPV)&&(b->pers->use_ttable==1)) {
			mt.real_score = ABNew(b, talfa, tbeta, depth-2,  ply, side, tree, nulls, att);
			if(engine_stop!=0) goto ABFINISH2;
			if(mt.real_score < talfa) {
				mt.real_score = ABNew(b, -iINFINITY, tbeta, depth-2,  ply, side, tree, nulls, att);
				if(engine_stop!=0) goto ABFINISH2;
			}
			if(retrieveHash(b->hs, &hash, side, ply, depth, b->pers->use_ttable_prev, b->stats)!=0)
				mt.move=hash.bestmove;
			else mt.move=DRAW_M;
		}
	}

// try to judge on position and reduce / quit move searching
// sort of forward pruning / forward reducing

	if((incheck!=1)&&(b->pers->quality_search_reduction!=0)&&(ply>4)) {
		qual=position_quality(b, att, talfa, tbeta, depth, ply, side);
		b->stats->position_quality_tests++;
		if(qual==0) {
			b->stats->position_quality_cutoffs++;
			if(b->pers->quality_search_reduction==-1) {
					tree->tree[ply][ply].move=FAILLOW_SC;
					tree->tree[ply][ply].score=-iINFINITY;
					goto ABFINISH2;
			} else reduce_o+=b->pers->quality_search_reduction;
		}
	}

//	tree->tree[ply][ply].move=NA_MOVE;
#endif

// generate moves

	sortMoveListNew_Init(b, att, &mvs);
	if((mt.move==DRAW_M)||(mt.move==NULL_MOVE)) mvs.hash.move=DRAW_M; else mvs.hash.move=mt.move;
	b->stats->poswithmove++;

// main loop
		LOGGER_4("%*d, *S , XXXX, amove ch:X, depth %d, talfa %d, tbeta %d,incheck %d, best %d\n", 2+ply, ply, depth, talfa, tbeta, incheck, mb->real_score);

	while ((getNextMove(b, att, &mvs, ply, side, incheck, &m, tree)!=0)&&(engine_stop==0)) {
		extend=extend_o;
		reduce=reduce_o;
//		b->stats->movestested++;
		tree->tree[ply][ply].move=m->move;

//!!!! debug only, remove
		u=MakeMove(b, m->move);
		if(u.captured==KING) {
			dump_moves(b, mvs.move, mvs.lastp-mvs.move, 0, "moves at error");
			printPV_simple_act(b,(tree_store*) tree, 99, b->side, NULL, NULL);
			assert(0);
		}

// is side to move in check, remember it and extend depth by one
		eval_king_checks(b, &(att->ke[b->side]), NULL, opside);
		if(isInCheck_Eval(b, att, b->side)) {
// idea from Crafty - extend only SAFE moves
			if(b->pers->check_extension>0) {
				pval = (u.captured < KING) ? b->pers->Values[0][u.captured] : 0;
				sval = SEE0(b, UnPackTo(m->move), side, pval) ;
					if(sval>=0) extend+=b->pers->check_extension;
				LOGGER_4("SEE0 sval %d, pval %d\n", sval, pval);
			}
			tree->tree[ply][ply].move|=CHECKFLAG;
			aftermovecheck=1;
		} else aftermovecheck=0;

// setup window
		ttbeta = ((mvs.count<=b->pers->PVS_full_moves)&&isPV) ? tbeta : talfa+1;

DEB_4(
		sprintfMoveSimple(m->move, b2);
		LOGGER_0("%*d, +S , %s, amove ch:%d, depth %d, talfa %d, tbeta %d, best %d, phase %d\n", 2+ply, ply, b2, aftermovecheck, depth, talfa, tbeta, mb->real_score, mvs.actph);
)
// setup reductions
	  if(mvs.count>b->pers->LMR_start_move && 
	  (b->pers->LMR_reduction>0) &&
	  (depth>=b->pers->LMR_remain_depth) && 
	  (incheck==0) && (aftermovecheck==0) &&
	  (extend==extend_o) && 
	  can_do_LMR(b, att, talfa, ttbeta, depth, ply, side, m)) {
		if(mvs.count>b->pers->LMR_prog_start_move) reduce += div(depth, b->pers->LMR_prog_mod).quot;
		reduce +=b->pers->LMR_reduction;
		b->stats->lmrtotal++;
	  }

	  m->real_score=SearchMoveNew(b, talfa, tbeta, ttbeta, depth, ply, extend, reduce, side, tree, nulls, att);
	  UnMakeMove(b, u);
	  if(engine_stop!=0) goto ABFINISH;
	
		LOGGER_4("%*d, -S , %s, amove ch:%d, depth %d, talfa %d, tbeta %d, best %d, val %d\n", 2+ply, ply, b2, aftermovecheck, depth, talfa, tbeta, mb->real_score, m->real_score);

		if(m->real_score>=tbeta) {
			if(mvs.count==1) b->stats->firstcutoffs++;
			b->stats->cutoffs++;
			if((b->pers->use_killer>=1)&&(is_quiet_move(b, att, m))) {
				update_killer_move(b->kmove, ply, m->move, b->stats);
				updateHHTable(b, b->hht, m, 0, side, depth, ply);
			}
			mb=m;
			break;
		}
		if(m->real_score>mb->real_score) {
			mb=m;
			if(mb->real_score>talfa) {
				talfa=mb->real_score;
				copyTree(tree, ply);
			}
		}
	}

	if(mvs.count<=0) {
		if(incheck==0) {
// no moves found, not in check => draw, if incheck means mated - default setting for mb
			mb->real_score=0;
			mb->move=DRAW_M;
		}
	}
	tree->tree[ply][ply].move=mb->move;
	tree->tree[ply][ply].score=mb->real_score;

		// update stats & store Hash

	hash.key=b->key;
	hash.depth=(int16_t)depth;
	hash.map=b->norm;
	hash.value=mb->real_score;
	hash.bestmove=mb->move;
	if(mb->real_score>alfa && mb->real_score<beta) {
		b->stats->failnorm++;
		hash.scoretype=EXACT_SC;
		if((b->pers->use_ttable==1)&&(b->pers->use_hash==1)&&(depth>0)&&(engine_stop==0)) {
			storeHash(b->hs, &hash, side, ply, depth, b->stats);
			storeExactPV(b->hs, b->key, b->norm, tree, ply);
		}
	} else {
		if(mb->real_score>=beta) {
			b->stats->failhigh++;
			hash.scoretype=FAILHIGH_SC;
		} else {
			b->stats->faillow++;
			hash.scoretype=FAILLOW_SC;
		} 
		if((b->pers->use_ttable==1)&&(depth>0)) storeHash(b->hs, &hash, side, ply, depth, b->stats);
	}
ABFINISH:

	b->stats->movestested+=mvs.count;
	b->stats->possiblemoves+=((mvs.lastp-mvs.move));
ABFINISH2:
	LOGGER_4("count %d, score %d\n", mvs.count, mb->real_score);
	return mb->real_score; 
}

int IterativeSearch(board *b, int alfa, int beta, int depth, int side, int start_depth, tree_store * tree)
{
	int f, l;
	search_history hist;
	struct _statistics s, r, s2;

	int reduce, pval, sval;
	int asp_win=0;
	int ply=0;

	int tc,cc, v, xcc, old_score, old_score_count ;
	move_entry move[300], backup[300];
	MOVESTORE bestmove, hashmove, i, t1pbestmove, t2pbestmove;
	move_entry *m, *n, tm;
	int opside;
	int legalmoves, incheck, best, talfa, tbeta, ttbeta, 
		tcheck, t1pbest, t2pbest, talfa_o, cct, xcct, aftermovecheck, isPVcount;
	unsigned long long int nodes_bmove;
	int extend;
	hashEntry hash;
	char b2[256];

	UNDO u;
	attack_model *att, ATT;
	unsigned long long tstart, ebfnodesold, tnow;

	old_score=best=0-iINFINITY;
	old_score_count=0;
	b->bestmove=NA_MOVE;
	b->bestscore=best;
	bestmove=hashmove=NA_MOVE;
	/*
	 * b->stats, complete stats for all iterations
	 * s stats at beginning of iteration
	 */
	clearSearchCnt(&s);
	clearSearchCnt(&s2);
	clearSearchCnt(b->stats);
	b->run.nodes_mask=(1ULL<<b->pers->check_nodes_count)-1;
	b->run.iter_start=b->run.time_start;
	b->run.nodes_at_iter_start=b->stats->nodes;

	opside = (side == WHITE) ? BLACK : WHITE;
	copyBoard(b, &(tree->tree_board ));

	printBoardNice(b);

	// make current line end here
	tree->tree[ply][ply].move=NA_MOVE;
	tree->tree[ply+1][ply+1].move=NA_MOVE;
	tree->tree[ply][ply+1].move=NA_MOVE;
	tree->tree[ply][ply].score=0;

	//		att=&(tree->tree[ply][ply].att);
	att=&ATT;
	att->phase = eval_phase(b, b->pers);
	eval_king_checks_all(b, att);

	// is opposite side in check ?
	if(isInCheck_Eval(b, att, opside)!=0) {
		DEB_1(printf("Opside in check4!\n");)
		tree->tree[ply][ply].move=MATE_M;
		//????
		return MATESCORE;
	}

	// is side to move in check ?
	if(isInCheck_Eval(b, att, side)!=0) {
		incheck=1;
	}	else incheck=0;

	// check database of openings
	i= probe_book(b);
//	i=NA_MOVE;
	if(i!=NA_MOVE) {
		tree->tree[ply][ply].move=i;
		tree->tree[ply][ply+1].move=NA_MOVE;
		tree->tree[ply][ply].score=0;
		b->bestmove=tree->tree[ply][ply].move;
		b->bestscore=tree->tree[ply][ply].score;
		return 0;
	}

	simple_pre_movegen(b, att, b->side);
	simple_pre_movegen(b, att, opside);
	//!!! optimalizace
	b->p_pv.line[ply].move=NA_MOVE; //???
	m = move;
	if(incheck==1) {
		generateInCheckMoves(b, att, &m);
	} else {
		generateCaptures(b, att, &m, 1);
		generateMoves(b, att, &m);
	}
	b->stats->poswithmove++;
	ebfnodesold=1;
	n = move;

	tc=(m-n);
	// store moves and ordering

	talfa=alfa;
	tbeta=beta;
	// 0 - not age hash table
	// 1 - age with new game
	// 2 - age with new move / Iterative search entry
	// 3 - age with new interation 
	if(b->pers->ttable_clearing>=2) invalidateHash(b->hs);
	// iterate and increase depth gradually
	oldPVcheck=0;

	// initial sort according
//		dump_moves(b, move, tc, 0, "initial root moves");
	cc = 0;
#if 1
	b->depth_run=1;
	if(!incheck)
	while (cc<tc) {
		u=MakeMove(b, move[cc].move);
		eval_king_checks(b, &(att->ke[b->side]), NULL, b->side);
		if(isInCheck_Eval(b ,att, b->side)) {
			extend+=b->pers->check_extension;
			move[cc].move|=CHECKFLAG;
		}
		tree->tree[ply][ply].move=move[cc].move;
		if(!incheck) {
			v = -QuiesceNew(b, -tbeta, -talfa, 0,  1, opside, tree, 0, att);
			move[cc].qorder=v;
		}
		UnMakeMove(b, u);
		cc++;
	}
#endif

//		dump_moves(b, move, tc, 0, "initial root moves rescanned");

	tstart=readClock();
	if(depth>MAXPLY) depth=MAXPLY;

	talfa=alfa;
	tbeta=beta;
	clearHHTable(b->hht);

	cct=0;
	xcct=1;
	if(depth>MAXPLY) depth=MAXPLY;
	
	for(f=start_depth;f<=depth;f++) {
	
		update_status(b);
		b->depth_run=f;

		if(b->pers->ttable_clearing>=3) invalidateHash(b->hs);
		if(b->pers->negamax==0) {
//			alfa=0-iINFINITY;
//			beta=iINFINITY;
			talfa=alfa;
			tbeta=beta;
		}
		talfa_o=talfa;
		CopySearchCnt(&s, b->stats);
		installHashPV(&b->p_pv, b, f-1, b->stats);
		clear_killer_moves(b->kmove);
		xcc=-1;
		// (re)sort moves
		hash.key=b->key;
		hash.map=b->norm;
//		hashmove=b->p_pv.line[0].move;
		hashmove=NA_MOVE;
		tc=sortMoveList_QInit(b, att, hashmove, move, (int)(m-n), ply, (int)(m-n) );
		getQNSorted(b, move, tc, 0, tc);
// move find pv move and move it to first place
		
		if(f>start_depth) {
//			sprintfMoveSimple(b->p_pv.line[0].move, b2);
//			LOGGER_0("PV BESTMOVE %s\n", b2);
			for(cc=0;cc<tc;cc++) 
			  if(move[cc].move==b->p_pv.line[0].move) break;
//			LOGGER_0("cc %d, tc %d\n", cc, tc);
			assert(cc<tc);
			tm=move[cc];
			if(cc>0) {
			  for(;cc>0;cc--) {
				move[cc]=move[cc-1];
			  }
			  move[0]=tm;
			}
		}
//		dump_moves(b, move, tc, 0, "root moves after sort");

		b->stats->positionsvisited++;
		b->stats->possiblemoves+=(unsigned int)tc;
		b->stats->nodes++;
		
		/*
		 * **********************************************************************************
		 */
		best=0-iINFINITY;
		isPVcount=0;
		
		// hack
		cc = 0;
		// loop over all moves
		// inicializujeme line
		tree->tree[ply][ply].move=NA_MOVE;
		legalmoves=0;
		eval_king_checks(b, &(att->ke[b->side]), NULL, b->side);
		
// looping moves for depth f
		while ((cc<tc)&&(engine_stop==0)) {

			extend=0;
			if(!(b->stats->nodes & b->run.nodes_mask)){
				update_status(b);
			}
//			nodes_bmove=b->stats->movestested;
			nodes_bmove=b->stats->movestested+b->stats->qmovestested;
			b->stats->movestested++;
			tree->tree[ply][ply].move=move[cc].move;
			move[cc].real_score=0;
			u=MakeMove(b, move[cc].move);
			eval_king_checks(b, &(att->ke[b->side]), NULL, b->side);
			aftermovecheck=0;
			if(isInCheck_Eval(b ,att, b->side)) {
// move gives check, so extend, remember the check, mark move as giving check - to pass it to next ply

				if(b->pers->check_extension>0) {
					pval = (u.captured < KING) ? b->pers->Values[0][u.captured] : 0;
					sval = SEE0(b, UnPackTo(move[cc].move), side, pval) ;
						if(sval>=0) extend+=b->pers->check_extension;
//					LOGGER_0("SEE0 sval %d, pval %d\n", sval, pval);
				}

				aftermovecheck=1;
				move[cc].move|=CHECKFLAG;
				tree->tree[ply][ply].move|=CHECKFLAG;
			}
//#ifdef DEBUG4
		sprintfMoveSimple(move[cc].move, b2);
		LOGGER_0("%*d, +I , %s, amove ch:%d, depth %d, talfa %d, tbeta %d\n", 2+ply, ply, b2, aftermovecheck, f, talfa, tbeta);
//		printPV_simple(b, tree, f, b->side , &s, b->stats);
//#endif

			reduce=0;
			if(legalmoves>=b->pers->LMR_start_move && (b->pers->LMR_reduction>0) && (depth>=b->pers->LMR_remain_depth) && (incheck==0) && (aftermovecheck==0) && can_do_LMR(b, att, talfa, tbeta, depth, ply, side, &(move[cc]))) {
				reduce+=b->pers->LMR_reduction;
				b->stats->lmrtotal++;
			}

			ttbeta = ((isPVcount<b->pers->PVS_root_full_moves)) ? tbeta : talfa+1;
			v=SearchMoveNew(b, talfa, tbeta, ttbeta, f, 0, extend, reduce, side, tree, b->pers->NMP_allowed, att);
			
/*
			if((int)isPVcount<(int)b->pers->PVS_root_full_moves) {
				// full window
				if((f-1+extend)>0) v = -ABNew(b, -tbeta, -talfa, f-1+extend, ply+1, opside, tree, b->pers->NMP_allowed, att);
				else v = -QuiesceNew(b, -tbeta, -talfa, 0,  ply+1, opside, tree, b->pers->quiesce_check_depth_limit, att);
			} else {
				reduce=0;
				if(legalmoves>=b->pers->LMR_start_move && (b->pers->LMR_reduction>0) && (depth>=b->pers->LMR_remain_depth) && (incheck==0) && (aftermovecheck==0) && can_do_LMR(b, att, talfa, tbeta, depth, ply, side, &(move[cc]))) {
					reduce+=b->pers->LMR_reduction;
					b->stats->lmrtotal++;
				}
				if((f-1+extend-reduce)>0) v = -ABNew(b, -(talfa+1), -talfa, f-1+extend-reduce,(int) ply+1, opside, tree, b->pers->NMP_allowed, att);
				else v = -QuiesceNew(b, -(talfa+1), -talfa,(int) f-1+extend-reduce, (int) ply+1, opside, tree, b->pers->quiesce_check_depth_limit, att);
				b->stats->zerototal++;
//alpha raised, full window search
				if((v>talfa && v < tbeta)&&(engine_stop==0)) {
					b->stats->zerorerun++;
					if((f-1+extend)>0) v = -ABNew(b, -tbeta, -talfa, f-1+extend, ply+1, opside, tree, b->pers->NMP_allowed, att);
					else v = -QuiesceNew(b, -tbeta, -talfa, 0,  ply+1, opside, tree, b->pers->quiesce_check_depth_limit, att);
					if(v<=talfa) b->stats->fhflcount++;
				}
			}
*/
			move[cc].real_score=v;
			LOGGER_0("%*d, -I , %s, amove ch:%d, depth %d, talfa %d, tbeta %d, val %d\n", 2+ply, ply, b2, aftermovecheck, depth, talfa, tbeta, v);
			if(engine_stop==0) {
//				unsigned long long tqorder=b->stats->movestested-nodes_bmove;
				unsigned long long tqorder=b->stats->movestested+b->stats->qmovestested-nodes_bmove;
				move[cc].qorder = (tqorder>=(LONG_MAX/2)) ? (LONG_MAX/2) : (long int) tqorder;
//				move[cc].qorder/=(f);
//				move[cc].qorder +=v;
//				move[cc].qorder=v;

				legalmoves++;
				if(v>talfa) isPVcount++;
				else if(cc==0) {
// handle faillow at first move
					xcc=-1;
					UnMakeMove(b, u);
					break;
				}
//				LOGGER_3("cc:%d, talfa %d, tbeta %d, best %d, value %d\n", cc, talfa, tbeta, best, v);
				if(v>best) {
					best=v;
					bestmove=move[cc].move;
					xcc=cc;
					if(v > talfa) {
						talfa=v;
						if(v >= tbeta) {
							if(b->pers->use_aspiration==0) {
								LOGGER_1("ERR: nemelo by jit pres TBETA v rootu\n");
								abort();
							}
							tree->tree[ply][ply+1].move=BETA_CUT;
							xcc=-1;
							UnMakeMove(b, u);
							break;
						}
						else {
							copyTree(tree, ply);
							// best line change
							if(b->uci_options->engine_verbose>=1) printPV_simple(b, tree, f, b->side , &s, b->stats);
						}
					}
				} else if(cc==0) {
						  xcc=-1;
						  UnMakeMove(b, u);
						  break;
				}
				UnMakeMove(b, u);
				cc++;
			} else UnMakeMove(b, u);
		}
//		dump_moves(b, move, tc, 0, NULL);
		tree->tree[ply][ply].move=bestmove;
		tree->tree[ply][ply].score=best;
		
		sprintfMoveSimple(bestmove, b2);
		LOGGER_0("BESTMOVE %s\n", b2);

// search has finished
		if(engine_stop==0) {
// was not stopped during last iteration 
			b->stats->iterations++;
// clear qorder for moves not processed
		int li;
		for(li=cc;li<tc;li++) move[li].qorder = 0;

// handle aspiration if used
// check for problems	
// over beta, not rising alfa at fist move or at all
			if((b->pers->use_aspiration!=0)&&(f>4)) {
// handle start of aspiration
				if((xcc!=-1)) {
					talfa=best-b->pers->use_aspiration;
					tbeta=best+b->pers->use_aspiration;
//						LOGGER_0("aspX cc:tc %d:%d f=%d, talfa %d, tbeta %d, best %d\n", cc, tc, f, talfa, tbeta, best);
					asp_win=1;
				} else {
// handle anomalies	
					if(asp_win==2) {
						talfa=alfa;
						tbeta=beta;
						b->stats->aspfailits++;
						asp_win=0;
					} else if(asp_win==1) {
						if(tbeta<=best) tbeta=beta;
						else talfa=alfa;
//						LOGGER_0("asp1 cc:tc %d:%d f=%d, talfa %d, tbeta %d, best %d\n", cc, tc, f, talfa, tbeta, best);
						b->stats->aspfailits++;
						asp_win=2;
					}			
					if(cc>=tc) {
						f--;
//						LOGGER_0("asp dec cc:tc %d:%d f=%d\n", cc, tc, f);
					}
				}
			} else {
				talfa=alfa;
				tbeta=beta;
			}
			// store proper bestmove & score
			// update stats & store Hash
			
			if(xcc!=-1) {
				hash.key=b->key;
				hash.depth=(int16_t)f;
				hash.map=b->norm;
				hash.value=best;
				hash.bestmove=bestmove;
				b->stats->failnorm++;
				hash.scoretype=EXACT_SC;
				if((b->pers->use_ttable==1)&&(f>0)&&(engine_stop==0)) {
//					LOGGER_0("PV store Iter\n");
					storeHash(b->hs, &hash, side, ply, f, b->stats);
					storeExactPV(b->hs, b->key, b->norm, tree, f);
				}
				store_PV_tree(tree, &b->p_pv);
				
				if(old_score==best) {
					old_score_count++;
					if((old_score_count>=3)&&(GetMATEDist(b->bestscore)<=(f-1)))  break;
				} else {
					old_score=best;
					old_score_count=0;
				}
			} // finished iteration
		}
		else {
// last iteration was not finished
			if(xcc>-1) {
// move was found			
				t1pbest=best;
				t1pbestmove=bestmove;
			} else {
				t1pbestmove=move[0].move;
				t1pbest=-MATEMAX;
			}
			if(f>start_depth) {
				t2pbestmove=b->p_pv.line[0].move;
				t2pbest=b->p_pv.line[0].score;
			} else {
				t2pbestmove=move[0].move;
				t2pbest=-MATEMAX;
			}
// compare what was found during unfinished iteration vs result of previous iteration
			if(t1pbest>=t2pbest) {
				tree->tree[ply][ply].move=t1pbestmove;
				tree->tree[ply][ply].score=t1pbest;
				tree->tree[ply][ply+1].move=NA_MOVE;
			} else {
				tree->tree[ply][ply].move=t2pbestmove;
				tree->tree[ply][ply].score=t2pbest;
				restore_PV_tree(&b->p_pv, tree);
			}
		}

		b->bestmove=tree->tree[ply][ply].move;
		b->bestscore=tree->tree[ply][ply].score;

		oldPVcheck=1;
//		dump_moves(b, move, tc, 0, "root moves before rescaling");
// rescale computed nodes counts
//		int li;
//		long int maxcount=A_OR_MAX;
//		for(li=0;li<tc;li++) if(maxcount<move[li].qorder) maxcount=move[li].qorder;
//		if(maxcount>A_OR_MAX) for(li=0;li<tc;li++) {
  	long long int tempsc;
// 			tempsc=(move[li].qorder*A_OR_MAX)/maxcount;
//			move[li].qorder=(long) tempsc;
// 		}

//		dump_moves(b, move, tc, 0, "root moves after rescaling");

		tnow=readClock();
		b->stats->elaps+=(tnow-tstart);

		if(engine_stop==0) {
			tstart=tnow;
			b->stats->ebfnodespri=ebfnodesold;
			ebfnodesold=(b->stats->nodes-s.nodes);
			b->stats->ebfnodes=ebfnodesold;
// calculate only finished iterations
			b->stats->depth=f;
			LOGGER_0("Depth %d, EBF %f, ItNodes %lld, PrevNodes %lld\n", f, (float) b->stats->ebfnodes/b->stats->ebfnodespri, b->stats->ebfnodes, b->stats->ebfnodespri);
		}
		DecSearchCnt(b->stats,&s,&r);
// update stats how f-ply search has performed
		AddSearchCnt(&(STATS[f]), &r);
		AddSearchCnt(&(STATS[MAXPLY]), &r);

		// break only if mate is now - not in qsearch
//		if(GetMATEDist(b->bestscore)<=(f-3)) {
//			break;
//		}
		if((engine_stop!=0)||(search_finished(b)!=0)) break;
		if((b->uci_options->engine_verbose>=1)&&(xcc!=-1)) printPV_simple(b, tree, f,b->side, &s, b->stats);
		LOGGER_0("BEST move POS %d\n",xcc);
	} //deepening
//	printf("dist %d, scr %d, f %d depth %d, stop %d, search_fin %d\n", GetMATEDist(b->bestscore), b->bestscore, f, depth, engine_stop, search_finished(b));
// finished here
	b->stats->depth_sum+=f;
	b->stats->depth_max_sum+=b->stats->depth_max;
	if(STATS[f].depth< b->stats->depth) STATS[f].depth=b->stats->depth ;
	if(STATS[f].depth_max< b->stats->depth_max) STATS[f].depth_max=b->stats->depth_max ;
	STATS[f].depth_sum+=b->stats->depth ;
	STATS[f].depth_max_sum+=b->stats->depth_max ;
	if(STATS[MAXPLY].depth< b->stats->depth) STATS[MAXPLY].depth=b->stats->depth ;
	if(STATS[MAXPLY].depth_max< b->stats->depth_max) STATS[MAXPLY].depth_max=b->stats->depth_max ;
	STATS[MAXPLY].depth_sum+=b->stats->depth ;
	STATS[MAXPLY].depth_max_sum+=b->stats->depth_max ;

//	if(b->uci_options->engine_verbose>=1) printPV_simple(b, tree, f,b->side, &s, b->stats);
	DEB_1 (printSearchStat(b->stats);)
//	DEB_1 (tnow=readClock();)
//	DEB_1 (LOGGER_1("TIMESTAMP: Start: %llu, Stop: %llu, Diff: %lld milisecs\n", b->run.time_start, tnow, (tnow-b->run.time_start));)

	return b->bestscore;
}

int IterativeSearchN(board *b, int alfa, int beta, int depth, int side, int start_depth, tree_store * tree)
{
	int f, l;
	search_history hist;
	struct _statistics s, r, s2;

	int reduce, pval, sval;
	int asp_win=0;
	int ply=0;

	int tc,cc, v, xcc, old_score, old_score_count ;
//	move_entry move[300], backup[300];
	MOVESTORE bestmove, hashmove, i, t1pbestmove, t2pbestmove;
	move_entry *m, *n, tm;
	move_cont mvs;

	int opside;
	int legalmoves, incheck, best, talfa, tbeta, ttbeta, 
		tcheck, t1pbest, t2pbest, talfa_o, xcct, cct, aftermovecheck, isPVcount;
	unsigned long long int nodes_bmove;
	int extend;
	hashEntry hash;
	DEB_4( char b2[256];)

	UNDO u;
	attack_model *att, ATT;
	unsigned long long tstart, ebfnodesold, tnow;

	old_score=best=0-iINFINITY;
	old_score_count=0;
	b->bestmove=NA_MOVE;
	b->bestscore=best;
	bestmove=hashmove=NA_MOVE;
	/*
	 * b->stats, complete stats for all iterations
	 * s stats at beginning of iteration
	 */
	clearSearchCnt(&s);
	clearSearchCnt(&s2);
	clearSearchCnt(b->stats);
	b->run.nodes_mask=(1ULL<<b->pers->check_nodes_count)-1;
	b->run.iter_start=b->run.time_start;
	b->run.nodes_at_iter_start=b->stats->nodes;

	opside = (side == WHITE) ? BLACK : WHITE;
	copyBoard(b, &(tree->tree_board ));

	DEB_1(printBoardNice(b);)

	// make current line end here
	tree->tree[ply][ply].move=NA_MOVE;
	tree->tree[ply+1][ply+1].move=NA_MOVE;
	tree->tree[ply][ply+1].move=NA_MOVE;
	tree->tree[ply][ply].score=0;
	b->p_pv.line[ply].move=NA_MOVE; //???

	att=&ATT;
	att->phase = eval_phase(b, b->pers);
	eval_king_checks_all(b, att);

	// is opposite side in check ?
	if(isInCheck_Eval(b, att, opside)!=0) {
		DEB_1(printf("Opside in check4!\n");)
		tree->tree[ply][ply].move=MATE_M;
		return MATESCORE;
	}

	// is side to move in check ?
	incheck= (isInCheck_Eval(b, att, side)!=0);

	// check database of openings
	i= probe_book(b);
	if(i!=NA_MOVE) {
		tree->tree[ply][ply].move=i;
		b->bestmove=tree->tree[ply][ply].move;
		b->bestscore=tree->tree[ply][ply].score;
		return 0;
	}

	att->att_by_side[opside]=KingAvoidSQ(b, att, opside);
//	simple_pre_movegen_n2(b, att, opside);
	if(incheck) simple_pre_movegen_n2check(b, att, b->side);
	else simple_pre_movegen_n2(b, att, b->side);

	sortMoveListNew_Init(b, att, &mvs);
	mvs.hash.move=DRAW_M;
	mvs.lastp=mvs.move;
	mvs.next=mvs.lastp;
	if(incheck==1) {
		generateInCheckMovesN(b, att, &(mvs.lastp), 1);
	} else {
		generateCapturesN(b, att, &(mvs.lastp), 1);
		generateMovesN(b, att, &(mvs.lastp));
	}
	tc=mvs.lastp-mvs.move;

	b->stats->poswithmove++;
	ebfnodesold=1;

	talfa=alfa;
	tbeta=beta;

	// 0 - not age hash table
	// 1 - age with new game
	// 2 - age with new move / Iterative search entry
	// 3 - age with new interation 
	if(b->pers->ttable_clearing>=2) invalidateHash(b->hs);
	// iterate and increase depth gradually
	oldPVcheck=0;

	// initial sort according
	cc = 0;
#if 1
	b->depth_run=1;
	if(!incheck)
	while (cc<tc) {
		u=MakeMove(b, mvs.move[cc].move);
		eval_king_checks(b, &(att->ke[b->side]), NULL, b->side);
		if(isInCheck_Eval(b ,att, b->side)) {
//			extend+=b->pers->check_extension;
			mvs.move[cc].move|=CHECKFLAG;
		}
		tree->tree[ply][ply].move=mvs.move[cc].move;
		v = -QuiesceNew(b, -tbeta, -talfa, 0,  1, opside, tree, 0, att);
		mvs.move[cc].qorder=v;
		UnMakeMove(b, u);
		cc++;
	}
#endif

	tstart=readClock();
	if(depth>MAXPLY) depth=MAXPLY;

	talfa=alfa;
	tbeta=beta;
	clearHHTable(b->hht);

	cct=0;
	xcct=1;
	if(depth>=MAXPLY) depth=MAXPLY-1;
	
	for(f=start_depth;f<=depth;f++) {
	
		update_status(b);
		b->depth_run=f;

		if(b->pers->ttable_clearing>=3) invalidateHash(b->hs);
		if(b->pers->negamax==0) {
			talfa=alfa;
			tbeta=beta;
		}
		talfa_o=talfa;
		CopySearchCnt(&s, b->stats);
		installHashPV(&b->p_pv, b, f-1, b->stats);
		clear_killer_moves(b->kmove);
		xcc=-1;
		// (re)sort moves
		SelectBestO(&mvs); 
		if(f>start_depth) {
			for(cc=0;cc<tc;cc++) 
			  if(mvs.move[cc].move==b->p_pv.line[0].move) break;
			tm=mvs.move[cc];
			if(cc>0) {
			  for(;cc>0;cc--) {
				mvs.move[cc]=mvs.move[cc-1];
			  }
			  mvs.move[0]=tm;
			}
		}

		b->stats->positionsvisited++;
		b->stats->possiblemoves+=(unsigned int)tc;
		b->stats->nodes++;
		
		best=0-iINFINITY;
		isPVcount=0;
		
		// hack
		cc = 0;
		// loop over all moves
		// inicializujeme line
		tree->tree[ply][ply].move=NA_MOVE;
		legalmoves=0;
		eval_king_checks(b, &(att->ke[b->side]), NULL, b->side);
		
// looping moves for depth f
		while ((cc<tc)&&(engine_stop==0)) {

			extend=0;
			if(!(b->stats->nodes & b->run.nodes_mask)){
				update_status(b);
			}
			nodes_bmove=b->stats->movestested+b->stats->qmovestested;
			b->stats->movestested++;
			tree->tree[ply][ply].move=mvs.move[cc].move;
			mvs.move[cc].real_score=0;
			u=MakeMove(b, mvs.move[cc].move);
			eval_king_checks(b, &(att->ke[b->side]), NULL, b->side);
			aftermovecheck=0;
			if(isInCheck_Eval(b ,att, b->side)) {
				tree->tree[ply][ply].move|=CHECKFLAG;
// move gives check, so extend, remember the check, mark move as giving check - to pass it to next ply

				if(b->pers->check_extension>0) {
					pval = (u.captured < KING) ? b->pers->Values[0][u.captured] : 0;
					sval = SEE0(b, UnPackTo(mvs.move[cc].move), side, pval) ;
						if(sval>=0) extend+=b->pers->check_extension;
					LOGGER_4("SEE0 sval %d, pval %d\n", sval, pval);
				}
//				aftermovecheck=1;
			}
DEB_4(
		sprintfMoveSimple(mvs.move[cc].move, b2);
		LOGGER_0("%*d, +I , %s, amove ch:%d, depth %d, talfa %d, tbeta %d\n", 2+ply, ply, b2, aftermovecheck, f, talfa, tbeta);
)

			reduce=0;
			if(legalmoves>=b->pers->LMR_start_move && (b->pers->LMR_reduction>0) && (depth>=b->pers->LMR_remain_depth) && (incheck==0) && (aftermovecheck==0) && can_do_LMR(b, att, talfa, tbeta, depth, ply, side, &(mvs.move[cc]))) {
				reduce+=b->pers->LMR_reduction;
				b->stats->lmrtotal++;
			}

			ttbeta = ((isPVcount<b->pers->PVS_root_full_moves)) ? tbeta : talfa+1;
			v=SearchMoveNew(b, talfa, tbeta, ttbeta, f, 0, extend, reduce, side, tree, b->pers->NMP_allowed, att);
			
			mvs.move[cc].real_score=v;
			LOGGER_4("%*d, -I , %s, amove ch:%d, depth %d, talfa %d, tbeta %d, val %d\n", 2+ply, ply, b2, aftermovecheck, depth, talfa, tbeta, v);
			if(engine_stop==0) {
				unsigned long long tqorder=b->stats->movestested+b->stats->qmovestested-nodes_bmove;
				mvs.move[cc].qorder = (tqorder>=(LONG_MAX/2)) ? (LONG_MAX/2) : (long int) tqorder;
//				mvs.move[cc].qorder = v;

				legalmoves++;
				if(v>talfa) isPVcount++;
				else if(cc==0) {
// handle faillow at first move
					xcc=-1;
					UnMakeMove(b, u);
					break;
				}
				if(v>best) {
					best=v;
					bestmove=mvs.move[cc].move;
					xcc=cc;
					if(v > talfa) {
						talfa=v;
						if(v >= tbeta) {
							if(b->pers->use_aspiration==0) {
								LOGGER_1("ERR: nemelo by jit pres TBETA v rootu\n");
								abort();
							}
							tree->tree[ply][ply+1].move=BETA_CUT;
							xcc=-1;
							UnMakeMove(b, u);
							break;
						}
						else {
							copyTree(tree, ply);
							tree->tree[ply][ply].score=best;
							// best line change
							if(b->uci_options->engine_verbose>=1) printPV_simple(b, tree, f, b->side , &s, b->stats);
DEB_4(
							sprintfMoveSimple(mvs.move[cc].move, b2);
							LOGGER_0("%*d, *I ch , %s, amove ch:%d, depth %d, talfa %d, tbeta %d, val %d\n", 2+ply, ply, b2, aftermovecheck, depth, talfa, tbeta, best);
)
						}
					}
				} else if(cc==0) {
						  xcc=-1;
						  UnMakeMove(b, u);
						  break;
				}
				UnMakeMove(b, u);
				cc++;
			} else UnMakeMove(b, u);
		}
		tree->tree[ply][ply].move=bestmove;
		tree->tree[ply][ply].score=best;
		
		DEB_4(sprintfMoveSimple(bestmove, b2);)
		LOGGER_4("BESTMOVE %s\n", b2);

// search has finished
		if(engine_stop==0) {
// was not stopped during last iteration 
			b->stats->iterations++;
// clear qorder for moves not processed
		int li;
		for(li=cc;li<tc;li++) mvs.move[li].qorder = 0;

// handle aspiration if used
// check for problems	
// over beta, not rising alfa at fist move or at all
			if((b->pers->use_aspiration!=0)&&(f>4)) {
// handle start of aspiration
				if((xcc!=-1)) {
					talfa=best-b->pers->use_aspiration;
					tbeta=best+b->pers->use_aspiration;
//						LOGGER_0("aspX cc:tc %d:%d f=%d, talfa %d, tbeta %d, best %d\n", cc, tc, f, talfa, tbeta, best);
					asp_win=1;
				} else {
// handle anomalies	
					if(asp_win==2) {
						talfa=alfa;
						tbeta=beta;
						b->stats->aspfailits++;
						asp_win=0;
					} else if(asp_win==1) {
						if(tbeta<=best) tbeta=beta;
						else talfa=alfa;
						b->stats->aspfailits++;
						asp_win=2;
					}			
					if(cc>=tc) {
						f--;
					}
				}
			} else {
				talfa=alfa;
				tbeta=beta;
			}
			// store proper bestmove & score
			// update stats & store Hash
			
			if(xcc!=-1) {
				hash.key=b->key;
				hash.depth=(int16_t)f;
				hash.map=b->norm;
				hash.value=best;
				hash.bestmove=bestmove;
				b->stats->failnorm++;
				hash.scoretype=EXACT_SC;
				if((b->pers->use_ttable==1)&&(f>0)&&(engine_stop==0)) {
					storeHash(b->hs, &hash, side, ply, f, b->stats);
					storeExactPV(b->hs, b->key, b->norm, tree, f);
				}
				store_PV_tree(tree, &b->p_pv);
				
				if(old_score==best) {
					old_score_count++;
					if((old_score_count>=3)&&(GetMATEDist(b->bestscore)<=(f-1)))  break;
				} else {
					old_score=best;
					old_score_count=0;
				}
			} // finished iteration
		}
		else {
// last iteration was not finished
			if(xcc>-1) {
// move was found			
				t1pbest=best;
				t1pbestmove=bestmove;
			} else {
				t1pbestmove=mvs.move[0].move;
				t1pbest=-MATEMAX;
			}
			if(f>start_depth) {
				t2pbestmove=b->p_pv.line[0].move;
				t2pbest=b->p_pv.line[0].score;
			} else {
				t2pbestmove=mvs.move[0].move;
				t2pbest=-MATEMAX;
			}
// compare what was found during unfinished iteration vs result of previous iteration
			if(t1pbest>=t2pbest) {
				tree->tree[ply][ply].move=t1pbestmove;
				tree->tree[ply][ply].score=t1pbest;
				tree->tree[ply][ply+1].move=NA_MOVE;
			} else {
				tree->tree[ply][ply].move=t2pbestmove;
				tree->tree[ply][ply].score=t2pbest;
				restore_PV_tree(&b->p_pv, tree);
			}
		}

		b->bestmove=tree->tree[ply][ply].move;
		b->bestscore=tree->tree[ply][ply].score;

		oldPVcheck=1;
  	long long int tempsc;

		tnow=readClock();
		b->stats->elaps+=(tnow-tstart);

		if(engine_stop==0) {
			tstart=tnow;
			b->stats->ebfnodespri=ebfnodesold;
			ebfnodesold=(b->stats->nodes-s.nodes);
			b->stats->ebfnodes=ebfnodesold;
// calculate only finished iterations
			b->stats->depth=f;
			LOGGER_1("Depth %d, EBF %f, ItNodes %lld, PrevNodes %lld\n", f, (float) b->stats->ebfnodes/b->stats->ebfnodespri, b->stats->ebfnodes, b->stats->ebfnodespri);
		}
		DecSearchCnt(b->stats,&s,&r);
// update stats how f-ply search has performed
		AddSearchCnt(&(STATS[f]), &r);
		AddSearchCnt(&(STATS[MAXPLY]), &r);

		// break only if mate is now - not in qsearch
		if((engine_stop!=0)||(search_finished(b)!=0)) break;
		if((b->uci_options->engine_verbose>=1)&&(xcc!=-1)) printPV_simple(b, tree, f,b->side, &s, b->stats);
		LOGGER_1("BEST move POS %d\n",xcc);
	} //deepening
// finished here
	b->stats->depth_sum+=f;
	b->stats->depth_max_sum+=b->stats->depth_max;
	if(STATS[f].depth< b->stats->depth) STATS[f].depth=b->stats->depth ;
	if(STATS[f].depth_max< b->stats->depth_max) STATS[f].depth_max=b->stats->depth_max ;
	STATS[f].depth_sum+=b->stats->depth ;
	STATS[f].depth_max_sum+=b->stats->depth_max ;
	if(STATS[MAXPLY].depth< b->stats->depth) STATS[MAXPLY].depth=b->stats->depth ;
	if(STATS[MAXPLY].depth_max< b->stats->depth_max) STATS[MAXPLY].depth_max=b->stats->depth_max ;
	STATS[MAXPLY].depth_sum+=b->stats->depth ;
	STATS[MAXPLY].depth_max_sum+=b->stats->depth_max ;

	DEB_1 (printSearchStat(b->stats);)

	return b->bestscore;
}

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

tree_node prev_it_global[TREE_STORE_DEPTH+1];
tree_node o_pv_global[TREE_STORE_DEPTH+1];

int moves_ret[MOVES_RET_MAX];
attack_model ATT_A[TREE_STORE_DEPTH];
int oldPVcheck;


int DEPPLY=30;

int inPV;
unsigned long long COUNT;


#if 1
int TRIG;
#endif

void store_PV_tree(tree_store * tree, tree_node * pv )
{
	int f;
	for(f=0;f<=TREE_STORE_DEPTH;f++) {
		pv[f]=tree->tree[0][f];
		copyBoard(&(tree->tree[0][f]).tree_board, &(pv[f]).tree_board);
		copyAttModel(&(tree->tree[0][f]).att, &(pv[f]).att);
	}
}

void copyTree(tree_store * tree, int level)
{
	int f;
	if(level>TREE_STORE_DEPTH) {
		printf("Error Depth: %d\n", level);
		abort();
	}
	for(f=level+1;f<=TREE_STORE_DEPTH;f++) {
		tree->tree[level][f]=tree->tree[level+1][f];
		copyBoard(&(tree->tree[level+1][f]).tree_board, &(tree->tree[level][f]).tree_board);
		copyAttModel (&(tree->tree[level+1][f]).att, &(tree->tree[level][f]).att);
	}
}

void installHashPV(tree_node * pv, int depth, struct _statistics *s)
{
hashEntry h;
	int f, q, mi, ply;
	// !!!!
//	depth=999;
// neulozime uplne posledni pozici ???
	for(f=0; f<depth; f++) {
		switch(pv[f].move) {
		case DRAW_M:
		case NA_MOVE:
		case WAS_HASH_MOVE:
		case ALL_NODE:
		case BETA_CUT:
		case MATE_M:
			break;
		default:
//			sprintfMove(&(tree->tree[0][f].tree_board), tree->tree[0][f].move, b2);
			h.key=pv[f].tree_board.key;
			h.map=pv[f].tree_board.norm;
			h.value=pv[f].score;
			h.bestmove=pv[f].move;
			storePVHash(&h,f, s);
			break;
		}
	}
}

void clearPV(tree_store * tree) {
	int f;
	for(f=0;f<=TREE_STORE_DEPTH;f++) {
		tree->tree[0][f].move=NA_MOVE;
		tree->tree[f][f].move=NA_MOVE;
	}
}

void sprintfPV(tree_store * tree, int depth, char *buff)
{
	int f, s, mi, ply;
	char b2[1024];

	buff[0]='\0';
	// !!!!
	depth=999;
	for(f=0; f<=depth; f++) {
		switch(tree->tree[0][f].move) {
		case DRAW_M:
		case NA_MOVE:
		case WAS_HASH_MOVE:
		case ALL_NODE:
		case BETA_CUT:
		case MATE_M:
			//				mi=GetMATEDist(tree->tree[0][0].score);
			sprintfMove(&(tree->tree[0][f].tree_board), tree->tree[0][f].move, b2);
			strcat(buff, b2);
//			strcat(buff," ");
			f=depth+1;
			break;
		default:
			sprintfMove(&(tree->tree[0][f].tree_board), tree->tree[0][f].move, b2);
			strcat(buff, b2);
//			strcat(buff," ");
			if(tree->tree[0][f+1].move!=MATE_M) strcat(buff," ");
			break;
		}
	}
	if(isMATE(tree->tree[0][0].score))  {
		ply=GetMATEDist(tree->tree[0][0].score);
		if (ply==0) mi=1;
		else {
			mi= tree->tree[0][0].tree_board.side==WHITE ? (ply+1)/2 : (ply/2)+1;
		}
	} else mi=-1;

	if(mi==-1) sprintf(b2,"EVAL:%d", tree->tree[0][0].score); else sprintf (b2,"MATE in:%d", mi);
	strcat(buff, b2);
}

void printPV(tree_store * tree, int depth)
{
char buff[1024];

	buff[0]='\0';
// !!!!
	sprintfPV(tree, depth, buff);
	LOGGER_1("BeLine: %s\n", buff);

}

void printPV_simple(board *b, tree_store * tree, int depth, struct _statistics * s, struct _statistics * s2)
{
int f, mi, xdepth, ply;
char buff[1024], b2[1024];
unsigned long long int tno;

	buff[0]='\0';
	xdepth=depth;
// !!!!
	xdepth=999;
	for(f=0; f<=xdepth; f++) {
		switch(tree->tree[0][f].move) {
			case DRAW_M:
			case MATE_M:
			case NA_MOVE:
			case WAS_HASH_MOVE:
			case ALL_NODE:
			case BETA_CUT:
				f=xdepth+1;
				break;
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
			mi= tree->tree[0][0].tree_board.side==WHITE ? (ply+1)/2 : (ply/2)+1;
		}
	} else mi=-1;

	tno=readClock()-b->time_start;
	
	if(mi==-1) sprintf(b2,"info score cp %d depth %d nodes %lld time %lld pv %s\n", tree->tree[0][0].score/10, depth, s->movestested+s2->movestested+s->qmovestested+s2->qmovestested, tno, buff);
	else sprintf (b2,"info score mate %d depth %d nodes %lld time %lld pv %s\n", mi, depth, s->movestested+s2->movestested+s->qmovestested+s2->qmovestested, tno, buff);
	tell_to_engine(b2);
	// LOGGER!!!
}


// called inside search
int update_status(board *b){
char buf[512];
	unsigned long long int tnow, slack, xx;
//	sprintf(buf, "Node test: %lld,", b->stats.nodes);
//	LOGGER_0("UPDT:",buf,"\n");
	if(b->uci_options.nodes>0) {
		if (b->stats.positionsvisited >= b->uci_options.nodes) engine_stop=1;
		return 0;
	}
	if(b->time_crit==0) return 0;
//tnow milisekundy
// movetime je v milisekundach
//
	tnow=readClock();
	slack=tnow-b->iter_start+1;
	xx=((b->time_crit-slack)*(b->stats.nodes-b->nodes_at_iter_start)/slack/(b->nodes_mask+1))-1;
//	xx=1;
		if ((b->time_crit + b->time_start <= tnow)||(xx<1)){
			sprintf(buf, "Time out loop - time_move_u, %d, %llu, %llu, %lld", b->time_move, b->time_start, tnow, (tnow-b->time_start));
			LOGGER_1("INFO: %s\n",buf);
			engine_stop=1;
		}
	return 0;
}

// called after iteration
int search_finished(board *b){

unsigned long long tnow, slack, slck2,xx;
char buf[512];

	if (engine_stop) {
		LOGGER_1("INFO: Engine stop called\n");
		return 9999;
	}

// moznosti ukonceni hledani
// pokud ponder nebo infinite, tak hledame dal
	if((b->uci_options.infinite==1)||(b->uci_options.ponder==1)) {
		return 0;
	}
// mate in X
// depth
// nodes

	if(b->uci_options.nodes>0) {
		if (b->stats.positionsvisited >= b->uci_options.nodes) return 1;
		else return 0;
	}

// time per move
	if(b->time_crit==0) {
		return 0;
	}

	tnow=readClock();
	slack=tnow-b->iter_start+1;
	slck2=200;
//	xx=(b->stats.nodes*(b->time_crit-slack)/slack)/(b->nodes_mask+1);
	xx=((b->time_crit-slack)*(b->stats.nodes-b->nodes_at_iter_start)/slack/(b->nodes_mask+1))-1;
//	xx=1;

	if(b->uci_options.movetime>0) {
		if (((b->time_crit + b->time_start) <= tnow)||(xx<1)) {
			LOGGER_1("Time out - movetime, %d, %llu, %llu, %lld\n", b->uci_options.movetime, b->time_start, tnow, (tnow-b->time_start));
			return 2;
		}
	} else if ((b->time_crit>0)) {
		if ((tnow - b->time_start) > b->time_crit){
			LOGGER_1("Time out - time_move, %d, %llu, %llu, %lld\n", b->time_crit, b->time_start, tnow, (tnow-b->time_start));
			return 3;
		} else {
			// konzerva
			if(b->uci_options.movestogo==1) return 0;
			//		if((3.5*slack)>(b->time_crit-slack)) {
			if((((tnow-b->time_start)*2)>b->time_crit)||(((tnow-b->time_start)*1.5)>b->time_move)||(xx<1)) {
				LOGGER_1("Time out run - time_move, %d, %llu, %llu, %lld\n", b->time_move, b->time_start, tnow, (tnow-b->time_start));
				return 33;
			}
		}
	}
	

	b->iter_start=tnow;
	b->nodes_at_iter_start=b->stats.nodes;
	return 0;
}

int can_do_NullMove(board *b, attack_model *a, int alfa, int beta, int depth, int ply, int side){
int pieces;

	if((depth<b->pers->NMP_min_depth) || (alfa != (beta-1))) return 0;

	pieces=BitCount((b->norm^b->maps[PAWN])&b->colormaps[b->side]);
// potreba dodelat evaluaci a podminky
// ne sach, ne PV
// mam tam vic nez jen krale a pesce, resp alespon 2 dalsi figury
// score je vetsi rovno beta
// je prostor pro redukci? - To mozna dam do search jako prechod do quiescence
//	return (pieces>3) && (a->sc.complete >= beta);
	return (pieces>=2);
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
 
int can_do_LMR(board *b, attack_model *a, int alfa, int beta, int depth, int ply, int side, move_entry *move)
{
int inch2;
// zakazani LMR - 9999
	if((depth<=b->pers->LMR_remain_depth) || (alfa != beta-1)) return 0;
	if( move->qorder>KILLER_OR) return 0;

// utoci neco na krale strany na tahu?
	inch2=AttackedTo_B(b, b->king[b->side], b->side);
	if(inch2!=0) return 0;

return 1;
}

/*
 * Quiescence looks for quiet positions, ie where no checks, no captures etc take place
 *
 */

int Quiesce(board *b, int alfa, int beta, int depth, int ply, int side, tree_store * tree, search_history *hist, int phase)
{
int bonus[] = { 00, 00, 00, 00, 00, 00, 00, 00, 00, 00 };

	attack_model *att, ATT;
	move_entry move[300];
	char b3[256];
	int  bestmove, cc;
	int val;
	int depth_idx, sc_need;

	move_entry *m, *n;
	int opside;
	int legalmoves, incheck, talfa, tbeta, gmr;
	int best, scr;
	int tc;

	int psort;
	int see_res;
	UNDO u;

	oldPVcheck=2;
	
	copyBoard(b, &(tree->tree[ply][ply].tree_board));
	tree->tree[ply][ply].move=NA_MOVE;
	tree->tree[ply+1][ply+1].move=NA_MOVE;
	tree->tree[ply][ply+1].move=NA_MOVE;
	b->stats.qposvisited++;
	b->stats.nodes++;
	if(!(b->stats.nodes & b->nodes_mask)){
		update_status(b);
	}
	
	att=&ATT; 
	att->phase=phase;
	eval_king_checks_all(b, att);
	eval(b, att, b->pers);

	if(side==WHITE) scr=att->sc.complete;
	else scr=0-att->sc.complete;

	if(b->pers->use_quiesce==0) return scr;
	if(ply>TREE_STORE_DEPTH) return scr;

	if (is_draw(b, att, b->pers)>0) {
		tree->tree[ply][ply].move=DRAW_M;
		return 0;
	}

	best=scr;
	if(engine_stop!=0) {
		return scr;
	}

	opside = (side == WHITE) ? BLACK : WHITE;
	gmr=GenerateMATESCORE(ply);
	
	// is opposite side in check ?
	if(isInCheck_Eval(b, att, opside)!=0) {
		tree->tree[ply][ply].move=MATE_M;
		LOGGER_1("ERR: Opside in check!\n");
		printBoardNice(b);
		printPV(tree,ply);
		return gmr;
	}

	// mate distance pruning
	if((gmr) <= alfa) {
		return alfa;
	}
	if(-gmr >= beta) {
		return beta;
	}

	talfa=alfa;
	tbeta=beta;

	// is side to move in check ?
	if(isInCheck_Eval(b, att, side)!=0) {
		incheck=1;
	}	else {
		incheck=0;
		if(scr>=beta) {
			return scr;
		}
		if(scr>talfa) talfa=scr;
	}

	bestmove=NA_MOVE;
	legalmoves=0;

	m = move;
	n = move;

/*
 * Quiescence should consider
 * hash move
 * good / neutral captures
 * promotions
 * checking moves - generate quiet moves giving check
 * all moves when in check
 *
 */

	if(incheck==1){
		generateInCheckMoves(b, att, &m);
		tc=sortMoveList_Init(b, att, DRAW_M, move, m-n, depth, 1 );
		getNSorted(move, tc, 0, 1);
	}
	else {
		generateCaptures(b, att, &m, 0);
		tc=sortMoveList_QInit(b, att, DRAW_M, move, m-n, depth, 1 );
		getNSorted(move, tc, 0, 1);
	}
	
	if(tc<=3) psort=tc;
	else psort=3;

	cc = 0;
	b->stats.qpossiblemoves+=tc;

	depth_idx= (0-depth) > 10 ? 10 : 0-depth;
	sc_need=alfa-best;

	while ((cc<tc)&&(engine_stop==0)) {
		if(psort==0) {
			psort=1;
			getNSorted(move, tc, cc, psort);
		}
		{
			b->stats.qmovestested++;
// check SEE
			see_res=1;
			if((incheck==0) && (((move[cc].qorder>A_OR2)&&(move[cc].qorder<=(A_OR2+800)))
					||((move[cc].qorder>A_OR_N)&&(move[cc].qorder<=(A_OR_N+60))))) {
				see_res=SEE(b, move[cc].move);
				b->stats.qSEE_tests++;
				if(see_res<0) b->stats.qSEE_cuts++;
				else {
					see_res-=(bonus[depth_idx]+sc_need);
				}
			}
			if((see_res)>=0){
				u=MakeMove(b, move[cc].move);
				{
					tree->tree[ply][ply].move=move[cc].move;
					if(legalmoves<b->pers->Quiesce_PVS_full_moves) {
						val = -Quiesce(b, -tbeta, -talfa, depth-1,  ply+1, opside, tree, hist, phase);
					} else {
						val = -Quiesce(b, -(talfa+1), -talfa, depth-1,  ply+1, opside, tree, hist, phase);
						b->stats.zerototal++;
						if(val>talfa && val < tbeta) {
							val = -Quiesce(b, -tbeta, -talfa, depth-1,  ply+1, opside, tree, hist, phase);
							b->stats.zerorerun++;
							if(val<=talfa) b->stats.fhflcount++;
						}
					}
				}
				move[cc].real_score=val;

				if(val>best) {
					best=val;
					bestmove=move[cc].move;
					if(val > talfa) {
						talfa=val;
						if(val >= tbeta) {
							tree->tree[ply][ply+1].move=BETA_CUT;
							UnMakeMove(b, u);
							break;
						}
						else {
							copyTree(tree, ply);
						}
					}
				}
				UnMakeMove(b, u);
				legalmoves++;
			}
			psort--;
		}
		cc++;
	}

// generate checks

	if((incheck==0) && ((b->pers->quiesce_check_depth_limit+depth)>0)) {
		n=m;
		generateQuietCheckMoves(b, att, &m);
		tc=sortMoveList_QInit(b, att, DRAW_M, n, m-n, depth, 1 );
		getNSorted(n, tc, 0, 1);

		if(tc<=3) psort=tc;
		else psort=3;

		cc = 0;
		b->stats.qpossiblemoves+=tc;

		while ((cc<tc)&&(engine_stop==0)) {
			if(psort==0) {
				psort=1;
				getNSorted(n, tc, cc, psort);
			}
			{
				b->stats.qmovestested++;
				see_res=SEE(b, n[cc].move);
				b->stats.qSEE_tests++;
				if(see_res<0) {
					b->stats.qSEE_cuts++;
				}
				else
				{
					u=MakeMove(b, n[cc].move);
					tree->tree[ply][ply].move=n[cc].move;
					if(legalmoves<b->pers->Quiesce_PVS_full_moves) {
						val = -Quiesce(b, -tbeta, -talfa, depth-1,  ply+1, opside, tree, hist, phase);
					} else {
						val = -Quiesce(b, -(talfa+1), -talfa, depth-1,  ply+1, opside, tree, hist, phase);
						b->stats.zerototal++;
						if(val>talfa && val < tbeta) {
							val = -Quiesce(b, -tbeta, -talfa, depth-1,  ply+1, opside, tree, hist, phase);
							b->stats.zerorerun++;
							if(val<=talfa) b->stats.fhflcount++;
						}
					}
					n[cc].real_score=val;
					if(val>best) {
						best=val;
						bestmove=n[cc].move;
						if(val > talfa) {
							talfa=val;
							if(val >= tbeta) {
								tree->tree[ply][ply+1].move=BETA_CUT;
								UnMakeMove(b, u);
								break;
							}
							else {
								copyTree(tree, ply);
							}
						}
					}
					UnMakeMove(b, u);
					legalmoves++;
				}
				psort--;
			}
			cc++;
		}
	}

	if(legalmoves==0) {
		if(incheck==0) {
			best=talfa;
			bestmove=NA_MOVE;
		}	else 	{
// I was mated! So best is big negative number...
			best=0-gmr;
			bestmove=MATE_M;
		}
	}
// restore best
	tree->tree[ply][ply].move=bestmove;
	tree->tree[ply][ply].score=best;

	if(best>=beta) {
		b->stats.failhigh++;
	} else {
		if(best<=alfa){
			b->stats.faillow++;
			tree->tree[ply][ply+1].move=ALL_NODE;
		} else {
			b->stats.failnorm++;
		}
	}
	return best;
}

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
 *
 * takze FailSoft!!!
 * upravovat Alfa - hodnota ktere urcite mohu dosahnout
 * upravovat Beta - hodnota kterou kdyz prekrocim tak si o uroven vyse tah vedouci do teto pozice nevyberou
 * udrzovat aktualni hodnotu nezavisle na A a B
 
 * - best - zatim nejvyssi hodnota
 * - bestmove - odpovidajici tah
 * - val - hodnota prave spocitaneho tahu
 */

int AlphaBeta(board *b, int alfa, int beta, int depth, int ply, int side, tree_store * tree, search_history *hist, int phase, int nulls)
// depth - jak hluboko mam jit, 0 znamena pouze evaluaci pozice, zadne dalsi pultahy
// ply - jak jsem hluboko, 0 jsem v root pozici
{
	int tc,cc, xcc;
	move_entry move[300];
	int bestmove, hashmove;
	move_entry *m, *n;
	int opside, isPV;
	int val, legalmoves, incheck, best, talfa, tbeta, gmr;
	int reduce, extend;
	struct _statistics s, r;

	char b3[256];
	hashEntry hash;

	int psort;
	int ddd=0;
	UNDO u;
	attack_model *att;

	if(b->pers->negamax==0) {
	// nechceme AB search, ale klasicky minimax
		alfa=0-iINFINITY;
		beta=iINFINITY;
	}

	isPV= alfa != beta-1;
	best=0-iINFINITY;
	bestmove=NA_MOVE;

	opside = (side == WHITE) ? BLACK : WHITE;
	copyBoard(b, &(tree->tree[ply][ply].tree_board));

	b->stats.positionsvisited++;
	b->stats.nodes++;
	if(!(b->stats.nodes & b->nodes_mask)){
		update_status(b);
	}
	
// inicializuj zvazovany tah na NA
	tree->tree[ply][ply].move=NA_MOVE;
	tree->tree[ply+1][ply+1].move=NA_MOVE;
	tree->tree[ply][ply+1].move=WAS_HASH_MOVE;
	
	att=&(tree->tree[ply][ply].att);
	att->phase=phase;
	
	eval_king_checks_all(b, att);
	
	if (is_draw(b, att, b->pers)>0) {
		tree->tree[ply][ply].move=DRAW_M;
		return 0; //!!!
	}

	gmr=GenerateMATESCORE(ply);

	// is opposite side in check ?
	if(isInCheck_Eval(b, att, opside)!=0) {
		tree->tree[ply][ply].move=MATE_M;
		LOGGER_1("ERR: Opside in check!\n");
		printBoardNice(b);
		printPV(tree,ply);
		return gmr; //!!!
	}
// mate distance pruning

	talfa=alfa;
	tbeta=beta;
	switch(isMATE(alfa)) {
	case -1:
		if((0-gmr)>=tbeta) return 0-gmr;
		if(talfa<(0-gmr)) talfa=0-gmr;
		break;
	case 1:
		if((gmr-1)<=talfa) return gmr-1;
		if((gmr)<tbeta) tbeta=gmr;
		break;
	default:
		break;
	}

//	clearSearchCnt(&s);
	CopySearchCnt(&s, &(b->stats));
//	b->stats.possiblemoves++;

	// is side to move in check ?
	if(isInCheck_Eval(b, att, side)!=0) {
		incheck=1;
	}	else incheck=0;

	{
//		b->stats.positionsvisited++;
// time to check hash table
// TT CUT off?
		hash.key=b->key;
		hash.map=b->norm;
		hash.scoretype=NO_NULL;
		if(b->pers->use_ttable==1 && (retrieveHash(&hash, side, ply, &(b->stats))!=0)) {
			if(hash.scoretype==NO_NULL) {
				hashmove=DRAW_M;
				nulls=0;
			} else {
				hashmove=hash.bestmove;
//FIXME je potreba nejak ukoncit PATH??
				if(hash.depth>=depth) {
					if((hash.scoretype!=FAILLOW_SC)&&(hash.value>=beta)) {
						b->stats.failhigh++;
						tree->tree[ply][ply].move=hash.bestmove;
						tree->tree[ply][ply].score=hash.value;
//						AddSearchCnt(&(b->stats), &s);
						return hash.value; //!!!
					}
					if((hash.scoretype!=FAILHIGH_SC)&&(hash.value<=alfa)){
						b->stats.faillow++;
						tree->tree[ply][ply].move=hash.bestmove;
						tree->tree[ply][ply].score=hash.value;
//						AddSearchCnt(&(b->stats), &s);
						return hash.value; //!!!
					}
					if(hash.scoretype==EXACT_SC) {
						tree->tree[ply][ply].move=hash.bestmove;
						tree->tree[ply][ply].score=hash.value;
						if(b->pers->use_hash) return hash.value; //!!!
					}
				}
			}
		} else {
			hashmove=DRAW_M;
		}
		
// null move PRUNING
		if(nulls && b->pers->NMP_allowed && (incheck==0) && can_do_NullMove(b, att, talfa, tbeta, depth, ply, side)) {
			u=MakeNullMove(b);
			b->stats.NMP_tries++;
			extend=0;
			reduce=b->pers->NMP_reduction;
			if((depth-reduce+extend-1)>0) {
				val = -AlphaBeta(b, -(talfa+1), -talfa, depth-reduce+extend-1,  ply+1, opside, tree, hist, phase, nulls-1);
			} else {
				val = -Quiesce(b, -(talfa+1), -talfa, depth-reduce+extend-1,  ply+1, opside, tree, hist, phase);
			}
			UnMakeNullMove(b, u);
			if(val>=tbeta) {
				tree->tree[ply][ply].move=NULL_MOVE;
				tree->tree[ply][ply].score=val;
				b->stats.NMP_cuts++;
//				AddSearchCnt(&(b->stats), &s);
				
				hash.key=b->key;
				hash.depth=depth;
				hash.map=b->norm;
				hash.value=val;
				hash.bestmove=NULL_MOVE;
				hash.scoretype=FAILHIGH_SC;
				if(b->pers->use_ttable==1) storeHash(&hash, side, ply, depth, &(b->stats));
				return val; //!!!
			}
		}
		
		if(hashmove==DRAW_M) {
// no hash, if we are deep enough and not in zero window, try IID
// IID, vypnout - 9999
			if((depth>b->pers->IID_remain_depth) && (isPV)) {
				val = AlphaBeta(b, talfa, tbeta, depth-b->pers->IID_remain_depth,  ply, side, tree, hist, phase, 0);
				// still no hash?, try everything!
				if(val < talfa) val = AlphaBeta(b, -iINFINITY, tbeta, depth-b->pers->IID_remain_depth,  ply, side, tree, hist, phase, 0);
				if((b->pers->use_ttable==1) && retrieveHash(&hash, side, ply, &(b->stats))!=0) {
					hashmove=hash.bestmove;
				} else {
					hashmove=DRAW_M;
				}
			}
		}

// generate bitmaps for movegen
		simple_pre_movegen(b, att, b->side);
// to get attacked bitmap of other side. 
// optimalizace: mozno ziskat z predchoziho pul tahy
		simple_pre_movegen(b, att, opside);
		
		tree->tree[ply+1][ply+1].move=NA_MOVE;
		tree->tree[ply][ply+1].move=WAS_HASH_MOVE;

		legalmoves=0;
		m = move;

		if(incheck==1) {
			generateInCheckMoves(b, att, &m);
// vypnuti nastavenim check_extension na 0
		} else {
			generateCaptures(b, att, &m, 1);
			generateMoves(b, att, &m);
		}
		
		n = move;
		tc=sortMoveList_Init(b, att, hashmove, move, m-n, depth, 1 );

		if(tc<=3) psort=tc;
		else {
			psort=3;
		}

		cc = 0;
		getNSorted(move, tc, cc, psort);
		b->stats.possiblemoves+=tc;

// hashed PV test
			{
				char h1[20], h2[20], h3[20];
				int p_op, p_hs, p_cm;
				if(oldPVcheck==1) {
					p_hs=UnPackPPos(hashmove);
					p_cm=UnPackPPos(move[0].move);
					p_op=prev_it_global[ply].move;
					sprintfMoveSimple(p_hs, h1);
					sprintfMoveSimple(p_cm, h2);
					sprintfMoveSimple(p_op, h3);
//					printf("HASHED PVs :%d (%s,%s,%s) ", ply, h2, h1, h3);
					if(depth<=2) { oldPVcheck=2; }
				}
			}
		
		
		// main loop
		while ((cc<tc)&&(engine_stop==0)) {
			extend=0;
			reduce=0;
			if(psort==0) {
				psort=1;
				getNSorted(move, tc, cc, psort);
			}
			b->stats.movestested++;
			u=MakeMove(b, move[cc].move);

// vloz tah ktery aktualne zvazujeme - na vystupu z funkce je potreba nastavit na BESTMOVE!!!
			tree->tree[ply][ply].move=move[cc].move;
//			tree->tree[ply+1][ply+1].move=NA_MOVE;

// is side to move in check
// the same check is duplicated one ply down in eval
			eval_king_checks_all(b, att);
			if(isInCheck_Eval(b ,att, b->side)) {
				extend+=b->pers->check_extension;
			}

// debug check
//			compareDBoards(b, DBOARDS);
//			compareDPaths(tree,DPATHS,ply);

// vypnuti ZERO window - 9999
			if(cc<b->pers->PVS_full_moves) {
				// full window
				if(depth+extend-1 > 0) val = -AlphaBeta(b, -tbeta, -talfa, depth+extend-1,  ply+1, opside, tree, hist, phase, 0);
				else val = -Quiesce(b, -tbeta, -talfa, depth+extend-1,  ply+1, opside, tree, hist, phase);
			} else {
// vypnuti LMR - LMR_start_move - 9999
				if(cc>=b->pers->LMR_start_move && (incheck==0) && can_do_LMR(b, att, talfa, tbeta, depth, ply, side, &(move[cc]))) {
					reduce=b->pers->LMR_reduction;
					b->stats.lmrtotal++;
					b->stats.zerototal++;
// zero window (with reductions)
					if(depth-reduce+extend-1 > 0) val = -AlphaBeta(b, -(talfa+1), -talfa, depth-reduce+extend-1,  ply+1, opside, tree, hist, phase, b->pers->NMP_allowed);
					else val = -Quiesce(b, -(talfa+1), -talfa, depth-reduce+extend-1,  ply+1, opside, tree, hist, phase);
// if alpha raised rerun without reductions, zero window
					if(val>talfa) {
						b->stats.lmrrerun++;
						if(depth+extend-1 > 0) val = -AlphaBeta(b, -(talfa+1), -talfa, depth+extend-1,  ply+1, opside, tree, hist, phase, b->pers->NMP_allowed);
						else val = -Quiesce(b, -(talfa+1), -talfa, depth+extend-1,  ply+1, opside, tree, hist, phase);
						if(val<=talfa) b->stats.fhflcount++;
//alpha raised, full window search
						if(val>talfa && val < tbeta) {
							b->stats.zerorerun++;
							if(depth+extend-1 > 0) val = -AlphaBeta(b, -tbeta, -talfa, depth+extend-1,  ply+1, opside, tree, hist, phase, b->pers->NMP_allowed);
							else val = -Quiesce(b, -tbeta, -talfa, depth+extend-1,  ply+1, opside, tree, hist, phase);
							if(val<=talfa) b->stats.fhflcount++;
						}
					}
				} else {
// zero window without reductions
					if(depth+extend-1 > 0) val = -AlphaBeta(b, -(talfa+1), -talfa, depth+extend-1,  ply+1, opside, tree, hist, phase, b->pers->NMP_allowed);
					else val = -Quiesce(b, -(talfa+1), -talfa, depth+extend-1,  ply+1, opside, tree, hist, phase);
					b->stats.zerototal++;
//alpha raised, full window search
					if(val>talfa && val < tbeta) {
						b->stats.zerorerun++;
						if(depth+extend-1 > 0) val = -AlphaBeta(b, -tbeta, -talfa, depth+extend-1,  ply+1, opside, tree, hist, phase, b->pers->NMP_allowed);
						else val = -Quiesce(b, -tbeta, -talfa, depth+extend-1,  ply+1, opside, tree, hist, phase);
						if(val<=talfa) b->stats.fhflcount++;
					}
				}
			}
			move[cc].real_score=val;
			
			legalmoves++;

			if(val>best) {
				xcc=cc;
				best=val;
				bestmove=move[cc].move;
				if(val > talfa) {
					talfa=val;
					if(val >= tbeta) {
// cutoff
						if(cc==0) b->stats.firstcutoffs++;
						b->stats.cutoffs++;
// record killer
						if((b->pers->use_killer>=1)&&(is_quiet_move(b, att, &(move[cc])))) {
							update_killer_move(ply, move[cc].move);
						}
						tree->tree[ply][ply+1].move=BETA_CUT;
						UnMakeMove(b, u);
						break;
					} else {
						copyTree(tree, ply);
					}
				}
			}
			UnMakeMove(b, u);
			psort--;
			cc++;
		}
		if(legalmoves==0) {
			if(incheck==0) {
// FIXME -- DRAW score, hack - PAWN is 1000
				best=-200;
				bestmove=DRAW_M;
			}	else 	{
				// I was mated! So best is big negative number...
				best=0-gmr;
				bestmove=MATE_M;
			}
		}
		tree->tree[ply][ply].move=bestmove;
		tree->tree[ply][ply].score=best;

// hash use testing
		if(b->pers->use_ttable==1)
		{
			if(hash.depth>=depth) {
				if(hash.scoretype==EXACT_SC) {
					if((hash.bestmove!=bestmove)||(hash.value!=best)) {
		char m1[20],m2[20];
						sprintfMoveSimple(hash.bestmove, m1);
						sprintfMoveSimple(bestmove, m2);
//						printf("HASH mismatch! Hash vs search: %s:%d vs %s:%d\n", m1,hash.value,m2,best);
					}
				}
			}
		}

		// update stats & store Hash

		hash.key=b->key;
		hash.depth=depth;
		hash.map=b->norm;
		hash.value=best;
		hash.bestmove=bestmove;
//!!!! changed talfa & tbeta to alfa & beta
		if(best>=beta) {
			b->stats.failhigh++;
			hash.scoretype=FAILHIGH_SC;
			if((b->pers->use_ttable==1)&&(depth>0)) storeHash(&hash, side, ply, depth, &(b->stats));
		} else {
			if(best<=alfa){
				b->stats.faillow++;
				hash.scoretype=FAILLOW_SC;
				if((b->pers->use_ttable==1)&&(depth>0)) storeHash(&hash, side, ply, depth, &(b->stats));
				tree->tree[ply][ply+1].move=ALL_NODE;
			} else {
				b->stats.failnorm++;
				hash.scoretype=EXACT_SC;
				if((b->pers->use_ttable==1)&&(depth>0)) storeHash(&hash, side, ply, depth, &(b->stats));
			}
		}
	}
	
	DecSearchCnt(&(b->stats), &s, &r);
	AddSearchCnt(&(STATS[ply]), &r);
	return best; //!!!
}

int IterativeSearch(board *b, int alfa, int beta, const int ply, int depth, int side, int start_depth, tree_store * tree)
{
int f, i, l;
char buff[1024], b2[2048], bx[2048];
search_history hist;
struct _statistics s, r, s2;

int reduce;

int tc,cc, v, xcc ;
move_entry move[300], backup[300];
int bestmove, hashmove;
move_entry *m, *n;
int opside;
int legalmoves, incheck, best, talfa, tbeta, nodes_bmove;
int extend;
hashEntry hash;

UNDO u;
attack_model *att, ATT;

tree_node *prev_it;
tree_node *o_pv;
// neni thread safe!!!
		prev_it=prev_it_global;
		o_pv=o_pv_global;


		o_pv[0].move=NA_MOVE;
//		initDBoards(DBOARDS);
//		initDPATHS(b, DPATHS);

		b->bestmove=NA_MOVE;
		b->bestscore=0;
		bestmove=hashmove=NA_MOVE;
		clearSearchCnt(&s);
		clearSearchCnt(&s2);
		clearSearchCnt(&(b->stats));
		clearALLSearchCnt(STATS);
//		b->stats.positionsvisited++;
		
		b->nodes_mask=(1<<b->pers->check_nodes_count)-1;
		b->iter_start=b->time_start;
		b->nodes_at_iter_start=b->stats.nodes;

		opside = (side == WHITE) ? BLACK : WHITE;
		copyBoard(b, &(tree->tree[ply][ply].tree_board));

// make current line end here
		tree->tree[ply][ply].move=NA_MOVE;

		att=&(tree->tree[ply][ply].att);
		att->phase = eval_phase(b);
		eval_king_checks_all(b, att);

		// is opposite side in check ?
		if(isInCheck_Eval(b, att, opside)!=0) {
			DEB_2(printf("Opside in check!\n"));
			tree->tree[ply][ply].move=MATE_M;
//????			
			return MATESCORE;
		}

		// is side to move in check ?
		if(isInCheck_Eval(b, att, side)!=0) {
			incheck=1;
		}	else incheck=0;

		if(side==WHITE) best=att->sc.complete;
		else best=0-att->sc.complete;

// check database of openings
		i=probe_book(b);
		if(i!=NA_MOVE) {
//			printfMove(b, i);
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
		o_pv[ply].move=NA_MOVE; //???
		m = move;
		if(incheck==1) {
			generateInCheckMoves(b, att, &m);
		} else {
			generateCaptures(b, att, &m, 1);
			generateMoves(b, att, &m);
		}
		n = move;

// store moves and ordering
		for(l=0;l<300;l++) {
			backup[l].move=move[l].move;
			backup[l].qorder=move[l].qorder;
		}
		
		alfa=0-iINFINITY;
		beta=iINFINITY;
		talfa=alfa;
		tbeta=beta;
// make hash age by new search not each iteration
		invalidateHash();
// iterate and increase depth gradually
		oldPVcheck=0;
		for(f=start_depth;f<=depth;f++) {
			if(b->pers->negamax==0) {
				alfa=0-iINFINITY;
				beta=iINFINITY;
				talfa=alfa;
				tbeta=beta;
			}
//			clearSearchCnt(&s);
			CopySearchCnt(&s, &(b->stats));
			hashmove=o_pv[ply].move;
			hashmove=NA_MOVE;
			installHashPV(o_pv, f-1, &(b->stats));
			clear_killer_moves();
			xcc=-1;
// (re)sort moves
			hash.key=b->key;
			hash.map=b->norm;
			if(b->pers->use_ttable==1 && (retrieveHash(&hash, side, ply, &(b->stats))!=0)) {
				hashmove=hash.bestmove;
//FIXME je potreba nejak ukoncit PATH??
				if(hash.depth>=depth) {
					if((hash.scoretype!=FAILLOW_SC)&&(hash.value>=tbeta)) {
						b->stats.failhigh++;
						tree->tree[ply][ply].move=hash.bestmove;
						tree->tree[ply][ply].score=hash.value;
					}
					if((hash.scoretype!=FAILHIGH_SC)&&(hash.value<=talfa)){
						b->stats.faillow++;
						tree->tree[ply][ply].move=hash.bestmove;
						tree->tree[ply][ply].score=hash.value;
					}
					if(hash.scoretype==EXACT_SC) {
						tree->tree[ply][ply].move=hash.bestmove;
						tree->tree[ply][ply].score=hash.value;
					}
				}
			} else {
				hashmove=DRAW_M;
			}
		
			tc=sortMoveList_Init(b, att, hashmove, move, m-n, ply, m-n );
			getNSorted(move, tc, 0, tc);
			assert(m!=0);

			b->stats.positionsvisited++;
			b->stats.possiblemoves+=tc;
			b->stats.nodes++;
			b->stats.depth=f-1;

/*
			test for HASH line
			move[0].move should be the same as hashmove which should be the same as prev_it[0].move
*/			
			{
				char h1[20], h2[20], h3[20];
				int p_op, p_hs, p_cm;
				if(oldPVcheck==1) {
					p_hs=UnPackPPos(hashmove);
					p_cm=UnPackPPos(move[0].move);
					p_op=prev_it[0].move;
					sprintfMoveSimple(p_hs, h1);
					sprintfMoveSimple(p_cm, h2);
					sprintfMoveSimple(p_op, h3);
//					printf("HASHED PVi Test: %d:%d (%s,%s,%s) ", f,0, h2, h1, h3);
				}
			}
			
			/*
			 * **********************************************************************************
			 */
			{
				best=0-iINFINITY;
				inPV=1;

// hack
				cc = 0;
// loop over all moves
// inicializujeme line
				tree->tree[ply][ply].move=NA_MOVE;
				tree->tree[ply][ply+1].move=NA_MOVE;
//				tree->tree[ply+1][ply+1].move=NA_MOVE;
				legalmoves=0;
				while ((cc<tc)&&(engine_stop==0)) {
					extend=0;
//					reduce=0;
					if(!(b->stats.nodes & b->nodes_mask)){
						update_status(b);
					}
					nodes_bmove=b->stats.possiblemoves+b->stats.qpossiblemoves;
					b->stats.movestested++;
					u=MakeMove(b, move[cc].move);
// aktualni zvazovany tah					
					tree->tree[ply][ply].move=move[cc].move;

// is side to move in check
// the same check is duplicated one ply down in eval
					eval_king_checks_all(b, att);
					if(isInCheck_Eval(b ,att, b->side)) {
						extend+=b->pers->check_extension;
					}

//					compareDBoards(b, DBOARDS);
//					compareDPaths(tree,DPATHS,ply);

// vypnuti ZERO window - 9999
					if(legalmoves<b->pers->PVS_root_full_moves) {
						// full window
						if((f-1+extend)>0) v = -AlphaBeta(b, -tbeta, -talfa, f-1+extend, 1, opside, tree, &hist, att->phase, b->pers->NMP_allowed);
						else v = -Quiesce(b, -tbeta, -talfa, 0,  1, opside, tree, &hist, att->phase);
					} else {
						if((f-1+extend)>0) v = -AlphaBeta(b, -(talfa+1), -talfa, f-1+extend, 1, opside, tree, &hist, att->phase, b->pers->NMP_allowed);
						else v = -Quiesce(b, -(talfa+1), -talfa, 0,  1, opside, tree, &hist, att->phase);
						b->stats.zerototal++;
		//alpha raised, full window search
						if(v>talfa && v < tbeta) {
							b->stats.zerorerun++;
							if((f+extend)>0) v = -AlphaBeta(b, -tbeta, -talfa, f-1+extend, 1, opside, tree, &hist, att->phase, b->pers->NMP_allowed);
							else v = -Quiesce(b, -tbeta, -talfa, 0,  1, opside, tree, &hist, att->phase);
							if(v<=talfa) b->stats.fhflcount++;
						}
					}
					move[cc].real_score=v;

					inPV=0;
					move[cc].qorder=b->stats.possiblemoves+b->stats.qpossiblemoves-nodes_bmove;
					legalmoves++;
					if(v>best) {
						best=v;
						bestmove=move[cc].move;
						xcc=cc;
						if(v > talfa) {
							talfa=v;
							if(v >= tbeta) {
								if(b->pers->use_aspiration==0) {
									LOGGER_1("ERR: nemelo by jit pres TBETA v rootu\n");
								}
								tree->tree[ply][ply+1].move=BETA_CUT;
								UnMakeMove(b, u);
								xcc=-1;
								break;
							}
							else {
								tree->tree[ply][ply].move=bestmove;
								tree->tree[ply][ply].score=best;
								copyTree(tree, ply);
// best line change								
								if(b->uci_options.engine_verbose>=1) printPV_simple(b, tree, f, &s, &(b->stats));
							}
						}
					}
					UnMakeMove(b, u);
					cc++;
				}
				if(legalmoves==0) {
					if(incheck==0) {
						best=0;
						bestmove=DRAW_M;
					}	else 	{
//????
						best=0-MATESCORE;
						bestmove=MATE_M;
					}
				}
				if(best>beta) {
					b->stats.failhigh++;
				} else {
					if(best<alfa){
						b->stats.faillow++;
					} else {
						b->stats.failnorm++;
					}
				}
			}
// store proper bestmove & score
			tree->tree[ply][ply].move=bestmove;
			tree->tree[ply][ply].score=best;

			
// hack
			store_PV_tree(tree, o_pv);
			/*
			 * **********************************************************************************
			 * must handle unfinished iteration
			 */
			if((engine_stop!=0)&&(f>start_depth)) {
				for(i=0;i<(f-1);i++) tree->tree[ply][i]=prev_it[i];
			} else {
				for(i=0;i<f;i++) prev_it[i]=tree->tree[ply][i];
			}
			
			b->bestmove=tree->tree[ply][ply].move;
			b->bestscore=tree->tree[ply][ply].score;

			oldPVcheck=1;
// restore moves and ordering
			for(l=0;l<300;l++) {
				move[l].move=backup[l].move;
				move[l].qorder=backup[l].qorder;
			}

			DEB_3 (printPV(tree, f));
			DecSearchCnt(&(b->stats),&s,&r);
			AddSearchCnt(&(STATS[0]), &r);
// break only if mate is now - not in qsearch
			if(GetMATEDist(b->bestscore)<f) {
				break;
			}
			if((engine_stop!=0)||(search_finished(b)!=0)) break;
			if((b->pers->use_aspiration>0) && (xcc!=-1)) {
// mame realny tah, pro dalsi iteraci pripravime okno okolo bestscore
				talfa=b->bestscore-b->pers->Values[0][ROOK];
				tbeta=b->bestscore+b->pers->Values[0][ROOK];
			} else
			 {
// faillow, nemame tah vratime okno, jak patri
				talfa=alfa;
				tbeta=beta;
// a provedeme tutez iteraci jeste jednou
// nicmene neni vyreseno, co kdyz bez aspiration window nenajdu tah?
				if(xcc==-1) f--;
			}
// time keeping
		}
		if(b->uci_options.engine_verbose>=1) printPV_simple(b, tree, f, &s, &(b->stats));
		DEB_1 (printSearchStat(&r));
		return b->bestscore;
}

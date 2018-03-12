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

tree_line prev_it_global;
tree_line o_pv_global;

int moves_ret[MOVES_RET_MAX];
attack_model ATT_A[MAXPLY];
int oldPVcheck;

int DEPPLY=30;

int inPV;
unsigned long long COUNT;

#if 1
int TRIG;
#endif

void store_PV_tree(tree_store * tree, tree_line * pv )
{
	int f;
	pv->score=tree->score=tree->tree_board.bestscore=tree->tree[0][0].score;
	tree->tree_board.bestmove=tree->tree[0][0].move;
	copyBoard(&tree->tree_board, &pv->tree_board) ;
	
	for(f=0;f<=MAXPLY;f++) {
		pv->line[f]=tree->tree[0][f];
//		copyBoard(&(tree->tree[0][f]).tree_board, &(pv[f]).tree_board);
	}
}

void restore_PV_tree(tree_line * pv, tree_store * tree )
{
	int f;
	copyBoard(&(pv->tree_board),&(tree->tree_board));
	for(f=0;f<=MAXPLY;f++) {
		tree->tree[0][f]=pv->line[f];
//		copyBoard(&(pv[f]).tree_board, &(tree->tree[0][f]).tree_board);
	}
}

void copyTree(tree_store * tree, int level)
{
	int f, to, from;
	if(level>MAXPLY) {
		printf("Error Depth: %d\n", level);
		abort();
	}

//	to=UnPackTo(tree->tree[level][2].move);
//	from=UnPackFrom(tree->tree[level][2].move);
//	if((from==045)&&(to==035)) {
//		printf ("qq");
//	}

	for(f=level+1;f<=MAXPLY;f++) {
		tree->tree[level][f]=tree->tree[level+1][f];
//		copyBoard(&(tree->tree[level+1][f]).tree_board, &(tree->tree[level][f]).tree_board);
	}
}

void installHashPV(tree_line * pv, board *b, int depth, struct _statistics *s)
{
hashEntry h;
UNDO u[MAXPLY+1];
	int f, q, mi, ply, l;
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
	depth=999;
	l=1;
	f=0;
	while((f<=depth)&&(l!=0)) {
		l=0;
		switch(tree->tree[0][f].move) {
		case DRAW_M:
		case NA_MOVE:
		case WAS_HASH_MOVE:
		case NULL_MOVE:
		case ALL_NODE:
		case BETA_CUT:
		case MATE_M:
		case ERR_NODE:
			sprintfMove(&(tree->tree_board), tree->tree[0][f].move, b2);
			strcat(buff, b2);
//			strcat(buff," ");
//			f=depth+1;
			break;
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
	xdepth=999;
	for(f=0; f<=xdepth; f++) {
		switch(tree->tree[0][f].move) {
			case DRAW_M:
			case NA_MOVE:
			case WAS_HASH_MOVE:
			case NULL_MOVE:
			case ALL_NODE:
			case BETA_CUT:
			case MATE_M:
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
			mi= tree->tree_board.side ==WHITE ? (ply+1)/2 : (ply/2)+1;
		}
	} else mi=-1;

	tno=readClock()-b->run.time_start;
	
	if(mi==-1) sprintf(b2,"info score cp %d depth %d nodes %lld time %lld pv %s", tree->tree[0][0].score/10, depth, s->movestested+s2->movestested+s->qmovestested+s2->qmovestested, tno, buff);
	else sprintf (b2,"info score mate %d depth %d nodes %lld time %lld pv %s", mi, depth, s->movestested+s2->movestested+s->qmovestested+s2->qmovestested, tno, buff);
	tell_to_engine(b2);
	LOGGER_1("BEST: %s\n",b2);
	// LOGGER!!!
}

// called inside search
int update_status(board *b){
	unsigned long long int tnow, slack;
	long long int xx;
//	LOGGER_3("Nodes at check %d\n",b->stats->nodes);
	if(b->uci_options->nodes>0) {
		if (b->stats->positionsvisited >= b->uci_options->nodes) engine_stop=2;
		return 0;
	}
	if(b->run.time_crit==0) return 0;
//tnow milisekundy
// movetime je v milisekundach
//
	tnow=readClock();
//	slack=tnow-b->run.iter_start+1;
//fixme
//s	xx=((b->time_crit-slack)*(b->stats->nodes-b->nodes_at_iter_start)/slack/(b->nodes_mask+1))-1;
	xx=1;
		if (((b->run.time_crit + b->run.time_start) <= tnow)||(xx<1)){
			LOGGER_3("INFO: Time out loop - time_move_u, %d, %llu, %llu, %lld\n", b->run.time_move, b->run.time_start, tnow, (tnow-b->run.time_start));
			engine_stop=3;
		}
	return 0;
}

// called after iteration
int search_finished(board *b){

unsigned long long tnow, tpsd, npsd;
long long trun, nrun, xx;

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
	npsd=b->stats->nodes-b->run.nodes_at_iter_start;

	trun=(long long int)(b->run.time_crit+b->run.time_start-tnow);
// 	nrun=trun*npsd/tpsd;
//  In new iteration it Must be able to search 2.5 more nodes than in current iteration
//  ie nrun>=2.5*npsd
//  xx = 100*trun/tpsd
//  xx >= 250
	xx=100*trun/tpsd;

//	LOGGER_0("Search Time Update tpsd:%d, npsd: %d, trun %d, nrun %d, Nodes_mask %d\n", tpsd, npsd, trun, nrun, b->run.nodes_mask);

	if(b->uci_options->movetime>0) {
		if (((b->run.time_crit + b->run.time_start) <= tnow)) {
			LOGGER_3("Time out - movetime, %d, %llu, %llu, %lld\n", b->uci_options->movetime, b->run.time_start, tnow, (tnow-b->run.time_start));
			return 2;
		}
	} else if ((b->run.time_crit>0)) {
		if ((tnow - b->run.time_start) >= b->run.time_crit){
			LOGGER_3("Time out - time_move, %d, %llu, %llu, %lld\n", b->run.time_crit, b->run.time_start, tnow, (tnow-b->run.time_start));
			return 3;
		} else {
			// konzerva
			if(b->uci_options->movestogo==1) return 0;
			if((((tnow-b->run.time_start)*100)>(b->run.time_move*55))||(xx<250)) {
				LOGGER_3("Time out run - time_move, %d, %llu, %llu, %lld\n", b->run.time_move, b->run.time_start, tnow, (tnow-b->run.time_start));
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
int pieces;
int sc;

	if(depth<b->pers->NMP_min_depth) return 0;
	if (alfa != (beta-1)) return 0;
	pieces=BitCount((b->norm^b->maps[PAWN])&b->colormaps[b->side]);
	if(pieces<2) return 0;
	sc=get_material_eval_f(b,b->pers);
// black to move?
	if(side==1) sc=0-sc;
	if(sc<beta) return 0;
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
  * LMR dont reduce captures, hashmove, killers, non captures with good history
  * checks not reduced normally
  */
int can_do_LMR(board *b, attack_model *a, int alfa, int beta, int depth, int ply, int side, move_entry *move)
{
BITVAR inch2;
// zakazani LMR - 9999
	if((depth<b->pers->LMR_remain_depth) || (alfa != beta-1)) return 0;
	if( move->qorder>A_OR2) return 0;

// utoci neco na krale strany na tahu?
	inch2=AttackedTo_B(b, b->king[b->side], b->side);
	if(inch2!=0) return 0;

return 1;
}

/*
 * Quiescence looks for quiet positions, ie where no checks, no captures etc take place
 *
 */

int Quiesce(board *b, int alfa, int beta, int depth, int ply, int side, tree_store * tree, search_history *hist, int phase, int checks)
{
//int bonus[] = { 0, 500, 1000, 1500, 2000, 3000, 4000, 5000, 6000, 7000 };
int bonus[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

	attack_model *att, ATT;
	move_entry move[300];
	MOVESTORE  bestmove;
	int val,cc, fr, to;
	int depth_idx, sc_need;

	move_entry *m, *n;
	int opside;
	int legalmoves, incheck, talfa, tbeta, gmr;
	int best, scr;
	int movlen;
	int tc;

	int psort;
	int see_res;
	UNDO u;

	oldPVcheck=2;
	
	b->stats->qposvisited++;
	b->stats->nodes++;
	if(!(b->stats->nodes & b->run.nodes_mask)){
		update_status(b);
		if(engine_stop!=0) {
			if(side==WHITE) return 0-iINFINITY;
			else return iINFINITY;;
		}
	}
	
	att=&ATT; 
	att->phase=phase;
	if (is_draw(b, att, b->pers)>0) {
		tree->tree[ply][ply].move=DRAW_M;
		return 0;
	}

	eval_king_checks_all(b, att);
	eval(b, att, b->pers);

	if(side==WHITE) scr=att->sc.complete;
	else scr=0-att->sc.complete;

	tree->tree[ply][ply].move=NA_MOVE;
	tree->tree[ply+1][ply+1].move=NA_MOVE;
	tree->tree[ply][ply+1].move=NA_MOVE;

	if(b->pers->use_quiesce==0) return scr;
	if(ply>=MAXPLY) return scr;
	//	copyBoard(b, &(tree->tree[ply][ply].tree_board));

	val=best=scr;

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

/*
	// mate distance pruning
	if((gmr) <= alfa) {
		return alfa;
	}
	if(-gmr >= beta) {
		return beta;
	}
*/
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

/*
 * m-n has type ptrdiff_t, in reality cannot be more than all moves from a position available, for which int type should suffice
 */
	if(incheck==1){
		generateInCheckMoves(b, att, &m);
		tc=sortMoveList_Init(b, att, DRAW_M, move, (int)(m-n), depth, 1 );
		getNSorted(move, tc, 0, 1);
	}
	else {
		generateCaptures(b, att, &m, 0);
		tc=sortMoveList_QInit(b, att, DRAW_M, move,(int)(m-n), depth, 1 );
//		getNSorted(move, tc, 0, 1);
	}
	
	if(tc<=3) psort=tc;
	else psort=3;

	cc = 0;
	b->stats->qpossiblemoves+=(unsigned int)tc;

	depth_idx= (0-depth) > 10 ? 10 : 0-depth;
	sc_need=talfa-best;

	while ((cc<tc)&&(engine_stop==0)) {
//		if(psort==0) {
//			psort=1;
//			getNSorted(move, tc, cc, psort);
//		}
		{
// check SEE
			see_res=1;
			if((incheck==0)) {
				if(((move[cc].qorder>A_OR2)&&(move[cc].qorder<=(A_OR2+800)))) {
					see_res=SEE(b, move[cc].move);
					b->stats->qSEE_tests++;
					if(see_res<0) b->stats->qSEE_cuts++;
					else {
						see_res-=(bonus[depth_idx]+sc_need);
					}
				} else {
					fr=b->pers->Values[0][b->pieces[UnPackFrom(move[cc].move)]&PIECEMASK] ;
					to=b->pers->Values[0][b->pieces[UnPackTo(move[cc].move)]&PIECEMASK] ;
					see_res=(to-fr)-(bonus[depth_idx]+sc_need);
					
				}
			}
			if((see_res)>=0){
				b->stats->qmovestested++;
				u=MakeMove(b, move[cc].move);
				{
//					tree->tree[ply][ply].move=move[cc].move;
					if(legalmoves<b->pers->Quiesce_PVS_full_moves) {
						val = -Quiesce(b, -tbeta, -talfa, depth-1,  ply+1, opside, tree, hist, phase, checks-1);
					} else {
						val = -Quiesce(b, -(talfa+1), -talfa, depth-1,  ply+1, opside, tree, hist, phase, checks-1);
						b->stats->zerototal++;
						if((val>talfa && val < tbeta)&&(engine_stop==0)) {
							val = -Quiesce(b, -tbeta, -talfa, depth-1,  ply+1, opside, tree, hist, phase, checks-1);
							b->stats->zerorerun++;
							if(val<=talfa) b->stats->fhflcount++;
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
							tree->tree[ply][ply].move=bestmove;
							UnMakeMove(b, u);
							break;
						}
						else {
							tree->tree[ply][ply].move=bestmove;
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

	if((incheck==0) && (checks>0) && (val<tbeta)&&(engine_stop==0)) {
		n=m;
		generateQuietCheckMoves(b, att, &m);
		tc=sortMoveList_QInit(b, att, DRAW_M, n, (int)(m-n), depth, 1 );
//		getNSorted(n, tc, 0, 1);

		if(tc<=3) psort=tc;
		else psort=3;

		cc = 0;
		b->stats->qpossiblemoves+=(unsigned int)tc;

		while ((cc<tc)&&(engine_stop==0)) {
//			if(psort==0) {
//				psort=1;
//				getNSorted(n, tc, cc, psort);
//			}
			{
				see_res=SEE(b, n[cc].move);
				b->stats->qSEE_tests++;
				if(see_res<0) {
					b->stats->qSEE_cuts++;
				}
				else
				{
					b->stats->qmovestested++;
					u=MakeMove(b, n[cc].move);
//					tree->tree[ply][ply].move=n[cc].move;
					if(legalmoves<b->pers->Quiesce_PVS_full_moves) {
						val = -Quiesce(b, -tbeta, -talfa, depth-1,  ply+1, opside, tree, hist, phase, checks-1);
					} else {
						val = -Quiesce(b, -(talfa+1), -talfa, depth-1,  ply+1, opside, tree, hist, phase, checks-1);
						b->stats->zerototal++;
						if((val>talfa && val < tbeta)&&(engine_stop==0)) {
							val = -Quiesce(b, -tbeta, -talfa, depth-1,  ply+1, opside, tree, hist, phase, checks-1);
							b->stats->zerorerun++;
							if(val<=talfa) b->stats->fhflcount++;
						}
					}
					n[cc].real_score=val;
					if(val>best) {
						best=val;
						bestmove=n[cc].move;
						if(val > talfa) {
							talfa=val;
							tree->tree[ply][ply].move=bestmove;
							tree->tree[ply][ply].score=best;
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

/*
	if(legalmoves==0) {
		if(incheck==0) {
			best=talfa;
			bestmove=NA_MOVE;
		}	else 	{
// I was mated! So best is big negative number...
			best=0-gmr;
			bestmove=MATE_M;
		}
//		tree->tree[ply][ply].move=bestmove;
//		tree->tree[ply][ply].score=best;
	}
*/
// restore best
	tree->tree[ply][ply].move=bestmove;
	tree->tree[ply][ply].score=best;

	if(best>=beta) {
		b->stats->failhigh++;
	} else {
		if(best<=alfa){
			b->stats->faillow++;
			tree->tree[ply][ply+1].move=ALL_NODE;
		} else {
			b->stats->failnorm++;
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
	MOVESTORE bestmove, hashmove;
	move_entry *m, *n;
	int opside, isPV;
	int val, legalmoves, incheck, best, talfa, tbeta, gmr, aftermovecheck, valn, cutn;
	int reduce, extend, ext;
	int reduce_o, extend_o;
	struct _statistics s, r;

	hashEntry hash;

	int psort;
	UNDO u;
	attack_model *att, ATT;

	if(b->pers->negamax==0) {
	// nechceme AB search, ale klasicky minimax
		alfa=0-iINFINITY;
		beta=iINFINITY;
	}

	isPV= alfa != beta-1;
	best=0-iINFINITY;
	bestmove=NA_MOVE;

	opside = (side == WHITE) ? BLACK : WHITE;
//	copyBoard(b, &(tree->tree[ply][ply].tree_board));

	b->stats->positionsvisited++;
	b->stats->nodes++;
// inicializuj zvazovany tah na NA
	tree->tree[ply][ply].move=NA_MOVE;
	tree->tree[ply+1][ply+1].move=ALL_NODE;
	tree->tree[ply][ply+1].move=WAS_HASH_MOVE;
	if(!(b->stats->nodes & b->run.nodes_mask)){
		update_status(b);
		if(engine_stop!=0) {
			return best;
		}
	}
	
//	att=&(tree->tree[ply][ply].att);
	att=&ATT;
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
/*
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
*/
//	clearSearchCnt(&s);
	CopySearchCnt(&s, b->stats);
//	b->stats->possiblemoves++;

	hashmove=DRAW_M;
// time to check hash table
// TT CUT off?
	if(b->pers->use_ttable==1) {
		hash.key=b->key;
		hash.map=b->norm;
		hash.scoretype=NO_NULL;
		if(retrieveHash(b->hs, &hash, side, ply, depth, b->pers->use_ttable_prev, b->stats)!=0) {
			hashmove=hash.bestmove;
//FIXME je potreba nejak ukoncit PATH??
/*
 * FAILLOW_SC - uz jsem vsechny tahy nekdy prosel a nedostal jsem se pres uvedenou hodnotu - vice to nemuze nikdy byt
 * FAILHIGH_SC - minimalne toto skore dana pozice ma
 * EXACT_SC - presne toto skore ma pozice
 *
 * opravdu je potreba vyloucit FAILLOW_SC, kdyz je pres beta?
 */
			if(hash.depth>=depth) {
				if((hash.scoretype!=FAILLOW_SC)&&(hash.value>=beta)) {
					b->stats->failhigh++;
					tree->tree[ply][ply].move=hash.bestmove;
					tree->tree[ply][ply].score=hash.value;
					best=hash.value;
					goto ABFINISH;
				}
				if((hash.scoretype!=FAILHIGH_SC)&&(hash.value<=alfa)){
					b->stats->faillow++;
					tree->tree[ply][ply].move=hash.bestmove;
					tree->tree[ply][ply].score=hash.value;
					best=hash.value;
					goto ABFINISH;
				}
				if(hash.scoretype==EXACT_SC) {
					tree->tree[ply][ply].move=hash.bestmove;
					tree->tree[ply][ply].score=hash.value;
					LOGGER_1("HASH: %d %d:%d %d %d\n", hash.value, alfa, beta, depth, hash.depth);
					if(b->pers->use_hash) {
//						copyTree(tree, ply);
//						if((b->pers->use_ttable==1)&&(depth>0)&&(engine_stop==0)) storeHash(&hash, side, ply, depth, b->stats);
						best=hash.value;
						goto ABFINISH;
					}
				}
			} else {
				if((b->pers->NMP_allowed>0)
					&& (hash.scoretype==FAILLOW_SC)
					&& (hash.depth>= (depth - b->pers->NMP_reduction - 1))
					&& (hash.value<beta)) nulls=0;
			}
		  }
	} else {
			hashmove=DRAW_M;
	}

	// is side to move in check ?
	if(isInCheck_Eval(b, att, side)!=0) {
		incheck=1;
	}	else incheck=0;
	reduce_o=0;
	extend_o=0;
	cutn=0;
	valn=0;
	val=0;
	
// null move PRUNING
	if((nulls>0) && (b->pers->NMP_allowed>0) && (incheck==0) && (can_do_NullMove(b, att, talfa, tbeta, depth, ply, side)!=0)) {
		u=MakeNullMove(b);
		b->stats->NMP_tries++;
		reduce=b->pers->NMP_reduction;
		ext=depth-reduce-1;
		if((ext)>0) {
			val = -AlphaBeta(b, -beta, -beta+1, ext, ply+1, opside, tree, hist, phase, nulls-1);
		} else {
			val = -Quiesce(b, -beta, -beta+1, ext,  ply+1, opside, tree, hist, phase, b->pers->quiesce_check_depth_limit);
		}
		UnMakeNullMove(b, u);
		if(val>=tbeta) {
			tree->tree[ply][ply].move=NULL_MOVE;
			tree->tree[ply][ply].score=val;
			b->stats->NMP_cuts++;
//			AddSearchCnt(b->stats, &s);
			
			hash.key=b->key;
			hash.depth=(int16_t)depth;
			hash.map=b->norm;
			hash.value=val;
			hash.bestmove=NULL_MOVE;
			hash.scoretype=FAILHIGH_SC;
			if((b->pers->use_ttable==1)&&(engine_stop==0)) storeHash(b->hs, &hash, side, ply, depth-reduce, b->stats);
			if(b->pers->NMP_search_reduction==0) {
				best=val;
				goto ABFINISH;
			} else if(b->pers->NMP_search_reduction==-1) {
				reduce_o=0;
				hashmove=DRAW_M;
			} else {
				reduce_o=b->pers->NMP_search_reduction;
			}
			cutn=1;
			valn=val;
		}
	} else {
		if((nulls<=0) && b->pers->NMP_allowed) nulls=b->pers->NMP_allowed;
	}
		
	if(hashmove==DRAW_M) {
// no hash, if we are deep enough and not in zero window, try IID
// IID, vypnout - 9999
		if((b->pers->IID_remain_depth<depth) && (isPV)&&(b->pers->use_ttable==1)) {
			val = AlphaBeta(b, talfa, tbeta, depth-b->pers->IID_remain_depth,  ply, side, tree, hist, phase, nulls);
				// still no hash?, try everything!
			if(val < talfa) val = AlphaBeta(b, -iINFINITY, tbeta, depth-b->pers->IID_remain_depth,  ply, side, tree, hist, phase, nulls);
			if(retrieveHash(b->hs, &hash, side, ply, depth, b->pers->use_ttable_prev, b->stats)!=0) {
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
// vypnuti nastavenim check_extension na 0!!!
	} else {
		generateCaptures(b, att, &m, 1);
		generateMoves(b, att, &m);
	}
	
	n = move;
	tc=sortMoveList_Init(b, att, hashmove, move, (int)(m-n), depth, 1 );
	if(tc==1) extend_o++;
	if(tc<=3) psort=tc;
	else {
		psort=3;
	}
	cc = 0;
	getNSorted(move, tc, cc, psort);
	b->stats->possiblemoves+=(unsigned int)tc;

// hashed PV test
/*
		{
			char h1[20], h2[20], h3[20];
			MOVESTORE p_op, p_hs, p_cm;
			if(oldPVcheck==1) {
				p_hs=UnPackPPos(hashmove);
				p_cm=UnPackPPos(move[0].move);
				p_op=prev_it_global[ply].move;
				sprintfMoveSimple(p_hs, h1);
				sprintfMoveSimple(p_cm, h2);
				sprintfMoveSimple(p_op, h3);
				printf("HASHED PVs :%d (%s,%s,%s) ", ply, h2, h1, h3);
				if(depth<=2) { oldPVcheck=2; }
			}
		}
*/
	
		// main loop

//	tree->tree[ply][ply].move=ALL_NODE;

	while ((cc<tc)&&(engine_stop==0)) {
		extend=extend_o;
		reduce=reduce_o;
		if(psort==0) {
			psort=2;
			getNSorted(move, tc, cc, psort);
		}
		b->stats->movestested++;
		u=MakeMove(b, move[cc].move);

// vloz tah ktery aktualne zvazujeme - na vystupu z funkce je potreba nastavit na BESTMOVE!!!
// proto abychom v kazdem okamziku meli konzistetni prave pocitanou variantu od roota
		tree->tree[ply][ply].move=move[cc].move;
//		tree->tree[ply+1][ply+1].move=NA_MOVE;

// is side to move in check
// the same check is duplicated one ply down in eval
		eval_king_checks_all(b, att);
		aftermovecheck=0;
		if(isInCheck_Eval(b ,att, b->side)) {
//			if(SEE_0(b, move[cc].move)>0) {
				extend+=b->pers->check_extension;
//			}
			aftermovecheck=1;
		}
// debug check
//		compareDBoards(b, DBOARDS);
//		compareDPaths(tree,DPATHS,ply);
// vypnuti ZERO window - 9999
// do not LMR reduce PVS
		ext=depth-reduce+extend-1;
		if(cc<b->pers->PVS_full_moves) {
			// full window
			if((ext > 0)&&(ply<MAXPLY)) val = -AlphaBeta(b, -tbeta, -talfa, ext,  ply+1, opside, tree, hist, phase, nulls);
			else val = -Quiesce(b, -tbeta, -talfa, ext, ply+1, opside, tree, hist, phase, b->pers->quiesce_check_depth_limit);
		} else {
// vypnuti LMR - LMR_start_move - 9999
// do not reduce extended, incheck, giving check
			if(cc>=b->pers->LMR_start_move && (incheck==0) && (aftermovecheck==0) &&(extend==extend_o) && can_do_LMR(b, att, talfa, tbeta, depth, ply, side, &(move[cc]))) {
				reduce+=b->pers->LMR_reduction;
				b->stats->lmrtotal++;
				b->stats->zerototal++;
// zero window (with LMR reductions)
				ext=depth-reduce+extend-1;
				if(ext > 0) val = -AlphaBeta(b, -(talfa+1), -talfa, ext,  ply+1, opside, tree, hist, phase, nulls);
				else val = -Quiesce(b, -(talfa+1), -talfa, ext,  ply+1, opside, tree, hist, phase,b->pers->quiesce_check_depth_limit);
// if alpha raised rerun without reductions, zero window
				if(val>talfa) {
					ext+=b->pers->LMR_reduction;
					b->stats->lmrrerun++;
					if(ext > 0) val = -AlphaBeta(b, -(talfa+1), -talfa, ext, ply+1, opside, tree, hist, phase, nulls);
					else val = -Quiesce(b, -(talfa+1), -talfa, ext,  ply+1, opside, tree, hist, phase, b->pers->quiesce_check_depth_limit);
					if(val<=talfa) b->stats->fhflcount++;
//alpha raised, full window search
					if(val>talfa && val < tbeta) {
						b->stats->zerorerun++;
						if(ext > 0) val = -AlphaBeta(b, -tbeta, -talfa, ext,  ply+1, opside, tree, hist, phase, nulls);
						else val = -Quiesce(b, -tbeta, -talfa, ext,  ply+1, opside, tree, hist, phase, b->pers->quiesce_check_depth_limit);
						if(val<=talfa) b->stats->fhflcount++;
					}
				}
			} else {
// zero window without LMR reductions
				if(ext > 0) val = -AlphaBeta(b, -(talfa+1), -talfa, ext,  ply+1, opside, tree, hist, phase, nulls);
				else val = -Quiesce(b, -(talfa+1), -talfa, ext,  ply+1, opside, tree, hist, phase, b->pers->quiesce_check_depth_limit);
				b->stats->zerototal++;
//alpha raised, full window search
				if(val>talfa && val < tbeta) {
					b->stats->zerorerun++;
					if(ext > 0) val = -AlphaBeta(b, -tbeta, -talfa, ext,  ply+1, opside, tree, hist, phase, nulls);
					else val = -Quiesce(b, -tbeta, -talfa, depth+extend-1,  ply+1, opside, tree, hist, phase, b->pers->quiesce_check_depth_limit);
					if(val<=talfa) b->stats->fhflcount++;
				}
			}
		}
		move[cc].real_score=val;
		legalmoves++;

//		tree->tree[ply][ply].move=ERR_NODE;

		if((val>best)&&(engine_stop==0)) {
//			xcc=cc;
			best=val;
			bestmove=move[cc].move;
			if(val > talfa) {
				talfa=val;
//				tree->tree[ply][ply].move=bestmove;
				tree->tree[ply][ply].score=best;
				if(val >= tbeta) {
// cutoff
					if(cc==0) b->stats->firstcutoffs++;
					b->stats->cutoffs++;
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

		// update stats & store Hash

	hash.key=b->key;
	hash.depth=(int16_t)depth;
	hash.map=b->norm;
	hash.value=best;
	hash.bestmove=bestmove;
//!!!! changed talfa & tbeta to alfa & beta
	if(best>=beta) {
		b->stats->failhigh++;
		hash.scoretype=FAILHIGH_SC;
		if((b->pers->use_ttable==1)&&(depth>0)&&(engine_stop==0)) storeHash(b->hs, &hash, side, ply, depth, b->stats);
	} else {
		if(best<=alfa){
			b->stats->faillow++;
			hash.scoretype=FAILLOW_SC;
			if((b->pers->use_ttable==1)&&(depth>0)&&(engine_stop==0)) storeHash(b->hs, &hash, side, ply, depth, b->stats);
			tree->tree[ply][ply+1].move=ALL_NODE;
		} else {
			b->stats->failnorm++;
			hash.scoretype=EXACT_SC;
			if((b->pers->use_ttable==1)&&(depth>0)&&(engine_stop==0)) storeHash(b->hs, &hash, side, ply, depth, b->stats);
		}
	}
	if(cutn!=0) LOGGER_0("NMP test alfa %d, beta %d, val %d, NCUT %d, NVAL %d, depth %d\n", alfa, beta, val, cutn, valn, depth);
	assert((cutn==0) ? 1:(valn>=beta));
ABFINISH:
	DecSearchCnt(b->stats, &s, &r);
	AddSearchCnt(&(STATS[ply]), &r);
	return best; //!!!
}

int IterativeSearch(board *b, int alfa, int beta, const int ply, int depth, int side, int start_depth, tree_store * tree)
{
	int f, l;
	search_history hist;
	struct _statistics s, r, s2;

	int reduce;

	int tc,cc, v, xcc ;
	move_entry move[300], backup[300];
	MOVESTORE bestmove, hashmove, i, t1pbestmove, t2pbestmove;
	move_entry *m, *n;
	int opside;
	int legalmoves, incheck, best, talfa, tbeta, tcheck, t1pbest, t2pbest;
	unsigned long long int nodes_bmove;
	int extend;
	hashEntry hash;

	UNDO u;
	attack_model *att, ATT;
	unsigned long long tnow;

//	tree_line *prev_it;
	tree_line *o_pv;
	// neni thread safe!!!
//	prev_it=&prev_it_global;
	o_pv=&o_pv_global;
//	clear_killer_moves();


	//		b->time_start=readClock();
	//		depth=1;

	//		o_pv[0].move=NA_MOVE;
	//		initDBoards(DBOARDS);
	//		initDPATHS(b, DPATHS);

	best=0-iINFINITY;
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
//	clearALLSearchCnt(STATS);
	//		b->stats->positionsvisited++;

	b->run.nodes_mask=(1<<b->pers->check_nodes_count)-1;
	b->run.iter_start=b->run.time_start;
	b->run.nodes_at_iter_start=b->stats->nodes;

	opside = (side == WHITE) ? BLACK : WHITE;
	copyBoard(b, &(tree->tree_board ));

	// make current line end here
	tree->tree[ply][ply].move=NA_MOVE;
	tree->tree[ply+1][ply+1].move=NA_MOVE;
	tree->tree[ply][ply+1].move=NA_MOVE;

	//		att=&(tree->tree[ply][ply].att);
	att=&ATT;
	att->phase = eval_phase(b, b->pers);
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
	i=NA_MOVE;
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
	o_pv->line[ply].move=NA_MOVE; //???
	m = move;
	if(incheck==1) {
		generateInCheckMoves(b, att, &m);
	} else {
		generateCaptures(b, att, &m, 1);
		generateMoves(b, att, &m);
	}
	n = move;

	tc=(int)(m-n);
	// store moves and ordering
//	for(l=0;l<tc;l++) {
//		backup[l].move=move[l].move;
//		backup[l].qorder=move[l].qorder;
//	}

	alfa=0-iINFINITY;
	beta=iINFINITY;
	talfa=alfa;
	tbeta=beta;
	// make hash age by new search not each iteration
	invalidateHash(b->hs);
	// iterate and increase depth gradually
	oldPVcheck=0;

	// initial sort according
	cc = 0;
//	tc=(int)(m-n);
#if 0
	while (cc<tc) {
		u=MakeMove(b, move[cc].move);
		v = -Quiesce(b, -tbeta, -talfa, 0,  1, opside, tree, &hist, att->phase, 0);
		move[cc].qorder=v;
		UnMakeMove(b, u);
		cc++;
	}
#endif

	if(tc==1) {
		start_depth=0;
		depth=1;
	}

	if(depth>MAXPLY) depth=MAXPLY;

	for(f=start_depth;f<=depth;f++) {
		if(b->pers->use_ttable_prev==0) invalidateHash(b->hs);
		if(b->pers->negamax==0) {
			alfa=0-iINFINITY;
			beta=iINFINITY;
			talfa=alfa;
			tbeta=beta;
		}
		//			clearSearchCnt(&s);
		CopySearchCnt(&s, b->stats);
		installHashPV(o_pv, b, f-1, b->stats);
		clear_killer_moves();
		xcc=-1;
		// (re)sort moves
		hash.key=b->key;
		hash.map=b->norm;
		hashmove=o_pv->line[0].move;
		tc=sortMoveList_Init(b, att, hashmove, move, (int)(m-n), ply, (int)(m-n) );
		getNSorted(move, tc, 0, tc);

		b->stats->positionsvisited++;
		b->stats->possiblemoves+=(unsigned int)tc;
		b->stats->nodes++;
		b->stats->depth=f-1;

		/*
			test for HASH line
			move[0].move should be the same as hashmove which should be the same as prev_it[0].move
		 */
		/*
			{
				char h1[20], h2[20], h3[20];
				MOVESTORE p_op, p_hs, p_cm;
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
		 */
		/*
		 * **********************************************************************************
		 */
		best=0-iINFINITY;
		inPV=1;

		// hack
		cc = 0;
		// loop over all moves
		// inicializujeme line
		tree->tree[ply][ply].move=NA_MOVE;
		tree->tree[ply][ply+1].move=NA_MOVE;
		tree->tree[ply+1][ply+1].move=NA_MOVE;
//				tree->tree[ply+1][ply+1].move=NA_MOVE;
		legalmoves=0;
		while ((cc<tc)&&(engine_stop==0)) {
			extend=0;
			if(!(b->stats->nodes & b->run.nodes_mask)){
				update_status(b);
			}
			nodes_bmove=b->stats->possiblemoves+b->stats->qpossiblemoves;
			b->stats->movestested++;
			u=MakeMove(b, move[cc].move);
// store evaluated move, to have current line actual... Restore to bestmove at the end of the function
			tree->tree[ply][ply].move=move[cc].move;

			// is side to move in check
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
				else v = -Quiesce(b, -tbeta, -talfa, 0,  1, opside, tree, &hist, att->phase, b->pers->quiesce_check_depth_limit);
			} else {
				if((f-1+extend)>0) v = -AlphaBeta(b, -(talfa+1), -talfa, f-1+extend, 1, opside, tree, &hist, att->phase, b->pers->NMP_allowed);
				else v = -Quiesce(b, -(talfa+1), -talfa, 0,  1, opside, tree, &hist, att->phase, b->pers->quiesce_check_depth_limit);
				b->stats->zerototal++;
				//alpha raised, full window search
				if(v>talfa && v < tbeta) {
					b->stats->zerorerun++;
					if((f+extend)>0) v = -AlphaBeta(b, -tbeta, -talfa, f-1+extend, 1, opside, tree, &hist, att->phase, b->pers->NMP_allowed);
					else v = -Quiesce(b, -tbeta, -talfa, 0,  1, opside, tree, &hist, att->phase, b->pers->quiesce_check_depth_limit);
					if(v<=talfa) b->stats->fhflcount++;
				}
			}
			move[cc].real_score=v;
			inPV=0;
			if(engine_stop==0) {
				move[cc].qorder=(b->stats->possiblemoves+b->stats->qpossiblemoves-nodes_bmove);
				legalmoves++;
				if(v>best) {
					best=v;
					bestmove=move[cc].move;
					xcc=cc;
					if(v > talfa) {
						talfa=v;
//						tree->tree[ply][ply].move=bestmove;
						tree->tree[ply][ply].score=best;
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
							copyTree(tree, ply);
							// best line change
//							printPV_simple(b, tree, f, b->side , &s, b->stats);
							if(b->uci_options->engine_verbose>=1) printPV_simple(b, tree, f, b->side , &s, b->stats);
						}
					}
				}
				UnMakeMove(b, u);
				cc++;
			} else {
				UnMakeMove(b, u);
			}
		} //moves testing
		tree->tree[ply][ply].move=bestmove;
		tree->tree[ply][ply].score=best;
		/*
		 * Co kdyz je iterace nedokoncena, co s tim?
		 */
		if(engine_stop==0) {
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

			// store proper bestmove & score
			// update stats & store Hash

			hash.key=b->key;
			hash.depth=(int16_t)f;
			hash.map=b->norm;
			hash.value=best;
			hash.bestmove=bestmove;
			//!!!! changed talfa & tbeta to alfa & beta
			if(best>=beta) {
				b->stats->failhigh++;
				hash.scoretype=FAILHIGH_SC;
				if((b->pers->use_ttable==1)&&(f>0)&&(engine_stop==0)) storeHash(b->hs, &hash, side, ply, f, b->stats);
			} else {
				if(best<=alfa){
					b->stats->faillow++;
					hash.scoretype=FAILLOW_SC;
					if((b->pers->use_ttable==1)&&(f>0)&&(engine_stop==0)) storeHash(b->hs, &hash, side, ply, f, b->stats);
					tree->tree[ply][ply+1].move=ALL_NODE;
				} else {
					b->stats->failnorm++;
					hash.scoretype=EXACT_SC;
					if((b->pers->use_ttable==1)&&(f>0)&&(engine_stop==0)) storeHash(b->hs, &hash, side, ply, f, b->stats);
				}
			}
			store_PV_tree(tree, o_pv);
//			for(i=0;i<f;i++) prev_it[i]=tree->tree[ply][i];
		} // finished iteration
		else {
			if(xcc>-1) {
				t1pbest=best;
				t1pbestmove=bestmove;
			} else {
				t1pbestmove=move[0].move;
				t1pbest=-MATEMAX;
			}
			if(f>start_depth) {
				t2pbestmove=o_pv->tree_board.bestmove;
				t2pbest=o_pv->tree_board.bestscore;
//				restore_PV_tree(o_pv, tree);
			} else {
				t2pbestmove=move[0].move;
				t2pbest=-MATEMAX;
			}
			if(t1pbest>=t2pbest) {
				tree->tree[ply][ply].move=t1pbestmove;
				tree->tree[ply][ply].score=t1pbest;
				tree->tree[ply][ply+1].move=NA_MOVE;
			} else {
				tree->tree[ply][ply].move=t2pbestmove;
				tree->tree[ply][ply].score=t2pbest;
				restore_PV_tree(o_pv, tree);
			}
//			for(i=0;i<(f-1);i++) tree->tree[ply][i]=prev_it[i];
		}

		b->bestmove=tree->tree[ply][ply].move;
		b->bestscore=tree->tree[ply][ply].score;

		oldPVcheck=1;
		// restore moves and ordering
		//			for(l=0;l<tc;l++) {
		//				move[l].move=backup[l].move;
		//				move[l].qorder=backup[l].qorder;
		//			}

//		compareBoardSilent(b, &(tree->tree_board));

		DEB_4 (printPV(tree, f));
		DecSearchCnt(b->stats,&s,&r);
		AddSearchCnt(&(STATS[0]), &r);
		// break only if mate is now - not in qsearch
		if(GetMATEDist(b->bestscore)<f) {
			break;
		}

		if((engine_stop!=0)||(search_finished(b)!=0)) break;
		// time keeping
		if((b->pers->use_aspiration>0) && (xcc!=-1)) {
			// mame realny tah, pro dalsi iteraci pripravime okno okolo bestscore
			talfa=b->bestscore-b->pers->Values[0][PAWN];
			tbeta=b->bestscore+b->pers->Values[0][PAWN];
		} else {
			// faillow, nemame tah vratime okno, jak patri
			talfa=alfa;
			tbeta=beta;
			// a provedeme tutez iteraci jeste jednou
			// nicmene neni vyreseno, co kdyz bez aspiration window nenajdu tah?
			if(xcc==-1) f--;
		}
		if(b->uci_options->engine_verbose>=1) printPV_simple(b, tree, f,b->side, &s, b->stats);
//		DEB_3 (printSearchStat(b->stats));
//		DEB_3 (tnow=readClock());
//		DEB_3 (LOGGER_1("TIMESTAMP: Start: %llu, Stop: %llu, Diff: %lld milisecs\n", b->run.time_start, tnow, (tnow-b->run.time_start)));
	} //deepening
	if(b->uci_options->engine_verbose>=1) printPV_simple(b, tree, f,b->side, &s, b->stats);
	DEB_1 (printSearchStat(b->stats));
	DEB_1 (tnow=readClock());
	DEB_1 (LOGGER_1("TIMESTAMP: Start: %llu, Stop: %llu, Diff: %lld milisecs\n", b->run.time_start, tnow, (tnow-b->run.time_start)));
	return b->bestscore;
}

/*
 *
 * $Id: evaluate.h,v 1.14.4.11 2007/01/31 19:59:53 mrt Exp $
 *
 */
 
#ifndef EVALUATE_H
#define EVALUATE_H

#include "bitmap.h"
#include "pers.h"

int quickEval(board * b, int move, int from, int to, unsigned char pfrom, unsigned char pto, int spec);
//int eval_king_quiet_old(board *b, king_eval *ke, personality *p, int side);
int eval_king_checks(board *b, king_eval *ke, personality *p, int side);
int eval(board *b, attack_model *a, personality *p);
uint8_t eval_phase(board *b, personality *);
int mat_info(int8_t *);
int mat_faze(uint8_t *);
int isDrawBy50(board * b);
int is_draw(board *b, attack_model *a, personality *p);
int create_attack_model(board * b, attack_model * att);
int create_attack_model2(board * b, attack_model * att);
int create_attack_model3(board * b, attack_model * att);
int EvaluateOwn(board * b, int pos);
int EvalPawnStruct(board * b, attack_model * att, int pos, int side, int opside, score_type *score);
int copyAttModel(attack_model *source, attack_model *dest);
int eval_king_checks_all(board * b, attack_model *a);
int simple_pre_movegen(board *b, attack_model *a, int side);

int get_material_eval_f(board *, personality *);

int TactPawn(board * b, attack_model * att, int pos, int side, int opside, score_type *score);
int TactKing(board * b, attack_model * att, int pos, int side, int opside, score_type *score);
int TactBishop(board * b, attack_model * att, int pos, int side, int opside, score_type *score);
int TactQueen(board * b, attack_model * att, int pos, int side, int opside, score_type *score);
int TactRook(board * b, attack_model * att, int pos, int side, int opside, score_type *score);
int TactKnight(board * b, attack_model * att, int pos, int side, int opside, score_type *score);

score_type DeepEval3(board * b, attack_model * att);

int meval_table_gen(meval_t *, personality *, int);
int check_mindex_validity(board *, int);
int MVVLVA_gen(int table[ER_PIECE+2][ER_PIECE], _values Values);
int SEE(board * b, int m);
int SEE_0(board * b, int to);
int PSQSearch(int , int , int , int , uint8_t , personality *);


/* dame xQ */
/*
#define NW_MI 1
#define NB_MI 3
#define BWL_MI 9
#define BBL_MI 18
#define BWD_MI 36
#define BBD_MI 72
#define RW_MI 144
#define RB_MI 432
#define QW_MI 1296
#define QB_MI 2592
#define PW_MI 5184
#define PB_MI 46656
*/

 
#define NW_MI 1
#define NB_MI (NW_MI*3)
#define BWL_MI (NB_MI*3)
#define BBL_MI (BWL_MI*2)
#define BWD_MI (BBL_MI*2)
#define BBD_MI (BWD_MI*2)
#define RW_MI (BBD_MI*2)
#define RB_MI (RW_MI*3)
#define QW_MI (RB_MI*3)
#define QB_MI (QW_MI*2)
#define PW_MI (QB_MI*2)
#define PB_MI (PW_MI*9)
#define XX_MI (PB_MI*9)

/*
 * pro mindex2 ud�l�me max hodnoty figur v po�ad� N,B,R,Q,P
 */
 
#define NW_MI2 1LL
#define NB_MI2 (NW_MI2*16)
#define BWL_MI2 (NB_MI2*16)
#define BBL_MI2 (BWL_MI2*16)
#define BWD_MI2 (BBL_MI2*16)
#define BBD_MI2 (BWD_MI2*16)
#define RW_MI2 (BBD_MI2*16)
#define RB_MI2 (RW_MI2*16)
#define QW_MI2 (RB_MI2*16)
#define QB_MI2 (QW_MI2*16)
#define PW_MI2 (QB_MI2*16)
#define PB_MI2 (PW_MI2*16)
#define XX_MI2 (PB_MI2*16)

 
/*
	nw , nb , bwl, bbl, bwd, bbd, rw , rb , qw , qb , pw , pb ,
	0-2, 0-2, 0-1, 0-1, 0-1, 0-1, 0-2, 0-2, 0-1, 0-1, 0-8, 0-8,
	1  , 3  , 9  , 18 , 36 , 72 , 144, 432, 1296, 2592, 5184, 46656, 419904
	1  , 4  , 16 , 32 , 64 , 128, 256, 1024, 4096, 8192, 16384, 262144, 4194303 
	2, 2, 1, 1, 1, 1, 2, 2, 1, 1, 4, 4
	
	teoreticky je mozne mit vice Q, na ukor pescu, jak to zakomponovat? 1Q+8P, 0Q+8P, 2Q+7P
*/

/*
 * soustava
 * 3 3 2 2 2 2 3 3 2 2 3*3 3*3
 * zakomponovat pocet W/B - B,N,R,Q,P pocet Bl, Bd, 
 * max pocty pro stranu - prakticky/teoreticky - 2/10, 2/10, 2/10, 1/9, 8/8, 1/9, 1/9
 */
/*
 * test masky
 */

 
#define MATidx(pw,pb,nw,nb,bwl,bwd,bbl,bbd,rw,rb,qw,qb) (pw*PW_MI+PB_MI*pb+NW_MI*nw+NB_MI*nb+BWL_MI*bwl+BBL_MI*bbl+BWD_MI*bwd+BBD_MI*bbd+QW_MI*qw+QB_MI*qb+RW_MI*rw+RB_MI*rb)
#define MATidx2(pw,pb,nw,nb,bwl,bwd,bbl,bbd,rw,rb,qw,qb) (pw*PW_MI2+PB_MI2*pb+NW_MI2*nw+NB_MI2*nb+BWL_MI2*bwl+BBL_MI2*bbl+BWD_MI2*bwd+BBD_MI2*bbd+QW_MI2*qw+QB_MI2*qb+RW_MI2*rw+RB_MI2*rb)

typedef enum { NO_INFO=0, INSUFF, UNLIKELY, DIV2, DIV4, DIV8 } score_types;


#define isMATE(x) ((x>=MATEMIN && x<=MATEMAX) ? 1 : ((x<=-MATEMIN && x>=-MATEMAX) ? -1 : 0))
#define isMATE2(x) ((-MATEMIN<x && x<MATEMIN) ? 0 : (x>0) ? 1 : -1)
/* 
		fixes mate score as seen from level 0
		x = value
		y = matescore
		z = depth
*/

#define MATESCORE (MATEMAX)
//#define FixMateScore(x,z) ((x>0) ? (x-z) : (0-(x+z)) )
#define FixMateScore2(x,z) ((x>0) ? (x-z) : (x+z) )
#define DecMateScore(x) ((x>0) ? (x--) : (x++))
#define GenerateMATESCORE(ply) (MATESCORE-ply)
// dist in plies
#define GetMATEDist(x) ((x>0) ? MATESCORE-x : MATESCORE+x)

#define PlaceValueWon 5
#define PlaceValueDef 10
#define MobilitySquare 1

#define isIsolatedP 0x1
#define isPassedP 0x2
#define isBlockedP 0x4
#define isDoubledP 0x8
#define hasLeftP 0x10
#define isConnectedP 0x20
#define isBackwardP 0x40

#endif

/*
 *
 * $Id: tests.c,v 1.15.2.11 2006/02/13 23:08:39 mrt Exp $
 *
*/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <tgmath.h>
//#include <math.h>
#include "bitmap.h"
#include "generate.h"
#include "attacks.h"
#include "movgen.h"
#include "search.h"
#include "tuner.h"
#include "hash.h"
#include "defines.h"
#include "evaluate.h"
#include "utils.h"
#include "ui.h"
#include "openings.h"
#include "globals.h"
#include "search.h"
#include "pers.h"

//#include "search.h"

#include <ctype.h>
#include <limits.h>
#include <time.h>
#include <inttypes.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>

int variables_reinit_material(void *data){
tuner_variables_pass *v;

	v=(tuner_variables_pass*)data;
	if(v->stage==0) {
		meval_table_gen(v->p->mat , v->p, 0);
	} else {
		meval_table_gen(v->p->mate_e,v->p, 1);
	}
	return 0;
}

int variables_restore_material(void *data){
tuner_variables_pass *v;

	v=(tuner_variables_pass*)data;
	if(v->stage==0) {
		meval_table_gen(v->p->mat , v->p, 0);
	} else {
		meval_table_gen(v->p->mate_e,v->p, 1);
	}
return 0;
}

double enforce_positive(double val){
	if(val>0) return val;
	return 0;
}

double enforce_negative(double val){
	if(val<0) return val;
	return 0;
}

int free_matrix(matrix_type *m, int count)
{
	int i;
	if(m!=NULL) {
		for(i=0;i<count;i++) {
			if(m[i].init_data!=NULL) free(m[i].init_data);
		}
		if(m!=NULL) free(m);
	}
	return 0;
}

int to_matrix(matrix_type **m, personality *p)
{
int i, max, gs, sd, pi, sq;
int len=4096;
matrix_type *mat;
tuner_variables_pass *v;

	max=len;
	mat=malloc(sizeof(matrix_type)*len);
	*m=mat;
	i=0;
#if 1
// eval_BIAS
		mat[i].init_f=NULL;
		mat[i].restore_f=NULL;
		mat[i].init_data=NULL;
		mat[i].upd=0;
		mat[i].u[0]=&p->eval_BIAS;
		mat[i].mid=0;
		mat[i].ran=10000;
		mat[i].max=mat[i].ran/2+mat[i].mid;
		mat[i].min=mat[i].mid-mat[i].ran/2;
		mat[i].norm_f=NULL;
		i++;
#endif
#if 0
// type gamestage

	// pawn isolated
		for(gs=0;gs<=1;gs++) {
			mat[i].init_f=NULL;
			mat[i].restore_f=NULL;
			mat[i].init_data=NULL;
			mat[i].upd=0;
			mat[i].u[0]=&p->isolated_penalty[gs];
			mat[i].mid=0;
			mat[i].ran=10000;
			mat[i].max=mat[i].ran/2+mat[i].mid;
			mat[i].min=mat[i].mid-mat[i].ran/2;
//			mat[i].norm_f=enforce_negative;
			mat[i].norm_f=NULL;
			i++;
		}
#endif
#if 0

	// pawn backward
		for(gs=0;gs<=1;gs++) {
			mat[i].init_f=NULL;
			mat[i].restore_f=NULL;
			mat[i].init_data=NULL;
				mat[i].upd=0;
				mat[i].u[0]=&p->backward_penalty[gs];
				mat[i].mid=0;
				mat[i].ran=5000;
				mat[i].max=mat[i].ran/2+mat[i].mid;
				mat[i].min=mat[i].mid-mat[i].ran/2;
//				mat[i].norm_f=enforce_negative;
				mat[i].norm_f=NULL;
				i++;
		}
#endif
#if 0
	// pawn on ah
		for(gs=0;gs<=1;gs++) {
			mat[i].init_f=NULL;
			mat[i].restore_f=NULL;
			mat[i].init_data=NULL;
				mat[i].upd=0;
				mat[i].u[0]=&p->pawn_ah_penalty[gs];
				mat[i].mid=0;
				mat[i].ran=10000;
				mat[i].max=mat[i].ran/2+mat[i].mid;
				mat[i].min=mat[i].mid-mat[i].ran/2;
//				mat[i].norm_f=enforce_negative;
				mat[i].norm_f=NULL;
				i++;
		}
#endif
#if 1
// type of passer with 6 values
		// passer bonus
			for(gs=0;gs<=1;gs++) {
				for(sq=0;sq<=5;sq++) {
					mat[i].init_f=NULL;
					mat[i].restore_f=NULL;
					mat[i].init_data=NULL;
					mat[i].upd=1;
					mat[i].u[0]=&p->passer_bonus[gs][WHITE][sq];
					mat[i].u[1]=&p->passer_bonus[gs][BLACK][sq];
					mat[i].mid=0;
					mat[i].ran=10000;
					mat[i].max=mat[i].ran/2+mat[i].mid;
					mat[i].min=mat[i].mid-mat[i].ran/2;
//					mat[i].norm_f=enforce_positive;
					mat[i].norm_f=NULL;
					i++;
				}
			}
#endif
#if 0
		// pawn blocked penalty
			for(gs=0;gs<=1;gs++) {
				for(sq=0;sq<=4;sq++) {
					mat[i].init_f=NULL;
					mat[i].restore_f=NULL;
					mat[i].init_data=NULL;
					mat[i].upd=1;
					mat[i].u[0]=&p->pawn_blocked_penalty[gs][WHITE][sq];
					mat[i].u[1]=&p->pawn_blocked_penalty[gs][BLACK][sq];
					mat[i].mid=0;
					mat[i].ran=10000;
					mat[i].max=mat[i].ran/2+mat[i].mid;
					mat[i].min=mat[i].mid-mat[i].ran/2;
//					mat[i].norm_f=enforce_negative;
					mat[i].norm_f=NULL;
					i++;
				}
			}
#endif
#if 0
		// pawn stopped penalty
				for(gs=0;gs<=1;gs++) {
					for(sq=0;sq<=4;sq++) {
						mat[i].init_f=NULL;
						mat[i].restore_f=NULL;
						mat[i].init_data=NULL;
						mat[i].upd=1;
						mat[i].u[0]=&p->pawn_stopped_penalty[gs][WHITE][sq];
						mat[i].u[1]=&p->pawn_stopped_penalty[gs][BLACK][sq];
						mat[i].mid=0;
						mat[i].ran=10000;
						mat[i].max=mat[i].ran/2+mat[i].mid;
						mat[i].min=mat[i].mid-mat[i].ran/2;
//						mat[i].norm_f=enforce_negative;
						mat[i].norm_f=NULL;
						i++;
					}
				}
#endif
#if 0
		// pawn weak open file
				for(gs=0;gs<=1;gs++) {
						mat[i].init_f=NULL;
						mat[i].restore_f=NULL;
						mat[i].init_data=NULL;
						mat[i].upd=0;
						mat[i].u[0]=&p->pawn_weak_onopen_penalty[gs];
						mat[i].mid=0;
						mat[i].ran=10000;
						mat[i].max=mat[i].ran/2+mat[i].mid;
						mat[i].min=mat[i].mid-mat[i].ran/2;
//						mat[i].norm_f=enforce_negative;
						mat[i].norm_f=NULL;
						i++;
				}
#endif
#if 0
		// pawn weak center file
				for(gs=0;gs<=1;gs++) {
						mat[i].init_f=NULL;
						mat[i].restore_f=NULL;
						mat[i].init_data=NULL;
						mat[i].upd=0;
						mat[i].u[0]=&p->pawn_weak_center_penalty[gs];
						mat[i].mid=0;
						mat[i].ran=10000;
						mat[i].max=mat[i].ran/2+mat[i].mid;
						mat[i].min=mat[i].mid-mat[i].ran/2;
						mat[i].norm_f=NULL;
						i++;
				}
#endif

#if 0
//piece to square - single
	for(gs=0;gs<=0;gs++) {
		for(pi=0;pi<=0;pi++) {
			for(sq=0;sq<=0;sq++){
				mat[i].init_f=NULL;
				mat[i].restore_f=NULL;
				mat[i].init_data=NULL;
				mat[i].upd=1;
				mat[i].u[0]=&p->piecetosquare[gs][WHITE][pi][sq];
				mat[i].u[1]=&p->piecetosquare[gs][BLACK][pi][Square_Swap[sq]];
				mat[i].mid=0;
				mat[i].ran=10000;
				mat[i].max=mat[i].ran/2+mat[i].mid;
				mat[i].min=mat[i].mid-mat[i].ran/2;
//				mat[i].norm_f=enforce_negative;
				mat[i].norm_f=NULL;
				i++;
			}
		}
	}
#endif

#if 1
	int pieces_in[]= { 0, 1, 2, 3, 4, 5, -1  };
	
	for(gs=0;gs<=1;gs++) {
		int ii=0;
		while(pieces_in[ii]!=-1) {
			pi=pieces_in[ii];
			for(sq=0;sq<=63;sq++){
				mat[i].init_f=NULL;
				mat[i].restore_f=NULL;
				mat[i].init_data=NULL;
				mat[i].upd=1;
				mat[i].u[0]=&p->piecetosquare[gs][WHITE][pi][sq];
				mat[i].u[1]=&p->piecetosquare[gs][BLACK][pi][Square_Swap[sq]];
				mat[i].mid=0;
				mat[i].ran=10000;
				mat[i].max=mat[i].ran/2+mat[i].mid;
				mat[i].min=mat[i].mid-mat[i].ran/2;
				mat[i].norm_f=NULL;
				i++;
			}
			ii++;
		}
	}
#endif

#if 0
// rook on 7th
	for(gs=0;gs<=1;gs++) {
		mat[i].init_f=NULL;
		mat[i].restore_f=NULL;
		mat[i].init_data=NULL;
			mat[i].upd=0;
			mat[i].u[0]=&p->rook_on_seventh[gs];
			mat[i].mid=0;
			mat[i].ran=10000;
			mat[i].max=mat[i].ran/2+mat[i].mid;
			mat[i].min=mat[i].mid-mat[i].ran/2;
//			mat[i].norm_f=enforce_positive;
			mat[i].norm_f=NULL;
			i++;
	}
#endif

#if 0
// rook on open
	for(gs=0;gs<=1;gs++) {
		mat[i].init_f=NULL;
		mat[i].restore_f=NULL;
		mat[i].init_data=NULL;
			mat[i].upd=0;
			mat[i].u[0]=&p->rook_on_open[gs];
			mat[i].mid=0;
			mat[i].ran=10000;
			mat[i].max=mat[i].ran/2+mat[i].mid;
			mat[i].min=mat[i].mid-mat[i].ran/2;
//			mat[i].norm_f=enforce_positive;
			mat[i].norm_f=NULL;
			i++;
	}
#endif

#if 0
// rook on semiopen
	for(gs=0;gs<=1;gs++) {
		mat[i].init_f=NULL;
		mat[i].restore_f=NULL;
		mat[i].init_data=NULL;
			mat[i].upd=0;
			mat[i].u[0]=&p->rook_on_semiopen[gs];
			mat[i].mid=0;
			mat[i].ran=10000;
			mat[i].max=mat[i].ran/2+mat[i].mid;
			mat[i].min=mat[i].mid-mat[i].ran/2;
//			mat[i].norm_f=enforce_positive;
			mat[i].norm_f=NULL;
			i++;
	}
#endif

#if 1
// mobility
	int mob_lengths[]= { 2, 9, 14, 15, 28, 9, -1  };
	for(gs=0;gs<=1;gs++) {
		for(pi=0;pi<=5;pi++) {
			for(sq=0;sq<mob_lengths[pi];sq++){
				mat[i].init_f=NULL;
				mat[i].restore_f=NULL;
				mat[i].init_data=NULL;
				mat[i].upd=1;
				mat[i].u[0]=&p->mob_val[gs][WHITE][pi][sq];
				mat[i].u[1]=&p->mob_val[gs][BLACK][pi][sq];
				mat[i].mid=0;
				mat[i].ran=10000;
				mat[i].max=mat[i].ran/2+mat[i].mid;
				mat[i].min=mat[i].mid-mat[i].ran/2;
				mat[i].norm_f=NULL;
				i++;
			}
		}
	}
	
#endif
/*

pawn_iso_center_penalty, _gamestage
pawn_iso_onopen_penalty, _gamestage
pawn_pot_protect, _passer
pawn_n_protect, _passer
pawn_dir_protect, _passer
doubled_n_penalty, _passer
pshelter_open_penalty, _gamestage
pshelter_isol_penalty, _gamestage
pshelter_hopen_penalty, _gamestage
pshelter_double_penalty, _gamestage
pshelter_prim_bonus, _gamestage
pshelter_sec_bonus, _gamestage

*/

#if 0
// pawn_iso center
	for(gs=0;gs<=1;gs++) {
		mat[i].init_f=NULL;
		mat[i].restore_f=NULL;
		mat[i].init_data=NULL;
			mat[i].upd=0;
			mat[i].u[0]=&p->pawn_iso_center_penalty[gs];
			mat[i].mid=0;
			mat[i].ran=10000;
			mat[i].max=mat[i].ran/2+mat[i].mid;
			mat[i].min=mat[i].mid-mat[i].ran/2;
//			mat[i].norm_f=enforce_negative;
			mat[i].norm_f=NULL;
			i++;
	}
#endif
#if 0
	for(gs=0;gs<=1;gs++) {
		mat[i].init_f=NULL;
		mat[i].restore_f=NULL;
		mat[i].init_data=NULL;
			mat[i].upd=0;
			mat[i].u[0]=&p->pawn_iso_onopen_penalty[gs];
			mat[i].mid=0;
			mat[i].ran=10000;
			mat[i].max=mat[i].ran/2+mat[i].mid;
			mat[i].min=mat[i].mid-mat[i].ran/2;
//			mat[i].norm_f=enforce_negative;
			mat[i].norm_f=NULL;
			i++;
	}
#endif
#if 0
	for(gs=0;gs<=1;gs++) {
		mat[i].init_f=NULL;
		mat[i].restore_f=NULL;
		mat[i].init_data=NULL;
			mat[i].upd=0;
			mat[i].u[0]=&p->pshelter_open_penalty[gs];
			mat[i].mid=0;
			mat[i].ran=10000;
			mat[i].max=mat[i].ran/2+mat[i].mid;
			mat[i].min=mat[i].mid-mat[i].ran/2;
//			mat[i].norm_f=enforce_negative;
			mat[i].norm_f=NULL;
			i++;
	}
#endif
#if 0
	for(gs=0;gs<=1;gs++) {
		mat[i].init_f=NULL;
		mat[i].restore_f=NULL;
		mat[i].init_data=NULL;
			mat[i].upd=0;
			mat[i].u[0]=&p->pshelter_isol_penalty[gs];
			mat[i].mid=0;
			mat[i].ran=10000;
			mat[i].max=mat[i].ran/2+mat[i].mid;
			mat[i].min=mat[i].mid-mat[i].ran/2;
//			mat[i].norm_f=enforce_negative;
			mat[i].norm_f=NULL;
			i++;
	}
#endif
#if 0
	for(gs=0;gs<=1;gs++) {
		mat[i].init_f=NULL;
		mat[i].restore_f=NULL;
		mat[i].init_data=NULL;
			mat[i].upd=0;
			mat[i].u[0]=&p->pshelter_hopen_penalty[gs];
			mat[i].mid=0;
			mat[i].ran=10000;
			mat[i].max=mat[i].ran/2+mat[i].mid;
			mat[i].min=mat[i].mid-mat[i].ran/2;
//			mat[i].norm_f=enforce_negative;
			mat[i].norm_f=NULL;
			i++;
	}
#endif
#if 0
	for(gs=0;gs<=1;gs++) {
		mat[i].init_f=NULL;
		mat[i].restore_f=NULL;
		mat[i].init_data=NULL;
			mat[i].upd=0;
			mat[i].u[0]=&p->pshelter_double_penalty[gs];
			mat[i].mid=0;
			mat[i].ran=10000;
			mat[i].max=mat[i].ran/2+mat[i].mid;
			mat[i].min=mat[i].mid-mat[i].ran/2;
//			mat[i].norm_f=enforce_negative;
			mat[i].norm_f=NULL;
			i++;
	}
#endif
#if 1
	for(gs=0;gs<=1;gs++) {
		mat[i].init_f=NULL;
		mat[i].restore_f=NULL;
		mat[i].init_data=NULL;
			mat[i].upd=0;
			mat[i].u[0]=&p->pshelter_prim_bonus[gs];
			mat[i].mid=0;
			mat[i].ran=10000;
			mat[i].max=mat[i].ran/2+mat[i].mid;
			mat[i].min=mat[i].mid-mat[i].ran/2;
//			mat[i].norm_f=enforce_positive;
			mat[i].norm_f=NULL;
			i++;
	}
#endif
#if 1
	for(gs=0;gs<=1;gs++) {
		mat[i].init_f=NULL;
		mat[i].restore_f=NULL;
		mat[i].init_data=NULL;
			mat[i].upd=0;
			mat[i].u[0]=&p->pshelter_sec_bonus[gs];
			mat[i].mid=0;
			mat[i].ran=10000;
			mat[i].max=mat[i].ran/2+mat[i].mid;
			mat[i].min=mat[i].mid-mat[i].ran/2;
//			mat[i].norm_f=enforce_positive;
			mat[i].norm_f=NULL;
			i++;
	}
#endif
#if 0
// pawn n protect
	for(gs=0;gs<=1;gs++) {
		for(sq=0;sq<=7;sq++) {
			mat[i].init_f=NULL;
			mat[i].restore_f=NULL;
			mat[i].init_data=NULL;
			mat[i].upd=1;
			mat[i].u[0]=&p->pawn_n_protect[gs][WHITE][sq];
			mat[i].u[1]=&p->pawn_n_protect[gs][BLACK][sq];
			mat[i].mid=0;
			mat[i].ran=10000;
			mat[i].max=mat[i].ran/2+mat[i].mid;
			mat[i].min=mat[i].mid-mat[i].ran/2;
//			mat[i].norm_f=enforce_positive;
			mat[i].norm_f=NULL;
			i++;
		}
	}
#endif
#if 0
// pawn pot protect
	for(gs=0;gs<=1;gs++) {
		for(sq=0;sq<=7;sq++) {
			mat[i].init_f=NULL;
			mat[i].restore_f=NULL;
			mat[i].init_data=NULL;
			mat[i].upd=1;
			mat[i].u[0]=&p->pawn_pot_protect[gs][WHITE][sq];
			mat[i].u[1]=&p->pawn_pot_protect[gs][BLACK][sq];
			mat[i].mid=0;
			mat[i].ran=10000;
			mat[i].max=mat[i].ran/2+mat[i].mid;
			mat[i].min=mat[i].mid-mat[i].ran/2;
//			mat[i].norm_f=enforce_positive;
			mat[i].norm_f=NULL;
			i++;
		}
	}

#endif
#if 0
// pawn dir protect
	for(gs=0;gs<=1;gs++) {
		for(sq=0;sq<=7;sq++) {
			mat[i].init_f=NULL;
			mat[i].restore_f=NULL;
			mat[i].init_data=NULL;
			mat[i].upd=1;
			mat[i].u[0]=&p->pawn_dir_protect[gs][WHITE][sq];
			mat[i].u[1]=&p->pawn_dir_protect[gs][BLACK][sq];
			mat[i].mid=0;
			mat[i].ran=10000;
			mat[i].max=mat[i].ran/2+mat[i].mid;
			mat[i].min=mat[i].mid-mat[i].ran/2;
//			mat[i].norm_f=enforce_positive;
			mat[i].norm_f=NULL;
			i++;
		}
	}
#endif
#if 0
// pawn doubled n penalty
	for(gs=0;gs<=1;gs++) {
		for(sq=0;sq<=7;sq++) {
			mat[i].init_f=NULL;
			mat[i].restore_f=NULL;
			mat[i].init_data=NULL;
			mat[i].upd=1;
			mat[i].u[0]=&p->doubled_n_penalty[gs][WHITE][sq];
			mat[i].u[1]=&p->doubled_n_penalty[gs][BLACK][sq];
			mat[i].mid=0;
			mat[i].ran=10000;
			mat[i].max=mat[i].ran/2+mat[i].mid;
			mat[i].min=mat[i].mid-mat[i].ran/2;
//			mat[i].norm_f=enforce_negative;
			mat[i].norm_f=NULL;
			i++;
		}
	}
#endif

#if 1
// mobUnSec
	int mob_lengths2[]= { 2, 9, 14, 15, 28, 9, -1  };
	for(gs=0;gs<=1;gs++) {
		for(pi=0;pi<=5;pi++) {
			for(sq=0;sq<mob_lengths2[pi];sq++){
				mat[i].init_f=NULL;
				mat[i].restore_f=NULL;
				mat[i].init_data=NULL;
				mat[i].upd=1;
				mat[i].u[0]=&p->mob_uns[gs][WHITE][pi][sq];
				mat[i].u[1]=&p->mob_uns[gs][BLACK][pi][sq];
				mat[i].mid=0;
				mat[i].ran=10000;
				mat[i].max=mat[i].ran/2+mat[i].mid;
				mat[i].min=mat[i].mid-mat[i].ran/2;
				mat[i].norm_f=NULL;
				i++;
			}
		}
	}

#endif

// for these we need callback function
#if 0
	for(gs=0;gs<=1;gs++) {
		mat[i].init_f=variables_reinit_material;
		mat[i].restore_f=variables_restore_material;
		v=malloc(sizeof(tuner_variables_pass));
		v->p=p;
		v->stage=gs;
		mat[i].init_data=v;
			mat[i].upd=0;
			mat[i].u[0]=&p->rook_to_pawn[gs];
			mat[i].mid=0;
			mat[i].ran=10000;
			mat[i].max=mat[i].ran/2+mat[i].mid;
			mat[i].min=mat[i].mid-mat[i].ran/2;
			mat[i].norm_f=NULL;
			i++;
	}
#endif
#if 0
	for(gs=0;gs<=1;gs++) {
		mat[i].init_f=variables_reinit_material;
		mat[i].restore_f=variables_restore_material;
		v=malloc(sizeof(tuner_variables_pass));
		v->p=p;
		v->stage=gs;
		mat[i].init_data=v;

			mat[i].upd=0;
			mat[i].u[0]=&p->bishopboth[gs];
			mat[i].mid=0;
			mat[i].ran=10000;
			mat[i].max=mat[i].ran/2+mat[i].mid;
			mat[i].min=mat[i].mid-mat[i].ran/2;
//			mat[i].norm_f=enforce_positive;
			mat[i].norm_f=NULL;
			i++;
	}
#endif
#if 1
int white_in[]={ 1,2,3,4,-1 };
int black_in[]={ 0,1,2,3,4,-1 };
int ii;

	gs=0;
	ii=0;
	while(white_in[ii]!=-1) {
		sq=white_in[ii];
		
		mat[i].init_f=variables_reinit_material;
		mat[i].restore_f=variables_restore_material;
		v=malloc(sizeof(tuner_variables_pass));
		v->p=p;
		v->stage=gs;
		mat[i].init_data=v;
		mat[i].upd=0;
		mat[i].u[0]=&p->Values[gs][sq];
		mat[i].mid=10000;
		mat[i].ran=20000;
		mat[i].max=mat[i].ran/2+mat[i].mid;
		mat[i].min=mat[i].mid-mat[i].ran/2;
//		mat[i].norm_f=enforce_positive;
		mat[i].norm_f=NULL;
		i++;
		ii++;
	}
	gs=1;
	ii=0;
	while(black_in[ii]!=-1) {
		sq=black_in[ii];
		
		mat[i].init_f=variables_reinit_material;
		mat[i].restore_f=variables_restore_material;
		v=malloc(sizeof(tuner_variables_pass));
		v->p=p;
		v->stage=gs;
		mat[i].init_data=v;
		mat[i].upd=0;
		mat[i].u[0]=&p->Values[gs][sq];
		mat[i].mid=10000;
		mat[i].ran=20000;
		mat[i].max=mat[i].ran/2+mat[i].mid;
		mat[i].min=mat[i].mid-mat[i].ran/2;
//		mat[i].norm_f=enforce_positive;
		mat[i].norm_f=NULL;
		i++;
		ii++;
	}
#endif
return i;
}

double calc_dir_penalty_single(double var, double lambda)
{
	return lambda*(var*var);
//	return (var*var);
}

double recompute_penalty(double *nvar, int pcount, double lambda)
{
int i;
double pen,p2;
	pen=0;
	for(i=0;i<pcount;i++) {
		p2=calc_dir_penalty_single(nvar[i], lambda);
		pen+=p2;
	}
return pen;
}

int compute_eval_dir(board *b, uint8_t *ph, personality *p, long offset)
{
attack_model a;
struct _ui_opt uci_options;
struct _statistics s;
int ev;
	b[offset].stats=&s;
	b[offset].uci_options=&uci_options;
	a.phase = ph[offset];
// eval - white pov
	eval_king_checks_all(&b[offset], &a);
	ev=eval(&b[offset], &a, p);
return ev;
}

/*
 * evaluation function is combination of linear functions ax + by + cz + d where x,y,z are derived from one position on board
 * and a,b,c are tuned parameters, d is just offset / bias not related to anything on board
 * J is matrix of partial derivations for particular position at posn, in contains score with parameters in m
 * m is matrix of initial parameters
 * val is matrix of current/tested parameters
 * pcount is numbers of parameters
 * 
 *  evaluaci prevest na f(x)=f(x0)+df/dx(x0)*(x-x0)
 *			x0 je v ival[i];
 *			x je nval[i];
 *			f(x0) je v J[pos][parameters+1]
 ***			bias je v J[pos][parameters]
 *			x je v val
 *			potreba vyresit veci ktere prijdou z eval ale nejsou ovlivneny ladenymi parametry
 *			
 *			J[pos][0] - BIAS - tuned param
 *			J[pos][1] - first tuned param

 *			J[pos][parameters+0] - constant from eval, not affected by tuning
 *			J[pos][parameters+1] - f(x0), x0 stored in ival
 *			J[pos][parameters+2] - f(x) for diff calculation
 */
#define LK1 (1.0)
#define LK2 (3000.0)
#define LK3 (0.000953)
//#define LK2 (2500.0)

/* 
 * ry - result 2/1/0 - white won, draw, black won
 * ev - evaluation result positive favors white, negative favors black
 * h0 - remapped ev via sigmoid to 2 - 0 range/probability - 2 for white, 0 for black, 1 remis
 * return - gives error, ie  how far is expected result (h0) from real result (ry)
 */


double comp_cost(double ev, double ry){
double h0, ret;
//	h0=(2.0/(1+pow(10, (0-LK1)*ev/LK2)))/2;
	h0=(2.0/(1+exp((0-LK3)*ev)))/2;
//	if(h0==1) {
//	}
	ret=(-log(h0+(1E-10))*ry/2.0-log(1-h0+(1E-10))*(1-ry/2.0));
	return ret;
}

double comp_cost_vk(double ev, double ry, double K){
double h0, ret;
	h0=(2.0/(1+exp((0-K)*ev)))/2;
	ret=(-log(h0+(1E-10))*ry/2.0-log(1-h0+(1E-10))*(1-ry/2.0));
	return ret;
}

double comp_cost_x(double ev, double ry){
double h0;
	h0=(2.0/(1+pow(10,(0-LK1)*ev/LK2)));
	return pow((ry-h0)/2,2);
}

double xxcompute_loss(board *b, int8_t *rs, uint8_t *ph, personality *p, long count, int *indir, long offset)
{
double res, r1, r2, ry, cost, h0, ev, t;
attack_model a;
struct _ui_opt uci_options;
struct _statistics s;
long i, q;
	res=0;
	for(i=0;i<count;i++) {
		q=indir[i+offset];
		b[q].stats=&s;
		b[q].uci_options=&uci_options;
		a.phase = ph[q];

// eval - white pov
		eval_king_checks_all(&b[q], &a);
		ev=(double)eval(&b[q], &a, p);
		ry=rs[q];
// results - white pov
		t=comp_cost(ev, ry);
		res+=t;
	}
return res;
}

double xxcompute_loss_dir(board *b, int8_t *rs, uint8_t *ph, personality *p, long count, long offset)
{
double res, r1, r2, ry, cost, h0, ev;
attack_model a;
struct _ui_opt uci_options;
struct _statistics s;
int evi;
long i,q;

	res=0;
	for(i=0;i<count;i++) {
		q=i+offset;
		b[q].stats=&s;
		b[q].uci_options=&uci_options;
		a.phase = ph[q];

// eval - white pov
		eval_king_checks_all(&b[q], &a);
		evi=eval(&b[q], &a, p);
		ev=(double) evi;
		ry=rs[q];
// results - white pov
// sig=rrr-h0
// r=0-2, h0=0-2
		res+=comp_cost(ev, ry);
	}
return res;
}

double xxcompute_loss_dir_vk(board *b, int8_t *rs, uint8_t *ph, personality *p, long count, long offset, double K)
{
double res, r1, r2, ry, cost, h0, ev;
attack_model a;
struct _ui_opt uci_options;
struct _statistics s;
int evi;
long i,q;

	res=0;
	for(i=0;i<count;i++) {
		q=i+offset;
		b[q].stats=&s;
		b[q].uci_options=&uci_options;
		a.phase = ph[q];
// eval - white pov
		eval_king_checks_all(&b[q], &a);
		evi=eval(&b[q], &a, p);
		ev=(double) evi;
		ry=rs[q];
		res+=comp_cost_vk(ev, ry, K);
	}
return res;
}

double xxcompute_loss_dir_x(board *b, int8_t *rs, uint8_t *ph, personality *p, long count, long offset)
{
double res, r1, r2, ry, cost, h0, ev;
attack_model a;
struct _ui_opt uci_options;
struct _statistics s;
int evi;
long i, q;
	res=0;
	for(i=0;i<count;i++) {
		q=i+offset;
		b[q].stats=&s;
		b[q].uci_options=&uci_options;
		a.phase = ph[q];

// eval - white pov
		eval_king_checks_all(&b[q], &a);
		evi=eval(&b[q], &a, p);
		ev=(double) evi;
		ry=rs[q];
// results - white pov
// sig=rrr-h0
// r=0-2, h0=0-2
		res+=comp_cost_x(ev, ry);
	}
return res;
}

void dump_jac(double *J, long count, int *indir, long offset, double *ivar, double *nvar, int pcount)
{
double *JJ, fxh2;
long pos, q;
int i;

	printf("Dump JAC\n");
	LOGGER_0("Dump JAC\n");
	for(pos=0;pos<count;pos++) {
		q=indir[pos+offset];
		JJ=J+q*(pcount+5);
		fxh2=0;
		for(i=0;i<pcount;i++) {
			printf("P:%d=%lf\tI:%lf\tN:%lf\n", i, JJ[i], ivar[i], nvar[i]);
			NLOGGER_0("P:%d=%lf\tI:%lf\tN:%lf\n", i, JJ[i], ivar[i], nvar[i]);
			fxh2+=JJ[i]*(nvar[i]-ivar[i]);
		}		
		printf("D: rest %lf ,eval comp: %lf, eval fx0: %lf, eval dyn: %lf\n", JJ[pcount], fxh2, JJ[pcount+1], JJ[pcount+2]);
		LOGGER_0("D: rest %lf ,eval comp: %lf, eval fx0: %lf, eval dyn: %lf\n", JJ[pcount], fxh2, JJ[pcount+1], JJ[pcount+2]);
	}
}

// f(x)=f(x0)+df/dx(x0)*(x-x0)

int recompute_jac(double *JJ, long count, int *indir, long offset, double *ivar, double *nvar, int pcount)
{
	long pos, q;
	double *J, fxh;
	int i;

	for(pos=0;pos<count;pos++) {
		q=indir[pos+offset];
		J=JJ+q*(pcount+5);
		fxh=0;
		for(i=0;i<pcount;i++) {
			fxh+=(nvar[i]-ivar[i])*J[i];
		}
		J[pcount+2]=fxh+J[pcount+1];
	}
return 0;
}

double compute_loss_jac(int8_t *rs, long count, int *indir, long offset,double *JJ, double *ival, double *nval, int pcount)
{
double res, r1, r2, ry, cost, h0, nv, iv, ev;
double *J;
int i;
long ii, q;
// eval - white pov
// evaluaci prevest na f(x,y)=f(x0,y0) +df/dx(x0)*(x-x0) +df/dy(y0)*(y-y0)
/*
 *			x0, y0 je v ival[i];
 *			x, y je v nval[i];
 *			f(x0,y0) je v JJ[pos][parameters+1]
 *			bias je v JJ[pos][parameters]
 *			
 */

	res=0;
	for(ii=0;ii<count;ii++) {
		q=indir[ii+offset];
		J=JJ+q*(pcount+5);
		ev= J[pcount+1];
		for(i=0;i<pcount;i++) {
			ev+=(nval[i]-ival[i])*J[i];
		}
		ry=rs[q];
		res+=comp_cost(ev, ry);
	}
return res;
}

double compute_loss_jac_diff(int8_t *rs, long count, int *indir, long offset, double *JJ, double *ival, double *nval, int i, double new_val, int pcount)
{
double res, ry, cost, ev, ev2, o;
double *J;
long ii, q;

// eval - white pov
// evaluaci prevest na f(x)=f(x0)+df/dx(x0)*(x-x0)
// vice parametru
/*
 * x a x0 jsou ve skutecnosti vektory, kde kazda dimenze odpovida jednomu ladenemu parametru evaluacni funkce
 */
/*
 *			x0 je v ival[i];
 *			x je v nval[i];
 *			f(x0) je v JJ[pos][parameters+1]
 *			bias je v JJ[pos][parameters]
 *			f(x) je v JJ[pos][parameters+2]
 *			funkce spocita zmenu skore pri zmene jedne z hodnot v nval
 */

	res=0;
	for(ii=0;ii<count;ii++) {
		q=indir[ii+offset];
		J=JJ+q*(pcount+5);
		ev2=J[pcount+1];
		o=nval[i];
		nval[i]=new_val;
		nval[i]=o;

		ev= J[pcount+2];
		ev+=(new_val-nval[i])*J[i];
		ry=rs[q];
		res+=comp_cost(ev, ry);
	}
return res;
}

double compute_loss_jac_dir(int8_t *rs, long count, long offset,double *JJ, double *ival, double *nval, int pcount)
{
double res, r1, r2, ry, cost, h0, ev;
double *J;
int i;
long ii, q;

	res=0;
	for(ii=0;ii<count;ii++) {
		q=ii+offset;

// eval - white pov
// evaluaci prevest na f(x)=f(x0)+df/dx(x0)*(x-x0)
/*
 *			x0 je v ival[i];
 *			x je v nval[i];
 *			f(x0) je v JJ[pos][parameters+1]
 *			bias je v JJ[pos][parameters]
 *			
 */

		J=JJ+q*(pcount+5);
		ev=(double)J[pcount+1];
		for(i=0;i<pcount;i++) {
			ev+=(double)(nval[i]-ival[i])*J[i];
		}
		ry=rs[q];
		res+=comp_cost(ev, ry);
	}
return res;
}

int init_tuner_jac(tuner_run *state,matrix_type *m, double *var, int pcount){
int i;
	for(i=0;i<pcount;i++) state[i].or1=0.0000001;
	for(i=0;i<pcount;i++) state[i].or2=0.0000001;
	for(i=0;i<pcount;i++) state[i].update=0;
	for(i=0;i<pcount;i++) state[i].grad=0;
	for(i=0;i<pcount;i++) state[i].real=var[i];
	return 0;
}

int dump_tuner_jac(tuner_run *state, int pcount){
int i;
	NLOGGER_0("\n");
	LOGGER_0("OR1\t");
	for(i=0;i<pcount;i++) NLOGGER_0("%f\t", state[i].or1);
	NLOGGER_0("\n");
	LOGGER_0("OR2\t");
	for(i=0;i<pcount;i++) NLOGGER_0("%f\t", state[i].or2);
	NLOGGER_0("\n");
	LOGGER_0("UP\t");
	for(i=0;i<pcount;i++) NLOGGER_0("%f\t", state[i].update);
	NLOGGER_0("\n");
	LOGGER_0("GR\t");
	for(i=0;i<pcount;i++) NLOGGER_0("%f\t", state[i].grad);
	NLOGGER_0("\n");
	LOGGER_0("RE\t");
	for(i=0;i<pcount;i++) NLOGGER_0("%f\t", state[i].real);
	NLOGGER_0("\n");
	
return 0;
}

int dump_vars_jac(int source, double *var, int pcount){
int f;
double *s;
//	s=var+source*pcount;
//	for(f=0;f<pcount;f++) {
	for(f=1077;f<pcount;f++) {
		printf("VAR %d:%lf\n", f, var[source*pcount+f]);
		NLOGGER_0("VAR %d:%lf\n", f, var[source*pcount+f]);
	}
return 0;
}

int dump_vars_jac_x(double *var, int pcount){
int f,i;
//	for(f=0;f<pcount;f++) {
	for(f=1077;f<pcount;f++) {
		printf("VAR %d: ", f);
		NLOGGER_0("VAR %d: ", f);
		for(i=0;i<17;i++) {
			printf("%lf ", var[i*pcount+f]);
			NLOGGER_0("%lf ", f, var[i*pcount+f]);
		}
		printf("\n");
		NLOGGER_0("\n");
	}
return 0;
}

int allocate_tuner(tuner_run **tr, int pcount){
//tuner_run *t;
	*tr=malloc(sizeof(tuner_run)*(pcount+1));
	return 0;
}

int copy_vars_jac(int source, int dest, double *ivar, double *nvar, int pcount){
int f, s, d;
	s=pcount*source;
	d=pcount*dest;
	
	for(f=0;f<pcount;f++) {
		ivar[d+f]=ivar[s+f];
		nvar[d+f]=nvar[s+f];
	}

return 0;
}

/*
 * count  - pocet B
 * pcount - pocet parametru
 */

typedef struct _th_control {
	int threads;
	pthread_t th[20];
	pthread_attr_t attr[20];
	void *data[20];
} th_control;

typedef volatile struct _update_th {
	int command;
	int status;
	int8_t *rs;
	long count;
	matrix_type *m;
	tuner_global *tun;
	tuner_run *state;
	double *ivar;
	double *nvar;
	long pcount;
	int* indir;
	long offset;
	int pbe;
	int pen;
	double penalty;
} update_th;

int compute_parameters_update(int8_t *rs, long count, matrix_type *m, tuner_global *tun, tuner_run *state, double *ivar, double *nvar,  int pcount, int* indir, long offset, int pbe, int pen, double penalty ){
int diff, ioon;
double fxhd, fxh2d, fxdiffd, o, on, step, pen_tem, pen_te1, pen_te2;
//!!!!
int i;

//	LOGGER_0("%p count %d %p %p %p %p %p pcount %d %p offset %ld pbe %d pen %d penalty %lf\n", rs, count, m, tun, state, ivar, nvar, pcount, indir, offset, pbe, pen, penalty);
	for(i=pbe;i<pen;i++) {
// compute gradient for cost function
		step=2;
// get parameter value			
		o=nvar[i];
		pen_tem=calc_dir_penalty_single(o, tun->reg_la);
		on=o+step;			
// iterate over the same parameters and update them with change;
		pen_te1=penalty+calc_dir_penalty_single(on, tun->reg_la)-pen_tem;	
// compute loss
		fxhd=(compute_loss_jac_diff(rs, count, indir, offset, tun->jac, ivar, nvar, i, on, pcount)+pen_te1)/count;			
		on=o-step;
		pen_te2=penalty+calc_dir_penalty_single(on, tun->reg_la)-pen_tem;				
		fxh2d=(compute_loss_jac_diff(rs, count, indir, offset, tun->jac, ivar, nvar, i, on, pcount)+pen_te2)/count;
		fxdiffd=fxhd-fxh2d;			
		state[i].grad=(fxdiffd)/(2*step);
	}
return 0;
}

void *jac_engine_thread2(void *arg){
update_th volatile *thd;
struct timespec rq, rm;

	rq.tv_sec=0;
	rq.tv_nsec=100;
	thd=(update_th *) arg;
//	LOGGER_0("TH start\n");
	while(thd->command!=999) {
//		LOGGER_0("TH1 %d:%d\n", thd->command, thd->status);
		switch(thd->command) {
		case 0:
// wait
			thd->status=0;
			while(thd->command==0){
//				LOGGER_0("TH2 %d:%d\n", thd->command, thd->status);
				nanosleep(&rq, &rm);
			}
			break;
		case 1: 
//			LOGGER_0("running\n");
			thd->status=100;
			compute_parameters_update(thd->rs, thd->count, thd->m, thd->tun, thd->state, thd->ivar, thd->nvar,  thd->pcount, thd->indir, thd->offset, thd->pbe, thd->pen, thd->penalty );
//			if(sleep(1)!=0) LOGGER_0("Interrupted\n");
			thd->status=1;			
//			LOGGER_0("run done\n");
			while(thd->command==1) {
//				LOGGER_0("TH3 %d:%d\n", thd->command, thd->status);
				nanosleep(&rq, &rm);
			}
//			LOGGER_0("NOT running\n");
			break;			
		case 2:
			break;
		default:
			nanosleep(&rq, &rm);
			break;
		};
	}
//	LOGGER_0("TH quit\n");
return NULL;
}

th_control* prepare_update_th(int8_t *rs, int count, matrix_type *m, tuner_global *tun, tuner_run *state, double *ivar, double *nvar, int pcount, int* indir, int m_thr)
{
int t,r,f,i;
update_th *thd;
th_control *thc;

	if(m_thr>20) m_thr=20;
	t=pcount/m_thr;
	r=pcount%m_thr;
	
	thd=malloc(sizeof(update_th)*m_thr);
	thc=malloc(sizeof(th_control)*1);
	i=0;
	for(f=0;f<m_thr;f++){
		thd[f].command=0;
		thd[f].status=1111+f;
		thd[f].rs=rs;
		thd[f].count=count;
		thd[f].m=m;
		thd[f].tun=tun;
		thd[f].state=state;
		thd[f].ivar=ivar;
		thd[f].nvar=nvar;
		thd[f].pcount=pcount;
		thd[f].indir=indir;
		thd[f].offset=0;
		thd[f].pbe=i;
		i+=t;
		if(r>0) {
			i++;
			r--;
		}
		thd[f].pen=i;
		thd[f].penalty=0;
		thc->th[f]=1234;
		thc->data[f]=&(thd[f]);
		pthread_attr_init(&(thc->attr[f]));
		pthread_attr_setdetachstate(&(thc->attr[f]), PTHREAD_CREATE_JOINABLE);
		pthread_create(&(thc->th[f]),&(thc->attr[f]), jac_engine_thread2, (void *) (&(thd[f])));
		pthread_attr_destroy(&(thc->attr[f]));
	}
	thc->threads=m_thr;

return thc;
}

int set_to_state(th_control *thc, int cmd){
int f;
update_th *thd;
	for(f=0;f<thc->threads;f++){
		thd=thc->data[f];
//		thd->status=111;
		thd->command=cmd;
	}
return 0;
}

int wait_for_state_change(th_control *thc, int state){
update_th volatile *thd;
int f;
struct timespec rq, rm;
//	LOGGER_0("threads %d\n", thc->threads);
	rq.tv_sec=0;
	rq.tv_nsec=100;
	for(f=0;f<thc->threads;f++){
		thd=thc->data[f];
//		LOGGER_0("status0 %d:%d\n", f, thd->status);
		if(state!=999) while(thd->status!=state) {
//			LOGGER_0("status1 %d:%d\n", f, thd->status);
			nanosleep(&rq, &rm);
		} else while(thd->status==999) {
//			LOGGER_0("status2 %d\n", thd->status);
		  }
		nanosleep(&rq, &rm);
	}
return 0;
}

int set_variables(th_control *thc, int offset, long count, double penalty){
update_th *thd;
int f;
	for(f=0;f<thc->threads;f++){
		thd=thc->data[f];
		thd->offset=offset;
		thd->count=count;
		thd->penalty=penalty;
	}
return 0;
}

int destroy_th(th_control *thr){
update_th *thd;
void *status;
int f;
	for(f=0;f<thr->threads;f++) {
		thd=thr->data[f];
		thd->command=999;
		pthread_join(thr->th[f], &status);
	}
	free(thr->data[0]);
	free(thr);
return 0;
}

int iterate_th(th_control *thr){
update_th *thd;
void *status;
int f;
	for(f=0;f<thr->threads;f++) {
		thd=thr->data[f];
		compute_parameters_update(thd->rs, thd->count, thd->m, thd->tun, thd->state, thd->ivar, thd->nvar,  thd->pcount, thd->indir, thd->offset, thd->pbe, thd->pen, thd->penalty );
	}
return 0;
}

// minimalizovat 

/*
 * count  - pocet B
 * pcount - pocet parametru
 */

void p_tuner_jac(int8_t *rs, long count, matrix_type *m, tuner_global *tun, tuner_run *state, double *ivar, double *nvar, int pcount, char * outp, int* indir, long offset, int iter, th_control *th)
{
int diff, ioon;
double r,rr,x,y,z, oon, g_reg, y_hat, x_hat, o, on, step, penalty, pen_tem, pen_te1, pen_te2, p2;
//!!!!
int i, n, sq, ii;
int q,g;
int pbe, pen,cc,mo;

		// loop over parameters
		recompute_jac(tun->jac, count, indir, offset, ivar, nvar, pcount);
		penalty=recompute_penalty(nvar, pcount, tun->reg_la);
// change params for all threads
		set_variables(th, offset, count, penalty);
// set all threads to run
		set_to_state(th, 1);
// wait for threads become idle
		wait_for_state_change(th, 1);
		set_to_state(th, 0);
		wait_for_state_change(th, 0);

//		iterate_th(th);

	//	cc=pcount/4;
	//	mo=pcount%4;

	//	if(cc>0) for(i=0;i<cc;i++) compute_parameters_update(rs, count, m, tun, state, ivar, nvar, pcount, indir, offset, i*4, (i+1)*4, penalty);
	//	if(mo>0) compute_parameters_update(rs, count, m, tun, state, ivar, nvar, pcount, indir, offset, cc*4, cc*4+mo, penalty);

// gradient descent
		for(i=0;i<pcount;i++) {
			switch(tun->method) {
				case 2:
				/*
				 * rmsprop
				 */
				// accumulate gradients
					state[i].or2=(state[i].or2*tun->la2)+(pow(state[i].grad,2))*(1-tun->la2);
				// compute update
					y=sqrt(state[i].or2+tun->small_c);
				// update
					r=0-state[i].grad/y;
					break;
				case 1:
				/*
				 * AdaDelta
				 */
				// accumulate gradients
					state[i].or2=(state[i].or2*tun->la2)+(pow(state[i].grad,2))*(1-tun->la2);
					x=sqrt(state[i].or1);
					y=sqrt(state[i].or2+tun->small_c);
				// adadelta update
					r=0-state[i].grad*x/y;
				// accumulate updates / deltas
					state[i].or1=(state[i].or1*tun->la1)+(pow(r,2))*(1-tun->la1);
				// store update / delta / rescale to parameter range
					break;
				case 0:
				/*
				 * Adam mod.
				 */
				// accumulate gradients squared
					state[i].or2=(state[i].or2*tun->la2)+(pow(state[i].grad,2))*(1-tun->la2);
				// accumulate gradients
					state[i].or1=(state[i].or1*tun->la1)+(pow(state[i].grad,1))*(1-tun->la1);
				// compute update
					y_hat=state[i].or2/(1.0-pow(tun->la2, iter));
					x_hat=state[i].or1/(1.0-pow(tun->la1, iter));
					x=x_hat;
					y=sqrt(y_hat)+tun->small_c;
				// update
					r=0-x/y;
					break;
				default:
					abort();
			}
			rr=state[i].real;
			state[i].real+=(r*tun->temp_step);
//			state[i].real=Max(-1, Min(1, state[i].real));
			if(m[i].norm_f!=NULL) state[i].real=m[i].norm_f(state[i].real);
			state[i].update=state[i].real-rr;
//			oon=unnorm_val(state[i].real,m[i].ran,m[i].mid);
			oon=state[i].real;
			nvar[i]=round(oon);
		}
}

double * allocate_jac(long records, int params){
double *J;

	J=(double*)malloc(sizeof(double)*(params+5)*records);
	return J;
}

int free_jac(double *J){
	if(J!=NULL) {
	    free(J);
	}
	return 0;
}

// eval is linear ax + by + cz + d, x,y,z come from board (like number of pawns, position of passer, etc)
// a,b,c (and d) are parameters we are tuning
// population computes partial derivations with respect to a,b,c so in J we will get x,y,z for each board
int populate_jac(double *J,board *b, int8_t *rs, uint8_t *ph, personality *p, long start, long stop, matrix_type *m, int pcount)
{
	int diff_step;
	long pos;
	int fxh, fxh2;
	double fxdiff, *JJ;
	//!!!!
	int i,ii;
	int o,q,g, on;

	for(i=0;i<pcount;i++) {

		// loop over parameters
//		diff_step=2;
		diff_step=m[i].ran/4;
		o=*(m[i].u[0]);
		
// update parameter in positive way		
		on=o+diff_step;
		for(ii=0;ii<=m[i].upd;ii++) {
			*(m[i].u[ii])=on;
		}
		if(m[i].init_f!=NULL) m[i].init_f(m[i].init_data);

// compute eval for all positions for the parameter
		for(pos=start;pos<=stop;pos++) {
			JJ=J+pos*(pcount+5); 
			// compute eval
			JJ[i]=compute_eval_dir(b, ph, p, pos);
		}

// update parameter in negative way
		on=o-diff_step;
		for(ii=0;ii<=m[i].upd;ii++) {
			*(m[i].u[ii])=on;
		}
		if(m[i].init_f!=NULL) m[i].init_f(m[i].init_data);

// compute eval all positions for the parameter
// compute change and partial derivative of the parameter
		for(pos=start;pos<=stop;pos++) {
			JJ=J+pos*(pcount+5);
			fxh2=compute_eval_dir(b, ph, p, pos);
			// compute gradient/partial derivative
			fxdiff=JJ[i]-fxh2;
			JJ[i]=(fxdiff)/(2*diff_step);
		}

//restore original values
		for(ii=0;ii<=m[i].upd;ii++) {
			*(m[i].u[ii])=o;
		}
		if(m[i].init_f!=NULL) m[i].init_f(m[i].init_data);
		if((i%100)==0) printf("*");
	}

// iterate positions
	for(pos=start;pos<=stop;pos++) {
		JJ=J+pos*(pcount+5);
		
// compute classical evaluation
		fxh=compute_eval_dir(b, ph, p, pos);

// recompute score
		fxh2=0;
		for(i=0;i<pcount;i++) {
			fxh2+=*(m[i].u[0])*JJ[i];
		}
		
// score from evaluation not affected by tuning
		JJ[i++]=fxh-fxh2;
// score from eval
		JJ[i++]=fxh;
		JJ[i++]=fxh;
	}
	printf("\n");
return 0;
}

typedef struct {
	double *J;
	board *b;
	int8_t *rs; 
	uint8_t *ph; 
	personality *p; 
	int start;
	int stop;
	matrix_type *m; 
	int pcount;
} _tt_data;

void *jac_engine_thread(void *arg){
_tt_data *d;
	d=(_tt_data *)arg;
	populate_jac(d->J,d->b, d->rs, d->ph, d->p, d->start, d->stop, d->m, d->pcount);
	return arg;
}

// do populate jac multithreaded
int populate_jac_pl(double *J,board *b, int8_t *rs, uint8_t *ph, personality *p, int start, int stop, matrix_type *m, int pcount, int threads){
void * status;
pthread_t th[20];
_tt_data data[20];
pthread_attr_t attr[20];
int f,pos,s,r;

	if(threads>20) threads=20;
	pos=(stop-start+1);
	if(pos<threads) threads=1;
	s=pos/threads;
	r=pos%threads;
	for(f=0;f<threads;f++) {
		data[f].J=J;
		data[f].b=b;
		data[f].rs=rs;
		data[f].ph=ph;
		data[f].p=init_personality(NULL);
		copyPers(p, data[f].p);
		data[f].start=(start+f*s);
		data[f].stop=(start+f*s+s-1);
		printf("Pos %d, start %d, stop %d\n", pos, data[f].start, data[f].stop);
		to_matrix(&(data[f].m), data[f].p);
		data[f].pcount=pcount;
		data[f].b->hps=NULL;
	}
	data[f-1].stop+=r;
	printf("Pos final %d, start %d, stop %d\n", pos, data[f-1].start, data[f-1].stop);

	for(f=0;f<threads;f++) {
		pthread_attr_init(attr+f);
		pthread_attr_setdetachstate(attr+f, PTHREAD_CREATE_JOINABLE);
		pthread_create(th+f,attr+f, jac_engine_thread, (void *) (data+f));
		pthread_attr_destroy(attr+f);
	}

	sleep(1);
	for(f=0;f<threads;f++) {
		pthread_join(th[f], &status);
	}
	for(f=0;f<threads;f++) {
		free_matrix(data[f].m, data[f].pcount);
		free(data[f].p);
	}

return 0;
}

int texel_load_files2(tuner_global *tuner, char *sts_tests[]){
	int8_t r;
	FILE * handle;
	char filename[256];
	char buffer[512];
	char res[128];
	char fen[512];
	char *name;

	int x;
	char *xx;
	long l,i;
	l=i=0;
	tuner->len=0;

		strcpy(filename, sts_tests[0]);
		if((handle=fopen(filename, "r"))==NULL) {
			printf("File %s is missing\n",filename);
			return -1;
		}
		while(!feof(handle)&&(tuner->len<tuner->max_records)) {
			xx=fgets(buffer, 511, handle);
			if(parseEPD(buffer, fen, NULL, NULL, NULL, NULL, res, NULL, &name)>0) {
//				printf("Parsed %s, %s\n", fen, res);
				if((i%(tuner->nth))==tuner->records_offset) {
					if(!strcmp(res, "1-0")) r=2;
					else if(!strcmp(res, "0-1")) r=0;
					else if(!strcmp(res, "1/2-1/2")) r=1;
					else {
						printf("Result parse error:%s\n",filename);
						abort();
					}
					setup_FEN_board(&tuner->boards[tuner->len], fen);
					tuner->phase[tuner->len]= eval_phase(&tuner->boards[tuner->len], tuner->pi);
					tuner->results[tuner->len]=r;
					tuner->boards[tuner->len].hps=NULL;
					tuner->len++;
				}
				free(name);
				i++;
			} else {
//				printf("Parse Error: %s\n",fen);
			}
		}
		fclose(handle);
	printf("L2: Imported %ld from total %ld of records\n", tuner->len, i);
	return 1;
}

int texel_load_files(tuner_global *tuner, char *sts_tests[]){
// results from white pov
	int8_t tests_setup[]= { 2, 1, 0, -1 };
	FILE * handle;
	char filename[256];
	char buffer[512];
	char fen[512];
	char *name;

	int x;
	int sc_b, sc_e;
	char *xx;
	long l,i;
	l=i=0;
	
	tuner->len=0;

	while((tests_setup[l]!=-1)&&(tuner->len<tuner->max_records)) {
		strcpy(filename, sts_tests[l]);
		if((handle=fopen(filename, "r"))==NULL) {
			printf("File %s is missing\n",filename);
			return -1;
		}
		while(!feof(handle)&&(tuner->len<tuner->max_records)) {
			xx=fgets(buffer, 511, handle);
			if(parseEPD(buffer, fen, NULL, NULL, NULL, NULL, NULL, NULL, &name)>0) {
				if((i%(tuner->nth))==tuner->records_offset) {
					(tuner->boards[tuner->len]).hps=NULL;
					setup_FEN_board(&tuner->boards[tuner->len], fen);
					tuner->phase[tuner->len]= eval_phase(&tuner->boards[tuner->len], tuner->pi);
					tuner->results[tuner->len]=tests_setup[l];
					tuner->len++;
				}
				free(name);
				i++;
			}
		}
		fclose(handle);
		l++;
	}
	printf("L1: Imported %ld from total %ld of records\n", tuner->len, i);
	return 1;
}

#if 0
// callback funkce
#define CBACK int (*cback)(char *fen, int8_t res, void *data)

void file_load_driver(int min, int max, int sw, CBACK, void *cdata)
{
char buffer[512], fen[100];
int i, depth;
board b;
attack_model *a, ATT;

struct _ui_opt uci_options;

	b.uci_options=&uci_options;
	b.stats=allocate_stats(1);
	b.hs=allocateHashStore(HASHSIZE, 2048);
	b.hps=allocateHashPawnStore(HASHPAWNSIZE);
	b.hht=allocateHHTable();
	a=&ATT;

		b.pers=(personality *) init_personality("pers.xml");
		i=1;
		readClock_wall(&st);
		while(cback(buffer,cdata)) {
			i++;
		}
		deallocate_stats(b.stats);
		freeHHTable(b.hht);
		freeHashPawnStore(b.hps);
		freeHashStore(b.hs);
		free(b.pers);
}

typedef struct __file_load_cb_data {
	FILE * handle;
	long offs;
	long max;
	long nth;
	int i;
} file_load_cb_data;

int file_load_cback(char *fen, void *data){
char buffer[512];
int x;
char *xx;
file_load_cb_data *i;
	i = (file_load_cb_data *)data;
	if(!feof(i->handle)) {
		xx=fgets(buffer, 511, i->handle);
		strcpy(fen, buffer);
		return 1;
	}
	return 0;
	
	int8_t r;
	FILE * handle;
	char filename[256];
	char buffer[512];
	char res[128];
	char fen[512];
	char *name;

	int x;
	char *xx;
	long l,i;
	l=i=0;
	tuner->len=0;

		if((handle=fopen(filename, "r"))==NULL) {
			printf("File %s is missing\n",filename);
			return -1;
		}
		while(!feof(handle)&&(tuner->len<tuner->max_records)) {
			xx=fgets(buffer, 511, handle);
			if(parseEPD(buffer, fen, NULL, NULL, NULL, NULL, res, NULL, &name)>0) {
//				printf("Parsed %s, %s\n", fen, res);
				if((i%(tuner->nth))==tuner->records_offset) {
					if(!strcmp(res, "1-0")) r=2;
					else if(!strcmp(res, "0-1")) r=0;
					else if(!strcmp(res, "1/2-1/2")) r=1;
					else {
						printf("Result parse error:%s\n",filename);
						abort();
					}
					setup_FEN_board(&tuner->boards[tuner->len], fen);
					tuner->phase[tuner->len]= eval_phase(&tuner->boards[tuner->len], tuner->pi);
					tuner->results[tuner->len]=r;
					tuner->boards[tuner->len].hps=NULL;
					tuner->len++;
				}
				free(name);
				i++;
			} else {
//				printf("Parse Error: %s\n",fen);
			}
		}
		fclose(handle);
	printf("L2: Imported %ld from total %ld of records\n", tuner->len, i);
	return 1;
}

int texel_file_load1(char *sts_tests[], long max, long nth, long offs)
{
file_load_cb_data *data;
char filename[256];

		strcpy(filename, sts_tests[0]);
		if((data->handle=fopen(filename, "r"))==NULL) {
			printf("File %s is missing\n",filename);
			return -1;
		}

return 0;
}

#endif

int texel_test_init(tuner_global *tuner)
{
	tuner->matrix_var_backup=NULL;
	tuner->pi=NULL;
	tuner->m=NULL;
	tuner->jac=NULL;
	tuner->nvar=NULL;

/*
 *  boards - array of boards to be analyzed
 *  results - array of corresponding results
 *  phase - array of corresponing phase
 */

	tuner->boards=malloc(sizeof(board)*tuner->max_records);
	tuner->results=malloc(sizeof(int8_t)*tuner->max_records);
	tuner->phase=malloc(sizeof(uint8_t)*tuner->max_records);
	if((tuner->boards==NULL)||(tuner->results==NULL)) abort();

	tuner->pi=(personality *) init_personality(NULL);
	tuner->pcount=to_matrix(&(tuner->m), tuner->pi);

	tuner->matrix_var_backup=malloc(sizeof(int)*tuner->pcount*17);
	tuner->nvar=malloc(sizeof(double)*tuner->pcount*17*2);
	tuner->ivar=tuner->nvar+tuner->pcount*17;

return 0;
}

int texel_test_fin(tuner_global *tuner)
{
	if(tuner->nvar!=NULL) free(tuner->nvar);
	if(tuner->matrix_var_backup!=NULL) free(tuner->matrix_var_backup);
	free_matrix(tuner->m, tuner->pcount);
	if(tuner->pi!=NULL) free(tuner->pi);
	if(tuner->phase!=NULL) free(tuner->phase);
	if(tuner->results!=NULL) free(tuner->results);
	if(tuner->boards!=NULL) free(tuner->boards);
return 0;
}


void texel_test_loop_jac(tuner_global *tuner, char * base_name, tuner_global *tun2, double fxa1)
{
	int n;
	tuner_run *state;

	unsigned long long int totaltime;
	struct timespec start, end;

	int gen, perc, ccc;
	int *rnd, *rids, rrid, r1,r2;
	char nname[256];
	double fxh, fxh2=0, fxh3, fxb,t, fxb1;
	
	double *cvar;
	long i,l;
	
	th_control *th;

// tuner->m maps personality in tuner->pi into variables used by tuner
	allocate_tuner(&state, tuner->pcount);
	
	rids=rnd=NULL;
// randomization init
	rnd=malloc(sizeof(int)*tuner->len);
	rids=malloc(sizeof(int)*tuner->len);

	for(i=0;i<tuner->len;i++){
		rnd[i]=i;
		rids[i]=i;
	}

	srand(time(NULL));
	switch(tuner->method) {
		case 2:
			t=tuner->rms_step;
			break;
		case 1:
			t=tuner->adadelta_step;
			break;
		case 0:
			t=tuner->adam_step;
			break;
		default:
			abort();
	}
	tuner->temp_step=t*(tuner->batch_len);

// looping over testing ...
	fxb=fxh=compute_loss_jac_dir(tuner->results, tuner->len, 0, tuner->jac, tuner->ivar, tuner->nvar, tuner->pcount)/tuner->len;

	init_tuner_jac(state, tuner->m, tuner->nvar, tuner->pcount);
	th=prepare_update_th(tuner->results, 0, tuner->m, tuner, state, tuner->ivar, tuner->nvar, tuner->pcount, rnd, 14);

	for(gen=1;gen<=tuner->generations;gen++) {

#if 1
		for(i=0;i<tuner->len;i++){
			rrid=rand() %tuner->len;
			r1=rnd[i];
			r2=rnd[rrid];
			rnd[i]=r2;
			rnd[rrid]=r1;
			rids[r2]=i;
			rids[r1]=rrid;
		}
#endif

		{
			readClock_wall(&start);
			sprintf(nname,"%s_%ld_%d.xml",base_name, tuner->batch_len,gen);
			// compute loss prior tuning
			LOGGER_0("GEN %d, blen %ld, Initial loss of whole data =%.10f JAC\n", gen, tuner->batch_len, fxh);
			// tuning part
			// in minibatches
			ccc=1;
			i=0;
			perc=10;
			while(tuner->len>i) {
				l= ((tuner->len-i)>=tuner->batch_len) ? tuner->batch_len : tuner->len-i;
				p_tuner_jac(tuner->results, l, tuner->m, tuner, state, tuner->ivar, tuner->nvar, tuner->pcount, nname, rnd,i, ccc, th);
				ccc++;
				if((i*100/tuner->len) > perc) {
					fflush(stdout);
					perc+=10;
				}
				i+=l;
			}
			fxh3=compute_loss_jac_dir(tuner->results, tuner->len, 0, tuner->jac, tuner->ivar, tuner->nvar, tuner->pcount);
			fxh2=fxh3/tuner->len;
			readClock_wall(&end);
			totaltime=diffClock(start, end);
			printf("GEN %d, blen %ld, Final loss of whole data =%.10f:%.10f, %.10f\n", gen, tuner->batch_len, fxh2, fxb, fxh3);
			LOGGER_0("GEN %d, blen %ld, Final loss of whole data =%.10f\n", gen, tuner->batch_len, fxh2);
			if(fxh2<fxb) {
				fxb=fxh2;
				if(tun2!=NULL){
					fxb1=(compute_loss_jac_dir(tun2->results, tun2->len, 0, tun2->jac, tuner->ivar, tuner->nvar, tuner->pcount))/tun2->len;
					LOGGER_0("Verification loss %f:%f\n", fxb1, fxa1);
					printf("Verification loss %f:%f\n", fxb1, fxa1);
					if(fxb1<fxa1) {
						copy_vars_jac(0,15,tuner->ivar, tuner->nvar, tuner->pcount);
						write_personality(tuner->pi, nname);
						printf("Update\n");
						LOGGER_0("Update\n");
						fxa1=fxb1;
					}	
				} else {
					copy_vars_jac(0,15,tuner->ivar, tuner->nvar, tuner->pcount);
					write_personality(tuner->pi, nname);
				}
			}
			fxh=fxh2;
		}
	}
	if(rnd!=NULL) free(rnd);
	if(rids!=NULL) free(rids);
	if(state!=NULL) free(state);
	destroy_th(th);
}

void texel_test()
{
int i, *iv,ll;
tuner_global tuner, tun2;
double fxb1, fxb2, fxb3, fxbj, fxb4, lambda, K;

	lambda=0.00032;
	LOGGER_0("Lambda %f\n", lambda);
// initialize tuner
	tuner.max_records=200000;
	texel_test_init(&tuner);

	tuner.generations=400;
	tuner.batch_len=16384;
	tuner.records_offset=0;
	tuner.nth=15;
	tuner.small_c=1E-30;
	tuner.adam_step=0.001;
//	tuner.adam_step=1.0;
	tuner.rms_step=0.001;
	tuner.adadelta_step=0.01;

// load position files and personality to seed tuning params
//	char *xxxx[]= { "../texel/1-0.txt", "../texel/0.5-0.5.txt", "../texel/0-1.txt" };
//	char *xxxx[]= { "../texel/1-0.epd", "../texel/0.5-0.5.epd", "../texel/0-1.epd" };
//	char *xxxx[]= { "../texel/quiet-labeled.epd" };
//	char *xxxx[]= { "../texel/lichess-quiet.txt" };

	char *files1[]= { "../texel/quiet-labeled.epd" };
	char *files2[]= { "../texel/lichess-quiet.txt" };

	texel_load_files2(&tuner, files2);
	load_personality("../texel/pers.xml", tuner.pi);

	tuner.reg_la=lambda/tuner.pcount;
	
	tun2.max_records=1000000;
	texel_test_init(&tun2);
	tun2.records_offset=0;
	tun2.nth=1;
	tun2.small_c=1E-20;
	
	texel_load_files2(&tun2, files1);

//	tuner.reg_la=0.00125*tuner.batch_len/tuner.len;

#if 1
// tranfer values from slot 16 to JAC based tuner - slot 0
	copy_vars_jac(16,0,tuner.ivar, tuner.nvar, tuner.pcount);

/*
	K=0.000960;
	for(i=0;i<100;i++) {
		fxb1=(compute_loss_dir_vk(tuner.boards, tuner.results, tuner.phase, tuner.pi, tuner.len, 0, K));
		fxb1/=tuner.len;
		LOGGER_0("K computation K: %f, loss= %f\n", K, fxb1);
		printf("K computation K: %f, loss= %f\n", K, fxb1);
		K-=0.000001;
	}
*/

// allocate jacobian and compute partial derivatives for each position loaded
	tuner.jac=NULL;
	tuner.jac=allocate_jac(tuner.len, tuner.pcount);
	if(tuner.jac!=NULL) {
		LOGGER_0("JACOBIAN population, positions %ld, parameters %d\n", tuner.len, tuner.pcount);
		populate_jac_pl(tuner.jac,tuner.boards, tuner.results, tuner.phase, tuner.pi, 0, tuner.len-1, tuner.m, tuner.pcount, 14);
		LOGGER_0("JACOBIAN populated\n");
	} else {
		LOGGER_0("JACOBIAN aborted\n");
		printf("JACOBIAN aborted\n");
		abort();	
	}

// compute jacobian for verification positions
	tun2.jac=NULL;
	tun2.jac=allocate_jac(tun2.len, tuner.pcount);
	if(tun2.jac!=NULL) {
		LOGGER_0("JACOBIAN population, positions %ld, parameters %d\n", tun2.len, tuner.pcount);
		printf("JACOBIAN population, positions %ld, parameters %d\n", tun2.len, tuner.pcount);
		populate_jac_pl(tun2.jac,tun2.boards, tun2.results, tun2.phase, tuner.pi, 0, tun2.len-1, tuner.m, tuner.pcount, 14);
		LOGGER_0("JACOBIAN populated\n");
		printf("JACOBIAN populated\n");
	} else {
		LOGGER_0("JACOBIAN aborted\n");
		printf("JACOBIAN aborted\n");
		abort();
	}

// compute loss based on JACOBIAN with values from jac tuner slot 0
	fxb4=(compute_loss_jac_dir(tuner.results, tuner.len, 0,tuner.jac, tuner.ivar, tuner.nvar, tuner.pcount))/tuner.len;
	LOGGER_0("INIT JAC loss %f\n", fxb4);
	printf("INIT JAC loss %f\n", fxb4);
#endif
	for(ll=0;ll<1;ll++) {
	
#if 0

// jac: copy values from slot 16 to slot 0. Slot 0 is temporary, after tuner run slot 15 contains best values
	copy_vars_jac(16,0,tuner.ivar, tuner.nvar, tuner.pcount);
	copy_vars_jac(16,15,tuner.ivar, tuner.nvar, tuner.pcount);

// adadelta JAC based
	LOGGER_0("ADADelta JAC\n");
	tuner.method=1;
	tuner.la1=0.8;
	tuner.la2=0.9;
	printf("ADADelta JAC %ld %f %f %f\n", tuner.batch_len, tuner.la1, tuner.la2, tuner.adadelta_step);
	texel_test_loop_jac(&tuner, "../texel/ptest_adeltaJ_", &tun2, fxb4);
// store best result into jac slot 2
	copy_vars_jac(15,2,tuner.ivar, tuner.nvar, tuner.pcount);

#endif
#if 0

// rmsprop from JAC, restore initial values from slot 16
	copy_vars_jac(16,0,tuner.ivar, tuner.nvar, tuner.pcount);
	copy_vars_jac(16,15,tuner.ivar, tuner.nvar, tuner.pcount);

	LOGGER_0("RMSprop JAC\n");
	tuner.method=2;
	tuner.la1=0.8;
	tuner.la2=0.9;
	printf("RMSprop JAC %ld %f %f %f\n", tuner.batch_len, tuner.la1, tuner.la2, tuner.rms_step);
	texel_test_loop_jac(&tuner, "../texel/ptest_rmsJ_", &tun2, fxb4);
//	texel_test_loop_jac(&tuner, "../texel/ptest_rmsJ_", NULL, fxb4);

// store best result into slot 1
	copy_vars_jac(15,1,tuner.ivar, tuner.nvar, tuner.pcount);

#endif
#if 1
	copy_vars_jac(16,0,tuner.ivar, tuner.nvar, tuner.pcount);
	copy_vars_jac(16,15,tuner.ivar, tuner.nvar, tuner.pcount);
// adam JAC
	tuner.method=0;
	tuner.la1=0.9;
	tuner.la2=0.99;
	printf("ADAM JAC %ld %f %f %f\n", tuner.batch_len, tuner.la1, tuner.la2, tuner.adam_step);
	texel_test_loop_jac(&tuner, "../texel/ptest_adamJ_", &tun2, fxb4);
// store best result into slot 3
//	dump_vars_jac(15, tuner.nvar, tuner.pcount);
	copy_vars_jac(15,3,tuner.ivar, tuner.nvar, tuner.pcount);

#endif

/*
 * Verifications?
 */

// verification run

// Various loss computations

#if 1
#endif
#if 1
// compute INIT loss JAC based, values from jac tuner
	copy_vars_jac(16,0,tuner.ivar, tuner.nvar, tuner.pcount);
	fxbj=(compute_loss_jac_dir(tun2.results, tun2.len, 0,tun2.jac, tuner.ivar, tuner.nvar, tun2.pcount))/tun2.len;
	LOGGER_0("INIT verification JAC, loss %f\n", fxbj);
	printf("INIT verfication JAC, loss %f\n", fxbj);
#endif
#if 0

// rmsprop, loss jac, values from jac tuner, from slot 1 to slot 0
	copy_vars_jac(1,0,tuner.ivar, tuner.nvar, tuner.pcount);

	fxbj=(compute_loss_jac_dir(tun2.results, tun2.len, 0,tun2.jac, tuner.ivar, tuner.nvar, tuner.pcount))/tun2.len;
	LOGGER_0("RMSprop JAC tuner, JAC loss %f\n", fxbj);
	printf("RMSprop JAC tuner, JAC loss %f\n", fxbj);

#endif
#if 0

// adadelta, loss jac, values from jac tuner, from slot 2 to slot 0
	copy_vars_jac(2,0,tuner.ivar, tuner.nvar, tuner.pcount);

	fxb1=(compute_loss_jac_dir(tun2.results, tun2.len, 0,tun2.jac, tuner.ivar, tuner.nvar, tuner.pcount))/tun2.len;
	LOGGER_0("ADADelta JAC tuner,  JAC loss %f\n", fxb1);
	printf("ADADelta JAC tuner, JAC loss %f\n", fxb1);

#endif
#if 1
	copy_vars_jac(3,0,tuner.ivar, tuner.nvar, tuner.pcount);
	fxb1=(compute_loss_jac_dir(tun2.results, tun2.len, 0,tun2.jac, tuner.ivar, tuner.nvar, tuner.pcount))/tun2.len;
	printf("%d:ADAM JAC tuner, JAC loss %f, %f, %f\n", ll, fxb1, tuner.reg_la, tuner.adam_step);
	LOGGER_0("%d:ADAM JAC tuner, JAC loss %f, %f, %f\n", ll, fxb1, tuner.reg_la, tuner.adam_step);
	
#endif
	//	tuner.adam_step/=10;
	//	tuner.reg_la/=2;
	}
	free_jac(tuner.jac);
	free_jac(tun2.jac);
	texel_test_fin(&tun2);
	texel_test_fin(&tuner);
}

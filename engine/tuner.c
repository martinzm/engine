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



double supercost;

//int meval_table_gen(meval_t *t, personality *p, int stage);

int variables_reinit_material(void *data){
tuner_variables_pass *v;

	v=(tuner_variables_pass*)data;
	if(v->stage==0) {
		meval_table_gen(v->p->mat , v->p, 0);
	} else {
		meval_table_gen(v->p->mate_e,v->p, 1);
	}
//	mat_info(v->p->mat_info);
//	mat_faze(v->p->mat_faze);
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
//	mat_info(v->p->mat_info);
//	mat_faze(v->p->mat_faze);
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
		i++;
#endif
#if 1
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
			i++;
		}
#endif
#if 0

	// pawn protected XXX
		for(gs=0;gs<=1;gs++) {
			mat[i].init_f=NULL;
			mat[i].restore_f=NULL;
			mat[i].init_data=NULL;
				mat[i].upd=0;
				mat[i].u[0]=&p->pawn_protect[gs];
				mat[i].mid=0;
				mat[i].ran=10000;
				mat[i].max=mat[i].ran/2+mat[i].mid;
				mat[i].min=mat[i].mid-mat[i].ran/2;
				i++;
		}
#endif
#if 1

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
				i++;
		}
#endif
#if 0

	// pawn backward fixable XXX
		for(gs=0;gs<=1;gs++) {
			mat[i].init_f=NULL;
			mat[i].restore_f=NULL;
			mat[i].init_data=NULL;
				mat[i].upd=0;
				mat[i].u[0]=&p->backward_fix_penalty[gs];
				mat[i].mid=0;
				mat[i].ran=10000;
				mat[i].max=mat[i].ran/2+mat[i].mid;
				mat[i].min=mat[i].mid-mat[i].ran/2;
				i++;
		}
#endif
#if 0

	// pawn doubled XXX
		for(gs=0;gs<=1;gs++) {
			mat[i].init_f=NULL;
			mat[i].restore_f=NULL;
			mat[i].init_data=NULL;
				mat[i].upd=0;
				mat[i].u[0]=&p->doubled_penalty[gs];
				mat[i].mid=0;
				mat[i].ran=10000;
				mat[i].max=mat[i].ran/2+mat[i].mid;
				mat[i].min=mat[i].mid-mat[i].ran/2;
				i++;
		}
#endif
#if 1
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
					i++;
				}
			}
#endif
#if 1
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
					i++;
				}
			}
#endif
#if 1
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
						i++;
					}
				}
#endif
#if 1
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
						i++;
				}
#endif
#if 1
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
						i++;
				}
#endif

#if 0
// king safety
	for(gs=0;gs<=1;gs++) {
		for(sq=0;sq<=6;sq++) {
			mat[i].init_f=NULL;
			mat[i].restore_f=NULL;
			mat[i].init_data=NULL;
			mat[i].upd=1;
			mat[i].u[0]=&p->king_s_pdef[gs][WHITE][sq];
			mat[i].u[1]=&p->king_s_pdef[gs][BLACK][sq];
			mat[i].mid=0;
			mat[i].ran=10000;
			mat[i].max=mat[i].ran/2+mat[i].mid;
			mat[i].min=mat[i].mid-mat[i].ran/2;
			i++;
		}
	}
#endif
#if 0
	for(gs=0;gs<=1;gs++) {
		for(sq=0;sq<=6;sq++) {
			mat[i].init_f=NULL;
			mat[i].restore_f=NULL;
			mat[i].init_data=NULL;
			mat[i].upd=1;
			mat[i].u[0]=&p->king_s_patt[gs][WHITE][sq];
			mat[i].u[1]=&p->king_s_patt[gs][BLACK][sq];
			mat[i].mid=0;
			mat[i].ran=10000;
			mat[i].max=mat[i].ran/2+mat[i].mid;
			mat[i].min=mat[i].mid-mat[i].ran/2;
			i++;
		}
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
				i++;
			}
		}
	}
#endif

#if 1
	int pieces_in[]= { 0, 1, 2, 3, 4, 5, -1  };
//	int pieces_in[]= { 4, -1  };
	
	for(gs=0;gs<=1;gs++) {
//		for(pi=0;pi<=5;pi++) {
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
				i++;
			}
			ii++;
		}
	}
#endif

#if 1
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
			i++;
	}
#endif

#if 1
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
			i++;
	}
#endif

#if 1
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

#if 1
// pawn_iso
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
			i++;
	}
#endif
#if 1
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
			i++;
	}
#endif
#if 0
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
			i++;
	}
#endif
#if 0
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
			i++;
	}
#endif
#if 1
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
			i++;
		}
	}
#endif
#if 1
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
			i++;
		}
	}

#endif
#if 1
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
			i++;
		}
	}
#endif
#if 1
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
		i++;
		ii++;
	}
#endif
return i;
}

// val in range <min,max>, converted to range <-1,1>
double norm_val(double val, double range, double mid)
{
	return (val-mid)*2/range;
}

// from <-1,1> to original range
double unnorm_val(double norm, double range, double mid)
{
	return (norm*range/2+mid);
}

double calc_dir_penalty_single(double var, double lambda)
{
	return lambda*(var*var);
//	return (var*var);
}

double calc_dir_penalty( matrix_type *m, tuner_global *tun, int pcount)
{
int i;
double pen=0.0;
		for(i=0;i<pcount;i++) {
			// compute loss
//			pen+=tun->reg_la*(pow(norm_val(*(m[i].u[0]), m[i].ran,m[i].mid),2));
//			pen+=tun->reg_la*(pow(*(m[i].u[0]));
			pen+= calc_dir_penalty_single(*(m[i].u[0]),tun->reg_la);
		}
//return pen/tun->len;
return pen;
}

double calc_dir_penalty_jac(double *var, matrix_type *m, tuner_global *tun, int pcount)
{
int i;
double pen=0.0;
		for(i=0;i<pcount;i++) {
			// compute loss
//			pen+=tun->reg_la*(pow(norm_val(var[i], m[i].ran,m[i].mid),2));
//			pen+=tun->reg_la*(pow(var[i],2));
			pen+= calc_dir_penalty_single(*(m[i].u[0]),tun->reg_la);
		}
return pen;
}


double calc_dir_penalty_jac2(double *var, matrix_type *m, tuner_global *tun, int pcount)
{
int i;
double pen=0.0;
		for(i=0;i<pcount;i++) {
			// compute loss
//			pen+=calc_dir_penalty_single(norm_val(var[i], m[i].ran,m[i].mid), tun->reg_la);
			pen+=calc_dir_penalty_single((var[i]), tun->reg_la);
		}
return pen;
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
#define LK1 (3.0)
#define LK2 (8000.0)
//#define LK2 (2500.0)

/* 
 * ry - result 2/1/0 - white won, draw, black won
 * ev - evaluation result positive favors white, negative favors black
 * h0 - remapped ev via sigmoid to 2 - 0 range/probability - 2 for white, 0 for black, 1 remis
 * return - gives error, ie  how far is expected result (h0) from real result (ry)
 */


double comp_cost_x(double ev, double ry){
double h0, ret;
	h0=(2.0/(1+pow(10, (0-LK1)*ev/LK2)))/2;
	if(h0==1) {
//		printf("%lF, %lF\n", h0, ev);
//		LOGGER_0("%lF, %lF\n", h0, ev);
	}
	ret=(-log(h0+(1E-10))*ry/2.0-log(1-h0+(1E-10))*(1-ry/2.0));
	return ret;
}

double comp_cost(double ev, double ry){
double h0;
	h0=(2.0/(1+pow(10,(0-LK1)*ev/LK2)));

//	printf("cost Ev %lF, SIG %lF, ry %lF\n",ev, h0, ry);
//	supercost+=h0/2;
	return pow((ry-h0)/2,2);
}

double compute_loss(board *b, int8_t *rs, uint8_t *ph, personality *p, int count, int *indir, long offset)
{
double res, r1, r2, ry, cost, h0, ev, t;
attack_model a;
struct _ui_opt uci_options;
struct _statistics s;
int i, q;
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
//		printf("Res EV %lF, ry %lF, cost %lF\n",ev, ry, t);
		res+=t;
	}
return res;
}

double compute_loss_dir(board *b, int8_t *rs, uint8_t *ph, personality *p, int count, long offset)
{
double res, r1, r2, ry, cost, h0, ev;
attack_model a;
struct _ui_opt uci_options;
struct _statistics s;
int evi,i, q;
	res=0;
	for(i=0;i<count;i++) {
//		printf("%d\n",i);
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

double compute_loss_dir_x(board *b, int8_t *rs, uint8_t *ph, personality *p, int count, long offset)
{
double res, r1, r2, ry, cost, h0, ev;
attack_model a;
struct _ui_opt uci_options;
struct _statistics s;
int evi,i, q;
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

void dump_jac(double *J, int count, int *indir, long offset, double *ivar, double *nvar, int pcount)
{
double *JJ, fxh2;
int i, pos, q;

	printf("Dump JAC\n");
	LOGGER_0("Dump JAC\n");
	for(pos=0;pos<count;pos++) {
		q=indir[pos+offset];
		JJ=J+q*(pcount+4);
		fxh2=0;
		for(i=0;i<pcount;i++) {
			printf("P:%d=%lf\tI:%lf\tN:%lf\n", i, JJ[i], ivar[i], nvar[i]);
			LOGGER_0("P:%d=%lf\tI:%lf\tN:%lf\n", i, JJ[i], ivar[i], nvar[i]);
			fxh2+=JJ[i]*(nvar[i]-ivar[i]);
		}		
		printf("D: rest %lf ,eval comp: %lf, eval fx0: %lf, eval dyn: %lf\n", JJ[pcount], fxh2, JJ[pcount+1], JJ[pcount+2]);
		LOGGER_0("D: rest %lf ,eval comp: %lf, eval fx0: %lf, eval dyn: %lf\n", JJ[pcount], fxh2, JJ[pcount+1], JJ[pcount+2]);
	}
}

// f(x)=f(x0)+df/dx(x0)*(x-x0)

int recompute_jac(double *JJ, int count, int *indir, long offset, double *ivar, double *nvar, int pcount)
{
	int pos;
	double *J, fxh;
	int i,q;

	for(pos=0;pos<count;pos++) {
		q=indir[pos+offset];
		J=JJ+q*(pcount+4);
//		fxh=J[pcount+1];
		fxh=0;
		for(i=0;i<pcount;i++) {
			fxh+=(nvar[i]-ivar[i])*J[i];
//			printf("RR P:%d=%lf\tI:%lf\tN:%lf\n", i, J[i], ivar[i], nvar[i]);
//			LOGGER_0("RR P:%d=%lf\tI:%lf\tN:%lf\n", i, J[i], ivar[i], nvar[i]);
		}
		J[pcount+2]=fxh+J[pcount+1];
//		printf("RR D: rest %lf ,eval comp: %lf, eval fx0: %lf, eval dyn: %lf\n", J[pcount], fxh, J[pcount+1], J[pcount+2]);
//		LOGGER_0("RR D: rest %lf ,eval comp: %lf, eval fx0: %lf, eval dyn: %lf\n", J[pcount], fxh, J[pcount+1], J[pcount+2]);
	}
return 0;
}

double compute_loss_jac(int8_t *rs, int count, int *indir, long offset,double *JJ, double *ival, double *nval, int pcount)
{
double res, r1, r2, ry, cost, h0, nv, iv, ev;
double *J;
int ii, i, q;
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
		J=JJ+q*(pcount+4);
		ev= J[pcount+1];
		for(i=0;i<pcount;i++) {
			ev+=(nval[i]-ival[i])*J[i];
		}
		ry=rs[q];
		res+=comp_cost(ev, ry);
	}
return res;
}

double compute_loss_jac_diff(int8_t *rs, int count, int *indir, long offset, double *JJ, double *ival, double *nval, int i, double new_val, int pcount)
{
double res, ry, cost, ev, ev2, o;
double *J;
int ii,nn, q;

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
		J=JJ+q*(pcount+4);
		ev2=J[pcount+1];
		o=nval[i];
		nval[i]=new_val;
//		printf("Init:ii%d==>EV2:%.10lf, EV:%.10lf, vIdx:%d, VAL:%.10lf, new_VAL:%.10lf, J[i]:%.10lf\n", ii, J[pcount+1], J[pcount+2], i, o, new_val, J[i]);
//		nlogger2("Init:ii%d==>EV2:%.10lf, EV:%.10lf, vIdx:%d, VAL:%.10lf, new_VAL:%.10lf, J[i]:%.10lf\n", ii, J[pcount+1], J[pcount+2], i, o, new_val, J[i]);

//		for(nn=0;nn<pcount;nn++) {
//			ev2+=(nval[nn]-ival[nn])*J[nn];
//			printf("nn:%d==>nval[nn]:%.10lf, ival[nn]:%.10lf, J[nn]:%.10lf==%.10lf\n", nn,nval[nn], ival[nn], J[nn], (nval[nn]-ival[nn])*J[nn]);
//			nlogger2("nn:%d==>nval[nn]:%.10lf, ival[nn]:%.10lf, J[nn]:%.10lf==%.10lf\n", nn,nval[nn], ival[nn], J[nn], (nval[nn]-ival[nn])*J[nn]);
//		}
		nval[i]=o;

		ev= J[pcount+2];
		ev+=(new_val-nval[i])*J[i];
//		printf("EVAL: ev2:%.10lf  ev:%.10lf\n", ev2, ev);
//		nlogger2("EVAL: ev2:%.10lf  ev:%.10lf\n", ev2, ev);
		
//		if(ev2!=ev) {
//			printf("Diff st vs diff: %.10lf!=%.10lf, c=%d, pc=%d, nval=%.10lf;;%.10lf\n", ev2, ev, ii, i, new_val,nval[i]);
//			LOGGER_0("Diff st vs diff: %.10lf!=%.10lf, c=%d, pc=%d, nval=%.10lf;;%.10lf\n", ev2, ev, ii, i, new_val, nval[i]);
//		}
		ry=rs[q];
		res+=comp_cost(ev, ry);
	}
return res;
}

double compute_loss_jac_dir(int8_t *rs, int count, long offset,double *JJ, double *ival, double *nval, int pcount)
{
double res, r1, r2, ry, cost, h0, ev;
double *J;
int ii, i, q;
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

		J=JJ+q*(pcount+4);
		ev=(double)J[pcount+1];
		for(i=0;i<pcount;i++) {
			ev+=(double)(nval[i]-ival[i])*J[i];
		}
		ry=rs[q];
		res+=comp_cost(ev, ry);
	}
return res;

}

// tuner_run - runtime variables needed for tuner,incl real representation of values/parameters
// matrix_type - matrix of pointers to int values/parameters for tuning

int init_tuner(tuner_run *state,matrix_type *m, int pcount){
int i;
	for(i=0;i<pcount;i++) state[i].or1=0.0000000;
	for(i=0;i<pcount;i++) state[i].or2=0.0000000;
//	for(i=0;i<pcount;i++) state[i].update=*(m[i].u[0]);
	for(i=0;i<pcount;i++) state[i].update=0;
	for(i=0;i<pcount;i++) state[i].grad=0;
//	for(i=0;i<pcount;i++) state[i].real=norm_val(*(m[i].u[0]),m[i].ran,m[i].mid);
	for(i=0;i<pcount;i++) state[i].real=*(m[i].u[0]);
	return 0;
}

int init_tuner_jac(tuner_run *state,matrix_type *m, double *var, int pcount){
int i;
	for(i=0;i<pcount;i++) state[i].or1=0.0000001;
	for(i=0;i<pcount;i++) state[i].or2=0.0000001;
//	for(i=0;i<pcount;i++) state[i].update=*(m[i].u[0]);
	for(i=0;i<pcount;i++) state[i].update=0;
	for(i=0;i<pcount;i++) state[i].grad=0;
	for(i=0;i<pcount;i++) state[i].real=var[i];
//	for(i=0;i<pcount;i++) state[i].real=norm_val(*(m[i].u[0]),m[i].ran,m[i].mid);
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


int allocate_tuner(tuner_run **tr, int pcount){
//tuner_run *t;
	*tr=malloc(sizeof(tuner_run)*(pcount+1));
	return 0;
}

int backup_matrix_values(matrix_type *m, int *backup, int pcount){
int i;
	for(i=0;i<pcount;i++) {
		backup[i]=*(m[i].u[0]);
//		printf("BI:%d\n",backup[i]);
	}
	return 0;
}

int restore_matrix_values(int *backup, matrix_type *m, int pcount){
	int i, ii, on;
		for(i=0;i<pcount;i++) {
			for(ii=0;ii<=m[i].upd;ii++) {
				*(m[i].u[ii])=backup[i];
			}
			if(m[i].init_f!=NULL) m[i].init_f(m[i].init_data);
		}
	return 0;
}

int restore_matrix_values2(int *backup, matrix_type *m, int pcount){
	int i, ii, on;
		for(i=0;i<pcount;i++) {
			for(ii=0;ii<=m[i].upd;ii++) {
				*(m[i].u[ii])=9999;
			}
			if(m[i].init_f!=NULL) m[i].init_f(m[i].init_data);
		}
	return 0;
}

int dump_matrix_values2(matrix_type *m, int pcount){
	int i, ii, on;
		for(i=0;i<pcount;i++) {
			for(ii=0;ii<=m[i].upd;ii++) {
				printf("%d, ",*(m[i].u[ii]));
			}
		}
		printf("\n");
	return 0;	
}

int copy_vars_jac(int source, int dest, double *ivar, double *nvar, int pcount){
int f;
double *s, *d;
//	printf("***copy vars, dest %d, source %d***\n", dest, source);
	s=ivar+source*pcount;
	d=ivar+dest*pcount;
	for(f=0;f<pcount;f++) { 
		*d=*s; d++; s++; 
	}
	s=nvar+source*pcount;
	d=nvar+dest*pcount;
	for(f=0;f<pcount;f++) { 
		*d=*s; d++; s++; 
	}

return 0;
}

int jac_to_matrix(int source, matrix_type *m, double *var, int pcount ){
double *s;
int i, ii, on;
	s=var+source*pcount;
	for(i=0;i<pcount;i++) {
		on=round(s[i]);
//		printf("toMatrix %d:%f\n", i,s[i]);
		for(ii=0;ii<=m[i].upd;ii++) {
			*(m[i].u[ii])=on;
		}
		if(m[i].init_f!=NULL) m[i].init_f(m[i].init_data);
	}
return 0;
}

int matrix_to_jac(int dest, matrix_type *m, double *var, int pcount ){
double *d;
int i;
	d=var+dest*pcount;
	for(i=0;i<pcount;i++) {
		d[i]=*(m[i].u[0]);
	}
return 0;
}

/*
 * count  - pocet B
 * pcount - pocet parametru
 */

void p_tuner(board *b, int8_t *rs, uint8_t *ph, personality *p, int count, matrix_type *m, tuner_global *tun, tuner_run *state, int pcount, char * outp, int* indir, long offset, int iter)
{
	int step, diff, ioon;
	double fx, fxh, fxh2, fxh3, fxt, x,y,z,zt, fxdiff, oon, g_reg, y_hat, x_hat, rr, r, pen, pen2, pen_tem;
	//!!!!
	int m_back[2048];
	int i, n, sq, ii;
	int o,q,g, on;
	int gen;

	n=0;

	for(gen=0;gen<1; gen++) {
	pen=calc_dir_penalty(m, tun, pcount);

		// loop over parameters
		for(i=0;i<pcount;i++) {
			pen2=calc_dir_penalty_single(*m[i].u[0], tun->reg_la);
			step=2;
			// get parameter value
			o=*(m[i].u[0]);
			on=o+step;
			
			// iterate over the same parameters and update them with change;
			for(ii=0;ii<=m[i].upd;ii++) {
				*(m[i].u[ii])=on;
			}
			if(m[i].init_f!=NULL) m[i].init_f(m[i].init_data);

			pen_tem=pen-pen2+calc_dir_penalty_single(*m[i].u[0], tun->reg_la);
			// compute loss
			fxh=compute_loss(b, rs, ph, p, count, indir, offset)/count+pen_tem/count;
//			fxh=compute_loss(b, rs, ph, p, count, indir, offset)/count;

			on=o-step;
			for(ii=0;ii<=m[i].upd;ii++) {
				*(m[i].u[ii])=on;
			}
			if(m[i].init_f!=NULL) m[i].init_f(m[i].init_data);
			pen_tem=pen-pen2+calc_dir_penalty_single(*m[i].u[0], tun->reg_la);
			fxh2=compute_loss(b, rs, ph, p, count, indir, offset)/count+pen_tem/count;
//			fxh2=compute_loss(b, rs, ph, p, count, indir, offset)/count;
			// compute gradient
			fxdiff=fxh-fxh2;
			state[i].grad=(fxdiff)/(2*step);
			//restore original values
			for(ii=0;ii<=m[i].upd;ii++) {
				*(m[i].u[ii])=o;
			}
			if(m[i].init_f!=NULL) m[i].init_f(m[i].init_data);
		}
		// gradient descent
		for(i=0;i<pcount;i++) {
			if(tun->method==2) {
				/*
				 * rmsprop
				 */
				// accumulate gradients
				state[i].or2=(state[i].or2*tun->la2)+(pow(state[i].grad,2))*(1-tun->la2); //!!!
				// compute update
				y=sqrt(state[i].or2+tun->small_c); //!!!
				// update
				r=0-state[i].grad/y;
			}
			else if(tun->method==1){
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
			} else {
				/*
				 * Adam mod.
				 */
				// accumulate gradients
				state[i].or2=(state[i].or2*tun->la2)+(pow(state[i].grad,2))*(1-tun->la2);
				// accumulate grads
				state[i].or1=(state[i].or1*tun->la1)+(pow(state[i].grad,1))*(1-tun->la1);
				// compute update
//				iter=1;
				y_hat=state[i].or2/(1.0-pow(tun->la2, iter));
				x_hat=state[i].or1/(1.0-pow(tun->la1, iter));
				x=x_hat;
				y=sqrt(y_hat)+tun->small_c;
				// update
				r=0-x/y;
			}
			rr=state[i].real;
			state[i].real+=(r*tun->temp_step);
//			state[i].real=Max(-1, Min(1, state[i].real));
			state[i].update=state[i].real-rr;

			oon=state[i].real; 

			// check limits
			for(ii=0;ii<=m[i].upd;ii++) {
				*(m[i].u[ii])=round(oon);
			}
			if(m[i].init_f!=NULL) m[i].init_f(m[i].init_data);
		}
		n++;
	}
}

// minimalizovat 

/*
 * count  - pocet B
 * pcount - pocet parametru
 */

void p_tuner_jac(int8_t *rs, int count, matrix_type *m, tuner_global *tun, tuner_run *state, double *ivar, double *nvar, int pcount, char * outp, int* indir, long offset, int iter)
{
	int diff, ioon;
	double fx, fxh, fxhd, fxh2, fxh2d, fxh3, fxt, r,rr,x,y,z, fxdiff, fxdiffd, oon, g_reg, y_hat, x_hat, o, on, step, penalty, pen_tem, pen_te1, pen_te2, p2;
	//!!!!
	int i, n, sq, ii;
	int q,g;
	int gen;

	for(gen=0;gen<1; gen++) {
		// loop over parameters

		recompute_jac(tun->jac, count, indir, offset, ivar, nvar, pcount);

		penalty=recompute_penalty(nvar, pcount, tun->reg_la);
		for(i=0;i<pcount;i++) {
// compute gradient for cost function
			step=2;
			// get parameter value			
			o=nvar[i];
			pen_tem=calc_dir_penalty_single(o, tun->reg_la);

			on=o+step;			
			// iterate over the same parameters and update them with change;
			pen_te1=penalty+calc_dir_penalty_single(on, tun->reg_la)-pen_tem;
			
			// compute loss
//			nvar[i]=o;
			fxhd=(compute_loss_jac_diff(rs, count, indir, offset, tun->jac, ivar, nvar, i, on, pcount)+pen_te1)/count;
//			nvar[i]=on;
//			fxh=(compute_loss_jac(rs, count, indir, offset, tun->jac, ivar, nvar, pcount)+pen_te1)/count;
			
			on=o-step;
			pen_te2=penalty+calc_dir_penalty_single(on, tun->reg_la)-pen_tem;
						
//			nvar[i]=o;
			fxh2d=(compute_loss_jac_diff(rs, count, indir, offset, tun->jac, ivar, nvar, i, on, pcount)+pen_te2)/count;
//			nvar[i]=on;
//			fxh2=(compute_loss_jac(rs, count, indir, offset, tun->jac, ivar, nvar, pcount)+pen_te2)/count;
			
			// compute gradient for cost functions
//			fxdiff=fxh-fxh2;
			// diff version
			fxdiffd=fxhd-fxh2d;
			
//			printf("!");
//			LOGGER_0("!");
//			if(fxdiff!=fxdiffd) {
//				printf("Difference! %.10lf; Orig Fxh %.10lf, Fxh2 %.10lf, Fxdiff %.10lf VS diff Fxh %.10lf, Fxh2 %.10lf, Fxdiff %.10lf\n", fxdiffd-fxdiff, fxh, fxh2, fxdiff, fxhd, fxh2d, fxdiffd);
//				LOGGER_0("Difference! %.10lf; Orig Fxh %.10lf, Fxh2 %.10lf, Fxdiff %.10lf VS diff Fxh %.10lf, Fxh2 %.10lf, Fxdiff %.10lf\n", fxdiffd-fxdiff, fxh, fxh2, fxdiff, fxhd, fxh2d, fxdiffd);
//			}
			
//			state[i].grad=(fxdiff)/(2*step);
			state[i].grad=(fxdiffd)/(2*step);
			//restore original values
//			nvar[i]=o;
		}
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
			state[i].update=state[i].real-rr;

//			oon=unnorm_val(state[i].real,m[i].ran,m[i].mid);
			oon=state[i].real;
			nvar[i]=round(oon);
		}
		// recompute jacobian

//		dump_tuner_jac(state, pcount);		
//		recompute_jac(tun->jac, count, indir, offset, ivar, nvar, pcount);
	}
}

double * allocate_jac(int records, int params){
double *J;

//	printf ("double %li, long double %li, request %Lf\n", sizeof(double), sizeof(long double),(long double) (sizeof(double)*(params+3)*records));
	J=(double*)malloc(sizeof(double)*(params+4)*records);
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
int populate_jac(double *J,board *b, int8_t *rs, uint8_t *ph, personality *p, int start, int stop, matrix_type *m, int pcount)
{
	int diff_step, pos;
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
//		JJ=J+pos*(pcount+4);
		for(pos=start;pos<=stop;pos++) {
			JJ=J+pos*(pcount+4); 
			// compute eval
			JJ[i]=compute_eval_dir(b, ph, p, pos);
//			printf("POP JJ[i]:%.10lf, %d:i, %d:pos\n", JJ[i], i, pos);
//			nlogger2("POP JJ[i]:%.10lf, %d:i, %d:pos\n", JJ[i], i, pos);
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
			JJ=J+pos*(pcount+4);
			fxh2=compute_eval_dir(b, ph, p, pos);
			// compute gradient/partial derivative
			fxdiff=JJ[i]-fxh2;
			JJ[i]=(fxdiff)/(2*diff_step);
//			printf("PO2 JJ[i]:%.10lf, fxh2:%d, diff:%.10lf, %d\n", JJ[i], fxh2, fxdiff, i);
//			nlogger2("PO2 JJ[i]:%.10lf, fxh2:%d, diff:%.10lf, %d\n", JJ[i], fxh2, fxdiff, i);
		}

//restore original values
		for(ii=0;ii<=m[i].upd;ii++) {
			*(m[i].u[ii])=o;
		}
		if(m[i].init_f!=NULL) m[i].init_f(m[i].init_data);
		printf("*");
	}

// iterate positions
	for(pos=start;pos<=stop;pos++) {
		JJ=J+pos*(pcount+4);
		
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

int populate_jac_old(double *J,board *b, int8_t *rs, uint8_t *ph, personality *p, int start, int stop, matrix_type *m, int pcount)
{
	int diff_step, pos;
	int fxh, fxh2;
	double fxdiff, *JJ;
	//!!!!
	int i,ii;
	int o,q,g, on;

	for(pos=start;pos<=stop;pos++) {
		// loop over parameters
		JJ=J+pos*(pcount+4);
		for(i=0;i<pcount;i++) {
			// get parameter value
			diff_step=m[i].ran/4;
			o=*(m[i].u[0]);
			on=o+diff_step;
			for(ii=0;ii<=m[i].upd;ii++) {
				*(m[i].u[ii])=on;
			}
			if(m[i].init_f!=NULL) m[i].init_f(m[i].init_data);
			// compute eval
			fxh=compute_eval_dir(b, ph, p, pos);
			on=o-diff_step;
			for(ii=0;ii<=m[i].upd;ii++) {
				*(m[i].u[ii])=on;
			}
			if(m[i].init_f!=NULL) m[i].init_f(m[i].init_data);
			fxh2=compute_eval_dir(b, ph, p, pos);
			// compute gradient/partial derivative
			fxdiff=fxh-fxh2;
			JJ[i]=(fxdiff)/(2*diff_step);
			//restore original values
			for(ii=0;ii<=m[i].upd;ii++) {
				*(m[i].u[ii])=o;
			}
			if(m[i].init_f!=NULL) m[i].init_f(m[i].init_data);
		}
		fxh=compute_eval_dir(b, ph, p, pos);
// recompute score with gradients

		fxh2=0;
		for(i=0;i<pcount;i++) {
			fxh2+=*(m[i].u[0])*JJ[i];
		}
		JJ[i++]=fxh-fxh2;
		JJ[i++]=fxh;
		JJ[i++]=fxh;
		if((pos%10)==0) {
			printf("*");
		}
	}
//	printf("\n");
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
//	populate_jac_old(d->J,d->b, d->rs, d->ph, d->p, d->start, d->stop, d->m, d->pcount);
	return arg;
}

// do populate jac multithreaded
int populate_jac_pl(double *J,board *b, int8_t *rs, uint8_t *ph, personality *p, int start, int stop, matrix_type *m, int pcount, int threads){
void * status;
pthread_t th[10];
_tt_data data[10];
pthread_attr_t attr[10];
int f,pos,s,r;

	if(threads>10) threads=10;
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

int texel_load_files2(tuner_global *tuner){
//	char *sts_tests[]= { "../texel/quiet-labeled.epd" };
	char *sts_tests[]= { "../texel/lichess-quiet.txt" };
	int8_t r;
	FILE * handle;
	char filename[256];
	char buffer[512];
	char res[128];
	char fen[512];
	char *name;

	int i,l,x;
	char *xx;
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
	printf("L2: Imported %d from total %d of records\n", tuner->len, i);
	return 1;
}

int texel_load_files(tuner_global *tuner){
//	char *sts_tests[]= { "texel/0.5-0.5.txt" };
//	int tests_setup[]= { 1, -1 };
// results from white pov
	char *sts_tests[]= { "../texel/1-0.txt", "../texel/0.5-0.5.txt", "../texel/0-1.txt" };

//	char *sts_tests[]= { "../texel/1-0.epd", "../texel/0.5-0.5.epd", "../texel/0-1.epd" };
	int8_t tests_setup[]= { 2, 1, 0, -1 };
	FILE * handle;
	char filename[256];
	char buffer[512];
	char fen[512];
	char *name;

	int i,l,x;
	int sc_b, sc_e;
	char *xx;
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
//					LOGGER_0("Load1 %d, %"PRIi8 ", %"PRIi8"==%s\n",tuner->phase[tuner->len],tuner->results[tuner->len],tests_setup[l], buffer);
					tuner->len++;
				}
				free(name);
				i++;
			}
		}
		fclose(handle);
		l++;
	}
	printf("L1: Imported %d from total %d of records\n", tuner->len, i);
	return 1;
}

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


void texel_test_loop_jac(tuner_global *tuner, char * base_name)
{
	int n,i,l;
	tuner_run *state;

	unsigned long long int totaltime;
	struct timespec start, end;

	int gen, perc, ccc;
	int *rnd, *rids, rrid, r1,r2;
	char nname[256];
	double fxh, fxh2=0, fxh3, fxb,t;
	
	double *cvar;

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
//	recompute_jac(tun->jac, count, indir, offset, ivar, nvar, pcount);
//	recompute_jac(tuner->jac, tuner->len, rnd, 0, tuner->ivar, tuner->nvar, tuner->pcount);
	fxb=fxh=compute_loss_jac_dir(tuner->results, tuner->len, 0, tuner->jac, tuner->ivar, tuner->nvar, tuner->pcount)/tuner->len;

	init_tuner_jac(state, tuner->m, tuner->nvar, tuner->pcount);

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
			sprintf(nname,"%s_%d_%d.xml",base_name, tuner->batch_len,gen);
			// compute loss prior tuning
//			printf("GEN %d, blen %d, Initial loss of whole data =%f JAC\n", gen, tuner->batch_len, fxh);
			LOGGER_0("GEN %d, blen %d, Initial loss of whole data =%.10f JAC\n", gen, tuner->batch_len, fxh);

//			printf("Dump1\n");
//			dump_matrix_values2(tuner->m, tuner->pcount);

			// tuning part
			// in minibatches
			ccc=1;
			i=0;
			perc=10;
			while(tuner->len>i) {
				l= ((tuner->len-i)>=tuner->batch_len) ? tuner->batch_len : tuner->len-i;
				p_tuner_jac(tuner->results, l, tuner->m, tuner, state, tuner->ivar, tuner->nvar, tuner->pcount, nname, rnd,i, ccc);
				ccc++;
				if((i*100/tuner->len) > perc) {
					fflush(stdout);
					perc+=10;
				}
				i+=l;
			}
//			dump_tuner_jac(state, tuner->pcount);
			fxh3=compute_loss_jac_dir(tuner->results, tuner->len, 0, tuner->jac, tuner->ivar, tuner->nvar, tuner->pcount);
			fxh2=fxh3/tuner->len;
			readClock_wall(&end);
			totaltime=diffClock(start, end);
			printf("GEN %d, blen %d, Final loss of whole data =%.10f:%.10f, %.10f\n\n", gen, tuner->batch_len, fxh2, fxb, fxh3);
			LOGGER_0("GEN %d, blen %d, Final loss of whole data =%.10f\n\n", gen, tuner->batch_len, fxh2);
			if(fxh2<fxb) {
				copy_vars_jac(0,15,tuner->ivar, tuner->nvar, tuner->pcount);
				jac_to_matrix(0, tuner->m, tuner->nvar, tuner->pcount);
				write_personality(tuner->pi, nname);
				fxb=fxh2;
			}
			fxh=fxh2;
		}
	}
	if(rnd!=NULL) free(rnd);
	if(rids!=NULL) free(rids);
	if(state!=NULL) free(state);
}

void texel_test_loop(tuner_global *tuner, char * base_name)
{
	int n,i,l;
	tuner_run *state;

	unsigned long long int totaltime;
	struct timespec start, end;

	int gen, perc, ccc;
	int *rnd, *rids, rrid, r1,r2;
	char nname[256];
	double fxh, fxh2=0, fxb, t;

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
//	tuner->temp_step=t*(tuner->batch_len);
	tuner->temp_step=t;


// init temporary step setting based on record count and batch len

// looping over testing ...

	fxb=fxh=compute_loss_dir(tuner->boards, tuner->results, tuner->phase, tuner->pi, tuner->len, 0)/tuner->len;
	for(gen=1;gen<=tuner->generations;gen++) {
		supercost=0;
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
			init_tuner(state, tuner->m, tuner->pcount);
			sprintf(nname,"%s_%d_%d.xml",base_name, tuner->batch_len,gen);
			// compute loss prior tuning
//			fxh=compute_loss(tuner->boards, tuner->results, tuner->phase, tuner->pi, tuner->len, rnd,0)/tuner->len;
//			printf("GEN %d, blen %d, Initial loss of whole data =%f\n", gen, tuner->batch_len, fxh);
			LOGGER_0("GEN %d, blen %d, Initial loss of whole data =%f\n", gen, tuner->batch_len, fxh);

			// tuning part
			// in minibatches
			ccc=1;
			i=0;
			perc=10;
			while(tuner->len>i) {
				l= ((tuner->len-i)>tuner->batch_len) ? tuner->batch_len : tuner->len-i;
				p_tuner(tuner->boards, tuner->results, tuner->phase, tuner->pi, l, tuner->m, tuner, state, tuner->pcount, nname, rnd,i, ccc);
				ccc++;
				if((i*100/tuner->len) > perc) {
					printf("*");
					fflush(stdout);
					perc+=10;
				}
				i+=l;
			}
//			printf("Supercost %lF\n",supercost);
			fxh2=compute_loss(tuner->boards, tuner->results, tuner->phase, tuner->pi, tuner->len, rnd,0)/tuner->len;
			readClock_wall(&end);
			totaltime=diffClock(start, end);
			printf("\nGEN %d, blen %d,Time: %lldm:%llds.%lld\n", gen, tuner->batch_len, totaltime/60000000,(totaltime%60000000)/1000000,(totaltime%1000000)/1000);
			printf("GEN %d, blen %d, Final loss of whole data =%f, %f,, %f\n", gen, tuner->batch_len, fxh2, fxb, fxh2*tuner->len);
			LOGGER_0("GEN %d, blen %d, Final loss of whole data =%f\n", gen, tuner->batch_len, fxh2);
			if(fxh2<fxb) {
				printf("TUNE improvement\n");
				backup_matrix_values(tuner->m, tuner->matrix_var_backup, tuner->pcount);
				write_personality(tuner->pi, nname);
				fxb=fxh2;
			}
			fxh=fxh2;
		}
	}
	if(rnd!=NULL) free(rnd);
	if(rids!=NULL) free(rids);
	if(state!=NULL) free(state);
}

void texel_test()
{
int i, *iv,ll;
tuner_global tuner, tun2;
double fxb1, fxb2, fxb3, fxbj, lambda;

	supercost=0;

	lambda=0.0000001;
// initialize tuner
	tuner.max_records=10000000;
	texel_test_init(&tuner);

	tuner.generations=10;
	tuner.batch_len=1024;
	tuner.records_offset=0;
	tuner.nth=25;
//	tuner.nth=1;
	tuner.small_c=1E-30;
//	tuner.adam_step=0.001;
	tuner.adam_step=0.001;
//	tuner.rms_step=0.001;
	tuner.rms_step=0.001;
	tuner.adadelta_step=0.01;


// load position files and personality to seed tuning params 
	texel_load_files2(&tuner);

	load_personality("../texel/pers.xml", tuner.pi);

	tuner.reg_la=lambda/tuner.pcount;
//	tuner.reg_la=0;

	tun2.max_records=1000000;
	texel_test_init(&tun2);
	tun2.records_offset=0;
	tun2.nth=10;
	tun2.small_c=1E-20;
	texel_load_files(&tun2);


//	tuner.reg_la=0.00125*tuner.batch_len/tuner.len;

// backup values from values to backup slot 16, normal tuning
	iv=tuner.matrix_var_backup+tuner.pcount*16;
	backup_matrix_values(tuner.m, iv, tuner.pcount);
//	dump_matrix_values2(tuner.m, tuner.pcount);

#if 1
// tranfer values from slot 16 to JAC based tuner - slot 0
	matrix_to_jac(16, tuner.m, tuner.ivar, tuner.pcount );
	matrix_to_jac(16, tuner.m, tuner.nvar, tuner.pcount );
	copy_vars_jac(16,0,tuner.ivar, tuner.nvar, tuner.pcount);
//	for(i=0;i<tuner.pcount;i++) {
//		printf("I:%lf\tN:%lf\n", tuner.ivar[i], tuner.nvar[i]);
//	}

	fxb1=(compute_loss_dir(tuner.boards, tuner.results, tuner.phase, tuner.pi, tuner.len, 0));
	fxb1/=tuner.len;
	LOGGER_0("INIT verification OLD, loss %f %d\n", fxb1, tuner.len);
	printf("INIT verification OLD, loss %f %d\n", fxb1, tuner.len);

// allocate jacobian and compute partial derivatives for each position loaded
	tuner.jac=NULL;
	tuner.jac=allocate_jac(tuner.len, tuner.pcount);
	if(tuner.jac!=NULL) {
		LOGGER_0("JACOBIAN population, positions %d, parameters %d\n", tuner.len, tuner.pcount);
//		printf("JACOBIAN population, positions %d, parameters %d\n", tuner.len, tuner.pcount);
		populate_jac_pl(tuner.jac,tuner.boards, tuner.results, tuner.phase, tuner.pi, 0, tuner.len-1, tuner.m, tuner.pcount, 10);
//		populate_jac(tuner.jac,tuner.boards, tuner.results, tuner.phase, tuner.pi, 0, tuner.len-1, tuner.m, tuner.pcount);
		LOGGER_0("JACOBIAN populated\n");
//		printf("JACOBIAN populated\n");
	}

// compute loss based on JACOBIAN with values from jac tuner slot 0
//	tuner.penalty=calc_dir_penalty_jac(tuner.nvar, tuner.m, &tuner, tuner.pcount);
	fxbj=(compute_loss_jac_dir(tuner.results, tuner.len, 0,tuner.jac, tuner.ivar, tuner.nvar, tuner.pcount))/tuner.len;
	LOGGER_0("INIT JAC loss %f\n", fxbj);
	printf("INIT JAC loss %f\n", fxbj);
#endif
	for(ll=0;ll<1;ll++) {
	
#if 0

// rmsprop
	LOGGER_0("RMSprop OLD\n");
	tuner.method=2;
	tuner.la1=0.8;
	tuner.la2=0.9;
//	tuner.rms_step=0.1;
	printf("RMSprop %d %f %f %f\n", tuner.batch_len, tuner.la1, tuner.la2, tuner.rms_step);
	texel_test_loop(&tuner, "../texel/pers_test_rms_");
// store best result into slot 1
	backup_matrix_values(tuner.m, tuner.matrix_var_backup+tuner.pcount*1, tuner.pcount);

#endif
#if 0
	restore_matrix_values(tuner.matrix_var_backup+tuner.pcount*16, tuner.m, tuner.pcount);
// adadelta
	LOGGER_0("ADADelta\n");
	tuner.method=1;
	tuner.la1=0.8;
	tuner.la2=0.9;
	printf("ADADelta %d %f %f %f\n", tuner.batch_len, tuner.la1, tuner.la2, tuner.adadelta_step);
//	texel_test_loop(&tuner, "../texel/pers_test_adelta_");
// store best result into slot 2
	backup_matrix_values(tuner.m, tuner.matrix_var_backup+tuner.pcount*2, tuner.pcount);

#endif
#if 0
	restore_matrix_values(tuner.matrix_var_backup+tuner.pcount*16, tuner.m, tuner.pcount);
// adam
	LOGGER_0("ADAM\n");
	tuner.method=0;
	tuner.la1=0.9;
	tuner.la2=0.99;
	printf("ADAM %d %f %f %f\n", tuner.batch_len, tuner.la1, tuner.la2, tuner.adam_step);
	texel_test_loop(&tuner, "../texel/pers_test_adam_");
// store best result into slot 3	
	backup_matrix_values(tuner.m, tuner.matrix_var_backup+tuner.pcount*3, tuner.pcount);

#endif
#if 0

// jac: copy values from slot 16 to slot 0. Slot 0 is temporary, after tuner run slot 15 contains best values
	copy_vars_jac(16,0,tuner.ivar, tuner.nvar, tuner.pcount);
	copy_vars_jac(16,15,tuner.ivar, tuner.nvar, tuner.pcount);

// adadelta JAC based
	LOGGER_0("ADADelta JAC\n");
	tuner.method=1;
	tuner.la1=0.8;
	tuner.la2=0.9;
	printf("ADADelta JAC %d %f %f %f\n", tuner.batch_len, tuner.la1, tuner.la2, tuner.adadelta_step);
	texel_test_loop_jac(&tuner, "../texel/ptest_adeltaJ_");
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
//	tuner.rms_step=0.1;
	printf("RMSprop JAC %d %f %f %f\n", tuner.batch_len, tuner.la1, tuner.la2, tuner.rms_step);
	texel_test_loop_jac(&tuner, "../texel/ptest_rmsJ_");

// store best result into slot 1
	copy_vars_jac(15,1,tuner.ivar, tuner.nvar, tuner.pcount);
//	for(int ii=0;ii<tuner.pcount;ii++) {
//	  printf("NVAR %d: %lf\n", ii, tuner.nvar[ii]);
//	}

#endif
#if 1
	copy_vars_jac(16,0,tuner.ivar, tuner.nvar, tuner.pcount);
	copy_vars_jac(16,15,tuner.ivar, tuner.nvar, tuner.pcount);
// adam JAC
//	LOGGER_0("ADAM JAC N\n");
	tuner.method=0;
	tuner.la1=0.9;
	tuner.la2=0.99;
	printf("ADAM JAC %d %f %f %f\n", tuner.batch_len, tuner.la1, tuner.la2, tuner.adam_step);
	texel_test_loop_jac(&tuner, "../texel/ptest_adamJ_");
// store best result into slot 3
	copy_vars_jac(15,3,tuner.ivar, tuner.nvar, tuner.pcount);

#endif

/*
 * Verifications?
 */

// verification run

// Various loss computations

// init faze, loss based on evaluation
//	LOGGER_0("INIT verification OLD\n");
//	printf("INIT verification OLD\n");
	restore_matrix_values(tuner.matrix_var_backup+tuner.pcount*16, tuner.m, tuner.pcount);
//	tuner.penalty=calc_dir_penalty(tuner.m, &tuner, tuner.pcount);
//	dump_matrix_values2(tuner.m, tuner.pcount);
	tuner.penalty=0;
	fxb1=(compute_loss_dir(tun2.boards, tun2.results, tun2.phase, tuner.pi, tun2.len, 0));
//	fxb1=(fxb1+tuner.penalty)/tun2.len;
	fxb1/=tun2.len;
	LOGGER_0("INIT verification OLD, loss %f %d\n", fxb1, tun2.len);
	printf("INIT verification OLD, loss %f %d\n", fxb1, tun2.len);
//	LOGGER_0("REG_LA %.20f\n", tuner.reg_la);
//	printf("REG_LA %.20f\n", tuner.reg_la);

#if 0
// compute jacobian for verification positions
	free_jac(tuner.jac);
//	restore_matrix_values(tuner.matrix_var_backup+tuner.pcount*16, tuner.m, tuner.pcount);
// tranfer values from slot 16 to JAC based tuner - slot 0
	matrix_to_jac(16, tuner.m, tuner.ivar, tuner.pcount );
	matrix_to_jac(16, tuner.m, tuner.nvar, tuner.pcount );
	tuner.jac=NULL;
	tuner.jac=allocate_jac(tun2.len, tuner.pcount);
	if(tuner.jac!=NULL) {
		LOGGER_0("JACOBIAN population, positions %d, parameters %d\n", tun2.len, tuner.pcount);
		printf("JACOBIAN population, positions %d, parameters %d\n", tun2.len, tuner.pcount);
//		populate_jac(tuner.jac,tun2.boards, tun2.results, tun2.phase, tuner.pi, 0, tun2.len-1, tuner.m, tuner.pcount);
		populate_jac_pl(tuner.jac,tun2.boards, tun2.results, tun2.phase, tuner.pi, 0, tun2.len-1, tuner.m, tuner.pcount, 4);
		LOGGER_0("JACOBIAN populated\n");
		printf("JACOBIAN populated\n");
	} else {
		LOGGER_0("JACOBIAN aborted\n");
		printf("JACOBIAN aborted\n");
		abort();
	}

#endif
#if 0
// compute INIT loss JAC based, values from jac tuner
	copy_vars_jac(16,0,tuner.ivar, tuner.nvar, tuner.pcount);
	fxbj=(compute_loss_jac_dir(tun2.results, tun2.len, 0,tuner.jac, tuner.ivar, tuner.nvar, tun2.pcount))/tun2.len;
//	fxbj=0;
	LOGGER_0("INIT verification JAC, loss %f\n", fxbj);
	printf("INIT verfication JAC, loss %f\n", fxbj);
#endif
#if 0

// rmsprop OLD best values verification, via OLD loss
	LOGGER_0("RMSprop OLD verification\n");
	restore_matrix_values(tuner.matrix_var_backup+tuner.pcount*1, tuner.m, tuner.pcount);
//	dump_matrix_values2(tuner.m, tuner.pcount);
	fxb1=(compute_loss_dir(tun2.boards, tun2.results, tun2.phase, tuner.pi, tun2.len, 0))/tun2.len;
	LOGGER_0("RMSprop OLD tuner, OLD loss %f\n", fxb1);
	printf("RMSprop OLD tuner, OLD loss %f\n", fxb1);

#endif
#if 1

// rmsprop JAC verification via OLD loss
	LOGGER_0("RMSprop JAC verification\n");
// jac slot 1 to evaluate parameters
	jac_to_matrix(1, tuner.m, tuner.nvar, tuner.pcount);
//	dump_matrix_values2(tuner.m, tuner.pcount);
	fxb1=(compute_loss_dir(tun2.boards, tun2.results, tun2.phase, tuner.pi, tun2.len, 0))/tun2.len;
	LOGGER_0("%d: RMS JAC tuner, OLD loss %f, %e\n", ll, fxb1, tuner.reg_la);
	printf("%d: RMS JAC tuner, OLD loss %f, %e\n", ll, fxb1, tuner.reg_la);

#endif
#if 0

// adadelta OLD verification , OLD loss calculation
	LOGGER_0("ADADelta OLD verification\n");
	restore_matrix_values(tuner.matrix_var_backup+tuner.pcount*2, tuner.m, tuner.pcount);
	fxb2=(compute_loss_dir(tuner.boards, tuner.results, tuner.phase, tuner.pi, tuner.len, 0))/tuner.len;
	LOGGER_0("ADADelta OLD tuner, OLD loss %f\n", fxb2);
	printf("ADADelta OLD tuner, OLD loss %f\n", fxb2);
#endif
#if 0

// adadelta JAC verification , OLD loss calculation
//	LOGGER_0("ADADelta JAC verification\n");
//	restore_matrix_values(tuner.matrix_var_backup+tuner.pcount*16, tuner.m, tuner.pcount);
	jac_to_matrix(2, tuner.m, tuner.nvar, tuner.pcount);
//	dump_matrix_values2(tuner.m, tuner.pcount);
	fxb1=(compute_loss_dir(tun2.boards, tun2.results, tun2.phase, tuner.pi, tun2.len, 0))/tun2.len;
	LOGGER_0("ADADelta JAC tuner, OLD loss %f\n", fxb1);
	printf("ADADelta JAC tuner, OLD loss %f\n", fxb1);

#endif
#if 0

// adam OLD , normal loss calculation
	LOGGER_0("ADAM OLD verification\n");
	restore_matrix_values(tuner.matrix_var_backup+tuner.pcount*3, tuner.m, tuner.pcount);
	tuner.penalty=0;
	fxb3=(compute_loss_dir(tun2.boards, tun2.results, tun2.phase, tuner.pi, tun2.len, 0))/tun2.len;
	LOGGER_0("ADAM OLD tuner, OLD loss %f\n", fxb3);
	printf("ADAM OLD tuner, OLD loss %f\n", fxb3);

#endif
#if 1

	LOGGER_0("ADAM JAC verification\n");
//	restore_matrix_values(tuner.matrix_var_backup+tuner.pcount*16, tuner.m, tuner.pcount);
//	copy_vars_jac(15,3,tuner.ivar, tuner.nvar, tuner.pcount);
	jac_to_matrix(3, tuner.m, tuner.nvar, tuner.pcount);

	fxb1=(compute_loss_dir(tun2.boards, tun2.results, tun2.phase, tuner.pi, tun2.len, 0))/tun2.len;
	LOGGER_0("%d: ADAM JAC tuner, OLD loss %f, %e, %e\n", ll, fxb1, tuner.reg_la, tuner.adam_step);
	printf("%d: ADAM JAC tuner, OLD loss %f, %e, %e\n", ll, fxb1, tuner.reg_la, tuner.adam_step);
//	write_personality(tuner.pi, "xx.xml");

#endif
#if 0

// rmsprop, loss jac, values from jac tuner, from slot 1 to slot 0
	copy_vars_jac(1,0,tuner.ivar, tuner.nvar, tuner.pcount);

//	jac_to_matrix(1, tuner.m, tuner.nvar, tuner.pcount);
//	matrix_to_jac(0, tuner.m, tuner.nvar, tuner.pcount);

	fxbj=(compute_loss_jac_dir(tun2.results, tun2.len, 0,tuner.jac, tuner.ivar, tuner.nvar, tuner.pcount))/tun2.len;
	LOGGER_0("RMSprop JAC tuner, JAC loss %f\n", fxbj);
	printf("RMSprop JAC tuner, JAC loss %f\n", fxbj);

#endif
#if 0

	restore_matrix_values(tuner.matrix_var_backup+tuner.pcount*1, tuner.m, tuner.pcount);
	matrix_to_jac(0, tuner.m, tuner.nvar, tuner.pcount);
	fxbj=(compute_loss_jac_dir(tun2.results, tun2.len, 0,tuner.jac, tuner.ivar, tuner.nvar, tuner.pcount))/tun2.len;
	LOGGER_0("RMSprop OLD tuner, JAC loss %f\n", fxbj);
	printf("RMSprop OLD tuner, JAC loss %f\n", fxbj);
#endif
#if 0

// adadelta, loss jac, values from jac tuner, from slot 2 to slot 0
	copy_vars_jac(2,0,tuner.ivar, tuner.nvar, tuner.pcount);
//	jac_to_matrix(2, tuner.m, tuner.nvar, tuner.pcount);
//	matrix_to_jac(0, tuner.m, tuner.nvar, tuner.pcount);

	fxb1=(compute_loss_jac_dir(tun2.results, tun2.len, 0,tuner.jac, tuner.ivar, tuner.nvar, tuner.pcount))/tun2.len;
	LOGGER_0("ADADelta JAC tuner,  JAC loss %f\n", fxb1);
	printf("ADADelta JAC tuner, JAC loss %f\n", fxb1);

#endif
#if 0

	restore_matrix_values(tuner.matrix_var_backup+tuner.pcount*2, tuner.m, tuner.pcount);
	matrix_to_jac(0, tuner.m, tuner.nvar, tuner.pcount);
	fxbj=(compute_loss_jac_dir(tun2.results, tun2.len, 0,tuner.jac, tuner.ivar, tuner.nvar, tuner.pcount))/tun2.len;
	LOGGER_0("Adadelta OLD tuner, JAC loss %f\n", fxbj);
	printf("Adadelta OLD tuner, JAC loss %f\n", fxbj);


// adam, loss jac, values from jac tuner, from slot 3 to slot 0
//	copy_vars_jac(3,0,tuner.ivar, tuner.nvar, tuner.pcount);
	restore_matrix_values(tuner.matrix_var_backup+tuner.pcount*3, tuner.m, tuner.pcount);
	matrix_to_jac(0, tuner.m, tuner.nvar, tuner.pcount);
	fxb1=(compute_loss_jac_dir(tun2.results, tun2.len, 0,tuner.jac, tuner.ivar, tuner.nvar, tuner.pcount))/tun2.len;
	LOGGER_0("ADAM OLD tuner, JAC loss %f\n", fxb1);
	printf("ADAM OLD  tuner, JAC loss %f\n", fxb1);

#endif
#if 0
//	jac_to_matrix(3, tuner.m, tuner.nvar, tuner.pcount);
//	matrix_to_jac(0, tuner.m, tuner.nvar, tuner.pcount);

	copy_vars_jac(3,0,tuner.ivar, tuner.nvar, tuner.pcount);
	fxb1=(compute_loss_jac_dir(tun2.results, tun2.len, 0,tuner.jac, tuner.ivar, tuner.nvar, tuner.pcount))/tun2.len;
	printf("%d:ADAM JAC tuner, JAC loss %f, %f, %f\n", ll, fxb1, tuner.reg_la, tuner.adam_step);
	LOGGER_0("%d:ADAM JAC tuner, JAC loss %f, %f, %f\n", ll, fxb1, tuner.reg_la, tuner.adam_step);
	
#endif
//		tuner.batch_len/=2;
//		tuner.adam_step/=10;
//		tuner.reg_la/=10;
	}
	free_jac(tuner.jac);
	texel_test_fin(&tun2);
	texel_test_fin(&tuner);
}

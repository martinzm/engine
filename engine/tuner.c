#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <tgmath.h>
#include <fenv.h>
#include <errno.h>
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
#include <omp.h>

int
variables_reinit_material (void *data)
{
  tuner_variables_pass *v;

  v = (tuner_variables_pass*) data;
  if (v->stage == 0)
	{
	  meval_table_gen (v->p->mat, v->p, 0);
	}
  else
	{
	  meval_table_gen (v->p->mate_e, v->p, 1);
	}
  return 0;
}

int
variables_restore_material (void *data)
{
  tuner_variables_pass *v;

  v = (tuner_variables_pass*) data;
  if (v->stage == 0)
	{
	  meval_table_gen (v->p->mat, v->p, 0);
	}
  else
	{
	  meval_table_gen (v->p->mate_e, v->p, 1);
	}
  return 0;
}

double
enforce_positive (double val)
{
  if (val > 0)
	return val;
  return 0;
}

double
enforce_negative (double val)
{
  if (val < 0)
	return val;
  return 0;
}

double
material_norm (double val)
{
  if (val < -40000)
	return -40000;
  if (val>40000)
	return 40000;
  return val;
}

int
free_matrix (matrix_type *m, int count)
{
  int i;
  if (m != NULL)
	{
	  for (i = 0; i < count; i++)
		{
		  if (m[i].init_data != NULL)
			free (m[i].init_data);
		}
	  if (m != NULL)
		free (m);
	}
  return 0;
}

int ff_dummy(void *data){
return 0;
}

#define MAT_SIN(MM, FF) \
    MM.init_f= &ff_dummy;\
    MM.restore_f= &ff_dummy;\
    MM.init_data= NULL;\
    MM.upd=0;\
    MM.u[0]=&FF;\
    MM.mid=0;\
    MM.ran=10000;\
    MM.max=MM.ran/2+MM.mid;\
    MM.min=MM.mid-MM.ran/2;\
    MM.norm_f= NULL;\
    MM.value_type=-1; \
    MM.counterpart=NULL;\
    MM.tunable=1;

#define MAT_DUO(MM1, MM2, FF1, FF2, I) \
    MM1.init_f= &ff_dummy;\
    MM1.restore_f= &ff_dummy;\
    MM1.init_data= NULL;\
    MM1.upd=0;\
    MM1.u[0]=&FF1;\
    MM1.mid=0;\
    MM1.ran=10000;\
    MM1.max=MM1.ran/2+MM1.mid;\
    MM1.min=MM1.mid-MM1.ran/2;\
    MM1.norm_f= NULL;\
    MM1.tunable=1;\
    MM2.init_f= &ff_dummy;\
    MM2.restore_f= &ff_dummy;\
    MM2.init_data= NULL;\
    MM2.upd=0;\
    MM2.u[0]=&FF2;\
    MM2.mid=0;\
    MM2.ran=10000;\
    MM2.max=MM2.ran/2+MM2.mid;\
    MM2.min=MM2.mid-MM2.ran/2;\
    MM2.norm_f=NULL;\
    MM2.tunable=1;\
    MM1.value_type=0; \
    MM2.value_type=1; \
    MM1.counterpart=I+1;\
    MM2.counterpart=I;


#define MAT_DUO_ADD(MM1, MM2, FF1, FF2) \
    MM1.upd++;\
    MM1.u[MM1.upd]=&FF1;\
    MM2.upd++;\
    MM2.u[MM2.upd]=&FF2;


int print_matrix(matrix_type *m, int pcount){
int i;
	for(i=0;i<pcount;i++) {
		LOGGER_0("Matrix i: %d init %d, value_type %d\n", i, m[i].u[0], m[i].value_type);
	}
return 0;
}

int
to_matrix (matrix_type **m, personality *p)
{
  int i, max, gs, sd, pi, sq, oi, ii, rank, file;
  int len = 16384;
  matrix_type *mat;
  tuner_variables_pass *v1, *v2;

  max = len;
  mat = malloc (sizeof(matrix_type) * len);
  *m = mat;
  i = 0;
#if 1
  // eval_BIAS
  MAT_DUO(mat[i],mat[i+1], p->eval_BIAS, p->eval_BIAS_e, i);
  i++;
#endif
#if 0
  // type gamestage
  // pawn isolated -
  MAT_DUO(mat[i], mat[i+1], p->isolated_penalty[0], p->isolated_penalty[1], i);
  i+=2;
#endif
#if 0
  // pawn backward +
  MAT_DUO(mat[i], mat[i+1], p->backward_penalty[0], p->backward_penalty[1], i);
  i+=2;
#endif
#if 0
  // pawn on ah +
  MAT_DUO(mat[i], mat[i+1], p->pawn_ah_penalty[0], p->pawn_ah_penalty[1], i);
  i+=2;
#endif
#if 1
  // type of passer with 6 values +
  // passer bonus
  for(sq=0;sq<=5;sq++) {
      MAT_DUO(mat[i], mat[i+1], p->passer_bonus[0][WHITE][sq], p->passer_bonus[1][WHITE][sq], i);
      MAT_DUO_ADD(mat[i], mat[i+1], p->passer_bonus[0][BLACK][sq], p->passer_bonus[1][BLACK][sq]);
      i+=2;
  }
#endif
#if 0
  // pawn blocked penalty +
  for(sq=0;sq<=4;sq++) {
      MAT_DUO(mat[i], mat[i+1], p->pawn_blocked_penalty[0][WHITE][sq], p->pawn_blocked_penalty[1][WHITE][sq], i);
      MAT_DUO_ADD(mat[i], mat[i+1], p->pawn_blocked_penalty[0][BLACK][sq], p->pawn_blocked_penalty[1][BLACK][sq]);
      i+=2;
  }
#endif
#if 0
  // pawn stopped penalty +
  for(sq=0;sq<=4;sq++) {
      MAT_DUO(mat[i], mat[i+1], p->pawn_stopped_penalty[0][WHITE][sq], p->pawn_stopped_penalty[1][WHITE][sq], i);
      MAT_DUO_ADD(mat[i], mat[i+1], p->pawn_stopped_penalty[0][BLACK][sq], p->pawn_stopped_penalty[1][BLACK][sq]);
      i+=2;
  }
#endif
#if 0
  // pawn weak open file
  MAT_DUO(mat[i], mat[i+1], p->pawn_weak_onopen_penalty[0], p->pawn_weak_onopen_penalty[1], i);
  i+=2;
#endif
#if 0
  // pawn weak center file -
  MAT_DUO(mat[i], mat[i+1], p->pawn_weak_center_penalty[0], p->pawn_weak_center_penalty[1], i);
  i+=2;
#endif

#if 0
  //piece to square - single piece
  for(gs=0;gs<=1;gs++) {
      for(pi=0;pi<=0;pi++) {
	  for(sq=0;sq<=63;sq++){
	      mat[i].init_f=mat[i].restore_f=	mat[i].init_data=NULL;
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
      }
  }
#endif

#if 1
// PST
  int pieces_in[]= { 0, 1, 2, 3, 4, 5, -1  };
//  int pieces_in[]= { 2, -1  };
  ii=0;
  while(pieces_in[ii]!=-1) {
      pi=pieces_in[ii];
      for(sq=0;sq<=63;sq++){
	  MAT_DUO(mat[i], mat[i+1], p->piecetosquare[0][WHITE][pi][sq], p->piecetosquare[1][WHITE][pi][sq], i);
	  MAT_DUO_ADD(mat[i], mat[i+1], p->piecetosquare[0][BLACK][pi][Square_Swap[sq]], p->piecetosquare[1][BLACK][pi][Square_Swap[sq]]);
	  i+=2;
      }
      ii++;
  }
#endif

#if 0
  // rook on 7th +
  MAT_DUO(mat[i], mat[i+1], p->rook_on_seventh[0], p->rook_on_seventh[1], i);
  i+=2;
#endif
#if 0
  // rook on open +
  MAT_DUO(mat[i], mat[i+1], p->rook_on_open[0], p->rook_on_open[1], i);
  i+=2;
#endif
#if 0
  // rook on semiopen +
  MAT_DUO(mat[i], mat[i+1], p->rook_on_semiopen[0], p->rook_on_semiopen[1], i);
  i+=2;
#endif
#if 1
  // mobility
  int mob_lengths[]= { 2, 9, 14, 15, 28, 9, -1  };
  for(pi=0;pi<=5;pi++) {
      for(sq=0;sq<mob_lengths[pi];sq++){
	  MAT_DUO(mat[i], mat[i+1], p->mob_val[0][WHITE][pi][sq], p->mob_val[1][WHITE][pi][sq], i);
	  MAT_DUO_ADD(mat[i], mat[i+1], p->mob_val[0][BLACK][pi][sq], p->mob_val[1][BLACK][pi][sq]);
	  i+=2;
      }
  }

#endif

#if 0
  // pawn_iso center +
  MAT_DUO(mat[i], mat[i+1], p->pawn_iso_center_penalty[0], p->pawn_iso_center_penalty[1], i);
  i+=2;
#endif
#if 0
  // +
  MAT_DUO(mat[i], mat[i+1], p->pawn_iso_onopen_penalty[0], p->pawn_iso_onopen_penalty[1], i);
  i+=2;
#endif
#if 0
  //
  MAT_DUO(mat[i], mat[i+1], p->pshelter_open_penalty[0], p->pshelter_open_penalty[1], i);
  i+=2;
#endif
#if 0
  MAT_DUO(mat[i], mat[i+1], p->pshelter_isol_penalty[0], p->pshelter_isol_penalty[1], i);
  i+=2;
#endif
#if 0
  MAT_DUO(mat[i], mat[i+1], p->pshelter_hopen_penalty[0], p->pshelter_hopen_penalty[1], i);
  i+=2;
#endif
#if 0
  MAT_DUO(mat[i], mat[i+1], p->pshelter_double_penalty[0], p->pshelter_double_penalty[1], i);
  i+=2;
#endif
#if 1
  MAT_DUO(mat[i], mat[i+1], p->pshelter_prim_bonus[0], p->pshelter_prim_bonus[1], i);
  i+=2;
#endif
#if 1
  MAT_DUO(mat[i], mat[i+1], p->pshelter_sec_bonus[0], p->pshelter_sec_bonus[1], i);
  i+=2;
#endif
#if 1
  // out of shelter
  MAT_DUO(mat[i], mat[i+1], p->pshelter_out_penalty[0], p->pshelter_out_penalty[1], i);
  i+=2;
#endif
#if 0
  // pawn n protect -
  for(sq=0;sq<=7;sq++) {
      MAT_DUO(mat[i], mat[i+1], p->pawn_n_protect[0][WHITE][sq], p->pawn_n_protect[1][WHITE][sq], i);
      MAT_DUO_ADD(mat[i], mat[i+1], p->pawn_n_protect[0][BLACK][sq], p->pawn_n_protect[1][BLACK][sq]);
      i+=2;
  }
#endif
#if 0
  // pawn pot protect -
  for(sq=0;sq<=7;sq++) {
      MAT_DUO(mat[i], mat[i+1], p->pawn_pot_protect[0][WHITE][sq], p->pawn_pot_protect[1][WHITE][sq], i);
      MAT_DUO_ADD(mat[i], mat[i+1], p->pawn_pot_protect[0][BLACK][sq], p->pawn_pot_protect[1][BLACK][sq]);
      i+=2;
  }
#endif
#if 0
  // pawn dir protect +
  for(sq=0;sq<=7;sq++) {
      MAT_DUO(mat[i], mat[i+1], p->pawn_dir_protect[0][WHITE][sq], p->pawn_dir_protect[1][WHITE][sq], i);
      MAT_DUO_ADD(mat[i], mat[i+1], p->pawn_dir_protect[0][BLACK][sq], p->pawn_dir_protect[1][BLACK][sq]);
      i+=2;
  }
#endif
#if 0
  // pawn doubled n penalty +
  for(sq=0;sq<=7;sq++) {
      MAT_DUO(mat[i], mat[i+1], p->doubled_n_penalty[0][WHITE][sq], p->doubled_n_penalty[1][WHITE][sq], i);
      MAT_DUO_ADD(mat[i], mat[i+1], p->doubled_n_penalty[0][BLACK][sq], p->doubled_n_penalty[1][BLACK][sq]);
      i+=2;
  }
#endif
#if 1
  // mobUnSec -
  int mob_lengths2[]= { 2, 9, 14, 15, 28, 9, -1  };
  for(pi=0;pi<=5;pi++) {
      for(sq=0;sq<mob_lengths2[pi];sq++){
	  MAT_DUO(mat[i], mat[i+1], p->mob_uns[0][WHITE][pi][sq], p->mob_uns[1][WHITE][pi][sq], i);
	  MAT_DUO_ADD(mat[i], mat[i+1], p->mob_uns[0][BLACK][pi][sq], p->mob_uns[1][BLACK][pi][sq]);
	  i+=2;
      }
  }

#endif

#if 0
  // bishopboth +
  MAT_DUO(mat[i], mat[i+1], p->bishopboth[0], p->bishopboth[1], i);
  i+=2;
#endif
#if 0
  MAT_DUO(mat[i], mat[i+1], p->rookpair[0], p->rookpair[1], i);
  i+=2;
#endif
#if 0
  MAT_DUO(mat[i], mat[i+1], p->knightpair[0], p->knightpair[1], i);
  i+=2;
#endif


  // for these we need callback function
#if 0
  for(gs=0;gs<=1;gs++) {
      mat[i].init_f=variables_reinit_material;
      mat[i].restore_f=variables_restore_material;
      v1=malloc(sizeof(tuner_variables_pass));
      v1->p=p;
      v1->stage=gs;
      mat[i].init_data=v1;
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

#if 1
  int start_in[] =
	{ 0, 1, 2, 3, 4, -1 };
//  int end_in[] =
//	{ 0, 1, 2, 3, 4, -1 };

  ii = 0;
  while (start_in[ii] != -1)
	{
	  sq = start_in[ii];
	  mat[i].init_f=&ff_dummy;
	  mat[i].restore_f=&ff_dummy;
	  mat[i].init_data=NULL;
	  mat[i].upd = 0;
	  mat[i].u[0] = &p->Values[0][sq];
	  mat[i].mid = 10000;
	  mat[i].ran = 20000;
	  mat[i].max = mat[i].ran / 2 + mat[i].mid;
	  mat[i].min = mat[i].mid - mat[i].ran / 2;
	  mat[i].norm_f = material_norm;
	  mat[i].value_type = 0;
	  mat[i].counterpart = i + 1;
	  mat[i].tunable = 1;
//	  mat[i].tunable = (sq != 0) ? 1 : 0; // make PAWN material for beginnng/mid game phase NOT tunable

	  mat[i + 1].init_f=&ff_dummy;
	  mat[i + 1].restore_f=&ff_dummy;
	  mat[i + 1].init_data=NULL;
	  mat[i + 1].upd = 0;
	  mat[i + 1].u[0] = &p->Values[1][sq];
	  mat[i + 1].mid = 10000;
	  mat[i + 1].ran = 20000;
	  mat[i + 1].max = mat[i + 1].ran / 2 + mat[i + 1].mid;
	  mat[i + 1].min = mat[i + 1].mid - mat[i + 1].ran / 2;
	  mat[i + 1].norm_f = material_norm;
	  mat[i + 1].value_type = 1;
	  mat[i + 1].counterpart = i;
	  mat[i + 1].tunable = 1;
	  i += 2;
	  ii++;
	}
#endif

#if 1
  int start_in2[] =
	{ 1, 2, 3, 4, -1 };
  ii = 0;
  while (start_in2[ii] != -1)
	{
		sq = start_in2[ii];
		int x;
		for(x=0;x<(PAWNS_TOT+1);x++) {
		  mat[i].init_f=&ff_dummy;
		  mat[i].restore_f=&ff_dummy;
		  mat[i].init_data=NULL;
		  mat[i].upd = 0;
		  mat[i].u[0] = &p->dvalues[sq][x];
//	  LOGGER_0("%d %d = %d\n", sq,x,p->dvalues[sq][x]);
		  mat[i].mid = 1000;
		  mat[i].ran = 2000;
		  mat[i].max = mat[i].ran / 2 + mat[i].mid;
		  mat[i].min = mat[i].mid - mat[i].ran / 2;
		  mat[i].norm_f = NULL;
		  mat[i].value_type = 2;
		  mat[i].counterpart = 0;
		  mat[i].tunable = 1;
		  i++;
	  }
	  ii++;
	}

#endif

  return i;
}

void koefs_to_matrix(double *koef, matrix_type *m, int pcount)
{
  int f, ii;
  for(f=0;f<pcount;f++) 
	  for (ii = 0; ii <= m[f].upd; ii++) *(m[f].u[ii]) = koef[f];
}

void matrix_to_koefs(double *koef, matrix_type *m, int pcount)
{
  int f, ii;
  for(f=0;f<pcount;f++)
	koef[f] = *(m[f].u[0]);
}

int
compute_neval_dir (board *b, attack_model *a, personality *p)
{
int8_t vi;
  struct _ui_opt uci_options;
  struct _statistics s;
  int ev;
  b->stats = &s;
  b->uci_options = &uci_options;
  // eval - white pov
  eval_king_checks_all (b, a);
  vi=b->mindex_validity;
  b->mindex_validity=0;
  ev = eval(b, a, p);
  b->mindex_validity=vi;
  return ev;
}

int
init_ntuner_jac (ntuner_run *state, matrix_type *m, int pcount)
{
  int i;
  for (i = 0; i < pcount; i++)
	state[i].or1 = 0.00000001;
  for (i = 0; i < pcount; i++)
	state[i].or2 = 0.00000001;
  for (i = 0; i < pcount; i++)
	state[i].grad = 0;
  return 0;
}

int
allocate_ntuner (ntuner_run **tr, int pcount)
{
  *tr = malloc (sizeof(ntuner_run) * (pcount));
  return 0;
}

int free_ntuner(ntuner_run *tr)
{
	free(tr);
	return 1;
}

int
allocate_njac (long records, int params, njac **state)
{

  LOGGER_0("sizeof %lld, %lld, %lld\n", sizeof(feat) * params * records, params, records);
	printf("sizeof %lld", sizeof(njac) * records);
	*state = (njac*) malloc (sizeof(njac) * records);
	if (*state != NULL) return 1;
  return 0;
}

int
free_njac (njac *state)
{
  if (state != NULL)
	{
	  free (state);
	}
  return 0;
}

int
dump_grads (ntuner_run *state, int pcount)
{
  int i;
  NLOGGER_0("\n");
  LOGGER_0("OR1\t");
  for (i = 0; i < pcount; i++)
	NLOGGER_0("%f\t", state[i].or1);
  NLOGGER_0("\n");
  LOGGER_0("OR2\t");
  for (i = 0; i < pcount; i++)
	NLOGGER_0("%f\t", state[i].or2);
  NLOGGER_0("\n");
  LOGGER_0("GR\t");
  for (i = 0; i < pcount; i++)
	NLOGGER_0("%f\t", state[i].grad);
  NLOGGER_0("\n");

return 0;
}

int
texel_load_files2 (tuner_global *tuner, char *sts_tests[])
{
  int8_t r;
  FILE *handle;
  char filename[256];
  char buffer[512];
  char res[128];
  char fen[512];
  char *name;

  int x;
  char *xx;
  long l, i;
  l = i = 0;
  tuner->len = 0;

  strcpy (filename, sts_tests[0]);
  if ((handle = fopen (filename, "r")) == NULL)
	{
	  printf ("File %s is missing\n", filename);
	  return -1;
	}
  while (!feof (handle) && (tuner->len < tuner->max_records))
	{
	  xx = fgets (buffer, 511, handle);
	  if ((i % (tuner->nth)) == tuner->records_offset)
		{
		  if (parseEPD (buffer, fen, NULL, NULL, NULL, NULL, res, NULL, &name)
			  > 0)
			{
			  if (!strcmp (res, "1-0"))
				r = 2;
			  else if (!strcmp (res, "0-1"))
				r = 0;
			  else if (!strcmp (res, "1/2-1/2"))
				r = 1;
			  else
				{
				  printf ("Result parse error:%s\n", filename);
				  abort ();
				}
			  setup_FEN_board (&tuner->boards[tuner->len], fen);
			  tuner->phase[tuner->len] = eval_phase (&tuner->boards[tuner->len],
													 tuner->pi);
			  tuner->results[tuner->len] = r;
			  tuner->boards[tuner->len].hps = NULL;
			  tuner->len++;
			  free (name);
			}
		}
	  i++;
	}
  fclose (handle);
  printf ("L2: Imported %ld from total %ld of records\n", tuner->len, i);
  return 1;
}

int
texel_load_files (tuner_global *tuner, char *sts_tests[])
{
  // results from white pov
  int8_t tests_setup[] =
	{ 2, 1, 0, -1 };
  FILE *handle;
  char filename[256];
  char buffer[512];
  char fen[512];
  char *name;

  int x;
  int sc_b, sc_e;
  char *xx;
  long l, i;
  l = i = 0;

  tuner->len = 0;

  while ((tests_setup[l] != -1) && (tuner->len < tuner->max_records))
	{
	  strcpy (filename, sts_tests[l]);
	  if ((handle = fopen (filename, "r")) == NULL)
		{
		  printf ("File %s is missing\n", filename);
		  return -1;
		}
	  while (!feof (handle) && (tuner->len < tuner->max_records))
		{
		  xx = fgets (buffer, 511, handle);
		  if ((i % (tuner->nth)) == tuner->records_offset)
			{
			  if (parseEPD (buffer, fen, NULL, NULL, NULL, NULL, NULL, NULL,
							&name) > 0)
				{
				  (tuner->boards[tuner->len]).hps = NULL;
				  setup_FEN_board (&tuner->boards[tuner->len], fen);
				  tuner->phase[tuner->len] = eval_phase (
					  &tuner->boards[tuner->len], tuner->pi);
				  tuner->results[tuner->len] = tests_setup[l];
				  tuner->len++;
				  free (name);
				}
			}
		  i++;
		}
	  fclose (handle);
	  l++;
	}
  printf ("L1: Imported %ld from total %ld of records\n", tuner->len, i);
  return 1;
}

#if 1
// callback funkce
#define CBACK long (*get_next)(char *fen, int8_t *res, void *data)

double
comp_cost_vkx (double evaluation, double entry_result, double K)
{
  double sigmoid;
  sigmoid = (2.0 / (1 + exp((-K)*evaluation)));
  return pow((entry_result - sigmoid) / 2, 2);
}

int
populate_njac (board *b, njac *nj, personality *p, matrix_type *m,
			   int pcount)
{
// potential problem
feat FF[10000];

  double diff_step;
  double fxdiff, fxh, fxh1, fxh2, fxb, tmp;
  int sce1, sce2, scb1, scb2;
  int scb1_w, scb1_b, sce1_w, sce1_b;
  int scb2_w, scb2_b, sce2_w, sce2_b;
  //!!!!
  int i, ii, count;
  int o, on;
  attack_model a;

  for (i = 0; i < pcount; i++)
	{
	// do not get feature values when it is END value - we get features from start/middle value counterpart
	if(m[i].value_type==1) continue;

	  // loop over parameters
	  o = *(m[i].u[0]);
	  diff_step = 100.0;

	  // update parameter in positive way
	  on = o + diff_step;
	  for (ii = 0; ii <= m[i].upd; ii++)
		{
		  *(m[i].u[ii]) = on;
		}
	  if (m[i].init_f != NULL)
		m[i].init_f (m[i].init_data);

	  // compute eval
	  fxh1 = (double) compute_neval_dir (b, &a, p);
	  sce1 = a.sc.score_e;
	  scb1 = a.sc.score_b;
	  scb1_w = a.sc.score_b_w;
	  scb1_b = a.sc.score_b_b;
	  sce1_w = a.sc.score_e_w;
	  sce1_b = a.sc.score_e_b;

	  // update parameter in negative way
	  on = o - diff_step;
	  for (ii = 0; ii <= m[i].upd; ii++)
		{
		  *(m[i].u[ii]) = on;
		}
	  if (m[i].init_f != NULL)
		m[i].init_f (m[i].init_data);

	  // compute eval
	  // compute change and partial derivative of the parameter
	  fxh2 = (double) compute_neval_dir (b, &a, p);
	  sce2 = a.sc.score_e;
	  scb2 = a.sc.score_b;
	  scb2_w = a.sc.score_b_w;
	  scb2_b = a.sc.score_b_b;
	  sce2_w = a.sc.score_e_w;
	  sce2_b = a.sc.score_e_b;

	  //restore original values
	  for (ii = 0; ii <= m[i].upd; ii++)
		{
		  *(m[i].u[ii]) = o;
		}
	  if (m[i].init_f != NULL)
		m[i].init_f (m[i].init_data);

//		LOGGER_0("Feature %d Am wb:%f, bb:%f, we:%f, be:%f, counterp %d\n", i, (scb1_w-scb2_w)/(2*(double)diff_step), (scb1_b-scb2_b)/(2*(double)diff_step),(sce1_w-sce2_w)/(2*(double)diff_step), (sce1_b-sce2_b)/(2*(double)diff_step),m[i].counterpart);
	  // compute gradient/partial derivative
	  FF[i].idx=i;
	  switch (m[i].value_type)
		{
		case -1:
		case 0:
		  FF[i].f_b=(uint8_t) ceil((scb1_b-scb2_b)/(2*(double)diff_step));
		  FF[i].f_w=(uint8_t) ceil((scb1_w-scb2_w)/(2*(double)diff_step));
		  FF[m[i].counterpart].f_b=FF[i].f_b;
		  FF[m[i].counterpart].f_w=FF[i].f_w;
		  FF[m[i].counterpart].idx=m[i].counterpart;
		  break;
		case 1:
		  FF[i].f_b=(uint8_t) ceil((sce1_b-sce2_b)/(2*(double)diff_step));
		  FF[i].f_w=(uint8_t) ceil((sce1_w-sce2_w)/(2*(double)diff_step));
		  break;
		case 2:
		  FF[i].f_b=(uint8_t) ceil((scb1_b-scb2_b)/(2*(double)diff_step));
		  FF[i].f_w=(uint8_t) ceil((scb1_w-scb2_w)/(2*(double)diff_step));		
		  break;
		default:
		  break;
		}
//		LOGGER_0("ID:%d, FeatW:%d, FeatB:%d, type %d\n", i, FF[i].f_w, FF[i].f_b, m[i].value_type);
//		LOGGER_0("IDc:%d, FeatW:%d, FeatB:%d, type %d\n",  m[i].counterpart, FF[m[i].counterpart].f_w, FF[m[i].counterpart].f_b, m[m[i].counterpart].value_type);

	}

  count=0;
  for (i = 0; i < pcount; i++)
	{
	// count non zero features
		if(FF[i].f_w!=FF[i].f_b) count++;
	}
	nj->fcount=count;
	nj->ftp=(feat*) malloc(sizeof(feat)*count);

  count=0;
  for (i = 0; i < pcount; i++)
	{
		if(FF[i].f_w!=FF[i].f_b) {
			nj->ftp[count]=FF[i];
			count++;
		}
	}

//	LOGGER_0("Nonzero Features count %d out of %d\n", count,i);

  // compute classical evaluation
  fxh = (double) compute_neval_dir (b, &a, p);
// org phase
//  nj->phase = a.phase;
// store phase as float for initial/middle value and end values, end value includes scaling correction
//  a.sc.scaling=255;
  nj->phb = (a.phase * a.sc.scaling) / 255.0 / 255.0;
  nj->phe = ((255-a.phase) * a.sc.scaling) / 255.0 / 255.0;

//  LOGGER_0("Eval Phase %i,scaling %i, phb %f, phe %f\n", a.phase, a.sc.scaling, nj->phb, nj->phe);
  // recompute score
  fxh2 = 0;
  for (i = 0; i < nj->fcount; i++)
	{
	  ii=nj->ftp[i].idx;
	  switch (m[ii].value_type)
		{
		case -1:
		case 0:
		  fxh2 += *(m[ii].u[0]) * (nj->ftp[i].f_w-nj->ftp[i].f_b) * nj->phb;
		  break;
		case 1:
		  fxh2 += *(m[ii].u[0]) * (nj->ftp[i].f_w-nj->ftp[i].f_b) * nj->phe;
		  break;
		case 2:
		  fxh2 += *(m[ii].u[0]) * (nj->ftp[i].f_w-nj->ftp[i].f_b);
		  break;		
		default:
		  break;
		}
	}
  // score from evaluation not affected by tuning
  fxh2 = trunc(fxh2);
  nj->rem = fxh - fxh2;
  // score from eval
  nj->fx0 = fxh;
  nj->fxnew = fxh;
//  LOGGER_0("Position Score classic %f, computed %f, rem %f\n", fxh, fxh2, nj->rem); 
//  nj->matfix=a.sc.scaling;
  return 0;
}

/*
 * Linear eval function eval= Ax+By+C, A is koef and x is feature value
 * includes ridge regularization, lambda describes how much to regularize
 * sigmoid with e^ -. K is sigmoid koef.
 */

// parameter, feature value, result, eval value, lambda, K
// just the derivation of coefficient+sigmoid+difference squared, summing and (derivation of) regularization is handled in caller
double
njac_pderiv (double koef, int8_t fea, double res, double ev, double phase,
			 double K)
{
  int fe;
  double der, O, P;
//  feclearexcept(FE_ALL_EXCEPT);
  O = exp(-K * ev);
//  fe=fetestexcept(FE_INVALID | FE_DIVBYZERO | FE_OVERFLOW | FE_UNDERFLOW);
//  if(fe !=0) LOGGER_0("DER K %0.30f, ev %.30f, err %d, O %.30f\n", -K, ev, errno, O);
  der = -(4.0*K*fea*phase*O*(res-(2/(O+1)))) / pow((O+1), 2);
//  der = 2.0*lmb*koef -(4.0*K*fea*phase*O*(res-(2/(O+1)))) / pow((O+1), 2);
  
//  if(isnan(der)) LOGGER_0("KKK eval %f, res %f, der %f, K %f, fea %d, phase %f, O %.30f\n", ev, res, der, K, fea, phase, O);
  return der;
}

/*
 * computes evaluation of one position - *nj
 * based on actual values for coefficients in ko. ko has values of all coefficients being tuned. In order imposed by to_matrix function.
 * m contains description/info of all coefficients being tuned 
 * nj->rem - non mutable evaluation - value to add to position, but not affected by tuning
 * nj->fcount - number of non zero features in this position. 
 *   Feature is for example number of rooks if position has non zero num of rooks and material value of rook is being tuned
 * nj->ftp[ii].idx is order/index into array of coefficients ko, m
 * nj->phb, nj->phe are contributions to final score based on phase. phb is for beginning and bhe for end phase. Contains score scaling
 * nj->ftp[ii].f_w - feature for white
 * nj->ftp[ii].f_b - feature for black
 * m[i].value_type - indicates if this is beginning/middle game value=>0 or end game value=>1; -1 means value with no relation to phase
 */

double
njac_eval (double *ko, njac *nj, matrix_type *m)
{
  int i, ii;
  double eval, fxh2;
  i=-1;
  eval = nj->rem;
  for (ii = 0; ii < nj->fcount; ii++)
	{
	  i=nj->ftp[ii].idx;
//	LOGGER_0("NJAC EVAL i:%ld, Coef:%6.10f, fe[i]w:%d, fe[i]b:%d, nj->phb:%f, nj->phe:%f, vtype:%i\n", i, ko[i], nj->ftp[ii].f_w, nj->ftp[ii].f_b, nj->phb, nj->phe, m[i].value_type);
	  switch (m[i].value_type)
		{
		case -1:
		case 0:
		  eval += ko[i] * (nj->ftp[ii].f_w-nj->ftp[ii].f_b) * nj->phb;
		  break;
		case 1:
		  eval += ko[i] * (nj->ftp[ii].f_w-nj->ftp[ii].f_b) * nj->phe;
		  break;
		case 2:
		  eval += ko[i] * (nj->ftp[ii].f_w-nj->ftp[ii].f_b);
		  break;
		default:
		  break;
		}
	}
//	if(i!=-1) LOGGER_0("NJAC EVAL FTS, val %f, rem %f\n", eval, nj->rem); 
//	else LOGGER_0("NJAC EVAL noFTS, val %f, rem %f\n", eval, nj->rem);
  return eval;
}

int compute_evals(double *ko, njac *nj, matrix_type *m,
			  long start, long end, long *indir)
	{
  long i, f;

  if(indir==NULL) {
#pragma omp parallel for 
	for (i = start; i < end; i++)
	{
	  nj[i].fxnew = njac_eval (ko, nj + i, m);
	}
  } else {
#pragma omp parallel for
	for (f = start; f < end; f++)
	{
	  i = indir[f];
	  nj[i].fxnew = njac_eval (ko, nj + i, m);
//	  LOGGER_0("NJAC EVAL i:%ld, nj[i].fxnew %f\n", i, nj[i].fxnew);
	}
  }
	return 0;
}

double compute_njac_error_dir(double *ko, njac *nj, long start, long stop, matrix_type *m, double K){
double err, eval;
long i;
njac *NN;

	err=0;
	compute_evals(ko, nj, m, start, stop, NULL);
#pragma omp parallel for reduction(+:err)
	for(i=start;i<stop;i++) {
		NN=nj+i;
//		LOGGER_0("i:%ld, ko:%f, nj:%p, NN:%p, m:%p\n", i, *ko, nj, NN, m);
		err+=comp_cost_vkx(NN->fxnew, NN->res, K);
	}
return err;
}

double compute_njac_error(double *ko, njac *nj, long start, long stop, matrix_type *m, long *indir, double K){
double err, eval;
long i,q;
njac *NN;

	err=0;
	compute_evals(ko, nj, m, start, stop, indir);
#pragma omp parallel for reduction(+:err)
	for(i=start;i<stop;i++) {
		q = indir[i];
		NN=nj+q;
//		LOGGER_0("i:%ld, q:%ld, ko:%f, nj:%p, NN:%p, m:%p\n", i, q, *ko, nj, NN, m);
		err+=comp_cost_vkx(NN->fxnew, NN->res, K);
	}
return err;
}

/*
 * koef values - values being tuned (total number of koefs is pcount)
 * feature values - features triggered by position
 * njac - description of each position - original evaluation, result of position, non mutable part of eval
 * matrix info - desc of koefs
 * tuner state
 * param counts - num of values tuned
 * start - index of first position
 * len - number of positions to 
 */

/*
 * xxx
 */

// compute parameter updates
int 
njac_pupdate (double *ko, njac *nj, matrix_type *m,
			  ntuner_run *state, int pcount, long start, long len,
			  ntuner_global *tun, long *indir, int iter)
{
  // compute evals
  long end, i;
  double grd, x, y, r, rr, x_hat, y_hat, oon, phase;

//  print_matrix(m, pcount);
  end = start + len;
  compute_evals( ko, nj, m, start, end, indir);

  for (i = 0; i < pcount; i++) state[i].grad = 0;

#pragma omp parallel
  {
  long q,ii,f;
  int8_t ft;
  
  #pragma omp for
  for (q = start; q < end; q++)
	{
	  f = indir[q];
	  for (ii = 0; ii < nj[f].fcount; ii++)
		{
		  i = nj[f].ftp[ii].idx;
		  if(m[i].tunable!=1) continue;
		  if((m[i].value_type) != 1) phase=nj[f].phb; 
			else phase=nj[f].phe;

//		  LOGGER_0("phase %d, %f\n", nj[f].phase, phase);
		  ft= nj[f].ftp[ii].f_w - nj[f].ftp[ii].f_b;
		  state[i].grad += njac_pderiv (ko[i], ft,
										nj[f].res, nj[f].fxnew, phase,
										tun->K);
//		  LOGGER_0("i:%d,ko[i]: %f, grad %f\n", i, ko[i], state[i].grad);
		}
	}
  }
  
  for (i = 0; i < pcount; i++)
	{
	  if(m[i].tunable!=1) continue;
//	  LOGGER_0("STATEi %d,ko[i] %.30f , grad %.30f\n", i,ko[i], state[i].grad);
	  state[i].grad /= len;
//	  LOGGER_0("STATE %d,ko[i] %.30f ,grad tot %.30f, grad %.30f, reg %.30f, grad abs %.30f\n", i,ko[i], state[i].grad+2*tun->reg_la*ko[i], state[i].grad, 2*tun->reg_la*ko[i], state[i].grad*len);
	  state[i].grad += 2*tun->reg_la*ko[i];
//	  state[i].grad /= pcount;
	}

  for (i = 0; i < pcount; i++)
	{
	  if(m[i].tunable!=1) continue;
	  switch (tun->method)
		{
		case 2:
		  state[i].or2 = (state[i].or2 * tun->la2)
			  + (pow(state[i].grad, 2)) * (1 - tun->la2);
		  y = sqrt(state[i].or2 + tun->small_c);
		  r = 0 - state[i].grad / y;
//		  LOGGER_0("CASE %d, R %f, Y %f, grad %f\n", i, r, y, state[i].grad);
		  break;
		case 1:
		  state[i].or2 = (state[i].or2 * tun->la2)
			  + (pow(state[i].grad, 2)) * (1 - tun->la2);
		  x = sqrt(state[i].or1);
		  y = sqrt(state[i].or2 + tun->small_c);
		  r = 0 - state[i].grad * x / y;
		  state[i].or1 = (state[i].or1 * tun->la1)
			  + (pow(r, 2)) * (1 - tun->la1);
		  break;
		case 0:
		  state[i].or2 = (state[i].or2 * tun->la2)
			  + (pow(state[i].grad, 2)) * (1 - tun->la2);
		  state[i].or1 = (state[i].or1 * tun->la1)
			  + (pow(state[i].grad, 1)) * (1 - tun->la1);
		  y_hat = state[i].or2 / (1.0 - pow(tun->la2, iter));
		  x_hat = state[i].or1 / (1.0 - pow(tun->la1, iter));
		  x = x_hat;
		  y = sqrt(y_hat) + tun->small_c;
		  r = 0 - x / y;
		  break;
		default:
		  abort ();
		}
	  // update koefs
//		dump_grads(state, pcount);
	  ko[i] += (r * tun->temp_step);
	  if(m[i].norm_f!=NULL) ko[i] = (m[i].norm_f)(ko[i]);
	}
  return 0;
}

long
file_load_driver (int long max, njac *state, matrix_type **m,
				  personality *p, int pcount, CBACK, void *cdata)
{
  int long l, ix;

  board b;
  struct _ui_opt uci_options;
  struct _statistics s;

  b.uci_options = &uci_options;
  b.stats = &s;
  b.hs = allocateHashStore (HASHSIZE, 2048);
  b.hht = allocateHHTable ();
  b.hps = NULL;
// paralelize 

  l=0;
  ix=0;

#pragma omp parallel firstprivate(b) default(none) shared(p, l, ix, max, state, cdata, pcount, get_next)
//#pragma omp parallel num_threads(2)
  {
  char fen[100];
  long ll,xx;
  int8_t res;
  matrix_type *mx;
  int stop;
  int unused_all;

// instantiate personality to each thread
// and map it to tuner matrix
  b.pers=init_personality(NULL);
  copyPers(p, b.pers);
  to_matrix(&mx, b.pers);
  
  unused_all=0;
  ll=0;
  stop=0;
  while ((l < max) && (ix != -1))
	{
	  xx=get_next (fen, &res, cdata);
// critical section
#pragma omp critical
	  {
	  if((l < max) && (ix != -1)) {
		if(xx==-1) {
		ix=xx;
		stop=1;
		}
		if(!unused_all) ll=l++;
		else unused_all=0;
	  } else stop=1;
	  }
	  if(stop==0) {
		  if (ll % 1000 == 0) printf ("ll:%ld ix:%ld\n", ll, ix);
		  setup_FEN_board (&b, fen);
// trying to ignore something

#if 0
//		  if(eval_phase(&b, b.pers)<127) {
		  if((p->mat_info[b.mindex][b.side])!=255) {
			unused_all=1;
			continue;
		  }
#endif
		  state[ll].res = res;
		  check_mindex_validity(&b, 1);
		  populate_njac (&b, &(state[ll]), b.pers, mx, pcount);	
	  }
	}

	free_matrix(mx, pcount);
	free(b.pers);
  }
  freeHHTable (b.hht);
  freeHashStore (b.hs);	
  printf ("Imported %ld positions\n", l);
  LOGGER_0("Imported %ld positions\n", l);
  return l;
}

typedef struct __file_load_cb_data
{
  FILE *handle;
  long offs;
  long nth;
  long len;
  long pos;
} file_load_cb_data;

long
file_load_cback1 (char *fen, int8_t *res, void *data)
{
  char buffer[512];
  char rrr[128], *xx;
  char *name;
  file_load_cb_data *i;

  i = (file_load_cb_data*) data;

  while (!feof (i->handle))
	{
	  xx = fgets (buffer, 511, i->handle);
	  if ((i->pos % (i->nth)) == i->offs)
		{
		  // get FEN
		  if (parseEPD (buffer, fen, NULL, NULL, NULL, NULL, rrr, NULL, &name)
			  > 0)
			{
			  if (!strcmp (rrr, "1-0"))
				*res = 2;
			  else if (!strcmp (rrr, "0-1"))
				*res = 0;
			  else if (!strcmp (rrr, "1/2-1/2"))
				*res = 1;
			  else
				{
				  printf ("Result parse error\n");
				  abort ();
				}
			  i->len++;
			  i->pos++;
			  return i->len-1;
			}
		}
	  i->pos++;
	}
  return -1;
}

int
texel_file_load1 (char *sts_tests[], long nth, long offs,
				  file_load_cb_data *data)
{
  char filename[256];
  strcpy (filename, sts_tests[0]);
  if ((data->handle = fopen (filename, "r")) == NULL)
	{
	  printf ("File %s is missing\n", filename);
	  return -1;
	}
  data->offs = offs;
  data->nth = nth;
  data->len = 0;
  data->pos = 0;
  return 0;
}

int
texel_file_stop1 (file_load_cb_data *data)
{
  fclose (data->handle);
  return 1;
}

int koef_load(double **ko,  matrix_type *m, int pcount)
{
int f;
	*ko=malloc(sizeof(double)*pcount);
	if(*ko==NULL) return 0;
	for(f=0;f<pcount;f++) {
	  (*ko)[f]= *(m[f].u[0]);
	  LOGGER_0("Koef %d => %d\n", f, *(m[f].u[0]));
	}
return 1;
}

int print_koefs(double *ko, int pcount)
{
int i;
char buf[256];
	LOGGER_0("Koefs: ");
	for(i=0;i<pcount;i++) {
		NLOGGER_0("xk %d:%f, ", i, ko[i]);
	}
	NLOGGER_0("\n");
return 0;
}

int copy_koefs(double *from, double *to, int pcount)
{
int f;
	for(f=0;f<pcount;f++) *to=*from;
return 0;
}

void
texel_loop_njac (ntuner_global *tuner, double *koefs, char *base_name, njac *ver, long vlen)
{
  int n;
  ntuner_run *state;
  double *best;

  unsigned long long int totaltime;
  struct timespec start, end;

  int gen, perc, ccc;
  long *rnd, *rids, r1, r2, rrid;
  char nname[256];
  double fxh, fxh2 = 0, fxh3, fxb, t, fxb1, vxh, vxh3;

  double *cvar;
  long i, l;

  if(ver==NULL) vlen=0;
  
  allocate_ntuner (&state, tuner->pcount);
  init_ntuner_jac (state, tuner->m, tuner->pcount);

  rids = rnd = NULL;
  // randomization init
  rnd = malloc (sizeof(long) * tuner->len);
  rids = malloc (sizeof(long) * tuner->len);

  best = (double *)malloc(sizeof(double)*tuner->pcount);

  for (i = 0; i < tuner->len; i++)
	{
	  rnd[i] = i;
	  rids[i] = i;
	}

  srand (time (NULL));
  switch (tuner->method)
	{
	case 2:
	  t = tuner->rms_step;
	  break;
	case 1:
	  t = tuner->adadelta_step;
	  break;
	case 0:
	  t = tuner->adam_step;
	  break;
	default:
	  abort ();
	}
  tuner->temp_step = t * (tuner->batch_len);

	copy_koefs(koefs, best, tuner->pcount);
	
  // looping over testing ...
  // compute loss with current parameters
  fxb = fxh = compute_njac_error_dir(koefs, tuner->nj, 0, tuner->len, tuner->m, tuner->K)/tuner->len;
  vxh=vxh3=0;
  if(vlen!=0) vxh = vxh3 = compute_njac_error_dir(koefs, ver, 0, vlen, tuner->m, tuner->K)/vlen;

  for (gen = 1; gen <= tuner->generations; gen++)
	{

#if 1
// randomize for each generation
	  for (i = 0; i < tuner->len; i++)
		{
		  r1 = rnd[i];
		  if(r1!=i) continue;
		  rrid = rand() % tuner->len;
		  r2 = rnd[rrid];
		  rnd[i] = r2;
		  rnd[rrid] = r1;
		  rids[r2] = i;
		  rids[r1] = rrid;
		}
#endif

//	  print_koefs(koefs, tuner->pcount);

		{
		  readClock_wall (&start);
		  sprintf (nname, "%s_%ld_%d.xml", base_name, tuner->batch_len, gen);
		  // compute loss prior tuning
		  LOGGER_0("GEN %d, blen %ld, Initial loss of whole data =%.10f JAC\n",
				   gen, tuner->batch_len, fxh);
		  // tuning part
		  // in minibatches
		  ccc = 1;
		  i = 0;
		  perc = 10;
		  while (tuner->len > i)
			{
			  // set new batch
			  l = ((tuner->len - i) >= tuner->batch_len) ?
				  tuner->batch_len : tuner->len - i;
			// update parameters based on this batch
			  njac_pupdate(koefs, tuner->nj, tuner->m, state, tuner->pcount, i, l, tuner, rnd, ccc);
//			  print_koefs(koefs, tuner->pcount);
			  ccc++;
			  if ((i * 100 / tuner->len) > perc)
				{
				  fflush (stdout);
				  perc += 10;
				}
			  i += l;
			}
		  // compute loss based on new parameters
		  fxh3 = compute_njac_error_dir(koefs, tuner->nj, 0, tuner->len, tuner->m, tuner->K);
		  fxh2 = fxh3 / tuner->len;
		  if(vlen!=0) vxh3 = compute_njac_error_dir(koefs, ver, 0, vlen, tuner->m, tuner->K)/vlen;
		  readClock_wall (&end);
		  totaltime = diffClock (start, end);
		  printf (
			  "GEN %d, blen %ld, Final loss of whole data =%.10f:%.10f, %.10f, VerLoss %.10f\n",
			  gen, tuner->batch_len, fxh2, fxh, fxh3, vxh3);
		  LOGGER_0 (
			  "GEN %d, blen %ld, Final loss of whole data =%.10f:%.10f, %.10f, VerLoss %.10f\n",
			  gen, tuner->batch_len, fxh2, fxh, fxh3, vxh3);
		  if ((fxh2 < fxh) && ((vxh3<vxh)||(vlen==0)))
			{
			  fxh = fxh2;
			  vxh = vxh3;
					  printf ("Update\n");
					  LOGGER_0("Update\n");
					  copy_koefs(koefs, best, tuner->pcount);
					  print_koefs(koefs, tuner->pcount);
					  koefs_to_matrix(koefs, tuner->m, tuner->pcount);
					  write_personality (tuner->pi, nname);
			}
		}
	}

  copy_koefs(best, koefs, tuner->pcount);
  free(best);
  free_ntuner(state);
  if (rnd != NULL)
	free (rnd);
  if (rids != NULL)
	free (rids);
}

#endif


void
texel_test ()
{
  int i, *iv, ll;
  double fxb1, fxb2, fxb3, fxbj, fxb4, lambda, K, *jac, *jacn, *koefs, KL, KH, Kstep, x;

  ntuner_global ntun;
  file_load_cb_data tmpdata;

  ntun.max_records = 50000000;
  ntun.generations = 10000;
  ntun.batch_len = 16384;
  ntun.records_offset = 0;
  ntun.nth = 1;
  ntun.small_c = 1E-30;
  ntun.rms_step = 0.000020;
  ntun.adam_step = 0.0002;
  ntun.K=0.00004;
  ntun.la1=0.9;
  ntun.la2=0.999;
  ntun.method=0;
  K=0.00072323115;
  
  njac *vnj;
  long vlen;

  // load position files and personality to seed tuning params
  //	char *xxxx[]= { "../texel/1-0.txt", "../texel/0.5-0.5.txt", "../texel/0-1.txt" };
  //	char *xxxx[]= { "../texel/1-0.epd", "../texel/0.5-0.5.epd", "../texel/0-1.epd" };
  //	char *xxxx[]= { "../texel/quiet-labeled.epd" };
  //	char *xxxx[]= { "../texel/lichess-quiet.txt" };
  //	char *xxxx[]= { "../texel/tc.epd" };
  //	char *xxxx[]= { "../texel/ec.epd" };

  char *files1[] =
	{ "../texel/quiet-labeled.epd" };
  char *files2[] =
//	{ "../texel/tc.epd" };
//	{ "../texel/lichess-quiet.txt" };
//	{ "../texel/e2.epd" };
//	{ "../texel/e12_52.epd" };
	{ "../texel/e12.epd" };
  //	char *files2[]= { "../texel/aa.txt" };
//	{ "../texel/ec.epd" };
//	{ "../texel/quiet-labeled.epd" };

// load personality
  ntun.pi = (personality*) init_personality ("../texel/pers.xml");
// put references to tuned params into structure  
  ntun.pcount = to_matrix (&ntun.m, ntun.pi);
// allocate koeficients array and setup values from personality loaded/matrix...
	if(koef_load(&koefs, ntun.m, ntun.pcount) == 0) abort();

// allocate njac
  if (allocate_njac (ntun.max_records, ntun.pcount, &ntun.nj) == 0)
	abort ();

// initiate files load
  texel_file_load1 (files2, ntun.nth, ntun.records_offset, &tmpdata);
// load each position into njac
  ntun.len=file_load_driver (ntun.max_records, ntun.nj, &ntun.m, ntun.pi,
					ntun.pcount, file_load_cback1, &tmpdata);
// finish loading process
  texel_file_stop1 (&tmpdata);


/*
 * setup verification
 */

  vnj=NULL;
  vlen=0;
#if 1
  if (allocate_njac (8000000, ntun.pcount, &vnj) == 0)
	abort ();
  ntun.nth = 1;
  texel_file_load1 (files1, 1, 0, &tmpdata);
  vlen=file_load_driver (8000000, vnj, &ntun.m, ntun.pi,
					ntun.pcount, file_load_cback1, &tmpdata);
  texel_file_stop1 (&tmpdata);
#endif 

  KL=0.0;
  KH=1.0;
  Kstep=0.05;
  fxb1 = compute_njac_error_dir(koefs, ntun.nj, 0, ntun.len, ntun.m, KL) / ntun.len;
  for(i=0;i<10;i++) {
	x=KL-Kstep;
	while(x<KH) {
	  x+=Kstep;
	  fxb2 = compute_njac_error_dir(koefs, ntun.nj, 0, ntun.len, ntun.m, x) / ntun.len;
	  LOGGER_0("K computation i:%d K: %.30f, loss= %.30f", i, x, fxb2);
	  printf("K computation i:%d K: %.30f, loss= %.30f\n", i, x, fxb2);
	  if(fxb2<=fxb1) {
		fxb1=fxb2;
		KL=x;
		K=x;
		NLOGGER_0(" Update\n");
	  } else NLOGGER_0("\n");
	}
	KH=KL+Kstep;
	KL-=Kstep;
	Kstep/=10.0;
  }

  if(K<1E-6) K=0.00072323115;

  ntun.K=K*1.0;
	LOGGER_0("Using K=%.30f\n", K);


int loo;
double *koef2;
char nname[256];

// allocate koeficients array and setup values from personality loaded/matrix...
	if(koef_load(&koef2, ntun.m, ntun.pcount) == 0) abort();

// LAMBDA

	lambda = 1E-14;
//  lambda = 0;
  for(loo=0;loo<1;loo++) {
	LOGGER_0("Lambda %e\n", lambda);
	ntun.reg_la = lambda;
	koefs_to_matrix(koef2, ntun.m, ntun.pcount);
	matrix_to_koefs(koefs, ntun.m, ntun.pcount);
	
// run tuner itself
	sprintf (nname, "%s_%d_", "../texel/ptest_ptune_" , loo);
	
	texel_loop_njac (&ntun, koefs, nname, vnj, vlen);
  }
  
  free(koef2);
  free(koefs);
  if(vnj!=NULL) free_njac(vnj);
  free_njac(ntun.nj);
  free_matrix (ntun.m, ntun.pcount);
  free (ntun.pi);
}

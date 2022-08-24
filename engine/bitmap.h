#ifndef BITMAP_H
#define BITMAP_H

//#define TUNING

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <pthread.h>
#include "macros.h"
//#include "search.h"
//#include "pers.h"
#include "stats.h"
//#include "globals.h"

typedef uint64_t BITVAR;
typedef uint16_t MOVESTORE;

typedef struct _shiftBit {
			BITVAR field;
			int pos;
} shiftBit;

typedef enum _SQUARES { 	A1=0,B1,C1,D1,E1,F1,G1,H1,
				A2,B2,C2,D2,E2,F2,G2,H2,
				A3,B3,C3,D3,E3,F3,G3,H3,		
				A4,B4,C4,D4,E4,F4,G4,H4,		
				A5,B5,C5,D5,E5,F5,G5,H5,		
				A6,B6,C6,D6,E6,F6,G6,H6,		
				A7,B7,C7,D7,E7,F7,G7,H7,		
				A8,B8,C8,D8,E8,F8,G8,H8,		
				ER_SQUARE } SQUARES;

typedef enum _PIECE { PAWN=0, KNIGHT, BISHOP, ROOK, QUEEN, KING, ER_PIECE } PIECE;
typedef enum _SIDE { WHITE=0, BLACK, ER_SIDE } SIDE;
typedef enum _CASTLE { NOCASTLE=0,  QUEENSIDE, KINGSIDE, BOTHSIDES, ER_CASTLE } CASTLE;
//typedef enum { OPENING=0, MIDDLE, ENDING, ER_GAMESTAGE } GAMESTAGE;
typedef enum _GAMESTAGE { OPENING=0, ENDING, ER_GAMESTAGE } GAMESTAGE;
typedef enum _MOBILITY { ER_MOBILITY=29 } MOBILITY;
typedef enum _ENGINE_STATES { MAKE_QUIT=0, STOP_THINKING, STOPPED, START_THINKING, THINKING } ENGINE_STATES;
typedef enum _SPECIAL_MOVES { DRAW_M=0x6000U, MATE_M=0x6041, NA_MOVE=0x6082, WAS_HASH_MOVE=0x60C3, ALL_NODE=0x6104, BETA_CUT=0x6145, NULL_MOVE=0x6186, ERR_NODE=0x61C7 } SPECIAL_MOVES;
typedef enum _LVA_SORT { K_OR_M=6U,P_OR=1U,N_OR=2U,B_OR=3U,R_OR=4U,Q_OR=5U,K_OR=6U,
	MV_BAD=4400U, MV_BAD_MAX=4499U,
	MV_OR=4000U, MV_OR_MAX=4399U,
	MV_HH=4800U, MV_HH_MAX=4999, 
	A_OR2=5000U, A_OR2_MAX=5100U,
	A_OR_N=7500U, A_OR_N_MAX=8412U,
	A_OR=10200U, A_OR_MAX=12600U,

	CS_K_OR=4995U, CS_Q_OR=4990U,
	A_MINOR_PROM=5000U, A_KNIGHT_PROM=10400U, A_QUEEN_PROM=10500U,
	A_CA_PROM_N=12560U, A_CA_PROM_Q=12580U,

	KILLER_OR=7510U, KILLER_OR_MAX=7540U,
	HASH_OR=13000U , PV_OR=13100U } LVA_SORT;

typedef enum _SCORES {  NO_NULL=0, FAILLOW_SC, EXACT_SC, FAILHIGH_SC, ER_SC } SCORES;

typedef enum _MOVEGEN_STATES { INIT=0, PVLINE, HASHMOVE, GENERATE_CAPTURES, CAPTUREA, SORT_CAPTURES, CAPTURES, KILLER1, KILLER2, KILLER3, KILLER4, 
		BADX, GENERATE_NORMAL, NORMAL, OTHER_SET, OTHER, DONE } MOVEGEN_STATES;

//enum _RANKS { RANK1, RANK2, RANK3, RANK4, RANK5, RANK6, RANK7, RANK8, ER_RANKS=8 } RANKS;
typedef enum _RANKS  { ER_RANKS=8 } RANKS;
#define BLACKPIECE 8
#define PIECEMASK 7

#define PAWNS_TOT 16

typedef struct _move_entry {
	MOVESTORE move;
	long int qorder;
	int real_score;
} move_entry;

//#define KMOVES_DEPTH 256
#define KMOVES_WIDTH 2

typedef struct _kmoves {
	MOVESTORE move;
	int value;
} kmoves;

typedef struct _move_cont {
	int phase;
	int actph;
	int count;
	int tcnt;
	move_entry *next;	// points at end of moves already offered, starts at move
						// it includes even moves that were considered but failed validity and/or were skipped then
	move_entry *badp;	// points at end of bad moves, starts at bad
	move_entry *exclp;	// points at end of excluded moves list, starts at excl
	move_entry *lastp;	// points at end of list of all moves generated, starts at move
	
	move_entry hash;
	move_entry killer1;
	move_entry killer2;
	move_entry killer3;
	move_entry killer4;
	
	move_entry move[300];
	move_entry bad [300];
	move_entry excl[300];
} move_cont;

#define EMPTYBITMAP 0ULL
#define RANK8 0xFF00000000000000ULL
#define RANK7 0x00FF000000000000ULL
#define RANK6 0x0000FF0000000000ULL
#define RANK5 0x000000FF00000000ULL
#define RANK4 0x00000000FF000000ULL
#define RANK3 0x0000000000FF0000ULL
#define RANK2 0x000000000000FF00ULL
#define RANK1 0x00000000000000FFULL
#define FULLBITMAP 0xFFFFFFFFFFFFFFFFULL

//#define MATEMAX 0x5000000
// giving space for 256 moves
//#define MATEMIN (MATEMAX-2*256)
#define MATEMAX 91000000
#define MATEMIN 90000000

#define CENTERBITMAP 0x0000001818000000ULL
#define CENTEREXBITMAP 0x00003C3C3C3C0000ULL
#define WHITEBITMAP 0xAA55AA55AA55AA55ULL
#define BLACKBITMAP 0x55AA55AA55AA55AAULL
#define FILEA 0x0101010101010101ULL 
#define FILEH 0x8080808080808080ULL
#define BOARDEDGE  0xFF818181818181FFLL
#define BOARDEDGEF 0x8181818181818181LL
#define BOARDEDGER 0xFF000000000000FFLL
#define SHELTERA7  0x0007070000000000LL
#define SHELTERH7  0x00E0E00000000000LL
#define SHELTERM7  0X003C3C0000000000LL
#define SHELTERA2  0x0000000000070700LL
#define SHELTERH2  0x00000000003C3C00LL
#define SHELTERM2  0x0000000000E0E000LL
// abc 0x07
// fgh 0xE0
// def 0x38
#define SHELTERA  0x0007070707070700LL
#define SHELTERH  0x00E0E0E0E0E0E000LL
#define SHELTERM  0X003C3C3C3C3C3C00LL

//#define iINFINITY (INT_MAX-10)
//#define iINFINITY 0x10000000
#define iINFINITY 777777777

void init_nmarks();

void printmask(BITVAR m, char *s);
void printmask90(BITVAR m, char *s);
void printmask45R(BITVAR m, char *s);
void printmask45L(BITVAR m, char *s);
BITVAR SetNorm(int pos, BITVAR map);
BITVAR Set90(int pos, BITVAR map);
BITVAR Set45R(int pos, BITVAR map);
BITVAR Set45L(int pos, BITVAR map);

BITVAR ClrNorm(int pos, BITVAR map);
BITVAR Clr90(int pos, BITVAR map);
BITVAR Clr45R(int pos, BITVAR map);
BITVAR Clr45L(int pos, BITVAR map);

BITVAR get45Rvector(BITVAR board, int pos);
BITVAR get45Lvector(BITVAR board, int pos);
BITVAR get90Rvector(BITVAR board, int pos);
BITVAR getnormvector(BITVAR board, int pos);

int get45Rvector2(BITVAR board, int pos, BITVAR *d1, BITVAR *d2);
int get45Lvector2(BITVAR board, int pos, BITVAR *d1, BITVAR *d2);
int get90Rvector2(BITVAR board, int pos, BITVAR *d1, BITVAR *d2);
int getnormvector2(BITVAR board, int pos, BITVAR *d1, BITVAR *d2);

inline int getRank(int pos){ return (pos>>3)&7;}
inline int getFile(int pos){ return pos&7;}
inline int getPos(int file, int rank){return (rank*8+file)&63;}

inline int BitCount(BITVAR board){ return __builtin_popcountll(board);}

//int LastOne(BITVAR board);
//inline __attribute__((always_inline)) int LastOne(BITVAR board) {return __builtin_ffsll((long long int)board)-1;}
inline __attribute__((always_inline)) int LastOne(BITVAR board) { return __builtin_ctzll((unsigned long long int)board);}

int FirstOne(BITVAR board);

#define ClrLO(x) (x &= x - 1)
//fix jak vymazat nejvyssi bit?
#define ClrHI(x) (x &= x)
shiftBit setupShift(BITVAR x);
int emptyShiftBit(shiftBit s);
void clrNormShift(int p, shiftBit *s);
int LastOneShift(shiftBit s);
		
#define Max(x,y) ((x) > (y) ? (x) : (y))
#define Min(x,y) ((x) < (y) ? (x) : (y))

#define Flip(side) ((side == WHITE) ? BLACK : WHITE)

void mask2add(char *s[9], char (*st)[9]);
void printmask2(BITVAR m, char (*st)[9], char *title);
void mask2print(char *b[9]);
void mask2init2(char (*b)[256], char *out[9]);
void mask2init(char (*b)[9]);

typedef struct _att_mov {
		BITVAR maps[ER_PIECE][64];
		BITVAR pawn_att[2][64];
		BITVAR pawn_move[2][64];
		BITVAR attack_norm [64][256];
		BITVAR attack_r90R [64][256];
		BITVAR attack_r45L [64][256];
		BITVAR attack_r45R [64][256];
		BITVAR attack_norm_2 [64][256];
		BITVAR attack_r90R_2 [64][256];
		BITVAR attack_r45L_2 [64][256];
		BITVAR attack_r45R_2 [64][256];
		BITVAR passed_p[2][64];
		BITVAR back_span_p[2][64];
		BITVAR isolated_p[64];
		BITVAR ep_mask[64];
		BITVAR file[64];
		BITVAR rank[64];
		BITVAR lefthalf[64];
		BITVAR righthalf[64];
		BITVAR uphalf[64];
		BITVAR downhalf[64];
		BITVAR pawn_surr[64];
		int    color_map[64];
		int    distance[64][64];
		int    distance2[64][64];
		int    ToPos[65536];
		BITVAR rays[64][64];
		BITVAR rays_int[64][64];
		BITVAR dirs[64][8];
		BITVAR rays_dir[64][64];
} att_mov;

struct _ui_opt {
//  0 sudden death
	int movestogo;
//
	int wtime;
	int btime;
	int winc;
	int binc;
//  0 ignore
	int depth;
//  0 ignore
	int nodes;
//  0 ignore
	int mate;
//  0 ignore
	int movetime;
//	0 ignore, 1 yes
	int infinite;
//	0 ignore, 1 yes
	int ponder;

	char position[100];
	int pos_moves[500]; //fixme
	int search_moves[100]; //fixme
	int engine_verbose;
};

typedef struct _meval_t {
	int mat;
	int mat_w;
//	int info;
} meval_t;

/*
 * hodnoty mohou byt multivalue 1, 7, 28, 64, 8
 * typy - hodnoty jsou zavisle na
	globalni 		GL
	zavisle na gamestage	GS
	zavisle na GS a Figure
 */

typedef int _general_option;
typedef int _gamestage[ER_GAMESTAGE];
typedef int _values[ER_GAMESTAGE][ER_PIECE+1];
typedef int _dvalues[ER_PIECE][PAWNS_TOT+1];
typedef int _mobility[ER_GAMESTAGE][ER_SIDE][ER_PIECE][ER_MOBILITY];
typedef int _squares[ER_GAMESTAGE][ER_SIDE][ER_SQUARE];
typedef int _squares_p[ER_GAMESTAGE][ER_SIDE][ER_PIECE][ER_SQUARE];
typedef int _passer[ER_GAMESTAGE][ER_SIDE][ER_RANKS];

#undef MLINE
#define MLINE(x,y,z,s_r,i) z y;

typedef struct _personality {
	
	int start_depth;

	_mobility mob_val;
	_mobility mob_uns;
	_squares_p piecetosquare;

// temporary created
// MVALVA
	int LVAcap[ER_PIECE+2][ER_PIECE];

// material
	meval_t mat[420000];
	meval_t mate_e[420000];
	uint8_t mat_info[420000][2];
	uint8_t mat_faze[420000];
	int *matdeb;

	E_OPTS

} personality;

typedef struct _attack_f {
	int i;
} attack_field;

typedef struct _score_type_one {
	int mobi_b; // mobility skore prvni faze
	int mobi_e; // mobility skore koncove faze
	int sqr_b;
	int sqr_e;
	int specs_b;
	int specs_e;

} score_type_one;

typedef struct _mob_eval {
	int pos_att_tot;
	int pos_mob_tot_b;
	int pos_mob_tot_e;
	int mob_count;
} mob_eval;

typedef struct _sqr_eval {
	int sqr_b;
	int sqr_e;
} sqr_eval;

typedef struct _king_eval {
	BITVAR cr_all_ray;
	BITVAR cr_att_ray;
	BITVAR cr_pins;
	BITVAR cr_attackers;
	BITVAR cr_blocks;
	BITVAR cr_blocker_ray;
	
	BITVAR di_all_ray;
	BITVAR di_att_ray;
	BITVAR di_pins;
	BITVAR di_attackers;
	BITVAR di_blocks;
	BITVAR di_blocker_ray;
	
	BITVAR kn_attackers;
	BITVAR kn_pot_att_pos;
	BITVAR pn_attackers;
	BITVAR pn_pot_att_pos;
	BITVAR attackers;
//	BITVAR blocker_ray[64];
	BITVAR attacker_ray[64];
	BITVAR ep_block;

} king_eval;


typedef struct _score_type {
	int material;
	int material_e;
	int material_b_w;
	int material_e_w;
	int material_b_b;
	int material_e_b;
	score_type_one side[ER_SIDE];
	int score_b;
	int score_e;
	int score_b_w;
	int score_b_b;
	int score_e_w;
	int score_e_b;
	int score_nsc;
	int complete;
	int scaling;
} score_type;

typedef struct _attack_model {
// faze - tapered eval
		int phase;
		int pad1;
		//attack_field fields[64];
		BITVAR mvs[64]; // bitmapy jednotlivych figur
// number of attacks from square
		int pos_c[(ER_PIECE | BLACKPIECE)+1];
		int pos_m[(ER_PIECE | BLACKPIECE)+1][10];

		mob_eval me[64];
		sqr_eval sq[64];
		sqr_eval specs[ER_SIDE][ER_PIECE];
  		king_eval ke[ER_SIDE];
// pawn attack moves		
		BITVAR pa_at[ER_SIDE];
		BITVAR pa_mo[ER_SIDE];
//		BITVAR pa_at_mo[ER_SIDE];
		BITVAR att_by_side[ER_SIDE];
		score_type sc;

		BITVAR pins;

} attack_model;

// pouze tuning
#ifdef TUNING
#define MAXPLY 401
#define MAXPLYHIST 2048
#define SEARCH_HISTORY_DEPTH 100
#define HASHSIZE 1000
#define HASHPOS 8
#define HASHPAWNSIZE 100
#define HASHPAWNPOS 4
#else
#define MAXPLY 401
#define MAXPLYHIST 2048
#define SEARCH_HISTORY_DEPTH 100
#define HASHSIZE 2000000
#define HASHPOS 8
#define HASHPAWNSIZE 25600
#define HASHPAWNPOS 4
#endif
//#define TREE_STORE_DEPTH 301
//#define TREE_STORE_WIDTH 301

typedef struct _runtime_o {
// timing
		unsigned long long int time_start;
		unsigned long long int nodes_mask;
		unsigned long long int iter_start;
		unsigned long long int nodes_at_iter_start;
		unsigned long long int time_move;
		unsigned long long int time_crit;
		pthread_t engine_thread;

} runtime_o;

// hashing

typedef struct _hashEntry {
	BITVAR key;
	BITVAR map;
	int32_t value; //
	MOVESTORE bestmove; //
	int16_t depth; //
	uint8_t age; //
	uint8_t  scoretype;
// 8,8,4,2,2,1,1
} hashEntry;

typedef struct _hashEntry_e {
	hashEntry e[HASHPOS];
} hashEntry_e;

typedef struct _hhTable {
	int val[2][6][64];
} hhTable;

typedef enum _variants { BAs=0, HEa, SHa, SHh, SHm, SHah, SHhh, SHmh, ER_VAR } VARIANTS;

typedef struct _PawnStore {
BITVAR not_pawns_file[2];
BITVAR maxpath[2];
BITVAR half_att[2][2];
BITVAR half_isol[2][2];
BITVAR double_att[2];
BITVAR odd_att[2];
BITVAR safe_att[2];
BITVAR paths[2];
BITVAR path_stop[2];
BITVAR path_stop2[2];
BITVAR one_side[2];
BITVAR one_s_att[2][2];
BITVAR passer[2];
BITVAR pass_end[2];
BITVAR stopped[2];
BITVAR blocked[2];
BITVAR isolated[2];
BITVAR doubled[2];
BITVAR back[2];
BITVAR prot[2];
BITVAR prot_p[2];
BITVAR prot_dir[2];
BITVAR spans[2][8][2];
BITVAR pot_sh[2];

BITVAR shelter_p[2][3];

sqr_eval shelter_a[2];
sqr_eval shelter_h[2];
sqr_eval shelter_m[2];

sqr_eval shelter_r_a[2];
sqr_eval shelter_r_h[2];
sqr_eval shelter_r_m[2];

sqr_eval sh_opts[2][3];

int pas_d[2][9], stop_d[2][9], block_d[2][9], double_d[2][9];
int pawns[2][9], outp[2][9], outp_d[2][9], prot_d[2][9], prot_p_d[2][9];
int prot_dir_d[2][9];

//sqr_eval opts[2][2];

sqr_eval score[2][ER_VAR];
sqr_eval t_sc[2][9][ER_VAR];

} PawnStore;


typedef struct _hashPawnEntry {
	BITVAR key;
	BITVAR map;
	PawnStore value; //
	uint8_t age; //
} hashPawnEntry;

typedef struct _hashPawnEntry_e {
	hashPawnEntry e[HASHPAWNPOS];
} hashPawnEntry_e;

typedef struct _hashPawnStore {
	int hashlen;
	uint8_t hashValidId;
	hashPawnEntry_e *hash;
} hashPawnStore;

typedef struct {
		MOVESTORE move;
//		board tree_board;
//		skore z podstromu pod/za vybranym "nejlepsim" tahem
		int score;
} tree_node;

typedef struct _hashEntryPV {
	BITVAR key;
	BITVAR map;
	tree_node pv[MAXPLY+2]; 
	uint8_t age; 
} hashEntryPV;

typedef struct _hashEntryPV_e {
	hashEntryPV e[16];
} hashEntryPV_e;

typedef struct _hashStore {
	int hashlen;
	int hashPVlen;
	uint8_t hashValidId;
	hashEntry_e *hash;
	hashEntryPV_e *pv;
} hashStore;

typedef struct _tree_line {
//		board tree_board;
		tree_node line[MAXPLY+3];
		int score;
} tree_line;

typedef struct _bit_board {
// *** board specific part ***
		BITVAR maps[ER_PIECE];
	
// board of relevant color, 0 white, 1 black
		BITVAR colormaps[ER_SIDE];
		BITVAR norm;
		BITVAR r90R;
		BITVAR r45R;
		BITVAR r45L;
		int8_t pieces[64]; // pieces
		uint8_t material[ER_SIDE][2*ER_PIECE]; // each side material, ER_PIECE+BISHOP = num of darkbishops
		int mindex;
		int psq_b;
		int psq_e;
		int8_t mindex_validity;
		int8_t ep; // e.p. square
		int8_t side; // side to move
		int8_t castle[ER_SIDE]; // castling possibility // 0 no, 1 - queenside, 2 - kingside, 3 - both
		int8_t king[ER_SIDE]; // king position
		int16_t move; //  plies... starts at 0 - ply/move to make
		int16_t rule50move; // ukazatel na posledni pozici, ktera vznikla branim nebo tahem pescem
		int16_t gamestage;

		int16_t move_start; // pocet plies, ktere nemam v historii
		int16_t move_ply_start;
// previous positions for repetition draw
		BITVAR positions[MAXPLYHIST+1]; // vzdy je ulozena pozice pred tahem. Tj. na 1 je pozice po tahu 0. Na pozici 0 je ulozena inicialni stav
		BITVAR posnorm[MAXPLYHIST+1];
		BITVAR key; // hash key
		BITVAR pawnkey; // pawn hash key

		struct _statistics *stats;
		struct _ui_opt *uci_options;
		struct _runtime_o run;
// search info
// from last completed evaluation
		MOVESTORE bestmove;
		int bestscore;
		int depth_run;
		tree_line p_pv;
// ...
		personality *pers;
		hashStore *hs;
		hashPawnStore *hps;
		hhTable *hht;
		kmoves *kmove;
// just for debugging, remove!
//		void *td;
//		int trace;

} board;

typedef struct _tree_store {
// situace na desce
		board tree_board;
		int depth;
		tree_node tree[MAXPLY+3][MAXPLY+3];
		long int score;
} tree_store;

typedef struct _search_history {
	board history[SEARCH_HISTORY_DEPTH];
} search_history;

typedef struct _opts {
			int zeromove;
			int killers;
			int quiesce;
			int hash;
			int alphabeta;
		} opts;


typedef int (*tuner_cback)(void *);

typedef struct _matrix_type {
	int upd;
	int *u[4];
	int (*init_f)(void*);
	int (*restore_f)(void*);
	void *init_data;
	int mid;
	int ran;
	int min;
	int max;
	int value_type;
	int tunable;
	int counterpart;
	double (*norm_f)(double);
	double vata;
} matrix_type;

typedef struct {
	double grad;
	double or2;
	double or1;
	double update;
	double real;
} tuner_run;

typedef struct {
	int generations;
	long batch_len;
	int method;
	long max_records;
	int records_offset;
	int nth;
	int diff_step;
	double small_c;
	double la1;
	double la2;
	double reg_la;
	double rms_step;
	double adadelta_step;
	double adam_step;
	board *boards;
	uint8_t *phase;
	int8_t *results;
	long len;
	personality *pi;
	int pcount;
	int *matrix_var_backup;
	matrix_type *m;
	double *jac;
	double *nvar;
	double *ivar;
	double penalty;
	double temp_step;
	double K;
} tuner_global;

typedef struct {
int idx;
int8_t type;
int8_t f_b;
int8_t f_w;
} feat;

typedef struct {
int8_t res;
//uint8_t phase;
double phb;
double phe;
double fx0;
double fxnew;
double rem;
feat *ftp;
int fcount;
//int matfix;
} njac;

typedef struct {
	double grad;
	double or2;
	double or1;
} ntuner_run;

typedef struct {
	int generations;
	long batch_len;
	int method;
	long max_records;
	int records_offset;
	int nth;
	double small_c;
	double la1;
	double la2;
	double reg_la;
	double rms_step;
	double adadelta_step;
	double adam_step;
	double temp_step;
	long len; // num of positions
	personality *pi;
	int pcount; // num of koefs
	matrix_type *m; // description of koefs
	njac *nj; // position info - current eval, phase, etc
	double penalty;
	double K;
} ntuner_global;

void backup_att(att_mov * z);
void backup_test(att_mov * z);

void SetAll(int pos, int side, int piece, board *b);
void ClearAll(int pos, int side, int piece, board *b);
void MoveFromTo(int from, int to, int side, int piece, board *b);

void outbinary(BITVAR m, char *o);

#endif

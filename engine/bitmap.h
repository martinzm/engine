#ifndef BITMAP_H
#define BITMAP_H

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "macros.h"
//#include "search.h"
//#include "pers.h"

typedef uint64_t BITVAR;

typedef struct _shiftBit {
			BITVAR field;
			int pos;
} shiftBit;

typedef enum { 	A1=0,B1,C1,D1,E1,F1,G1,H1,
				A2,B2,C2,D2,E2,F2,G2,H2,
				A3,B3,C3,D3,E3,F3,G3,H3,		
				A4,B4,C4,D4,E4,F4,G4,H4,		
				A5,B5,C5,D5,E5,F5,G5,H5,		
				A6,B6,C6,D6,E6,F6,G6,H6,		
				A7,B7,C7,D7,E7,F7,G7,H7,		
				A8,B8,C8,D8,E8,F8,G8,H8,		
				ER_SQUARE } SQUARES;

//typedef enum { BISHOP=0, KNIGHT, ROOK, KING, QUEEN, PAWN, ER_PIECE } PIECE;
typedef enum { PAWN=0, KNIGHT, BISHOP, ROOK, QUEEN, KING, ER_PIECE } PIECE;
typedef enum { WHITE=0, BLACK, ER_SIDE } SIDE;
typedef enum { NOCASTLE=0,  QUEENSIDE, KINGSIDE, BOTHSIDES, ER_CASTLE } CASTLE;
//typedef enum { OPENING=0, MIDDLE, ENDING, ER_GAMESTAGE } GAMESTAGE;
typedef enum { OPENING=0, ENDING, ER_GAMESTAGE } GAMESTAGE;
typedef enum { ER_MOBILITY=28 } MOBILITY;

typedef enum { MAKE_QUIT=0, STOP_THINKING, STOPPED, START_THINKING, THINKING } engine_states;

typedef enum { DRAW_M=0x4000U, MATE_M, NA_MOVE, WAS_HASH_MOVE, ALL_NODE, BETA_CUT, NULL_MOVE } special_moves;

#define ER_RANKS 8

//#define DRAW_M 0xefef
//#define MATE_M 0xffff
//#define NA_MOVE 0xeeee
//#define WAS_HASH_MOVE 0xfefe
//#define ALL_NODE 0xffee
//#define BETA_CUT 0xffee
//#define NULL_MOVE 0x0

#define BLACKPIECE 8
#define PIECEMASK 7

#define PV_OR 10100
#define HASH_OR 10000
#define KILLER_OR 6900
// defines for LVA
#define A_OR 7000
// loosing cap
#define A_OR2 5000
#define P_OR 10
#define N_OR 20
#define B_OR 30
#define R_OR 40
#define Q_OR 50
#define K_OR 60
// pro pohyb samotny bude K za 1
#define CS_Q_OR 6200
#define CS_K_OR 6300
#define MV_OR 2000

/*
 
 */
 
typedef struct _move_entry {
	int move;
	int qorder;
	int real_score;
} move_entry;

#define EMPTYBITMAP 0ULL
#define RANK8 0xFF00000000000000ULL
#define RANK7 0x00FF000000000000ULL
#define RANK6 0x0000FF0000000000ULL
#define RANK5 0x000000FF00000000ULL
#define RANK4 0x00000000FF000000ULL
#define RANK3 0x0000000000FF0000ULL
#define RANK2 0x000000000000FF00ULL
#define RANK1 0x00000000000000FFULL

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

//#define iINFINITY (INT_MAX-10)
//#define iINFINITY 0x10000000
#define iINFINITY 777777777

typedef enum {  NO_SC=0, FAILLOW_SC, EXACT_SC, FAILHIGH_SC, ER_SC } SCORES;

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
BITVAR get90Rvector2(BITVAR board, int pos);
BITVAR getnormvector(BITVAR board, int pos);
BITVAR getnormvector2(BITVAR board, int pos);

int getFile(int pos);
int getRank(int pos);

int BitCount(BITVAR board);
int LastOne(BITVAR board);
int FirstOne(BITVAR board);

#define ClrLO(x) (x &= x - 1)
//fix jak vymazat nejvyssi bit?
#define ClrHI(x) (x &= x)
shiftBit setupShift(BITVAR x);
int emptyShiftBit(shiftBit s);
void clrNormShift(int p, shiftBit *s);
int LastOneShift(shiftBit s);
		
typedef struct _att_mov {
		BITVAR maps[ER_PIECE][64];
		BITVAR pawn_att[2][64];
		BITVAR pawn_move[2][64];
		BITVAR tramp1[256];
		BITVAR attack_norm [64][256];
		BITVAR attack_r90R [64][256];
		BITVAR attack_r45L [64][256];
		BITVAR attack_r45R [64][256];
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
} att_mov;

struct _statistics {
	unsigned long long failnorm;
	unsigned long long faillow;
	unsigned long long failhigh;
	unsigned long long positionsvisited;
	unsigned long long movestested;
	unsigned long long possiblemoves;
	unsigned long long zerototal;
	unsigned long long zerorerun;
	unsigned long long quiesceoverrun;
	unsigned long long qposvisited;
	unsigned long long qmovestested;
	unsigned long long qpossiblemoves;
	unsigned long long lmrtotal;
	unsigned long long lmrrerun;
	unsigned long long fhflcount;
	unsigned long long firstcutoffs;
	unsigned long long cutoffs;
	unsigned long long NMP_cuts;
	unsigned long long NMP_tries;

	int depth;
};

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
	int info;
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
typedef int _values[ER_GAMESTAGE][ER_PIECE];
typedef int _mobility[ER_GAMESTAGE][ER_SIDE][ER_PIECE][ER_MOBILITY];
typedef int _squares[ER_GAMESTAGE][ER_SIDE][ER_SQUARE];
typedef int _squares_p[ER_GAMESTAGE][ER_SIDE][ER_PIECE][ER_SQUARE];
typedef int _passer[ER_GAMESTAGE][ER_SIDE][ER_RANKS];

#undef MLINE
#define MLINE(x,y,z,i) z y;

typedef struct _personality {
	
	_passer passer_bonus;

	_mobility mob_val;
	_squares_p piecetosquare;
	_values Values;
// temporary created
	meval_t mat[420000];
	meval_t mate_e[420000];

	E_OPTS;

} personality;

typedef struct _attack_f {
	int i;
} attack_field;

typedef struct _score_type_one {
	int pstruct; 	// pawn structure quality
	int ksafety; 	// how king is protected
	int space; 	// space covered
	int center; 	// how is center occupied
	int threats;	// attacks ?
	int pieces;		// pieces / squares

	int mobi_b; // mobility skore prvni faze
	int mobi_e; // mobility skore koncove faze
	int sqr_b;
	int sqr_e;

} score_type_one;

typedef struct _mob_eval {
	int pos_att_tot;
	int pos_mob_tot_b;
	int pos_mob_tot_e;
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
	BITVAR cr_blocker_piece;
	BITVAR cr_blocker_ray;
	
	BITVAR di_all_ray;
	BITVAR di_att_ray;
	BITVAR di_pins;
	BITVAR di_attackers;
	BITVAR di_blocker_piece;
	BITVAR di_blocker_ray;
	
	BITVAR kn_attackers;
	BITVAR kn_pot_att_pos;
	BITVAR pn_attackers;
	BITVAR pn_pot_att_pos;
	BITVAR attackers;

} king_eval;

typedef struct _score_type {
	int material; 	// material
	int material_e;
	score_type_one side[ER_SIDE];
	int complete;
} score_type;

typedef struct _attack_model {
// faze - tapered eval
		int phase;
		//attack_field fields[64];
		BITVAR mvs[64]; // bitmapy jednotlivych figur
// number of attacks from square
		int pos_c[ER_PIECE|BLACKPIECE];
		int pos_m[ER_PIECE|BLACKPIECE][10];

		mob_eval me[64];
		sqr_eval sq[64];
		king_eval ke[ER_SIDE];
		BITVAR pa_at[ER_SIDE];
		BITVAR att_by_side[ER_SIDE];
		score_type sc;

		BITVAR pins;

} attack_model;

typedef struct _bit_board {
// *** board specific part ***
		BITVAR maps[ER_PIECE];
	
// board of relevant color, 0 white, 1 black
		BITVAR colormaps[ER_SIDE];
		BITVAR norm;
		BITVAR r90R;
		BITVAR r45R;
		BITVAR r45L;
		int pieces[64]; // pieces
		unsigned char material[ER_SIDE][2*ER_PIECE]; // each side material, ER_PIECE+BISHOP = num of darkbishops
//		int mcount[ER_SIDE];
		int mindex;
		int mindex_validity;
		BITVAR mindex2;
		int ep; // e.p. square
		int side; // side to move
		int castle[ER_SIDE]; // castling possibility // 0 no, 1 - queenside, 2 - kingside, 3 - both
		int king[ER_SIDE]; // king position
		int move; //  plies... starts at 0 - ply/move to make
		int rule50move; // ukazatel na posledni pozici, ktera vznikla branim nebo tahem pescem
		
		BITVAR key; // hash key

// previous positions for repetition draw
		BITVAR positions[384]; // vzdy je ulozena pozice pred tahem. Tj. na 1 je pozice po tahu 0. Na pozici 0 je ulozena inicialni stav
		BITVAR posnorm[384];
		int gamestage;

// timing
		unsigned long long int time_start;
		unsigned long long int nodes_mask;
		unsigned long long int iter_start;
		int time_move;
		int time_crit;

		struct _statistics stats;
		struct _ui_opt uci_options;
// search info
// from last completed evaluation
		int bestmove;
		int bestscore;
// ...
		personality *pers;
} board;

#define TREE_STORE_DEPTH 128
#define TREE_STORE_WIDTH 128

typedef struct {
// situace na desce
		board tree_board;
		board after_board;
// evaluace desky
		attack_model att;
		attack_model after_att;
// 		vybrany tah pro pokracovani
		int move;
//		skore z podstromu pod/za vybranym "nejlepsim" tahem
		int score;
} tree_node;
 
typedef struct _tree_store {
//		int depth;
//		int offset;
		tree_node tree[TREE_STORE_DEPTH+1][TREE_STORE_DEPTH+1];
} tree_store;

#define SEARCH_HISTORY_DEPTH 100
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

void backup_att(att_mov * z);
void backup_test(att_mov * z);

void SetAll(int pos, int side, int piece, board *b);
void ClearAll(int pos, int side, int piece, board *b);
void MoveFromTo(int from, int to, int side, int piece, board *b);

void outbinary(BITVAR m, char *o);

#endif

#ifndef MOVGEN_H
#define MOVGEN_H

#include "bitmap.h"

typedef struct _undo {
	int castle[ER_SIDE];
	int side;
	int move;
	unsigned char captured; //what was captured
	unsigned char moved; // promoted to in case promotion, otherwise the same as old
	unsigned char old; //what was the old piece
	int rule50move;
	int ep;
	int mindex_validity;
	BITVAR key;
	BITVAR old50key; //what was the old key
	BITVAR old50pos; //what was the old position
} UNDO;

#if 0
#define SPECFLAG 0x8000U
#define CHECKFLAG 0x10000U

#define PackMove(from,to,prom,spec)  ((((from)&63) + (((to)&63) << 6) + (((prom)&7) << 12))|((spec)&(SPECFLAG|CHECKFLAG)))
#define UnPackFrom(a) ((a) & 63)
#define UnPackTo(a) (((a)>>6) & 63)
#define UnPackProm(a) (((a)>>12) & 7)
#define UnPackSpec(a) (((a) & SPECFLAG))
#define UnPackCheck(a) (((a) & CHECKFLAG))
#define UnPackPPos(a) ((a) & 0x0FFF) //only from & to extracted

#else
#define SPECFLAG 0x2000000U
#define CHECKFLAG 0x1000000U

#define PackMove(from,to,prom,spec)  (unsigned int)((((unsigned int)(from)& 0x3FU) + (((unsigned int)(to)& 0x3FU) * 0x100U) + (((unsigned int)(prom)&7U) *0x10000U))|((unsigned int)(spec)&(SPECFLAG|CHECKFLAG)))
#define UnPackFrom(a) ((unsigned int)(a) & 0x3FU)
#define UnPackTo(a) (((unsigned int)(a)/0x100U) & 0x3FU)
#define UnPackProm(a) (((unsigned int)(a) / 0x10000U) & 7U)
#define UnPackSpec(a) (((unsigned int)(a) & SPECFLAG))
#define UnPackCheck(a) (((unsigned int)(a) & CHECKFLAG))
#define UnPackPPos(a) ((unsigned int)(a) & 0x3F3FU) //only from & to extracted

#endif

BITVAR isInCheck_Eval(board *b, attack_model *a, unsigned char side);
void generateCaptures(board * b, attack_model *a, move_entry ** m, int gen_u);
void generateMoves(board * b, attack_model *a, move_entry ** m);
void generateInCheckMoves(board * b, attack_model *a, move_entry ** m);
void generateQuietCheckMoves(board * b, attack_model *a, move_entry ** m);
int alternateMovGen(board * b, int *filter);
UNDO MakeMove(board *b, int move);
UNDO MakeNullMove(board *b);
void UnMakeMove(board *b, UNDO u);
void UnMakeNullMove(board *b, UNDO u);
int MoveList_Legal(board *b, attack_model *a, int  h, move_entry *n, int count, int ply, int sort);
int sortMoveList_Init(board *b, attack_model *a, int  h,move_entry *n, int count, int ply, int sort);
int sortMoveList_QInit(board *b, attack_model *a, int  h,move_entry *n, int count, int ply, int sort);
int getNSorted(move_entry *n, int total, int start, int count);
int is_quiet_move(board *, attack_model *a, move_entry *m);
int isQuietCheckMove(board * b, attack_model *a, move_entry *m);

void printfMove(board *b, int m);
void sprintfMoveSimple(int m, char *buf);
void sprintfMove(board *b, int m, char * buf);
void printBoardNice(board *b);
void printBoardEval_PSQ(board *b, attack_model *a);
void printBoardEval_MOB(board *b, attack_model *a);
int printScoreExt(attack_model *a);
void log_divider(char *s);

int copyStats(struct _statistics *source, struct _statistics *dest);

void dump_moves(board *b, move_entry * m, int count, int ply);
int compareBoardSilent(board *source, board *dest);
int copyBoard(board *source, board *dest);

int boardCheck(board *b);
void clearKillers();
int updateKillers(int depth, int move);

int pininit(void);
int pindump(void);

#endif

/*
 * globals.h
 *
 *  Created on: 31 Aug 2014
 *      Author: m
 */

#ifndef GLOBALS_H_
#define GLOBALS_H_

#include "bitmap.h"
#include "hash.h"

extern char *SQUARES_ASC[];
extern char PIECES_ASC[];

extern const int Piece_Map[6];
extern const int Square_Swap[64];

extern BITVAR rays[64][64];
extern BITVAR rays_int[64][64];

extern char ToPos [65536];
extern kmoves killers[100];

extern unsigned char ind45R[];
extern unsigned char ind45L[];
extern unsigned char ind90[];
extern unsigned char indnorm[];

extern BITVAR nnormmark[64];
extern BITVAR nmark90[64];
extern BITVAR nmark45R[64];
extern BITVAR nmark45L[64];

extern BITVAR normmark[];
extern att_mov attack;

extern unsigned int Values[];

/*
   material table index
*/

extern int MATIdxIncW[ER_PIECE*2];
extern int MATIdxIncB[ER_PIECE*2];
extern BITVAR MATincW2[ER_PIECE*2];
extern BITVAR MATincB2[ER_PIECE*2];

extern opts options;

extern int faillow, failhigh, nodecount;
extern int posBPV, tempposBPV, BPV[100], tempBPV[100], matescore;

extern int hfailhigh, hfaillow, nodeprintcount , quiescecount, quiesceoverrun, tthits, xDEBUG;
//validposcount, zerototal, zerorerun ;
//extern int posBPV, tempposBPV, BPV[100], tempBPV[100];



extern BITVAR randomTable[ER_SIDE][ER_SQUARE][ER_PIECE];
extern BITVAR sideKey;
extern BITVAR epKey[ER_SQUARE+1];
extern BITVAR castleKey[ER_SIDE][ER_CASTLE];
extern hashEntry_e * hashTable;
extern int hashValidId;

extern kmoves *killer_moves;


/*
   status of 64 squares - white/black/no piece,
   side to move - white/black,
   status of castle white - no/queen/king/both
   status of castle black - no/queen/king/both
   EP square - if opposite side pawn can be target of EP - where it was?

*/


extern int DCount;
extern int engine_stop;
extern struct _statistics STATS[TREE_STORE_DEPTH+1];


#endif /* GLOBALS_H_ */

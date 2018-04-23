#ifndef STATS_H
#define STATS_H

//#include "bitmap.h"

struct _statistics {
	unsigned long long failnorm; // node normalni
	unsigned long long faillow; // node neprekonal alfa
	unsigned long long failhigh; // node prekonal beta
	unsigned long long nodes; // mel by byt souctem positionsvisited a qposvisited, je to pro time management
	unsigned long long positionsvisited; // kolik pozic jsme navstivili? Neni to totez jako movestested?
	unsigned long long movestested; //kolik bylo opravdu testovanych
	unsigned long long possiblemoves; //kolik bylo moznych tahu
	unsigned long long zerototal;
	unsigned long long zerorerun;
	unsigned long long quiesceoverrun;
	unsigned long long qposvisited; // stejne jako non q verze
	unsigned long long qmovestested; // stejne jako non q verze
	unsigned long long qpossiblemoves; // stejne jako non q verze
	unsigned long long lmrtotal;
	unsigned long long lmrrerun;
	unsigned long long fhflcount;
	unsigned long long firstcutoffs;
	unsigned long long cutoffs;
	unsigned long long NMP_cuts;
	unsigned long long NMP_tries;
	unsigned long long qSEE_tests;
	unsigned long long qSEE_cuts;
	unsigned long long poswithmove; //num of positions for movegen ran
	unsigned long long ebfnodes; 
	unsigned long long ebfnodespri; 
	unsigned long long elaps;

// hash
	unsigned long long hashStores;
	unsigned long long hashStoreColl;
	unsigned long long hashAttempts;
	unsigned long long hashHits;
	unsigned long long hashColls;
	unsigned long long hashMiss;
	unsigned long long hashStoreMiss;
	unsigned long long hashStoreInPlace;
	unsigned long long hashStoreHits;

	int depth;
};

void clearSearchCnt(struct _statistics *);
void AddSearchCnt(struct _statistics *, struct _statistics *);
void CopySearchCnt(struct _statistics *, struct _statistics *);
void DecSearchCnt(struct _statistics *, struct _statistics *, struct _statistics *);
void printSearchStat(struct _statistics *);
void printSearchStat2(struct _statistics *, char *);
void clearALLSearchCnt(struct _statistics *);
void printALLSearchCnt(struct _statistics *);
struct _statistics * allocate_stats(int);
void deallocate_stats(struct _statistics *);

#endif

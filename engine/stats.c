#include <stdlib.h>
#include <string.h>
#include "stats.h"
#include "bitmap.h"
#include "utils.h"

void clearSearchCnt(struct _statistics * s)
{
	s->faillow=0;
	s->failhigh=0;
	s->failnorm=0;
	s->nodes=0;
// doopravdy otestovanych tahu
	s->movestested=0;
	s->qmovestested=0;
// all possible moves from visited position
	s->possiblemoves=0;
	s->qpossiblemoves=1;
// zero window run
	s->zerototal=0;
// zero window rerun/ alpha improved in zero
	s->zerorerun=0;
	s->lmrtotal=0;
	s->lmrrerun=0;
// quiesce zero rerun
	s->quiesceoverrun=0;
	s->positionsvisited=0;
	s->qposvisited=0;
	s->fhflcount=0;
	s->firstcutoffs=0;
	s->cutoffs=0;
	s->NMP_tries=0;
	s->NMP_cuts=0;
	s->qSEE_tests=0;
	s->qSEE_cuts=0;
	s->hashStores=0;
	s->hashStoreColl=0;
	s->hashAttempts=0;
	s->hashHits=0;
	s->hashColls=0;
	s->hashMiss=0;
	s->hashStoreMiss=0;
	s->hashStoreInPlace=0;
	s->hashStoreHits=0;

}

// do prvniho parametru je pricten druhy
void AddSearchCnt(struct _statistics * s, struct _statistics * b)
{
	s->faillow+=b->faillow;
	s->failhigh+=b->failhigh;
	s->failnorm+=b->failnorm;
	s->nodes+=b->nodes;
	s->movestested+=b->movestested;
	s->qmovestested+=b->qmovestested;
	s->possiblemoves+=b->possiblemoves;
	s->qpossiblemoves+=b->qpossiblemoves;
	s->zerototal+=b->zerototal;
	s->zerorerun+=b->zerorerun;
	s->lmrtotal+=b->lmrtotal;
	s->lmrrerun+=b->lmrrerun;
	s->quiesceoverrun+=b->quiesceoverrun;
	s->positionsvisited+=b->positionsvisited;
	s->qposvisited+=b->qposvisited;
	s->fhflcount+=b->fhflcount;
	s->firstcutoffs+=b->firstcutoffs;
	s->cutoffs+=b->cutoffs;
	s->NMP_tries+=b->NMP_tries;
	s->NMP_cuts+=b->NMP_cuts;
	s->qSEE_tests+=b->qSEE_tests;
	s->qSEE_cuts+=b->qSEE_cuts;
	s->hashStores+=b->hashStores;
	s->hashStoreColl+=b->hashStoreColl;
	s->hashAttempts+=b->hashAttempts;
	s->hashHits+=b->hashHits;
	s->hashColls+=b->hashColls;
	s->hashMiss+=b->hashMiss;
	s->hashStoreMiss+=b->hashStoreMiss;
	s->hashStoreInPlace+=b->hashStoreInPlace;
	s->hashStoreHits+=b->hashStoreHits;

}

// do prvniho parametru je skopirovan druhy
void CopySearchCnt(struct _statistics * s, struct _statistics * b)
{
	s->faillow=b->faillow;
	s->failhigh=b->failhigh;
	s->failnorm=b->failnorm;
	s->nodes=b->nodes;
	s->movestested=b->movestested;
	s->qmovestested=b->qmovestested;
	s->possiblemoves=b->possiblemoves;
	s->qpossiblemoves=b->qpossiblemoves;
	s->zerototal=b->zerototal;
	s->zerorerun=b->zerorerun;
	s->lmrtotal=b->lmrtotal;
	s->lmrrerun=b->lmrrerun;
	s->quiesceoverrun=b->quiesceoverrun;
	s->positionsvisited=b->positionsvisited;
	s->qposvisited=b->qposvisited;
	s->fhflcount=b->fhflcount;
	s->firstcutoffs=b->firstcutoffs;
	s->cutoffs=b->cutoffs;
	s->NMP_tries=b->NMP_tries;
	s->NMP_cuts=b->NMP_cuts;
	s->qSEE_tests=b->qSEE_tests;
	s->qSEE_cuts=b->qSEE_cuts;
	s->hashStores=b->hashStores;
	s->hashStoreColl=b->hashStoreColl;
	s->hashAttempts=b->hashAttempts;
	s->hashHits=b->hashHits;
	s->hashColls=b->hashColls;
	s->hashMiss=b->hashMiss;
	s->hashStoreMiss=b->hashStoreMiss;
	s->hashStoreInPlace=b->hashStoreInPlace;
	s->hashStoreHits=b->hashStoreHits;
}

// od prvniho je odecten druhy a vlozen do tretiho
void DecSearchCnt(struct _statistics * s, struct _statistics * b, struct _statistics * r)
{
	r->faillow=	s->faillow-b->faillow;
	r->failhigh= s->failhigh-b->failhigh;
	r->failnorm= s->failnorm-b->failnorm;
	r->nodes= s->nodes-b->nodes;
	r->movestested= s->movestested-b->movestested;
	r->qmovestested= s->qmovestested-b->qmovestested;
	r->possiblemoves= s->possiblemoves-b->possiblemoves;
	r->qpossiblemoves= s->qpossiblemoves-b->qpossiblemoves;
	r->zerototal= s->zerototal-b->zerototal;
	r->zerorerun= s->zerorerun-b->zerorerun;
	r->lmrtotal= s->lmrtotal-b->lmrtotal;
	r->lmrrerun= s->lmrrerun-b->lmrrerun;
	r->quiesceoverrun= s->quiesceoverrun-b->quiesceoverrun;
	r->positionsvisited= s->positionsvisited-b->positionsvisited;
	r->qposvisited= s->qposvisited-b->qposvisited;
	r->fhflcount= s->fhflcount-b->fhflcount;
	r->firstcutoffs= s->firstcutoffs-b->firstcutoffs;
	r->cutoffs= s->cutoffs-b->cutoffs;
	r->NMP_tries= s->NMP_tries-b->NMP_tries;
	r->NMP_cuts= s->NMP_cuts-b->NMP_cuts;
	r->qSEE_tests=s->qSEE_tests-b->qSEE_tests;
	r->qSEE_cuts=s->qSEE_cuts-b->qSEE_cuts;
	r->hashStores=s->hashStores-b->hashStores;
	r->hashStoreColl=s->hashStoreColl-b->hashStoreColl;
	r->hashAttempts=s->hashAttempts-b->hashAttempts;
	r->hashHits=s->hashHits-b->hashHits;
	r->hashColls=s->hashColls-b->hashColls;
	r->hashMiss=s->hashMiss-b->hashMiss;
	r->hashStoreMiss=s->hashStoreMiss-b->hashStoreMiss;
	r->hashStoreInPlace=s->hashStoreInPlace-b->hashStoreInPlace;
	r->hashStoreHits=s->hashStoreHits-b->hashStoreHits;
}

void printSearchStat(struct _statistics *s)
{
	LOGGER_0("Info: Low %lld, High %lld, Normal %lld, Positions %lld, MovesSearched %lld (%lld%%) of %lld TotalMovesAvail. Branching %lld, %lld\n", s->faillow, s->failhigh, s->failnorm, s->positionsvisited, s->movestested, (s->movestested*100/(s->possiblemoves+1)), s->possiblemoves, (s->movestested/(s->positionsvisited+1)), (s->possiblemoves/(s->positionsvisited+1)));
	LOGGER_0("HASH: Get:%lld, GHit:%lld,%%%lld, GMiss:%lld, GCol: %lld\n", s->hashAttempts, s->hashHits, s->hashHits*100/(s->hashAttempts+1), s->hashMiss, s->hashColls);
	LOGGER_0("HASH: Stores:%lld, SHit:%lld, SInPlace:%lld, SMiss:%lld SCCol:%lld\n",s->hashStores, s->hashStoreHits, s->hashStoreInPlace, s->hashStoreMiss, s->hashColls);
	LOGGER_0("Info: QPositions %lld, QMovesSearched %lld,(%lld%%) of %lld QTotalMovesAvail\n", s->qposvisited, s->qmovestested, (s->qmovestested*100/(s->qpossiblemoves+1)), s->qpossiblemoves);
	LOGGER_0("Info: ZeroN %lld, ZeroRerun %lld, QZoverRun %lld, LmrN %lld, LmrRerun %lld, FhFlCount: %lld\n", s->zerototal, s->zerorerun, s->quiesceoverrun, s->lmrtotal, s->lmrrerun, s->fhflcount);
	LOGGER_0("Info: Cutoffs: First move %lld, Any move %lld, Ratio of first %lld%%\n",s->firstcutoffs, s->cutoffs,100*s->firstcutoffs/(s->cutoffs+1));
	LOGGER_0("Info: QuiesceSEE: Tests %lld, Cuts %lld, Ratio %lld%%\n",s->qSEE_tests, s->qSEE_cuts,100*s->qSEE_cuts/(s->qSEE_tests+1));
	LOGGER_0("Info: NULL MOVE: Tries %lld, Cuts %lld, Ratio %lld%%\n",s->NMP_tries, s->NMP_cuts,100*s->NMP_cuts/(s->NMP_tries+1));
}

void printSearchStat2(struct _statistics *s, char *buff)
{
char bb[1024];
	sprintf(buff, "Low %lld, High %lld, Normal %lld, Positions %lld, MovesSearched %lld (%lld%%) of %lld TotalMovesAvail. Branching %lld, %lld\n", s->faillow, s->failhigh, s->failnorm, s->positionsvisited, s->movestested, (s->movestested*100/(s->possiblemoves+1)), s->possiblemoves, (s->movestested/(s->positionsvisited+1)), (s->possiblemoves/(s->positionsvisited+1)));
	strcat(buff,bb);
	sprintf(bb, "Get:%lld, GHit:%lld,%%%lld, GMiss:%lld, GCol: %lld\n", s->hashAttempts, s->hashHits, s->hashHits*100/(s->hashAttempts+1), s->hashMiss, s->hashColls);
	strcat(buff,bb);
	sprintf(bb, "Stores:%lld, SHit:%lld, SInPlace:%lld, SMiss:%lld SCCol:%lld\n",s->hashStores, s->hashStoreHits, s->hashStoreInPlace, s->hashStoreMiss, s->hashColls);
	strcat(buff,bb);
	sprintf(bb, "QPositions %lld, QMovesSearched %lld,(%lld%%) of %lld QTotalMovesAvail\n", s->qposvisited, s->qmovestested, (s->qmovestested*100/(s->qpossiblemoves+1)), s->qpossiblemoves);
	strcat(buff,bb);
	sprintf(bb, "ZeroN %lld, ZeroRerun %lld, QZoverRun %lld, LmrN %lld, LmrRerun %lld, FhFlCount: %lld\n", s->zerototal, s->zerorerun, s->quiesceoverrun, s->lmrtotal, s->lmrrerun, s->fhflcount);
	strcat(buff,bb);
	sprintf(bb, "Cutoffs: First move %lld, Any move %lld, Ratio of first %lld%%, \n",s->firstcutoffs, s->cutoffs,100*s->firstcutoffs/(s->cutoffs+1));
	strcat(buff,bb);
	sprintf(buff, "QuiesceSEE: Tests %lld, Cuts %lld, Ratio %lld%%, \n",s->qSEE_tests, s->qSEE_cuts,100*s->qSEE_cuts/(s->qSEE_tests+1));
	strcat(buff,bb);
	sprintf(bb, "NULL MOVE: Tries %lld, Cuts %lld, Ratio %lld%%, \n",s->NMP_tries, s->NMP_cuts,100*s->NMP_cuts/(s->NMP_tries+1));
	strcat(buff,bb);
}

void clearALLSearchCnt(struct _statistics * s) {
int f;
	for(f=MAXPLY;f>=0;f--) {
		clearSearchCnt(&(s[f]));
	}
}

void printALLSearchCnt(struct _statistics * s) {
int f;
char buff[1024];
	for(f=0;f<=30+1;f++) {
		sprintf(buff, "Level %d", f);
		LOGGER_1("Stats: %s\n",buff);
		printSearchStat(&(s[f]));
	}
	LOGGER_1("Stats: Konec\n");
}

struct _statistics * allocate_stats(int count)
{
struct _statistics *s;
	s=malloc(sizeof(struct _statistics)*count);
	return s;
}

void deallocate_stats(struct _statistics *s)
{
	free(s);
}


#include "utils.h"

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <time.h>
#include "bitmap.h"
#include "evaluate.h"
//#include <linux/time.h>

FILE * debugf;

int logger(char *p, char *s,char *a){
	int hh, mm, ss, nn;
	unsigned long long  en;
	en=readClock();

	nn=en%1000;

	en=en/1000;
	ss=en%60;

	en=en/60;
	mm=en%60;

	en=en/60;
	hh=en%24;

	fprintf(debugf, "%02d:%02d:%02d:%04d  %s%s%s",hh, mm, ss, nn, p, s, a);
//	fprintf(debugf, "%s%s%s", p, s, a);
	return 0;
}

int open_log(char *filename){
	debugf=fopen(filename, "w+");
	setvbuf(debugf, NULL, _IONBF, 16384);
	return 0;
}

int close_log(void){
	fclose(debugf);
	return 0;
}

char * tokenizer(char *str, char *delim, char **next){
	char *s, *t;
	int i;
//	logger("TokBu:", str, ":uETok\n");
	i=strspn(str, delim);
	s=str+i;
	t=strpbrk(s, delim);
	if(t!=NULL){
		*t=0x0;
		t++;
	}
	if(*s==0x0) s=NULL;
	*next=t;
//	logger("TokBP:", str, ":PETok\n");
	return s;
}

int indexer(char *str, char *delim, char **index){
	char *tok, *b2;
	int i;

	i=0;
	index[i]=NULL;
	tok = tokenizer(str,delim,&b2);
	while(tok){
		index[i]=tok;
		i++;
		index[i]=NULL;
		tok = tokenizer(b2,delim, &b2);
	}
	return i;
}

int indexof(char **index, char *str){
int i;
	i=0;
	while(index[i]!=NULL) {
		if(!strcmp(str, index[i])){
			return i;
		}
		i++;
	}
	return -1;
}

// clock in miliseconds
unsigned long long int readClock(void)
{
	struct timespec t;
	clock_gettime( CLOCK_REALTIME, &t);

	return (t.tv_sec*1000 +t.tv_nsec/1000000);
}

// returns diff in times in microseconds
unsigned long long diffClock(struct timespec start, struct timespec end)
{
	struct timespec temp;
	temp.tv_sec = end.tv_sec - start.tv_sec;
	if(end.tv_nsec<start.tv_nsec) {
		temp.tv_nsec= 1000000000+end.tv_nsec-start.tv_nsec;
		temp.tv_sec--;
	} else temp.tv_nsec=end.tv_nsec-start.tv_nsec;
	return temp.tv_sec*1000000+temp.tv_nsec/1000;
}

int readClock_wall(struct timespec *t)
{
	clock_gettime( CLOCK_MONOTONIC, t);
	return 0;
}

int readClock_proc(struct timespec *t)
{
	clock_gettime( CLOCK_PROCESS_CPUTIME_ID, t);
	return 0;
}

int generate_log_name(char *n, char *pref, char *b) {

time_t rawtime;
struct tm *info;
char buffer[80];

	   time( &rawtime );
	   info = localtime( &rawtime );

	   strftime(buffer,80,"%y%m%d-%H%M%S", info);
	   sprintf(b, "%s-%s%s.txt", n, buffer, pref);
	return 0;
}

int parse_cmd_line_check_sec(int argc, char *argv[]){
int i;
	for (i=1; i< argc; i++) {
		if(!strcmp(argv[i], "-second")) return 1;
	 }
	return 0;
}

int computeMATIdx(board *b){
int idx;
int bwl, bwd, bbl, bbd, pw, pb, nw, nb, rw, rb, qw, qb;

	bwd=BitCount((b->maps[BISHOP]) & (b->colormaps[WHITE]) & BLACKBITMAP);
	bbd=BitCount((b->maps[BISHOP]) & (b->colormaps[BLACK]) & BLACKBITMAP);
	bwl=BitCount((b->maps[BISHOP]) & (b->colormaps[WHITE]))-bwd;
	bbl=BitCount((b->maps[BISHOP]) & (b->colormaps[BLACK]))-bbd;
	pw=BitCount((b->maps[PAWN]) & (b->colormaps[WHITE]));
	pb=BitCount((b->maps[PAWN]) & (b->colormaps[BLACK]));
	nw=BitCount((b->maps[KNIGHT]) & (b->colormaps[WHITE]));
	nb=BitCount((b->maps[KNIGHT]) & (b->colormaps[BLACK]));
	rw=BitCount((b->maps[ROOK]) & (b->colormaps[WHITE]));
	rb=BitCount((b->maps[ROOK]) & (b->colormaps[BLACK]));
	qw=BitCount((b->maps[QUEEN]) & (b->colormaps[WHITE]));
	qb=BitCount((b->maps[QUEEN]) & (b->colormaps[BLACK]));
	idx=MATidx(pw,pb,nw,nb,bwl,bwd,bbl,bbd,rw,rb,qw,qb);

	return idx;
}

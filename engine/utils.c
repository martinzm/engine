

#include "utils.h"

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <time.h>
#include <stdarg.h>
#include "bitmap.h"
#include "evaluate.h"
#include "movgen.h"
#include "generate.h"
#include "ui.h"
#include <iconv.h>

FILE * debugf;
debugEntry DBOARDS[DBOARDS_LEN+1];
_dpaths DPATHS[DPATHSwidth+1];

int initDBoards(debugEntry *d)
{
board b;
int f;
char *boards[]={
		"1k5R/8/1K6/8/8/8/8/8 b - - 23 12" ,
		"3k4/R7/8/8/8/8/8/4K3 w - - 2 2",
		NULL };
	f=0;
	while(boards[f]!=NULL) {
		setup_FEN_board(&b, boards[f]);
		d[f].key=b.key;
		d[f].map=b.norm;
		f++;
	}
	d[f].map=0;
	return 0;
}

int initDPATHS(board *b, _dpaths *DP)
{
int i,f,n;
char str[512];
MOVESTORE *q;
char *paths[] = {
		"f7e6",
		NULL };
	f=n=0;
	while(paths[f]!=NULL) {
// prvni integer v kazdem radku DPATHS udava skutecne ulozenou delku

	strcpy(str, paths[f]);
	q=&(DP[f][1]);
	DP[n][0]=(MOVESTORE) (move_filter_build(str,q)-1);
	if(validatePATHS(b, (DP[n]))!=1) DP[n][0]=0; else n++;
	f++;
	}
	DP[n][0]=0;
return 0;
}

int validatePATHS(board *b, MOVESTORE *m) {
UNDO u[256];
MOVESTORE mm[2];
int f,i, r;

attack_model att[1];

	mm[1]=0;
	r=1;
	for(f=1;f<=m[0];f++) {
		mm[0]=m[f];
		eval_king_checks_all(b, &att[0]);
		eval(b, &att[0], b->pers);
		i=alternateMovGen(b, mm);
		if(i!=1) {
			LOGGER_2("INFO3:","move problem!\n","");
			r=0;
			break;
		}
		m[f]=mm[0];
		u[f]=MakeMove(b, mm[0]);
	}
	for(f--;f>0;f--) {
	 UnMakeMove(b, u[f]);
	}
	return r;
}

int compareDBoards(board *b, debugEntry *h)
{
int i;
	i=0;
	while(h[i].map!=0) {
		if((b->key==h[i].key)&&(b->norm==h[i].map)) {
			return 1;
		}
		i++;
	}
	return 0;
}

int compareDPaths(tree_store *tree, _dpaths *dp, int plylen){
int r,i,f,filt, move, p1, p2,e;
char b2[512], buff[512];
	r=i=0;
	while((dp[i][0]!=0)) {
		if((plylen+1)<dp[i][0]) {
			i++;
			continue;
		}
		e=(plylen+1)< dp[i][0] ? plylen+1 : dp[i][0];
		for(f=1;f<=e;f++) {
// compare move
			filt=UnPackPPos(dp[i][f]);
			move=UnPackPPos(tree->tree[f-1][f-1].move);
			if(filt!=move) break;
			p1=UnPackProm(dp[i][f]);
			p2=UnPackProm(tree->tree[f-1][f-1].move);
			if(p1!=p2) break;
			r=1;
		}
		if(r==1) {
			sprintf(buff, "HIT! ");
			for(f=0;f<=plylen;f++) {
//				printBoardNice(&(tree->tree[f][f].tree_board));
				sprintfMoveSimple(tree->tree[f][f].move, b2);
				strcat(buff, b2);
				strcat(buff," ");
			}
			strcat(buff,"\n");
			printf("%s",buff);
			return 1;
		}
		i++;
	}
	return 0;
}

int logger2(char *fmt, ...) {
char buf[512];
int n;
int hh, mm, ss, nn;
unsigned long long  en;
va_list ap;

	en=readClock();
	nn=(int)(en%1000);
	en=en/1000;
	ss=(int)(en%60);
	en=en/60;
	mm=(int)(en%60);
	en=en/60;
	hh=(int)(en%24);

	va_start(ap, fmt);
	n = vsnprintf(buf, 512, fmt, ap);
	va_end(ap);
	fprintf(debugf, "%02d:%02d:%02d:%04d  %s",hh, mm, ss, nn, buf);
return 0;
}

int nlogger2(char *fmt, ...) {
char buf[512];
int n;
va_list ap;

	va_start(ap, fmt);
	n = vsnprintf(buf, 512, fmt, ap);
	va_end(ap);
	fprintf(debugf, "%s",buf);
return 0;
}

int logger(char *p, char *s,char *a){
	int hh, mm, ss, nn;
	unsigned long long  en;
	en=readClock();

	nn=(int)(en%1000);

	en=en/1000;
	ss=(int)(en%60);

	en=en/60;
	mm=(int)(en%60);

	en=en/60;
	hh=(int)(en%24);

	fprintf(debugf, "%02d:%02d:%02d:%04d  %s%s%s",hh, mm, ss, nn, p, s, a);
//	fprintf(debugf, "%s%s%s", p, s, a);
	return 0;
}

int open_log(char *filename){
	debugf=fopen(filename, "w+");
	setvbuf(debugf, NULL, _IOFBF, 16384);
	return 0;
}

int close_log(void){
	fclose(debugf);
	return 0;
}

char * tokenizer(char *str, char *delim, char **next){
	char *s, *t;
	size_t i;
//	logger("TokBu:", str, ":uETok\n");
	if(str!=NULL) {
		i=strspn(str, delim);
		s=str+i;
		t=strpbrk(s, delim);
		if(t!=NULL){
			*t=0x0;
			t++;
		}
		if(*s==0x0) s=NULL;
		*next=t;
	} else {
		s=NULL;
		*next=NULL;
	}
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

	return ((unsigned long long)t.tv_sec*1000 +(unsigned long long) t.tv_nsec/1000000);
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
	return (unsigned long long)temp.tv_sec*1000000+(unsigned long long) temp.tv_nsec/1000;
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
struct timespec ti;
char buffer[80];
int r;

	   time( &rawtime );
	   info = localtime( &rawtime );
	   readClock_wall(&ti);
	   r=(int)((ti.tv_nsec)/100)%0x8000;
	   strftime(buffer,80,"%y%m%d-%H%M%S", info);
	   sprintf(b, "%s-%s%s-%X.txt", n, buffer, pref,r);
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
//	printf("MatCounts: %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d\n", pw,pb,nw,nb,bwl,bwd,bbl,bbd,rw,rb,qw,qb);
	idx=MATidx(pw,pb,nw,nb,bwl,bwd,bbl,bbd,rw,rb,qw,qb);

	return idx;
}

int UTF8toWchar(unsigned char *in, wchar_t *out)
{
iconv_t cd;
size_t il;
size_t ol=1024;
size_t ret;
char * ooo;
	ooo=(char *) out;
//	printf("UUU:%ls\n", (wchar_t*)in);
	cd = iconv_open("WCHAR_T", "UTF-8");
	if ((iconv_t) -1 == cd) {
//		perror("iconv_open");
		return -1;
	}
	ret = iconv(cd, NULL, NULL, &ooo, &ol);

	il=strlen((const char *)in)+1;
	ret = iconv(cd,(char **) &in, &il, &ooo, &ol);
	iconv_close(cd);
	*ooo=L'\0';
	if ((size_t) -1 == ret) {
//		perror("iconv");
//		printf("iconv problem\n");
		return -2;
	}
	return 0;
}

int WchartoUTF8(wchar_t *in, unsigned char *out)
{
iconv_t cd;
size_t il;
size_t ol=1024;
size_t ret;
char * ooo;
	ooo=(char *) out;
	cd = iconv_open("UTF-8", "WCHAR_T");
	if ((iconv_t) -1 == cd) {
		perror("iconv_open");
		return -1;
	}
	il=wcslen(in)*4;
	ret = iconv(cd, (char **)&in, &il, &ooo, &ol);
	iconv_close(cd);
	*ooo=L'\0';
	if ((size_t) -1 == ret) {
		perror("iconv");
		return -2;
	}
return 0;
}

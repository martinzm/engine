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
#include "tests.h"
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

char *perft_default_tests[]={"8/k1P5/8/1K6/8/8/8/8 w - - 0 1 perft 7 = 567584 ; id X stalemate/checkmate;",
							"r4rk1/pp2ppNp/2pp2p1/6q1/8/P2nPPK1/1P1P2PP/R1B3NR w - - 0 1 perft 1 = 1; jumping over pieces under check;",
							"8/8/1P2K3/8/2n5/1q6/8/5k2 b - - 0 1 perft 5 = 1004658 ; id  X discovered check;",
							"5K2/8/1Q6/2N5/8/1p2k3/8/8 w - - 0 1 perft 5 = 1004658 ; id  X discovered check;",
							"r3k2r/1b4bq/8/8/8/8/7B/R3K2R w KQkq - 0 1 perft 4 = 1274206 ; id  X castling (including losing cr due to rook capture);",
							"r3k2r/7b/8/8/8/8/1B4BQ/R3K2R b KQkq - 0 1 perft 4 = 1274206 ; id  X castling (including losing cr due to rook capture);",
							"2K2r2/4P3/8/8/8/8/8/3k4 w - - 0 1 perft 6 = 3821001 ; id X promote out of check;",
							"3K4/8/8/8/8/8/4p3/2k2R2 b - - 0 1 perft 6 = 3821001 ; id X promote out of check;",
							"8/5bk1/8/2Pp4/8/1K6/8/8 w - d6 0 1 perft 6 = 824064 ; id X avoid illegal en passant capture;",
							"8/8/1k6/8/2pP4/8/5BK1/8 b - d3 0 1 perft 6 = 824064 ; id X avoid illegal en passant capture;",
							"8/8/1k6/2b5/2pP4/8/5K2/8 b - d3 0 1 perft 6 = 1440467 ; id X en passant capture checks opponent;",
							"8/5k2/8/2Pp4/2B5/1K6/8/8 w - d6 0 1 perft 6 = 1440467 ; id X en passant capture checks opponent;",
							"8/p7/8/1P6/K1k3p1/6P1/7P/8 w - - 0 1 perft 5 = 14062 ; id X numpty 1 l5;",
							"K1k5/8/P7/8/8/8/8/8 w - - 0 1 perft 6 = 2217 ;  id X self stalemate;",
							"8/8/8/8/8/p7/8/k1K5 b - - 0 1 perft 6 = 2217 ;  id X self stalemate;",
							"8/P1k5/K7/8/8/8/8/8 w - - 0 1 perft 6 = 92683 ; id X underpromote to check;",
							"8/8/8/8/8/k7/p1K5/8 b - - 0 1 perft 6 = 92683 ; id X underpromote to check;",
							"4k3/1P6/8/8/8/8/K7/8 w - - 0 1 perft 6 = 217342 ; id X promote to give check;",
							"8/k7/8/8/8/8/1p6/4K3 b - - 0 1 perft 6 = 217342 ; id X promote to give check;",
							"8/8/8/8/1k6/8/K1p5/8 b - - 0 1 perft 7 = 567584 ; id X stalemate/checkmate;",
							"5k2/8/8/8/8/8/8/4K2R w K - 0 1 perft 6 = 661072 ; id  X short castling gives check;",
							"4k2r/8/8/8/8/8/8/5K2 b k - 0 1 perft 6 = 661072 ; id  X short castling gives check;",
							"3k4/8/8/8/8/8/8/R3K3 w Q - 0 1 perft 6 = 803711 ; id  X long castling gives check;",
							"r3k3/8/8/8/8/8/8/3K4 b q - 0 1 perft 6 = 803711 ; id  X long castling gives check;",
							"r3k2r/8/3Q4/8/8/5q2/8/R3K2R b KQkq - 0 1 perft 4 = 1720476 ; id  X castling prevented;",
							"r3k2r/8/5Q2/8/8/3q4/8/R3K2R w KQkq - 0 1 perft 4 = 1720476 ; id  X castling prevented;",
							"8/8/2k5/5q2/5n2/8/5K2/8 b - - 0 1 perft 4 = 23527 ; id  X double check;",
							"8/5k2/8/5N2/5Q2/2K5/8/8 w - - 0 1 perft 4 = 23527 ; id  X double check;",
							NULL };

char *perft_default_tests2[]={
		"r3k2r/1b4bq/8/8/8/8/7B/R3K2R w KQkq - 0 1 perft 4 = 1274206 ; id  X castling (including losing cr due to rook capture);",
							NULL };

char *remis_default_tests[]={"3k4/4R1R1/8/8/8/K5pp/6r1/7q w - - 0 1",
							 "qq3rk1/ppp1p2p/3p2p1/8/8/3Q4/2Q3PK/8 w - - 0 1",
							 "5k2/8/5P2/5K2/8/8/8/8 w - - 2 3",
							 "4k3/7p/8/8/8/8/6R1/R3K3 w - - 98 55",
							NULL };

char *key_default_tests[]={"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 key = 463b96181691fc9c;",
		"rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1 key = 823c9b50fd114196;",
		"rnbqkbnr/ppp1pppp/8/3p4/4P3/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 2 key = 0756b94461c50fb0;",
		"rnbqkbnr/ppp1pppp/8/3pP3/8/8/PPPP1PPP/RNBQKBNR b KQkq - 0 2 key = 662fafb965db29d4;",
		"rnbqkbnr/ppp1p1pp/8/3pPp2/8/8/PPPP1PPP/RNBQKBNR w KQkq f6 0 3 key = 22a48b5a8e47ff78;",
		"rnbqkbnr/ppp1p1pp/8/3pPp2/8/8/PPPPKPPP/RNBQ1BNR b kq - 0 3 key = 652a607ca3f242c1;",
		"rnbq1bnr/ppp1pkpp/8/3pPp2/8/8/PPPPKPPP/RNBQ1BNR w - - 0 4 key = 00fdd303c946bdd9;",
		"rnbqkbnr/p1pppppp/8/8/PpP4P/8/1P1PPPP1/RNBQKBNR b KQkq c3 0 3 key = 3c8123ea7b067637;",
		"rnbqkbnr/p1pppppp/8/8/P6P/R1p5/1P1PPPP1/1NBQKBNR b Kkq - 0 4 key = 5c3f9b829b279560;", NULL
};


char *timed_default_tests[]={ "8/4PK2/3k4/8/8/8/8/8 w - - 2 13 bm e8=Q; \"xxx\";",
		"8/4k3/8/8/4PK2/8/8/8 w - - 0 1 bm e4; \"test E1x\";",
								NULL
};
attack_model aa[50];

board TESTBOARD;

int triggerBoard(){
	printf("trigger!\n");
return 0;
}

int compareBoard(board *source, board *dest){
int i;
char a1[100];
char a2[100];
char a3[100];
int r;

	if(dest->ep!=source->ep) { printf("EP!\n"); triggerBoard(); }
	if(dest->gamestage!=source->gamestage) { printf("GAMESTAGE!\n"); triggerBoard(); }
	if(dest->key!=source->key) { printf("KEY!\n"); triggerBoard(); }
	if(dest->move!=source->move) { printf("MOVE!\n"); triggerBoard(); }
	if(dest->norm!=source->norm) { printf("NORM!\n"); triggerBoard(); }
	if(dest->r45L!=source->r45L) { printf("r45L!\n"); triggerBoard(); }
	if(dest->r45R!=source->r45R) { printf("r45R!\n"); triggerBoard(); }
	if(dest->r90R!=source->r90R) { printf("r90R!\n"); triggerBoard(); }
	if(dest->rule50move!=source->rule50move) { printf("rule50move!\n"); triggerBoard(); }
	if(dest->side!=source->side) { printf("side!\n"); triggerBoard(); }

	for(i=WHITE;i<ER_SIDE;i++) if(dest->castle[i]!=source->castle[i]) { printf("CASTLE %d!\n",i); triggerBoard(); }
	for(i=0;i<2;i++) if(dest->colormaps[i]!=source->colormaps[i]) { printf("COLORMAPS %d!\n",i); triggerBoard(); }
	for(i=0;i<2;i++) if(dest->king[i]!=source->king[i]) { printf("KING %d!\n",i); triggerBoard(); }
	for(i=0;i<6;i++) if(dest->maps[i]!=source->maps[i]) { printf("MAPS %d!\n",i); triggerBoard(); }
//	for(i=0;i<2;i++) if(dest->mcount[i]!=source->mcount[i]) { printf("MCOUNT %d!\n",i); triggerBoard(); }
	for(i=0;i<64;i++) if(dest->pieces[i]!=source->pieces[i]) { printf("PIECES %d!\n",i); triggerBoard(); }
	for(i=0;i<MAXPLY;i++) if(dest->positions[i]!=source->positions[i]) { printf("POSITIONS %d!\n",i); triggerBoard(); }
	for(i=0;i<MAXPLY;i++) if(dest->posnorm[i]!=source->posnorm[i]) { printf("POSNORM %d!\n",i); triggerBoard(); }
	for(i=0;i<ER_PIECE;i++) if(dest->material[WHITE][i]!=source->material[WHITE][i]) { printf("MATERIAL WHITE %d!\n",i); triggerBoard(); }
	for(i=0;i<ER_PIECE;i++) if(dest->material[BLACK][i]!=source->material[BLACK][i]) { printf("MATERIAL BLACK %d!\n",i); triggerBoard(); }
	if(dest->mindex!=source->mindex) { 
		printf("MIndex!\n"); 
		r=source->mindex-dest->mindex;
		outbinary((BITVAR)source->mindex, a1);
		outbinary((BITVAR)dest->mindex, a2);
		outbinary((BITVAR)r, a3);
		printf("s: %s\nd: %s\nr: %s\n", a1, a2, a3);
		triggerBoard(); 
	}
return 0;
}

/*
 * b contains setup board
 * simple loop
 *
 * - generateMoves
 * 	- store board status
 * 	- make move
 * 	- unmake move
 * 	- check board with stored
 */

int moveGenTest1(board *z){
board *b, work;
UNDO u;
move_entry move[300], *m, *n;
int tc, cc, hashmove;
attack_model att;

	b=&work;
	b->stats=allocate_stats(1);
	b->hs=allocateHashStore(HASHSIZE, 2048);
	b->hps=allocateHashPawnStore(HASHPAWNSIZE);
	b->hht=allocateHHTable();
	b->kmove=allocateKillerStore();

	copyBoard(z,b);

	m=move;
	att.phase=eval_phase(b, b->pers);
	eval(b, &att, b->pers);
	if(isInCheck(b, b->side)!=0) {
		generateInCheckMoves(b, &att, &m);
	} else {
		generateCaptures(b, &att, &m, 1);
		generateMoves(b, &att, &m);
	}

	n=move;
	hashmove=DRAW_M;
	tc=sortMoveList_Init(b, &att, hashmove, move, (int)(m-n), 1, (int)(m-n) );

	cc = 0;
	while (cc<tc) {
		u=MakeMove(b, move[cc].move);
		if(computeMATIdx(b)!=b->mindex) {
			printf("matindex problem!\n");
			triggerBoard();
		}
		UnMakeMove(b, u);
		compareBoard(z, b);
		cc++;
	}
	freeKillerStore(b->kmove);
	freeHHTable(b->hht);
	freeHashPawnStore(b->hps);
	freeHashStore(b->hs);
	deallocate_stats(b->stats);
return 0;
}



#define CMTLEN 256

int getEPDmoves(char *bu, char (*m)[CMTLEN], int len)
{
char sep1[]=" \t,\"";
char *tks1, *tok1;
char buf[512];
int i;
	for(i=0;i<len;i++) (m)[i][0]='\0';
	i=0;
	strncpy(buf, bu, 511);
	tok1=strtok_r(buf, sep1, &tks1);
	while((tok1!=NULL)&&(i<len)) {
		if(strlen(tok1)>0) {
			strncpy(*m, tok1, CMTLEN-1);
			m++;
			i++;
		}
		tok1=strtok_r(NULL, sep1, &tks1);
	}
return i;
}

/*
 * tahy jsou oddeleny mezerou
 * moznosti (whitespace)*(Alnum)+(whitespace)*
			(whitespace)*"(cokoliv krome ")+"(whitespace)*
 */

int getEPDmovesO(char *bu, char (*m)[CMTLEN], int len)
{
	printf("%s\n", bu);
	
	int i, c;
	size_t ap;
	size_t x;
	char *b2;
	(*m)[0]='\0';
	if(bu==NULL) return 0;
	i=0;
	for(i=0;i<len;i++) (m)[i][0]='\0';
	x=strlen(bu);
	while((x>0)&&(i<len)) {
		while(isspace(*bu)&&(x>0)) {
			bu++;
			x--;
		}
		if(x>0) {
			if(*bu=='\"') {
				bu++;
				x--;
				b2=strstr(bu, "\"");
				if(b2!=NULL) ap=(size_t)(b2-bu); else ap=(size_t)x;
				if(ap>0) {
					printf("%s\n", *m);
					strncpy(*m, bu, ap);
					m++;
					i++;
					x-=ap;
					bu+=ap;
				}
			} else {
				c=0;
				while((!isspace(*bu))&&(x>0)) {
					(*m)[c++]=*bu;
					bu++;
					x--;
				}
				if((*m)[c]==',') c--;
				(*m)[c++]='\0';
				printf("%s\n", *m);
				m++;
				i++;
			}
		}
	}
	return i;
}

int getEPD_str(char *b, char *w,  char *f)
{
char *z1, *z2, *z3, *z4, *zz;
int i1;
	i1=0;
	zz=b;
	while(i1==0) {
		z1=strstr(zz, w); if(z1==NULL) return 0;
		z2=strstr(zz, ";"); if(z2==NULL) z2=z1+strlen(z1);
		z3=strstr(zz, "\""); if(z3==NULL) z3=z1+strlen(z1);
		if((z2<z1) && (z2<z3)) {
			zz=z2+1;
			continue;
		}
		if(z3<z1) {
			z4=strstr(z3+1, "\"");
			if(z4==NULL) return 0;
			zz=z4+1;
			if(zz==NULL) return 0;
			continue;
		}
		i1=1;
	}

	zz=z1+1;
	z1+=strlen(w);

	z2=strstr(zz, ";"); if(z2==NULL) z2=z1+strlen(z1);
	z3=strstr(zz, "\""); if(z3==NULL) z3=z1+strlen(z1);
	if(z3<z2) {
		z4=strstr(z3+1, "\"");
		if(z4!=NULL) {
			zz=z4+1;
			z1=z3+1;
			z2=z4;
		} else {
			z2=z1+strlen(z1);
		}
	}
	while(z2>=z1) {
		z2--;
		if(!isspace(*z2)) break;
	}
	zz=z2+1;
	while(z2>=z1) {
		if(!isspace(*z1)) break;
		z1++;
	}
	zz=z2+1;
	//		zz++;

	strncpy(f, z1, (size_t)(zz-z1));
	f[zz-z1]='\0';
return 1;
}

int get_token(char *st,int first, char *del, int *b, int *e) {
int  f, i, ret;
	size_t dl,l;
	ret=0;
	dl=strlen(del);
	l=strlen(st);
	f=first;
// skip mezery
	while((f<l)&&(isspace(st[f]))) {
		f++;
	}
	*e=*b=f;

	while((f<l)&&(ret<=0)) {
		for(i=0; i<dl; i++) {
			if(st[f]==del[i]) {
				*e=f;
				ret=(*e)-(*b);
				break;
			}
		}
		f++;
	}
	if((f==l)) {
		*e=f;
		ret=(*e)-(*b);
	}
	return ret;
}

/*
 * FEN: position activecolor castlingAvail ENSquare halfmoveclock fullmovenumber
 * EPD: position activecolor castlingAvail ENSquare { operations }
 *
 * operations   opcode {operand | "xx sdjsk";}
 * operandy
 * avoid move	am{ tah}*
 * best move	bm{ tah}*
 * direct move count dm { pocet }
 * id	id string
 * predicted move	pm string
 * perft	perft depth = pocet nodu 
 * material key 	key = HexString
 * Musim udelat rozliseni FEN a EPD !!!
 * dm - direct moves - pocet tahu do matu
 * pv - principal variation
 * cX (c0..c9) - comment. In STS C0 contains solutions with score for it, for example  c0 "f5=10, Be5+=2, Bf2=3, Bg4=2";
 */

int parseEPD(char * buffer, char FEN[100], char (*am)[CMTLEN], char (*bm)[CMTLEN], char (*pv)[CMTLEN], char (*cm)[CMTLEN], char *cm9, int *matec, char **name)
{
char * an, *endp;
char b[256], token[256], comment[256];
int count;
int f;
int s,e,i;
size_t l;

// get FEN
//			printf("buffer: %s\n", buffer);

			count=3;
//			st=0;
			if(!isalnum(buffer[0])) return 0;
			e=0;
			while(count>0) {
				s=e;
				l=get_token(buffer,s," ",&s, &e);
				if(l==0) break;
				count--;
			}
			if(count !=0) {
				printf(" FEN error %d!\n", count);
				printf("%s\n", buffer+e);
				return 0;
			}
			count=1;
			while(count>0) {
				s=e;
				l=get_token(buffer,s," ;",&s, &e);
				if(l==0) break;
				count--;
			}
			if(count !=0) {
				printf(" FEN error %d!\n", count);
				printf("%s\n", buffer+e);
				return 0;
			}
// auto/detect epd / fen 
// pokud jsou nasledujici dve pole pouze cisla pak je budeme povazovat za halfmoves a moves
			f=s=e;
			i=0;
			l=get_token(buffer,s," ;",&s, &e);
			if(l>0) {
				strncpy(token, buffer+s, l);
// mozna zakoncit string
				token[l]='\0';
				strtol(token, &endp, 10);
				if(*endp=='\0') {
					s=e;
					l=get_token(buffer,s," ;",&s, &e);
					if(l>0) {
						strncpy(token, buffer+s, l);
						token[l]='\0';
						strtol(token, &endp, 10);
						if(*endp=='\0') {
							i=1;
							f=e;
						}
					}
				}
			}

			strncpy(FEN,buffer, (size_t)f);
			FEN[f]='\0';
			f++; 
			if(i==0) {
// hack			
				strcat(FEN," 0 1");
				f--;
			}
			
//hledam id
// an zacatek bufferu
// an2 zacatek hledaneho retezce
// an3 konec pole
// an4 rozdeleni na radek
			an=buffer+f;

			*name=(char *)malloc(256);
			(*name)[0]='\0';
			if(getEPD_str(an, "id ", b)) {
//				*name=(char *) malloc(strlen(b)+256);
				strcpy(*name, b);
			}
//			else {
//				*name=(char *)malloc(256);
//				(*name)[0]='\0';
//			}

			if(am!=NULL) {
				am[0][0]='\0';
				if(getEPD_str(an, "am ", b)) {
					getEPDmoves(b, am, 10);
				}
			}

			if(bm!=NULL) {
				bm[0][0]='\0';
				if(getEPD_str(an, "bm ", b)) {
					getEPDmoves(b, bm, 10);
				}
			}

			if(matec!=NULL) {
				*matec=-1;
				if(getEPD_str(an, "dm ", b)) {
					*matec= atoi(b);
				}
			}

			if(pv!=NULL) {
				pv[0][0]='\0';
				if(getEPD_str(an, "pv ", b)) {
					getEPDmoves(b, pv, 10);
				}
			}

			if(cm!=NULL) {
				cm[0][0]='\0';
				if(getEPD_str(an, "c0 ", b)) {
					getEPDmoves(b, cm, 10);
				}
			}
			if(cm9!=NULL) {
				cm9[0]='\0';
				if(getEPD_str(an, "c9 ", b)) {
					strcpy(cm9, b);
				}
			}
return 1;
}

int getKeyFEN(char FEN[100], BITVAR *key){
int ret;

	char * pst;
	ret=0;
	pst=strstr(FEN,"key");
	if(pst!=NULL){
		sscanf(pst, "key = %llx", (long long unsigned *) key);
		ret=1;
	}
	return ret;
}

int getPerft(char FEN[100], int *depth, unsigned long long *moves){
int ret;

	char * pst;
	ret=0;

	pst=strstr(FEN,"perft");
	if(pst!=NULL){
		sscanf(pst, "%*s %d = %llu", depth, moves);
		ret=1;
	}
	return ret;
}

int matchLine(int *pv, tree_store *t){
int f;
	if(pv==NULL) return 1;
	if(pv[0]==0) return 1;
	for(f=1;f<=pv[0];f++){
		if(pv[f]!=t->tree[0][f-1].move) return 0;
	}
	return 1;
}

int evaluateAnswer(board *b, int ans, int adm ,MOVESTORE *aans, MOVESTORE *bans, int *pv, int dm, tree_store *t, int len){
	int as, ad, ap, src, des, p, res, prom_need, ba ,i;

	as=UnPackFrom(ans);
	ad=UnPackTo(ans);
	ap=UnPackProm(ans);
	//	asp=UnPackSpec(ans);
	prom_need=0;
	if((b->side==WHITE) && (ad>=A8) && (b->pieces[as]==PAWN)) prom_need=1;
	else if((b->side==BLACK) && (ad>=H1) && (b->pieces[as]==PAWN)) prom_need=1;

	ba=0;
	res=-1;
// the move must be in bans - best moves, if bans is populated
	if(bans!=NULL) {
		for(i=0;i<len;i++) if((*bans!=NA_MOVE)) {
			src=UnPackFrom(*bans);
			des=UnPackTo(*bans);
			p=UnPackProm(*bans);
			ba++;
			if((src==as)&&(des==ad)) {
				if((prom_need!=0)) {
					if (ap==p) res=1;
				} else res=1;
			}
			bans++;
		}
	}
	if(ba==0) res=1;
// the move must NOT be in aans - avoid moves, if aans is populated
	if((aans!=NULL)&&(res==1)) {
		for(i=0;i<len;i++) if((*aans!=NA_MOVE)) {
			src=UnPackFrom(*aans);
			des=UnPackTo(*aans);
			p=UnPackProm(*aans);
			if((src==as)&&(des==ad)) {
				if((prom_need!=0)) {
					if (ap==p) res=-2;
				} else res=-2;
			}
			aans++;
		}
	}
// if DM available, the solution should be that far
	if((dm>0)&&(res==1)) {
//get full moves from adm
		if(adm!=dm) res=-3;
	}

// if PV available, then the PV of the result should be the same
	if((pv!=NULL)&&(res==1)) if(!matchLine(pv,t)) res=-4;
	return res;
}

int evaluateStsAnswer(board *b, int ans, MOVESTORE *bans, MOVESTORE *cans, int *val, int len){
int as, ad, ap, src, des, p, res, prom_need, ba;
int i,n;

	res=0;
	as=UnPackFrom(ans);
	ad=UnPackTo(ans);
	ap=UnPackProm(ans);
	prom_need=0;
	if((b->side==WHITE) && (ad>=A8) && (b->pieces[as]==PAWN)) prom_need=1;
	else if((b->side==BLACK) && (ad>=H1) && (b->pieces[as]==PAWN)) prom_need=1;

	ba=i=-1;
	if(*cans==NA_MOVE) {
		for(n=0;n<len;n++) if((*bans!=NA_MOVE)) {
			src=UnPackFrom(*bans);
			des=UnPackTo(*bans);
			p=UnPackProm(*bans);
			ba++;
			if((src==as)&&(des==ad)) {
				if((prom_need!=0)) {
					if (ap==p) i=ba;
				} else i=ba;
			} 
			bans++;
		}
		if(i!=-1) res=10;
	} else {
		n=0;
		for(n=0;n<len;n++) if((*cans!=NA_MOVE)) {
			src=UnPackFrom(*cans);
			des=UnPackTo(*cans);
			p=UnPackProm(*cans);
			ba++;
			if((src==as)&&(des==ad)) {
				if((prom_need!=0)) {
					if (ap==p) i=ba;
				} else i=ba;
			} 
			cans++;
		}
		if(i>-1) res=val[i];
	}
	LOGGER_4("RES %d:%d\n", i, res);
	return res;
}

//move
//  [Piece][Zdroj][X]DestP[=?|e.p.][+]
// 0-0 0-0-0

MOVESTORE parseOneMove(board *b, char *m)
{
int r,c,p, pp, sr, sf, sp, des, ep_t, p_pole, src, prom_need, cap;
BITVAR aa, xx, bb, x2;
MOVESTORE mm[2];
int zl, sl, l, ll, tl;

MOVESTORE res=NA_MOVE;

		cap=0;
		sr=sf=-1;
		p=sp=ER_PIECE;
		prom_need=0;

		if((strstr(m, "0-0-0")!=NULL)||(strstr(m, "O-O-O")!=NULL)) {
			sp=KING;
			p=KING;
			aa=b->maps[KING] & b->colormaps[b->side];
			sf=4;
			c=2;
			if(b->side==WHITE) {
				sr=0;
				r=0;
			} else {
				sr=7;
				r=7;
			}
			des=r*8+c;
			goto POKR;
		}
		if((strstr(m, "0-0")!=NULL)||(strstr(m, "O-O")!=NULL)) {
			sp=KING;
			p=KING;
			aa=b->maps[KING] & b->colormaps[b->side];
			sf=4;
			c=6;
			if(b->side==WHITE) {
				sr=0;
				r=0;
			} else {
				sr=7;
				r=7;
			}			
			des=r*8+c;
			goto POKR;
		}
		//sync on destination, ll je delka, l ma ukazovat na zacatek dest pole, tl na pridavne info, zl je figura a pocatecni pole
		ll=l=(int)strlen(m);
		if(l<2) return NA_MOVE;
		for(;l>0;l--) {
			if(isdigit(m[l])) break;
		}
		if(l==0) return NA_MOVE;
		l--;
		c=toupper(m[l])-'A';
		r=m[l+1]-'1';

		if(b->side==WHITE) {
			if(r==7) prom_need=1;
		} else if(r==0) prom_need=1;

		tl=l+2;
		if(tl>=ll) goto ZTAH;
		if(m[tl]=='=') {
			// promotion
			tl++;
			pp=toupper(m[tl]);
			switch(pp) {
			case 'Q' : p=QUEEN;
			break;
			case 'R' : p=ROOK;
			break;
			case 'B' : p=BISHOP;
			break;
			case 'N' : p=KNIGHT;
			break;
			default:
					return NA_MOVE;
				break;
			}
			tl++;
		}
		if(tl>=ll) goto ZTAH;

		if(strstr(m+tl, "e.p.")!=NULL) {
			tl+=4;
		}
		if(tl>=ll) goto ZTAH;

		if(m[tl]=='+') {
			tl++;
		}
		if(m[tl]=='#') {
			tl++;
		}
ZTAH:
        sl=0;
		zl=l-1;
		sp=PAWN;
		if(zl<0) goto ETAH;
		if(m[zl]=='x') {
			cap=1;
			zl--;
		}
		if(zl<0) goto ETAH;
		pp=m[sl];
		if(isupper(pp)) {
			switch(pp) {
			case 'Q' : sp=QUEEN;
     				   sl++;
     				   break;
			case 'R' : sp=ROOK;
			   	   	   sl++;
			   	   	   break;
			case 'B' : sp=BISHOP;
				       sl++;
				       break;
			case 'N' : sp=KNIGHT;
          			   sl++;
          			   break;
			case 'K' : sp=KING;
					   sl++;
					   break;
			case 'P' : sp=PAWN;
					   sl++;
			break;
			default:
				sp=PAWN;
				break;
			}
		}
		if(zl>=sl) {
			if(isdigit(m[zl])) {
				sr=m[zl]-'1';
				zl--;
			}
		}
		if(zl>=sl) {
			if(isalpha(m[zl])) {
				sf=tolower(m[zl])-'a';
				zl--;
			}
		}
ETAH:

/*
 * mame limitovanou informaci, zdali jde o tah, brani, jakou figurou a mozna odkud
 * je na case najit pozici figurky
 */
// kontrola
		des=r*8+c;
// ep test
		xx=0;
		if(sp==PAWN) {
			if(cap!=0)	{
				if(b->side==WHITE) ep_t=b->ep+8;
				else ep_t=b->ep-8;
				if((b->ep!=-1) && (des==ep_t)) {
					xx=(attack.ep_mask[b->ep]) & (b->maps[PAWN]) & (b->colormaps[b->side]);
					p=PAWN;
				}
			} else {
				if(b->side==WHITE) p_pole=des-8;
				else p_pole=des+8;
				x2 = normmark[p_pole];
				xx = xx | x2;
				if((b->side==WHITE && r==3) && ((x2 & b->norm)==0)) xx = xx | normmark[p_pole-8];
				else if((b->side==BLACK && r==4)&& ((x2 & b->norm)==0)) xx = xx | normmark[p_pole+8];
			}
		}
		//ostatni test
		//		printBoardNice(b);
		// krome pescu ostatni figury utoci stejne jako se pohybuji, odstranime pesce neni li to brani
		aa=AttackedTo(b, des);
		if(cap!=0) {
			aa|=xx;
		} else {
			aa&=(~(b->maps[PAWN] & b->colormaps[b->side]));
			aa|=xx;
		}
		bb=b->maps[sp];
		aa&=bb;
		bb=b->colormaps[b->side];
		aa&=bb;
		if(sr!=-1) aa=aa& attack.rank[sr*8];
		if(sf!=-1) aa=aa& attack.file[sf];
POKR:

		if(BitCount(aa)!=1) {
			LOGGER_3("EPDmove parse problem, bitcount %d!\n", BitCount(aa));
		} else {
//			if(prom_need==1 && p==PAWN) p=QUEEN;
			src=LastOne(aa);
			res = PackMove(src, des,  p, 0);
			mm[0]=1;
			mm[1]=res;
//			if(validatePATHS(b, &(mm[0]))!=1) res=NA_MOVE;
//			else res=mm[1];
		}
return res;
}

int parseEDPMoves(board *b, attack_model *a, MOVESTORE *ans,  char (*bm)[CMTLEN], int len)
{
int i=0;
	char b2[256];

	for(i=0;i<len;i++) ans[i]=NA_MOVE;
	for(i=0;i<len;i++) if(((*bm)[0]!='\0')) {
		*ans=parseOneMove(b, *bm);
		if(*ans!=NA_MOVE) {
			if(isMoveValid(b, *ans, a, b->side, NULL)) {
				DEB_3(sprintfMove(b, *ans, b2);)
				LOGGER_3("Move A/B: %s\n",b2);
				ans++;
			} else *ans=NA_MOVE;
		}
		bm++;
	}
return 1;
}

int parseCommentMoves(board *b, attack_model *a, MOVESTORE *ans, int *val, char (*bm)[CMTLEN], int len)
{
char m[CMTLEN], v[256];
size_t i;
int f, n;
char *p, *q;
	for(n=0;n<len;n++) ans[n]=NA_MOVE;
	
	n=0;
	for(n=0;n<len;n++) if((*bm)[0]!='\0') {
		p=strstr(*bm,"=");
		if(p!=NULL) {
			i=(size_t)(p-(*bm));
			strncpy(m,*bm, i);
			m[i]='\0';
			p++;
			strcpy(v,p);
			q=v+strlen(v)-1;
			while((q>=v)&&(isdigit(*q)==0)) {
				q--;
			}
			q++;
			*q='\0';
			*ans=parseOneMove(b, m);

			*val=atoi(v);
			LOGGER_4("CMT parse %o:%d %s ", *ans, *val, m);
			if(isMoveValid(b, *ans, a, b->side, NULL)) {
				NLOGGER_4(" valid");
				ans++;
				val++;
			} else *ans=NA_MOVE;
			NLOGGER_4("\n");
		}
		bm++;
	}
return 1;
}

int parsePVMoves(board *b, attack_model *a, int *ans, char (*bm)[CMTLEN], int len)
{
UNDO u[256];
attack_model att;
MOVESTORE mm[2];
int f,i,r, *z, n;
	char b2[256];

	for(n=0;n<len;n++) ans[n]=NA_MOVE;

	z=ans;
	ans++;
	f=1;
	mm[1]=0;
	n=0;
	for(n=0;n<len;n++) if((*bm)[0]!='\0') {
		mm[0]=parseOneMove(b, *bm);
		if(mm[0]!=NA_MOVE) {
			eval_king_checks_all(b, &att);
			if(isMoveValid(b, mm[0], &att, b->side, NULL)) {
				DEB_3(sprintfMove(b, mm[0], b2);)
				LOGGER_3("Move PV: %s\n",b2);
				u[f]=MakeMove(b, mm[0]);
				f++;
				*ans=mm[0];
				ans++;
			} else *ans=NA_MOVE;
		}
		bm++;
	}
	f--;
	*z=f;

	for(;f>0;f--) {
	 UnMakeMove(b, u[f]);
	}
	r=1;
return r;
}


void movegenTest(char *filename)
{
	char buffer[512], fen[100];
	char am[10][CMTLEN];
	char bm[10][CMTLEN];
	char cm[10][CMTLEN];
	char pm[256][CMTLEN];
	int dm;
	FILE * handle;
	int i,x;
	board b;
	char * name;
	char * xx;
	struct _ui_opt uci_options;

	b.uci_options=&uci_options;
	b.stats=allocate_stats(1);
	b.hs=allocateHashStore(HASHSIZE, 2048);
	b.hps=allocateHashPawnStore(HASHPAWNSIZE);
	b.hht=allocateHHTable();
	b.kmove=allocateKillerStore();

			if((handle=fopen(filename, "r"))==NULL) {
				printf("File %s is missing\n",filename);
				return;
			}
			xx=fgets(buffer, 511, handle);
			i=0;

			b.pers=(personality *) init_personality("pers.xml");

			while(!feof(handle)) {
				if(parseEPD(buffer, fen, am, bm, pm, cm, NULL, &dm, &name)==1) {
					setup_FEN_board(&b, fen);
					printBoardNice(&b);
					printf("----- MoveGenTest, name:%s -----\n",name);
					DEB_4(boardCheck(&b));
					moveGenTest1(&b);
					free(name);
				}
				i++;
				xx=fgets(buffer, 511, handle);
			}
			free(b.pers);
			fclose(handle);
			freeKillerStore(b.kmove);
			freeHHTable(b.hht);
			freeHashPawnStore(b.hps);
			freeHashStore(b.hs);
			deallocate_stats(b.stats);
}

/*
 * b contains setup board
 * simple loop
 *
 * - generateMoves
 * 	- store board status
 * 	- make move
 * 	- unmake move
 * 	- check board with stored
 */

unsigned long long int perftLoopO_int(board *b, int d, int side, attack_model *tolev){
UNDO u;
move_entry move[300], *m, *n;
int opside, incheck, incheck2;
int tc, cc, tc2;
unsigned long long nodes, tnodes;
attack_model *a, ATT;
BITVAR attacks;

	if (d==0) return 1;
	nodes=0;
	opside = (side == WHITE) ? BLACK : WHITE;
	a=&ATT;

	a->ke[b->side]=tolev->ke[b->side];
	simple_pre_movegen(b, a, side);
	a->att_by_side[opside]=KingAvoidSQ(b, a, opside);

	if(isInCheck_Eval(b, a, side)!=0) incheck=1; else incheck=0;
	n=m=move;
	if(incheck==1) generateInCheckMoves(b, a, &m);
	else {
		generateCaptures(b, a, &m, 1);
		generateMoves(b, a, &m);
	}

	tc=m-n;
	cc = 0;
	if(d==1) return tc;

	while (cc<tc) {
		u=MakeMove(b, move[cc].move);
		eval_king_checks(b, &(a->ke[opside]), NULL, opside);
		tnodes=perftLoopO_int(b, d-1, opside, a);
		nodes+=tnodes;
		UnMakeMove(b, u);
		cc++;
	}
return nodes;
}

unsigned long long int perftLoopX_int(board *b, int d, int side, attack_model *tolev, int incheck){
UNDO u;
move_entry move[300], *m, *n;
int opside, incheck2;
int tc, cc, tc2;
unsigned long long nodes, tnodes;
attack_model *a, ATT;
BITVAR attacks;
char buf[300];

	if (d==0) return 1;
	nodes=0;
	opside = (side == WHITE) ? BLACK : WHITE;
	a=&ATT;

	a->ke[b->side]=tolev->ke[b->side];
	a->att_by_side[opside]=KingAvoidSQ(b, a, opside);

	n=m=move;
	if(incheck==1) {
		simple_pre_movegen_n2check(b, a, side);
		generateInCheckMovesN(b, a, &m, 1);
	}
	else {
		simple_pre_movegen_n2(b, a, side);
		generateCapturesN(b, a, &m, 1);
		generateMovesN(b, a, &m);
	}

	tc=m-n;
	cc = 0;

	if(d==1) return tc;
	while (cc<tc) {
	
		u=MakeMove(b, move[cc].move);
		eval_king_checks(b, &(a->ke[opside]), NULL, opside);
		
		tnodes=perftLoopX_int(b, d-1, opside, a, (a->ke[opside].attackers!=0));
		nodes+=tnodes;
		UnMakeMove(b, u);
		cc++;
	}
return nodes;
}

unsigned long long int perftLoopN_int(board *b, int d, int side, attack_model *tolev){
UNDO u;
int opside, incheck, incheck2, t2;
unsigned int cc;
unsigned long long nodes, tnodes;
attack_model *a, ATT;
move_cont mvs;
move_entry *m;
move_entry move[300], *n;
king_eval ke;
char b2[256];
BITVAR attacks;

	if (d==0) return 1;
	n=move;
	nodes=0;
	opside = (side == WHITE) ? BLACK : WHITE;
	a=&ATT;

	a->ke[b->side]=tolev->ke[b->side];
	a->att_by_side[opside]=KingAvoidSQ(b, a, opside);

	if(a->att_by_side[opside]&normmark[b->king[side]]) {
		incheck=1;
		simple_pre_movegen_n2check(b, a, side);
	}else {
		simple_pre_movegen_n2(b, a, side);
		incheck=0;
	}

	sortMoveListNew_Init(b, a, &mvs);
	while ((getNextMove(b, a, &mvs, 1, side, incheck, &m, NULL)!=0)) {
		*n=*(m);
		n++;
		if(d!=1) {
			t2=b->mindex;
			u=MakeMove(b, m->move);
			eval_king_checks(b, &(a->ke[opside]), NULL, opside);
			tnodes=perftLoopN_int(b, d-1, opside, a);
			nodes+=tnodes;
			UnMakeMove(b, u);
			if(t2!=b->mindex) {
				printBoardNice(b);
				sprintfMoveSimple(m->move, b2);
				printf("%s\n", b2 );
				LOGGER_1("%s\n", b2 );
			}
		} else nodes++;
	}
return nodes;
}

unsigned long long int perftLoopN_v(board *b, int d, int side, attack_model *tolev, int div){
UNDO u;
int opside, incheck, incheck2;
unsigned int cc;
unsigned long long nodes, tnodes;
attack_model *a, ATT;
move_cont mvs;
move_entry *m;
move_entry move[300], *n;
char buf[300];
BITVAR attacks;

	n=move;
	if (d==0) return 1;
	nodes=0;
	opside = (side == WHITE) ? BLACK : WHITE;
	a=&ATT;

	eval_king_checks(b, &(a->ke[side]), NULL, side);
	eval_king_checks(b, &(a->ke[opside]), NULL, opside);
	attacks=KingAvoidSQ(b, a, opside);
	a->att_by_side[opside]=attacks;

	if(attacks&normmark[b->king[side]]) {
		incheck=1;
		simple_pre_movegen_n2check(b, a, side);
	}else {
		simple_pre_movegen_n2(b, a, side);
		incheck=0;
	}

	sortMoveListNew_Init(b, a, &mvs);
	while ((getNextMove(b, a, &mvs, 0, side, incheck, &m, NULL)!=0)) {
		*n=*(m);
		n++;
	  if(d!=1) {
		u=MakeMove(b, m->move);
		eval_king_checks(b, &(a->ke[opside]), NULL, opside);
		tnodes=perftLoopN_int(b, d-1, opside, a);
		UnMakeMove(b, u);
	  } else tnodes=1;
		nodes+=tnodes; 
	  if(div) {
		sprintfMoveSimple(m->move, buf);
		printf("%s\t\t%lld\n", buf, tnodes );
		LOGGER_1("%s\t\t%lld\n", buf, tnodes );
	  }
	}
return nodes;
}

unsigned long long int perftLoopO_v(board *b, int d, int side, attack_model *tolev, int div){
UNDO u;
move_entry move[300], *m, *n;
int tc, cc, opside, incheck;
unsigned long long nodes, tnodes;
attack_model *a, ATT;
struct timespec start, end;
unsigned long long int totaltime;
char buf[20], fen[100];
BITVAR attacks;


	if (d==0) return 1;

	nodes=0;
	opside = (side == WHITE) ? BLACK : WHITE;
	a=&ATT;

	eval_king_checks(b, &(a->ke[side]), NULL, side);
	eval_king_checks(b, &(a->ke[opside]), NULL, opside);
	simple_pre_movegen(b, a, side);
	attacks=KingAvoidSQ(b, a, opside);
	a->att_by_side[opside]=attacks;

	if(isInCheck_Eval(b, a, opside)!=0) {
		log_divider("OPSIDE in check!");
		printBoardNice(b);
		printboard(b);
		printf("Opside in check!\n");
		return 0;
	}
	if(isInCheck_Eval(b, a, side)!=0) incheck=1; else incheck=0;

	n=m=move;
	if(incheck==1) generateInCheckMoves(b, a, &m);
	else {
		generateCaptures(b, a, &m, 1);
		generateMoves(b, a, &m);
	}

	tc=(int)(m-n);
	cc = 0;
	
	if((d==1)&(div==0)) return tc;
	while (cc<tc) {
		readClock_wall(&start);
		u=MakeMove(b, move[cc].move);
		eval_king_checks(b, &(a->ke[opside]), NULL, opside);
		tnodes=perftLoopO_int(b, d-1, opside, a);
		nodes+=tnodes;
		if(div) {
			sprintfMoveSimple(move[cc].move, buf);
			writeEPD_FEN(b, fen, 0,"");
			readClock_wall(&end);
			totaltime=diffClock(start, end)+1;
			printf("%s\t\t%lld\t\t(%lld:%lld.%lld\t%lld tis/sec,\t\t%s perft %d = %lld )\n", buf, tnodes, totaltime/60000000,(totaltime%60000000)/1000000,(totaltime%1000000)/1000, tnodes*1000/totaltime, fen, d-1, tnodes );
			LOGGER_1("%s\t\t%lld\t\t(%lld:%lld.%lld\t%lld tis/sec,\t\t%s perft %d = %lld )\n", buf, tnodes, totaltime/60000000,(totaltime%60000000)/1000000,(totaltime%1000000)/1000, tnodes*1000/totaltime, fen, d-1, tnodes );
		}
		UnMakeMove(b, u);
		cc++;
	}
return nodes;
}

unsigned long long int perftLoopX_v(board *b, int d, int side, attack_model *tolev, int div){
UNDO u;
move_entry move[300], *m, *n;
int tc, cc, opside, incheck;
unsigned long long nodes, tnodes;
attack_model *a, ATT;
struct timespec start, end;
unsigned long long int totaltime;
char buf[20], fen[100];
BITVAR attacks;

	if (d==0) return 1;

	nodes=0;
	opside = (side == WHITE) ? BLACK : WHITE;
	a=&ATT;

	eval_king_checks(b, &(a->ke[side]), NULL, side);
	eval_king_checks(b, &(a->ke[opside]), NULL, opside);
	attacks=KingAvoidSQ(b, a, opside);
	a->att_by_side[opside]=attacks;
//	printmask(a->att_by_side[opside], "attacks");

	n=m=move;
	if(a->ke[side].attackers!=0) {
		incheck=1;
		simple_pre_movegen_n2check(b, a, side);
		generateInCheckMovesN(b, a, &m, 1);
	}else {
		simple_pre_movegen_n2(b, a, side);
		incheck=0;
		generateCapturesN(b, a, &m, 1);
		generateMovesN(b, a, &m);
	}

	tc=(int)(m-n);
	cc = 0;
	if((d==1)&(div==0)) return tc;
	while (cc<tc) {
		readClock_wall(&start);
		u=MakeMove(b, move[cc].move);
		eval_king_checks(b, &(a->ke[opside]), NULL, opside);
		tnodes=perftLoopX_int(b, d-1, opside, a, (a->ke[opside].attackers!=0));
		nodes+=tnodes;
		if(div) {
			sprintfMoveSimple(move[cc].move, buf);
			writeEPD_FEN(b, fen, 0,"");
			readClock_wall(&end);
			totaltime=diffClock(start, end)+1;
			printf("%s\t\t%lld\t\t(%lld:%lld.%lld\t%lld tis/sec,\t\t%s perft %d = %lld )\n", buf, tnodes, totaltime/60000000,(totaltime%60000000)/1000000,(totaltime%1000000)/1000, tnodes*1000/totaltime, fen, d-1, tnodes );
			LOGGER_1("%s\t\t%lld\t\t(%lld:%lld.%lld\t%lld tis/sec,\t\t%s perft %d = %lld )\n", buf, tnodes, totaltime/60000000,(totaltime%60000000)/1000000,(totaltime%1000000)/1000, tnodes*1000/totaltime, fen, d-1, tnodes );
		}
		UnMakeMove(b, u);
		cc++;
	}
return nodes;
}

unsigned long long int perftLoopN(board *b, int d, int side, attack_model *tolev){
return perftLoopN_v(b, d, side, tolev, 0);
}

unsigned long long int perftLoopN_d(board *b, int d, int side, attack_model *tolev){
return perftLoopN_v(b, d, side, tolev, 1);
}

unsigned long long int perftLoopO(board *b, int d, int side, attack_model *tolev){
return perftLoopO_v(b, d, side, tolev, 0);
}

unsigned long long int perftLoopO_d(board *b, int d, int side, attack_model *tolev){
return perftLoopO_v(b, d, side, tolev, 1);
}

unsigned long long int perftLoopX(board *b, int d, int side, attack_model *tolev){
return perftLoopX_v(b, d, side, tolev, 0);
}

unsigned long long int perftLoopX_d(board *b, int d, int side, attack_model *tolev){
return perftLoopX_v(b, d, side, tolev, 1);
}

// callback funkce
#define CBACK int (*cback)(char *fen, void *data)

void perft_driver(int min, int max, int sw, CBACK, void *cdata)
{
char buffer[512], fen[100];
int i, depth;
board b;
attack_model *a, ATT;
unsigned long long int nodes, counted;
char * name;
struct timespec start, end, st, et;
unsigned long long int totaltime, nds;

unsigned long long int (*loop)(board *b, int d, int side, attack_model *aaa);

struct _ui_opt uci_options;

	b.uci_options=&uci_options;
	b.stats=allocate_stats(1);
	b.hs=allocateHashStore(HASHSIZE, 2048);
	b.hps=allocateHashPawnStore(HASHPAWNSIZE);
	b.hht=allocateHHTable();
	b.kmove=allocateKillerStore();
	a=&ATT;

// normal mode
		switch(sw) {
			case 1: loop=&perftLoopO_d;
					break;
			case 2: loop=&perftLoopX;
					break;
			case 3:	loop=&perftLoopX_d;
					break;
			case 4:	loop=&perftLoopN;
					break;
			case 5: loop=&perftLoopN_d;
					break;
			default:
					loop=&perftLoopO;
		}
		b.pers=(personality *) init_personality("pers.xml");		
		nds=0;
		i=1;
		readClock_wall(&st);
		while(cback(buffer,cdata)) {
			if(parseEPD(buffer, fen, NULL, NULL, NULL, NULL, NULL, NULL, &name)>0) {
				if(getPerft(buffer,&depth,&nodes)==1) {
					setup_FEN_board(&b, fen);

					readClock_wall(&start);
					if((min<=depth)&&(depth<=max)) {
						LOGGER_1("----- Evaluate:%d Begin, Depth:%d, Nodes Exp:%llu; %s -----\n",i, depth, nodes, name);
						printf("----- Evaluate:%d Begin, Depth:%d, Nodes Exp:%llu; %s -----\n",i, depth, nodes, name);
						eval_king_checks_all(&b, a);
						counted=loop(&b, depth, b.side, a);
						readClock_wall(&end);
						totaltime=diffClock(start, end)+1;
						printf("----- Evaluate:%d -END-, Depth:%d, Nodes Cnt:%llu, Time: %lld:%lld.%lld; %lld tis/sec,  %s -----\n",i, depth, counted,totaltime/60000000,(totaltime%60000000)/1000000,(totaltime%1000000)/1000, (counted*1000/totaltime), name);
						LOGGER_1("----- Evaluate:%d -END-, Depth:%d, Nodes Cnt:%llu, Time: %lld:%lld.%lld; %lld tis/sec,  %s -----\n",i, depth, counted,totaltime/60000000,(totaltime%60000000)/1000000,(totaltime%1000000)/1000, (counted*1000/totaltime), name);
						nds+=counted;
						if(nodes!=counted){
							printf("Not Match!\n");
							LOGGER_1("NOT MATCH!\n");
							printBoardNice(&b);
						}
					} else {
//						printf("----- Evaluate:%d -END-, SKIPPED %d -----\n",i,depth);
//						LOGGER_1("----- Evaluate:%d -END-, SKIPPED %d -----\n",i,depth);
					}
					free(name);
				}
			}
			i++;
		}
	readClock_wall(&et);
		totaltime=diffClock(st, et)+1;
		printf("Nodes: %llu, Time: %lldm:%llds.%lld; %lld tis/sec\n",nds, totaltime/60000000,(totaltime%60000000)/1000000,(totaltime%1000000)/1000, (nds*1000/totaltime));
		LOGGER_1("Nodes: %llu, Time: %lldm:%llds.%lld; %lld tis/sec\n",nds, totaltime/60000000,(totaltime%60000000)/1000000,(totaltime%1000000)/1000, (nds*1000/totaltime));
		deallocate_stats(b.stats);
		freeKillerStore(b.kmove);
		freeHHTable(b.hht);
		freeHashPawnStore(b.hps);
		freeHashStore(b.hs);
		free(b.pers);
}

int perft2_def_cback(char *fen, void *data){
int *i;
	i = (int *)data ;
	if(perft_default_tests[*i]!=NULL) {
		strcpy(fen, perft_default_tests[*i]);
		(*i)++;
		return 1;
	}
	return 0;
}

void perft2_def(int min, int max, int sw){
int i=0;
	perft_driver(min, max, sw, perft2_def_cback, &i);
}

typedef struct __perft2_cb_data {
	FILE * handle;
	int loops;
	int lo;
} perft2_cb_data;

int perft2_cback(char *fen, void *data){
char buffer[512];
int x;
char *xx;
perft2_cb_data *i;
	i = (perft2_cb_data *)data ;
nextloop:
	if(!feof(i->handle)) {
		xx=fgets(buffer, 511, i->handle);
		if(xx!=NULL)	strcpy(fen, buffer);
		return 1;
	}
	i->lo++;
	if(i->lo<i->loops) {
		fseek(i->handle, 0, SEEK_SET);
		goto nextloop;
	}
	return 0;
}

typedef struct {
	FILE * handle;
	int i;
	int n;
} sts_cb_data;

int sts_cback(char *fen, void *data){
char buffer[2048];
int x;
char *xx;
sts_cb_data *i;
	i = (sts_cb_data *)data ;
	
	fen[0]='\0';
	while(fgets(buffer, 2047, i->handle)!=NULL) {;
		strcpy(fen, buffer);
		i->n++;
		return 1;
	}
	return 0;
}

void perft2(char * filename, int min, int max, int sw){
perft2_cb_data cb;
	if((cb.handle=fopen(filename, "r"))==NULL) {
		printf("File %s is missing\n",filename);
		return;
	}
	cb.lo=0;
	cb.loops=1;
	perft_driver(min, max, sw, perft2_cback, &cb);
	fclose(cb.handle);
}

void perft2x(char * filename, int min, int max, int sw, int l){
perft2_cb_data cb;
	if((cb.handle=fopen(filename, "r"))==NULL) {
		printf("File %s is missing\n",filename);
		return;
	}
	cb.lo=0;
	cb.loops=l;
	perft_driver(min, max, sw, perft2_cback, &cb);
	fclose(cb.handle);
}

int timed2_def_cback(char *fen, void *data){
int *i;
	i = (int *)data ;
	if(timed_default_tests[*i]!=NULL) {
		strcpy(fen, timed_default_tests[*i]);
		(*i)++;
		return 1;
	}
	return 0;
}

int timed2_remis_cback(char *fen, void *data){
int *i;
	i = (int *)data ;
	if(remis_default_tests[*i]!=NULL) {
		strcpy(fen, remis_default_tests[*i]);
		(*i)++;
		return 1;
	}
	return 0;
}

int timed_driver(int t, int d, int max,personality *pers_init, int sts_mode, struct _results *results, CBACK, void *cdata)
{
	char buffer[10], fen[100], b2[1024], b3[2048], b4[512];
	char bx[2048];
	char am[10][CMTLEN];
	char bm[10][CMTLEN];
	char cc[10][CMTLEN], (*cm)[CMTLEN];
	int v[10];
	char pm[256][CMTLEN];
	char (*x)[CMTLEN];
	MOVESTORE bans[20], aans[20], cans[20];
	int dm, adm;
	int pv[CMTLEN];
	int i, time, depth, f, n;
	board b;
	int val, error, passed, res_val;
	unsigned long long starttime, endtime, ttt;
	struct _statistics s;
	struct _ui_opt uci_options;
	struct _statistics *stat;
	attack_model ATT, *a;
	char * name;
	tree_store * moves;
	// normal mode
	cm=NULL;
	if(sts_mode!=0) cm=cc;
	passed=error=res_val=0;
	moves = (tree_store *) malloc(sizeof(tree_store));
	b.stats=allocate_stats(1);
	b.pers=pers_init;
	b.hs=allocateHashStore(HASHSIZE, 2048);
	b.hps=allocateHashPawnStore(HASHPAWNSIZE);
	b.hht=allocateHHTable();
	b.kmove=allocateKillerStore();
	b.uci_options=&uci_options;

	stat = allocate_stats(1);
	moves->tree_board.stats=stat;

	a=&ATT;
// personality should be provided by caller
	i=0;
	clearSearchCnt(&s);
	while(cback(bx, cdata)&&(i<max)) {
		if(parseEPD(bx, fen, am, bm, pm, cm, NULL, &dm, &name)>0) {
			time=t;
			depth=d;
			setup_FEN_board(&b, fen);
			eval_king_checks(&b, &(a->ke[b.side]), pers_init, b.side);
			DEB_3(printBoardNice(&b);)
			parseEDPMoves(&b,a, bans, bm, 10);
			parseEDPMoves(&b,a, aans, am, 10);
			parsePVMoves(&b, a, pv, pm, 10);
			if(sts_mode!=0) parseCommentMoves(&b, a, cans, v, cm, 10);

			//setup limits
			b.uci_options->engine_verbose=0;
			b.uci_options->binc=0;
			b.uci_options->btime=0;
			b.uci_options->depth=depth;
			b.uci_options->infinite=0;
			b.uci_options->mate=0;
			b.uci_options->movestogo=1;
			b.uci_options->movetime=0;
			b.uci_options->ponder=0;
			b.uci_options->winc=0;
			b.uci_options->wtime=0;
			b.uci_options->search_moves[0]=0;

			b.uci_options->nodes=0;
			b.uci_options->movetime=time-1;

			b.run.time_move=b.uci_options->movetime;
			b.run.time_crit=b.uci_options->movetime;

			engine_stop=0;
			clear_killer_moves(b.kmove);
			initPawnHash(b.hps);
			initHash(b.hs);
//			invalidateHash();
			clearSearchCnt(b.stats);

			starttime=readClock();
			b.run.time_start=starttime;
			b.move_ply_start=b.move;
//			printBoardNice(&b);
			val=IterativeSearchN(&b, 0-iINFINITY, iINFINITY, b.uci_options->depth, b.side, 1, moves);

			endtime=readClock();
			ttt=endtime-starttime;
//			(LOGGER_1("TIMESTAMP: Start: %llu, Stop: %llu, Diff: %lld milisecs\n", b.run.time_start, endtime, (endtime-b.run.time_start)));
			CopySearchCnt(&(results[i].stats), b.stats);
			AddSearchCnt(&s, b.stats);
			sprintfMove(&b, b.bestmove, buffer);

//			printf("%d\n", b.bestscore);
			if(isMATE(b.bestscore))  {
				int ply=GetMATEDist(b.bestscore);
				if(ply==0) adm=1;
				else {
					adm= (b.side==WHITE ? (ply+1)/2 : (ply/2)+1);
				}
			} else adm=-1;
			// ignore exact PV

			results[i].bestscore=val;
			results[i].time=ttt;
			sprintf(results[i].move, "%s", buffer);
			results[i].dm=adm;
			
			val=0;
			if(sts_mode!=0) {
				val=evaluateStsAnswer(&b, b.bestmove, bans, cans, v, 10);
			} else {
				val=evaluateAnswer(&b, b.bestmove, adm , aans, bans, NULL, dm, moves, 10);
			}
			results[i].passed=val;
			if(val<=0) {
			
				sprintf(b2, "Error: Move %s, DtM %d => ", buffer, adm);
				error++;
				
				if(val==-1) {
					sprintf(b4,"BM ");
					x=bm;
					while((*x)[0]!=0) {
						strcat(b4, (*x));
						strcat(b4," ");
						x++;
					}
					strcat(b2, b4);
				}
				if(val==-2) {
					sprintf(b4,"AM ");
					strcat(b2, b4);
				}
				if(val==-3) {
					sprintf(b4, "DM %i", dm);
					strcat(b2, b4);
				}
			}
			else {
				sprintf(b2, "Passed, Move: %s, toMate: %i", buffer, adm);
				passed++;
				res_val+=val;
			}
//			printf("%s %i\n", bx, val);
			sprintf(b3, "%d: FEN:%s, %s, Time: %dh, %dm, %ds, %dms\n",i, fen, b2, (int) ttt/3600000, (int) (ttt%3600000)/60000, (int) (ttt%60000)/1000, (int)ttt%1000);
			printf(b3);
			LOGGER_0(b3);
			free(name);
			i++;
		}
	}

	CopySearchCnt(&(results[i].stats), &s);
	freeKillerStore(b.kmove);
	freeHHTable(b.hht);
	freeHashPawnStore(b.hps);
	freeHashStore(b.hs);
	deallocate_stats(stat);
	deallocate_stats(b.stats);
	free(moves);
	if(sts_mode!=0) sprintf(b3, "Positions Total %d, Passed %d with total Value %d, Error %d\n",passed+error, passed, res_val, error);
	else {
		sprintf(b3, "Positions Total %d, Passed %d, Error %d\n",passed+error, passed, error);
		tell_to_engine(b3);
	}
	return i;
}

int timed_driver_eval(int t, int d, int max,personality *pers_init, int sts_mode, struct _results *results, CBACK, void *cdata)
{
	char buffer[512], fen[100], b3[1024], b4[512];
	char bx[512];
//	char cc[10][20];
//	MOVESTORE cans[20];
	int i, time, depth, f, n;
	board b;
	int val, error, passed, res_val;
	struct _statistics *stat;

	attack_model a;
	struct _ui_opt uci_options;
	struct _statistics s;
	int ev, ph, ev2;

	char * name;
	b.stats=allocate_stats(1);
	b.pers=pers_init;
	b.hs=allocateHashStore(HASHSIZE, 2048);
	b.hps=allocateHashPawnStore(HASHPAWNSIZE);
	b.hht=allocateHHTable();
	b.kmove=allocateKillerStore();
	b.uci_options=&uci_options;

	stat = allocate_stats(1);

	i=0;

// personality should be provided by caller
	i=0;
	while(cback(bx, cdata)&&(i<max)) {
		if(parseEPD(bx, fen, NULL, NULL, NULL, NULL, NULL, NULL, &name)>0) {

			time=t;
			depth=d;

			setup_FEN_board(&b, fen);
			DEB_3(printBoardNice(&b);)

			ph= eval_phase(&b, pers_init);
			ev=eval(&b, &a, pers_init);

//			if((i%1000)==0) printf("Records %d\n",i);
			b.side = (b.side==WHITE) ? BLACK : WHITE;
			ev2=eval(&b, &a, pers_init);

			results[i].bestscore=ev;
			results[i].passed=ev-ev2;
			if(ev!=ev2) {
//				printf("Nesoulad %d\n", i);
				writeEPD_FEN(&b, fen, 0, "");
				logger2("%s\n",fen);
			}
			i++;
		}
	}

	freeKillerStore(b.kmove);
	freeHHTable(b.hht);
	freeHashPawnStore(b.hps);
	freeHashStore(b.hs);
	deallocate_stats(stat);
	deallocate_stats(b.stats);
//	tell_to_engine(b3);
	return i;
}

void timed2_def(int time, int depth, int max){
int i=0;
personality *pi;
struct _results *r;
	r = (struct _results*)malloc(sizeof(struct _results) * (max+1));
	pi=(personality *) init_personality("pers.xml");
	timed_driver(time, depth, max, pi, 0, r, timed2_def_cback, &i);
	printSearchStat(&(r[max].stats));
	free(r);
	free(pi);
}

void timed2_remis(int time, int depth, int max){
int i=0;
personality *pi;
struct _results *r;
	r = malloc(sizeof(struct _results) * (max+1));
	pi=(personality *) init_personality("pers.xml");
	timed_driver(time, depth, max, pi, 0, r, timed2_remis_cback, &i);
	printSearchStat(&(r[max].stats));
	free(r);
	free(pi);
}

void timed2Test(char *filename, int max_time, int max_depth, int max_positions){
perft2_cb_data cb;
personality *pi;
int p1,f,i1,i;
unsigned long long t1;
char b[1024];
struct _results *r1;

	r1 = malloc(sizeof(struct _results) * (max_positions+1));
	pi=(personality *) init_personality("pers.xml");

	if((cb.handle=fopen(filename, "r"))==NULL) {
		printf("File %s is missing\n",filename);
		goto cleanup;
	}
	i1=timed_driver(max_time, max_depth, max_positions, pi, 0, r1, perft2_cback, &cb);
	fclose(cb.handle);

// prepocitani vysledku
	t1=0;
	p1=0;
	for(f=0;f<i1;f++){
		t1+=r1[f].time;
		if(r1[f].passed>0) p1++;
	}

//reporting
	logger2("Details  \n====================\n");
	logger2("Run#1 Results %d/%d, , Time: %dh, %dm, %ds,, %lld\n",p1,i1, (int) t1/3600000, (int) (t1%3600000)/60000, (int) (t1%60000)/1000, t1);
	logger2("Stats\n");
	i= i1;
	for(f=0;f<i;f++){
	logger2("====================\n");
		t1=r1[f].time;
		logger2("RUN #1, Position %d, , Time: %dh, %dm, %ds,, %lld\n",f, (int) t1/3600000, (int) (t1%3600000)/60000, (int) (t1%60000)/1000, t1);
		logger2("========\n");
		printSearchStat(&(r1[f].stats));
	}
	logger2("====================\n");

cleanup:
	free(r1);
	free(pi);
}

void timed2Test_IQ(char *filename, int max_time, int max_depth, int max_positions){
perft2_cb_data cb;
personality *pi;
int p1,f,i1,i;
long int t1;
char b[1024];
struct _results *r1;
float score;

	r1 = malloc(sizeof(struct _results) * (max_positions+1));
	pi=(personality *) init_personality("pers.xml");

	if((cb.handle=fopen(filename, "r"))==NULL) {
		printf("File %s is missing\n",filename);
		goto cleanup;
	}
	i1=timed_driver(max_time, max_depth, max_positions, pi, 0, r1, perft2_cback, &cb);
	fclose(cb.handle);
	LOGGER_0("finished max %d, real %d\n", max_positions, i1);

// prepocitani vysledku
	t1=0;
	p1=0;
	for(f=0;f<i1;f++){
		t1+=r1[f].time;
		if(r1[f].passed>0) p1++;
	}
	LOGGER_0("computed\n");	
	score=p1*670.0f/f + 1995.0f;
//	score=p1*670.0f/t1 + 1995.0f;
//reporting
	logger2("Details  \n====================\n");
	logger2("Run#1 Results %d/%d, , Time: %dh, %dm, %ds,, %lld\n",p1,i1, (int) t1/3600000, (int) (t1%3600000)/60000, (int) (t1%60000)/1000, t1);
	logger2("IQ score %lf\n", score);

cleanup:
	free(r1);
	free(pi);
}

void timed2Test_x(char *filename, int max_time, int max_depth, int max_positions){
perft2_cb_data cb;
personality *pi;
int p1,f,i1;
char b[1024];
struct _results *r1;

	r1 = malloc(sizeof(struct _results) * (max_positions+1));
	pi=(personality *) init_personality("pers.xml");

	if((cb.handle=fopen(filename, "r"))==NULL) {
		printf("File %s is missing\n",filename);
		goto cleanup;
	}
	i1=timed_driver_eval(max_time, max_depth, max_positions, pi, 0, r1, perft2_cback, &cb);
	fclose(cb.handle);

// prepocitani vysledku
	p1=0;
	for(f=0;f<i1;f++){
		if(r1[f].passed==0) p1++;
	}

//reporting
	logger2("Details  \n====================\n");
	logger2("Run#1 Results passed %d/%d\n",p1,i1);

//	results[i].bestscore=ev;
//	results[i].passed=ev-ev2;
//	if(ev!=ev2) {
//				printf("Nesoulad %d\n", i);
//			}

	for(f=0;f<i1;f++){
		if(r1[f].passed!=0) printf("%d\tERRr: %d, %d, %d\n", f, r1[f].bestscore, r1[f].bestscore-r1[f].passed, r1[f].passed);
	}

cleanup:
	free(r1);
	free(pi);
}

void timed2STS(int max_time, int max_depth, int max_positions, char *per1, char *per2){
sts_cb_data cb;
personality *pi;
int n, q,f;
int p1[20][20], i1[20][20], v1[20][20], vt1[20][20];
int p1m[20], i1m[20], v1m[20], vt1m[20];

unsigned long long t1[20][20];

int p2[20][20], i2[20][20], v2[20][20], vt2[20][20];
unsigned long long t2[20][20];
char b[5000], filename[512], b2[5000];
struct _results *r1[16];
struct _results *rh;

int times[]= { 100, 500, 1000, 2000,  5000, 10000, 20000, -1 }, maximum_t;
char *sts_tests[]= { "../tests/sts1.epd","../tests/sts2.epd", "../tests/sts3.epd","../tests/sts4.epd","../tests/sts5.epd","../tests/sts6.epd","../tests/sts7.epd","../tests/sts8.epd",
"../tests/sts9.epd","../tests/sts10.epd","../tests/sts11.epd","../tests/sts12.epd","../tests/sts13.epd", "../tests/sts14.epd", "../tests/sts15.epd" };
//int tests_setup[]= { 10,100, 1,100, 6,00, 7,00, 12,00, 8,00, 11,00, 3,00, 4,00, 0,00, 2,00, 9,00, 5,00 ,-1};
//int tests_setup[]= { 10,100, 1,100, 6,100, 7,100, 12,100, 8,100, 11,100, 3,100, 4,100, 0,100, 2,100, 9,100, 5,100 ,-1};
int tests_setup[]= { 12,10, 11,10, 8,10, 7,10, 3,10, 6,10, 10,10, 14,10, 1,10, 2,10, 0,10, 4,10, 9,10, 13,10, 5,10, -1,-1};
int index, mx, count, pos, cc;

	LOGGER_0("STS time %d, depth %d, pos %d, pers1 %s, pers2 %s\n", max_time, max_depth, max_positions, per1, per2);

	if(per1==NULL) pi=(personality *) init_personality("pers.xml");
	else pi=(personality *) init_personality(per1);

	index=0;
	count=0;
	for(maximum_t=0; times[maximum_t]!=-1;maximum_t++) if(times[maximum_t]>max_time) break;

	while(tests_setup[index]!=-1) {
		index++;
		if(tests_setup[index]>max_positions) cc=max_positions; else cc=tests_setup[index];
		count+=cc;
		index++;
	}

	rh = malloc(sizeof(struct _results) * (count+1));
	for(q=0;q<maximum_t;q++) {
		max_time=times[q];
		index=pos=mx=0;
		while(tests_setup[index]!=-1) {
			n=tests_setup[index++];
			if(tests_setup[index]>max_positions) cc=max_positions; else cc=tests_setup[index];
			r1[pos] = rh+mx;
			index++;
			strcpy(filename, sts_tests[n]);
			mx+=cc;
			printf("Primary %s %d, %d\n", filename, cc, n);
			if((cb.handle=fopen(filename, "r"))==NULL) {
				printf("File %s, %d, %s is missing\n",filename, n, sts_tests[n]);
				goto cleanup;
			}
			cb.n=0;
			i1[q][n]=timed_driver(max_time, max_depth, cc, pi, 1, r1[pos], sts_cback, &cb);
			fclose(cb.handle);
			
			if(per2==NULL) printf("%d\n",i1[q][n]); else printf("\n");
			// prepocitani vysledku
			t1[q][n]=p1[q][n]=v1[q][n]=vt1[q][n]=0;
			
			for(f=0;f<i1[q][n];f++){
				t1[q][n]+=r1[pos][f].time;
				if(r1[pos][f].passed>0) (p1[q][n])++;
				v1[q][n]+=r1[pos][f].passed;
				vt1[q][n]+=10;
			}
			pos++;
		}

//reporting
		logger2("Details  \n====================\n");
		if(per2==NULL) printf("Details  \n====================\n");
		index=0;
		while(tests_setup[index]!=-1) {
			f=tests_setup[index++];
			if(tests_setup[index++]<=0) continue;
			logger2("Run#%d Results for STS:%d %d/%d, value %d/%d (%d), %lld\n",q ,f+1, p1[q][f],i1[q][f], v1[q][f],vt1[q][f], v1[q][f]*100/vt1[q][f], t1[q][f]);
			if(per2==NULL) printf("Run#%d Results for STS:%d %d/%d, value %d/%d (%d), %lld\n",q, f+1, p1[q][f],i1[q][f], v1[q][f],vt1[q][f], v1[q][f]*100/vt1[q][f], t1[q][f]);
		}
		
	}
	
	if(per2!=NULL) {
		free(pi);
		pi=(personality *) init_personality(per2);
		for(q=0;q<maximum_t;q++) {
			max_time=times[q];
			index=pos=mx=0;
			while(tests_setup[index]!=-1) {
				n=tests_setup[index++];
				if(tests_setup[index]>max_positions) cc=max_positions; else cc=tests_setup[index];
				r1[pos] = rh+mx;
				index++;
				strcpy(filename, sts_tests[n]);
				mx+=cc;
				printf("Secondary %s %d, %d\n", filename, cc, n);
				if((cb.handle=fopen(filename, "r"))==NULL) {
					printf("File %s, %d, %s is missing\n",filename, n, sts_tests[n]);
					goto cleanup;
				}
				cb.n=0;
				i2[q][n]=timed_driver(max_time, max_depth, cc, pi, 1, r1[pos], sts_cback, &cb);
				fclose(cb.handle);		
				t2[q][n]=p2[q][n]=v2[q][n]=vt2[q][n]=0;		
				for(f=0;f<i2[q][n];f++){
					t2[q][n]+=r1[pos][f].time;
					if(r1[pos][f].passed>0) (p2[q][n])++;
					v2[q][n]+=r1[pos][f].passed;
					vt2[q][n]+=10;
				}
				pos++;
			}
		}
	}

//reporting
	
	strcpy(b, "STS");
	for(q=0;q<maximum_t;q++) {
		sprintf(b2,"\t#%6d ",times[q]);
		strcat(b,b2);
		p1m[q]=i1m[q]=v1m[q]=vt1m[q]=0;
	}

	strcat(b,"\n");
	logger2("%s",b);
	printf("%s",b);
	
	index=0;
	while(tests_setup[index]!=-1) {
		f=tests_setup[index++];
		if(tests_setup[index++]<=0) continue;
		sprintf(b, "%d",f+1);
		for(q=0;q<maximum_t;q++) {
			p1m[q]+=p1[q][f];
			i1m[q]+=i1[q][f];
			v1m[q]+=v1[q][f];
			vt1m[q]+=vt1[q][f];
			sprintf(b2, "\t%d/%d %d/%d",p1[q][f],i1[q][f], v1[q][f],vt1[q][f]);
			strcat(b, b2);
		}
		printf("%s\n", b);
		logger2("%s\n", b);
	}
	sprintf(b, "%s","Tot");
	for(q=0;q<maximum_t;q++) {
		sprintf(b2, "\t%d/%d %d/%d",p1m[q],i1m[q], v1m[q],vt1m[q]);
		strcat(b, b2);
	}
	printf("%s\n", b);
	logger2("%s\n", b);

	if(per2!=NULL) {
	strcpy(b, "\nSEC");
	for(q=0;q<maximum_t;q++) {
		sprintf(b2,"\t#%6d ",times[q]);
		strcat(b,b2);
	}
	strcat(b,"\n");
	logger2("%s",b);
	printf("%s",b);
	
	index=0;
	while(tests_setup[index]!=-1) {
		f=tests_setup[index++];
		if(tests_setup[index++]<=0) continue;
		sprintf(b, "%d",f+1);
		for(q=0;q<maximum_t;q++) {
			sprintf(b2, "\t%d/%d %d/%d",p2[q][f],i2[q][f], v2[q][f],vt2[q][f]);
			strcat(b, b2);
		}
		printf("%s\n", b);
		logger2("%s\n", b);
	}
	}


cleanup:
	free(rh);
	free(pi);
}


void timed2STSn(int max_time, int max_depth, int max_positions, char *per1, char *per2){
sts_cb_data cb;
personality *pi;
int n, q,f;
int p1[20][20], i1[20][20], v1[20][20], vt1[20][20];
int p1m[20], i1m[20], v1m[20], vt1m[20];

unsigned long long t1[20][20];

int p2[20][20], i2[20][20], v2[20][20], vt2[20][20];
unsigned long long t2[20][20];
char b[5000], filename[512], b2[5000];
struct _results *r1[16];
struct _results *rh;

int times[]= { 100, 500, 1000, 2000,  5000, 10000, 20000, -1 }, maximum_t;
char *sts_tests[]= { "../tests/STS1-STS15_LAN_v6.epd" };
int tests_setup[]= { 0,1500, -1,-1};
int index, mx, count, pos, cc;

	LOGGER_0("STS time %d, depth %d, pos %d, pers1 %s, pers2 %s\n", max_time, max_depth, max_positions, per1, per2);

	if(per1==NULL) pi=(personality *) init_personality("pers.xml");
	else pi=(personality *) init_personality(per1);

	index=0;
	count=0;
	for(maximum_t=0; times[maximum_t]!=-1;maximum_t++) if(times[maximum_t]>max_time) break;

	while(tests_setup[index]!=-1) {
		index++;
		if(tests_setup[index]>max_positions) cc=max_positions; else cc=tests_setup[index];
		count+=cc;
		index++;
	}

	rh = malloc(sizeof(struct _results) * (count+1));
	for(q=0;q<maximum_t;q++) {
		max_time=times[q];
		index=pos=mx=0;
		while(tests_setup[index]!=-1) {
			n=tests_setup[index++];
			if(tests_setup[index]>max_positions) cc=max_positions; else cc=tests_setup[index];
			r1[pos] = rh+mx;
			index++;
			strcpy(filename, sts_tests[n]);
			mx+=cc;
			printf("Primary %s %d, %d\n", filename, cc, n);
			if((cb.handle=fopen(filename, "r"))==NULL) {
				printf("File %s, %d, %s is missing\n",filename, n, sts_tests[n]);
				goto cleanup;
			}
			cb.n=0;
			i1[q][n]=timed_driver(max_time, max_depth, cc, pi, 1, r1[pos], sts_cback, &cb);
			fclose(cb.handle);
			
			if(per2==NULL) printf("Processed %d\n",i1[q][n]); else printf("\n");
			// prepocitani vysledku
			t1[q][n]=p1[q][n]=v1[q][n]=vt1[q][n]=0;
			
			for(f=0;f<i1[q][n];f++){
				t1[q][n]+=r1[pos][f].time;
				if(r1[pos][f].passed>0) (p1[q][n])++;
				v1[q][n]+=r1[pos][f].passed;
				vt1[q][n]+=100;
			}
			pos++;
		}

//reporting
		logger2("Details  \n====================\n");
		if(per2==NULL) printf("Details  \n====================\n");
		index=0;
		while(tests_setup[index]!=-1) {
			f=tests_setup[index++];
			if(tests_setup[index++]<=0) continue;
			logger2("Run#%d Results for STS:%d %d/%d, value %d/%d (%d), %lld\n",q ,f+1, p1[q][f],i1[q][f], v1[q][f],vt1[q][f], v1[q][f]*100/vt1[q][f], t1[q][f]);
			if(per2==NULL) printf("Run#%d Results for STS:%d %d/%d, value %d/%d (%d), %lld\n",q, f+1, p1[q][f],i1[q][f], v1[q][f],vt1[q][f], v1[q][f]*100/vt1[q][f], t1[q][f]);
		}
		
	}
	
	if(per2!=NULL) {
		free(pi);
		pi=(personality *) init_personality(per2);
		for(q=0;q<maximum_t;q++) {
			max_time=times[q];
			index=pos=mx=0;
			while(tests_setup[index]!=-1) {
				n=tests_setup[index++];
				if(tests_setup[index]>max_positions) cc=max_positions; else cc=tests_setup[index];
				r1[pos] = rh+mx;
				index++;
				strcpy(filename, sts_tests[n]);
				mx+=cc;
				printf("Secondary %s %d, %d\n", filename, cc, n);
				if((cb.handle=fopen(filename, "r"))==NULL) {
					printf("File %s, %d, %s is missing\n",filename, n, sts_tests[n]);
					goto cleanup;
				}
				cb.n=0;
				i2[q][n]=timed_driver(max_time, max_depth, cc, pi, 1, r1[pos], sts_cback, &cb);
				fclose(cb.handle);		
				t2[q][n]=p2[q][n]=v2[q][n]=vt2[q][n]=0;		
				for(f=0;f<i2[q][n];f++){
					t2[q][n]+=r1[pos][f].time;
					if(r1[pos][f].passed>0) (p2[q][n])++;
					v2[q][n]+=r1[pos][f].passed;
					vt2[q][n]+=100;
				}
				pos++;
			}
		}
	}

//reporting
	
	strcpy(b, "STS");
	for(q=0;q<maximum_t;q++) {
		sprintf(b2,"\t#%6d ",times[q]);
		strcat(b,b2);
		p1m[q]=i1m[q]=v1m[q]=vt1m[q]=0;
	}

	strcat(b,"\n");
	logger2("%s",b);
	printf("%s",b);
	
	index=0;
	while(tests_setup[index]!=-1) {
		f=tests_setup[index++];
		if(tests_setup[index++]<=0) continue;
		sprintf(b, "%d",f+1);
		for(q=0;q<maximum_t;q++) {
			p1m[q]+=p1[q][f];
			i1m[q]+=i1[q][f];
			v1m[q]+=v1[q][f];
			vt1m[q]+=vt1[q][f];
			sprintf(b2, "\t%d/%d %d/%d",p1[q][f],i1[q][f], v1[q][f],vt1[q][f]);
			strcat(b, b2);
		}
		printf("%s\n", b);
		logger2("%s\n", b);
	}
	sprintf(b, "%s","Tot");
	for(q=0;q<maximum_t;q++) {
		sprintf(b2, "\t%d/%d %d/%d",p1m[q],i1m[q], v1m[q],vt1m[q]);
		strcat(b, b2);
	}
	printf("%s\n", b);
	logger2("%s\n", b);

	if(per2!=NULL) {
	strcpy(b, "\nSEC");
	for(q=0;q<maximum_t;q++) {
		sprintf(b2,"\t#%6d ",times[q]);
		strcat(b,b2);
		p1m[q]=i1m[q]=v1m[q]=vt1m[q]=0;
	}
	strcat(b,"\n");
	logger2("%s",b);
	printf("%s",b);
	
	index=0;
	while(tests_setup[index]!=-1) {
		f=tests_setup[index++];
		if(tests_setup[index++]<=0) continue;
		sprintf(b, "%d",f+1);
		for(q=0;q<maximum_t;q++) {
			p1m[q]+=p2[q][f];
			i1m[q]+=i2[q][f];
			v1m[q]+=v2[q][f];
			vt1m[q]+=vt2[q][f];
			sprintf(b2, "\t%d/%d %d/%d",p2[q][f],i2[q][f], v2[q][f],vt2[q][f]);
			strcat(b, b2);
		}
		printf("%s\n", b);
		logger2("%s\n", b);
	}

	sprintf(b, "%s","Tot");
	for(q=0;q<maximum_t;q++) {
		sprintf(b2, "\t%d/%d %d/%d",p1m[q],i1m[q], v1m[q],vt1m[q]);
		strcat(b, b2);
	}
	printf("%s\n", b);
	logger2("%s\n", b);

	}


cleanup:
	free(rh);
	free(pi);
}


void timed2Test_comp(char *filename, int max_time, int max_depth, int max_positions){
perft2_cb_data cb;
personality *pi;
int p1,p2,f,i1,i2,i;
unsigned long long t1,t2;
char b[1024];
struct _results *r1, *r2;

//	max_positions=10;
	r1 = malloc(sizeof(struct _results) * (max_positions+1));
	r2 = malloc(sizeof(struct _results) * (max_positions+1));
	pi=(personality *) init_personality("pers.xml");

// round one
	if((cb.handle=fopen(filename, "r"))==NULL) {
		printf("File %s is missing\n",filename);
		goto cleanup;
	}
	i1=timed_driver(max_time, max_depth, max_positions, pi, 0, r1, perft2_cback, &cb);
	fclose(cb.handle);
	pi=(personality *) init_personality("pers2.xml");

// round two
	if((cb.handle=fopen(filename, "r"))==NULL) {
		printf("File %s is missing\n",filename);
		goto cleanup;
	}
	i2=timed_driver(max_time, max_depth, max_positions, pi, 0, r2, perft2_cback, &cb);
	fclose(cb.handle);

// prepocitani vysledku
	t1=0;
	p1=0;
	for(f=0;f<i1;f++){
		t1+=r1[f].time;
		if(r1[f].passed>0) p1++;
	}

	t2=0;
	p2=0;
	for(f=0;f<i2;f++){
		t2+=r2[f].time;
		if(r2[f].passed>0) p2++;
	}

//reporting
	logger2("Details  \n====================\n");
	logger2("Run#1 Results %d/%d, , Time: %dh, %dm, %ds,, %lld\n",p1,i1, (int) t1/3600000, (int) (t1%3600000)/60000, (int) (t1%60000)/1000, t1);
	logger2("Run#2 Results %d/%d, , Time: %dh, %dm, %ds,, %lld\n",p2,i2, (int) t2/3600000, (int) (t2%3600000)/60000, (int) (t2%60000)/1000, t2);
	if(i1!=i2) {
		logger2("Different number of tests %d:%d!\n", i1, i2);
	} else {
		for(f=0;f<i2;f++) {
			if(r1[f].passed!=r2[f].passed) {
				logger2("Test %d results %d:%d, time %dh, %dm, %ds, %dh, %dm, %ds\n", f,r1[f].passed, r2[f].passed,(int) r1[f].time/3600000, (int) (r1[f].time%3600000)/60000, (int) (r1[f].time%60000)/1000, (int) r2[f].time/3600000, (int) (r2[f].time%3600000)/60000, (int) (r2[f].time%60000)/1000);
			}
		}
	}
	logger2("Stats\n");
	i= i1>i2 ? i2:i1;
	for(f=0;f<i;f++){
	logger2("====================\n");
		t1=r1[f].time;
		t2=r2[f].time;
		logger2("RUN #1, Position %d, , Time: %dh, %dm, %ds,, %lld\n",f, (int) t1/3600000, (int) (t1%3600000)/60000, (int) (t1%60000)/1000, t1);
		logger2("RUN #2, Position %d, , Time: %dh, %dm, %ds,, %lld\n",f, (int) t2/3600000, (int) (t2%3600000)/60000, (int) (t2%60000)/1000, t2);
		logger2("========\n");
		printSearchStat(&(r1[f].stats));
		logger2("====\n");
		printSearchStat(&(r2[f].stats));
	}
	logger2("====================\n");

cleanup:
	free(r1);
	free(r2);
	free(pi);
}

void see_test()
{
int result;
MOVESTORE move;
	board b;
	char buf[256];
	char *fen[]= {
			"1q5k/1r5p/Q2b1p1N/Pp1Pp3/8/P1r2P2/5P1P/R3R1K b - - 1 35",
			"1k1r4/1pp4p/p7/4p3/8/P5P1/1PP4P/2K1R3 w - -",
			"1k1r3q/1ppn3p/p4b2/4p3/8/P2N2P1/1PP1R1BP/2K1Q3 w - -",
			"k1b5/1N5Q/1K6/8/8/8/8/8 b - -",
			"k1b5/1N5Q/8/1K6/8/8/8/8 b - -",
			"k1b5/1N5Q/8/1R6/1K6/8/8/8 b - -"
	};
	struct _ui_opt uci_options;

	b.uci_options=&uci_options;

	b.stats=allocate_stats(1);
	b.hs=allocateHashStore(HASHSIZE, 2048);
	b.hps=allocateHashPawnStore(HASHPAWNSIZE);
	b.hht=allocateHHTable();
	b.kmove=allocateKillerStore();
	b.pers=(personality *) init_personality("pers.xml");

	setup_FEN_board(&b, fen[0]);
	printBoardNice(&b);
	move = PackMove(C3, A3,  ER_PIECE, 0);
	result=SEE(&b, move);
//	sprintfMoveSimple(move, buf);
	
	sprintfMove(&b, move, buf);
	LOGGER_0("Move %s, SEE:0:0==%d\n", buf, result);
	
	move = PackMove(D6, A3,  ER_PIECE, 0);
	result=SEE(&b, move);
	sprintfMove(&b, move, buf);
	LOGGER_0("Move %s, SEE:0:1==%d\n", buf, result);

	setup_FEN_board(&b, fen[1]);
	printBoardNice(&b);
	move = PackMove(E1, E5,  ER_PIECE, 0);
	result=SEE(&b, move);
	sprintfMove(&b, move, buf);
	LOGGER_0("Move %s, SEE:1:0==%d\n", buf, result);

	setup_FEN_board(&b, fen[2]);
	printBoardNice(&b);
	move = PackMove(D3, E5,  ER_PIECE, 0);
	result=SEE(&b, move);
	sprintfMove(&b, move, buf);
	LOGGER_0("Move %s, SEE:2:0==%d\n", buf, result);
	
	setup_FEN_board(&b, fen[3]);
	printBoardNice(&b);
	move = PackMove(C8, B7,  ER_PIECE, 0);
	result=SEE(&b, move);
	sprintfMove(&b, move, buf);
	LOGGER_0("Move %s, SEE:3:0==%d\n", buf, result);

	setup_FEN_board(&b, fen[4]);
	printBoardNice(&b);
	move = PackMove(C8, B7,  ER_PIECE, 0);
	result=SEE(&b, move);
	sprintfMove(&b, move, buf);
	LOGGER_0("Move %s, SEE:4:0==%d\n", buf, result);

	setup_FEN_board(&b, fen[5]);
	printBoardNice(&b);
	move = PackMove(C8, B7,  ER_PIECE, 0);
	result=SEE(&b, move);
	sprintfMove(&b, move, buf);
	LOGGER_0("Move %s, SEE:5:0==%d\n", buf, result);

	freeKillerStore(b.kmove);
	freeHHTable(b.hht);
	freeHashPawnStore(b.hps);
	freeHashStore(b.hs);
	deallocate_stats(b.stats);
	return;
}

void keyTest_def(void){
	char fen[100];
	char am[10][CMTLEN];
	char bm[10][CMTLEN];
	char cm[10][CMTLEN];
	char pm[256][CMTLEN];
	int dm;
	int i;
	board b;
	BITVAR key, k2;
	char * name;
	struct _ui_opt uci_options;

	b.uci_options=&uci_options;

	i=0;
	b.stats=allocate_stats(1);
	b.hs=allocateHashStore(HASHSIZE, 2048);
	b.hps=allocateHashPawnStore(HASHPAWNSIZE);
	b.hht=allocateHHTable();
	b.kmove=allocateKillerStore();
	
	while(key_default_tests[i]!=NULL) {
		if(parseEPD(key_default_tests[i], fen, am, bm, pm, cm, NULL, &dm, &name)>0) {
			if(getKeyFEN(key_default_tests[i],&key)==1) {
				setup_FEN_board(&b, fen);
				//						DEBUG_BOARD_CHECK(&b);
				DEB_4(boardCheck(&b));
				computeKey(&b, &k2);
				printf("----- Evaluate: %d -END-, %llx -----\n",i, (long long) key);
				free(name);
				if(key!=k2){
					printf("Not Match!\n");
					printBoardNice(&b);
				}
			}
		}
		i++;
	}
	freeKillerStore(b.kmove);
	freeHHTable(b.hht);
	freeHashPawnStore(b.hps);
	freeHashStore(b.hs);
	deallocate_stats(b.stats);
}

// see_0 tests
// 1k2r3/1p1bP3/2p2p1Q/Ppb5/5p1P/5N1P/5PB/4q1K w - - 1 3; tah q xx->E1


void see0_test()
{
int result;
	MOVESTORE move;
	board b;
	struct _ui_opt uci_options;

	b.uci_options=&uci_options;

	char *fen[]= {
			"1k2r3/1p1bP3/2p2p1Q/Ppb5/5p1P/5N1P/5PB/4q1K b - - 1 3",
	};

	b.stats=allocate_stats(1);
	b.pers=(personality *) init_personality("pers.xml");
	b.hs=allocateHashStore(HASHSIZE, 2048);
	b.hps=allocateHashPawnStore(HASHPAWNSIZE);
	b.hht=allocateHHTable();
	b.kmove=allocateKillerStore();

	setup_FEN_board(&b, fen[0]);
	move = PackMove(E1, E2,  ER_PIECE, 0);
	MakeMove(&b, move);
	printBoardNice(&b);
	result=SEE0(&b, E2, BLACK, 0);
	LOGGER_0("SEE0 %d\n", result);

	setup_FEN_board(&b, fen[0]);
	move = PackMove(E1, E3,  ER_PIECE, 0);
	MakeMove(&b, move);
	printBoardNice(&b);
	result=SEE0(&b, E3, BLACK, 0);
	LOGGER_0("SEE0 %d\n", result);

	setup_FEN_board(&b, fen[0]);
	move = PackMove(E1, E5,  ER_PIECE, 0);
	MakeMove(&b, move);
	printBoardNice(&b);
	result=SEE0(&b, E5, BLACK, 0);
	LOGGER_0("SEE0 %d\n", result);

	freeKillerStore(b.kmove);
	freeHHTable(b.hht);
	freeHashPawnStore(b.hps);
	deallocate_stats(b.stats);
	freeHashStore(b.hs);

	return;
}

void eeval_driver(CBACK, void *cdata)
{
int result, move, i;
int ev;
char fen[512];
char buffer[512];
char *name;
char cm[10][CMTLEN];

MOVESTORE m[10], mm;

king_eval W,B;
BITVAR t;

board b;
attack_model *a, ATT;
struct _ui_opt uci_options;
int sc, sc2, sc3, sc4, bc, ec, count;

	b.uci_options=&uci_options;
	b.stats=allocate_stats(1);
	b.pers=(personality *) init_personality("pers.xml");
	b.hs=allocateHashStore(HASHSIZE, 2048);
	b.hps=allocateHashPawnStore(HASHPAWNSIZE);
	b.hht=allocateHHTable();
	b.kmove=allocateKillerStore();
//	b.trace=0;
	a=&ATT;

		count=0;
		while(cback(buffer,cdata)) {
			if(parseEPD(buffer, fen, NULL, NULL, NULL, cm , NULL, NULL, &name)>0) {
				count++;
				setup_FEN_board(&b, fen);
				parseEDPMoves(&b, a, m, cm, 10);
				
				LOGGER_0("\n");
				LOGGER_0("Position #%d\n", count);
				printBoardNice(&b);

//				move_filter_build(cm[0] ,m);
				if(m[0]!=0) {
					m[1]=0;
					i=alternateMovGen(&b, &m[0]);
					if(i!=1) {
						LOGGER_0("move not unique\n");
						free(name);
						continue;
					}
				} else {
						LOGGER_0("move invalid/missing\n");
						free(name);
					continue;
				}

				eval(&b, a, b.pers);
// jen test zdali psq+material staci na quiesce rozhodovani
				sc2=mpsq_eval(&b, a, b.pers);
				bc=b.psq_b;
				ec=b.psq_e;
				sc3=psq_eval(&b, a, b.pers);
				sc4=get_material_eval_f(&b, b.pers);
				LOGGER_0("+ mpsq: tot ev %d:%d, mm b:e %d:%d, calc bc:ec %d:%d => sc %d, mat %d\n", sc2, a->sc.complete, bc, ec, b.psq_b, b.psq_e, sc3, sc4);
				b.psq_b=bc;
				b.psq_e=ec;

				LOGGER_0("move %s move %o\n", cm[0], m[0]);
				MakeMove(&b, m[0]);
				printBoardNice(&b);

				eval(&b, a, b.pers);
				sc2=mpsq_eval(&b, a, b.pers);
				bc=b.psq_b;
				ec=b.psq_e;
				sc3=psq_eval(&b, a, b.pers);
				sc4=get_material_eval_f(&b, b.pers);
				LOGGER_0("- mpsq: tot ev %d:%d, mm b:e %d:%d, calc bc:ec %d:%d => sc %d, mat %d\n", sc2, a->sc.complete, bc, ec, b.psq_b, b.psq_e, sc3, sc4);
				b.psq_b=bc;
				b.psq_e=ec;
				
#if 0
//				result=eval_king_checks_all(&b, a);
				result=eval_king_checks_n(&b, &(a->ke[WHITE]), NULL, WHITE);
				result=eval_king_checks(&b, &(W), NULL, WHITE);

				result=eval_king_checks_n(&b, &(a->ke[BLACK]), NULL, BLACK);
				result=eval_king_checks(&b, &(B), NULL, BLACK);
				printBoardNice(&b);
				if((a->ke[WHITE].cr_attackers^W.cr_attackers)) {
					printmask(a->ke[WHITE].cr_attackers, "cr attackers");
					printmask(W.cr_attackers, "W cr attackers");
				}
				if(a->ke[WHITE].cr_att_ray^W.cr_att_ray){
					printmask(a->ke[WHITE].cr_att_ray, "cr att ray");
					printmask(W.cr_att_ray, "W cr att ray");
				}
				if(a->ke[WHITE].cr_pins^W.cr_pins){
					printmask(a->ke[WHITE].cr_pins, "cr pins");
					printmask(W.cr_pins, "W cr pins");
				}
				if(a->ke[WHITE].di_attackers^W.di_attackers){
					printmask(a->ke[WHITE].di_attackers, "di attackers");
					printmask(W.di_attackers, "W di attackers");
				}
				if(a->ke[WHITE].di_att_ray^W.di_att_ray){
					printmask(a->ke[WHITE].di_att_ray, "di att ray");
					printmask(W.di_att_ray, "W di att ray");
				}
				if(a->ke[WHITE].di_pins^W.di_pins){
					printmask(a->ke[WHITE].di_pins, "di pins");
					printmask(W.di_pins, "W di pins");
				}
				if((a->ke[BLACK].cr_attackers^B.cr_attackers)) {
					printmask(a->ke[BLACK].cr_attackers, "cr attackers");
					printmask(B.cr_attackers, "O cr attackers");
				}
				if(a->ke[BLACK].cr_att_ray^B.cr_att_ray){
					printmask(a->ke[BLACK].cr_att_ray, "cr att ray");
					printmask(B.cr_att_ray, "O cr att ray");
				}
				if(a->ke[BLACK].cr_pins^B.cr_pins){
					printmask(a->ke[BLACK].cr_pins, "cr pins");
					printmask(B.cr_pins, "O cr pins");
				}
				if(a->ke[BLACK].di_attackers^B.di_attackers){
					printmask(a->ke[BLACK].di_attackers, "di attackers");
					printmask(B.di_attackers, "O di attackers");
				}
				if(a->ke[BLACK].di_att_ray^B.di_att_ray){
					printmask(a->ke[BLACK].di_att_ray, "di att ray");
					printmask(B.di_att_ray, "O di att ray");
				}
				if(a->ke[BLACK].di_pins^B.di_pins){
					printmask(a->ke[BLACK].di_pins, "di pins");
					printmask(B.di_pins, "O di pins");
				}
#endif

				free(name);
			}
		}
STOP:
	deallocate_stats(b.stats);
	freeKillerStore(b.kmove);
	freeHHTable(b.hht);
	freeHashPawnStore(b.hps);
	freeHashStore(b.hs);

	return;
}

void eeval_test(char * filename){
perft2_cb_data cb;
	LOGGER_0("EEVAL test\n");
	if((cb.handle=fopen(filename, "r"))==NULL) {
		printf("File %s is missing\n",filename);
		goto cleanup;
	}
	cb.lo=0;
	cb.loops=1;
	eeval_driver(perft2_cback, &cb);
	fclose(cb.handle);
cleanup:
	LOGGER_0("EEVAL test finished\n");
	printf("EEVAL test finished\n");
}

void fill_test()
{
char fen[]={"8/pk5P/1p4P1/2p2P2/3pP3/3Pp3/2P2p2/4K3 w - - 0 1"};
int i;
board b;
BITVAR res;
attack_model *a, ATT;
struct _ui_opt uci_options;
int ev;
	b.uci_options=&uci_options;
	b.stats=allocate_stats(1);
	b.pers=(personality *) init_personality("pers.xml");

	setup_FEN_board(&b, fen);
	printBoardNice(&b);
	res=FillNorth(RANK1, b.maps[PAWN]&b.colormaps[WHITE], RANK1);
	printmask(res, "RES1");
	res=FillNorth(RANK1, ~(b.maps[PAWN]&b.colormaps[WHITE]), RANK1);
	printmask(res, "RES2");
	res=FillNorth(RANK1, (b.maps[PAWN]&b.colormaps[WHITE]), 0);
	printmask(res, "RES3");
	res=FillNorth(RANK1, ~(b.maps[PAWN]&b.colormaps[WHITE]), 0);
	printmask(res, "RES4");
//	ATT.phase= eval_phase(&b, b.pers);
//	ev=eval(&b, &ATT, b.pers);
	deallocate_stats(b.stats);
}

void print_pawn_analysis(board *b, attack_model *a, PawnStore *ps, personality *p)
{
char b2[9][9];
char b3[9][256];
char *bb[9];
int f,from,s;

	DEB_3(printBoardNice(b);)

	mask2init(b2);
	mask2init2(b3, bb);

	printmask2(b->maps[PAWN]&b->colormaps[WHITE], b2, "WPs");
	mask2add(bb, b2);
	printmask2(ps->half_att[WHITE][0], b2, "Whalf0at");
	mask2add(bb, b2);
	printmask2(ps->half_att[WHITE][1], b2, "Whalf1at");
	mask2add(bb, b2);
	printmask2(ps->double_att[WHITE], b2, "Wdo at");
	mask2add(bb, b2);
	printmask2(ps->odd_att[WHITE], b2, "Wodd at");
	mask2add(bb, b2);
	printmask2(ps->safe_att[WHITE], b2, "Wsafe at");
	mask2add(bb, b2);
	printmask2(ps->paths[WHITE], b2, "Wpaths");
	mask2add(bb, b2);
	printmask2(ps->path_stop[WHITE], b2, "Wstop");
	mask2add(bb, b2);
	printmask2(ps->path_stop2[WHITE], b2, "Wstop2");
	mask2add(bb, b2);
	printmask2(ps->pass_end[WHITE], b2, "Wend");
	mask2add(bb, b2);
	mask2print(bb);

	mask2init(b2);
	mask2init2(b3, bb);

	printmask2(b->maps[PAWN]&b->colormaps[BLACK], b2, "BPs");
	mask2add(bb, b2);
	printmask2(ps->half_att[BLACK][0], b2, "Bhalf0at");
	mask2add(bb, b2);
	printmask2(ps->half_att[BLACK][1], b2, "Bhalf1at");
	mask2add(bb, b2);
	printmask2(ps->double_att[BLACK], b2, "Bdo at");
	mask2add(bb, b2);
	printmask2(ps->odd_att[BLACK], b2, "Bodd at");
	mask2add(bb, b2);
	printmask2(ps->safe_att[BLACK], b2, "Bsafe at");
	mask2add(bb, b2);
	printmask2(ps->paths[BLACK], b2, "B sf paths");
	mask2add(bb, b2);
	printmask2(ps->path_stop[BLACK], b2, "Bstop");
	mask2add(bb, b2);
	printmask2(ps->path_stop2[BLACK], b2, "Bstop2");
	mask2add(bb, b2);
	printmask2(ps->pass_end[BLACK], b2, "Bend");
	mask2add(bb, b2);
	mask2print(bb);

	mask2init(b2);
	mask2init2(b3, bb);

	printmask2(ps->stopped[WHITE], b2, "Wstop");
	mask2add(bb, b2);
	printmask2(ps->passer[WHITE], b2, "Wpasser");
	mask2add(bb, b2);
	printmask2(ps->blocked[WHITE], b2, "Wblocked");
	mask2add(bb, b2);
	printmask2(ps->half_isol[WHITE][0], b2, "Wisol0");
	mask2add(bb, b2);
	printmask2(ps->half_isol[WHITE][1], b2, "Wisol1");
	mask2add(bb, b2);
	printmask2(ps->doubled[WHITE], b2, "Wdoubled");
	mask2add(bb, b2);
	printmask2(ps->back[WHITE], b2, "Wback");
	mask2add(bb, b2);
	printmask2(ps->prot[WHITE], b2, "Wprot");
	mask2add(bb, b2);
	printmask2(ps->prot_p[WHITE], b2, "Wprot_p");
	mask2add(bb, b2);
	printmask2(ps->not_pawns_file[WHITE], b2, "Wnot_p");
	mask2add(bb, b2);
	mask2print(bb);

	mask2init(b2);
	mask2init2(b3, bb);

	printmask2(ps->stopped[BLACK], b2, "Bstop");
	mask2add(bb, b2);
	printmask2(ps->passer[BLACK], b2, "Bpasser");
	mask2add(bb, b2);
	printmask2(ps->blocked[BLACK], b2, "Bblocked");
	mask2add(bb, b2);
	printmask2(ps->half_isol[BLACK][0], b2, "Bisol0");
	mask2add(bb, b2);
	printmask2(ps->half_isol[BLACK][1], b2, "Bisol1");
	mask2add(bb, b2);
	printmask2(ps->doubled[BLACK], b2, "Bdoubled");
	mask2add(bb, b2);
	printmask2(ps->back[BLACK], b2, "Bback");
	mask2add(bb, b2);
	printmask2(ps->prot[BLACK], b2, "Bprot");
	mask2add(bb, b2);
	printmask2(ps->prot_p[BLACK], b2, "Bprot_p");
	mask2add(bb, b2);
	printmask2(ps->not_pawns_file[BLACK], b2, "Bnot_p");
	mask2add(bb, b2);
	mask2print(bb);
	
	mask2init(b2);
	mask2init2(b3, bb);
	printmask2(ps->not_pawns_file[WHITE], b2, "Wnot");
	mask2add(bb, b2);
	printmask2(ps->not_pawns_file[BLACK], b2, "Bnot");
	mask2add(bb, b2);
	mask2print(bb);

	for(s=0;s<=1;s++) {
		f=0;
		from=ps->pawns[s][f];
		while(from!=-1) {
			logger2("Side: %d, from %d, pas %d, stop %d, block %d, double %d, outp %d, outp_d %d, prot FR %d, prot BH %d, prot DIR %d\n",
				s, from, ps->pas_d[s][f], ps->stop_d[s][f], ps->block_d[s][f], ps->double_d[s][f], ps->outp[s][f], 
				ps->outp_d[s][f], ps->prot_d[s][f], ps->prot_p_d[s][f], ps->prot_dir_d[s][f]);
			f++;
			from=ps->pawns[s][f];
		}
	}

return;
}

int driver_pawn_eval(int max,personality *pers_init, CBACK, void *cdata)
{
char buffer[512], fen[100];
char bx[512];
int i;
board b;
PawnStore ps;
struct _statistics *stat;

attack_model a;
struct _ui_opt uci_options;
struct _statistics s;
int ev, ph;
char * name;

	b.stats=allocate_stats(1);
	b.pers=pers_init;
	b.hs=allocateHashStore(HASHSIZE, 2048);
	b.hps=allocateHashPawnStore(HASHPAWNSIZE);
	b.hht=allocateHHTable();
	b.kmove=allocateKillerStore();
	b.uci_options=&uci_options;

	stat = allocate_stats(1);

	i=0;

// personality should be provided by caller
	i=0;
	while(cback(bx, cdata)&&(i<max)) {
		if(parseEPD(bx, fen, NULL, NULL, NULL, NULL, NULL, NULL, &name)>0) {

			setup_FEN_board(&b, fen);
//			LOGGER_3("%d: %s\n", i, fen);
			ph= eval_phase(&b, pers_init);
			printBoardNice(&b);
			premake_pawn_model(&b, &a, &(a.hpe), pers_init);
			print_pawn_analysis(&b, &a, &(a.hpe.value), pers_init);
			free(name); 
			i++;
		}
	}

	freeKillerStore(b.kmove);
	freeHHTable(b.hht);
	freeHashPawnStore(b.hps);
	freeHashStore(b.hs);
	deallocate_stats(stat);
	deallocate_stats(b.stats);
	return i;
}

void pawnEvalTest(char *filename, int max_positions){
perft2_cb_data cb;
personality *pi;
int p1,f,i1,i;
unsigned long long t1;
char b[1024];

	printf("Pawn Eval Test start\n");
	pi=(personality *) init_personality("pers.xml");
	if((cb.handle=fopen(filename, "r"))==NULL) {
		printf("File %s is missing\n",filename);
		goto cleanup;
	}
	i1=driver_pawn_eval(max_positions, pi, perft2_cback, &cb);
	fclose(cb.handle);
	printf("Pawn Eval Test finish\n");

cleanup:
	free(pi);
}

int driver_king_check_test(int max,personality *pers_init, CBACK, void *cdata)
{
char buffer[512], fen[100];
char bx[512];
int i;
board b;
PawnStore ps;
struct _statistics *stat;

attack_model a;
struct _ui_opt uci_options;
struct _statistics s;
int ev, ph;
char * name;

	b.stats=allocate_stats(1);
	b.pers=pers_init;
	b.hs=allocateHashStore(HASHSIZE, 2048);
	b.hps=allocateHashPawnStore(HASHPAWNSIZE);
	b.hht=allocateHHTable();
	b.kmove=allocateKillerStore();
	b.uci_options=&uci_options;

	stat = allocate_stats(1);

	i=0;

// personality should be provided by caller
	i=0;
	while(cback(bx, cdata)&&(i<max)) {
		if(parseEPD(bx, fen, NULL, NULL, NULL, NULL, NULL, NULL, &name)>0) {

			setup_FEN_board(&b, fen);
			printBoardNice(&b);
//			printboard(b);
			eval_king_checks_n(&b, &(a.ke[b.side]), pers_init, b.side);
			free(name);
			i++;
		}
	}

	freeKillerStore(b.kmove);
	freeHHTable(b.hht);
	freeHashPawnStore(b.hps);
	freeHashStore(b.hs);
	deallocate_stats(stat);
	deallocate_stats(b.stats);
	return i;
}

void king_check_test(char *filename, int max_positions){
perft2_cb_data cb;
personality *pi;
int p1,f,i1,i;
unsigned long long t1;
char b[1024];

	printf("King Check Test start\n");
	pi=(personality *) init_personality("pers.xml");
	if((cb.handle=fopen(filename, "r"))==NULL) {
		printf("File %s is missing\n",filename);
		goto cleanup;
	}
	i1=driver_king_check_test(max_positions, pi, perft2_cback, &cb);
	fclose(cb.handle);
	printf("King Check Test finish\n");

cleanup:
	free(pi);
}

#if 0
eval requirements

	att->ke[side]=tolev->ke[side];
	att->att_by_side[opside]=KingAvoidSQ(b, att, opside);

	eval_king_checks_all(b, att);

	simple_pre_movegen_n2(b, a, Flip(side));
	simple_pre_movegen_n2(b, a, side);

#endif


void EvalCompare(char *pn1[], int pns, char *testfile[], int tss, int threshold){
perft2_cb_data cb;
personality *p1[100] ;
int f,i1,i,ff;
unsigned long long t1;
char buffer[1024], rrr[256], fen[256];
char *name, *xx;

board b1[100];
attack_model *a1[100], ATT1[100];
struct _ui_opt uci_options;
int sc, sct1[100][4], scf1, scu1, sct2[4], scf2, scu2, sc1, sc2, count, res;

int lo=pns;

	for(f=0;f<lo; f++) {
		b1[f].uci_options=&uci_options;
		b1[f].stats=allocate_stats(1);
		b1[f].hs=allocateHashStore(HASHSIZE, 2048);
		b1[f].hps=allocateHashPawnStore(HASHPAWNSIZE);
		b1[f].hht=allocateHHTable();
		b1[f].kmove=allocateKillerStore();
//		b1[f].trace=0;
		a1[f]=&(ATT1[f]);
	}
	


	printf("Eval comparison start\n");
	LOGGER_0("Eval comparison start\n");

	for(f=0;f<pns; f++) {
		p1[f]=(personality *) init_personality(pn1[f]);
		b1[f].pers=p1[f];
	}


	for(ff=0;ff<tss;ff++) {
	if((cb.handle=fopen(testfile[ff], "r"))==NULL) {
		printf("File %s is missing\n",testfile[ff]);
		goto cleanup;
	}
	
	count=0;
	for(f=0;f<lo;f++) {
		for(i=0;i<4;i++) sct1[f][i]=0;
	}
	
    while (!feof (cb.handle))
	{
	  xx = fgets (buffer, 511, cb.handle);
		{
		  // get FEN
		  if (parseEPD (buffer, fen, NULL, NULL, NULL, NULL, rrr, NULL, &name) > 0)
			{
			  if (!strcmp (rrr, "1-0"))
				res = 2;
			  else if (!strcmp (rrr, "0-1"))
				res = 0;
			  else if (!strcmp (rrr, "1/2-1/2"))
				res = 1;
			  else
				{
				  printf ("Result parse error\n");
				  abort ();
				}
			  count++;
//			  if((count%10000)==0) printf("count %d\n", count);
//			  LOGGER_0("FEN %s\n", fen);
			  
			  for(f=0;f<lo;f++) {
				  setup_FEN_board(&(b1[f]), fen);
				  eval_king_checks_all(&(b1[f]), a1[f]);
				  simple_pre_movegen_n2(&(b1[f]), a1[f], Flip(b1[f].side));
				  simple_pre_movegen_n2(&(b1[f]), a1[f], b1[f].side);

				  eval(&(b1[f]), a1[f], p1[f]);
			  }
			  free(name);
			  
			  for(f=0;f<lo;f++) {
				  if(res==2) {
					if(a1[f]->sc.complete>=1000) sct1[f][0]++;
					if(a1[f]->sc.complete>=5000) sct1[f][1]++;
					if(a1[f]->sc.complete>=10000) sct1[f][2]++;
				  }
				  else if(res==0){
					if(a1[f]->sc.complete<=-1000) sct1[f][0]++;
					if(a1[f]->sc.complete<=-5000) sct1[f][1]++;
					if(a1[f]->sc.complete<=-10000) sct1[f][2]++;
				  }
				  else if(res==1) {
					if((a1[f]->sc.complete<= 1000)&&(a1[f]->sc.complete>= -1000)) sct1[f][0]++;
					if((a1[f]->sc.complete<= 5000)&&(a1[f]->sc.complete>= -5000)) sct1[f][1]++;
					if((a1[f]->sc.complete<= 10000)&&(a1[f]->sc.complete>= -10000)) sct1[f][2]++;
				  }
			  }
			}
		}
	}
	fclose(cb.handle);
	
	printf("File %s\n", testfile[ff]);
	LOGGER_0("File %s\n", testfile[ff]);
	for(f=0;f<lo;f++) {
		printf("P%d %2.2f%%, %2.2f%%, %2.2f%%, %d, %d, %d, %d\n", f, sct1[f][0]*100.0/count, sct1[f][1]*100.0/count, sct1[f][2]*100.0/count, count, sct1[f][0], sct1[f][1], sct1[f][2]);
		LOGGER_0("P%d %2.2f%%, %2.2f%%, %2.2f%%, %d, %d, %d, %d\n", f, sct1[f][0]*100.0/count, sct1[f][1]*100.0/count, sct1[f][2]*100.0/count, count, sct1[f][0], sct1[f][1], sct1[f][2]);
	}
	
	}
	printf("Eval comparison finish\n");
	LOGGER_0("Eval comparison finish\n");

cleanup:
	for(f=0;f<lo;f++) {
		free(p1[f]);

		freeKillerStore(b1[f].kmove);
		freeHHTable(b1[f].hht);
		freeHashPawnStore(b1[f].hps);
		freeHashStore(b1[f].hs);
		deallocate_stats(b1[f].stats);
	}

}

// 5rk/bb3p1p/1p1p1qp/pBpP4/N1Pp2P/P2Q3P/1P3PK/3R4 w - c6 1 30; move d5c6

int driver_eval_checker(int max,personality *pers_init, CBACK, void *cdata)
{
char buffer[512], fen[100];
char bx[512];
int i;
board b;
struct _statistics *stat;

attack_model a;
struct _ui_opt uci_options;
struct _statistics s;
int ev, ph;
char * name;
int fullrun;
PawnStore *ps=&a.pps;
int side, opside, from, f, ff, idx;

	b.stats=allocate_stats(1);
	b.pers=pers_init;
	b.hs=allocateHashStore(HASHSIZE, 2048);
	b.hps=allocateHashPawnStore(HASHPAWNSIZE);
	b.hht=allocateHHTable();
	b.kmove=allocateKillerStore();
	b.uci_options=&uci_options;

	stat = allocate_stats(1);

	i=0;

// personality should be provided by caller
	i=0;
	while(cback(bx, cdata)&&(i<max)) {
		if(parseEPD(bx, fen, NULL, NULL, NULL, NULL, NULL, NULL, &name)>0) {
			setup_FEN_board(&b, fen);
			printBoardNice(&b);
			lazyEval(&b, &a, -iINFINITY, iINFINITY, WHITE, 0, 1, pers_init, &fullrun);

int vars[]= { BAs, HEa, SHa, SHh, SHm, SHah, SHhh, SHmh, -1 };

			for(side=0;side<=1;side++) {
				opside = Flip(side);
				f=0;
				from=ps->pawns[side][f];
				while(from!=-1) {
					LOGGER_0("P:%d:%d:%o=",side,f,from);
					ff=0;
					while(vars[ff]!=-1) {
						idx=vars[ff];
						NLOGGER_0("{%d:%d}",ps->t_sc[side][f][idx].sqr_b,ps->t_sc[side][f][idx].sqr_e);
						ff++;
					}
					NLOGGER_0("\n");
					from=ps->pawns[side][++f];
				}
				LOGGER_0("P:%d=",side);
				ff=0;
				while(vars[ff]!=-1) {
					idx=vars[ff];
					NLOGGER_0("{%d:%d}",ps->score[side][idx].sqr_b, ps->score[side][idx].sqr_e);
					ff++;
				}
				NLOGGER_0("\n");
			}
			LOGGER_0("Score %d, %d:%d\n", a.sc.complete, a.sc.score_b, a.sc.score_e);
			free(name);
			i++;
		}
	}

	freeKillerStore(b.kmove);
	freeHHTable(b.hht);
	freeHashPawnStore(b.hps);
	freeHashStore(b.hs);
	deallocate_stats(stat);
	deallocate_stats(b.stats);
	return i;
}

void eval_checker(char *filename, int max_positions){
perft2_cb_data cb;
personality *pi;
int p1,f,i1,i;
unsigned long long t1;
char b[1024];

	printf("Eval Checker start\n");
	pi=(personality *) init_personality("pers.xml");
	if((cb.handle=fopen(filename, "r"))==NULL) {
		printf("File %s is missing\n",filename);
		goto cleanup;
	}
	cb.loops=1;
	i1=driver_eval_checker(max_positions, pi, perft2_cback, &cb);
	fclose(cb.handle);
	printf("Eval Checker finish\n");

cleanup:
	free(pi);
}

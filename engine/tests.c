/*
 *
 * $Id: tests.c,v 1.15.2.11 2006/02/13 23:08:39 mrt Exp $
 *
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
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

char *perft_default_tests[]={"r4rk1/pp2ppNp/2pp2p1/6q1/8/P2nPPK1/1P1P2PP/R1B3NR w - - 0 1 perft 1 = 1; jumping over pieces under check;",
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
							"8/k1P5/8/1K6/8/8/8/8 w - - 0 1 perft 7 = 567584 ; id X stalemate/checkmate;",
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
	for(i=0;i<102;i++) if(dest->positions[i]!=source->positions[i]) { printf("POSITIONS %d!\n",i); triggerBoard(); }
	for(i=0;i<102;i++) if(dest->posnorm[i]!=source->posnorm[i]) { printf("POSNORM %d!\n",i); triggerBoard(); }
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

	copyBoard(z,b);

	m=move;
	att.phase=eval_phase(b);
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
return 0;
}

/*
 * tahy jsou odd�leny mezerou
 * moznosti (whitespace)*(Alnum)+(whitespace)*
			(whitespace)*"(cokoliv krome ")+"(whitespace)*
 */

int getEPDmoves(char *bu, char (*m)[20])
{
	int i, ap, c, x;
	char *b2;
	(*m)[0]='\0';
	if(bu==NULL) return 0;
	x=strlen(bu);
	while(x>0) {
		while(isspace(*bu)&&(x>0)) {
			bu++;
			x--;
		}
		if(x>0) {
			if(*bu=='\"') {
				b2=strstr(bu, "\"");
				bu++;
				if(b2!=NULL) ap=b2-bu-1; else ap=x;
				ap--;
				if(ap>0) {
					strncpy(*m, bu, ap);
					m++;
					i++;
					x-=ap;
					bu+=ap;
				}
			} else {
				c=0;
				while(!isspace(*bu)&&(x>0)) {
					(*m)[c++]=*bu;
					bu++;
					x--;
				}
				(*m)[c++]='\0';
//				printf("%s\n", *m);
				m++;
				i++;
			}
		}
	}
	(*m)[0]='\0';
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

	strncpy(f, z1, zz-z1);
	f[zz-z1]='\0';
return 1;
}

int get_token(char *st,int first, char *del, int *b, int *e) {
int l, f, i, dl, ret;
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
 */

int parseEPD(char * buffer, char FEN[100], char (*am)[20], char (*bm)[20], char (*pv)[20], int *matec, char **name)
{
char * an, *endp;
char b[256], token[256];
int count;
unsigned int f;
int s,e,l,i;
// get FEN
//			printf("buffer: %s\n", buffer);

			(*am)[0]='\0';
			(*bm)[0]='\0';
			*matec=-1;
			*name=NULL;
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

			strncpy(FEN,buffer, f);
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

			if(getEPD_str(an, "id ", b)) {
				*name=(char *) malloc(strlen(b));
				strcpy(*name, b);
			} else {
				*name=(char *)malloc(1);
				(*name)[0]='\0';
			}

			if(getEPD_str(an, "am ", b)) {
				am[0][0]='\0';
				getEPDmoves(b, am);
			}

			if(getEPD_str(an, "bm ", b)) {
				bm[0][0]='\0';
				getEPDmoves(b, bm);
			}

			if(getEPD_str(an, "dm ", b)) {
				*matec= atoi(b);
			}

			pv[0][0]=0;
			if(getEPD_str(an, "pv ", b)) {
				getEPDmoves(b, pv);
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

int evaluateAnswer(board *b, int ans, int adm ,int *aans, int *bans, int *pv, int dm, tree_store *t){
	int as, ad, ap, src, des, p, res, prom_need, ba;

	as=UnPackFrom(ans);
	ad=UnPackTo(ans);
	ap=UnPackProm(ans);
	//	asp=UnPackSpec(ans);
	prom_need=0;
	if((b->side==WHITE) && (ad>=A8) && (b->pieces[as]==PAWN)) prom_need=1;
	else if((b->side==BLACK) && (ad>=H1) && (b->pieces[as]==PAWN)) prom_need=1;

	ba=res=0;
	while(*bans!=0) {
		src=UnPackFrom(*bans);
		des=UnPackTo(*bans);
		p=UnPackProm(*bans);
		ba++;
		if((src==as)&&(des==ad)) {
			if((prom_need!=0)) {
				if (ap==p) res=1;
			} else res=1;
		} else {
//			printf("Error:NON match S,D,P: %o-%o, %o-%o, %o-%o\n", src,as,des,ad,p, ap);
		}
		bans++;
	}
	if(ba==0) res=1;
	while(*aans!=0) {
		src=UnPackFrom(*aans);
		des=UnPackTo(*aans);
		p=UnPackProm(*aans);
		if((src==as)&&(des==ad)) {
			if((prom_need!=0)) {
				if (ap==p) res|=2;
			} else res|=2;
		}
		aans++;
	}
	if(dm>0) {
//get full moves from adm
		if(adm!=dm) res|=4;
	}

	if(pv!=NULL) if(!matchLine(pv,t)) res|=8;
	return res;
}

//move
//  [Piece][Zdroj][X]DestP[=?|e.p.][+]
// 0-0 0-0-0

int parseOneMove(board *b, char *m) 
{
int l,zl,sl, ll,tl, r,c,p, pp, sr, sf, sp, des, ep_t, p_pole, src, prom_need, cap;
BITVAR aa, xx, bb;
int mm[2];

int res=-1;


		cap=0;
		sr=sf=-1;
		p=sp=PAWN;
		prom_need=0;

		if(strstr(m, "O-O-O")!=NULL) {
			sp=KING;
			p=ER_PIECE;
			sf=4;
			c=6;
			if(b->side==WHITE) {
				sr=0;
				r=0;
			} else {
				sr=7;
				r=7;
				//break
			}
			goto POKR;
		}
		if(strstr(m, "O-O")!=NULL) {
			sp=KING;
			p=ER_PIECE;
			sf=4;
			c=2;
			if(b->side==WHITE) {
				sr=0;
				r=0;
			} else {
				sr=7;
				r=7;
			}
			goto POKR;
		}
		//sync on destination
		ll=l=strlen(m);
		if(l<2) return -1;
		for(;l>0;l--) {
			if(isdigit(m[l])) break;
		}
		if(l==0) return -1;
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
					return -1;
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
POKR:

/*
 * mame limitovanou informaci, zdali jde o tah, brani, jakou figurou a mozna odkud
 * je na case najit pozici figurky
 */
// kontrola
		des=r*8+c;
// ep test
		xx=0;
		if(sp==PAWN) {
			if(cap!=0) {
				if(b->ep!=-1) {
					if(b->side==WHITE) ep_t=b->ep+8;
					else ep_t=b->ep-8;
					if(des==ep_t) {
						xx=(attack.ep_mask[b->ep]) & (b->maps[PAWN]) & (b->colormaps[b->side]);
					}
				}
			} else {
				if(b->side==WHITE) p_pole=des-8;
				else p_pole=des+8;
				xx = xx | normmark[p_pole];
				if(b->side==WHITE && r==3) xx = xx | normmark[p_pole-8];
				else if(b->side==BLACK && r==4) xx = xx | normmark[p_pole+8];
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
		if(BitCount(aa)!=1) {
			LOGGER_3("EPDmove parse problem, bitcount %d!\n", BitCount(aa));
		} else {
			if(prom_need==1 && p==PAWN) p=QUEEN;
			src=LastOne(aa);
			res = PackMove(src, des,  p, 0);
			mm[0]=1;
			mm[1]=res;
			if(validatePATHS(b, &(mm[0]))!=1) res=-1;
			else res=mm[1];
		}
return res;
}

int parseEDPMoves(board *b, int *ans, char (*bm)[20])
{
	char b2[256];
	while((*bm)[0]!='\0') {
		*ans=parseOneMove(b, *bm);
		if(*ans!=-1) {
			DEB_1(sprintfMove(b, *ans, b2));
			LOGGER_1("Move: %s\n",b2);
			ans++;
			}
		bm++;
	}
	*ans=0;
return 1;
}

int parsePVMoves(board *b, int *ans, char (*bm)[20])
{
UNDO u[256]	;
attack_model att;
int mm[2];
int f,i,r, *z;
	char b2[256];
	printBoardNice(b);

	z=ans;
	ans++;
	f=1;
	mm[1]=0;
	while((*bm)[0]!='\0') {
		mm[0]=parseOneMove(b, *bm);
		if(mm[0]!=-1) {
			DEB_1(sprintfMove(b, mm[0], b2));
			LOGGER_1("Move: %s\n",b2);
			i=alternateMovGen(b, mm);
			if(i!=1) {
				LOGGER_2("INFO3: move problem!\n");
				break;
			}
			eval(b, &att, b->pers);
			u[f]=MakeMove(b, mm[0]);
			printBoardNice(b);
			f++;
			*ans=mm[0];
			ans++;
		}
		bm++;
	}
	*ans=0;
	f--;
	*z=f;

	for(;f>0;f--) {
	 UnMakeMove(b, u[f]);
	}

	printBoardNice(b);
	r=1;
return r;
}


void movegenTest(char *filename)
{
	{
	char buffer[512], fen[100];
	char am[10][20];
	char bm[10][20];
	char pm[256][20];
	int dm;
	FILE * handle;
	int i;
	board b;

	char * name;

			if((handle=fopen(filename, "r"))==NULL) {
				printf("File %s is missing\n",filename);
				return;
			}
			fgets(buffer, 511, handle);
			i=0;

			b.pers=(personality *) init_personality("pers.xml");

			while(!feof(handle)) {
				if(parseEPD(buffer, fen, am, bm, pm, &dm, &name)==1) {
					setup_FEN_board(&b, fen);
					printBoardNice(&b);
					printf("----- MoveGenTest, name:%s -----\n",name);
					DEB_4(boardCheck(&b));
					moveGenTest1(&b);
					free(name);
				}
				i++;
				fgets(buffer, 511, handle);
			}
			free(b.pers);
			fclose(handle);
	}
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

unsigned long long int perftLoop(board *b, int d, int side){
UNDO u;
move_entry move[300], *m, *n;
int tc, cc, opside, incheck;
unsigned long long nodes, tnodes;
attack_model *a, ATT;

	if (d==0) return 1;

	nodes=0;
	opside = (side == WHITE) ? BLACK : WHITE;
//	a=&(aa[d]);
	a=&ATT;

//	a->phase=eval_phase(b);
//	eval(b, a, b->pers);
	a->phase=eval_phase(b);
	eval_king_checks_all(b, a);
	simple_pre_movegen(b, a, b->side);
	simple_pre_movegen(b, a, opside);

	if(isInCheck_Eval(b, a, opside)!=0) {
		log_divider("OPSIDE in check!");
		printBoardNice(b);
		printboard(b);
		printf("Opside in check!\n");
		return 0;
	}
	if(isInCheck_Eval(b, a, side)!=0) {
		incheck=1;
	}	else incheck=0;

	m=move;
	n=move;
	if(incheck==1) {
		generateInCheckMoves(b, a, &m);
	} else {
		generateCaptures(b, a, &m, 1);
		generateMoves(b, a, &m);
	}

//	hashmove=DRAW_M;
//	tc=sortMoveList_Init(b, a, hashmove, move, m-n, d, m-n );
	tc=m-n;
//	printBoardNice(b);
//	dump_moves(b, move, m-n );
	cc = 0;
	if(d==1) return tc;

	while (cc<tc) {
//		readClock_wall(&start);
//		sprintfMove(b, move[cc].move, buf);
//		sprintfMoveSimple(move[cc].move, buf);
		u=MakeMove(b, move[cc].move);
//		writeEPD_FEN(b, fen, 0,"");
		tnodes=perftLoop(b, d-1, opside);
		nodes+=tnodes;
		UnMakeMove(b, u);
		cc++;
	}
return nodes;
}


unsigned long long int perftLoop_divide(board *b, int d, int side){
UNDO u;
move_entry move[300], *m, *n;
int tc, cc, opside, incheck;
unsigned long long nodes, tnodes;
attack_model *a, ATT;
struct timespec start, end;
unsigned long long int totaltime;
char buf[20], fen[100];

	if (d==0) return 1;

	nodes=0;
	opside = (side == WHITE) ? BLACK : WHITE;
//	a=&(aa[d]);
	a=&ATT;

//	a->phase=eval_phase(b);
//	eval(b, a, b->pers);
	a->phase=eval_phase(b);
	eval_king_checks_all(b, a);
	simple_pre_movegen(b, a, b->side);
	simple_pre_movegen(b, a, opside);

	if(isInCheck_Eval(b, a, opside)!=0) {
		log_divider("OPSIDE in check!");
		printBoardNice(b);
		printboard(b);
		printf("Opside in check!\n");
		return 0;
	}
	if(isInCheck_Eval(b, a, side)!=0) {
		incheck=1;
	}	else incheck=0;

	m=move;
	n=move;
	if(incheck==1) {
		generateInCheckMoves(b, a, &m);
	} else {
		generateCaptures(b, a, &m, 1);
		generateMoves(b, a, &m);
	}

//	hashmove=DRAW_M;
//	tc=sortMoveList_Init(b, a, hashmove, move, m-n, d, m-n );
	tc=m-n;
//	printBoardNice(b);
//	dump_moves(b, move, m-n );
	cc = 0;
//	if(d==1) return tc;

	while (cc<tc) {
		readClock_wall(&start);
//		sprintfMove(b, move[cc].move, buf);
		sprintfMoveSimple(move[cc].move, buf);
		u=MakeMove(b, move[cc].move);
		writeEPD_FEN(b, fen, 0,"");
		tnodes=perftLoop(b, d-1, opside);
		nodes+=tnodes;
		UnMakeMove(b, u);
		readClock_wall(&end);
		totaltime=diffClock(start, end);
		printf("%s\t\t%lld\t\t(%lld:%lld.%lld\t%lld tis/sec,\t\t%s)\n", buf, tnodes, totaltime/60000000,(totaltime%60000000)/1000000,(totaltime%1000000)/1000, tnodes*1000/totaltime, fen );
		LOGGER_1("%s\t\t%lld\t\t(%lld:%lld.%lld\t%lld tis/sec,\t\t%s)\n", buf, tnodes, totaltime/60000000,(totaltime%60000000)/1000000,(totaltime%1000000)/1000, tnodes*1000/totaltime, fen );
		cc++;
	}
return nodes;
}

unsigned long long int perftLoop_divide_N(board *b, int d, int side){
UNDO u;
move_entry move[300], *m, *n;
int tc, cc, hashmove, opside, incheck;
unsigned long long nodes, tnodes;
attack_model *a, ATT;
struct timespec start, end;
unsigned long long int totaltime;
char buf[20], fen[100];

	if (d==0) return 1;

	nodes=0;
	opside = (side == WHITE) ? BLACK : WHITE;
//	a=&(aa[d]);
	a=&ATT;

	a->phase=eval_phase(b);
//	eval(b, a, b->pers);

	eval_king_checks_all(b, a);
	if(isInCheck_Eval(b, a, opside)!=0) {
		log_divider("OPSIDE in check!");
		printBoardNice(b);
		printboard(b);
		printf("Opside in check!\n");
		return 0;
	}
	if(isInCheck_Eval(b, a, side)!=0) {
		incheck=1;
	}	else incheck=0;

	simple_pre_movegen(b, a, b->side);
	simple_pre_movegen(b, a, opside);
//	eval(b, a, b->pers);

	
	m=move;
	n=move;
	if(incheck==1) {
		generateInCheckMoves(b, a, &m);
	} else {
		generateCaptures(b, a, &m, 1);
		generateMoves(b, a, &m);
	}

	hashmove=DRAW_M;
	tc=MoveList_Legal(b, a, hashmove, move, m-n, d, m-n );
//	printBoardNice(b);
//	dump_moves(b, move, m-n );
	cc = 0;
	if(d==1) return tc;

	while (cc<tc) {
		readClock_wall(&start);
//		sprintfMove(b, move[cc].move, buf);
		sprintfMoveSimple(move[cc].move, buf);
		u=MakeMove(b, move[cc].move);
		writeEPD_FEN(b, fen, 0,"");
		tnodes=perftLoop(b, d-1, opside);
		nodes+=tnodes;
		UnMakeMove(b, u);
		readClock_wall(&end);
		totaltime=diffClock(start, end);
		printf("%s\t\t%lld\t\t(%lld:%lld.%lld\t%lld tis/sec,\t\t%s)\n", buf, tnodes, totaltime/60000000,(totaltime%60000000)/1000000,(totaltime%1000000)/1000, tnodes*1000/totaltime, fen );
		LOGGER_1("%s\t\t%lld\t\t(%lld:%lld.%lld\t%lld tis/sec,\t\t%s)\n", buf, tnodes, totaltime/60000000,(totaltime%60000000)/1000000,(totaltime%1000000)/1000, tnodes*1000/totaltime, fen );
		cc++;
	}
return nodes;
}

// callback funkce
#define CBACK int (*cback)(char *fen, void *data)

void perft_driver(int min, int max, int sw, CBACK, void *cdata)
{
char buffer[512], fen[100];
char am[10][20];
char bm[10][20];
char pm[256][20];
int dm;
int i;
board b;
unsigned long long int nodes, counted;
int depth;
char * name;
struct timespec start, end, st, et;
unsigned long long int totaltime, nds;

unsigned long long int (*loop)(board *b, int d, int side);

		switch(sw) {
			case 1: loop=&perftLoop_divide;
					break;
			case 2: loop=&perftLoop_divide_N;
					break;
			default:
					loop=&perftLoop;
		}

		b.pers=(personality *) init_personality("pers.xml");
		
//		pininit();

		nds=0;
		i=1;
		readClock_wall(&st);
		while(cback(buffer,cdata)) {
			if(parseEPD(buffer, fen, am, bm, pm, &dm, &name)>0) {
				if(getPerft(buffer,&depth,&nodes)==1) {
					setup_FEN_board(&b, fen);

					LOGGER_1("----- Evaluate:%d Begin, Depth:%d, Nodes Exp:%llu; %s -----\n",i, depth, nodes, name);

					printf("----- Evaluate:%d Begin, Depth:%d, Nodes Exp:%llu; %s -----\n",i, depth, nodes, name);
//					DCount=depth;
					readClock_wall(&start);
					if((min<=depth)&&(depth<=max)) {
						counted=loop(&b, depth, b.side);
						readClock_wall(&end);
						totaltime=diffClock(start, end);
						printf("----- Evaluate:%d -END-, Depth:%d, Nodes Cnt:%llu, Time: %lld:%lld.%lld; %lld tis/sec,  %s -----\n",i, depth, counted,totaltime/60000000,(totaltime%60000000)/1000000,(totaltime%1000000)/1000, (counted*1000/totaltime), name);
						LOGGER_1("----- Evaluate:%d -END-, Depth:%d, Nodes Cnt:%llu, Time: %lld:%lld.%lld; %lld tis/sec,  %s -----\n",i, depth, counted,totaltime/60000000,(totaltime%60000000)/1000000,(totaltime%1000000)/1000, (counted*1000/totaltime), name);
						nds+=counted;
						if(nodes!=counted){
							printf("Not Match!\n");
							LOGGER_1("NOT MATCH!\n");
							printBoardNice(&b);
						}
					} else {
						printf("----- Evaluate:%d -END-, SKIPPED -----\n",i);
						LOGGER_1("----- Evaluate:%d -END-, SKIPPED -----\n",i);
					}
					free(name);
				}
//				evaluateAnswer(answer, move);
			}
//			printf("Again!\n");
			i++;
//			break;
		}
		readClock_wall(&et);
		totaltime=diffClock(st, et);
		printf("Nodes: %llu, Time: %lldm:%llds.%lld; %lld tis/sec\n",nds, totaltime/60000000,(totaltime%60000000)/1000000,(totaltime%1000000)/1000, (nds*1000/totaltime));
		LOGGER_1("Nodes: %llu, Time: %lldm:%llds.%lld; %lld tis/sec\n",nds, totaltime/60000000,(totaltime%60000000)/1000000,(totaltime%1000000)/1000, (nds*1000/totaltime));

		free(b.pers);
//		pindump();
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
	int i;
} perft2_cb_data;

int perft2_cback(char *fen, void *data){
char buffer[512];
perft2_cb_data *i;
	i = (perft2_cb_data *)data ;
	if(!feof(i->handle)) {
		fgets(buffer, 511, i->handle);
		strcpy(fen, buffer);
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

int timed_driver(int t, int d, int max,personality *pers_init, struct _results *results, CBACK, void *cdata)
{
	char buffer[512], fen[100], b2[1024], b3[1024], b4[512];
	char bx[512];
	char am[10][20];
	char bm[10][20];
	char pm[256][20];
	char (*x)[20];
	int bans[20], aans[20];
	int dm, adm;
	int pv[256];
	int i, time, depth;
	board b;
	int val, error, passed;
	unsigned long long starttime, endtime, ttt;
	struct _statistics s;

	char * name;
	tree_store * moves;
	passed=error=0;
	moves = (tree_store *) malloc(sizeof(tree_store));
	b.pers=pers_init;
// personality should be provided by caller
	i=0;
	clearSearchCnt(&s);
	while(cback(bx, cdata)&&(i<max)) {
		if(parseEPD(bx, fen, am, bm, pm, &dm, &name)>0) {

			time=t;
			depth=d;

			setup_FEN_board(&b, fen);
			printBoardNice(&b);
			parseEDPMoves(&b,bans, bm);
			parseEDPMoves(&b,aans, am);
			parsePVMoves(&b, pv, pm);

			//setup limits
			b.uci_options.engine_verbose=0;
			b.uci_options.binc=0;
			b.uci_options.btime=0;
			b.uci_options.depth=depth;
			b.uci_options.infinite=0;
			b.uci_options.mate=0;
			b.uci_options.movestogo=1;
			b.uci_options.movetime=0;
			b.uci_options.ponder=0;
			b.uci_options.winc=0;
			b.uci_options.wtime=0;
			b.uci_options.search_moves[0]=0;

			b.uci_options.nodes=0;
			b.uci_options.movetime=time-100;

			b.time_move=b.uci_options.movetime;
			b.time_crit=b.uci_options.movetime;

			engine_stop=0;
			invalidateHash();
			clearSearchCnt(&(b.stats));

			starttime=readClock();
			b.time_start=starttime;

			val=IterativeSearch(&b, 0-iINFINITY, iINFINITY, 0, b.uci_options.depth, b.side, 0, moves);
			endtime=readClock();
			ttt=endtime-starttime;
			results[i].bestscore=val;
			results[i].time=ttt;
			results[i].passed=1;
			CopySearchCnt(&(results[i].stats), &(b.stats));
			AddSearchCnt(&s, &(b.stats));
			sprintfMove(&b, b.bestmove, buffer);

			if(isMATE(b.bestscore))  {
				int ply=GetMATEDist(b.bestscore);
				if(ply==0) adm=1;
				else {
					adm= (b.side==WHITE ? (ply+1)/2 : (ply/2)+1);
				}
			} else adm=-1;
			// ignore exact PV
			val=evaluateAnswer(&b, b.bestmove, adm , aans, bans, NULL, adm, moves);
//			val=evaluateAnswer(&b, b.bestmove, adm , aans, bans, pv, dm, moves);
			if(val!=1) {
				results[i].passed=0;
				sprintf(b2, "Error: %s %d, proper:",buffer, val);
				error++;
				if((*bm)[0]!=0) {
					sprintf(b4,"BM ");
					x=bm;
					while((*x)[0]!=0) {
						strcat(b4, (*x));
						strcat(b4," ");
						x++;
					}
					strcat(b2, b4);
				}
				if((*am)[0]!=0) {
					sprintf(b4,"AM ");
					x=am;
					while((*x)[0]!=0) {
						strcat(b4, (*x));
						strcat(b4," ");
						x++;
					}
					strcat(b2, b4);
				}
				if(dm>=0) {
					sprintf(b4, "DM %i", dm);
					strcat(b2, b4);
				}
			}
			else {
				sprintf(b2, "Passed, Move: %s, toMate: %i", buffer, adm);
				passed++;
			}
			sprintf(b3, "Position %d, name:%s, %s, Time: %dh, %dm, %ds,, %lld\n",i,name, b2, (int) ttt/3600000, (int) (ttt%3600000)/60000, (int) (ttt%60000)/1000, ttt);

			tell_to_engine(b3);
			free(name);
			i++;
		}
//		i++;
		//				break;
	}

	CopySearchCnt(&(results[i].stats), &s);
	free(moves);
	sprintf(b3, "Positions Total %d, Passed %d, Error %d\n",passed+error, passed, error);
	tell_to_engine(b3);
	return i;
}

void timed2_def(int time, int depth, int max){
int i=0;
personality *pi;
struct _results *r;
	r = malloc(sizeof(struct _results) * (max+1));
	pi=(personality *) init_personality("pers.xml");
	timed_driver(time, depth, max, pi, r, timed2_def_cback, &i);
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
	timed_driver(time, depth, max, pi, r, timed2_remis_cback, &i);
	printSearchStat(&(r[max].stats));
	free(r);
	free(pi);
}

void timed2Test(char *filename, int max_time, int max_depth, int max_positions){
perft2_cb_data cb;
personality *pi;
int p1,p2,f,i1,i2;
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
	i1=timed_driver(max_time, max_depth, max_positions, pi, r1, perft2_cback, &cb);
	fclose(cb.handle);

	clear_killer_moves();
	initHash();
	pi=(personality *) init_personality("pers2.xml");

// round two
	if((cb.handle=fopen(filename, "r"))==NULL) {
		printf("File %s is missing\n",filename);
		goto cleanup;
	}
	i2=timed_driver(max_time, max_depth, max_positions, pi, r2, perft2_cback, &cb);
	fclose(cb.handle);

// prepocitani vysledku
	t1=p1=0;
	for(f=0;f<i1;f++){
		t1+=r1[f].time;
		p1+=r1[f].passed;
	}

	t2=p2=0;
	for(f=0;f<i2;f++){
		t2+=r2[f].time;
		p2+=r2[f].passed;
	}

//reporting
	logger2("Details  \n====================\n");
	logger2("Run#1 Results %d/%d, , Time: %dh, %dm, %ds,, %lld\n",p1,i1, (int) t1/3600000, (int) (t1%3600000)/60000, (int) (t1%60000)/1000, t1);
	logger2("Run#2 Results %d/%d, , Time: %dh, %dm, %ds,, %lld\n",p2,i2, (int) t2/3600000, (int) (t2%3600000)/60000, (int) (t2%60000)/1000, t2);
	logger2("Details 1\n====================\n");
//	printSearchStat2(&(r1[i1].stats), b);
	printSearchStat(&(r1[i1].stats));
	logger2("%s",b);
	logger2("Details 2\n====================\n");
//	printSearchStat2(&(r2[i2].stats), b);
	printSearchStat(&(r2[i2].stats));
	logger2("%s",b);

	if(i1!=i2) {
		logger2("Different number of tests %d:%d!\n", i1, i2);
	} else {
		for(f=0;f<i2;f++) {
//			if(r1[f].passed!=r2[f].passed) {
				logger2("Test %d results %d:%d, time %dh, %dm, %ds, %dh, %dm, %ds\n", f,r1[f].passed, r2[f].passed,(int) r1[f].time/3600000, (int) (r1[f].time%3600000)/60000, (int) (r1[f].time%60000)/1000, (int) r2[f].time/3600000, (int) (r2[f].time%3600000)/60000, (int) (r2[f].time%60000)/1000);
//			}
		}
	}

cleanup:
	free(r1);
	free(r2);
	free(pi);
}

void see_test()
{
int result, move;
	board b;
	char *fen[]= {
			"1k1r4/1pp4p/p7/4p3/8/P5P1/1PP4P/2K1R3 w - -",
			"1k1r3q/1ppn3p/p4b2/4p3/8/P2N2P1/1PP1R1BP/2K1Q3 w - -"
	};

	b.pers=(personality *) init_personality("pers.xml");

	setup_FEN_board(&b, fen[0]);
	printBoardNice(&b);
	move = PackMove(E1, E5,  ER_PIECE, 0);
	result=SEE(&b, move);

	setup_FEN_board(&b, fen[1]);
	printBoardNice(&b);
	move = PackMove(D3, E5,  ER_PIECE, 0);
	result=SEE(&b, move);
	return;
}



void epd_parse(char * filename, char * f2)
{
char buffer[512], fen[100];
char am[10][20];
char bm[10][20];
char pm[256][20];
int dm;
FILE * handle, *h2;
int i;
board b;
char * name;

		b.pers=(personality *) init_personality("pers.xml");

		if((handle=fopen(filename, "r"))==NULL) {
			printf("File %s is missing\n",filename);
			return;
		}
		h2=fopen(f2,"w");
		while(!feof(handle)) {
			fgets(buffer, 511, handle);
			i=24;
			while(!feof(handle)&&(i>0)){
				if(strlen(buffer)<8) i=20;
				fgets(buffer, 511, handle);
				i--;
			}
			i=20;
			while(!feof(handle)&&(i>0)){
				if(strlen(buffer)<8) break;
				if(parseEPD(buffer, fen, am, bm, pm, &dm, &name)>0) {
						setup_FEN_board(&b, fen);
						writeEPD_FEN(&b, fen, 0,"");
						fprintf(h2,"%s\n", fen);
				}
				i--;
				fgets(buffer, 511, handle);
			}
			while(!feof(handle)){
				if(strlen(buffer)<8)break;
				fgets(buffer, 511, handle);
			}
		}
		fclose(handle);
		fclose(h2);
		free(b.pers);
}

void epd_driver(char * filename)
{
char fen[100];
FILE * handle;
int moves, time;
tree_store * mm;
board b, *bs;
unsigned long long now;

		mm = (tree_store *) malloc(sizeof(tree_store));

		bs=&b;

		b.pers=(personality *) init_personality("pers.xml");

		if((handle=fopen(filename, "r"))==NULL) {
			printf("File %s is missing\n",filename);
			return;
		}
		while(!feof(handle)) {
			fgets(fen, 99, handle);
			setup_FEN_board(&b, fen);
			bs->uci_options.engine_verbose=0;
			bs->uci_options.binc=0;
			bs->uci_options.depth=999999;
			bs->uci_options.infinite=0;
			bs->uci_options.mate=0;
			bs->uci_options.movetime=0000;
			bs->uci_options.ponder=0;
			bs->uci_options.winc=0;
			bs->uci_options.search_moves[0]=0;

			bs->uci_options.nodes=0;

			bs->time_move=0;
			bs->time_crit=0;

			bs->uci_options.movestogo=1;
			bs->uci_options.btime=120000;
			bs->uci_options.wtime=120000;


			if(bs->uci_options.infinite!=1) {
				if(bs->uci_options.movestogo==0){
		// sudden death
					moves=40; //fixme
				} else moves=bs->uci_options.movestogo;
				time= (bs->side==0) ? bs->uci_options.wtime : bs->uci_options.btime;
				bs->time_move=time/moves*0.7-1;
				bs->time_crit=bs->time_move*2;
				if(bs->uci_options.movetime!=0) {
					bs->time_move=bs->uci_options.movetime;
					bs->time_crit=bs->uci_options.movetime;
				}
			}
			engine_stop=0;
			invalidateHash();
			bs->time_start=readClock();
//			IterativeSearch(bs, 0-iINFINITY, iINFINITY ,0 , bs->uci_options.depth, bs->side,1, mm);
			IterativeSearch(bs, 0-iINFINITY, iINFINITY ,0 , 256, bs->side,10, mm);
			now=readClock();
			printf("%lld \t%s", now-bs->time_start, fen);
		}
		fclose(handle);
		free(b.pers);
		free(mm);
}

void keyTest_def(void){
	char fen[100];
	char am[10][20];
	char bm[10][20];
	char pm[256][20];
	int dm;
	int i;
	board b;
	BITVAR key, k2;
	char * name;

			i=0;
			while(key_default_tests[i]!=NULL) {
				if(parseEPD(key_default_tests[i], fen, am, bm, pm, &dm, &name)>0) {
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
}

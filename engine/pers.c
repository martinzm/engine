#include "defines.h"
#include "pers.h"
#include "evaluate.h"
#include "globals.h"
#include "utils.h"

#include "macros.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <libxml/parser.h>
#include <wchar.h>
#include <assert.h>

int valuetoint(unsigned char *buf, int *bb, int max)
{
int count=0,i;
char *p;
char d[]= {',','\n'};
char *last=NULL;

//fixme
	p=(char *) buf;
	last=p;
	while (count<max) {
		if(*p=='\0') {
			bb[count]=atoi(last);
			last=p;
			count++;
			break;
		}
		for(i=0;i<2;i++) {
			if(*p==d[i]) {
				*p='\0';
				bb[count]=atoi(last);
				last=p+1;
				count++;
				break;
			}
		}
		p++;
	}
	return (count<max ? -1:0);
}

int valuetointW(wchar_t *buf, int *bb, int max)
{
int count=0,i;
wchar_t *p;
wchar_t d[]= {L',',L'\n'};
wchar_t *last=NULL, *rest;
long int res;

	p=(wchar_t *) buf;
//	printf("X: %S\n", p);
	last=p;
	while (count<max) {
//		printf("X: %S\n", last);
		if(*p==L'\0') {
			res=wcstol(last,&rest,10);
//			printf("OUT: %i == %S\n", res, rest);
			bb[count]=(int) res;
			last=p;
			count++;
			break;
		}
		for(i=0;i<2;i++) {
			if(*p==d[i]) {
				*p=L'\0';
				res=wcstol(last,&rest,10);
				bb[count]=(int) res;
//				printf("ORR: %i :: %S\n", res, rest);
				last=p+1;
				count++;
				break;
			}
		}
		p++;
	}
	return (count<max ? -1:0);
}

int parse_basic_value (xmlDocPtr doc, xmlNodePtr cur, int* st) {
//xmlChar buf[2048];
int r;
wchar_t ww[256];

	r=-1;
	xmlChar *s;
//	s=xmlGetProp(cur, (const xmlChar *) "value");
	s=xmlNodeGetContent(cur);
	if(s!=NULL) {
		UTF8toWchar(s,ww, sizeof(wchar_t)*256);
		valuetointW(ww, st, 1);
		xmlFree(s);
	}
	return r;
}

int parse_value (xmlDocPtr doc, xmlNodePtr cur, int *bb, int max, int* st) {
//xmlChar buf[2048];
int r;
wchar_t ww[256];

	r=-1;
	xmlChar *key;
	xmlChar *s;
//	key=xmlGetProp(cur, (const xmlChar *) "value");
	key=xmlNodeGetContent(cur);
	if(key!=NULL) {
//		xmlStrPrintf(buf,2048, key);
		UTF8toWchar(key,ww, sizeof(wchar_t)*256);
		r=valuetointW(ww, bb, max);
		xmlFree(key);
	}
	s=xmlGetProp(cur, (const xmlChar *) "gamestage");
	if(s!=NULL) {
//		xmlStrPrintf(buf,2048, s);
		UTF8toWchar(s,ww, sizeof(wchar_t)*256);
		r=valuetointW(ww, st, max);
		xmlFree(s);
	}
	
    return r;
}

int parse_value2 (xmlDocPtr doc, xmlNodePtr cur, int *bb, int max, int* st, int *side, int *piece) {
//xmlChar buf[2048];
int r;
wchar_t ww[2048];

	r=-1;
	xmlChar *key;
	xmlChar *s;
	*st=9;
	*side=9;
	*piece=9;
//	key=xmlGetProp(cur, (const xmlChar *) "value");
	key=xmlNodeGetContent(cur);
	if(key!=NULL) {
//		xmlStrPrintf(buf,2048, key);	 
		if(UTF8toWchar(key,ww, sizeof(wchar_t)*2048)!=0) {
			LOGGER_0("conversion1 error!");
			abort();
		}
		r=valuetointW(ww, bb, max);
		xmlFree(key);
	}
	s=xmlGetProp(cur, (const xmlChar *) "gamestage");
	if(s!=NULL) {
//		xmlStrPrintf(buf,2048, s);	 
		if(UTF8toWchar(s,ww, sizeof(wchar_t)*2048)!=0) {
			LOGGER_0("conversion2 error!");
			abort();
		}
		r=valuetointW(ww, st, 1);
		xmlFree(s);
	}
	s=xmlGetProp(cur, (const xmlChar *) "piece");
	if(s!=NULL) {
//		xmlStrPrintf(buf,2048, s);	 
		if(UTF8toWchar(s,ww, sizeof(wchar_t)*2048)!=0) {
			LOGGER_0("conversion3 error!");
			abort();
		}
		r=valuetointW(ww, piece, 1);
		xmlFree(s);
	}
	s=xmlGetProp(cur, (const xmlChar *) "side");
	if(s!=NULL) {
//		xmlStrPrintf(buf,2048, s);	 
		if(UTF8toWchar(s,ww, sizeof(wchar_t)*2048)!=0) {
			LOGGER_0("conversion4 error!");
			abort();
		}
		r=valuetointW(ww, side, 1);
		xmlFree(s);
	}
    return r;
}

int parse_value5 (xmlDocPtr doc, xmlNodePtr cur, int *bb, int max, int *piece) {
int r;
wchar_t ww[2048];

	r=-1;
	xmlChar *key;
	xmlChar *s;
	*piece=9;
//	key=xmlNodeGetProp(cur, (const xmlChar *) "value");
	key=xmlNodeGetContent(cur);
	LOGGER_0("%s\n",key);
	if(key!=NULL) {
		if(UTF8toWchar(key,ww, sizeof(wchar_t)*2048)!=0) {
			LOGGER_0("conversion1 error!");
			abort();
		}
		r=valuetointW(ww, bb, max);
		xmlFree(key);
	}
	s=xmlGetProp(cur, (const xmlChar *) "piece");
	if(s!=NULL) {
		if(UTF8toWchar(s,ww, sizeof(wchar_t)*2048)!=0) {
			LOGGER_0("conversion3 error!");
			abort();
		}
		r=valuetointW(ww, piece, 1);
		xmlFree(s);
	}
    return r;
}

int setup_gamestage(_gamestage *v, int *buffer, int stage)
{
	if(stage>=ER_GAMESTAGE) return 1;
	(*v)[stage]=*buffer;

return 0;
}

int setup_value(_values *v, int *buffer, int count, int stage)
{
int f;
	if(stage>=ER_GAMESTAGE) return 1;
	for(f=0;f<count;f++) {
		(*v)[stage][f]=buffer[f];
	}
return 0;
}

int setup_value5(_dvalues *v, int *buffer, int count, int piece)
{
int f;
	if(piece>=ER_PIECE) return 1;
	for(f=0;f<count;f++) {
		(*v)[piece][f]=buffer[f];
//		LOGGER_0("Setup 5 %d:%d=%d\n", piece, f, buffer[f]);
	}
return 0;
}

int setup_value2(_squares_p *s, int *buffer, int count, int stage, int side, int piece)
{
int f;
	if(stage>=ER_GAMESTAGE) return 1;
	if(side>=ER_SIDE) return 2;
	if(piece>=ER_PIECE) return 3;
	for(f=0;f<count;f++) {
		(*s)[stage][side][piece][f]=buffer[f];
//		if(f%8==0) {
//			printf("\n%d:",f/8+1);
//		}
//		printf("%d,",buffer[f]);
	}
//	printf("\n");
return 0;
}

int setup_value3(_mobility *s, int *buffer, int count, int stage, int side, int piece)
{
int f;
	if(stage>=ER_GAMESTAGE) return 1;
	if(side>=ER_SIDE) return 2;
	if(piece>=ER_PIECE) return 3;
	for(f=0;f<count;f++) {
		(*s)[stage][side][piece][f]=buffer[f];
		if(f%8==0) {
	//		printf("\n%d:",f/8+1);
		}
//		printf("%d,",buffer[f]);
	}
//	printf("\n");
return 0;
}

int setup_value4(_passer *s, int *buffer, int count, int stage, int side)
{
int f;
	if(stage>=ER_GAMESTAGE) return 1;
	if(side>=ER_SIDE) return 2;
	for(f=0;f<count;f++) {
		(*s)[stage][side][f]=buffer[f];
//		logger2("%d %d,\n",f, buffer[f]);
	}
//	logger2("\n");
return 0;
}

int reorganize_values(int *b)
{
int pawn, queen, rook, bishop, king, knight;
	bishop=b[2];
	knight=b[1];
	rook=b[3];
	king=b[5];
	queen=b[4];
	pawn=b[0];
	
	b[PAWN]=pawn;
	b[KNIGHT]=knight;
	b[BISHOP]=bishop;
	b[ROOK]=rook;
	b[QUEEN]=queen;
	b[KING]=king;

return 0;
}

int swap_board(int *bb)
{
int f;
int b;

	for(f=0;f<32;f++) {
		b=bb[f];
		bb[f]=bb[Square_Swap[f]];
		bb[Square_Swap[f]]=b;
	}
	return 0;
}

/*
 *
 */

int params_init_general_option(_general_option *x, int s_r, int *i) {
	*x=*i;
	return 0;
}

int params_load_general_option(xmlDocPtr doc, xmlNodePtr cur, int* st, int s_r, _general_option *o) {
//	if ((!xmlStrcmp(cur->name, (const xmlChar *) "use_hash"))) {
//		printf("xxx");
//	}
	parse_basic_value(doc,cur, st);
	params_init_general_option(o, s_r, st);
return 0;
}

int params_out_general_option(char *x, _general_option *i) {
	LOGGER_0("PERS: %s %i\n", x, *i);
	return 0;
}

int params_write_general_option(xmlNodePtr parent,char *name, int s_r, _general_option *i)
{
wchar_t bw[1024];
xmlChar b8[1024], v8[1024];
xmlNodePtr root, cur;

//printf("8\n");

//	printf("%s\n",name);
	swprintf(bw,999, L"%s", name);
	WchartoUTF8(bw, b8, 1024);
//printf("81\n");

	swprintf(bw,999, L"%i", *i);
	WchartoUTF8(bw, v8, 1024);
	cur=xmlNewTextChild(parent, NULL, b8, v8);

//	xmlNewProp(cur, (xmlChar*) "value", b8);

//	xmlFree(bw);
return 0;
}

int params_init_gamestage(_gamestage *x, int s_r, int *i) {
//	printf("%i %i", i[0], i[1]);
	setup_gamestage(x, i, 0);
	setup_gamestage(x, i+1, 1);
	return 0;
}

int params_load_gamestage(xmlDocPtr doc, xmlNodePtr cur, int* st, int s_r, _gamestage *o) {
int stage;
	parse_value (doc, cur, st, 1, &stage);
	setup_gamestage(o, st, stage);
return 0;
}

int params_out_gamestage(char *x, _gamestage *i){
int f;
char buf[512], b2[512];
		sprintf(buf,"PERS: %s ",x);
		for(f=0;f<ER_GAMESTAGE;f++) {
			sprintf(b2,"GS[%i]:%i\t", f, (*i)[f]);
			strcat(buf, b2);
		}
		LOGGER_0("%s\n", buf);
return 0;
}

int params_write_gamestage(xmlNodePtr parent,char *name, int s_r, _gamestage *i){
int f;
char buf[512], b2[512];
wchar_t bw[1024];
xmlChar b8[256], b82[256], n8[256];
xmlNodePtr root, cur;
//printf("7\n");
	swprintf(bw,999, L"%s", name);
	WchartoUTF8(bw, n8, 256);
	for(f=0;f<(ER_GAMESTAGE);f++) {
		swprintf(bw,999, L"%i", (*i)[f]);
		WchartoUTF8(bw, b8, 256);
		swprintf(bw,999, L"%d", f);
		WchartoUTF8(bw, b82, 256);
		cur=xmlNewTextChild(parent, NULL, n8, b8);
		xmlNewProp(cur, (xmlChar *)"gamestage", b82);
//		xmlNewProp(cur, (xmlChar *)"value", b8);
	}

return 0;
}

int params_init_values(_values *x, int s_r, int *i) {
	setup_value(x, i  , ER_PIECE, 0);
	setup_value(x, i+ER_PIECE, ER_PIECE, 1);
	return 0;
}

int params_load_values(xmlDocPtr doc, xmlNodePtr cur, int* st, int s_r, _values *o) {
	int val[6];
		parse_value (doc, cur, val, ER_PIECE, st);
		setup_value(o, val, ER_PIECE, *st);
	return 0;
}

int params_out_values(char *x, _values *i){
int f;
char buf[512], b2[512];
		sprintf(buf,"PERS: %s ",x);
		for(f=0;f<ER_GAMESTAGE;f++) {
			sprintf(b2,"VAL[%i]:%i, %i, %i, %i, %i, %i\t", f, (*i)[f][PAWN],(*i)[f][KNIGHT],(*i)[f][BISHOP],(*i)[f][ROOK],(*i)[f][QUEEN],(*i)[f][KING]);
			LOGGER_0("%s\n", b2);
		}
return 0;
}

int params_write_values(xmlNodePtr parent,char *name, int s_r, _values *i){
int f,n;
char buf[512], b2[128];
xmlNodePtr root, cur;

wchar_t bw[1024];
xmlChar b8[512], b82[256], n8[256];

//printf("'6\n");

	swprintf(bw,999, L"%s", name);
	WchartoUTF8(bw, n8, 256);

	for(f=0;f<(ER_GAMESTAGE);f++) {
		buf[0]='\0';
		for(n=0;n<5;n++) {
			sprintf(b2,"%d,",(*i)[f][n]);
			strcat(buf,b2);
		}
		sprintf(b2,"%d",(*i)[f][5]);
		strcat(buf,b2);

//		cur=xmlNewTextChild(parent, NULL, name, NULL);
//		xmlNewProp(cur, "gamestage", b2);
//		xmlNewProp(cur, "value", buf);

		swprintf(bw,999, L"%d", f);
		WchartoUTF8(bw, b82, 256);

		swprintf(bw,999, L"%s", buf);
		WchartoUTF8(bw, b8, 256);

		cur=xmlNewTextChild(parent, NULL, n8, b8);
		xmlNewProp(cur, (xmlChar *)"gamestage", b82);
//		xmlNewProp(cur, (xmlChar *)"value", b8);
	}
return 0;
}

int params_init_dvalues(_dvalues *x, int s_r, int *i) {
int piece;
	for(piece=0;piece<ER_PIECE;piece++) {
		setup_value5(x, i+piece*(PAWNS_TOT+1)  , PAWNS_TOT+1, piece);
	}
	return 0;
}

int params_load_dvalues(xmlDocPtr doc, xmlNodePtr cur, int* st, int s_r, _dvalues *o) {
int val[PAWNS_TOT+1];
int piece;
		parse_value5(doc, cur, val, PAWNS_TOT+1, &piece);
		setup_value5(o, val, PAWNS_TOT+1 , piece);
	return 0;
}

int params_out_dvalues(char *x, _dvalues *i){
int f,n;
char buf[512], b2[512];
		sprintf(buf,"PERS: %s \n",x);
		for(f=0;f<ER_PIECE;f++) {
			sprintf(buf,"VAL[%i]:\t", f);
			for(n=0;n<(PAWNS_TOT);n++) {
				sprintf(b2,"%d\n", (*i)[f][n]);
				strcat(buf, b2);
			}
			sprintf(b2,"%d\n", (*i)[f][n]);
			strcat(buf, b2);
			
			LOGGER_0("%s\n", buf);
		}
return 0;
}

int params_write_dvalues(xmlNodePtr parent,char *name, int s_r, _dvalues *i){
int f,n;
char buf[512], b2[128];
xmlNodePtr root, cur;

wchar_t bw[1024];
xmlChar b8[512], b82[256], n8[256];

	swprintf(bw,999, L"%s", name);
	WchartoUTF8(bw, n8, 256);

	for(f=0;f<(ER_PIECE);f++) {
		buf[0]='\0';
		for(n=0;n<(PAWNS_TOT);n++) {
			sprintf(b2,"%d,",(*i)[f][n]);
			strcat(buf,b2);
		}
		sprintf(b2,"%d",(*i)[f][n]);
		strcat(buf,b2);

		swprintf(bw,999, L"%d", f);
		WchartoUTF8(bw, b82, 256);

		swprintf(bw,999, L"%s", buf);
		WchartoUTF8(bw, b8, 256);

		cur=xmlNewTextChild(parent, NULL, n8, b8);
		xmlNewProp(cur, (xmlChar *)"piece", b82);
//		xmlNewProp(cur, (xmlChar *)"value", b8);
	}
return 0;
}

int params_init_passer(_passer *x, int s_r, int *i) {
int side,f;
int p[ER_RANKS];
_passer p1;
_passer p2;
	side=*i;
	i++;
//	logger2("passer sides %d\n", side);
	if(side==2) {
		setup_value4(x, i, ER_RANKS, 0, 0);
		setup_value4(x, i+ER_RANKS, ER_RANKS, 1, 0);
		// swap if needed
		if(s_r&1) {
//			logger2("swap\n");
			for(f=0; f<ER_RANKS; f++) (*x)[0][1][f]=(*x)[0][0][ER_RANKS-1-f];
			for(f=0; f<ER_RANKS; f++) (*x)[1][1][f]=(*x)[1][0][ER_RANKS-1-f];
		} else {
			for(f=0; f<ER_RANKS; f++) (*x)[0][1][f]=(*x)[0][0][f];
			for(f=0; f<ER_RANKS; f++) (*x)[1][1][f]=(*x)[1][0][f];
		}
	} else {
		if(side>2) return 1;
		setup_value4(x, i, ER_RANKS, 0, side);
		setup_value4(x, i+ER_RANKS, ER_RANKS, 1, side);
		x+=ER_RANKS*2;
		side=*i;
		i++;
		if(side>2) return 1;
		setup_value4(x, i, ER_RANKS, 0, side);
		setup_value4(x, i+ER_RANKS, ER_RANKS, 1, side);
	}
	return 0;
}

int params_load_passer(xmlDocPtr doc, xmlNodePtr cur, int* st, int s_r, _passer *o) {
int side, stage, piece,f;
int bb[128];

	parse_value2 (doc, cur, bb, ER_RANKS, &stage, &side, &piece);
	assert(stage!=9);
//	assert(piece!=9);
	assert(side!=9);
	if((side==1) || (side==0)) {
		setup_value4(o, bb, ER_RANKS, stage, side);
	} else if(side==2) {
		setup_value4(o, bb, ER_RANKS, stage, 0);
		if(s_r&1) {
			for(f=0; f<ER_RANKS; f++) (*o)[stage][1][f]=(*o)[stage][0][ER_RANKS-1-f];
		} else {
			setup_value4(o,bb, ER_RANKS, stage, 1);
		}
	}
return 0;
}

int params_out_passer(char *x, _passer *i){
int f,n;
char buf[512], b2[512];
//		LOGGER_2("PERS: %s ",x);
//		sprintf(buf,"PERS: %s ",x);
		for(f=0;f<ER_GAMESTAGE;f++) {
			LOGGER_0("GS[%i]:SIDE[WHITE]=%i, %i, %i, %i, %i, %i, %i, %i\n", f,(*i)[f][0][0],(*i)[f][0][1],(*i)[f][0][2],(*i)[f][0][3],(*i)[f][0][4],(*i)[f][0][5],(*i)[f][0][6],(*i)[f][0][7]);
			LOGGER_0("GS[%i]:SIDE[BLACK]=%i, %i, %i, %i, %i, %i, %i, %i\n", f,(*i)[f][1][0],(*i)[f][1][1],(*i)[f][1][2],(*i)[f][1][3],(*i)[f][1][4],(*i)[f][1][5],(*i)[f][1][6],(*i)[f][1][7]);
//			sprintf(b2,"VAL[%i]:%i, %i, %i, %i, %i, %i\t", f, (*i));
//			strcat(buf, b2);
		}
return 0;
}

int params_write_passer(xmlNodePtr parent,char *name, int s_r, _passer *i){
int f,n, side, piece;
char buf[512], b1[512], b2[512], b3[20];
xmlNodePtr root, cur;
wchar_t bw[2048];
xmlChar g8[256], s8[256], v8[1024], n8[256];

//printf("5\n");
	swprintf(bw,999, L"%s", name);
	WchartoUTF8(bw, n8, 256);


	for(f=0;f<(ER_GAMESTAGE);f++) {
// check for side type
		side=2;
		for(n=0; n<ER_RANKS; n++) {
			if(s_r&1) {
				if((*i)[f][1][n]!=(*i)[f][0][ER_RANKS-1-n]) {
					side=0;
					break;
				}
			} else {
				if((*i)[f][1][n]!=(*i)[f][0][n]) {
					side=0;
					break;
				}
			}
		}
		buf[0]='\0';
		for(n=0; n<(ER_RANKS-1); n++){
			sprintf(b2,"%d,",(*i)[f][0][n]);
			strcat(buf,b2);
		}

		swprintf(bw,999, L"%s%d", buf,(*i)[f][0][ER_RANKS-1]);
		WchartoUTF8(bw, v8, 1024);
		swprintf(bw,999, L"%d", f);
		WchartoUTF8(bw, g8, 256);
		swprintf(bw,999, L"%d", side);
		WchartoUTF8(bw, s8, 256);

		cur=xmlNewTextChild(parent, NULL, n8, v8);
		xmlNewProp(cur, (xmlChar *) "gamestage", g8);
		xmlNewProp(cur, (xmlChar *) "side", s8);
//		xmlNewProp(cur, (xmlChar *) "value", v8);
		if(side!=2) {
			side=1;
			buf[0]='\0';
			if(s_r&1) {
				for(n=ER_RANKS-1; n>0; n--){
					sprintf(b2,"%d,",(*i)[f][1][n]);
					strcat(buf,b2);
				}
				sprintf(b2,"%d",(*i)[f][1][0]);
				strcat(buf,b2);
			} else {
				for(n=0; n<(ER_RANKS-1); n++){
					sprintf(b2,"%d,",(*i)[f][1][n]);
					strcat(buf,b2);
				}
				sprintf(b2,"%d",(*i)[f][1][ER_RANKS-1]);
				strcat(buf,b2);
			}

			swprintf(bw,999, L"%s", buf);
			WchartoUTF8(bw, v8, 1024);
			swprintf(bw,999, L"%d", side);
			WchartoUTF8(bw, s8, 256);

			cur=xmlNewTextChild(parent, NULL, n8, v8);
			xmlNewProp(cur, (xmlChar *) "gamestage", g8);
			xmlNewProp(cur, (xmlChar *) "side", s8);
//			xmlNewProp(cur, (xmlChar *) "value", v8);
		}
	}
return 0;
}

#define OPTION_GS(x,y)	if ((!xmlStrcmp(cur->name, (const xmlChar *)x)))\
		{	parse_value (doc, cur, bb, 1, &stage);\
			setup_gamestage(&(y), bb, stage); }

#undef MLINE
#define MLINE(x,y,z,s_r,i)	if ((!xmlStrcmp(cur->name, (const xmlChar *) #x)))\
		{	params_load ## z(doc, cur, bb, s_r, &(p->y)); }

static void parsedoc(char *docname, personality * p) {
//char buf[256];
int bb[128];
int stage, side, piece, x;

	xmlDocPtr doc;
	xmlNodePtr cur;

	doc = xmlParseFile(docname);
	if (doc == NULL ) {
		fprintf(stderr,"Document not parsed successfully. \n");
		return;
	}
	
	cur = xmlDocGetRootElement(doc);
	if (cur == NULL) {
		fprintf(stderr,"empty document\n");
		xmlFreeDoc(doc);
		return;
	}
	
	if (xmlStrcmp(cur->name, (xmlChar *) "configuration")) {
		fprintf(stderr,"document of the wrong type, root node != documentation");
		xmlFreeDoc(doc);
		return;
	}	
	cur = cur->xmlChildrenNode;
// musime najit material a nastavit premapovavani figur, v souboru je to vzdy v poradi PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING
// ktere ale v programu nemusi mit takto pridelena cisla
// FIXME

	while (cur != NULL) {

		E_OPTS;
// musi byt prochazeno jako prvni...
		if ((!xmlStrcmp(cur->name, (const xmlChar *)"PieceToSquare"))){
			parse_value2 (doc, cur, bb, 64, &stage, &side, &piece);
			piece=Piece_Map[piece];
		
			if((side==1) || (side==0)) {
				setup_value2(&(p->piecetosquare),bb, 64, stage, side, piece);
			} else if(side==2) {
				setup_value2(&(p->piecetosquare),bb, 64, stage, 0, piece);
				swap_board(bb);
				setup_value2(&(p->piecetosquare),bb, 64, stage, 1, piece);
			}
		}
		if ((!xmlStrcmp(cur->name, (const xmlChar *)"Mobility"))){
			parse_value2 (doc, cur, bb, 29, &stage, &side, &piece);
			piece=Piece_Map[piece];
			if((side==1) || (side==0)) {
				setup_value3(&(p->mob_val),bb, 29, stage, side, piece);
			} else if(side==2) {
				setup_value3(&(p->mob_val),bb, 29, stage, 0, piece);
				setup_value3(&(p->mob_val),bb, 29, stage, 1, piece);
			}
		}
		if ((!xmlStrcmp(cur->name, (const xmlChar *)"MobUnSec"))){
			parse_value2 (doc, cur, bb, 29, &stage, &side, &piece);
			piece=Piece_Map[piece];
			if((side==1) || (side==0)) {
				setup_value3(&(p->mob_uns),bb, 29, stage, side, piece);
			} else if(side==2) {
				setup_value3(&(p->mob_uns),bb, 29, stage, 0, piece);
				setup_value3(&(p->mob_uns),bb, 29, stage, 1, piece);
			}
		}
		cur = cur->next;
	}	
	xmlFreeDoc(doc);
	return;
}

// setup defaults
// MLINE(material, Values, _values, 0, M_P(1000,3250,3250,5000,9750,0,1000,3250,3250,5000,9750,88888))

#undef MLINE
#define MLINE(x,y,z,s_r,i) { int __qq__[]={i}; params_init ## z(&(p->y), s_r, __qq__); }

void setup_init_pers(personality * p)
{
	int f,x,i;
	
	E_OPTS;

	for(f=0;f<ER_GAMESTAGE;f++) {
		for(x=0;x<ER_PIECE;x++) {
			for(i=0;i<64;i++) {
				p->piecetosquare[f][WHITE][x][i]=0;
				p->piecetosquare[f][BLACK][x][i]=0;
			}
		}
	}
	for(f=0;f<ER_GAMESTAGE;f++) {
		for(x=0;x<ER_PIECE;x++) {
			for(i=0;i<ER_MOBILITY;i++) {
				p->mob_val[f][WHITE][x][i]=0;
				p->mob_val[f][BLACK][x][i]=0;
			}
		}
	}
	for(f=0;f<ER_GAMESTAGE;f++) {
		for(x=0;x<ER_PIECE;x++) {
			for(i=0;i<ER_MOBILITY;i++) {
				p->mob_uns[f][WHITE][x][i]=0;
				p->mob_uns[f][BLACK][x][i]=0;
			}
		}
	}
//	for(f=0;f<ER_GAMESTAGE;f++) {
//		for(x=0;x<ER_RANKS;x++) {
//			p->passer_bonus[f][WHITE][x]=(256>>x)*40*(f+1);
//			p->passer_bonus[f][BLACK][x]=(256>>x)*40*(f+1);
//		}
//	}
}

int print_pers_values2(char *b, _squares_p *s, int count, int stage, int side, int piece ){
int f;
char b2[2048];
	sprintf(b, "%6d", (*s)[stage][side][piece][0]);
	for(f=1;f<count;f++) {
		sprintf(b2, ", %6d", (*s)[stage][side][piece][f]);
		strcat(b, b2);
	}
	return 0;
}

int print_pers_values3(char *b, _mobility *s, int count, int stage, int side, int piece ){
int f;
char b2[2048];
	sprintf(b, "%6d", (*s)[stage][side][piece][0]);
	for(f=1;f<count;f++) {
		sprintf(b2, ", %6d", (*s)[stage][side][piece][f]);
		strcat(b, b2);
	}
	return 0;
}

int print_pers_values(char *b, _squares_p *s, int count, int stage, int side, int piece ){
int f;
char b2[2048];
	f=7;
	sprintf(b, "%6d\t%6d\t%6d\t%6d\t%6d\t%6d\t%6d\t%6d\n",
		(*s)[stage][side][piece][f*8+0], (*s)[stage][side][piece][f*8+1],
		(*s)[stage][side][piece][f*8+2], (*s)[stage][side][piece][f*8+3],
		(*s)[stage][side][piece][f*8+4], (*s)[stage][side][piece][f*8+5],
		(*s)[stage][side][piece][f*8+6], (*s)[stage][side][piece][f*8+7]);
	for(f=6;f>=0;f--) {
		sprintf(b2, "%6d\t%6d\t%6d\t%6d\t%6d\t%6d\t%6d\t%6d\n",
			(*s)[stage][side][piece][f*8+0], (*s)[stage][side][piece][f*8+1],
			(*s)[stage][side][piece][f*8+2], (*s)[stage][side][piece][f*8+3],
			(*s)[stage][side][piece][f*8+4], (*s)[stage][side][piece][f*8+5],
			(*s)[stage][side][piece][f*8+6], (*s)[stage][side][piece][f*8+7]);
		strcat(b, b2);
	}
return 0;
}

#undef MLINE
#define MLINE(x,y,z,s_r,i) { params_out ## z(#x,&(p->y)); }

int personality_dump(personality *p){
	int f, x;
	char buf[2048];

	DEB_1(E_OPTS;)
	
	for(f=0;f<ER_GAMESTAGE;f++) {
		for(f=0;f<ER_GAMESTAGE;f++) {
			for(x=0;x<ER_PIECE;x++) {
				print_pers_values(buf, &(p->piecetosquare), 64, f, WHITE, x);
				LOGGER_0("PERS: PieceToSquare, stage %d, side %d, piece %d\n%s", f, 0, x, buf);
			}
		}
		for(f=0;f<ER_GAMESTAGE;f++) {
			for(x=0;x<ER_PIECE;x++) {
				print_pers_values(buf, &(p->piecetosquare), 64, f, BLACK, x);
				LOGGER_0("PERS: PieceToSquare, stage %d, side %d, piece %d\n%s", f, 1, x, buf);
			}
		}
		for(x=0;x<ER_SIDE;x++) {
			for(f=0;f<ER_GAMESTAGE;f++) {
				print_pers_values3(buf, &(p->mob_val), 1, f, x, PAWN);
				LOGGER_0("PERS: Mobility, stage %d, side %d, piece %d, %s\n", f, x, 0, buf);
				print_pers_values3(buf, &(p->mob_val), 9, f, x, KNIGHT);
				LOGGER_0("Mobility, stage %d, side %d, piece %d, %s\n", f, x, 1, buf);
				print_pers_values3(buf, &(p->mob_val), 14, f, x, BISHOP);
				LOGGER_0("PERS: Mobility, stage %d, side %d, piece %d, %s\n", f, x, 2, buf);
				print_pers_values3(buf, &(p->mob_val), 15, f, x, ROOK);
				LOGGER_0("Mobility, stage %d, side %d, piece %d, %s\n", f, x, 3, buf);
				print_pers_values3(buf, &(p->mob_val), 29, f, x, QUEEN);
				LOGGER_0("Mobility, stage %d, side %d, piece %d, %s\n", f, x, 4, buf);
				print_pers_values3(buf, &(p->mob_val), 9, f, x, KING);
				LOGGER_0("Mobility, stage %d, side %d, piece %d, %s\n", f, x, 5, buf);
			}
		}
	}
	return 0;
}

int load_personality(char *docname, personality * p)
{
	parsedoc(docname, p);
	return 1;
}
//int default_piece_values[5]={ 1000, 3250, 3250, 5000, 9750 };

void * init_personality(char *docname) {
personality *p;

	p=(personality*) malloc(sizeof(personality));
	setup_init_pers(p);
	if(docname!=NULL) {
		load_personality(docname, p);
		LOGGER_1("INFO: Personality file: %s loaded.\n",docname);
	}
	personality_dump(p);
	meval_table_gen(p->mat, p, 0);
	meval_table_gen(p->mate_e, p, 1);
//	LOGGER_0("mat index check %d\n", p->mate_e[0x1c320].mat_w);
	
//	p->matdeb=&(p->mate_e[0x1c320].mat_w);
	mat_info(p->mat_info);
	mat_faze(p->mat_faze);
	MVVLVA_gen((p->LVAcap), p->Values);
return p;
}

#undef MLINE
#define MLINE(x,y,z,s_r,i)	{ params_write ## z(root, #x, s_r, &(p->y)); }

int write_personality(personality *p, char *docname){
xmlDocPtr doc;
xmlNodePtr root, cur;
char buf[1024], b2[1024], b3[40], b4[40], b1[1024];

wchar_t bw[2048];
xmlChar g8[512], s8[512], v8[2048], n8[512], p8[512];

int gs, piece, sq;

int mob_lengths[]= { 1, 9, 14, 15, 28, 9, -1  };

	doc = xmlNewDoc((unsigned char*)"1.0");
	xmlNewDocComment(doc, (xmlChar *) "test");
	root=xmlNewDocNode(doc, NULL,(xmlChar *) "configuration", NULL );
	xmlDocSetRootElement(doc, root);

	E_OPTS;

//psts gs, piece, side, value

	for(piece=0;piece<=5;piece++) {
		swprintf(bw,999, L"%d", piece);
		WchartoUTF8(bw, p8, 512);
		for(gs=0;gs<=1;gs++) {
			swprintf(bw,999, L"%d", gs);
			WchartoUTF8(bw, g8, 512);
			buf[0]='\0';
			for(sq=0;sq<63;sq++){
				sprintf(b2,"%d,",p->piecetosquare[gs][0][piece][sq]);
				strcat(buf,b2);
			}

			swprintf(bw,999, L"%s%d",buf,p->piecetosquare[gs][0][piece][sq]);
			WchartoUTF8(bw, v8, 2048);
			swprintf(bw,999, L"%d", 2);
			WchartoUTF8(bw, s8, 512);
			cur=xmlNewTextChild(root, NULL, (xmlChar *) "PieceToSquare", v8);
			xmlNewProp(cur, (xmlChar *) "gamestage", g8);
			xmlNewProp(cur, (xmlChar *) "piece", p8);
			xmlNewProp(cur, (xmlChar *) "side", s8);
//			xmlNewProp(cur, (xmlChar *) "value", v8);
		}
	}

// mobility gs, piece, side, value
	for(piece=0;piece<=5;piece++) {
		swprintf(bw,999, L"%d", piece);
		WchartoUTF8(bw, p8, 512);
		for(gs=0;gs<=1;gs++) {
			swprintf(bw,999, L"%d", gs);
			WchartoUTF8(bw, g8, 512);
			buf[0]='\0';
			for(sq=0;sq<(mob_lengths[piece]-1);sq++){
				sprintf(b2,"%d,",p->mob_val[gs][0][piece][sq]);
				strcat(buf,b2);
			}
			swprintf(bw,999, L"%s%d",buf,p->mob_val[gs][0][piece][mob_lengths[piece]-1]);
			WchartoUTF8(bw, v8, 2048);
			swprintf(bw,999, L"%d", 2);
			WchartoUTF8(bw, s8, 512);

			cur=xmlNewTextChild(root, NULL, (xmlChar *) "Mobility", v8);
			xmlNewProp(cur, (xmlChar *) "gamestage", g8);
			xmlNewProp(cur, (xmlChar *) "piece", p8);
			xmlNewProp(cur, (xmlChar *) "side", s8);
//			xmlNewProp(cur, (xmlChar *) "value", v8);
		}
	}

	for(piece=0;piece<=5;piece++) {
		swprintf(bw,999, L"%d", piece);
		WchartoUTF8(bw, p8, 512);
		for(gs=0;gs<=1;gs++) {
			swprintf(bw,999, L"%d", gs);
			WchartoUTF8(bw, g8, 512);
			buf[0]='\0';
			for(sq=0;sq<(mob_lengths[piece]-1);sq++){
				sprintf(b2,"%d,",p->mob_uns[gs][0][piece][sq]);
				strcat(buf,b2);
			}
			swprintf(bw,999, L"%s%d",buf,p->mob_uns[gs][0][piece][mob_lengths[piece]-1]);
			WchartoUTF8(bw, v8, 2048);
			swprintf(bw,999, L"%d", 2);
			WchartoUTF8(bw, s8, 512);

			cur=xmlNewTextChild(root, NULL, (xmlChar *) "MobUnSec", v8);
			xmlNewProp(cur, (xmlChar *) "gamestage", g8);
			xmlNewProp(cur, (xmlChar *) "piece", p8);
			xmlNewProp(cur, (xmlChar *) "side", s8);
//			xmlNewProp(cur, (xmlChar *) "value", v8);
		}
	}

	xmlSaveFormatFile(docname, doc, 1);
	xmlFreeDoc(doc);
	return 0;
}

int copyPers(personality *source, personality *dest) {
	memcpy(dest, source, sizeof(personality));
return 0;
}

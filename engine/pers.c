/*
 *
 * $Id: pers.c,v 1.1.2.4 2006/02/09 20:30:07 mrt Exp $
 *
 */

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

int valuetoint(unsigned char *buf, int *bb, int max)
{
int count=0;
char *p;
char d[]= {',','\n'};
char *last;	

//fixme
	p=strtok_r((char*)buf,d,&last);
	while((p!=NULL)&&(count<max)) {
		bb[count]=atoi(p);
		count++;
		p=strtok_r(NULL,d,&last);	
	}
	return (count<max ? -1:0);
}

int parse_basic_value (xmlDocPtr doc, xmlNodePtr cur, int* st) {
xmlChar buf[2048];
int r;

	r=-1;
	xmlChar *s;
	s=xmlGetProp(cur, (const xmlChar *) "value");
	if(s!=NULL) {
		xmlStrPrintf(buf,2048, s);
		r=valuetoint(buf, st, 1);
		xmlFree(s);
	}
	return r;
}

int parse_value (xmlDocPtr doc, xmlNodePtr cur, int *bb, int max, int* st) {
xmlChar buf[2048];
int r;

	r=-1;
	xmlChar *key;
	xmlChar *s;
	key=xmlGetProp(cur, (const xmlChar *) "value");
	if(key!=NULL) {
		xmlStrPrintf(buf,2048, key);
		r=valuetoint(buf, bb, max);
		xmlFree(key);
	}
	s=xmlGetProp(cur, (const xmlChar *) "gamestage");
	if(s!=NULL) {
		xmlStrPrintf(buf,2048, s);	 
		r=valuetoint(buf, st, 1);
		xmlFree(s);
	}
	
    return r;
}

int parse_value2 (xmlDocPtr doc, xmlNodePtr cur, int *bb, int max, int* st, int *side, int *piece) {
xmlChar buf[2048];
int r;

	r=-1;
	xmlChar *key;
	xmlChar *s;

	key=xmlGetProp(cur, (const xmlChar *) "value");
	if(key!=NULL) {
		xmlStrPrintf(buf,2048, key);	 
		r=valuetoint(buf, bb, max);
		xmlFree(key);
	}
	s=xmlGetProp(cur, (const xmlChar *) "gamestage");
	if(s!=NULL) {
		xmlStrPrintf(buf,2048, s);	 
		r=valuetoint(buf, st, 1);
		xmlFree(s);
	}
	s=xmlGetProp(cur, (const xmlChar *) "piece");
	if(s!=NULL) {
		xmlStrPrintf(buf,2048, s);	 
		r=valuetoint(buf, piece, 1);
		xmlFree(s);
	}
	s=xmlGetProp(cur, (const xmlChar *) "side");
	if(s!=NULL) {
		xmlStrPrintf(buf,2048, s);	 
		r=valuetoint(buf, side, 1);
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

int setup_value2(_squares_p *s, int *buffer, int count, int stage, int side, int piece)
{
int f;
	if(stage>=ER_GAMESTAGE) return 1;
	if(side>=ER_SIDE) return 2;
	if(piece>=ER_PIECE) return 3;
	for(f=0;f<count;f++) {
		(*s)[stage][side][piece][f]=buffer[f];
		if(f%8==0) {
//			printf("\n%d:",f/8+1);
		}
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
//		printf("%d %d,",f, buffer[f]);
	}
//	printf("\n");
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

int params_init_general_option(_general_option *x, int *i) {
	*x=*i;
	return 0;
}

int params_load_general_option(xmlDocPtr doc, xmlNodePtr cur, int* st, _general_option *o) {
	parse_basic_value(doc,cur, st);
	params_init_general_option(o, st);
return 0;
}

int params_init_gamestage(_gamestage *x, int *i) {
//	printf("%i %i", i[0], i[1]);
	setup_gamestage(x, i, 0);
	setup_gamestage(x, i+1, 1);
	return 0;
}

int params_load_gamestage(xmlDocPtr doc, xmlNodePtr cur, int* st, _gamestage *o) {
	int stage;
		parse_value (doc, cur, st, 1, &stage);
		setup_gamestage(o, st, stage);
	return 0;
}

int params_out_general_option(char *x, _general_option *i) {
char b2[512];
	sprintf(b2, "%s %i\n", x, *i); LOGGER_2("PERS:",b2,"");
	return 0;
}

int params_out_gamestage(char *x, _gamestage *i){
	char b2[512], b3[256];
	int f;
		sprintf(b2, "%s ",x);
		for(f=0;f<ER_GAMESTAGE;f++) {
			sprintf(b3,"GS[%i]:%i\t", f, (*i)[f]);
			strcat(b2,b3);
		}
		strcat(b2, "\n");
		LOGGER_2("PERS:",b2,"");
return 0;
}

#define OPTION_GS(x,y)	if ((!xmlStrcmp(cur->name, (const xmlChar *)x)))\
		{	parse_value (doc, cur, bb, 1, &stage);\
			setup_gamestage(&(y), bb, stage); }

#undef MLINE
#define MLINE(x,y,z,i)	if ((!xmlStrcmp(cur->name, (const xmlChar *) #x)))\
		{	params_load ## z(doc, cur, bb, &(p->y)); }


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
	
	if (xmlStrcmp(cur->name, (const xmlChar *) "configuration")) {
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
		
		if ((!xmlStrcmp(cur->name, (const xmlChar *)"material"))){
			parse_value (doc, cur, bb, 6, &stage);
			reorganize_values(bb);
			setup_value(&(p->Values), bb, 6, stage);
//			printf("Material: %d\n",stage);
		}
//		OPTION_GS("bishopboth", p->bishopboth);
//		OPTION_GS("isolated_penalty", p->isolated_penalty);
//		OPTION_GS("backward_penalty", p->backward_penalty);
//		OPTION_GS("backward_fix_penalty", p->backward_fix_penalty);
//		OPTION_GS("doubled_penalty", p->doubled_penalty);
		if ((!xmlStrcmp(cur->name, (const xmlChar *)"PieceToSquare"))){
			parse_value2 (doc, cur, bb, 64, &stage, &side, &piece);
			piece=Piece_Map[piece];
		
			if((side==1) || (side==0)) {
//				printf("PieceMap %d %d %d\n", stage, side, piece);
				setup_value2(&(p->piecetosquare),bb, 64, stage, side, piece);
			} else if(side==2) {
//				printf("PieceMap %d %d %d\n", stage, side, piece);
				setup_value2(&(p->piecetosquare),bb, 64, stage, 0, piece);
				swap_board(bb);
//				printf("PieceMap %d %d %d\n", stage, side, piece);
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
		if ((!xmlStrcmp(cur->name, (const xmlChar *)"passer_bonus"))){
			parse_value2 (doc, cur, bb, 8, &stage, &side, &piece);
			if((side==1) || (side==0)) {
				setup_value4(&(p->passer_bonus),bb, 8, stage, side);
			} else if(side==2) {
				setup_value4(&(p->passer_bonus),bb, 8, stage, 0);
				for(x=0; x<ER_RANKS; x++) p->passer_bonus[stage][1][x]=p->passer_bonus[stage][0][ER_RANKS-1-x];
			}
		}
		cur = cur->next;
	}	
	xmlFreeDoc(doc);
	return;
}

// setup defaults
#undef MLINE
#define MLINE(x,y,z,i) { int __qq__[]={i}; params_init ## z(&(p->y), __qq__); }

void setup_init_pers(personality * p)
{
	int f,x,i;
	
	E_OPTS;

	for(f=0;f<ER_GAMESTAGE;f++) {
		p->Values[f][BISHOP]=3250;
		p->Values[f][KNIGHT]=3250;
		p->Values[f][ROOK]=5000;
		p->Values[f][QUEEN]=9750;
		p->Values[f][PAWN]=1000;
		p->Values[f][KING]=0;
	}
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
		for(x=0;x<ER_RANKS;x++) {
			p->passer_bonus[f][WHITE][x]=(2<<x)*40*(f+1);
			p->passer_bonus[f][BLACK][ER_RANKS-1-x]=(2<<x)*40*(f+1);
		}
	}
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
#define MLINE(x,y,z,i) { params_out ## z(#x,&(p->y)); }

int personality_dump(personality *p){
	int f, x;
	char buf[2048], b2[2048];

	E_OPTS;
	
	for(f=0;f<ER_GAMESTAGE;f++) {
		sprintf(buf, "Values: Stage %i %i, %i, %i, %i, %i, %i\n", f, p->Values[f][PAWN], p->Values[f][BISHOP], p->Values[f][KNIGHT], p->Values[f][ROOK], p->Values[f][QUEEN], p->Values[f][KING]);
		for(f=0;f<ER_GAMESTAGE;f++) {
			for(x=0;x<ER_PIECE;x++) {
				print_pers_values(buf, &(p->piecetosquare), 64, f, WHITE, x);
				sprintf(b2, "PieceToSquare, stage %d, side %d, piece %d\n%s", f, 0, x, buf);
				LOGGER_2("PERS:",b2,"");
			}
		}
		for(f=0;f<ER_GAMESTAGE;f++) {
			for(x=0;x<ER_PIECE;x++) {
				print_pers_values(buf, &(p->piecetosquare), 64, f, BLACK, x);
				sprintf(b2, "PieceToSquare, stage %d, side %d, piece %d\n%s", f, 1, x, buf);
				LOGGER_2("PERS:",b2,"");
			}
		}
		for(x=0;x<ER_SIDE;x++) {
			for(f=0;f<ER_GAMESTAGE;f++) {
				print_pers_values3(buf, &(p->mob_val), 6, f, x, PAWN);
				sprintf(b2,"Mobility, stage %d, side %d, piece %d, %s\n", f, x, 0, buf);
				LOGGER_2("PERS:",b2,"");
				print_pers_values3(buf, &(p->mob_val), 9, f, x, KNIGHT);
				sprintf(b2,"Mobility, stage %d, side %d, piece %d, %s\n", f, x, 1, buf);
				LOGGER_2("PERS:",b2,"");
				print_pers_values3(buf, &(p->mob_val), 14, f, x, BISHOP);
				sprintf(b2,"Mobility, stage %d, side %d, piece %d, %s\n", f, x, 2, buf);
				LOGGER_2("PERS:",b2,"");
				print_pers_values3(buf, &(p->mob_val), 15, f, x, ROOK);
				sprintf(b2,"Mobility, stage %d, side %d, piece %d, %s\n", f, x, 3, buf);
				LOGGER_2("PERS:",b2,"");
				print_pers_values3(buf, &(p->mob_val), 29, f, x, QUEEN);
				sprintf(b2,"Mobility, stage %d, side %d, piece %d, %s\n", f, x, 4, buf);
				LOGGER_2("PERS:",b2,"");
				print_pers_values3(buf, &(p->mob_val), 9, f, x, KING);
				sprintf(b2,"Mobility, stage %d, side %d, piece %d, %s\n", f, x, 5, buf);
				LOGGER_2("PERS:",b2,"");
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
		LOGGER_1("INFO: Personality file: ",docname, " loaded.\n");
	}
	personality_dump(p);
	meval_table_gen(p->mat, p, 0);
	meval_table_gen(p->mate_e, p, 1);
return p;
}

int copyPers(personality *source, personality *dest) {
	memcpy(dest, source, sizeof(personality));
return 0;
}

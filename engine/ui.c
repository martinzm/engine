#include "ui.h"
#include "bitmap.h"
#include "generate.h"
#include "attacks.h"
#include "movgen.h"
#include "search.h"
#include "tests.h"
#include "hash.h"
#include "pers.h"
#include "utils.h"
#include "evaluate.h"
#include "globals.h"

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include <ctype.h>
#include <pthread.h>

#define NUM_THREADS 1
#define INPUT_BUFFER_SIZE 16384

//board bs[NUM_THREADS];
// komunikace mezi enginem a uci rozhranim
// bs je nastavovano uci rozhranim a je pouzivano enginem na vypocty
board bs;

// engine state rika
// uci state dtto pro uci stav
// engine_stop - 0 ok, 1 zastav! pouziti pokud je engine "THINKING"
int engine_state;
int uci_state;

int tell_to_engine(char *s){
	fprintf(stdout, "%s", s);
	LOGGER_0("TO_E:",s,"");
	return 0;
}

int uci_send_bestmove(int b){
	char buf[50], b2[50];
//	if(b!=0){
	LOGGER_4("INFO:","bestmove sending","\n");
	sprintfMoveSimple(b, buf);
	sprintf(b2,"bestmove %s\n", buf);
	tell_to_engine(b2);
//	}
	LOGGER_4("INFO:","bestmove sent","\n");
	return 0;
}

/*
 * engine states
 * 0 - make_quit
 * 4 - stop_thinking
 * 1 - stopped
 * 3 - start_thinking
 * 2 - thinking
 */


void *engine_thread(void *arg){
	tree_store * moves;
	char buf[100];
	board *b;

	moves = (tree_store *) malloc(sizeof(tree_store));

	b=(board *)arg;
	LOGGER_3("THREAD:","started","\n");
	while (engine_state!=MAKE_QUIT){
		if(engine_state==START_THINKING ){
			// start thinking
			engine_state=THINKING;
//			sprintf(buf,"Started thinking!, E:%d U:%d", engine_state, uci_state);
//			LOGGER_3("THREAD:",buf,"\n");
//			DEB_2(printBoardNice(&bs));

			IterativeSearch(b, 0-iINFINITY, iINFINITY ,0 , b->uci_options.depth, b->side,1, moves);
			engine_state=STOPPED;
			uci_state=2;
			if(b->bestmove!=0) uci_send_bestmove(b->bestmove);
			else {
				LOGGER_3("INFO:","no bestmove!","\n");
				uci_send_bestmove(moves->tree[0][0].move);
			}
		}
		if(engine_state==STOP_THINKING) {
			sprintf(buf,"already stopped!,  E:%d U:%d", engine_state, uci_state);
			LOGGER_3("THREAD:",buf,"\n");
			engine_state=STOPPED;
		}
		sprintf(buf,"idle,  E:%d U:%d", engine_state, uci_state);
		LOGGER_4("THREAD:",buf,"\n");
		sleep(1);
	}
	free(moves);
	LOGGER_3("THREAD:","quit","\n");
	pthread_exit(NULL);
}

int handle_uci(){
	char buff[1024];
	sprintf(buff,"id name ENGINE v0.19.4 %s %s\n",__DATE__,__TIME__);
	tell_to_engine(buff);
	sprintf(buff,"id author Martin Zampach\n");
	tell_to_engine(buff);
	sprintf(buff,"uciok\n");
	tell_to_engine(buff);
	return 0;
}

int handle_newgame(board *bs){
	setup_normal_board(bs);
	LOGGER_1("INFO:","newgame\n","");
	return 0;
}

// potrebuje specialni fixy pro
// promotion - spec flag
// ep - spec flag
// castle - spec flag, fix dest pole
int move_filter_build(char *str, int *m){
	char *tok, *b2;
	int i,a,b,c,d,q,l,spec, v;

	// replay moves
	i=0;
	m[0]=0;
	if(!str) return 0;
	tok = tokenizer(str," \n\r\t", &b2);
	if(!tok) return 0;
	do {
		// tah je ve tvaru AlphaNumAlphaNum(Alpha*)

		l=strlen(tok);
		a=b=c=d=q=spec=0;
		if(l>=4 && l<=5){
			if(isalpha(tok[0])) if((tolower(tok[0])>='a') && (tolower(tok[0])<='h')) {
				a=tolower(tok[0])-'a';
			}
			if(isdigit(tok[1])) if((tok[1]>='0') && (tok[1]<='9')) {
				b=tok[1]-'0'-1;
			}
			if(isalpha(tok[2])) if((tolower(tok[2])>='a') && (tolower(tok[2])<='h')) {
				c=tolower(tok[2])-'a';
			}
			if(isdigit(tok[3])) if((tok[3]>='0') && (tok[3]<='9')) {
				d=tok[3]-'0'-1;
			}
// promotion
			q=ER_PIECE;
			if(l==5) {
				spec=SPECFLAG;
				if(isalpha(tok[4])) switch (tolower(tok[4])) {
				case 'b':
					q=BISHOP;
					break;
				case 'n':
					q=KNIGHT;
					break;
				case 'r':
					q=ROOK;
					break;
				case 'q':
					q=QUEEN;
					break;
				default:
					spec=0;
					break;
				}
			}
			// ep from, to, PAWN, SPECFLAG
			// castling e1g1 atd
			v=PackMove(b*8+a, d*8+c,q, spec);
			m[i++]=v;
		}
		if(!b2) break;
		tok = tokenizer(b2," \n\r\t", &b2);
	} while(tok);

	m[i++]=0;
	return i;
}

int handle_position(board *bs, char *str){
attack_model att;

	char *tok, *b2, bb[100];
	int m[301], mm[301], i, a;

	if(engine_state!=STOPPED) {
		sprintf(bb,"Not stopped!, E:%d U:%d", engine_state, uci_state);
		LOGGER_2("UCI: INFO:",bb,"\n");
		engine_stop=1;
		engine_state=STOP_THINKING;

		sleep(1);
		while(engine_state!=STOPPED) {
			sprintf(bb,"Stopping!, E:%d U:%d", engine_state, uci_state);
			LOGGER_3("UCI: INFO:",bb,"\n");
			engine_stop=1;
			engine_state=STOP_THINKING;
			sleep(1);
		}
	}

	tok = tokenizer(str," \n\r\t",&b2);
	while(tok){
		LOGGER_3("PARSE:",tok,"\n");

		if(!strcasecmp(tok,"fen")) {
			LOGGER_1("INFO: FEN+moves",b2,"\n");
			setup_FEN_board(bs,b2);
			tok = tokenizer(b2," \n\r\t", &b2);
			tok = tokenizer(b2," \n\r\t", &b2);
			tok = tokenizer(b2," \n\r\t", &b2);
			tok = tokenizer(b2," \n\r\t", &b2);
			tok = tokenizer(b2," \n\r\t", &b2);
			tok = tokenizer(b2," \n\r\t", &b2);
//			break;
		} else if (!strcasecmp(tok,"startpos")) {
			LOGGER_1("INFO: startpos",b2,"\n");
			setup_normal_board(bs);
			DEB_2(printBoardNice(bs));
//			break;
		} else if (!strcasecmp(tok,"moves")) {
// build filter moves
			move_filter_build(b2,m);
			a=0;
			mm[1]=0;
			DEB_2(printBoardNice(bs));
			while(m[a]!=0) {
				mm[0]=m[a];
//				eval(bs, &att, bs->pers);
//				printBoardNice(bs);
				i=alternateMovGen(bs, mm);
				if(i!=1) {
					LOGGER_2("INFO3:","move problem!\n","");
					abort();
// abort
				}
				DEB_1(sprintfMove(bs, mm[0], bb));
				LOGGER_1("MOVES parse:",bb, "\n");
				MakeMove(bs, mm[0]);
				a++;
			}
			printBoardNice(bs);

//play moves
//			LOGGER_3("INFO: moves",b2, "\n");
			break;
		}
		tok = tokenizer(b2," \n\r\t", &b2);
	}
return 0;
}

int ttest_def(char *str){
int i;
	i=atoi(str);
	if(i==0) i=-1;
//	timedTest("test_pozice.epd", 60000, 999999);
//	timedTest("test_a.epd", i,90);
	timed2_def(i, 24, 100);
	return 0;
}

int ttest_remis(char *str){
int i;
	i=atoi(str);
	if(i==0) i=10000;
	timed2_remis(i, 24, 100);
	return 0;
}

int ttest_def2(char *str){
int i;
	i=atoi(str);
	if(i==0) i=3600;
//	timed2Test("matein4.epd", i,90, 100);
	timed2Test("test_a.epd", i,90, 100);
	return 0;
}

int thash_def(char *str){
int i;
	i=atoi(str);
	if(i==0) i=420000;
	timed2Test("test_hash.epd", i,250, 100);
	return 0;
}

int ttest_wac(char *str){
int i;
	i=atoi(str);
	if(i==0) i=10000;
	timed2Test("test_wac.epd", i,90, 100);
	return 0;
}

int ttest_wac2(char *str){
int i;
	i=atoi(str);
	if(i==0) i=300000;
	timed2Test("test_a.epd", i,90, 100);
	return 0;
}

int mtest_def(){
	movegenTest("test_pozice.epd");
	return 0;
}

int handle_go(board *bs, char *str){
	int n, moves, time, inc, basetime, cm, lag;

	char buff[100];
	char *i[100];

	if(engine_state!=STOPPED) {
		sprintf(buff,"Not stopped!, E:%d U:%d", engine_state, uci_state);
		LOGGER_2("UCI: INFO:",buff,"\n");
		engine_stop=1;
		engine_state=STOP_THINKING;

		sleep(1000);
		while(engine_state!=STOPPED) {
			sprintf(buff,"Stopping!, E:%d U:%d", engine_state, uci_state);
			LOGGER_2("UCI: INFO:",buff,"\n");
			engine_stop=1;
			engine_state=STOP_THINKING;
			sleep(1000);
		}
	}

// ulozime si aktualni cas co nejdrive...
	bs->time_start=readClock();


	lag=150; //miliseconds
	//	initialize ui go options

	bs->uci_options.engine_verbose=1;

	bs->uci_options.binc=0;
	bs->uci_options.btime=0;
	bs->uci_options.depth=999999;
	bs->uci_options.infinite=0;
	bs->uci_options.mate=0;
	bs->uci_options.movestogo=0;
	bs->uci_options.movetime=0;
	bs->uci_options.ponder=0;
	bs->uci_options.winc=0;
	bs->uci_options.wtime=0;
	bs->uci_options.search_moves[0]=0;

	bs->uci_options.nodes=0;

	bs->time_move=0;
	bs->time_crit=0;

// if option is not sent, such option should not affect/limit search

	LOGGER_3("PARSEx:",str,"\n");
	n=indexer(str, " \n\r\t",i);
	sprintf(buff, "%i",n);
	LOGGER_3("PARSE: indexer ",buff,"\n");

	if((n=indexof(i,"wtime"))!=-1) {
// this time is left on white clock
		bs->uci_options.wtime=atoi(i[n+1]);
		LOGGER_3("PARSE: wtime",i[n+1],"\n");
	}
	if((n=indexof(i,"btime"))!=-1) {
		bs->uci_options.btime=atoi(i[n+1]);
		LOGGER_3("PARSE: btime",i[n+1],"\n");
	}
	if((n=indexof(i,"winc"))!=-1) {
		bs->uci_options.winc=atoi(i[n+1]);
		LOGGER_3("PARSE: winc",i[n+1],"\n");
	}
	if((n=indexof(i,"binc"))!=-1) {
		bs->uci_options.binc=atoi(i[n+1]);
		LOGGER_3("PARSE: binc",i[n+1],"\n");
	}
	if((n=indexof(i,"movestogo"))!=-1) {
// this number of moves till next time control
		bs->uci_options.movestogo=atoi(i[n+1]);
		LOGGER_3("PARSE: movestogo",i[n+1],"\n");
	}
	if((n=indexof(i,"depth"))!=-1) {
// limit search do this depth
		bs->uci_options.depth=atoi(i[n+1]);
		LOGGER_3("PARSE: depth",i[n+1],"\n");
	}
	if((n=indexof(i,"nodes"))!=-1) {
// limit search to this number of nodes
		bs->uci_options.nodes=atoi(i[n+1]);
		LOGGER_3("PARSE: nodes",i[n+1],"\n");
	}
	if((n=indexof(i,"mate"))!=-1) {
// search for mate this deep
		bs->uci_options.mate=atoi(i[n+1]);
		LOGGER_3("PARSE: mate",i[n+1],"\n");
	}
	if((n=indexof(i,"movetime"))!=-1) {
// search exactly for this long
		bs->uci_options.movetime=atoi(i[n+1]);
		LOGGER_3("PARSE: movetime",i[n+1],"\n");
	}
	if((n=indexof(i,"infinite"))!=-1) {
// search forever
		bs->uci_options.infinite=1;
		LOGGER_3("PARSE: infinite","","\n");
	}
	if((n=indexof(i,"ponder"))!=-1) {
		bs->uci_options.ponder=1;
		LOGGER_3("PARSE: ponder","","\n");
	}
	if((n=indexof(i,"searchmoves"))!=-1) {
//		uci_options.searchmoves=atoi(i[n+1]);
		LOGGER_3("PARSE: searchmoves",i[n+1],"\n");
	}

	// pred spustenim vypoctu jeste nastavime limity casu
	if(bs->uci_options.infinite!=1) {
		if(bs->uci_options.movetime!=0) {
// pres time_crit nejede vlak
// time_move 
			bs->time_move=bs->uci_options.movetime*10;
			bs->time_crit=bs->uci_options.movetime-lag;
		} else {
			if(bs->uci_options.movestogo==0){
// sudden death
				moves=35; //fixme
			} else moves=bs->uci_options.movestogo;
			if((bs->side==0)) {
				time=bs->uci_options.wtime;
				inc=bs->uci_options.winc;
				cm=bs->uci_options.btime-bs->uci_options.wtime;
			} else {
				time=bs->uci_options.btime;
				inc=bs->uci_options.binc;
				cm=bs->uci_options.wtime-bs->uci_options.btime;
			}
			if(time>0) {
				basetime=((time-lag*(moves)-inc)/(moves)+inc);
				if(cm>0) basetime*=0.8; //!!!
				bs->time_crit=basetime-lag;
				if(bs->time_crit<lag) bs->time_crit=lag;
				basetime*=0.75;
				if(basetime<lag) basetime=lag;
				bs->time_move=basetime;
			}
		}
	}
	DEB_2(printBoardNice(bs));
	engine_stop=0;
	invalidateHash();

	uci_state=4;
	engine_state=START_THINKING;
	LOGGER_1("UCI:","go activated","\n");
	sleep(1);

	return 0;
}

int handle_stop(){
	char buf[100];
	LOGGER_1("UCI: INFO:","STOP has been received from UI","\n");
	while(engine_state!=STOPPED) {
		sprintf(buf,"running, E:%d U:%d", engine_state, uci_state);
		LOGGER_3("UCI: INFO:",buf,"\n");
		engine_state=STOP_THINKING;
		engine_stop=1;
		sleep(1);
	}
	sprintf(buf,"stopped, E:%d U:%d", engine_state, uci_state);
	LOGGER_3("UCI: INFO:",buf,"\n");
	return 0;
}

int uci_loop(int second){
	pthread_attr_t attr;
	pthread_t threads[NUM_THREADS];
	char *buff, *tok, *b2;
	char b3[100];
	void *status;

	int inp_len, bytes_read;
	int position_setup=0;
	board *b;
//	b=&bs;
	b=malloc(sizeof(board)*1);

//	open_log(DEBUG_FILENAME);
	engine_state=STOPPED;
//	t=357;
	pthread_attr_init(&attr);

	/*
	 * initialize and run thread
	 */

	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	//for(i=0;i<NUM_THREADS;i++) {
		pthread_create(&threads[0],&attr, engine_thread, (void *) b);
	//}
	pthread_attr_destroy(&attr);

	buff = (char *) malloc(INPUT_BUFFER_SIZE+1);
	inp_len=INPUT_BUFFER_SIZE;
	uci_state=1;

	LOGGER_1("INFO:","UCI started\n","");

/*
 * 	setup personality
 */

	if(second) {
		b->pers=(personality *) init_personality("pers2.xml");
	} else {
		b->pers=(personality *) init_personality("pers.xml");
	}

	/*
	 * parse and dispatch UCI protocol/commands
	 * uci_states
	 * 0 - quit
	 * 1 - non uci/waiting for uci
	 * 2 - uci handled, idle
	 * 3 - computing
	 */

	while(uci_state!=0){

		/*
		 * wait & get line from standard input
		 */
		bytes_read=getline(&buff,(size_t*) (&inp_len), stdin);
		if(bytes_read==-1){
			LOGGER_1("INFO:","input read error!\n","");
			break;
		}
		else{
reentry:
			LOGGER_2("FROM:",buff,"");
			tok = tokenizer(buff," \n\r\t",&b2);
			while(tok){
				sprintf(b3," %d\n",uci_state);
				LOGGER_3("PARSE:",tok,b3);

				if(!strcasecmp(tok,"quit")) {
					uci_state=0;
					engine_stop=1;
					break;
				} else if (!strcasecmp(tok,"isready")) {
					tell_to_engine("readyok\n");
					break;
				}

				if(uci_state==1) {
					if(!strcmp(tok,"uci")) {
						handle_uci();
						uci_state=2;
						break;
					}
					if(!strcmp(tok,"perft")) {
						perft2_def(1,7,0);
						break;
					}
					if(!strcmp(tok,"perft1")) {
						perft2("test_perft.epd",1, 7, 1);
					}
					if(!strcmp(tok,"perft2")) {
						perft2("test_perft.epd",1, 11, 1);
					}
					if(!strcmp(tok,"perft3")) {
						perft2("test_perftsuite.epd",2, 11, 0);
					}
					if(!strcmp(tok, "testsee")) {
						see_test();
					}
					if(!strcmp(tok,"ttdef")) {
						ttest_def(b2);
						break;
					}
					if(!strcmp(tok,"ttremis")) {
						ttest_remis(b2);
						break;
					}
					if(!strcmp(tok,"ttfile")) {
						ttest_def2(b2);
						break;
					}
					if(!strcmp(tok,"wac")) {
						ttest_wac(b2);
						break;
					}
					if(!strcmp(tok,"wac2")) {
						ttest_wac2(b2);
						break;
					}
					if(!strcmp(tok,"mts")) {
						mtest_def();
						break;
					}
					if(!strcmp(tok,"thash")) {
						thash_def("0");
						break;
					}
					if(!strcmp(tok, "mtst")) {
//						strcpy(buff, "position startpos moves e2e4 g8f6 e4e5 f6g8 d2d4 b8c6 g1f3 d7d6 f1b5 a7a6 b5c6 b7c6 e1g1 f7f6 h2h3 d6e5 d4e5 d8d1 f1d1 c8d7 a2a3 e8c8 b1c3 e7e6 e5f6 g7f6 c1e3 g8e7 e3c5 e7d5 c3e4 f6f5");
//						strcpy(buff, "position startpos moves e2e4 c7c6 g1f3 d7d5 b1c3 g8f6 e4d5 c6d5 f1b5 b8c6 b5c6 b7c6 f3e5 d8b6 e1g1");
						strcpy(buff, "position startpos moves b2b3 e7e5 c1b2 b8c6 e2e3 g8f6 f1b5 f8d6 b1a3 e8g8 a3c4 a7a6 b5c6 d7c6 d2d3 c8g4 f2f3 g4e6 b2e5 e6c4 e5d6 c7d6 b3c4 d8e8 d1e2 e8e5 a1d1 a8d8 g1h3 e5a5 e2d2 a5d2 e1d2 a6a5 h1f1 h7h6 e3e4 b7b5 d2c1 a5a4 c4b5 c6b5 h3f4 d8c8 d3d4 b5b4 c1b1 a4a3 f4d3 c8d8 d3b4 d8b8 c2c3 d6d5 e4e5 f6d7 d1d3 b8b5 b1c1 f8c8 c1b1 f7f6 e5e6 d7f8 e6e7 f8g6 d3e3 g6e7 b1a1 e7c6 b4c6 c8c6 e3e8 g8h7 e8d8 c6c3 d8d7 c3c2 d7a7 b5b4 a7a3 b4d4 g2g4 c2h2 a3a7 d4d2 a7a5 d5d4 a5a4 h2g2 a1b1 d4d3 b1a1 d2f2 f1b1 f2c2 b1d1 c2f2 f3f4 g2g4 a4d4 g4g2 d4a4 f2d2 d1d2 g2d2 a4d4 h7g6 d4d7 h6h5 a1b1 g6f5 d7g7 d2d1 b1b2 f5e4 g7e7 e4f4 b2c3 f6f5 e7h7 f4g5 h7d7 d1a1 d7a7 a1d1 a7d7 d1c1 c3b3 c1b1 b3c3 b1c1 c3b4 c1a1 a2a3 a1d1 b4c3 g5f6 d7d3 d1c1 c3b3 c1g1 d3d2 g1e1 d2h2 f6g5 h2g2 g5f6 g2h2 f6g5 h2g2 g5h6 g2f2 e1e5 f2d2 e5e8 d2d6 h6h7 d6d7 h7g8 a3a4 f5f4 d7d5 e8b8 b3a3 h5h4 d5g5 g8f7 g5f5 f7e6 f5f4 b8h8 f4e4 e6d6 e4d4 d6e5 d4d3 h4h3 d3d1 h3h2 d1h1 h8h4 a4a5 e5f4 a5a6 h4h7 a3b2 f4g3 b2c3 g3g2 h1b1 h7h3 c3b2 h3h6 b2c3 h6c6 c3d4 c6d6 d4c4 d6a6 b1b2 g2g3 b2b3 g3g2 b3b2 g2g3 b2b3 g3g4 b3b1 g4f3 b1b3 f3g4 b3b1 g4f3 b1b3 f3e4 b3h3 a6c6 c4b3 c6b6 b3c2 b6a6 c2b1 a6b6 b1a1 b6a6 a1b1 a6b6 b1a2 h2h1r h3h1 b6a6 a2b2 a6a7 h1e1 e4f3 e1f1 f3e3 f1e1 e3d2 e1h1 a7g7 b2b3 d2e3 b3c4 g7a7 h1e1 e3f2 e1e5 f2f3 e5e6 a7a2");
						uci_state=2;
						goto reentry;
					}
				} else if(uci_state==2){
					if(!strcasecmp(tok,"ucinewgame")) {
						handle_newgame(b);
						position_setup=1;
						break;
					} else if(!strcasecmp(tok,"position")){
						handle_position(b, b2);
						break;
					} else if(!strcasecmp(tok,"my")){
//hack
//						strcpy(buff,"startpos moves e2e3 g8f6 d1f3 d7d5 f1b5 c7c6 b5d3 b8d7 f3f5 d7c5 f5f4 c5d3 c2d3 d8d6 f4f3 g7g6 b1c3 c8f5 d3d4 f8h6 g1e2 e8g8\n");
//						strcpy(buff,"startpos moves e2e3 g8f6 d1f3\n");
//						strcpy(buff,"fen r2qk2r/p2nbppp/bpp1p3/3p4/2PP4/1PB3P/P2NPPBP/R2QK2R b KQkq - 2 22 moves e8h8 e1g1\n");
						strcpy(buff, "fen r1b1kb1r/p3pppp/1qp2n2/3pN3/8/2N5/PPPP1PPP/R1BQK2R w KQkq - 3 8 moves e1g1");
						handle_position(b, buff);
						position_setup=1;
						break;
					} else if(!strcasecmp(tok,"go")){
						if(!position_setup) {
							handle_newgame(b);
							position_setup=1;
						}
						handle_go(b, b2);
						break;
					}
				} else if(uci_state==4){
					if(!strcasecmp(tok,"stop")){
						handle_stop();
						uci_state=2;
						break;
					} else {
					}
				}
				tok = tokenizer(b2," \n\r\t", &b2);
			}
			//
		}
	}
	sprintf(b3,"exiting...\n");
	LOGGER_2("INFO:", b3, "");

	/*
	 * wait for threads to quit
	 */

	engine_state=MAKE_QUIT;
	sleep(1);
//	for(i=0;i<NUM_THREADS;i++) {
		pthread_join(threads[0], &status);
//}
	//	pthread_exit(NULL);
	free(b->pers);
	LOGGER_1("INFO:","UCI stopped\n","");
//	close_log();
	return 0;
}

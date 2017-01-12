
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

//#define NUM_THREADS 1
#define INPUT_BUFFER_SIZE 16384

#ifdef WIN32
#include <windows.h>
#elif _POSIX_C_SOURCE >= 199309L
#include <time.h>   // for nanosleep
#else
#include <unistd.h> // for usleep
#endif

void sleep_ms(int milliseconds) // cross-platform sleep function
{
#ifdef WIN32
    Sleep(milliseconds);
#elif _POSIX_C_SOURCE >= 199309L
    struct timespec ts;
    ts.tv_sec = milliseconds / 1000;
    ts.tv_nsec = (milliseconds % 1000) * 1000000;
    nanosleep(&ts, NULL);
#else
    usleep(milliseconds * 1000);
#endif
}


//board bs[NUM_THREADS];
// komunikace mezi enginem a uci rozhranim
// bs je nastavovano uci rozhranim a je pouzivano enginem na vypocty
//board bs;

// engine state rika
// uci state dtto pro uci stav
// engine_stop - 0 ok, 1 zastav! pouziti pokud je engine "THINKING"
int engine_state;
int uci_state;

int tell_to_engine(char *s){
	fprintf(stdout, "%s", s);
	LOGGER_0("TO_E: %s\n",s);
	return 0;
}

int uci_send_bestmove(MOVESTORE b){
	char buf[50], b2[50];
//	if(b!=0){
	LOGGER_4("INFO: bestmove sending\n");
	sprintfMoveSimple(b, buf);
	sprintf(b2,"bestmove %s\n", buf);
	tell_to_engine(b2);
//	}
	LOGGER_4("INFO: bestmove sent\n");
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
	board *b;

	moves = (tree_store *) malloc(sizeof(tree_store));

	b=(board *)arg;
	engine_stop=1;
	LOGGER_3("THREAD: started\n");
	while (engine_state!=MAKE_QUIT){
		switch (engine_state) {
		case START_THINKING:
			engine_stop=0;
			engine_state=THINKING;
			IterativeSearch(b, 0-iINFINITY, iINFINITY ,0 , b->uci_options.depth, b->side,b->pers->start_depth, moves);
			engine_state=STOPPED;
			uci_state=2;
			if(b->bestmove!=0) uci_send_bestmove(b->bestmove);
			else {
				LOGGER_3("INFO: no bestmove!\n");
				uci_send_bestmove(moves->tree[0][0].move);
			}
			break;
		case STOPPED:
			sleep_ms(1);
			break;
		}
	}
	free(moves);
	LOGGER_3("THREAD: quit\n");
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
	LOGGER_1("INFO: newgame\n");
	return 0;
}

// potrebuje specialni fixy pro
// promotion - spec flag
// ep - spec flag
// castle - spec flag, fix dest pole
int move_filter_build(char *str, MOVESTORE *m){
	char *tok, *b2;
	int i,a,b,c,d,q,spec;
	MOVESTORE v;
	size_t l;
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
			// ep from, to, PAWN
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

char *tok, *b2, bb[100];
int  i, a;
MOVESTORE m[301],mm[301];

	if(engine_state!=STOPPED) {
		LOGGER_3("UCI: INFO: Not stopped!, E:%d U:%d\n", engine_state, uci_state);
//		engine_stop=1;
		engine_state=STOP_THINKING;

		sleep(1);
		while(engine_state!=STOPPED) {
			LOGGER_3("UCI: INFO: Stopping!, E:%d U:%d\n", engine_state, uci_state);
//			engine_stop=1;
			engine_state=STOP_THINKING;
			sleep_ms(1);
		}
	}

	tok = tokenizer(str," \n\r\t",&b2);
	while(tok){
		LOGGER_4("PARSE: %s\n",tok);

		if(!strcasecmp(tok,"fen")) {
			LOGGER_4("INFO: FEN+moves %s\n",b2);
			setup_FEN_board(bs,b2);
			tok = tokenizer(b2," \n\r\t", &b2);
			tok = tokenizer(b2," \n\r\t", &b2);
			tok = tokenizer(b2," \n\r\t", &b2);
			tok = tokenizer(b2," \n\r\t", &b2);
			tok = tokenizer(b2," \n\r\t", &b2);
			tok = tokenizer(b2," \n\r\t", &b2);
//			break;
		} else if (!strcasecmp(tok,"startpos")) {
			LOGGER_4("INFO: startpos %s\n",b2);
			setup_normal_board(bs);
//			DEB_2(printBoardNice(bs));
//			break;
		} else if (!strcasecmp(tok,"moves")) {
// build filter moves
			move_filter_build(b2,m);
			a=0;
			mm[1]=0;
			DEB_4(printBoardNice(bs));
			while(m[a]!=0) {
				mm[0]=m[a];
//				eval(bs, &att, bs->pers);
//				printBoardNice(bs);
				i=alternateMovGen(bs, mm);
				if(i!=1) {
					LOGGER_2("INFO3: move problem!\n");
					abort();
// abort
				}
				DEB_1(sprintfMove(bs, mm[0], bb));
				LOGGER_1("MOVES parse: %s\n",bb);
				MakeMove(bs, mm[0]);
				a++;
			}
//			printBoardNice(bs);

//play moves
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
	if(i==0) i=10000;
	timed2Test("test_a.epd", i,8, 9999);
	return 0;
}

int thash_def(char *str){
int i;
	i=atoi(str);
	if(i==0) i=3600000;
	timed2Test("test_hash.epd", i, 200, 100);
	return 0;
}

int thash_def_comp(char *str){
int i;
	i=atoi(str);
	if(i==0) i=3600000;
	timed2Test_comp("test_hash.epd", i, 200, 100);
	return 0;
}

int ttsts_def(char *str){
int i;
	i=atoi(str);
	if(i==0) i=10000;
	timed2STS(i, 200, 9999);
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

	char *i[100];

	if(engine_state!=STOPPED) {
		LOGGER_2("UCI: INFO: Not stopped!, E:%d U:%d\n", engine_state, uci_state);
		engine_stop=1;
//		engine_state=STOP_THINKING;

		sleep_ms(1000);
		while(engine_state!=STOPPED) {
			LOGGER_2("UCI: INFO: Stopping!, E:%d U:%d\n", engine_state, uci_state);
			engine_stop=1;
//			engine_state=STOP_THINKING;
			sleep_ms(1000);
		}
	}

// ulozime si aktualni cas co nejdrive...
	bs->time_start=readClock();

	lag=100; //miliseconds
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

	LOGGER_4("PARSEx: %s\n",str);
	n=indexer(str, " \n\r\t",i);
	LOGGER_4("PARSE: indexer %i\n",n);

	if((n=indexof(i,"wtime"))!=-1) {
// this time is left on white clock
		bs->uci_options.wtime=atoi(i[n+1]);
		LOGGER_4("PARSE: wtime %s\n",i[n+1]);
	}
	if((n=indexof(i,"btime"))!=-1) {
		bs->uci_options.btime=atoi(i[n+1]);
		LOGGER_4("PARSE: btime %s\n",i[n+1]);
	}
	if((n=indexof(i,"winc"))!=-1) {
		bs->uci_options.winc=atoi(i[n+1]);
		LOGGER_4("PARSE: winc %s\n",i[n+1]);
	}
	if((n=indexof(i,"binc"))!=-1) {
		bs->uci_options.binc=atoi(i[n+1]);
		LOGGER_4("PARSE: binc %s\n",i[n+1]);
	}
	if((n=indexof(i,"movestogo"))!=-1) {
// this number of moves till next time control
		bs->uci_options.movestogo=atoi(i[n+1]);
		LOGGER_4("PARSE: movestogo %s\n",i[n+1]);
	}
	if((n=indexof(i,"depth"))!=-1) {
// limit search do this depth
		bs->uci_options.depth=atoi(i[n+1]);
		LOGGER_4("PARSE: depth %s\n",i[n+1]);
	}
	if((n=indexof(i,"nodes"))!=-1) {
// limit search to this number of nodes
		bs->uci_options.nodes=atoi(i[n+1]);
		LOGGER_4("PARSE: nodes %s\n",i[n+1]);
	}
	if((n=indexof(i,"mate"))!=-1) {
// search for mate this deep
		bs->uci_options.mate=atoi(i[n+1]);
		LOGGER_4("PARSE: mate %s\n",i[n+1]);
	}
	if((n=indexof(i,"movetime"))!=-1) {
// search exactly for this long
		bs->uci_options.movetime=atoi(i[n+1]);
		LOGGER_4("PARSE: movetime %s\n",i[n+1]);
	}
	if((n=indexof(i,"infinite"))!=-1) {
// search forever
		bs->uci_options.infinite=1;
		LOGGER_4("PARSE: infinite\n");
	}
	if((n=indexof(i,"ponder"))!=-1) {
		bs->uci_options.ponder=1;
		LOGGER_4("PARSE: ponder\n");
	}
	if((n=indexof(i,"searchmoves"))!=-1) {
//		uci_options.searchmoves=atoi(i[n+1]);
		LOGGER_4("PARSE: searchmoves %s",i[n+1]);
	}

	// pred spustenim vypoctu jeste nastavime limity casu
	if(bs->uci_options.infinite!=1) {
		if(bs->uci_options.movetime!=0) {
// pres time_crit nejede vlak
// time_move - target time
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
				basetime=((time-lag*(moves+2)-inc)/(moves+2)+inc);
				if(cm>0) {
					basetime*=8; //!!!
					basetime/=10;
				}
				if(basetime<lag) basetime=lag;
				bs->time_crit=3*basetime;
				bs->time_move=3*basetime/2;
			}
		}
	}
	DEB_2(printBoardNice(bs));
//	engine_stop=0;
	invalidateHash();

//	bs->time_start=readClock();

	bs->pers->start_depth=1;
	uci_state=4;
	engine_state=START_THINKING;

	LOGGER_4("UCI: go activated\n");
	sleep_ms(1);

	return 0;
}

int handle_stop(){
	LOGGER_4("UCI: INFO: STOP has been received from UI\n");
	while(engine_state!=STOPPED) {
		LOGGER_4("UCI: INFO: running, E:%d U:%d\n", engine_state, uci_state);
		engine_stop=1;
		sleep_ms(1);
	}
	LOGGER_4("UCI: INFO: stopped, E:%d U:%d", engine_state, uci_state);
	return 0;
}

board * start_threads(){
	board *b;
	pthread_attr_t attr;
	b=malloc(sizeof(board)*1);
	engine_state=STOPPED;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	pthread_create(&(b->engine_thread),&attr, engine_thread, (void *) b);
	pthread_attr_destroy(&attr);
return b;
}

int stop_threads(board *b){
void *status;
	engine_state=MAKE_QUIT;
	sleep_ms(1);
	pthread_join(b->engine_thread, &status);
return 0;
}

int uci_loop(int second){
	char *buff, *tok, *b2;

	size_t inp_len;
	int bytes_read;
	int position_setup=0;
	board *b;
	
	b=start_threads();
	uci_state=1;

	buff = (char *) malloc(INPUT_BUFFER_SIZE+1);
	inp_len=INPUT_BUFFER_SIZE;

	LOGGER_4("INFO: UCI started\n");

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
		bytes_read=(int) getline(&buff, (&inp_len), stdin);
		if(bytes_read==-1){
			LOGGER_1("INFO: input read error!\n");
			break;
		}
		else{
reentry:
			LOGGER_2("FROM:%s",buff);
			tok = tokenizer(buff," \n\r\t",&b2);
			while(tok){
				LOGGER_4("PARSE: %d %s\n",uci_state,tok);

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
					if(!strcmp(tok,"tthash")) {
						thash_def("0");
						break;
					}
					if(!strcmp(tok,"tthashc")) {
						thash_def_comp("0");
						break;
					}
					if(!strcmp(tok,"ttsts")) {
						ttsts_def("10000");
						break;
					}
					if(!strcmp(tok, "mtst")) {
//						strcpy(buff, "position startpos moves e2e4 g8f6 e4e5 f6g8 d2d4 b8c6 g1f3 d7d6 f1b5 a7a6 b5c6 b7c6 e1g1 f7f6 h2h3 d6e5 d4e5 d8d1 f1d1 c8d7 a2a3 e8c8 b1c3 e7e6 e5f6 g7f6 c1e3 g8e7 e3c5 e7d5 c3e4 f6f5");
//						strcpy(buff, "position startpos moves e2e4 c7c6 g1f3 d7d5 b1c3 g8f6 e4d5 c6d5 f1b5 b8c6 b5c6 b7c6 f3e5 d8b6 e1g1");
						strcpy(buff, "position startpos moves d2d3 d7d5 c1f4");
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
						position_setup=1;
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
					} else if(!strcasecmp(tok,"gox")){
						strcpy(buff,"go movetime 1000");
						goto reentry;
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
	LOGGER_4("INFO: exiting...\n");
	stop_threads(b);
	free(b->pers);
	LOGGER_1("INFO: UCI stopped\n");
	free(buff);
	return 0;
}

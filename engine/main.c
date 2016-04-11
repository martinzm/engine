/*
 *
 * $Id: main.c,v 1.50.4.7 2006/02/09 20:30:07 mrt Exp $
 *
 */

#include "bitmap.h"
#include "generate.h"
#include "evaluate.h"
#include "attacks.h"
#include "movgen.h"
#include "search.h"
#include "tests.h"
#include "hash.h"
#include "pers.h"
#include "ui.h"
#include "utils.h"
#include "openings.h"
#include "globals.h"

#include <stdio.h>
#include <stdlib.h>

personality default_pers;

int main (int argc, char **argv)
{
char logn[512];
int i, second, book;

	if(parse_cmd_line_check_sec(argc, argv)) {
		second=1;
		generate_log_name(DEBUG_FILENAME,"_B_", logn);
	}
	else {
		second=0;
		generate_log_name(DEBUG_FILENAME,"_A_", logn);
	}
	open_log(logn);
	LOGGER_1("INFO:","Logging opened\n","");

	setvbuf(stdout, NULL, _IOLBF, 16384);
	setvbuf(stdin, NULL, _IOLBF, 1024);

	generate_rook(attack.maps[ROOK]);
	generate_bishop(attack.maps[BISHOP]);
	generate_knight(attack.maps[KNIGHT]);
	generate_king(attack.maps[KING]);
	generate_w_pawn_moves(attack.pawn_move[WHITE]);
	generate_w_pawn_attack(attack.pawn_att[WHITE]);
	generate_b_pawn_moves(attack.pawn_move[BLACK]);
	generate_b_pawn_attack(attack.pawn_att[BLACK]);

	generate_ep_mask(attack.ep_mask);
	
	init_nmarks();
	generate_rays();
	generate_attack_norm(attack.attack_norm);
	generate_attack_r45L(attack.attack_r45L);
	generate_attack_r45R(attack.attack_r45R);
	generate_attack_r90R(attack.attack_r90R);

	generate_w_passed_pawn_mask(attack.passed_p[WHITE]);
	generate_b_passed_pawn_mask(attack.passed_p[BLACK]);
	generate_w_back_pawn_mask(attack.back_span_p[WHITE]);
	generate_b_back_pawn_mask(attack.back_span_p[BLACK]);
	generate_file_mask(attack.file);
	generate_rank_mask(attack.rank);
	generate_iso_w_pawn_mask(attack.isolated_p);
	generate_color_map(attack.color_map);
	
	generate_topos();
	generate_distance(attack.distance);
	generate_lefthalf(attack.lefthalf, attack);
	generate_righthalf(attack.righthalf, attack);
	generate_uphalf(attack.uphalf, attack);
	generate_downhalf(attack.downhalf, attack);
	generate_pawn_surr(attack.pawn_surr, attack);
	
//	generateRandomFile("Xrandoms.h");


//	b.pers=(personality *) init_personality("pers.xml");
	
	initRandom();
	initHash();
	clear_killer_moves();
	setup_options();
//    val=IterativeSearch(&b, 0-INFINITY, INFINITY ,depth , depth, b.side, move);
//	open_log();
//	close_log();
//	keyTest_def();
	//i=uci_loop();

	LOGGER_1("INFO:","Opening book\n","");
	book=open_open("book.bin");
//	timedTest("test_a.epd", 7200000,155);

//	epd_parse("cwex2500.pgn", "ax1.pgn");
//	epd_driver("test.pgn");

	i=uci_loop(second);
//	timedTest_def();
//	timedTest("test_tah.epd", 300000, 88888);
//	timedTest("test_p1.epd", 3000000, 2);
	
//	perft("test_perft.epd",1, 6, 1);
//	perft("test_perftsuite.epd",1, 6);
//	perft("test_perft_se.epd",1, 6, 1);
//	movegenTest("test_pozice.epd");
//	timedTest("test_pozice.epd", 6000000, 12);
//	timedTest("test_suite_arasan.epd", 30000,80);
//	timedTest("test_suite_bk.epd", 120000,80);
// minuta = 60000
//	free(b.pers);
	close_open();
	LOGGER_1("INFO:","Book closed\n","");
	LOGGER_1("INFO:","Finishing...\n","");
	close_log();
    return 0;
}

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
int second;
int book;
int i;

	if(parse_cmd_line_check_sec(argc, argv)) {
		second=1;
		generate_log_name(DEBUG_FILENAME,"_B_", logn);
	}
	else {
		second=0;
		generate_log_name(DEBUG_FILENAME,"_A_", logn);
	}
	open_log(logn);
	LOGGER_1("INFO: Logging opened\n");

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
	generate_rays(attack.rays, attack.rays_int);
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
	
	generate_topos(attack.ToPos);
	generate_distance(attack.distance);
	generate_lefthalf(attack.lefthalf, attack);
	generate_righthalf(attack.righthalf, attack);
	generate_uphalf(attack.uphalf, attack);
	generate_downhalf(attack.downhalf, attack);
	generate_pawn_surr(attack.pawn_surr, attack);

	initRandom();
	initHash();
	clear_killer_moves();

	LOGGER_1("INFO: Opening book\n");
	book=open_open("book.bin");

//	i=uci_loop(second);
	timed2STS(i, 200, 9999);
	close_open();
	LOGGER_1("INFO: Book closed\n");
	LOGGER_1("INFO: Finishing...\n");
	close_log();
    return 0;
}

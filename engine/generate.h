/*
 *
 * $Id: generate.h,v 1.12.6.6 2006/02/09 20:30:07 mrt Exp $
 *
 */

#ifndef GENERATE_H
#define GENERATE_H

#include "bitmap.h"

void generate_rook(BITVAR norm[]);
void generate_bishop(BITVAR norm[]);
void generate_knight(BITVAR norm[]);
void generate_king(BITVAR norm[]);
void generate_w_pawn_moves(BITVAR norm[]);
void generate_w_pawn_attack(BITVAR norm[]);
void generate_b_pawn_moves(BITVAR norm[]);
void generate_b_pawn_attack(BITVAR norm[]);
void generate_ep_mask(BITVAR norm[]);

void generate_file_mask(BITVAR map[64]);
void generate_rank_mask(BITVAR map[64]);
void generate_iso_w_pawn_mask(BITVAR map[64]);

void generate_rays(BITVAR [64][64], BITVAR [64][64]);
void generate_rays_dir(BITVAR [64][64]);
void generate_topos(int *);
void generate_directions(BITVAR [64][8]);

void generate_w_passed_pawn_mask(BITVAR map[64]);
void generate_b_passed_pawn_mask(BITVAR map[64]);
void generate_w_back_pawn_mask(BITVAR map[64]);
void generate_b_back_pawn_mask(BITVAR map[64]);

void generate_color_map(int map[64]);
void generate_distance(int map[64][64]);
void generate_uphalf(BITVAR map[64], att_mov att);
void generate_downhalf(BITVAR map[64], att_mov att);
void generate_lefthalf(BITVAR map[64], att_mov att);
void generate_righthalf(BITVAR map[64], att_mov att);
void generate_pawn_surr(BITVAR map[64], att_mov att);

void generate_attack_r45R(BITVAR map[64][256]);
void generate_attack_r45L(BITVAR map[64][256]);
void generate_attack_r90R(BITVAR map[64][256]);
void generate_attack_norm(BITVAR map[64][256]);
void setup_normal_board(board *b);
void setup_FEN_board(board *b, char * fen);
void writeEPD_FEN(board *b, char *fen, int epd, char *option);

void printboard(board *b) ;
void empty_board(board *b);

#endif

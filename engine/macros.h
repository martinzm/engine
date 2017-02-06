// text option, b->, init/print suffix, init value
#undef MLINE

/* 
	xml parameter keyword, name of variable, type of parameter, default value
*/
/*
 * parameters are initiates as follows
 * general_option directy
 * gamestage options for stages 0 and 1
 * values as 6 values for stage 0 followed by 6 values for stage 1
 * passer as side, 8x values for stage0, 8x values for stage1, side, 8x values for stage0, 8x values for stage1,
 * if side==2 it is only one record and s_r tells whether to swap from white to black
 */
	
#define MLINE (x,y,z,s_r,i)

#define M_P(...) __VA_ARGS__

#define E_OPTS \
MLINE(NMP_allowed, NMP_allowed, _general_option, 0, 0)\
MLINE(NMP_reduction, NMP_reduction, _general_option, 0, 1)\
MLINE(NMP_min_depth, NMP_min_depth, _general_option, 0, 2)\
MLINE(LMR_reduction, LMR_reduction, _general_option, 0, 0)\
MLINE(LMR_start_move, LMR_start_move, _general_option, 0, 3)\
MLINE(LMR_remain_depth, LMR_remain_depth, _general_option, 0, 3)\
MLINE(IID_remain_depth, IID_remain_depth, _general_option, 0, 4)\
MLINE(NEGAMAX, negamax, _general_option, 0, 1)\
MLINE(use_hash, use_hash, _general_option, 0, 0)\
MLINE(use_ttable, use_ttable, _general_option, 0, 1)\
MLINE(use_killer, use_killer, _general_option, 0, 0)\
MLINE(use_aspiration, use_aspiration, _general_option, 0, 0)\
MLINE(use_quiesce, use_quiesce, _general_option, 0, 0)\
MLINE(quiesce_check_depth_limit, quiesce_check_depth_limit, _general_option, 0, 1)\
MLINE(PVS_full_moves, PVS_full_moves, _general_option, 0, 2)\
MLINE(Quiesce_PVS_full_moves, Quiesce_PVS_full_moves, _general_option, 0, 9999)\
MLINE(PVS_root_full_moves, PVS_root_full_moves, _general_option, 0, 9999)\
MLINE(check_extension, check_extension, _general_option, 0, 1)\
MLINE(check_nodes_count, check_nodes_count, _general_option, 0, 12)\
MLINE(bishopboth, bishopboth, _gamestage, 0, M_P(500,500) ) \
MLINE(rook_to_pawn, rook_to_pawn, _gamestage, 0, M_P(125,125) ) \
MLINE(isolated_penalty, isolated_penalty, _gamestage, 0, M_P(-250,-250) ) \
MLINE(backward_penalty, backward_penalty, _gamestage, 0, M_P(-250,-250) ) \
MLINE(backward_fix_penalty, backward_fix_penalty, _gamestage, 0, M_P(-50,-50) ) \
MLINE(doubled_penalty, doubled_penalty, _gamestage, 0, M_P(-150,-150) )\
MLINE(pawn_ah_penalty, pawn_ah_penalty, _gamestage, 0, M_P(-150,-150) )\
MLINE(passer_bonus, passer_bonus, _passer, 1, M_P(2, 0,50,100,150,250,500,1000,0, 0,1000,1500,2000,2750,3500,5000,0) )\
MLINE(material, Values, _values, 0, M_P(1000,3250,3250,5000,9750,0,1000,3250,3250,5000,9750,0))


/*
 * chybi
 * passer bonus
 * pst
 * mobility
 * 
 */

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
MLINE(simple_EVAL, simple_EVAL, _general_option, 0, 0)\
MLINE(NMP_allowed, NMP_allowed, _general_option, 0, 1)\
MLINE(NMP_reduction, NMP_reduction, _general_option, 0, 2)\
MLINE(NMP_min_depth, NMP_min_depth, _general_option, 0, 2)\
MLINE(NMP_search_reduction, NMP_search_reduction, _general_option, 0, 0)\
MLINE(LMR_reduction, LMR_reduction, _general_option, 0, 3)\
MLINE(LMR_start_move, LMR_start_move, _general_option, 0, 4)\
MLINE(LMR_remain_depth, LMR_remain_depth, _general_option, 0, 3)\
MLINE(IID_remain_depth, IID_remain_depth, _general_option, 0, 4)\
MLINE(quiesce_phase_limit, quiesce_phase_limit, _general_option, 0, 30)\
MLINE(quiesce_phase_bonus, quiesce_phase_bonus, _general_option, 0, 2000)\
MLINE(use_hash, use_hash, _general_option, 0, 1)\
MLINE(use_ttable, use_ttable, _general_option, 0, 1)\
MLINE(use_ttable_prev, use_ttable_prev, _general_option, 0, 0)\
MLINE(ttable_clearing, ttable_clearing, _general_option, 0, 2)\
MLINE(use_killer, use_killer, _general_option, 0, 0)\
MLINE(use_aspiration, use_aspiration, _general_option, 0, 2500)\
MLINE(use_quiesce, use_quiesce, _general_option, 0, 1)\
MLINE(quiesce_check_depth_limit, quiesce_check_depth_limit, _general_option, 0, 1)\
MLINE(PVS_full_moves, PVS_full_moves, _general_option, 0, 2)\
MLINE(Quiesce_PVS_full_moves, Quiesce_PVS_full_moves, _general_option, 0, 1)\
MLINE(PVS_root_full_moves, PVS_root_full_moves, _general_option, 0, 1)\
MLINE(check_extension, check_extension, _general_option, 0, 1)\
MLINE(NEGAMAX, negamax, _general_option, 0, 1)\
MLINE(check_nodes_count, check_nodes_count, _general_option, 0, 3)\
MLINE(eval_BIAS, eval_BIAS, _general_option, 0, 0)\
MLINE(futility_depth, futility_depth, _general_option, 0, 2)\
MLINE(bishopboth, bishopboth, _gamestage, 0, M_P(500,500) ) \
MLINE(rook_on_seventh, rook_on_seventh, _gamestage, 0, M_P(300, 0) )\
MLINE(rook_on_open, rook_on_open, _gamestage, 0, M_P(150, 50) )\
MLINE(rook_on_semiopen, rook_on_semiopen, _gamestage, 0, M_P(150, 150) )\
MLINE(rook_to_pawn, rook_to_pawn, _gamestage, 0, M_P(125,125) ) \
MLINE(pawn_ah_penalty, pawn_ah_penalty, _gamestage, 0, M_P(-150,-150) )\
MLINE(isolated_penalty, isolated_penalty, _gamestage, 0, M_P(-250,-250) ) \
MLINE(pawn_protect, pawn_protect, _gamestage, 0, M_P(100,50) ) \
MLINE(backward_penalty, backward_penalty, _gamestage, 0, M_P(-250,-250) ) \
MLINE(backward_fix_penalty, backward_fix_penalty, _gamestage, 0, M_P(-50,-50) ) \
MLINE(doubled_penalty, doubled_penalty, _gamestage, 0, M_P(-150,-150) )\
MLINE(passer_bonus, passer_bonus, _passer, 0, M_P(2, 0,1000,500,250,200,150,100,0, 0,5000,3500,2750,2000,1500,1000,0) )\
MLINE(king_s_pdef, king_s_pdef, _passer, 0, M_P(2, 0,0,-100,-200,-200,-200,-200,-250, 0,0,0,0,0,0,0,0) )\
MLINE(king_s_patt, king_s_patt, _passer, 0, M_P(2, -150,-150,-100,-50,0,0,0,0, 0,0,0,0,0,0,0,0) )\
MLINE(pawn_blocked_penalty, pawn_blocked_penalty, _passer, 0, M_P(2, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0) )\
MLINE(pawn_stopped_penalty, pawn_stopped_penalty, _passer, 0, M_P(2, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0) )\
MLINE(material, Values, _values, 0, M_P(1000,3250,3250,5000,9750,0,1000,3250,3250,5000,9750,88888))




/*
 * chybi
 * pst
 * mobility
 * 
 */

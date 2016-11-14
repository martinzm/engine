// text option, b->, init/print suffix, init value
#undef MLINE

/* 
	xml parameter keyword, name of variable, type of parameter, default value
*/
	
#define MLINE (x,y,z,i)

#define M_P(...) __VA_ARGS__

#define E_OPTS \
MLINE(NMP_allowed, NMP_allowed, _general_option, 0)\
MLINE(NMP_reduction, NMP_reduction, _general_option, 1)\
MLINE(NMP_min_depth, NMP_min_depth, _general_option, 2)\
MLINE(LMR_reduction, LMR_reduction, _general_option, 0)\
MLINE(LMR_start_move, LMR_start_move, _general_option, 3)\
MLINE(LMR_remain_depth, LMR_remain_depth, _general_option, 3)\
MLINE(IID_remain_depth, IID_remain_depth, _general_option, 4)\
MLINE(NEGAMAX, negamax, _general_option, 1)\
MLINE(use_hash, use_hash, _general_option, 0)\
MLINE(use_ttable, use_ttable, _general_option, 1)\
MLINE(use_killer, use_killer, _general_option, 0)\
MLINE(use_aspiration, use_aspiration, _general_option, 0)\
MLINE(use_quiesce, use_quiesce, _general_option, 0)\
MLINE(quiesce_check_depth_limit, quiesce_check_depth_limit, _general_option, 1)\
MLINE(PVS_full_moves, PVS_full_moves, _general_option, 2)\
MLINE(Quiesce_PVS_full_moves, Quiesce_PVS_full_moves, _general_option, 9999)\
MLINE(PVS_root_full_moves, PVS_root_full_moves, _general_option, 9999)\
MLINE(check_extension, check_extension, _general_option, 1)\
MLINE(bishopboth, bishopboth, _gamestage, M_P(500,500) ) \
MLINE(rook_to_pawn, rook_to_pawn, _gamestage, M_P(125,125) ) \
MLINE(isolated_penalty, isolated_penalty, _gamestage, M_P(-250,-250) ) \
MLINE(backward_penalty, backward_penalty, _gamestage, M_P(-250,-250) ) \
MLINE(backward_fix_penalty, backward_fix_penalty, _gamestage, M_P(-50,-50) ) \
MLINE(doubled_penalty, doubled_penalty, _gamestage, M_P(-150,-150) )\
MLINE(pawn_ah_penalty, pawn_ah_penalty, _gamestage, M_P(-150,-150) )\
MLINE(check_nodes_count, check_nodes_count, _general_option, 12)


#ifndef UI_H
#define UI_H
#include "bitmap.h"
int uci_loop(int second);
int uci_loop2(int second);
int tell_to_engine(char *s);
int move_filter_build(char *str, MOVESTORE *m);
#endif

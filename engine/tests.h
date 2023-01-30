/*
    Carrot is a UCI chess playing engine by Martin Žampach.
    <https://github.com/martinzm/Carrot>     <martinzm@centrum.cz>

    Carrot is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Carrot is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>
*/

#ifndef TESTS_H
#define TESTS_H
#include "bitmap.h"

struct _results {
	struct _statistics stats;
	unsigned long long int time;
	int passed;
	int bestscore;
	int dm;
	char move[10];
};

//void perft(char * filename, int mix, int max, int sw);
void perft2(char *, int, int, int);
//void perft_def();
void perft2x(char *, int, int, int, int);
void perft2_def(int, int, int);
void testEPD(char * filename);
void timed2Test(char *, int, int, int);
void timed2Test_IQ(char *, int, int, int);
void timed2Test_comp(char *, int , int , int );
void timed2STS(int, int, int, char *, char *);
void timed2STSn(int, int, int, char *, char *);
void movegenTest(char *filename);
void keyTest_def(void);
void timed2_def(int time, int depth, int max);
void timed2_remis(int time, int depth, int max);
void see_test();
void see0_test();
void epd_parse(char * filename, char * f2);
void epd_driver(char * filename);

void texel_test();
void timed2Test_x(char *, int, int, int);

int computeMATIdx(board *b);
void fill_test();
void eeval_test(char *);
void pawnEvalTest(char*, int);
void king_check_test(char *, int);
void EvalCompare(char *[], int, char *[],int, int);
void eval_checker(char *, int);

#endif

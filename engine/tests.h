/*
 *
 * $Id: tests.h,v 1.1.6.3 2006/02/09 20:30:06 mrt Exp $
 *
 */

#ifndef TESTS_H
#define TESTS_H

//void perft(char * filename, int mix, int max, int sw);
void perft2(char *, int, int, int);
//void perft_def();
void perft2_def(int, int, int);
void testEPD(char * filename);
void timedTest(char *filename, int time, int depth);
void timed2Test(char *filename, int time, int depth);
void movegenTest(char *filename);
void keyTest_def(void);
void timedTest_def(void);
void timed2_def(void);
void epd_parse(char * filename, char * f2);
void epd_driver(char * filename);

int computeMATIdx(board *b);
#endif

/*
 *
 * $Id: tests.h,v 1.1.6.3 2006/02/09 20:30:06 mrt Exp $
 *
 */

#ifndef TUNER_H
#define TUNER_H
#include "bitmap.h"

int parseEPD(char *, char [100], char (*)[20], char (*)[20], char (*)[20], char (*)[20], char*, int *, char **);

typedef struct {
	personality *p;
	int stage;
} tuner_variables_pass;

void texel_test();
#endif

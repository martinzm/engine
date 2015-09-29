/*
 *
 * $Id: pers.h,v 1.1.2.5 2006/02/09 20:30:07 mrt Exp $
 *
 */
 
#ifndef PERS_H
#define PERS_H

#include "bitmap.h"

void setup_init_pers(personality * p);
int personality_dump(personality *p);
int load_personality(char *, personality *);
void * init_personality(char *docname);
int copyPers(personality *source, personality *dest);

#endif

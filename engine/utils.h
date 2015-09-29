#ifndef UTILS_H
#define UTILS_H

#include <time.h>

#define DEBUG_FILENAME "/media/sf_dump/log/debug"

#if defined (DEBUG3) || defined (DEBUG2) || defined (DEBUG1) || defined (DEBUG4)
	#define LOGGER_1(x,y,z) logger(x,y,z)
#else
	#define LOGGER_1(x,y,z) ;
#endif

#if defined (DEBUG2) || defined (DEBUG3) || defined (DEBUG4)
	#define LOGGER_2(x,y,z) logger(x,y,z)
#else
	#define LOGGER_2(x,y,z) ;
#endif

#if defined (DEBUG3) || defined (DEBUG4)
	#define LOGGER_3(x,y,z) logger(x,y,z)
#else
	#define LOGGER_3(x,y,z) ;
#endif

#if defined (DEBUG4)
	#define LOGGER_4(x,y,z) logger(x,y,z)
#else
	#define LOGGER_4(x,y,z) ;
#endif

#if defined (DEBUG3) || defined (DEBUG2) || defined (DEBUG1) || defined (DEBUG4)
	#define DEB_1(x) x
#else
	#define DEB_1(x) ;
#endif

#if defined (DEBUG3) || defined (DEBUG2) || defined (DEBUG4)
	#define DEB_2(x) x
#else
	#define DEB_2(x) ;
#endif

#if defined (DEBUG3) || defined (DEBUG4)
	#define DEB_3(x) x
#else
	#define DEB_3(x) ;
#endif

#if defined (DEBUG4)
	#define DEB_4(x) x
#else
	#define DEB_4(x) ;
#endif


int logger(char *p, char *s,char *a);
int open_log(char *filename);
int close_log(void);
char * tokenizer(char *str, char *delim, char **index);
int indexer(char *str, char *delim, char **index);
int indexof(char **index, char *str);

unsigned long long int readClock(void);

unsigned long long diffClock(struct timespec start, struct timespec end);
int readClock_wall(struct timespec *t);
int readClock_proc(struct timespec *t);
int generate_log_name(char *n, char *pref, char *b);
int parse_cmd_line_check_sec(int argc, char *argv[]);



#endif

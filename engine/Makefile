CC = gcc
LINK = gcc
LDFLAGS = -lxml2 -lpthread -lz -lm -ldl
COMPILE = gcc
CPPFLAGS = -I/usr/include/libxml2   
OBJS = attacks.o bitmap.o evaluate.o generate.o globals.o hash.o main.o movgen.o openings.o pers.o search.o stats.o tests.o ui.o utils.o 
DEPS = bitmap.h defines.h attack.h evaluate.h generate.h globals.h hash.h movgen.h openings.h randoms.h pers.h search.h ui.h utils.h tests.h
ifeq ($(MAKECMDGOALS), normal)
    CPPFLAGS += -Wall -O3 -c -MMD -MP -DDEBUG1 
    LDFLAGS += -DDEBUG1 
else 
ifeq ($(MAKECMDGOALS), prof)
    CPPFLAGS += -pg -g -DDEBUG1
    LDFLAGS += -pg -g -DDEBUG1 
else 
ifeq ($(MAKECMDGOALS), cov)
    CPPFLAGS += -fprofile-arcs -ftest-coverage -pg -g -DDEBUG1
    LDFLAGS += -lgcov --coverage -pg -g -DDEBUG1 
else 
ifeq ($(MAKECMDGOALS), debug)
    CPPFLAGS += -pg -DDEBUG3 -O0 -g3 -pedantic -Wall -Wconversion -c -MMD -MP
    LDFLAGS += -pg -DDEBUG3
else 
ifeq ($(MAKECMDGOALS), tuner)
    CPPFLAGS += -DTUNING -O3 -Wall -c -fmessage-length=0 -Wno-unused-variable -MMD -MP 
    LDFLAGS += -DTUNING
else 
ifeq ($(MAKECMDGOALS), tunerd)
    CPPFLAGS += -pg -DDEBUG3 -DTUNING -O0 -g3 -pedantic -Wall -Wconversion -c -MMD -MP
    LDFLAGS += -pg -DDEBUG3 -DTUNING
endif
endif
endif
endif
endif
endif

all: index

normal: index

prof: index

cov: index

debug: index

tuner: index

tunerd: index

clean:
	rm -rf *.o engine core gmon.out engine.exe
	
index: $(OBJS)
	$(LINK) -o engine $(OBJS) $(LDFLAGS) 

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)


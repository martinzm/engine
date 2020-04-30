CC = gcc
LINK = gcc
LDFLAGS = -lxml2 -lpthread -lz -lm -ldl
COMPILE = gcc
CPPFLAGS = -I/usr/include/libxml2   
OBJS = attacks.o bitmap.o evaluate.o generate.o globals.o hash.o main.o movgen.o openings.o pers.o search.o stats.o tests.o ui.o utils.o tuner.o
DEPS = bitmap.h defines.h attack.h evaluate.h generate.h globals.h hash.h movgen.h openings.h randoms.h pers.h search.h ui.h utils.h tests.h tuner.h
ifeq ($(MAKECMDGOALS), normal)
    CPPFLAGS += -Wall -O3 -DDEBUG1 -Wno-unused-variable 
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
    CPPFLAGS += -DDEBUG3 -O0 -g3 -pedantic -Wall -Wconversion -c -MMD -MP -Wno-unused-label -Wno-unused-variable
    LDFLAGS += -DDEBUG3
else 
ifeq ($(MAKECMDGOALS), tune)
    CPPFLAGS += -DTUNING -O3 -Wall -c -fmessage-length=0 -Wno-unused-variable -MMD -MP 
    LDFLAGS += -DTUNING
else 
ifeq ($(MAKECMDGOALS), tuned)
    CPPFLAGS += -pg -DDEBUG3 -DTUNING -O0 -g3 -pedantic -Wall -Wconversion -c -MMD -MP -fprofile-arcs -no-pie
    LDFLAGS += -pg -DDEBUG3 -DTUNING -fprofile-arcs -no-pie
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

tune: index

tuned: index

clean:
	rm -rf *.o engine core gmon.out engine.exe *.gc?? *.d

index: $(OBJS)
	$(LINK) -o engine $(OBJS) $(LDFLAGS) 

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)


CC = gcc
LINK = g++
LDFLAGS = -L/usr/lib -lxml2 -lz -liconv -lm 
COMPILE = g++ 
CPPFLAGS = -Wall -g -I/usr/include/libxml2
OBJS = search.o bitmap.o main.o generate.o attacks.o movgen.o evaluate.o xboard.o tests.o hash.o pers.o
HEADERS = attacks.h bitmap.h defines.h evaluate.h generate.h hash.h movgen.h search.h tests.h xboard.h pers.h

ifeq ($(MAKECMDGOALS), profile)
    CPPFLAGS += -pg
    LDFLAGS += -pg
else 
ifeq ($(MAKECMDGOALS), debug)
    CPPFLAGS += -pg -DDEBUG
    LDFLAGS += -pg -DDEBUG
else     
ifeq ($(MAKECMDGOALS), debug2)
    CPPFLAGS += -pg -DDEBUG -DDEBUG2
    LDFLAGS += -pg -DDEBUG -DDEBUG2
endif
endif
endif

all: index

profile: index

debug: index

debug2: index

clean:
	rm -rf *.o engine core gmon.out engine.exe
	
index: $(OBJS)
	$(LINK) -o engine $(OBJS) $(LDFLAGS) 
	
$(OBJS) : $(HEADERS)

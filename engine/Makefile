CC = gcc
LINK = g++
LDFLAGS = -lxml2 -lpthread -lz -lm -ldl
COMPILE = g++ 
CPPFLAGS = -I/usr/include/libxml2   
OBJS = attacks.o bitmap.o evaluate.o generate.o globals.o hash.o main.o movgen.o openings.o pers.o search.o stats.o tests.o ui.o utils.o 

ifeq ($(MAKECMDGOALS), normal)
    CPPFLAGS += -O3 -Wall -c -fmessage-length=0 -Wno-unused-variable -MMD -MP 
    LDFLAGS +=
else 
ifeq ($(MAKECMDGOALS), debug)
    CPPFLAGS += -pg -DDEBUG3 -O0 -g3 -pedantic -Wall -Wconversion -c -MMD -MP
    LDFLAGS += -pg -DDEBUG3
else 
ifeq ($(MAKECMDGOALS), tuner)
    CPPFLAGS += -DTUNING -O3 -Wall -c -fmessage-length=0 -Wno-unused-variable -MMD -MP 
    LDFLAGS += -DTUNING
ifeq ($(MAKECMDGOALS), tunerd)
    CPPFLAGS += -pg -DDEBUG3 -DTUNING -O0 -g3 -pedantic -Wall -Wconversion -c -MMD -MP
    LDFLAGS += -pg -DDEBUG3 -DTUNING
endif
endif
endif
endif

all: index

normal: index

debug: index

tuner: index

tunerd: index

clean:
	rm -rf *.o engine core gmon.out engine.exe
	
index: $(OBJS)
	$(LINK) -o engine $(OBJS) $(LDFLAGS) 
	

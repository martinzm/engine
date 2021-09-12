CC = gcc
LINK = gcc
LDFLAGS = -lxml2 -lpthread -lz -lm -ldl -fopenmp
COMPILE = gcc
CPPFLAGS = -I/usr/include/libxml2 -fopenmp
OBJS = attacks.o bitmap.o evaluate.o generate.o globals.o hash.o main.o movgen.o openings.o pers.o search.o stats.o tests.o ui.o utils.o tuner.o
DEPS = bitmap.h defines.h attack.h evaluate.h generate.h globals.h hash.h movgen.h openings.h randoms.h pers.h search.h ui.h utils.h tests.h tuner.h
ifeq ($(MAKECMDGOALS), normal)
    CPPFLAGS += -Wall -O1 -DDEBUG1 -Wno-unused-variable -Finline-functions -Winline
    LDFLAGS += -DDEBUG1 
else 
ifeq ($(MAKECMDGOALS), release)
    CPPFLAGS += -Wall -O3 -Wno-unused-variable -DNDEBUG -Finline-functions -Winline
    LDFLAGS += -DNDEBUG
else 
ifeq ($(MAKECMDGOALS), prof)
    CPPFLAGS += -pg -g -DNDEBUG -Finline-functions -Winline -O2 -fprofile-arcs
    LDFLAGS += -pg -g -DNDEBUG -fprofile-arcs
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
    CPPFLAGS += -DTUNING -O2 -Wall -c -fmessage-length=0 -Wno-unused-variable -MMD -MP -Finline-functions -Winline
    LDFLAGS += -DTUNING
else 
ifeq ($(MAKECMDGOALS), tuned)
    CPPFLAGS += -pg -g -DTUNING  -DDEBUG1 -O0 -g3 -Wall -c -fmessage-length=0 -Wno-unused-variable -MMD -MP -Finline-functions -Winline -fprofile-arcs 
    LDFLAGS += -pg -g -DTUNING -DDEBUG1 -fprofile-arcs 
endif
endif
endif
endif
endif
endif
endif

all: index

normal: index

release: index

prof: index

cov: index

debug: index

tune: index-t

tuned: index-t

clean:
	rm -rf *.o engine core gmon.out engine.exe *.gc?? *.d engine-t

index: $(OBJS)
	$(LINK) -o engine $(OBJS) $(LDFLAGS) 

index-t: $(OBJS)
	$(LINK) -o engine-t $(OBJS) $(LDFLAGS) 

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)


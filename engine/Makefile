CC = gcc
LINK = gcc
LDFLAGS = -lxml2 -lpthread -lz -lm -ldl -fopenmp
COMPILE = gcc
CPPFLAGS = -I/usr/include/libxml2 -fopenmp
OBJS = attacks.o bitmap.o evaluate.o generate.o globals.o hash.o main.o movgen.o openings.o pers.o search.o stats.o tests.o ui.o utils.o tuner.o
DEPS = bitmap.h defines.h attack.h evaluate.h generate.h globals.h hash.h movgen.h openings.h randoms.h pers.h search.h ui.h utils.h tests.h tuner.h
ifeq ($(MAKECMDGOALS), normal)
    CPPFLAGS += -Wall -O1 -DDEBUG1 -Wno-unused-variable -Finline-functions -Winline -pedantic -Wall
    LDFLAGS += -DDEBUG1 
endif
ifeq ($(MAKECMDGOALS), sedebug)
    CPPFLAGS += -Wall -O1 -DDEBUG1 -DSEDEBUG -Wno-unused-variable -Finline-functions -Winline -pedantic -Wall
    LDFLAGS += -DDEBUG1 
endif
ifeq ($(MAKECMDGOALS), release)
    CPPFLAGS += -Wall -O3 -Wno-unused-variable -DNDEBUG -Finline-functions -Winline
    LDFLAGS += -DNDEBUG
endif
ifeq ($(MAKECMDGOALS), prof)
    CPPFLAGS += -pg -g -DNDEBUG -Finline-functions -Winline -O2 -fprofile-arcs
    LDFLAGS += -pg -g -DNDEBUG -fprofile-arcs
endif
ifeq ($(MAKECMDGOALS), cov)
    CPPFLAGS += --coverage -DDEBUG1
    LDFLAGS += --coverage -DDEBUG1
endif
ifeq ($(MAKECMDGOALS), debug)
    CPPFLAGS += -DDEBUG1 -Og -g -pedantic -Wall -Wconversion -c -MMD -MP -Wno-unused-label -Wno-unused-variable
    LDFLAGS += -DDEBUG1 
endif
ifeq ($(MAKECMDGOALS), tune)
    CPPFLAGS += -DTUNING -O3 -Wall -c -fmessage-length=0 -Wno-unused-variable -MMD -MP -Finline-functions -Winline
    LDFLAGS += -DTUNING
endif
ifeq ($(MAKECMDGOALS), tuned)
    CPPFLAGS += -pg -g -DTUNING  -DDEBUG1 -O0 -g3 -Wall -c -fmessage-length=0 -Wno-unused-variable -MMD -MP -Finline-functions -Winline -fprofile-arcs 
    LDFLAGS += -pg -g -DTUNING -DDEBUG1 -fprofile-arcs 
endif
ifeq ($(MAKECMDGOALS), sanit)
    CPPFLAGS += -DDEBUG3 -O0 -pg -g3 -pedantic -Wall -Wconversion -c -MMD -MP -Wno-unused-label -fsanitize=address -fsanitize=undefined -fno-sanitize-recover=all -fsanitize=float-divide-by-zero -fsanitize=float-cast-overflow -fno-sanitize=null -fno-sanitize=alignment -Wno-unused-variable
    LDFLAGS += -DDEBUG3 -pg -fsanitize=address -fsanitize=undefined -fno-sanitize-recover=all -fsanitize=float-divide-by-zero -fsanitize=float-cast-overflow -fno-sanitize=null -fno-sanitize=alignment
endif

all: index

normal: index

sedebug: index

release: index

prof: index

cov: index

debug: index

sanit: index

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


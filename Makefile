CC = gcc
LD = gcc
CFLAGS = -g -O3 -pg -Wall  -pedantic -c
LDFLAGS = -g -O3 -pg -o

# libraries
LIBS = -L. -lm
INCLUDES = -I.

# Treat NT and non-NT windows the same
ifeq ($(OS),Windows_NT)
	OS = Windows
endif

ifeq ($(OS),Windows)
	EXE = .exe
	DEL = del
else	#assume Linux/Unix
	EXE =
	DEL = rm
endif

all:	CalcArboles$(EXE)

CalcArboles$(EXE):	CalcArboles.o 
		$(LD) $^ $(LIBS) $(LDFLAGS) $@

CalcArboles.o:	CalcArboles.c ConvexHull.h
		$(CC) $(INCLUDES) $(CFLAGS) $<

clean:
		$(DEL) *.o
		$(DEL) CalcArboles$(EXE)

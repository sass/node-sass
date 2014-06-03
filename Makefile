
ifeq ($(OS),Windows_NT)
	MV ?= move
	CP ?= copy /Y
	RM ?= del /Q /F
	EXESUFFIX ?= .exe
	SUFFIX ?= 2>NULL
else
	MV ?= mv -f
	CP ?= cp -f
	RM ?= rm -rf
	EXESUFFIX ?=
	SUFFIX ?=
endif

all: sass2scss

sass2scss.o: sass2scss.cpp
	g++ -Wall -c sass2scss.cpp

sass2scss: sass2scss.o
	g++ -Wall -o sass2scss -I. tool/sass2scss.cpp sass2scss.o

clean:
	ifeq ($(OS),Windows_NT)
		${RM} sass2scss.o 2>NUL
		${RM} sass2scss.exe 2>NUL
	else
		${RM} sass2scss.o
		${RM} sass2scss
	endif

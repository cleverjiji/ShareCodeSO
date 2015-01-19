SHELL := /bin/bash

EXE_DIR := exe
INCLUDE := -I./

CFLAGS := -O0 -g
CXX := g++ 
CC := gcc
ext := test

#########################################
### DO NOT MODIFY THE FOLLOWING LINES ###          
#########################################

SRC_CPP := $(wildcard *.cpp)
SRC_C := $(wildcard *.c)
SRC := ${SRC_C} ${SRC_CPP}

EXE := $(patsubst %.cpp,$(EXE_DIR)/%.$(ext),${SRC_CPP})
EXE += $(patsubst %.c,$(EXE_DIR)/%.$(ext),${SRC_C})

.PHONY: all clean test

all: $(EXE)

clean:
	@rm -rf $(EXE_DIR)  

test:
	@for i in ${EXE}; do\
		if \
		LD_PRELOAD=./../../sc.so ./$$i; \
		then echo -e "[\e[32mSUCC \e[m] \e[33;1m$$i\e[m"; \
		else echo -e "[\e[31mFAIL\e[m] \e[33;1m$$i\e[m"; exit -1; fi; \
		done


$(EXE_DIR)/%.$(ext): %.cpp
	@mkdir -p $(EXE_DIR);
	@if \
	$(CXX) ${CFLAGS} ${INCLUDE} $< -o $@; \
	then echo -e "[\e[32mCXX \e[m] \e[33m$<\e[m \e[36m->\e[m \e[33;1m$@\e[m"; \
	else echo -e "[\e[31mFAIL\e[m] \e[33m$<\e[m \e[36m->\e[m \e[33;1m$@\e[m"; exit -1; fi;

$(EXE_DIR)/%.$(ext): %.c
	@mkdir -p $(EXE_DIR);
	@if \
	$(CC) ${CFLAGS} ${INCLUDE} $< -o $@; \
	then echo -e "[\e[32mCC  \e[m] \e[33m$<\e[m \e[36m->\e[m \e[33;1m$@\e[m"; \
	else echo -e "[\e[31mFAIL\e[m] \e[33m$<\e[m \e[36m->\e[m \e[33;1m$@\e[m"; exit -1; fi;

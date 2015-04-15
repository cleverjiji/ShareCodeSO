SHELL := /bin/bash
LIB_BIN := libsc.so

BUILD_DIR := build
SRC_DIR := intercept memory libc-sc communication
TEST_DIR := test

LIBC_PATH := $(shell which ls | xargs ldd | grep libc.so | cut -d ' ' -f 3)

INCLUDE := -I./include 

OPTIMIZE := -O0 -g
EXTRA_FLAGS := -D_DEBUG -D_GNU_SOURCE -DLIBC_PATH="\""$(LIBC_PATH)"\""
CFLAGS := -fno-omit-frame-pointer $(OPTIMIZE) -fPIC -Wall ${EXTRA_FLAGS}

LIBS := -ldl -lrt
LD_FLAGS := -shared

CXX := g++ -Xlinker -rpath=/home/wangzhe/glibc/lib:/home/wangzhe/gcc/lib:/home/wangzhe/gcc/lib64 -Xlinker -I/home/wangzhe/glibc/lib/ld-2.17.so
CC := gcc -Xlinker -rpath=/home/wangzhe/glibc/lib:/home/wangzhe/gcc/lib:/home/wangzhe/gcc/lib64 -Xlinker -I/home/wangzhe/glibc/lib/ld-2.17.so

#########################################
### DO NOT MODIFY THE FOLLOWING LINES ###          
#########################################

SRC := $(foreach d,${SRC_DIR},$(wildcard ${d}/*.cpp))
SRC += $(foreach d,${SRC_DIR},$(wildcard ${d}/*.c))

OBJ := $(foreach d,${SRC_DIR},$(addprefix $(BUILD_DIR)/,$(notdir $(patsubst %.cpp,%.o,$(wildcard ${d}/*.cpp)))))
OBJ += $(foreach d,${SRC_DIR},$(addprefix $(BUILD_DIR)/,$(notdir $(patsubst %.c,%.o,$(wildcard ${d}/*.c)))))

DEP := $(foreach d,${SRC_DIR},$(addprefix $(BUILD_DIR)/,$(notdir $(patsubst %.cpp,%.d,$(wildcard ${d}/*.cpp)))))
DEP += $(foreach d,${SRC_DIR},$(addprefix $(BUILD_DIR)/,$(notdir $(patsubst %.c,%.d,$(wildcard ${d}/*.c)))))

.PHONY: all clean 

vpath %.cpp $(SRC_DIR)
vpath %.c $(SRC_DIR)

all: $(OBJ) 
	@if \
	$(CC) $(OBJ) $(LD_FLAGS) -Wl,-soname,$(LIB_BIN) -o $(LIB_BIN) $(LIBS);\
	then echo -e "[\e[32;1mLINK\e[m] \e[33m$(OBJ)\e[m \e[36m->\e[m \e[32;1m$(LIB_BIN)\e[m"; \
	else echo -e "[\e[31mFAIL\e[m] \e[33m$(OBJ)\e[m \e[36m->\e[m \e[32;1m$(LIB_BIN)\e[m"; exit -1; fi;
clean:
	@echo -e "[\e[32mCLEAN\e[m] \e[33m$(LIB_BIN) $(BUILD_DIR)/\e[m"
	@rm -rf $(LIB_BIN) build  
	@make clean -s -C ${TEST_DIR}

sinclude $(DEP)

$(BUILD_DIR)/%.d: %.cpp Makefile
	@mkdir -p $(BUILD_DIR);
	@if \
	$(CXX) ${INCLUDE} -MM $< > $@ && sed -i '1s/^/$(BUILD_DIR)\//g' $@; \
	then echo -e "[\e[32mCXX \e[m] \e[33m$<\e[m \e[36m->\e[m \e[33;1m$@\e[m"; \
	else echo -e "[\e[31mFAIL\e[m] \e[33m$<\e[m \e[36m->\e[m \e[33;1m$@\e[m"; exit -1; fi;

$(BUILD_DIR)/%.d: %.c Makefile
	@mkdir -p $(BUILD_DIR);
	@if \
	$(CC) ${INCLUDE} -MM $< > $@ && sed -i '1s/^/$(BUILD_DIR)\//g' $@;\
	then echo -e "[\e[32mCC  \e[m] \e[33m$<\e[m \e[36m->\e[m \e[33;1m$@\e[m"; \
	else echo -e "[\e[31mFAIL\e[m] \e[33m$<\e[m \e[36m->\e[m \e[33;1m$@\e[m"; exit -1; fi;

$(BUILD_DIR)/%.o: %.cpp Makefile
	@if \
	$(CXX) ${CFLAGS} ${INCLUDE} -c $< -o $@; \
	then echo -e "[\e[32mCXX \e[m] \e[33m$<\e[m \e[36m->\e[m \e[33;1m$@\e[m"; \
	else echo -e "[\e[31mFAIL\e[m] \e[33m$<\e[m \e[36m->\e[m \e[33;1m$@\e[m"; exit -1; fi;

$(BUILD_DIR)/%.o: %.c Makefile
	@if \
	$(CC) ${CFLAGS} ${INCLUDE} -c $< -o $@; \
	then echo -e "[\e[32mCC  \e[m] \e[33m$<\e[m \e[36m->\e[m \e[33;1m$@\e[m"; \
	else echo -e "[\e[31mFAIL\e[m] \e[33m$<\e[m \e[36m->\e[m \e[33;1m$@\e[m"; exit -1; fi;

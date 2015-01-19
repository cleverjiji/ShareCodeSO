SHELL= /bin/bash
SUBDIR = $(shell ls -F | grep "/")
.PHONY: all clean

all:
	@for dir in ${SUBDIR}; do\
		echo -e "[\e[32;1mENTER\e[m] \e[33m$${dir}\e[m"; \
		make -s -C $${dir};\
		echo -e "[\e[32;1mLEAVE\e[m] \e[33m$${dir}\e[m"; \
	done

clean:
	@for dir in ${SUBDIR}; do\
		echo -e "[\e[32mCLEAN\e[m] \e[33m$${dir}exe\e[m"; \
		make clean -s -C $${dir};\
	done

test:
	@for dir in ${SUBDIR}; do\
		echo -e "[\e[32mTEST\e[m] \e[33m$${dir}\e[m"; \
		make test -s -C $${dir};\
	done



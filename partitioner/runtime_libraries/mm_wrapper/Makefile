###############################################################################
#            Makefile for building the memory management wrapper              #
#                                                                             #
# This Makefile builds a POSIX-compliant shared library that acts as a        #
# wrapper for memory management functions defined in the C standard library   #
# (namely malloc, calloc, realloc and free).  In order to link against this   #
# library, copy the library built from this Makefile into your project        #
# directory and add the following switch to your command line:                #
#                                                                             #
#     gcc <arguments> <source files> -lmm_wrapper                             #
#                                                                             #
# This enables the runtime support necessary for partitioning programs.       #
###############################################################################

LIBNAME	:= libmm_wrapper.so

CC	:= gcc
CFLAGS	:= -O3 -Wall

CUDA_CC		:= nvcc
CUDA_CFLAGS	:= -O3

LIBFLAGS	:= -shared
POSIND		:= -fPIC
DEBUG		:= -g

TIMER_LOC := ../../../stopwatch/x86

SRC	:= mm_wrapper.c rbtree.c
OBJS	:= $(addprefix build/, $(SRC:.c=.o))
LIBS	:= -lrt

WRAP		:= -Wl,-wrap,malloc -Wl,-wrap,calloc -Wl,-wrap,realloc -Wl,-wrap,valloc \
			-Wl,-wrap,memalign -Wl,-wrap,free
CUDA_WRAP	:= -Xlinker "-wrap" -Xlinker "malloc" -Xlinker "-wrap" -Xlinker \
			"calloc" -Xlinker "-wrap" -Xlinker "realloc" -Xlinker "-wrap" \
			-Xlinker "free"

%/.dir:
	mkdir $*
	touch $@

all: $(LIBNAME)

build/%.o: %.c
	$(CC) $(CFLAGS) $(POSIND) -c $< -o $@

$(LIBNAME): build/.dir $(OBJS)
	$(CC) $(CFLAGS) $(DEBUG) $(WRAP) $(LIBFLAGS) $(OBJS) $(LIBS) -o $@

statistics: $(SRC) $(TIMER_LOC)/timer.c
	$(CC) $(CFLAGS) $(DEBUG) -I$(TIMER_LOC) -D_STATISTICS $(POSIND) $(WRAP) \
		$(LIBFLAGS) $(SRC) $(TIMER_LOC)/timer.c $(LIBS) -o $(LIBNAME)

debug: $(SRC) $(TIMER_LOC)/timer.c
	$(CC) $(CFLAGS) $(DEBUG) -I$(TIMER_LOC) -D_STATISTICS -D_DEBUG $(POSIND) \
		$(WRAP) $(LIBFLAGS) $(SRC) $(TIMER_LOC)/timer.c $(LIBS) -o $(LIBNAME)

test: mm_test mm_cuda_test rbtree_test

mm_test: mm_test.c $(LIBNAME)
	$(CC) $(CFLAGS) $(WRAP) -o $@ $< -L. $(LIBS) -lmm_wrapper

mm_cuda_test: mm_test.cu $(LIBNAME)
	$(CUDA_CC) $(CUDA_CFLAGS) $(CUDA_WRAP) -o $@ $< -L. $(LIBS) -lmm_wrapper

rbtree_test: rbtree_test.c rbtree.c
	$(CC) $(CFLAGS) -o $@ rbtree_test.c rbtree.c

clean: testclean
	rm -f $(LIBNAME) $(OBJS)
	rm -rf ./build/

testclean:
	rm -f mm_test
	rm -f mm_cuda_test
	rm -f rbtree_test


ifeq ($(ocl),)
OCL=/usr
else
OCL=$(ocl)
endif

ifeq ($(ocl_inc),)
OCL_INC=$(OCL)/include
else
OCL_INC=$(ocl_inc)
endif

ifeq ($(ocl_lib),)
OCL_LIB=$(OCL)/lib
else
OCL_LIB=$(ocl_lib)
endif

CC := gcc
CFLAGS := -O2 -Wall -Wl,-rpath,../
LOC := -I../include -I$(OCL_INC) -L.. -L$(OCL_LIB)

LIB := -lOpenCL_rt -lOpenCL

all: precompile

$(OCL_RT): ../libOpenCL_rt.so
	make -C .. ocl_inc=$(OCL_INC) ocl_lib=$(OCL_LIB)

precompile: $(OCL_RT) precompile.c
	$(CC) $(CFLAGS) $(LOC) -o $@ precompile.c $(LIB)

clean:
	rm -f precompile

.PHONY: all clean

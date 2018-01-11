BIN := print_opencl_info test_subdevices timer_resolution clInfo

# Let user specify location of OpenCL installation
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
CFLAGS := -O3 -Wall -Wl,-rpath,../
LOC := -I../include -I$(OCL_INC) -L.. -L$(OCL_LIB)

LIB := -lOpenCL_rt -lOpenCL

all: $(BIN)

$(OCL_RT): ../libOpenCL_rt.so
	make -C ..

print_opencl_info: print_opencl_info.c $(OCL_RT)
	$(CC) $(CFLAGS) $(LOC) -o print_opencl_info print_opencl_info.c $(LIB)

test_subdevices: test_subdevices.c $(OCL_RT)
	$(CC) $(CFLAGS) $(LOC) -o test_subdevices test_subdevices.c $(LIB)

timer_resolution: timer_resolution.c $(OCL_RT)
	$(CC) $(CFLAGS) $(LOC) -o timer_resolution timer_resolution.c $(LIB)

clInfo: clInfo.c
	$(CC) $(CFLAGS) $(LOC) -o clInfo clInfo.c $(LIB)

clean:
	rm -f $(BIN)

.PHONY: all clean

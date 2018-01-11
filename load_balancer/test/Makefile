BIN := single_client multiple_clients

OCL_RT := ../../opencl_runtime

# Common flags & files
CC := gcc
CFLAGS := -O3 -Wall \
					-I../include -L../ -Wl,-rpath,../ \
					-I$(OCL_RT)/include -L$(OCL_RT) -Wl,-rpath,$(OCL_RT)
LIB := -laira-lb -lOpenCL_rt

all: $(BIN)

single_client: single_client.c ../libaira-lb.so
	$(CC) $(CFLAGS) -o $@ $< $(LIB)

multiple_clients: multiple_clients.c ../libaira-lb.so
	$(CC) $(CFLAGS) -fopenmp -o $@ $< $(LIB)

clean:
	rm -rf $(BIN)

.PHONY: all clean

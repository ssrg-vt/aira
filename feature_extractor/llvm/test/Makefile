CC := clang
CFLAGS := -O3 -Wall
IRFLAG := -emit-llvm -S
BITCODE := -emit-llvm

SRC := test.c
BC := $(SRC:.c=.bc)
IR := $(SRC:.c=.ll)

%.bc: %.c
	$(CC) $(CFLAGS) $(BITCODE) -o $@ -c $<

%.ll: %.c
	$(CC) $(CFLAGS) $(IRFLAG) -o $@ -c $<

all: $(BC) $(IR)

clean:
	rm -rf $(BC) $(IR)

.PHONY: all clean

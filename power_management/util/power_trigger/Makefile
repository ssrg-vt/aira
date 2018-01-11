CC := gcc
CFLAGS := -O2 -Wall

HDR := power_trigger.h parallel_io.h

LIB := libpowertrigger.so
LIB_CFLAGS := $(CFLAGS) -fPIC
LIB_LDFLAGS := $(LIB_CFLAGS) -shared
LIB_HDR := $(HDR)
LIB_SRC := power_trigger.c parallel_io.c
LIB_OBJ := $(LIB_SRC:.c=.o)

BIN := power_trigger
BIN_LDFLAGS := -L. -Wl,-rpath,$(shell pwd)
BIN_HDR := power_trigger.h
BIN_SRC := main.c
BIN_LIB := -lpowertrigger

all: $(LIB) $(BIN)

%.o: %.c
	@echo "[CC] $<"
	@$(CC) $(LIB_CFLAGS) -o $@ -c $<

$(LIB): $(LIB_HDR) $(LIB_OBJ)
	@echo "[LD] $@"
	@$(CC) $(LIB_LDFLAGS) -o $(LIB) $(LIB_OBJ)

$(BIN): $(LIB) $(BIN_HDR) $(BIN_SRC)
	@echo "[CC] $@"
	@$(CC) $(CFLAGS) $(BIN_LDFLAGS) -o $(BIN) $(BIN_SRC) $(BIN_LIB)

clean:
	@echo "[RM] $(LIB_OBJ)"
	@rm -rf $(LIB_OBJ)
	@echo "[RM] $(LIB)"
	@rm -f $(LIB)
	@echo "[RM] $(BIN)"
	@rm -f $(BIN)

.PHONY: all clean

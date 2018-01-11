BIN := powermeasurement_test
PM := ../src/

CC := gcc
CFLAGS := -O3 -Wall -g
INCLUDE := -I$(PM)/lib
LDFLAGS := -L$(PM) -Wl,-rpath,$(PM)
LIBS := -lpowermeasurement

$(BIN): $(BIN).c
	$(CC) $(CFLAGS) $(INCLUDE) $(LDFLAGS) -o $@ $< $(LIBS)

clean:
	rm -f $(BIN)

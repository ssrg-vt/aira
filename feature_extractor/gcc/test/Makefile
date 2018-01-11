BIN := calc_pi

CC := g++
CFLAGS := -Wall -O3 -g -o $(BIN) -fopenmp

SRC := $(shell ls *.c)

$(BIN): $(SRC)
	$(CC) $(CFLAGS) $(SRC)

features: $(SRC)
	$(CC) $(CFLAGS) -fprofile-generate -fplugin=../libfeature_extractor.so $(SRC)
	./$(BIN)
	$(CC) $(CFLAGS) -fprofile-use -fprofile-correction -fplugin=../libfeature_extractor.so $(SRC)

clean:
	rm -rf $(BIN) *features *gcda

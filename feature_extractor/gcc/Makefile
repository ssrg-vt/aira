SRC=feature_extractor.cpp features.cpp
OBJS=feature_extractor.o features.o
TARGET=libfeature_extractor.so
LIBS=

CC=g++

CFLAGS=-O3 -Wall -I`$(CC) -print-file-name=plugin`/include
LDFLAGS=
POSIND=-fPIC
LIBFLAGS=-shared

all: feature_extractor

%.o: %.cpp %.h
	$(CC) $(CFLAGS) $(POSIND) -c $<

%.o: %.cpp
	$(CC) $(CFLAGS) $(POSIND) -c $<

feature_extractor: $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(LIBFLAGS) -o $(TARGET) $(OBJS)

clean:
	rm -f $(OBJS) $(TARGET)

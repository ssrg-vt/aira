# Include definitions
-include ../Makefile.define

INCLUDE	:= -I./include/ -I../common/
SRCS	:= $(shell ls *.cpp)
OBJS	:= $(addprefix build/, $(SRCS:.cpp=.o))

%/.dir:
	mkdir $*
	touch $@

build/%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(INCLUDE) -c $< -o $@

all: build/.dir $(OBJS)

clean:
	rm -rf build/

# Include definitions
-include ../../Makefile.define

CPPFLAGS	+= -I../../common -I../../utils/include -I$(ROSE_INCLUDE_DIR)

SRCS	:= $(shell ls *.cpp)
HEADERS	:= $(shell ls *.h)
OBJS	:= $(addprefix build/, $(SRCS:.cpp=.o)) $(shell ls ../../utils/build/*.o)

%/.dir:
	mkdir $*
	touch $@

build/%.o: %.cpp $(HEADERS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

find_compatible_architectures: build/.dir $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $@ \
	    -Wl,-rpath $(ROSE_LIB_DIR) \
	    -Wl,-rpath $(BOOST_LIB_DIR) \
	    -Wl,-rpath $(JVM_LIB_DIR) \
	    -L$(ROSE_LIB_DIR) \
	    -L$(BOOST_LIB_DIR) \
	    -L$(JVM_LIB_DIR) \
	    -lrose $(BOOST_LIBS) -ljvm

all: find_compatible_architectures

testclean:
	rm -f rose_*.c *.dot *.png

clean: testclean
	rm -rf build/ find_compatible_architectures

test: find_compatible_architectures
	./find_compatible_architectures tests/test.c -rose:skipfinalCompileStep

.PHONY: all clean test

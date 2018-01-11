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

cleanup_code: build/.dir $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $@ \
	    -Wl,-rpath $(ROSE_LIB_DIR) \
	    -Wl,-rpath $(BOOST_LIB_DIR) \
	    -Wl,-rpath $(JVM_LIB_DIR) \
	    -L$(ROSE_LIB_DIR) \
	    -L$(BOOST_LIB_DIR) \
	    -L$(JVM_LIB_DIR) \
	    -lrose $(BOOST_LIBS) -ljvm

all: cleanup_code

testclean:
	rm -f rose_*.c

clean: testclean
	rm -rf build/ cleanup_code

test: cleanup_code
	./cleanup_code tests/test.c -rose:skipfinalCompileStep

.PHONY: all clean test

# Include definitions
-include ../../Makefile.define

CPPFLAGS	+= -I../../common/ -I../../utils/include/

SRCS	:= $(shell ls *.cpp)
HEADERS	:= $(shell ls *.h)
OBJS	:= $(addprefix build/, $(SRCS:.cpp=.o)) $(shell ls ../../utils/build/*.o)

%/.dir:
	mkdir $*
	touch $@

build/%.o: %.cpp $(HEADERS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

add_scheduler_calls: build/.dir $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $@ \
	    -Wl,-rpath $(ROSE_LIB_DIR) \
	    -Wl,-rpath $(BOOST_LIB_DIR) \
	    -Wl,-rpath $(JVM_LIB_DIR) \
	    -L$(ROSE_LIB_DIR) \
	    -L$(BOOST_LIB_DIR) \
	    -L$(JVM_LIB_DIR) \
	    -lrose $(BOOST_LIBS) -ljvm

all: add_scheduler_calls

testclean:
	rm -f rose_*.c

clean: testclean
	rm -rf build/ add_scheduler_calls

test: add_scheduler_calls
	./add_scheduler_calls tests/test.c -rose:skipfinalCompileStep

.PHONY: all clean test

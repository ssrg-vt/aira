# Include definitions
-include ../../Makefile.define

CPPFLAGS  += -I../../common/ -I../../utils/include/

SRCS  := $(shell ls *.cpp)
HEADERS := $(shell ls *.h)
OBJS  := $(addprefix build/, $(SRCS:.cpp=.o)) $(shell ls ../../utils/build/*.o)

%/.dir:
	mkdir $*
	touch $@

build/%.o: %.cpp $(HEADERS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

migration_outliner: build/.dir $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $@ \
      -Wl,-rpath $(ROSE_LIB_DIR) \
      -Wl,-rpath $(BOOST_LIB_DIR) \
      -Wl,-rpath $(JVM_LIB_DIR) \
      -L$(ROSE_LIB_DIR) \
      -L$(BOOST_LIB_DIR) \
      -L$(JVM_LIB_DIR) \
      -lrose $(BOOST_LIBS) -ljvm

all: migration_outliner

testclean:
	rm -f rose_*.c
	#rm -f *.dot templates/corrected*.c

clean: testclean
	rm -rf build/ migration_outliner

cfail:
	rm *.o
	rm a.out

testsimple: migration_outliner
	./migration_outliner tests/outline1.c

.PHONY: all clean test
	

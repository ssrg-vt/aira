###############################################################################
# Makefile for the partitioner built on top of Rose.  You need to define      #
# several variables in regards to where your Rose installation is located:    #
#    ROSE_INSTALL_DIR                                                         #
#    BOOST_INSTALL_DIR                                                        #
#    JVM_LIB_DIR                                                              #
# YOU MUST DEFINE THESE FOR YOUR MACHINE/ROSE BUILD                           #
# You can define them in Makefile.override, or if you like you can run:       #
#    make ROSE_INSTALL_DIR=... BOOST_INSTALL_DIR=... JVM_LIB_DIR=...          #
###############################################################################

# Location of Rose build directory after "make install"
ROSE_INSTALL_DIR=/home/rlyerly/Research/TestGround/rose-0.9.5a_build

# Location of Boost build directory (either your native Boost directory or the
# one built by "build_rose.sh")
BOOST_INSTALL_DIR=/home/rlyerly/Research/TestGround/rose-0.9.5a_build/boost_1_47_0_build

# Location of Java library directory on your machine
JVM_LIB_DIR=/usr/lib/jvm/java-1.6.0-openjdk-amd64/jre/lib/amd64/cacao

-include Makefile.override

###############################################################################

#Location of include/library directories for Rose and Boost
ROSE_LIB_DIR=$(ROSE_INSTALL_DIR)/lib
ROSE_INCLUDE_DIR=$(ROSE_INSTALL_DIR)/include

BOOST_INCLUDE_DIR=$(BOOST_INSTALL_DIR)/include
BOOST_LIB_DIR=$(BOOST_INSTALL_DIR)/lib
BOOST_CPPFLAGS=-pthread -I$(BOOST_INCLUDE_DIR)
BOOST_LIBS=-lboost_date_time -lboost_thread -lboost_program_options \
           -lboost_regex -lboost_system -lboost_wave -lboost_filesystem 

CXX      := g++
CPPFLAGS := $(BOOST_CPPFLAGS) -Ibuild -I$(ROSE_INCLUDE_DIR)
CXXFLAGS := -Wall -O2

SRCS := $(shell ls *.cpp)
HEADERS := $(shell ls *.h)
OBJS := $(addprefix build/, $(SRCS:.cpp=.o))

%/.dir:
	mkdir $*
	touch $@

#build/%.h: build/.dir
#	echo "#error If this happened then PCH is not working" > $@
#.PRECIOUS: build/%.h

#build/%.h.gch: $(ROSE_INCLUDE_DIR)/%.h build/.dir build/%.h
#	$(CXX) $(CPPFLAGS) -I$(ROSE_INCLUDE_DIR) $(CXXFLAGS) -c $< -o $@
#.PRECIOUS: build/%.h.gch

build/%.o: %.cpp $(HEADERS) build/.dir build/rose.h.gch
	$(CXX) $(CPPFLAGS) -I$(ROSE_INCLUDE_DIR) $(CXXFLAGS) -c $< -o $@

partition: build/.dir $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $@ \
	    -Wl,-rpath $(ROSE_LIB_DIR) \
	    -Wl,-rpath $(BOOST_LIB_DIR) \
	    -Wl,-rpath $(JVM_LIB_DIR) \
	    -L$(ROSE_LIB_DIR) \
	    -L$(BOOST_LIB_DIR) \
	    -L$(JVM_LIB_DIR) \
	    -lrose $(BOOST_LIBS) -ljvm

all: partition

testclean:
	rm -f rose_test.* test*.dot test*.png \
	      rose_empty_partition_mpi.c empty_partition*.dot empty_partition*.png

clean: testclean
	rm -rf build/

test: partition
	./partition tests/small.c -rose:skipfinalCompileStep --checkAST --mpiHeader="openmpi/mpi.h" --generateAST
	for a in *.dot ; do \
		dot -T png $$a -o $${a%.dot}.png ; \
	done

.PHONY: all clean test

###############################################################################
# Makefile for the partitioner built on top of Rose.  You need to define      #
# several variables in Makefile.define to be able to compile:		      #
#    ROSE_INSTALL_DIR                                                         #
#    BOOST_INSTALL_DIR                                                        #
#    JVM_LIB_DIR                                                              #
# YOU MUST DEFINE THESE FOR YOUR MACHINE/ROSE BUILD                           #
###############################################################################

# Include definitions
-include Makefile.define

# Number of jobs, for parallel make
JOBS	:= 4

# Pre-compiled rose.h header
#PCH	:= ./common/rose.h.gch

# Partitioning passes
PASS_DIR			:= ./passes
ADD_SCHEDULER_CALLS		:= $(PASS_DIR)/add_scheduler_calls
ADD_WRAPPER_CALLS		:= $(PASS_DIR)/add_wrapper_calls
CLEANUP_CODE			:= $(PASS_DIR)/cleanup_code
FIND_COMPATIBLE_ARCHITECTURES	:= $(PASS_DIR)/find_compatible_architectures
KERNEL_INTERFACE		:= $(PASS_DIR)/kernel_interface
PARTITION_KERNELS		:= $(PASS_DIR)/partition_kernels

# Runtime libraries
LIB_DIR		:= ./runtime_libraries
MM_WRAPPER	:= $(LIB_DIR)/mm_wrapper
SCHEDULER	:= $(LIB_DIR)/scheduler

# Utilities
UTILS	:= ./utils

all: utilities partitioner_passes 
	ln -f -s scripts/src/partition_code.py partition_code

#libraries

# Build pre-compiled rose.h header
common/%.h:
	echo "#error Could not use pre-compiled rose.h header" > $@
.PRECIOUS: common/%.h

#common/%.h.gch: $(ROSE_INCLUDE_DIR)/%.h common/%.h
#	$(CXX) $(CPPFLAGS) -I$(ROSE_INCLUDE_DIR) $(CXXFLAGS) -c $< -o $@
#.PRECIOUS: common/%.h.gch

# Build partitioner passes
partitioner_passes: $(PCH)
	make all -j $(JOBS) -C $(ADD_SCHEDULER_CALLS)
	make all -j $(JOBS) -C $(ADD_WRAPPER_CALLS)
	make all -j $(JOBS) -C $(CLEANUP_CODE)
	make all -j $(JOBS) -C $(FIND_COMPATIBLE_ARCHITECTURES)
	make all -j $(JOBS) -C $(KERNEL_INTERFACE)
	make all -j $(JOBS) -C $(PARTITION_KERNELS)

# Build runtime libraries
libraries:
	make all -j $(JOBS) -C $(MM_WRAPPER)
	make all -j $(JOBS) -C $(SCHEDULER)

# Build utilities
utilities: $(PCH)
	make all -j $(JOBS) -C $(UTILS)

clean: clean_passes clean_libraries clean_utils
	rm -f partition_code
	rm -f common/rose.h common/rose.h.gch

# Clean passes
clean_passes:
	make clean -C $(ADD_SCHEDULER_CALLS)
	make clean -C $(ADD_WRAPPER_CALLS)
	make clean -C $(CLEANUP_CODE)
	make clean -C $(FIND_COMPATIBLE_ARCHITECTURES)
	make clean -C $(KERNEL_INTERFACE)
	make clean -C $(PARTITION_KERNELS)

# Clean runtime libraries
clean_libraries:
	make clean -C $(MM_WRAPPER)
	#make clean -C $(SCHEDULER)

# Clean utilities
clean_utils:
	make clean -C $(UTILS)

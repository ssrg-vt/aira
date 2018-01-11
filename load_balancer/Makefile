###############################################################################
#            Makefile for building heterogeneous load balancer                #
#                                                                             #
# This Makefile builds the load-balancing daemon & client-side communication  #
# library to allocate & receive resources on which to execute compute kernels #
###############################################################################

LIB := libaira-lb.so
SERVER := aira-lb

# Select system
ifeq ($(system),)
SYSTEM := systems/frankenstein.c
else
SYSTEM := $(system)
endif

BUILD := ./build
OCL_RT := ../opencl_runtime
ML := ../analysis/machine_learning

# Common flags & files
CC := gcc
CFLAGS := -O3 -Wall -I./include
CXX	:= g++
CXXFLAGS := $(CFLAGS) -I$(OCL_RT)/include -I$(ML)/include -std=c++11

HEADERS	:= $(shell ls include/*.h)
SRC	:= $(shell ls src/*.c)

# Library (client) flags & files
POSIND := -fPIC
LIB_FLAGS := -shared
LIB_SRC := $(shell ls src/client/*.c)
LIB_OBJS := $(subst src,$(BUILD),$(SRC:.c=.o)) \
						$(subst src/client,$(BUILD),$(LIB_SRC:.c=.o))

# Server flags & files
SRV_HEADERS := $(HEADERS) $(shell ls include/server/*.h) \
							 $(shell ls $(ML)/include/*.hh)
SRV_SRC := $(shell ls src/server/*.cpp)
SRV_OBJS := $(subst src/server,$(BUILD),$(SRV_SRC:.cpp=.o)) \
						$(subst systems,$(BUILD),$(SYSTEM:.c=.o))
SRV_LIB := -L$(OCL_RT) -L$(ML)/$(BUILD) -lm -lmpfr -lOpenCL_rt -laira-ml \
					 -lopencv_core -lopencv_ml -Wl,-rpath,$(OCL_RT) \
					 -Wl,-rpath,$(ML)/$(BUILD)

%/.dir:
	mkdir $*
	touch $@

all: $(LIB) $(SERVER) utility

$(BUILD)/%.o: ./src/%.c $(HEADERS)
	@echo "[CC-so] $<"
	@$(CC) $(CFLAGS) $(POSIND) -c $< -o $@

$(BUILD)/%.o: ./src/client/%.c $(HEADERS)
	@echo "[CC-so] $<"
	@$(CC) $(CFLAGS) $(POSIND) -c $< -o $@

$(LIB): $(BUILD)/.dir $(LIB_OBJS)
	@echo "[LD-so] $@"
	@$(CC) $(CFLAGS) $(LIB_FLAGS) $(LIB_OBJS) -o $(LIB) -lm

$(BUILD)/%.o: src/server/%.cpp $(SRV_HEADERS)
	@echo "[CXX] $<"
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD)/%.o: systems/%.c $(SRV_HEADERS)
	@echo "[CC] $<"
	@$(CC) $(CFLAGS) -c $< -o $@

$(SERVER): $(BUILD)/.dir $(SRV_OBJS) $(SRC)
	@echo "[CXX] $@"
	@$(CXX) $(CXXFLAGS) -o $@ $(SRV_OBJS) $(SRC) $(SRV_LIB)

utility:
	@$(MAKE) -C ./utility

libclean:
	@echo "[RM] $(BUILD)"
	@rm -rf $(BUILD)
	@echo "[RM] $(LIB)"
	@rm -f $(LIB)

srvclean:
	@echo "[RM] $(SERVER)"
	@rm -f $(SERVER)

clean: libclean srvclean
	@$(MAKE) -C ./utility clean

.PHONY: all utility libclean srvclean clean

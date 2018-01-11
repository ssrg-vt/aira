###############################################################################
# Definitions
###############################################################################

CC := g++
CFLAGS := -O3 -Wall -g
SO_CFLAGS := -fPIC -DPIC
INCLUDE := -I. -I./device -I./lib -I./manager -I./monitoring -I./utility -I./vendor
LDFLAGS := -ldl -lrt

ifeq ($(mode),debug)
CFLAGS += -g -D_VERBOSE
endif

# Common includes, sources & objects
HDR := $(shell ls *.h)
SRC := common.cpp
OBJ := $(addprefix build/,$(SRC:.cpp=.o))
DEVICE_HDR := $(shell ls device/*.h)
DEVICE_SRC := $(shell ls device/*.cpp)
DEVICE_OBJ := $(subst device,build,$(DEVICE_SRC:.cpp=.o))
MANAGER_HDR := $(shell ls manager/*.h)
MANAGER_SRC := $(shell ls manager/*.cpp)
MANAGER_OBJ := $(subst manager,build,$(MANAGER_SRC:.cpp=.o))
UTILITY_HDR := $(shell ls utility/*.h)
UTILITY_SRC := $(shell ls utility/*.cpp)
UTILITY_OBJ := $(subst utility,build,$(UTILITY_SRC:.cpp=.o))
COMMON_OBJ := $(OBJ) $(DEVICE_OBJ) $(MANAGER_OBJ) $(UTILITY_OBJ)

# Daemon includes, sources & objects
DAEMON_SRC := main.cpp
DAEMON_OBJ := $(addprefix build/,$(DAEMON_SRC:.cpp=.o))
MONITORING_HDR := $(shell ls monitoring/*.h)
MONITORING_SRC := $(shell ls monitoring/*.cpp)
MONITORING_OBJ := $(subst monitoring,build,$(MONITORING_SRC:.cpp=.o))

# Library includes, sources & objects
PIC_OBJ := $(subst .o,.lo,$(COMMON_OBJ))
LIBRARY_HDR := $(shell ls lib/*.h)
LIBRARY_SRC := $(shell ls lib/*.cpp)
LIBRARY_OBJ := $(subst lib,build,$(LIBRARY_SRC:.cpp=.lo))

# Main targets
BIN := PowerManager
LIB := libpowermeasurement.so

###############################################################################
# Targets
###############################################################################

all: $(BIN) $(LIB)

clean:
	@echo "[RM]  build"
	@echo "[RM]  $(BIN)"
	@echo "[RM]  $(LIB)"
	@rm -rf build $(BIN) $(LIB)

$(BIN): build/.dir $(DAEMON_OBJ) $(MONITORING_OBJ) $(COMMON_OBJ)
	@echo "[LD]  $@"
	@$(CC) $(CFLAGS) -o $@ $(DAEMON_OBJ) \
		$(MONITORING_OBJ) \
		$(COMMON_OBJ) \
		$(LDFLAGS)

$(LIB): build/.dir $(LIBRARY_OBJ) $(PIC_OBJ)
	@echo "[LD-so]  $@"
	@$(CC) $(CFLAGS) -shared -o $@ $(LIBRARY_OBJ) \
		$(PIC_OBJ) \
		$(LDFLAGS)

%/.dir:
	@mkdir -p $*
	@touch $@

######################
# Standard compilation
######################

build/%.o: %.cpp $(HDR) $(MANAGER_HDR) $(MONITORING_HDR)
	@echo "[CXX] $<"
	@$(CC) $(CFLAGS) $(INCLUDE) -o $@ -c $<

build/%.o: monitoring/%.cpp $(MONITORING_HDR)
	@echo "[CXX] $<"
	@$(CC) $(CFLAGS) $(INCLUDE) -o $@ -c $<

build/%.o: device/%.cpp $(DEVICE_HDR)
	@echo "[CXX] $<"
	@$(CC) $(CFLAGS) $(INCLUDE) -o $@ -c $<

build/%.o: manager/%.cpp $(MANAGER_HDR)
	@echo "[CXX] $<"
	@$(CC) $(CFLAGS) $(INCLUDE) -o $@ -c $<
	
build/%.o: utility/%.cpp $(UTILITY_HDR)
	@echo "[CXX] $<"
	@$(CC) $(CFLAGS) $(INCLUDE) -o $@ -c $<

#################
# PIC compilation
#################

build/%.lo: %.cpp $(HDR) $(MANAGER_HDR) $(MONITORING_HDR)
	@echo "[CXX-so] $<"
	@$(CC) $(CFLAGS) $(INCLUDE) $(SO_CFLAGS) -o $@ -c $<

build/%.lo: lib/%.cpp $(LIBRARY_HDR)
	@echo "[CXX-so] $<"
	@$(CC) $(CFLAGS) $(INCLUDE) $(SO_CFLAGS) -o $@ -c $<

build/%.lo: device/%.cpp $(DEVICE_HDR)
	@echo "[CXX-so] $<"
	@$(CC) $(CFLAGS) $(INCLUDE) $(SO_CFLAGS) -o $@ -c $<

build/%.lo: manager/%.cpp $(MANAGER_HDR)
	@echo "[CXX-so] $<"
	@$(CC) $(CFLAGS) $(INCLUDE) $(SO_CFLAGS) -o $@ -c $<
	
build/%.lo: utility/%.cpp $(UTILITY_HDR)
	@echo "[CXX-so] $<"
	@$(CC) $(CFLAGS) $(INCLUDE) $(SO_CFLAGS) -o $@ -c $<

.PHONY: all clean

CXX := g++
CXXFLAGS := -std=c++11 -O2 -Wall -g -I./include

BUILD := build

HDR := $(addprefix include/,$(shell ls include))

LIB := libaira_regression.so
LIB_CXXFLAGS := $(CXXFLAGS) -fPIC
LIB_LDFLAGS := -shared
LIB_LIBS := -lgsl -lgslcblas -lm
LIB_SRC := $(addprefix lib/,$(shell ls lib))
LIB_OBJ := $(subst lib,$(BUILD),$(LIB_SRC:.cc=.o))

BIN := fit_model
BIN_LDFLAGS := -L./ -Wl,-rpath,./
BIN_LIBS := -laira_regression -lgsl -lgslcblas -lm
BIN_SRC := $(shell ls *.cc)

all: $(LIB) $(BIN)

$(BUILD)/.dir:
	@mkdir -p ./$(BUILD)
	@touch $(BUILD)/.dir

build/%.o: lib/%.cc $(HDR)
	@echo "[CXX] $<"
	@$(CXX) $(LIB_CXXFLAGS) -o $@ -c $<

$(LIB): $(BUILD)/.dir $(LIB_OBJ)
	@echo "[LD]  $@"
	@$(CXX) $(LIB_CXXFLAGS) $(LIB_LDFLAGS) -o $@ $(LIB_OBJ) $(LIB_LIBS)

$(BIN): $(LIB) $(BIN_SRC)
	@echo "[CXX] $@"
	@$(CXX) $(CXXFLAGS) $(BIN_LDFLAGS) -o $@ $(BIN_SRC) $(BIN_LIBS)

clean:
	@echo "[RM]  $(LIB) $(BIN) $(BUILD)"
	@rm -f $(LIB) $(BIN)
	@rm -rf $(BUILD)

.PHONY: all clean

BIN := config-lb

CXX := g++
CXXFLAGS := -std=c++11 -O3 -Wall -I../include

HDR := ../include/server/config_parser.h

all: $(BIN)

config-lb: ../include/server/config_parser.h ../src/server/config_parser.cpp config-lb.cpp
	@echo "[CXX] $@"
	@$(CXX) $(CXXFLAGS) -o $@ config-lb.cpp ../src/server/config_parser.cpp

clean:
	@echo "[RM] $(BIN)"
	@rm -rf $(BIN)

.PHONY: all clean

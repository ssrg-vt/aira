HOME := $(shell pwd)

BIN := bin
LIB := lib
INC := include

all: $(BIN)/.dir $(LIB)/.dir $(INC)/.dir daemon lib power_trigger

%/.dir:
	@mkdir -p $*
	@touch $@

daemon: PowerManager
PowerManager: $(BIN)/.dir
	@$(MAKE) -C ./src PowerManager
	@cp $(HOME)/src/PowerManager $(BIN)/

lib: powermeasurement
powermeasurement: $(LIB)/.dir $(INC)/.dir
	@$(MAKE) -C ./src libpowermeasurement.so
	@cp $(HOME)/src/libpowermeasurement.so $(LIB)/
	@cp src/lib/PowerMeasurement.h $(INC)/
	@cp src/devices.h $(INC)/

power_trigger: $(BIN)/.dir $(LIB)/.dir $(INC)/.dir
	@$(MAKE) -C ./util/power_trigger
	@cp $(HOME)/util/power_trigger/libpowertrigger.so $(LIB)/
	@cp $(HOME)/util/power_trigger/power_trigger $(BIN)/
	@cp $(HOME)/util/power_trigger/power_trigger.h $(INC)/

clean:
	@$(MAKE) -C ./src clean
	@$(MAKE) -C ./util/power_trigger clean
	@rm -rf $(BIN)
	@rm -rf $(LIB)
	@rm -rf $(INC)

.PHONY: all clean

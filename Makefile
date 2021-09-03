########
# Config
########


OUT_DIR=build
YOSYS_BUILD_DIR=$(OUT_DIR)/yosys
YOSYS_SRC := yosys/addFi.cc
YOSYS_MODULE := $(patsubst yosys/%.cc,$(YOSYS_BUILD_DIR)/%.so,$(YOSYS_SRC))

YOSYS_TEST_OUT=$(OUT_DIR)/tests

# Set YOSYS_SHELL to stop after the last test command to get a Yosys shell,
# if it is not set, a log is created.
YOSYS_SHELL ?= 0
ifeq ($(YOSYS_SHELL),1)
	YOSYS_SHELL_OR_LOG = -p 'shell'
else
# Expansion of the variable happens inside `yosys_standard`
	YOSYS_SHELL_OR_LOG = > $(YOSYS_TEST_OUT)/$(2).log
endif

include tests/Makefile.inc

#######
# Build
#######

all: yosys

$(YOSYS_TEST_OUT) $(YOSYS_BUILD_DIR):
	mkdir -p $@

.PHONY: yosys
yosys: $(YOSYS_MODULE)
$(YOSYS_MODULE): $(YOSYS_SRC) | $(YOSYS_BUILD_DIR)
	yosys-config --build $@ $<


example_verilator: example_verilator_simple example_verilator_full

example_verilator_simple: example_verilator_simple_build example_verilator_simple_run

example_verilator_simple_build: example_verilator_simple_fi
	fusesoc --cores-root . run --target=sim --setup --build towoe:fifoss:example_verilator_simple

example_verilator_simple_run:
	./build/towoe_fifoss_example_verilator_simple_0.1/sim-verilator/Vtop

example_verilator_simple_fi: $(YOSYS_MODULE)
	cd example/simple/ && yosys -m $(realpath $(YOSYS_MODULE)) -c ./tcl/yosys_fi.tcl

example_verilator_full: example_verilator_full_build example_verilator_full_run

example_verilator_full_build: example_verilator_full_fi
	fusesoc --cores-root . run --target=sim --setup --build towoe:fifoss:example_verilator_full

example_verilator_full_run:
	./build/towoe_fifoss_example_verilator_full_0.1/sim-verilator/Vtop -n 100 -l -z 4,20

example_verilator_full_fi: $(YOSYS_MODULE)
	cd example/full/ && yosys -m $(realpath $(YOSYS_MODULE)) -c ./tcl/yosys_fi.tcl

.PHONY: clean
clean:
	rm -rf $(OUT_DIR)

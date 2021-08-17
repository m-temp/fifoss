########
# Config
########

BUILD_OUT=build
YOSYS_SRC := yosys/addFi.cc
YOSYS_MODULE := $(patsubst yosys/%.cc,$(BUILD_OUT)/%.so,$(YOSYS_SRC))

# Set YOSYS_SHELL to stop after the last test command to get a Yosys shell,
# if it is not set, a log is created.
YOSYS_SHELL ?= 0
ifeq ($(YOSYS_SHELL),1)
	YOSYS_SHELL_OR_LOG = -p 'shell'
else
# Expansion of the variable happens inside `yosys_standard`
	YOSYS_SHELL_OR_LOG = > $(BUILD_OUT)/$(2).log
endif

include tests/Makefile.inc

#######
# Build
#######

all: yosys

$(BUILD_OUT):
	@mkdir -p $@

.PHONY: yosys
yosys: $(YOSYS_MODULE)
$(YOSYS_MODULE): $(YOSYS_SRC) | $(BUILD_OUT)
	yosys-config --build $@ $<

.PHONY: clean
clean:
	rm -rf $(BUILD_OUT)

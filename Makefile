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

.PHONY: yosys
yosys: $(YOSYS_MODULE)
$(YOSYS_MODULE):
	yosys-config --build $@ yosys/addFi.cc

# Variable to execute Yosys tests
# Arguments:
# 1 SystemVerilog source file
# 2 Test name
# 3 (optional) Arguments for addFi
# 4 (optional) Additional commands before addFi
# 5 (optional) Additional commands after addFi
define yosys_standard
	yosys -m $(YOSYS_MODULE)\
		-p 'read_verilog -sv $(1)'\
		-p 'hierarchy -check -top top'\
		-p 'proc'\
		$(4)\
		-p 'dump'\
		-p 'debug addFi $(3)'\
		$(5)\
		-p 'dump'\
		-p 'write_verilog $(BUILD_OUT)/$(2).v'\
		-p 'show -prefix $(BUILD_OUT)/$(2) -format dot top'\
		$(YOSYS_SHELL_OR_LOG)
endef

# Target to execute all tests
.PHONY: test
test: yosys flipflop_all

flipflop_all: flipflop flipflop_clean flipflop_ff flipflop_comb

# Target to run tests separately, make sure to create/update the Yosys module
# first.
flipflop: tests/flipflop.sv
	$(call yosys_standard,$<,$@)

flipflop_clean: tests/flipflop.sv
	$(call yosys_standard,$<,$@,-type xor,-p 'clean',-p 'clean')

flipflop_ff: tests/flipflop.sv
	$(call yosys_standard,$<,$@,-no-comb)

flipflop_comb: tests/flipflop.sv
	$(call yosys_standard,$<,$@,-no-ff)

.PHONY: clean
clean:
	rm -f $(BUILD_OUT)/*

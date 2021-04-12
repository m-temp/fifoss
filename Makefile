.PHONY: yosys
yosys:
	yosys-config --build build/insertFIXor.so yosys/insertFIXor.cc

.PHONY: clean
clean:
	rm -f build/*

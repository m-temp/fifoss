.PHONY: yosys
yosys:
	yosys-config --build build/addFi.so yosys/addFi.cc

.PHONY: clean
clean:
	rm -f build/*

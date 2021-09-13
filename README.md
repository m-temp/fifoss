# Fault Injection Framework Open Source Software (FIFOSS)

Framework to inject faults into netlists.

## Yosys module

The Yosys pass `addFi` changes a design by inserting additional logic to
selected cells and forwards control signals to the top-level module.

### Requirements

Yosys, together with with the development files must be installed.
To create the module the binary `yosys-config` must be available.
This module uses current features of Yosys.
Installation from a recent source is advisable.
The preferred compiler for Yosys is Clang.
Tested with:

    $ yosys --version
    Yosys 0.9+4274 (git sha1 e6dd4db0a, clang 12.0.1 -fPIC -Os)

### Building

In order to create the Yosys module run:

    $ make yosys

The output can be found in 'build/addFi.so'.

### Using the `addFi` pass

To use the `addFi` pass Yosys must be told where to find it first.
Start Yosys with the path to the shared object.

    $ yosys -m build/addFi.so

Now Yosys will know the additional command `addFi`.
Print the help of the command:

    yosys> help addFi

An example flow could look similar to this:

    yosys> read_verilog -sv my_design.sv
    yosys> hierarchy -check -top top
    yosys> proc
    yosys> opt
    yosys> addFi
    yosys> clean

To show more information about what is happening, print the debug messages:

    yosys> debug addFi

### Tests
A few simple SystemVerilog test cases exists to investigate and visualize
the behaviour of `addFi`.
*There is no check for correctness.*
If there is an error in the `addFi` pass it might however create a failure
when running the tests.

Run all tests with

    $ make test

Call a specific test

    $ make flipflop_orig

For each test various output files are created and stored in the `build`
directory.
- Verilog file after inserting the fault injection logic
- Graph of the top-level module (can be viewed with 'xdot')
- Log output from Yosys

To further investigate a specific test (or all) the variable `YOSYS_SHELL` can
be set to start a Yosys shell after the run instead of creating the log output
file.

    $ make flipflop_orig_opt YOSYS_SHELL=1

## Verilator class

After the design was altered by Yosys the next step is to make use of the fault
injection.
When using Verilator the class `FaultInjection` provides support to utilizes
this in a simulation.

### Requirements

Verilator must be installed.
To run the simulation example version 4.200 or higher must be used.
Tested with:

    $ verilator --version
    Verilator 4.211 devel rev v4.210-70-gc69ddc46f

### Using `FaultInjection`

Create an instance of `FaultInjection` and set the length of the fault
injection bus. Configure the mode, by either setting it directly or by parsing
command line arguments. Call `UpdateInsert()` in each clock cycle and provide a
pointer to the fault injection signal.

This could look roughly like this:

    ...
    FaultInjection fi(100); // Fault bus length 100
    fi.SetModePrecise(40, 50); // Fault in cycle 40 and bit 50
    ...
    while() {
        top->clk = !top->clk;
        if (top->clk) {
            fi.UpdateInsert(top->fi_combined); // Pointer to fault injection bus
        }
    }
    ...

### Running the examples

Two examples are provided.
A minimal usage of `FaultInjection` is provided in `example/simple`.
In `example/full` all features of `FaultInjection` are shown.

Run all examples with

    $ make example_verilator

This will create the `addFi` Yosys pass, modify the design, build and run the
simulation for each example.

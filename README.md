# Fault Injection Framework Open Source Software (FIFOSS)

Framework to inject faults into netlists.

## Yosys module

The Yosys pass `addFi` changes a design by inserting additional logic to
selected cells and forwards control signals to the top-level module.

### Building

In order to create the Yosys module, install Yosys and then run:
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

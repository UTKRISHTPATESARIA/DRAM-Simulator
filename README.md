# DRAM-Simulator

Generating memory traces for an application using pintool and then simulating on DRAMSim3 simulator on various addressing schemes and Row Buffer policies.

## Running the Tool

Use python version 3.6+ to run python files, also make sure to install the necessary packages required for running the program.

1) Generate the benchmarks executables using the makefile.
2) Generate the inscount*.so files from the inscount*.cpp files that are provided using the make file present in the /path/to/pin/source/tools/ManualExample.
3) Move the benchmarks inside the pin folder and place the inscount*.so files accordingly and run script.sh, which will generate the trace files accordingly.
4) Run the add_cycles.py file by using 2 arguments <path_to_trace_file>, <scaling_factor> which generates the final trace file. (will create new_traces.out, new_traces_1.out, new_traces_2.out).
5) Move the generate traces file into the benchmarks folder that needs to be created inside the DRAMsim folder provided.
6) Move all the config files provided into the configs folder of the DRAMsim folder.
7) Build the DRAMsim using make command provided in the github.
8) Run the script1.sh which will generate the required json files in different benchmark folders.
9) Run the generate_plots.py by making sure its in the same path as the above results folder (Need to edit the python file (line 85 and 86) for each benchmark seperately) to generate the plots and final data that is used in the report.

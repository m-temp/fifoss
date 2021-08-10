#include "kernel/yosys.h"
#include "kernel/sigtools.h"
#include <cstddef>
#include <sys/types.h>

USING_YOSYS_NAMESPACE
PRIVATE_NAMESPACE_BEGIN

struct AddFi : public Pass {
	AddFi() : Pass("addFi", "add fault injection signals") { }

	void help() override
	{
		//   |---v---|---v---|---v---|---v---|---v---|---v---|---v---|---v---|---v---|---v---|
		log("\n");
		log("    addFi [-no-ff] [-no-comb] [-no-add-input] [-type <cell>]");
		log("\n");
		log("Add a fault injection signal to every selected cell and wire the control signal\n");
		log("to the top-level.\n");
		log("\n");
		log("    -no-ff");
		log("       Do not insert fault cells for flip-flops.\n");
		log("\n");
		log("    -no-comb");
		log("       Do not insert fault cells for combinational cells.\n");
		log("\n");
		log("    -no-add-input");
		log("       Do not add the fault signal bus to the top-level input port.\n");
		log("\n");
		log("    -type <cell>");
		log("       Specify the type of the inserted fault control cell.\n");
		log("       Possible values are 'or', 'and' and 'xor' (default).\n");
		log("\n");
	}

	typedef std::vector<std::pair<RTLIL::Module*, RTLIL::Wire*>> connectionStorage;

	void add_toplevel_fi_module(RTLIL::Design* design, connectionStorage *addedInputs, connectionStorage *toplevelSigs, bool add_input_signal)
	{
		log_debug("Connection clean-up: Initial number of added inputs to forward: %zu\n", addedInputs->size());
		connectionStorage work_queue_inputs;
		int j = 0;
		while (!addedInputs->empty())
		{
			work_queue_inputs = *addedInputs;
			log_debug("Connection clean-up: Current number of inputs to forward: %zu\n", work_queue_inputs.size());
			addedInputs->clear();
			for (auto m : work_queue_inputs)
			{
				int i = 0;
				log_debug("Connection clean-up: Searching for instances of module: `%s' with signal `%s'\n", m.first->name.c_str(), log_signal(m.second));
				// Search in all modules, not search for a specific module in the design, but cells of this type.
				for (auto module : design->modules())
				{
					RTLIL::SigSpec fi_cells;
					// And check for all cells as those can be the instances of modules.
					for (auto c : module->cells())
					{
						// Did we find a cell of the correct type?
						if (c->type == m.first->name)
						{
							// New wire for cell to combine the available signals
							int cell_width = m.second->width;
							// TODO The following wire is not really needed later as it is appended to the SigSpec which is then
							// connected as a wire to the input.
							// - The unused wires could be deleted later.
							// - Find another way to connect the cells to the input.
							//   - Do not create wires here, but just iterate and store the info, then create the SigSpec and connect it
							//     on the one end to the input and iterate on the other end for connecting the cells.
							// - Just forward all wires separately (not really nice).
							Wire *s = module->addWire(stringf("\\fi_%s_%d_%s", log_id(c), i++, log_id(m.second->name)), cell_width);
							// Collect all signals from all cells to create a single input later
							fi_cells.append(s);
							log_debug("Connection clean-up: Instance `%s' in `%s' with width %u, connecting wire `%s' to port `%s'\n",
									log_id(c), log_id(c->module), cell_width, log_signal(s), log_signal(m.second));
							c->setPort(m.second->name, s);
						}
					}
					if (fi_cells.size())
					{
						// Create a single signal to all cells
						RTLIL::Wire *mod_in = module->addWire(stringf("\\fi_forward_%s_%d", log_id(module->name), j++), fi_cells.size());
						if (!module->get_bool_attribute(ID::top))
						{
							// Forward wires to top
							mod_in->port_input = 1;
							module->fixup_ports();
						}
						module->connect(fi_cells, mod_in);
						if (!module->get_bool_attribute(ID::top))
						{
							log_debug("Connection clean-up: Adding `%s' to signal forward list\n", log_id(mod_in->name));
							addedInputs->push_back(std::make_pair(module, mod_in));
						}
						else
						{
							log_debug("Connection clean-up: Adding signal `%s' to top-level signal list\n", log_signal(mod_in));
							toplevelSigs->push_back(std::make_pair(module, mod_in));
						}
					}
				}
			}
		}

		// Stop if there are no signals to connect
		if (toplevelSigs->empty()) {
			log_debug("Connection clean-up: No signals at top-level. Sopping.\n");
			return;
		}

		// Connect all signals at the top to a FI module
		RTLIL::Module *top_module;
		for (auto mod : design->modules())
		{
			if (mod->get_bool_attribute(ID::top)) {
				top_module = mod;
			}
		}
		// Stop if no top module can be found
		if (top_module == nullptr) {
			return;
		}

		// TODO Make it possible to update the figenerator module
		// This would allow to run the pass more than once for different parts of the design.
		// This could be useful to run it with different configurations for different parts.
		// In a successive pass the module should be altered to incorporate the new wires.
		log_debug("Connection clean-up: Number of signals for top-level module `%s': %lu\n", top_module->name.c_str(), toplevelSigs->size());
		auto figen = design->addModule("\\figenerator");
		log_debug("Connection clean-up: Create module `%s'\n", figen->name.c_str());
		// Connect a single input to all outputs
		RTLIL::SigSpec passing_signal;
		size_t single_signal_num = 0;
		size_t total_width = 0;
		// Remember the new output port for the fault signal
		std::vector<std::pair<RTLIL::Wire*, RTLIL::Wire*>> fi_port_list;

		// Create output ports
		for (auto &t : *toplevelSigs)
		{
			total_width += t.second->width;
			// Continuous signal number naming
			auto fi_o = figen->addWire(stringf("\\fi_%lu", single_signal_num++), t.second);
			fi_o->port_output = 1;
			passing_signal.append(fi_o);
			fi_port_list.push_back(std::make_pair(fi_o, t.second));
		}
		log_debug("Connection clean-up: Adding combined input port to `%s'\n", figen->name.c_str());
		RTLIL::Wire *fi_combined_in;
		if (add_input_signal) {
			fi_combined_in = figen->addWire("\\fi_combined", total_width);
			fi_combined_in->port_input = 1;
			RTLIL::SigSpec input_port(fi_combined_in);
			figen->connect(passing_signal, input_port);
		}
		figen->fixup_ports();

		std::string figen_instance_name = stringf("\\u_%s", log_id(figen->name));
		auto u_figen = top_module->addCell(figen_instance_name.c_str(), figen->name);

		// Connect output ports
		log_debug("Connection clean-up: Connecting signals to `%s'\n", figen->name.c_str());
		for (auto &l : fi_port_list)
		{
			log_debug("Connection clean-up: Connecting signal `%s' to port `%s'\n", log_id(l.second), log_id(l.first));
			u_figen->setPort(l.first->name, l.second);
		}
		if (add_input_signal) {
			auto top_fi_input = top_module->addWire("\\fi_combined", total_width);
			top_fi_input->port_input = 1;
			u_figen->setPort(fi_combined_in->name, top_fi_input);
			top_module->fixup_ports();
			log_debug("Connection clean-up: Added input signal `%s'\n", top_fi_input->name.c_str());
		}
	}

	Wire *storeFaultSignal(RTLIL::Module *module, RTLIL::Cell *cell, IdString output, int faultNum, RTLIL::SigSpec *fi_signal_module)
	{
		std::string fault_sig_name, sig_type;
		if (output == ID::Q) {
			sig_type = "ff";
		} else if (output == ID::Y) {
			sig_type = "comb";
		}
		if (module->get_bool_attribute(ID::top))
		{
			fault_sig_name = stringf("\\fi_%s_%d", sig_type.c_str(), faultNum);
		}
		else
		{
			fault_sig_name = stringf("\\fi_%s_%s_%d", sig_type.c_str(), log_id(module), faultNum);
		}
		log_debug("Module `%s': Adding wire `%s'\n", module->name.c_str(), fault_sig_name.c_str());
		Wire *s = module->addWire(fault_sig_name, cell->getPort(output).size());
		fi_signal_module->append(s);
		return s;
	}

	void addModuleFiInut(RTLIL::Module *module, RTLIL::SigSpec fi_signal_module, std::string fault_input_name, connectionStorage *moduleInputs, connectionStorage *toplevelSigs)
	{
		if (!fi_signal_module.size()) {
			return;
		}
		Wire *input = module->addWire(fault_input_name, fi_signal_module.size());
		module->connect(fi_signal_module, input);
		if (!module->get_bool_attribute(ID::top)) {
			log_debug("Module `%s': Adding input `%s' (size: %d)\n", module->name.c_str(), input->name.c_str(), input->width);
			input->port_input = true;
			module->fixup_ports();
			log_debug("Module `%s': Adding wire `%s' to signal forward list\n", module->name.c_str(), log_id(input->name));
			moduleInputs->push_back(std::make_pair(module, input));
		}
		else
		{
			log_debug("Module `%s': Adding wire `%s' to top-level signal list\n", module->name.c_str(), log_id(input->name));
			toplevelSigs->push_back(std::make_pair(module, input));
		}
	}

	void appendFiCell(std::string fi_type, RTLIL::Module *module, RTLIL::Cell *cell, RTLIL::IdString output, SigSpec outputSig, Wire *s)
	{
		Wire *xor_input = module->addWire(NEW_ID, cell->getPort(output).size());
		SigMap sigmap(module);
		RTLIL::SigSpec outMapped = sigmap(outputSig);
		outMapped.replace(outputSig, xor_input);
		cell->setPort(output, outMapped);

		// Output of FF, input to XOR
		Wire *newOutput = module->addWire(NEW_ID, cell->getPort(output).size());
		module->connect(outputSig, newOutput);
		// Output of XOR
		// TODO store module with 's' wire input and replace this with the new big wire 'fi_xor' at the end?
		if (fi_type.compare("xor") == 0) {
			module->addXor(NEW_ID, s, xor_input, newOutput);
		} else if (fi_type.compare("and") == 0) {
			module->addAnd(NEW_ID, s, xor_input, newOutput);
		} else if (fi_type.compare("or") == 0) {
			module->addOr(NEW_ID, s, xor_input, newOutput);
		}
	}

	void insertFi(std::string fi_type, RTLIL::Module *module, RTLIL::Cell *cell, int faultNum, RTLIL::SigSpec *fi_signal_module)
	{
		RTLIL::IdString output;
		if (cell->hasPort(ID::Q)) {
				output = ID::Q;
		} else if (cell->hasPort(ID::Y)) {
				output = ID::Y;
		} else {
			return;
		}
		SigSpec sigOutput = cell->getPort(output);
		log_debug("Module `%s': Inserting fault injection '%s' to cell type `%s' (size: %u)\n",
				module->name.c_str(), fi_type.c_str(), log_id(cell->type), sigOutput.size());

		// Wire for FI signal
		Wire *s = storeFaultSignal(module, cell, output, faultNum, fi_signal_module);
		// Cell for FI control
		appendFiCell(fi_type, module, cell, output, sigOutput, s);
	}

	void execute(vector<string> args, RTLIL::Design* design) override
	{
		bool flag_add_fi_input = true;
		bool flag_inject_ff = true;
		bool flag_inject_combinational = true;
		std::string option_fi_type;

		// parse options
		size_t argidx;
		for (argidx = 1; argidx < args.size(); argidx++)
		{
			std::string arg = args[argidx];
			if (arg == "-no-ff") {
				flag_inject_ff = false;
				continue;
			}
			if (arg == "-no-comb") {
				flag_inject_combinational = false;
				continue;
			}
			if (arg == "-no-add-input") {
				flag_add_fi_input = false;
				continue;
			}
			if (arg == "-type") {
				if (++argidx >= args.size())
					log_cmd_error("Option -type requires an additional argument!\n");
				option_fi_type = args[argidx];
				continue;
			}
			// TODO do not create the figenerator module
			// Add a argument to prevent the creation of the module.
			// Two possible ways to handle the signals:
			//   - Keep them unconnected at the top-level (-no-add-input)
			//   - Create the forwarding bus and add it as an input port
			// The reason to having this module is an easy access to the bus
			// for the simulation. One could apply a patch to the HDL part for
			// some DPI functions which can then be used by the simulation.
			break;
		}
		extra_args(args, argidx, design);

		connectionStorage addedInputs, toplevelSigs;

		if (option_fi_type.empty())
		{
			option_fi_type = "xor";
		}

		for (auto module : design->selected_modules())
		{
			log("Updating module `%s'\n", module->name.c_str());
			int i = 0;
			RTLIL::SigSpec fi_ff, fi_comb;
			// Add a FI cell for each cell in the module
			log_debug("Module `%s': Searching for cells to append with fault injection\n", module->name.c_str());
			for (auto cell : module->selected_cells())
			{
				// Only operate on standard cells (do not change modules)
				if (!cell->type.isPublic()) {
					if ((flag_inject_ff && cell->type.in(RTLIL::builtin_ff_cell_types()))) {
							insertFi(option_fi_type, module, cell, i++, &fi_ff);
					}
					if (flag_inject_combinational && !cell->type.in(RTLIL::builtin_ff_cell_types())) {
							insertFi(option_fi_type, module, cell, i++, &fi_comb);
					}
				}
			}
			// Update the module with a port to control all new XOR cells
			log_debug("Module `%s': Updating modules inputs\n", module->name.c_str());
			addModuleFiInut(module, fi_ff, "\\fi_ff", &addedInputs, &toplevelSigs);
			addModuleFiInut(module, fi_comb, "\\fi_comb", &addedInputs, &toplevelSigs);
		}
		// Update all modified modules in the design and add wiring to the top
		add_toplevel_fi_module(design, &addedInputs, &toplevelSigs, flag_add_fi_input);
	}
} AddFi;

PRIVATE_NAMESPACE_END

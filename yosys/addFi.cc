#include "kernel/yosys.h"
#include "kernel/sigtools.h"
#include <cstddef>
#include <sys/types.h>

USING_YOSYS_NAMESPACE
PRIVATE_NAMESPACE_BEGIN

struct AddFi : public Pass {
	AddFi() : Pass("AddFi") { }

	typedef std::vector<std::pair<RTLIL::Module*, RTLIL::Wire*>> connectionStorage;

	void add_toplevel_fi_module(RTLIL::Design* design, connectionStorage *addedInputs, connectionStorage *toplevelSigs, bool add_input_signal)
	{
		log_debug("Updating all modified modules with new fault injection wiring: %zu\n", addedInputs->size());
		connectionStorage work_queue_inputs;
		int j = 0;
		while (!addedInputs->empty())
		{
			work_queue_inputs = *addedInputs;
			log_debug("Number of modules to update: %zu\n", work_queue_inputs.size());
			addedInputs->clear();
			for (auto m : work_queue_inputs)
			{
				int i = 0;
				log_debug("Searching for instances of module: '%s' with signal '%s'\n", log_id(m.first), log_signal(m.second));
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
							Wire *s = module->addWire(stringf("\\fault_%s_%d_%s", log_id(c), i++, log_id(m.second->name)), cell_width);
							fi_cells.append(s);
							log_debug("Instance '%s' in '%s' with width '%u', connecting wire '%s' to port '%s'\n",
									log_id(c), log_id(c->module), cell_width, log_id(s->name), log_id(m.second->name));
							c->setPort(m.second->name, s);
						}
					}
					if (fi_cells.size())
					{
						RTLIL::Wire *mod_in = module->addWire(stringf("\\fi_forward_%d", j++), fi_cells.size());
						if (!module->get_bool_attribute(ID::top))
						{
							// Forward wires to top
							mod_in->port_input = 1;
							module->fixup_ports();
						}
						module->connect(fi_cells, mod_in);
						if (!module->get_bool_attribute(ID::top))
						{
							log_debug("Adding signal to forward list as this is not yet at the top: %s\n", log_id(mod_in->name));
							addedInputs->push_back(std::make_pair(module, mod_in));
						}
						else
						{
							log_debug("New input at top level: %s\n", log_signal(mod_in));
							toplevelSigs->push_back(std::make_pair(module, mod_in));
						}
					}
				}
			}
		}

		// Stop if there are no signals to connect
		if (toplevelSigs->empty()) {
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

		log_debug("Number of fault injection signals: %lu for top module '%s'\n", toplevelSigs->size(), log_id(top_module));
		auto figen = design->addModule("\\figenerator");
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
			// Continous signal number naming
			auto fi_o = figen->addWire(stringf("\\fi_%lu", single_signal_num++), t.second);
			fi_o->port_output = 1;
			passing_signal.append(fi_o);
			fi_port_list.push_back(std::make_pair(fi_o, t.second));
		}
		log_debug("Adding combined input port to figenerator\n");
		RTLIL::Wire *fi_combined_in;
		if (add_input_signal) {
			fi_combined_in = figen->addWire("\\fi_combined", total_width);
			fi_combined_in->port_input = 1;
			RTLIL::SigSpec input_port(fi_combined_in);
			figen->connect(passing_signal, input_port);
		}
		figen->fixup_ports();

		auto u_figen = top_module->addCell("\\u_figenerator", "\\figenerator");

		// Connect output ports
		for (auto &l : fi_port_list)
		{
			log_debug("Connecting signal '%s' to port '%s'\n", log_id(l.second), log_id(l.first));
			u_figen->setPort(l.first->name, l.second);
		}
		if (add_input_signal) {
			auto top_fi_input = top_module->addWire("\\fi_combined", total_width);
			top_fi_input->port_input = 1;
			u_figen->setPort(fi_combined_in->name, top_fi_input);
			top_module->fixup_ports();
		}
	}

	Wire *storeFaultSignal(RTLIL::Module *module, RTLIL::Cell *cell, IdString output, int faultNum, RTLIL::SigSpec *fi_mod)
	{
		std::string fault_sig_name;
		if (module->get_bool_attribute(ID::top))
		{
			fault_sig_name = stringf("\\fi_%d", faultNum);
		}
		else
		{
			fault_sig_name = stringf("\\fi_%s_%d", log_id(module), faultNum);
		}
		Wire *s = module->addWire(fault_sig_name, cell->getPort(output).size());
		fi_mod->append(s);
		return s;
	}

	void addModuleFiInut(RTLIL::Module *module, RTLIL::SigSpec fi_mod, connectionStorage *moduleInputs, connectionStorage *toplevelSigs)
	{
		if (!fi_mod.size()) {
			return;
		}
		Wire *input = module->addWire("\\fi_xor", fi_mod.size());
		module->connect(fi_mod, input);
		if (!module->get_bool_attribute(ID::top)) {
			input->port_input = true;
			module->fixup_ports();
		}
		log_debug("Creating module local FI input %s with size %d\n", log_id(input->name), input->width);
		if (module->get_bool_attribute(ID::top))
		{
			log_debug("Adding to top level '%s'\n", log_id(module));
			toplevelSigs->push_back(std::make_pair(module, input));
		}
		else
		{
			log_debug("Adding to model input %s\n", log_id(input->name));
			moduleInputs->push_back(std::make_pair(module, input));
		}
	}

	void addFiXor(RTLIL::Module *module, RTLIL::Cell *cell, RTLIL::IdString output, SigSpec outputSig, Wire *s)
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
		module->addXor(NEW_ID, s, xor_input, newOutput);
	}

	void insertXor(RTLIL::Module *module, RTLIL::Cell *cell, int faultNum, RTLIL::SigSpec *fi_mod)
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
		log_debug("Insertig fault injection XOR to cell of type '%s' with size '%u'\n", log_id(cell->type), sigOutput.size());

		Wire *s = storeFaultSignal(module, cell, output, faultNum, fi_mod);
		addFiXor(module, cell, output, sigOutput, s);
	}

	void execute(vector<string> args, RTLIL::Design* design) override
	{
		bool flag_add_fi_input = true;
		bool flag_inject_ff = true;
		bool flag_inject_logic = true;

		// parse options
		size_t argidx;
		for (argidx = 1; argidx < args.size(); argidx++)
		{
			std::string arg = args[argidx];
			if (arg == "-noff") {
				flag_inject_ff = false;
				continue;
			}
			if (arg == "-nologic") {
				flag_inject_logic = false;
				continue;
			}
			if (arg == "-nofiinput") {
				flag_add_fi_input = false;
				continue;
			}
			break;
		}
		extra_args(args, argidx, design);

		connectionStorage addedInputs, toplevelSigs;

		for (auto module : design->selected_modules())
		{
			log("Updating module %s\n", log_id(module));
			int i = 0;
			RTLIL::SigSpec fi_mod;
			// Add a Xor cell for each cell in the module
			for (auto cell : module->selected_cells())
			{
				// Only operate on standard cells (do not change modules)
				if (!cell->type.isPublic()) {
					if ((flag_inject_ff && cell->type.in(RTLIL::builtin_ff_cell_types())) ||
						(flag_inject_logic && !cell->type.in(RTLIL::builtin_ff_cell_types()))) {
							insertXor(module, cell, i++, &fi_mod);
					}
				}
			}
			// Update the module with a port to control all new XOR cells
			addModuleFiInut(module, fi_mod, &addedInputs, &toplevelSigs);
		}
		// Update all modified modules in the design and add wiring to the top
		add_toplevel_fi_module(design, &addedInputs, &toplevelSigs, flag_add_fi_input);
	}
} AddFi;

PRIVATE_NAMESPACE_END

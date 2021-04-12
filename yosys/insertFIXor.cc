#include "kernel/yosys.h"
#include "kernel/sigtools.h"
#include <cstddef>
#include <sys/types.h>

USING_YOSYS_NAMESPACE
PRIVATE_NAMESPACE_BEGIN

struct InsertFIXor : public Pass {
	InsertFIXor() : Pass("insertFIXor") { }

	typedef std::vector<std::pair<RTLIL::Module*, RTLIL::Wire*>> connectionStorage;

	void add_toplevel_fi_module(RTLIL::Design* design, connectionStorage *addedInputs, connectionStorage *toplevelSigs, bool add_input_signal)
	{
		log_debug("Connecting fault signals to new toplevel module\n");
		connectionStorage work_queue_inputs;
		while (!addedInputs->empty())
		{
			work_queue_inputs = *addedInputs;
			log_debug("Length of input queue: %zu\n", work_queue_inputs.size());
			addedInputs->clear();
			for (auto m : work_queue_inputs)
			{
				int i = 0;
				log_debug("Checking for input '%s' of module: '%s'\n", log_signal(m.second), log_id(m.first));
				for (auto module : design->modules())
				{
					for (auto c : module->cells())
					{
						if (c->type == m.first->name)
						{
							int cell_width = m.second->width;
							log_debug("Instance '%s' in '%s' with width '%u'\n", log_id(c), log_id(c->module), cell_width);
							Wire *s = module->addWire(stringf("\\fault_%s_%d_%s", log_id(c), i++, log_id(m.second->name)), cell_width);
							if (module != design->top_module())
							{
								// Forward wires to top
								s->port_input = 1;
								module->fixup_ports();
							}
							log_debug("Connecting wire '%s' to port '%s'\n", log_id(s->name), log_id(m.second->name));
							c->setPort(m.second->name, s);
							if (module != design->top_module())
							{
								log_debug("Adding signal to forward list as this is not yet at the top\n");
								addedInputs->push_back(std::make_pair(module, s));
							}
							else
							{
								log_debug("Storing top signals in list\n");
								toplevelSigs->push_back(std::make_pair(module, s));
							}
						}
					}
				}
			}
		}

		// Connect all signals at the top to a FI module
		auto top_module = design->top_module();
		log_debug("Number of fault injection signals: %lu\n", toplevelSigs->size());
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

	Wire *addFaultSignal(RTLIL::Module *module, RTLIL::Cell *cell, IdString output, int faultNum, connectionStorage *moduleInputs, connectionStorage *toplevelSigs)
	{
		std::string fault_sig_name;
		int input;
		if (module == module->design->top_module())
		{
			fault_sig_name = stringf("\\fi_%d", faultNum);
			input = 0;
		}
		else
		{
			fault_sig_name = stringf("\\fi_%s_%d", log_id(module), faultNum);
			input = 1;
		}
		Wire *s = module->addWire(fault_sig_name, cell->getPort(output).size());
		s->port_input = input;
		module->fixup_ports();
		log_debug("Cell '%s': adding input '%s' for the module '%s'\n", log_id(cell), log_id(s->name), log_id(cell->module->name));
		if (module == module->design->top_module())
		{
			toplevelSigs->push_back(std::make_pair(module, s));
		}
		else
		{
			moduleInputs->push_back(std::make_pair(module, s));
		}
		return s;
	}

	void addFiXor(RTLIL::Module *module, RTLIL::Cell *cell, RTLIL::IdString output, SigSpec outputSig, Wire *s)
	{
		Wire *xor_input = module->addWire(NEW_ID, cell->getPort(output).size());
		SigMap sigmap(module);
		RTLIL::SigSpec outMapped = sigmap(outputSig);
		log_debug("Mapped signal x: %s\n", log_signal(outMapped));
		outMapped.replace(outputSig, xor_input);
		cell->setPort(output, outMapped);

		// Output of FF, input to XOR
		Wire *newOutput = module->addWire(NEW_ID, cell->getPort(output).size());
		module->connect(outputSig, newOutput);
		// Output of XOR
		module->addXor(NEW_ID, s, xor_input, newOutput);
		log_debug("Mapped signal x: %s\n", log_signal(outMapped));
	}

	void insertXor(RTLIL::Module *module, RTLIL::Cell *cell, RTLIL::IdString output, int faultNum, connectionStorage *addedInputs, connectionStorage *toplevelSigs)
	{
		SigSpec sigOutput = cell->getPort(output);
		log_debug("Modifying cell of type '%s' with size '%u'\n", log_id(cell->type), sigOutput.size());

		Wire *s = addFaultSignal(module, cell, output, faultNum, addedInputs, toplevelSigs);
		addFiXor(module, cell, output, sigOutput, s);
	}

	void insertXorFF(RTLIL::Module *module, RTLIL::Cell *cell, int faultNum, connectionStorage *addedInputs, connectionStorage *toplevelSigs)
	{
		insertXor(module, cell, ID::Q, faultNum, addedInputs, toplevelSigs);
	}

	void insertXorLogic(RTLIL::Module *module, RTLIL::Cell *cell, int faultNum, connectionStorage *addedInputs, connectionStorage *toplevelSigs)
	{
		insertXor(module, cell, ID::Y, faultNum, addedInputs, toplevelSigs);
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

		connectionStorage addedInputs, toplevelSigs;

		for (auto module : design->selected_modules())
		{
			int i = 0;
			for (auto cell : module->selected_cells())
			{
				if (flag_inject_ff && cell->type.in(ID($adff))) {
					insertXorFF(module, cell, i++, &addedInputs, &toplevelSigs);
				}
				if (flag_inject_logic && cell->type.in(ID($and), ID($or), ID($xor), ID($xnor))) {
					insertXorLogic(module, cell, i++, &addedInputs, &toplevelSigs);
				}
			}
			module->fixup_ports();
		}
		add_toplevel_fi_module(design, &addedInputs, &toplevelSigs, flag_add_fi_input);
	}
} InsertFIXor;

PRIVATE_NAMESPACE_END

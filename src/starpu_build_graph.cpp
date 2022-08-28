#include <starpu_build_graph.hpp>
#include <tfheppwrapper.hpp>

namespace Tangor{

// Vectors which holds ciphertexts
std::vector<TFHEpp::TLWE<P>> cipherin;
std::vector<TFHEpp::TLWE<P>> cipherout;
std::vector<TFHEpp::TLWE<P>> cipherdffq;
std::vector<TFHEpp::TLWE<P>> cipherdffd;
std::vector<TFHEpp::TLWE<P>> cipherwire;

//Handlers for above vectors
std::vector<starpu_data_handle_t> cipherin_handle;
std::vector<starpu_data_handle_t> cipherout_handle;
std::vector<starpu_data_handle_t> cipherdffq_handle;
std::vector<starpu_data_handle_t> cipherdffd_handle;
std::vector<starpu_data_handle_t> cipherwire_handle;

starpu_data_handle_t search_pointer(
    const uint id, const YosysJSONparser::ParsedBC& BCnetlist,
    const std::string bitname)
{
    std::vector<starpu_data_handle_t>::iterator res;
    const std::vector<uint>::const_iterator outiterator = std::find(
        BCnetlist.output_vector.begin(), BCnetlist.output_vector.end(), id);
    const std::vector<uint>::const_iterator initerator = std::find(
        BCnetlist.input_vector.begin(), BCnetlist.input_vector.end(), id);
    const std::vector<uint>::const_iterator dffditerator = std::find(
        BCnetlist.DFF_Q_vector.begin(), BCnetlist.DFF_D_vector.end(), id);
    const std::vector<uint>::const_iterator dffqiterator = std::find(
        BCnetlist.DFF_Q_vector.begin(), BCnetlist.DFF_Q_vector.end(), id);
    const std::vector<uint>::const_iterator wireiterator = std::find(
        BCnetlist.wire_vector.begin(), BCnetlist.wire_vector.end(), id);
    if (initerator != BCnetlist.input_vector.end())
        res = cipherin_handle.begin() +
              std::distance(BCnetlist.input_vector.begin(), initerator);
	else if (dffditerator != BCnetlist.DFF_D_vector.end())
        res = cipherdffd_handle.begin() +
              std::distance(BCnetlist.DFF_D_vector.begin(), dffditerator);
    else if (dffqiterator != BCnetlist.DFF_Q_vector.end())
        res = cipherdffq_handle.begin() +
              std::distance(BCnetlist.DFF_Q_vector.begin(), dffqiterator);
    else if (wireiterator != BCnetlist.wire_vector.end())
        res = cipherwire_handle.begin() +
              std::distance(BCnetlist.wire_vector.begin(), wireiterator);
    else if (outiterator != BCnetlist.output_vector.end())
        res = cipherout_handle.begin() +
              std::distance(BCnetlist.output_vector.begin(), outiterator);
    else
        std::cout << bitname << " bit parse error" << std::endl;
    return *res;
}

void starpu_build_graph(const YosysJSONparser::ParsedBC& BCnetlist, const TFHEpp::EvalKey& ek){
// Resize vectors which holds ciphertexts
cipherin.resize(
	BCnetlist.input_vector.size());
cipherin_handle.resize(
	BCnetlist.input_vector.size());
cipherout.resize(
	BCnetlist.output_vector.size());
cipherout_handle.resize(
	BCnetlist.output_vector.size());
cipherdffq.resize(
	BCnetlist.DFF_Q_vector.size());
cipherdffq_handle.resize(
	BCnetlist.DFF_Q_vector.size());
cipherdffd.resize(
	BCnetlist.DFF_D_vector.size());
cipherdffd_handle.resize(
	BCnetlist.DFF_D_vector.size());
cipherwire.resize(
	BCnetlist.wire_vector.size());
cipherwire_handle.resize(
	BCnetlist.wire_vector.size());

//Register vectors to StarPU
for(int i = 0; i < cipherin.size();i++)
	starpu_vector_data_register(&(cipherin_handle[i]),STARPU_MAIN_RAM,reinterpret_cast<uintptr_t>(cipherin[i].data()),P::k*P::n+1,sizeof(typename P::T));
for(int i = 0; i < cipherout.size();i++)
	starpu_vector_data_register(&(cipherout_handle[i]),STARPU_MAIN_RAM,reinterpret_cast<uintptr_t>(cipherout[i].data()),P::k*P::n+1,sizeof(typename P::T));
for(int i = 0; i < cipherdffq.size();i++)
	starpu_vector_data_register(&(cipherdffq_handle[i]),STARPU_MAIN_RAM,reinterpret_cast<uintptr_t>(cipherdffq[i].data()),P::k*P::n+1,sizeof(typename P::T));
for(int i = 0; i < cipherdffd.size();i++)
	starpu_vector_data_register(&(cipherdffd_handle[i]),STARPU_MAIN_RAM,reinterpret_cast<uintptr_t>(cipherdffd[i].data()),P::k*P::n+1,sizeof(typename P::T));
for(int i = 0; i < cipherwire.size();i++)
	starpu_vector_data_register(&(cipherwire_handle[i]),STARPU_MAIN_RAM,reinterpret_cast<uintptr_t>(cipherwire[i].data()),P::k*P::n+1,sizeof(typename P::T));

// Initialize registers by 0
for (TFHEpp::TLWE<P> &dffq : cipherdffq)
	TFHEpp::HomCONSTANTZERO(dffq);

// Submit tasks to StarPU
for (int gate_index = 1; gate_index <= BCnetlist.gate_vector.size();
		gate_index++) {
	const YosysJSONparser::GateStruct &gate =
		BCnetlist.gate_vector[gate_index - 1];
	const std::set<std::string> in2out1gates{
		"NAND", "NOR", "XNOR", "AND", "OR", "XOR", "ANDYN", "ORYN"};
	#ifdef USE_M12
	const std::set<std::string> in3out1gates{
		"MUX","NMUX","AOI3","OAI3"};
	#else
	const std::set<std::string> in3out1gates{
		"MUX","NMUX"};
	#endif
	const std::set<std::string> in2out2gates{
		"ha"};
	const std::set<std::string> in3out2gates{
		"fa"};
	if (gate.name == "NOT") {
		// 1 input gate
		const starpu_data_handle_t outcipher =
			search_pointer(gate.out[0], BCnetlist, "output");
		const starpu_data_handle_t
			inacipher =
				search_pointer(gate.in[0], BCnetlist, "in0");
		starpu_task_insert(&clHomNOT, STARPU_W, outcipher, STARPU_R, inacipher, 0);
	}
	else if (in2out1gates.find(gate.name) != in2out1gates.end()) {
		const starpu_data_handle_t outcipher =
			search_pointer(gate.out[0], BCnetlist, "output");
		const starpu_data_handle_t
			inacipher =
				search_pointer(gate.in[0], BCnetlist, "in0");
		const starpu_data_handle_t
			inbcipher =
				search_pointer(gate.in[1], BCnetlist, "in1");
		// 2 input gates
		if (gate.name == "NAND")
			starpu_task_insert(&clHomNAND, STARPU_W, outcipher, STARPU_R, inacipher, STARPU_R, inbcipher, STARPU_VALUE, &ek, sizeof(ek), 0);
		else if (gate.name == "NOR")
			starpu_task_insert(&clHomNOR, STARPU_W, outcipher, STARPU_R, inacipher, STARPU_R, inbcipher, STARPU_VALUE, &ek, sizeof(ek), 0);
		else if (gate.name == "XNOR")
			starpu_task_insert(&clHomXNOR, STARPU_W, outcipher, STARPU_R, inacipher, STARPU_R, inbcipher, STARPU_VALUE, &ek, sizeof(ek), 0);
		else if (gate.name == "AND")
			starpu_task_insert(&clHomAND, STARPU_W, outcipher, STARPU_R, inacipher, STARPU_R, inbcipher, STARPU_VALUE, &ek, sizeof(ek), 0);
		else if (gate.name == "OR")
			starpu_task_insert(&clHomOR, STARPU_W, outcipher, STARPU_R, inacipher, STARPU_R, inbcipher, STARPU_VALUE, &ek, sizeof(ek), 0);
		else if (gate.name == "XOR")
			starpu_task_insert(&clHomXOR, STARPU_W, outcipher, STARPU_R, inacipher, STARPU_R, inbcipher, STARPU_VALUE, &ek, sizeof(ek), 0);
		else if (gate.name == "ANDYN")
			starpu_task_insert(&clHomANDYN, STARPU_W, outcipher, STARPU_R, inacipher, STARPU_R, inbcipher, STARPU_VALUE, &ek, sizeof(ek), 0);
		else if (gate.name == "ORYN")
			starpu_task_insert(&clHomORYN, STARPU_W, outcipher, STARPU_R, inacipher, STARPU_R, inbcipher, STARPU_VALUE, &ek, sizeof(ek), 0);
		else {
			std::cout << "GATE PARSE ERROR" << std::endl;
			std::cout << gate.name << std::endl;
		}
	}
	else if(in3out1gates.find(gate.name) != in3out1gates.end()){
		// 3 input gate, MUX or NMUX
		const starpu_data_handle_t outcipher =
			search_pointer(gate.out[0], BCnetlist, "output");
		const starpu_data_handle_t
			inacipher =
				search_pointer(gate.in[0], BCnetlist, "in0");
		const starpu_data_handle_t
			inbcipher =
				search_pointer(gate.in[1], BCnetlist, "in1");
		const starpu_data_handle_t 
			inscipher =
				search_pointer(gate.in[2], BCnetlist, "in2");
		if (gate.name == "MUX")
			starpu_task_insert(&clHomMUX, STARPU_W, outcipher, STARPU_R, inscipher, STARPU_R, inbcipher, STARPU_R, inacipher, STARPU_VALUE, &ek, sizeof(ek), 0);
		else if(gate.name == "NMUX")
			starpu_task_insert(&clHomNMUX, STARPU_W, outcipher, STARPU_R, inscipher, STARPU_R, inbcipher, STARPU_R, inacipher, STARPU_VALUE, &ek, sizeof(ek), 0);
		// #ifdef USE_M12
		// else if(gate.name == "AOI3")
		// 	gatetasknet[gate_index - 1] = taskflow.emplace([=, &ek]() {
		// 		TFHEpp::HomAOI3<P>(*outcipher, *inacipher, *inbcipher,
		// 						*inscipher, ek);
		// 	});
		// 	else if(gate.name == "OAI3")
		// 	gatetasknet[gate_index - 1] = taskflow.emplace([=, &ek]() {
		// 		TFHEpp::HomOAI3<P>(*outcipher, *inacipher, *inbcipher,
		// 						*inscipher, ek);
		// 	});
		// #endif
	}
	// #ifdef USE_M12
	// else if(in2out2gates.find(gate.name) != in2out2gates.end()){
	// 	const starpu_data_handle_t outcipher0 =
	// 		search_pointer(gate.out[0], BCnetlist, "output0");
	// 	const starpu_data_handle_t outcipher1 =
	// 		search_pointer(gate.out[1], BCnetlist, "output1");
	// 	const starpu_data_handle_t
	// 		inacipher =
	// 			search_pointer(gate.in[0], BCnetlist, "in0");
	// 	const starpu_data_handle_t
	// 		inbcipher =
	// 			search_pointer(gate.in[1], BCnetlist, "in1");
	// 	starpu_task_insert(&clHomHalfAdder, STARPU_W, outcipher1,  STARPU_W, outcipher0, STARPU_R, inscipher, STARPU_R, inbcipher, STARPU_R, inacipher, STARPU_VALUE, &ek, sizeof(ek));
	// }
	// else if(in3out2gates.find(gate.name) != in3out2gates.end()){
	// 	const starpu_data_handle_t outcipher0 =
	// 		search_pointer(gate.out[0], BCnetlist, cipherdffd, cipherwire,
	// 						cipherout, "output0");
	// 	const starpu_data_handle_t outcipher1 =
	// 		search_pointer(gate.out[1], BCnetlist, "output1");
	// 	const starpu_data_handle_t
	// 		inacipher =
	// 			search_pointer(gate.in[0], BCnetlist, "in0");
	// 	const starpu_data_handle_t
	// 		inbcipher =
	// 			search_pointer(gate.in[1], BCnetlist, "in1");
	// 	const starpu_data_handle_t
	// 		inccipher =
	// 			search_pointer(gate.in[2], BCnetlist, "in2");
	// 	#ifdef USE_M12
	// 	gatetasknet[gate_index - 1] = taskflow.emplace([=, &ek]() {
	// 		TFHEpp::HomFullAdder(*outcipher1, *outcipher0, *inacipher, *inbcipher,
	// 						*inccipher, ek);
	// 	}).name("FA");
	// 	#else
	// 	gatetasknet[gate_index - 1] = taskflow.emplace([=, &ek]() {
	// 		TFHEpp::Hom2BRFullAdder(*outcipher1, *outcipher0, *inacipher, *inbcipher,
	// 						*inccipher, ek);
	// 	}).name("FA");
	// 	#endif
	// }
	// #endif
	else{
		std::cout <<"GATE PARSE ERROR!"<<std::endl;
	}
}
}
}
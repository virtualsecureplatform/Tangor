#pragma once
#include "tangorparam.hpp"
#include "yosys-json-parser.hpp"
#include <starpu.h>

namespace Tangor{
// Vectors which holds ciphertexts
extern std::vector<TFHEpp::TLWE<P>> cipherin;
extern std::vector<TFHEpp::TLWE<P>> cipherout;
extern std::vector<TFHEpp::TLWE<P>> cipherdffq;
extern std::vector<TFHEpp::TLWE<P>> cipherdffd;
extern std::vector<TFHEpp::TLWE<P>> cipherwire;

//Handlers for above vectors
extern std::vector<starpu_data_handle_t> cipherin_handle;
extern std::vector<starpu_data_handle_t> cipherout_handle;
extern std::vector<starpu_data_handle_t> cipherdffq_handle;
extern std::vector<starpu_data_handle_t> cipherdffd_handle;
extern std::vector<starpu_data_handle_t> cipherwire_handle;

void starpu_build_graph(const YosysJSONparser::ParsedBC& BCnetlist, const TFHEpp::EvalKey& ek);
}

#pragma once
#include <algorithm>
#include <array>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

// Parse JSON netlist
namespace YosysJSONparser {
// Assuming there is a limit of a number of inputs for each gate.
constexpr uint MAX_NUM_INPUT = 3;
constexpr uint MAX_NUM_OUTPUT = 3;

// Holding information of each gate
struct GateStruct {
    std::string name;
    std::array<uint, MAX_NUM_INPUT> in;
    std::array<uint, MAX_NUM_OUTPUT> out;
};

struct ParsedBC {
    std::vector<uint> input_vector;
    std::vector<uint> output_vector;
    std::vector<uint> DFF_Q_vector;
    std::vector<uint> DFF_D_vector;
    std::vector<uint> wire_vector;
    std::vector<GateStruct> gate_vector;
    std::vector<std::array<uint, 2>> direct_port_pair_vector;
    // Because input ports, output ports and wires have unique index, we can
    // hold the information about which gate output to specific wires or output
    // ports by this vector.
    std::vector<uint> dependency_vector;
    ParsedBC(const std::string &);
};

}  // namespace YosysJSONparser
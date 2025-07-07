#pragma once

#include <string>
#include <vector>
#include <unordered_map>

// Call solver.py with a single equation and return raw output
std::string solve_eq(const std::string& equation);

// Parse a Python dictionary string (e.g., "{'x0': 2, 'x1': 3}")
std::unordered_map<std::string, size_t> parse_dictionary(const std::string& dict_str);

// Parse solver.py output into a vector of variable assignment maps
std::vector<std::unordered_map<std::string, size_t>> parse_solver_output(const std::string& solver_output);

// Optional: print solutions nicely
void print_parsed_solutions(const std::vector<std::unordered_map<std::string, size_t>>& solutions);


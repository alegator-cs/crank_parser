#include "solver_interface.hpp"
#include <sstream>
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <algorithm>
#include <charconv>

std::string solve_eq(const std::string& equation) {
    std::string command = "python solver.py \"" + equation + "\"";
    std::cout << "Command: " << command << "\n";
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) return "";
    char buffer[256];
    std::ostringstream output;
    while (fgets(buffer, sizeof(buffer), pipe)) {
        output << buffer;
    }
    pclose(pipe);
    return output.str();
}

std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\n\r");
    size_t end = s.find_last_not_of(" \t\n\r");
    return (start == std::string::npos || end == std::string::npos) ? "" : s.substr(start, end - start + 1);
}

std::string remove_quotes(const std::string& s) {
    if (s.size() >= 2 && s.front() == '\'' && s.back() == '\'') return s.substr(1, s.size() - 2);
    return s;
}

std::unordered_map<std::string, size_t> parse_dictionary(const std::string& dict_str) {
    std::unordered_map<std::string, size_t> result;
    size_t start = dict_str.find('{');
    size_t end = dict_str.find('}');
    if (start == std::string::npos || end == std::string::npos || end <= start) return result;

    std::string content = dict_str.substr(start + 1, end - start - 1);
    std::istringstream iss(content);
    std::string token;

    while (std::getline(iss, token, ',')) {
        size_t colon = token.find(':');
        if (colon == std::string::npos) continue;

        std::string raw_key = remove_quotes(trim(token.substr(0, colon)));
        std::string val_str = trim(token.substr(colon + 1));
        size_t val = 0;
        std::from_chars(val_str.data(), val_str.data() + val_str.size(), val);

        // Strip anything after ':' in the key
        size_t spec_pos = raw_key.find(':');
        std::string base_key = (spec_pos != std::string::npos) ? raw_key.substr(0, spec_pos) : raw_key;

        result[base_key] = val;
    }

    return result;
}

std::vector<std::unordered_map<std::string, size_t>> parse_solver_output(const std::string& solver_output) {
    std::vector<std::unordered_map<std::string, size_t>> solutions;
    std::istringstream iss(solver_output);
    std::string line;
    bool in_solution_section = false;
    while (std::getline(iss, line)) {
        line = trim(line);
        if (!in_solution_section) {
            if (line.find("Filtered solutions passing binary constraints:") != std::string::npos) {
                in_solution_section = true;
            }
            continue;
        }
        if (!line.empty() && line[0] == '{') {
            auto dict = parse_dictionary(line);
            if (!dict.empty()) solutions.push_back(std::move(dict));
        }
    }
    return solutions;
}

void print_parsed_solutions(const std::vector<std::unordered_map<std::string, size_t>>& solutions) {
    std::cout << "Parsed candidate solutions:\n";
    for (const auto& sol : solutions) {
        std::cout << "{ ";
        for (const auto& [key, val] : sol) {
            std::cout << key << "=" << val << " ";
        }
        std::cout << "}\n";
    }
}


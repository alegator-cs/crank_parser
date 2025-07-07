#pragma once

#include "core.hpp"
#include <string>
#include <vector>

// Fragment gen
void gen_frags(Expr& expr, size_t& xvar_count, size_t& bvar_count);
std::string gen_eqs(const Expr& expr, size_t input_size);

// Fragment manipulation
void scalar_mult_frag(std::string& frag, std::string& c);
void scalar_mult_eqs(std::vector<Equation>& eqs, std::string& c);

// Utility
std::string n_m_to_xvar(size_t n, size_t m, size_t xvar_count);

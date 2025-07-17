#pragma once

#include "core.hpp"
#include <string_view>
#include <unordered_map>

bool match_leaf(std::string_view leaf, std::string_view input, size_t pos);
bool match(Expr& expr, const Equation& eq, const std::unordered_map<std::string, size_t>& sol, std::string_view input, size_t pos);

void get_leaves(Expr& expr, std::vector<Expr*>& leaves);
void set_depths(Expr* node, size_t current_depth = 0);
void optimize_parse_tree(Expr& expr, std::string_view input);
void propagate_inactives(Expr& expr);
void match_up(std::vector<std::tuple<Expr*, size_t, std::string>> exprs, std::vector<Expr*>& groups, const size_t N, const std::string_view input, std::vector<size_t> match, std::vector<std::vector<size_t>>& matches);
void match_down(std::vector<Expr*> exprs, std::vector<Expr*>& groups, const size_t N, const std::string_view input, std::vector<std::vector<size_t>>& matches);
void get_groups(Expr& expr, std::vector<Expr*>& groups);

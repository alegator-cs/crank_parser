#pragma once

#include "core.hpp"
#include <string_view>
#include <memory>

// Parsing entry point
std::unique_ptr<Expr> parse(std::string_view input);
std::unique_ptr<Expr> parse(std::string_view input, size_t& ref_id);

// Group scanning and classification
GroupType check_group(std::string_view input);
bool is_valid_group(GroupType group_type);
bool is_wrapped(GroupType type);
bool is_bracketed(GroupType type);

// Group scanners
std::string_view scan_group(std::string_view input, GroupType& type, size_t ref_id);
std::string_view scan_group_implicit(std::string_view input);
std::string_view scan_group_wrapped(std::string_view input, GroupType& type);
std::string_view unwrap_group(std::string_view input);
std::string_view scan_ref(std::string_view input, GroupType& type, size_t ref_id);

// Bracket handling
std::string_view scan_bracket(std::string_view input, GroupType& type);
std::string_view scan_bracket_inner(std::string_view input, GroupType& type);
bool is_valid_bracket_class(std::string_view input);
bool is_valid_bracket_collation(std::string_view input);
bool is_valid_bracket_equivalence(std::string_view input);
std::string_view scan_bracket_char(std::string_view input, GroupType& type);
char eval_bracket_char(std::string_view input);

// Misc scanning
std::string_view scan_number(std::string_view input);


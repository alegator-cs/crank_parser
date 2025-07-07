#pragma once

#include "core.hpp"
#include <string_view>

// Link and op checkers
OpType check_op(std::string_view input);
LinkType check_link(std::string_view input);

bool is_valid_op(OpType op_type);
bool is_valid_link(LinkType link_type);
bool is_valid_repetition(OpType op_type);

// Scanners that consume op/link tokens
std::string_view scan_op(std::string_view input, OpType& op_type);
std::string_view scan_link(std::string_view input, LinkType& link_type);

// Range and repetition helpers
void set_range(Expr& expr);


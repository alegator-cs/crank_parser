#include "ops.hpp"
#include <charconv>
#include <limits>
#include <algorithm>

OpType check_op(std::string_view input) {
    if (input.empty()) return OpType::ONE;
    char c = input[0];
    switch (c) {
        case '?': return OpType::ZERO_OR_ONE;
        case '*': return OpType::ZERO_OR_MORE;
        case '+': return OpType::ONE_OR_MORE;
        case '{': {
            auto end = input.find('}');
            if (end == std::string::npos) return OpType::INVALID_REPETITION_UNMATCHED;
            auto content = input.substr(1, end - 1);
            if (!std::all_of(content.begin(), content.end(), [](char ch){ return std::isdigit(ch) || ch == ','; }))
                return OpType::INVALID_REPETITION_CHAR;
            if (std::count(content.begin(), content.end(), ',') > 1)
                return OpType::INVALID_REPETITION_COMMA_MORE_THAN_ONE;

            size_t comma_pos = content.find(',');
            if (comma_pos == std::string_view::npos) return OpType::N;
            if (comma_pos == 0) return OpType::M_OR_LESS;
            if (comma_pos == content.size() - 1) return OpType::N_OR_MORE;

            auto n_str = content.substr(0, comma_pos);
            auto m_str = content.substr(comma_pos + 1);
            size_t n = 0, m = 0;
            std::from_chars(n_str.data(), n_str.data() + n_str.size(), n);
            std::from_chars(m_str.data(), m_str.data() + m_str.size(), m);
            return (n < m) ? OpType::N_M : OpType::INVALID_REPETITION_M_LTE_N;
        }
    }
    return OpType::NONE;
}

LinkType check_link(std::string_view input) {
    if (input.empty()) return LinkType::NONE;
    if (input[0] == '|') return (input.size() == 1) ? LinkType::INVALID_ALTERNATION_RHS_EMPTY : LinkType::ALTERNATION;
    return LinkType::CONCATENATION;
}

bool is_valid_repetition(OpType op_type) {
    return op_type == OpType::N ||
           op_type == OpType::N_OR_MORE ||
           op_type == OpType::M_OR_LESS ||
           op_type == OpType::N_M;
}

bool is_valid_op(OpType op_type) {
    return op_type == OpType::NONE ||
           op_type == OpType::ONE ||
           op_type == OpType::ZERO_OR_ONE ||
           op_type == OpType::ZERO_OR_MORE ||
           op_type == OpType::ONE_OR_MORE ||
           is_valid_repetition(op_type);
}

bool is_valid_link(LinkType link_type) {
    return link_type == LinkType::CONCATENATION ||
           link_type == LinkType::ALTERNATION;
}

std::string_view scan_op(std::string_view input, OpType& op_type) {
    op_type = check_op(input);
    if (op_type == OpType::ZERO_OR_ONE ||
        op_type == OpType::ZERO_OR_MORE ||
        op_type == OpType::ONE_OR_MORE) {
        return input.substr(0, 1);
    } else if (is_valid_repetition(op_type)) {
        return input.substr(0, input.find('}') + 1);
    }
    return "";
}

std::string_view scan_link(std::string_view input, LinkType& link_type) {
    link_type = check_link(input);
    if (link_type == LinkType::ALTERNATION) return input.substr(0, 1);
    return "";
}

void set_range(Expr& expr) {
    if (expr.op.empty()) { expr.n = 0; expr.m = 0; return; }
    if (!is_valid_op(expr.op_type)) return;
    auto op = expr.op.substr(1);
    size_t max = std::numeric_limits<size_t>::max();
    switch (expr.op_type) {
        case OpType::ZERO_OR_ONE: expr.n = 0; expr.m = 1; break;
        case OpType::ZERO_OR_MORE: expr.n = 0; expr.m = max; break;
        case OpType::ONE_OR_MORE: expr.n = 1; expr.m = max; break;
        case OpType::N: {
            std::from_chars(op.data(), op.data() + op.size(), expr.n);
            expr.m = expr.n;
            break;
        }
        case OpType::N_OR_MORE: {
            auto n = op.substr(0, op.size() - 1);
            std::from_chars(n.data(), n.data() + n.size(), expr.n);
            expr.m = max;
            break;
        }
        case OpType::M_OR_LESS: {
            auto m = op.substr(1);
            expr.n = 0;
            std::from_chars(m.data(), m.data() + m.size(), expr.m);
            break;
        }
        case OpType::N_M: {
            size_t comma_pos = op.find(',');
            auto n = op.substr(0, comma_pos);
            auto m = op.substr(comma_pos + 1);
            std::from_chars(n.data(), n.data() + n.size(), expr.n);
            std::from_chars(m.data(), m.data() + m.size(), expr.m);
            break;
        }
        default: break;
    }
}


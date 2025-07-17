#include "parse.hpp"
#include "ops.hpp"
#include <algorithm>
#include <charconv>

using namespace std::literals::string_view_literals;

std::string_view scan_number(std::string_view input) {
    size_t i = 0;
    while (i < input.size() && std::isdigit(input[i])) i++;
    return input.substr(0, i);
}

bool is_valid_group(GroupType type) {
    return type == GroupType::IMPLICIT ||
           type == GroupType::CAPTURE ||
           type == GroupType::NONCAPTURE ||
           type == GroupType::BRACKET ||
           type == GroupType::BRACKET_NEGATED;
}

GroupType check_group(std::string_view input) {
    if (input.empty()) return GroupType::EMPTY;
    auto c1 = input[0];
    auto c2 = input.size() > 1 ? input[1] : '\0';
    auto c3 = input.size() > 2 ? input[2] : '\0';
    if ("{|?*+"sv.contains(c1)) return GroupType::INVALID_GROUP_START;
    if (c1 == '(') return (c2 == '?' && c3 == ':') ? GroupType::NONCAPTURE : GroupType::CAPTURE;
    if (c1 == '[') return (c2 == '^') ? GroupType::BRACKET_NEGATED : GroupType::BRACKET;
    if (c1 == '\\' && std::isdigit(c2)) return GroupType::REF;
    return GroupType::IMPLICIT;
}

bool is_wrapped(GroupType type) {
    return type == GroupType::CAPTURE || type == GroupType::NONCAPTURE;
}

bool is_bracketed(GroupType type) {
    return type == GroupType::BRACKET || type == GroupType::BRACKET_NEGATED;
}

std::string_view scan_group_implicit(std::string_view input) {
    constexpr std::string_view end = "(){|?*+";
    constexpr std::string_view split = "?*+{";
    size_t pos = input.find_first_of(end);
    if (pos == std::string_view::npos) return input;
    return input.substr(0, (split.contains(input[pos]) && pos > 1) ? pos - 1 : pos);
}

std::string_view scan_group_wrapped(std::string_view input, GroupType& type) {
    int balance = 0;
    size_t i = 0;
    for (; i < input.size(); i++) {
        if (input[i] == '(') balance++;
        else if (input[i] == ')') balance--;
        if (balance == 0) break;
    }
    if (balance != 0) {
        type = GroupType::INVALID_GROUP_UNMATCHED;
        return "";
    }
    return input.substr(0, i + 1);
}

std::string_view unwrap_group(std::string_view input) {
    return (input.size() >= 2) ? input.substr(1, input.size() - 2) : "";
}

std::string_view scan_ref(std::string_view input, GroupType& type, size_t ref_id) {
    auto num = scan_number(input.substr(1));
    size_t n;
    std::from_chars(num.data(), num.data() + num.size(), n);
    if (n > ref_id) {
        type = GroupType::INVALID_REF_ID;
        return "";
    }
    return input.substr(0, 1 + num.size());
}

std::string_view scan_bracket(std::string_view input, GroupType& type) {
    constexpr std::string_view special = ":.=";
    auto end_it = std::adjacent_find(input.begin(), input.end(), [=](char a, char b) { return !special.contains(a) && b == ']'; });
    bool closed = (end_it != input.end());
    if (!closed) {
        type = GroupType::INVALID_BRACKET_UNMATCHED;
        return "";
    }
    ++end_it;
    size_t end_pos = std::distance(input.begin(), end_it);
    return input.substr(0, end_pos);
}

std::string_view scan_bracket_inner(std::string_view input, GroupType& type) {
    if (input.size() < 2) {
        type = GroupType::INVALID_BRACKET_INNER_START;
        return "";
    }
    std::string_view valid_second = ":.=";
    auto open = input.substr(0, 2);
    auto second = open[1];
    if (!valid_second.contains(second)) {
        type = GroupType::INVALID_BRACKET_INNER_START;
        return "";
    }
    auto end_it = std::adjacent_find(input.begin(), input.end(), [=](char a, char b) { return (a == second) && (b == ']'); });
    if (end_it == input.end()) {
        type = GroupType::INVALID_BRACKET_INNER_UNMATCHED;
        return "";
    }
    ++end_it;
    size_t size = std::distance(input.begin(), end_it + 1);
    return input.substr(0, size);
}

bool is_valid_bracket_class(std::string_view input) {
    constexpr std::string_view classes[] = {
        "[:upper:]", "[:lower:]", "[:alpha:]", "[:digit:]", "[:xdigit:]",
        "[:alnum:]", "[:punct:]", "[:blank:]", "[:space:]", "[:cntrl:]",
        "[:graph:]", "[:print:]"
    };
    for (auto cls : classes) {
        if (input.starts_with(cls)) return true;
    }
    return false;
}

bool is_valid_bracket_collation(std::string_view) { return true; }
bool is_valid_bracket_equivalence(std::string_view) { return true; }

std::string_view scan_bracket_char(std::string_view input, GroupType& type) {
    if (input.empty()) return "";
    if (input[0] != '\\') return input.substr(0, 1);
    if (input.size() < 2) {
        type = GroupType::INVALID_BRACKET_ESCAPE;
        return "";
    }
    if (input.substr(0, 2) != "\\x") return input.substr(0, 2);
    if (input.size() < 4) {
        type = GroupType::INVALID_BRACKET_HEX;
        return "";
    }
    auto hex = input.substr(2, 2);
    if (!std::all_of(hex.begin(), hex.end(), ::isxdigit)) {
        type = GroupType::INVALID_BRACKET_HEX;
        return "";
    }
    return input.substr(0, 4);
}

char eval_bracket_char(std::string_view input) {
    if (input.empty()) return '\0';
    if (input.substr(0, 2) == "\\x") {
        char c = 0;
        std::from_chars(input.data() + 2, input.data() + 4, c, 16);
        return c;
    }
    if (input[0] == '\\') {
        switch (input[1]) {
            case 't': return '\t';
            case 'n': return '\n';
            case 'r': return '\r';
            case 'f': return '\f';
            case 'v': return '\v';
            case 'a': return '\a';
            case 'b': return '\b';
            case '\\': return '\\';
            case '\'': return '\'';
            case '"': return '"';
            default: return input[1];
        }
    }
    return input[0];
}

std::string_view scan_group(std::string_view input, GroupType& type, size_t ref_id) {
    type = check_group(input);
    if (type == GroupType::REF) return scan_ref(input, type, ref_id);
    if (is_bracketed(type)) return scan_bracket(input, type);
    if (is_wrapped(type)) return scan_group_wrapped(input, type);
    return scan_group_implicit(input);
}

std::unique_ptr<Expr> parse(std::string_view input) {
    size_t ref_id = 1;
    return parse(input, ref_id);
}

std::unique_ptr<Expr> parse(std::string_view input, size_t& ref_id) {
    std::unique_ptr<Expr> root;
    size_t local_ref_id = ref_id;
    GroupType group_type;
    OpType op_type;
    LinkType link_type;

    auto scan = scan_group(input, group_type, ref_id);
    auto rest = input.substr(scan.size());
    auto op = scan_op(rest, op_type);
    auto link = scan_link(rest.substr(op.size()), link_type);

    bool wrapped = is_wrapped(group_type);
    auto empty_op = ""sv;
    auto empty_link = ""sv;
    size_t no_ref_id = 0;
    size_t zero_idx = 0;
    if (group_type == GroupType::CAPTURE) ref_id++;

    if (!wrapped && scan.size() == input.size()) {
        size_t div = scan.size();
        return std::make_unique<Expr>(group_type, OpType::NONE, LinkType::NONE, input, empty_op, empty_link, no_ref_id, zero_idx, div);
    } else if (wrapped && scan.size() + op.size() == input.size()) {
        root = std::make_unique<Expr>(group_type, op_type, LinkType::NONE, scan, op, empty_link, local_ref_id);
        set_range(*root);
    } else {
        root = std::make_unique<Expr>(GroupType::IMPLICIT, OpType::ONE, LinkType::NONE, input, empty_op, empty_link, no_ref_id);
    }
    
    size_t idx = 0;
    while (!scan.empty()) {
        auto maybe_unwrap = !wrapped ? scan : unwrap_group(scan);
        auto lhs = parse(maybe_unwrap, ref_id);

        lhs->parent = root.get();
        lhs->idx = idx++;
        lhs->group = maybe_unwrap;
        lhs->group_type = group_type;
        lhs->op = op;
        lhs->op_type = op_type;
        lhs->link = link;
        lhs->link_type = link_type;
        set_range(*lhs);

        if (lhs->group_type == GroupType::CAPTURE) lhs->ref_id = local_ref_id;
        root->children.push_back(std::move(lhs));

        rest.remove_prefix(op.size() + link.size());
        scan = scan_group(rest, group_type, ref_id);
        rest = rest.substr(scan.size());
        op = scan_op(rest, op_type);
        link = scan_link(rest.substr(op.size()), link_type);
        wrapped = is_wrapped(group_type);

        if (!is_valid_group(group_type)) {}
        if (!is_valid_op(op_type)) {}
        if (!is_valid_link(link_type)) {}
    }

    return root;
}


#include "frags.hpp"
#include "parse.hpp"
#include <charconv>
#include <limits>
#include <algorithm>
#include <sstream>
#include <iostream>

// Check if token is a bvar, e.g. "{b0}"
bool is_bvar(const std::string& s) {
    return s.find("{b") != std::string::npos;
}

// Check if token is a numeric constant like "1", "42"
bool is_constant(const std::string& s) {
    return !s.empty() && std::all_of(s.begin(), s.end(), ::isdigit);
}

std::string to_beq(std::string_view x_frag) {
    std::string result;
    size_t term_start = 0;
    while (term_start < x_frag.size()) {
        size_t term_end = x_frag.find('+', term_start);
        if (term_end == std::string_view::npos) {
            term_end = x_frag.size();
        }
        std::string_view term = x_frag.substr(term_start, term_end - term_start);
        size_t bvar_start = term.find("{b", 0);
        while (bvar_start != std::string_view::npos) {
            size_t bvar_end = term.find("}", bvar_start + 2);
            std::string_view bvar = term.substr(bvar_start, bvar_end - bvar_start + 1);
            result += bvar;
            result += "*";
            bvar_start = term.find("{b", bvar_end + 1);
        }
        if (!result.empty()) {
            result[result.size() - 1] = '+';
        }
        term_start = term_end + 1;
    }
    if (!result.empty()) {
        result.erase(result.size() - 1, 1);
    }

    return result;
}

// Post-order collection of bvar constraints
void collect_b_eqs(const Expr& expr, std::vector<std::string>& b_eqs, bool is_root = false) {
    for (const auto& ch : expr.children) {
        collect_b_eqs(*ch, b_eqs, false);
    }
    std::string beq = to_beq(expr.x_frag);
    if (!beq.empty() && std::count(beq.begin(), beq.end(), 'b') > 1) {
        beq += is_root ? "==1" : "<=1";
        b_eqs.push_back(beq);
    }
}

// Returns the single semicolon-separated equation string
std::string gen_eqs(const Expr& expr, size_t input_size) {
    std::stringstream ss;

    // First constraint: x_frag = input length
    ss << expr.x_frag << "=" << input_size;

    // Then collect bvar constraints
    std::vector<std::string> b_eqs;
    collect_b_eqs(expr, b_eqs, true);

    for (const auto& beq : b_eqs) {
        ss << ";" << beq;
    }

    return ss.str();
}

std::string_view scan_const(std::string_view input) {
    auto maybe_const = scan_number(input);
    auto rest = input.substr(maybe_const.size());
    return (rest.empty() || rest[0] == '+') ?
               (maybe_const) :
               ("");
}

void scalar_mult_frag(std::string& frag, std::string& c) {
    size_t last_pos = 0;
    size_t pos = 0;
    std::string_view term;
    while ((pos = frag.find('+', pos)) != std::string::npos) {
        term = frag.substr(last_pos, pos);
        frag.insert(pos, "*" + c);
        pos += c.length() + 2;
        last_pos = pos;
    }
    frag += "*" + c;
}

std::string n_m_to_xvar(size_t n, size_t m, size_t xvar_count) {
    size_t min = 0;
    size_t max = std::numeric_limits<decltype(m)>::max();
    std::string nt = (n > min) ? (std::to_string(n)) : ("");
    std::string mt = (m < max) ? (std::to_string(m)) : ("");
    auto count = std::to_string(xvar_count);
    if (n == min && m == max) {
        return "{x" + count + "}";
    }
    if (nt == mt) {
        return "{x" + count + ":" + nt + "}";
    }
    return "{x" + count + ":" + nt + "," + mt + "}";
}

/*
std::vector<std::vector<Expr*>> get_groups(Expr& expr) {
    std::vector<std::vector<Expr*>> groups;
    groups[0].push_back({&expr});
    size_t level = 0;
    bool group_nonempty = true;
    while (group_nonempty) {
        group_nonempty = false;
        size_t imax = groups[level].size();
        for (size_t i = 0; i < imax; i++) {
            Expr& expr = groups[level][i];
            size_t jmax = expr.children.size();
            group_nonempty |= (jmax > 0);
            for (size_t j = 0; j < jmax; j++) {
                auto* ch = expr.children[j].get();
                if (
                group.push_back(ch);
            }
        }
        level++;
    }
}

void match(Expr& expr) {
    std::vector<std::vector<Expr*>> level_groups;

}
*/

void gen_frags(Expr& expr, size_t& xvar_count, size_t& bvar_count) {
    if (expr.children.empty()) {
        auto number = std::to_string(expr.group.size());
        expr.x_frag = number;
    }
    LinkType last_link = LinkType::NONE;
    size_t cat_from = 0;
    // size_t b_eq_from = bvar_count;
    size_t max = expr.children.size();
    for (size_t i = 0; i < max; i++) {
        auto* ch = expr.children[i].get();
        gen_frags(*ch, xvar_count, bvar_count);
        bool last_alt = (last_link == LinkType::ALTERNATION);
        bool alt = (ch->link_type == LinkType::ALTERNATION);
        if (last_alt || alt) {
            ch->bvar = "{b" + std::to_string(bvar_count++) + "}";
            for (size_t j = cat_from; j <= i; j++) {
                scalar_mult_frag(expr.children[j]->x_frag, ch->bvar);
            }
            cat_from = i + 1;
        }
        last_link = ch->link_type;
    }
    for (size_t i = 0; i < max; i++) {
        auto* ch = expr.children[i].get();
        if (!expr.x_frag.empty()) expr.x_frag += "+";
        expr.x_frag += ch->x_frag;
    }
    if (expr.op_type != OpType::NONE && expr.op_type != OpType::ONE) {
        expr.xvar = n_m_to_xvar(expr.n, expr.m, xvar_count++);
        scalar_mult_frag(expr.x_frag, expr.xvar);
    }
}

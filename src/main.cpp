#include "core.hpp"
#include "parse.hpp"
#include "ops.hpp"
#include "frags.hpp"
#include "matching.hpp"
#include "solver_interface.hpp"

#include <iostream>
#include <string>
#include <unordered_map>
#include <cmath>

std::vector<std::pair<int, int>> to_terms(const std::string& eq) {
    size_t eq_pos = eq.find('=');
    std::string lhs = eq.substr(0, eq_pos);

    std::vector<std::pair<int, int>> terms;
    size_t term_start = 0;

    while (term_start < lhs.size()) {
        size_t term_end = lhs.find('+', term_start);
        if (term_end == std::string::npos) term_end = lhs.size();
        std::string term = lhs.substr(term_start, term_end - term_start);

        int coeff = 1;
        size_t y_pos = term.find("y");

        if (y_pos >= 2 && term[y_pos - 1] == '*') {
            std::string coeff_str = term.substr(0, y_pos - 1);
            coeff = std::stoi(coeff_str);
        } else if (y_pos == 1 && term[0] == '-') {
            coeff = -1;
        } else if (y_pos != 0) {
            coeff = std::stoi(term.substr(0, y_pos));
        }

        int var_index = std::stoi(term.substr(y_pos + 1));
        terms.emplace_back(coeff, var_index);

        term_start = term_end + 1;
    }

    return terms;
}

std::vector<int> add_terms(const std::vector<int>& a, const std::vector<int>& b) {
    size_t n = std::max(a.size(), b.size());
    std::vector<int> result(n, 0);
    for (size_t i = 0; i < a.size(); ++i) result[i] += a[i];
    for (size_t i = 0; i < b.size(); ++i) result[i] += b[i];
    return result;
}

std::vector<int> scale_terms(const std::vector<int>& terms, int factor) {
    std::vector<int> result = terms;
    for (int& val : result) val *= factor;
    return result;
}

bool verify_solution(const std::string& eq, const std::vector<std::vector<int>>& sol) {
    auto terms = to_terms(eq);
    std::vector<int> combined;

    int size = static_cast<int>(sol.size());
    for (const auto& [co, idx] : terms) {
        if (idx >= size) throw std::runtime_error("Solution index out of range.");
        auto scaled = scale_terms(sol[idx], co);
        combined = add_terms(combined, scaled);
    }

    return std::all_of(combined.begin(), combined.end(), [](int v) { return v == 0; });
}

std::string rewrite_as_linear(const std::string& equation) {
    std::string result;
    size_t eq_pos = equation.find('=');
    if (eq_pos == std::string::npos) throw std::runtime_error("Missing '=' in equation.");
    std::string lhs = equation.substr(0, eq_pos);
    int rhs = std::stoi(equation.substr(eq_pos + 1));

    size_t term_start = 0;
    int y_index = 0;
    int constant_sum = 0;
    while (term_start < lhs.size()) {
        size_t term_end = lhs.find('+', term_start);
        if (term_end == std::string::npos) term_end = lhs.size();

        std::string term = lhs.substr(term_start, term_end - term_start);
        term.erase(std::remove_if(term.begin(), term.end(), ::isspace), term.end());

        if (term.find('{') == std::string::npos) {
            constant_sum += std::stoi(term);
        } else {
            size_t mul_pos = term.find('*');
            int coef = 1;
            if (mul_pos != std::string::npos) {
                coef = std::stoi(term.substr(0, mul_pos));
            }
            result += std::to_string(coef) + "*y" + std::to_string(y_index++) + "+";
        }

        term_start = term_end + 1;
    }

    if (!result.empty() && result.back() == '+') {
        result.pop_back();
    }

    result += "=" + std::to_string(rhs - constant_sum);
    return result;
}

std::tuple<int, int, int> extended_gcd(int a, int b) {
    if (a == 0) return {b, 0, 1};
    auto [g, x1, y1] = extended_gcd(b % a, a);
    return {g, y1 - (b / a) * x1, x1};
}

std::vector<std::vector<int>> solve_linear(const std::string& eq) {
    size_t eq_pos = eq.find('=');
    std::string lhs = eq.substr(0, eq_pos);
    int rhs = std::stoi(eq.substr(eq_pos + 1));

    std::vector<int> coeffs;
    size_t term_start = 0;
    while (term_start < lhs.size()) {
        size_t term_end = lhs.find('+', term_start);
        if (term_end == std::string::npos) term_end = lhs.size();
        std::string term = lhs.substr(term_start, term_end - term_start);
        size_t mul_pos = term.find('*');
        coeffs.push_back(std::stoi(term.substr(0, mul_pos)));
        term_start = term_end + 1;
    }

    int n = coeffs.size();
    std::vector<std::vector<int>> sol(n);

    if (n == 1) {
        if (rhs % coeffs[0] == 0) sol[0] = {rhs / coeffs[0]};
        return sol;
    }

    if (n == 2) {
        int a = coeffs[0], b = coeffs[1];
        auto [g, x, y] = extended_gcd(abs(a), abs(b));
        if (rhs % g != 0) return {};
        x *= rhs / g; y *= rhs / g;
        if (a < 0) x = -x;
        if (b < 0) y = -y;
        sol[0] = {x, b / g};
        sol[1] = {y, -a / g};
        return sol;
    }

    int a = coeffs[0], b = coeffs[1];
    auto [g, x, y] = extended_gcd(abs(a), abs(b));
    if (a < 0) x = -x;
    if (b < 0) y = -y;

    std::string reduced = std::to_string(g) + "*g";
    for (int i = 2; i < n; ++i) reduced += "+" + std::to_string(coeffs[i]) + "*y" + std::to_string(i);
    reduced += "=" + std::to_string(rhs);

    auto sub = solve_linear(reduced);
    if (sub.empty()) return {};

    std::vector<int> g_expr = sub[0];
    std::vector<int> x_expr(g_expr.size() + 1), y_expr(g_expr.size() + 1);
    for (size_t i = 0; i < g_expr.size(); ++i) {
        x_expr[i] = x * g_expr[i];
        y_expr[i] = y * g_expr[i];
    }
    x_expr.back() = b / g;
    y_expr.back() = -a / g;

    sol[0] = x_expr;
    sol[1] = y_expr;
    for (size_t i = 1; i < sub.size(); ++i) {
        sol[i + 1] = sub[i];
    }

    return sol;
}


void print_expr(const Expr& expr, int indent = 0) {
    std::string pad(indent * 2, ' ');
    std::cout << pad << "Expr: group=\"" << expr.group << "\", op=\"" << expr.op
              << "\", link=\"" << expr.link << "\", active=" << expr.active << "\n";
    for (const auto& eq : expr.eqs) {
        std::cout << pad << "  eq: " << eq.text << "\n";
    }
    std::cout << pad << "  x_frag: " << expr.x_frag << "\n";
    for (const auto& b_eq : expr.b_eqs) {
        std::cout << pad << "  b_eq: " << b_eq << "\n";
    }
    for (const auto& ch : expr.children) {
        print_expr(*ch, indent + 1);
    }
}

void set_expr_with_eq(Expr& expr, const Equation& eq, const std::unordered_map<std::string, size_t>& sol) {
    size_t sum = expr.start;
    for (auto& ch : expr.children) {
        if (std::find(eq.traversed.begin(), eq.traversed.end(), ch.get()) != eq.traversed.end()) {
            ch->start = sum;
            set_expr_with_eq(*ch, eq, sol);
            sum += ch->size;
        }
    }
    expr.size = expr.children.empty() ? expr.group.size() : sum;
    char ch = (!expr.xvar.empty() && expr.xvar.find('x')) ? 'x' : 'b';
    if (!expr.xvar.empty()) {
        auto pos = expr.xvar.find(ch);
        auto num = expr.xvar.substr(pos + 1);
        auto id = num.substr(0, num.find_first_not_of("0123456789"));
        auto key = std::string(ch, 1) + std::string(id);
        auto mult = sol.at(key);
        expr.size *= mult;
    }
}

void print_solution(const std::vector<std::vector<int>>& solution) {
    for (const auto& y : solution) {
        std::string expr;
        for (size_t i = 0; i < y.size(); ++i) {
            int coeff = y[i];
            if (coeff == 0) continue;
            if (!expr.empty()) expr += (coeff > 0 ? "+" : "");
            if (i == 0) {
                expr += std::to_string(coeff);
            } else {
                if (coeff == 1) expr += "t" + std::to_string(i - 1);
                else if (coeff == -1) expr += "-t" + std::to_string(i - 1);
                else expr += std::to_string(coeff) + "*t" + std::to_string(i - 1);
            }
        }
        if (expr.empty()) expr = "0";
        std::cout << expr << std::endl;
    }
}


int main() {
    std::string regex, input;
    std::cout << "Enter a regex: ";
    std::getline(std::cin, regex);
    std::cout << "Enter input string: ";
    std::getline(std::cin, input);

    auto expr = parse(regex);
    if (!expr) {
        std::cerr << "Parse failed.\n";
        return 1;
    }

    size_t xcount = 0, bcount = 0;
    gen_frags(*expr, xcount, bcount);
    optimize_parse_tree(*expr, input);
    set_depths(expr.get());
    

    std::cout << "\nExpression Tree:\n";
    print_expr(*expr);

    std::vector<std::vector<size_t>> matches;
    match_down({expr.get()}, input.size(), input, matches);
    for (size_t i = 0, imax = matches.size(); i < imax; i++) {
        std::cout << "match: ";
        for (size_t j = 0, jmax = matches[i].size(); j < jmax; j++) {
            std::cout << matches[i][j] << ", ";
        }
        std::cout << std::endl; 
    }

    return 0;

    std::string full_eq = gen_eqs(*expr, input.size());
    std::string first_eq = full_eq.substr(0, full_eq.find(';', 0));
    std::cout << "Solving: " << full_eq << "\n";

    try {
        // Rewrite as linear diophantine equation
        auto linear_eq = rewrite_as_linear(first_eq);
        std::cout << "Linear diophantine form: " << linear_eq << "\n";
        
        // Solve the linear diophantine equation
        std::cout << "General solution:\n";
        auto solution = solve_linear(linear_eq);
        print_solution(solution);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "testing some solver cases.." << std::endl;
    // print_solution(solve_linear("0*y0+0*y1+4*y2=0"));
    std::cout << "2*y0+4*y1=0.." << std::endl;
    print_solution(solve_linear("2*y0+4*y1=0"));
    std::cout << "2*y0+4*y1=5.." << std::endl;
    print_solution(solve_linear("2*y0+4*y1=5"));
    std::cout << "4*y0+6*y1+2*y2=0.." << std::endl;
    print_solution(solve_linear("4*y0+6*y1+2*y2=0"));
    std::cout << "2*y0+4*y1+6*y2=0.." << std::endl;
    print_solution(solve_linear("2*y0+4*y1+6*y2=0.."));
    if (verify_solution("2*y0+4*y1+6*y2=0", solve_linear("2*y0+4*y1+6*y2=0"))) {
        std::cout << "verified" << std::endl;
    } else {
        std::cout << "solution failed!" << std::endl;
    }
    

    // matching is still temporarily out of scope, until the next task!
    /*
    bool matched = false;
    for (const auto& sol : solutions) {
        // set_expr_with_eq(*expr, full_eq, sol);
        if (match(*expr, input, 0, sol)) {
            matched = true;
            std::cout << "Matched: ";
            for (const auto& [k, v] : sol) std::cout << k << "=" << v << " ";
            std::cout << "\n";
        }
    }
    if (!matched) std::cout << "No matches.\n";
    */

    return 0;
}


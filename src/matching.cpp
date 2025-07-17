#include "matching.hpp"
#include <algorithm>
#include <utility>
#include <numeric>
#include <string_view>

template <typename T>
std::vector<T> vec_cat(const std::vector<T>& v1, const std::vector<T>& v2) {
    std::vector<T> combined = v1;
    combined.insert(combined.end(), v2.begin(), v2.end());
    return combined;
}

void match_up(std::vector<std::tuple<Expr*, size_t, std::string>> exprs, const size_t N, const std::string_view input, std::vector<size_t> match, std::vector<std::vector<size_t>>& matches) {
    size_t g = 0;
    size_t s = 0;
    for (size_t i = 0, max = exprs.size(); i < max; i++) {
        size_t d = std::get<size_t>(exprs[i]);
        g = std::gcd(g, d);
        s += d;
    }
    if ((g > 0 && N % g != 0) || s > N) return;
    std::vector<std::vector<size_t>> expansions;
    std::vector<size_t> group_indices = {0};
    auto* p = std::get<Expr*>(exprs[0])->parent;
    for (size_t i = 0, imax = exprs.size(); i < imax; i++) {
        auto* e = std::get<Expr*>(exprs[i]);
        if (p && e->depth <= p->depth) {
            group_indices.push_back(i);
            break;   
        }
        if (e->parent != p) {
            group_indices.push_back(i);
            p = e->parent;
        }
        if (i + 1 == imax) {
            group_indices.push_back(i + 1);
        }
        std::vector<size_t> candidates;
        auto div = std::get<size_t>(exprs[i]);
        div = (div > 0) ? (div) : (1);
        auto op_type = e->op_type;
        size_t min = 0;
        size_t max = 0;
        if (op_type == OpType::ONE || op_type == OpType::NONE) {
            min = 1;
            max = 1;
        }
        else if (op_type == OpType::ZERO_OR_ONE) {
            min = 0;
            max = 1;
        }
        else if (op_type == OpType::N) {
            min = e->n;
            max = min;
        }
        else if (op_type == OpType::N_M) {
            min = e->n;
            max = e->m;
        }
        else if (op_type == OpType::ZERO_OR_MORE) {
            min = 0;
            max = N / div;
        }
        else if (op_type == OpType::ONE_OR_MORE) {
            min = 1;
            max = N / div;
        }
        else if (op_type == OpType::N_OR_MORE) {
            min = e->n;
            max = N / div;
        }
        else if (op_type == OpType::M_OR_LESS) {
            min = 0;
            max = e->m;
        }
        if (min == 0) {
            candidates.push_back(0);
        }
        for (size_t pos = 0; pos < N; pos++) {
            size_t count = 0;
            std::string leaf = std::get<std::string>(exprs[i]);
            while (match_leaf(leaf, input, pos)) {
                pos += std::get<std::string>(exprs[i]).size();
                count++;
                if (count >= min && count <= max) {
                    auto it = std::find(candidates.begin(), candidates.end(), count);
                    if (it == candidates.end()) {
                        it = std::lower_bound(candidates.begin(), candidates.end(), count);
                        candidates.insert(it, count);
                    }
                }
            }
        }
        if (candidates.empty()) return;
        expansions.push_back(candidates);
    }
    std::function<void(size_t depth, std::vector<size_t>&& current)> product = [&](size_t depth, std::vector<size_t>&& current) {
        if (depth == expansions.size()) {
            std::vector<std::tuple<Expr*, size_t, std::string>> collapsed;
            for (size_t g = 0, gmax = group_indices.size() - 1; g < gmax; g++) {
                size_t min_group_idx = group_indices[g];
                size_t max_group_idx = group_indices[g + 1];
                auto* e = std::get<Expr*>(exprs[min_group_idx]);
                auto* p = e->parent;
                size_t pdiv = 0;
                std::string pleaf = "";
                for (size_t i = min_group_idx, imax = max_group_idx; i < imax; i++) {
                    auto leaf = std::get<std::string>(exprs[i]);
                    auto div = std::get<size_t>(exprs[i]);
                    auto candidate = current[i];
                    for (size_t j = 0, jmax = candidate; j < jmax; j++) {
                        pleaf += leaf;
                    }
                    pdiv += div * candidate;
                }
                collapsed.push_back({p, pdiv, pleaf});
            }
            size_t last_group_idx = group_indices[group_indices.size() - 1];
            for (size_t i = last_group_idx, imax = exprs.size(); i < imax; i++) {
                collapsed.push_back(exprs[i]);
            }
            if (exprs.size() > 1) {
                std::vector<size_t> m = match;
                for (size_t i = 0, imax = current.size(); i < imax; i++) {
                    auto* e = std::get<Expr*>(exprs[i]);
                    auto op_type = e->op_type;
                    if (op_type != OpType::ONE) m.push_back(current[i]);
                }
                match_up(std::move(collapsed), N, input, m, matches);
            } else if (std::get<std::string>(collapsed[0]) == input) {
                matches.push_back(std::move(match));
            }
            return;
        }
        for (auto choice : expansions[depth]) {
            auto next = current;
            next.push_back(choice);
            product(depth + 1, std::move(next));
        }
    };
    product(0, {});
}

void match_down(std::vector<Expr*> exprs, const size_t N, const std::string_view input, std::vector<std::vector<size_t>>& matches) {
    bool all_leaf = std::all_of(exprs.begin(), exprs.end(), [](Expr* e) {
        return e->children.empty();
    });
    if (all_leaf) {
        std::vector<std::tuple<Expr*, size_t, std::string>> exprs_up;
        bool any_active = false;
        for (auto* e : exprs) {
            if (e->active) {
                any_active = true;
                size_t div = e->group.size();
                std::string leaf = std::string{e->group};
                exprs_up.push_back({e, div, leaf});
            }
        }
        if (any_active) match_up(exprs_up, N, input, {}, matches);
        return;
    }
    std::vector<std::vector<Expr*>> expansions;
    size_t tail_idx = 0;
    bool prev_alt = false;
    bool alt = false;
    for (auto* e : exprs) {
        for (size_t i = 0, imax = e->children.size(); i < imax; i++) {
            auto* ch = e->children[i].get();
            prev_alt = alt;
            alt = (ch->link_type == LinkType::ALTERNATION);
            bool cat = !(alt || prev_alt);
            bool first_alt = !prev_alt && alt;
            if (cat || first_alt) {
                expansions.push_back({ch});
                tail_idx++;
            } else {
                expansions[tail_idx].push_back(ch);
            }
        }
    }
    for (auto* e : exprs) {
        if (e->children.empty()) {
            expansions.push_back({e});
        }
    }
    std::function<void(size_t depth, std::vector<Expr*> current)> product = [&](size_t depth, std::vector<Expr*> current) {
        if (depth == expansions.size()) {
            match_down(current, N, input, matches);
            return;
        }
        for (auto* choice : expansions[depth]) {
            auto next = current;
            next.push_back(choice);
            product(depth + 1, std::move(next));
        }
    };
    product(0, {});
}

bool match_leaf(std::string_view leaf, std::string_view input, size_t pos) {
    if (pos >= input.size() || leaf.empty()) return false;
    char target = input[pos];

    if (leaf[0] == '\\') {
        if (leaf.size() < 2) return false;
        char esc = leaf[1];
        switch (esc) {
            case 'd': return std::isdigit(static_cast<unsigned char>(target));
            case 'D': return !std::isdigit(static_cast<unsigned char>(target));
            case 'w': return std::isalnum(static_cast<unsigned char>(target)) || target == '_';
            case 'W': return !(std::isalnum(static_cast<unsigned char>(target)) || target == '_');
            case 's': return std::isspace(static_cast<unsigned char>(target));
            case 'S': return !std::isspace(static_cast<unsigned char>(target));
            default: return target == esc;
        }
    } else if (leaf[0] == '[') {
        auto it = std::adjacent_find(leaf.begin() + 1, leaf.end(), [](char a, char b) {
            return b == ']' && (a != ':' && a != '=' && a != '.');
        });
        if (it == leaf.end()) return false;
        size_t close = (it - leaf.begin()) + 1;
        std::string_view content = leaf.substr(1, close - 1);
        bool negated = !content.empty() && content[0] == '^';
        if (negated) content.remove_prefix(1);

        bool match = false;
        for (size_t i = 0; i < content.size();) {
            if (i + 1 < content.size() && content[i] == '[' && content[i+1] == ':') {
                size_t end = content.find(":]", i);
                if (end == std::string_view::npos) break;
                std::string_view cls = content.substr(i + 2, end - (i + 2));
                if ((cls == "upper" && std::isupper(target)) ||
                    (cls == "lower" && std::islower(target)) ||
                    (cls == "alpha" && std::isalpha(target)) ||
                    (cls == "digit" && std::isdigit(target)) ||
                    (cls == "alnum" && std::isalnum(target)) ||
                    (cls == "space" && std::isspace(target))) {
                    match = true;
                }
                i = end + 2;
            } else if (i + 2 < content.size() && content[i+1] == '-') {
                if (content[i] <= target && target <= content[i+2]) match = true;
                i += 3;
            } else {
                if (content[i] == target) match = true;
                ++i;
            }
        }
        return negated ? !match : match;
    }
    return leaf[0] == target;
}

bool match(Expr& expr, const Equation& eq, const std::unordered_map<std::string, size_t>& sol, std::string_view input, size_t pos) {
    size_t count = 1;
    if (!expr.xvar.empty()) {
        auto xpos = expr.xvar.find('x');
        auto num = expr.xvar.substr(xpos + 1);
        size_t end = num.find_first_not_of("0123456789");
        auto key = std::string("x") + std::string(num.substr(0, end));
        count = sol.at(key);
    }
    if (expr.children.empty()) {
        for (size_t i = 0; i < count; ++i) {
            if (!match_leaf(expr.group, input, pos)) return false;
            pos += expr.group.size();
        }
    } else {
        for (size_t i = 0; i < count; ++i) {
            Expr* first = nullptr;
            for (auto& ch : expr.children) {
                if (std::find(eq.traversed.begin(), eq.traversed.end(), ch.get()) != eq.traversed.end()) {
                    if (!first) {
                        first = ch.get();
                        pos = first->start;
                    }
                    if (!match(*ch, eq, sol, input, pos)) return false;
                    pos += ch->size;
                }
            }
        }
    }
    return true;
}

void get_leaves(Expr& expr, std::vector<Expr*>& leaves) {
    if (expr.children.empty()) leaves.push_back(&expr);
    for (auto& ch : expr.children) get_leaves(*ch, leaves);
}

void set_depths(Expr* node, size_t current_depth) {
    node->depth = current_depth;
    for (size_t i = 0, imax = node->children.size(); i < imax; i++) {
        auto* ch = node->children[i].get();
        set_depths(ch, current_depth + 1);
    }
}

void optimize_parse_tree(Expr& expr, std::string_view input) {
    std::vector<Expr*> leaves;
    get_leaves(expr, leaves);
    for (auto* leaf : leaves) {
        bool matched = false;
        for (size_t i = 0; i < input.size(); ++i) {
            if (match_leaf(leaf->group, input, i)) {
                matched = true;
                break;
            }
        }
        if (!matched) leaf->active = false;
    }
    propagate_inactives(expr);
}

void propagate_inactives(Expr& expr) {
    for (auto& ch : expr.children) propagate_inactives(*ch);
    if (expr.children.empty()) return;

    bool active = true;
    size_t i = 0;
    while (i < expr.children.size()) {
        LinkType type = expr.children[i]->link_type;
        bool chain_active = (type == LinkType::ALTERNATION) ? false : true;
        while (true) {
            if (type == LinkType::ALTERNATION) {
                if (expr.children[i]->active) chain_active = true;
            } else {
                if (!expr.children[i]->active) chain_active = false;
            }
            if (expr.children[i]->link_type == LinkType::NONE) {
                ++i;
                break;
            }
            ++i;
        }
        if (!chain_active) active = false;
    }
    expr.active = expr.active && active;
}


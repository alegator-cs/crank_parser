#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <memory>

// Enums for regex parsing

enum class LinkType : size_t {
    INVALID_ALTERNATION_RHS_EMPTY,
    NONE,
    CONCATENATION,
    ALTERNATION,
};

enum class OpType : size_t {
    INVALID_REPETITION_UNMATCHED,
    INVALID_REPETITION_CHAR,
    INVALID_REPETITION_COMMA_MORE_THAN_ONE,
    INVALID_REPETITION_N_AND_M_MISSING,
    INVALID_REPETITION_M_LTE_N,

    NONE,
    ONE,
    ZERO_OR_ONE,
    ZERO_OR_MORE,
    ONE_OR_MORE,
    N,
    N_OR_MORE,
    M_OR_LESS,
    N_M,
};

enum class GroupType : size_t {
    INVALID_GROUP_UNMATCHED,
    INVALID_GROUP_START,
    INVALID_BRACKET_UNMATCHED,
    INVALID_BRACKET_INNER_UNMATCHED,
    INVALID_BRACKET_INNER_START,
    INVALID_BRACKET_INNER,
    INVALID_BRACKET_RANGE,
    INVALID_BRACKET_COLLATE,
    INVALID_BRACKET_EQUIVALENCE,
    INVALID_BRACKET_HEX,
    INVALID_BRACKET_ESCAPE,
    INVALID_REF_ID,

    EMPTY,
    IMPLICIT,
    CAPTURE,
    NONCAPTURE,
    BRACKET,
    BRACKET_NEGATED,
    REF,
};

struct Expr;

struct Equation {
    std::string text;
    std::vector<Expr*> traversed;

    Equation(std::string t = {}, std::vector<Expr*> tr = {})
        : text(std::move(t)), traversed(std::move(tr)) {}
};

struct Expr {
    GroupType group_type;
    OpType op_type;
    LinkType link_type;
    std::string_view group;
    std::string_view op;
    std::string_view link;
    size_t ref_id;
    size_t idx;
    size_t depth;
    Expr* parent;
    std::vector<std::unique_ptr<Expr>> children;
    std::vector<Equation> eqs;
    std::string x_frag;
    std::vector<std::string> b_eqs;

    std::string xvar;
    std::string bvar;
    size_t n;
    size_t m;
    size_t start;
    size_t size;
    bool active;

    Expr(GroupType group_type = GroupType::IMPLICIT,
         OpType op_type = OpType::ONE,
         LinkType link_type = LinkType::NONE,
         std::string_view group = "",
         std::string_view op = "",
         std::string_view link = "",
         size_t ref_id = 0,
         size_t idx = 0,
         size_t depth = 0,
         Expr* parent = nullptr,
         std::vector<std::unique_ptr<Expr>> children = {},
         std::vector<Equation> eqs = {},
         std::string x_frag = {},
         std::vector<std::string> b_eqs = {},
         std::string xvar = "",
         std::string bvar = "",
         size_t n = 0,
         size_t m = 0,
         size_t start = 0,
         size_t size = 0,
         bool active = true);
};


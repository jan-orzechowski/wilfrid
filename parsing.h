#pragma once

typedef struct expr expr;
typedef struct stmt stmt;

typedef enum expr_kind
{
    EXPR_NONE = 0,
    EXPR_INT,
    EXPR_NAME,
    EXPR_UNARY,
    EXPR_BINARY,
    EXPR_TERNARY
} expr_kind;

typedef struct unary_expr unary_expr;
typedef struct binary_expr binary_expr;
typedef struct ternary_expr ternary_expr;

struct unary_expr
{
    token_kind operator;
    expr* operand;
};

struct binary_expr
{
    token_kind operator;
    expr* left_operand;
    expr* right_operand;
};

struct ternary_expr
{
    expr* condition;
    expr* if_true;
    expr* if_false;
};

struct expr
{
    expr_kind kind;
    union
    {
        int number_value;
        const char* identifier;
        const char* string_value;
        unary_expr unary_expr_value;
        binary_expr binary_expr_value;
        ternary_expr ternary_expr_value;
    };
};

typedef struct stmt_block
{
    // list of statements
    stmt* statements;
    size_t statements_count;
} stmt_block;

typedef enum stmt_kind
{
    STMT_NONE,
    STMT_RETURN,
    STMT_BREAK,
    STMT_CONTINUE,
    STMT_PRINT,
    STMT_LIST,
    STMT_IF_ELSE,
    STMT_WHILE,
    STMT_FOR,
    STMT_SWITCH,
    STMT_EXPRESSION
} stmt_kind;

typedef struct else_if
{
    expr* condition;
    stmt_block then;
} else_if;

typedef struct if_else
{
    expr* cmp_expr;
    else_if* else_ifs;
    size_t else_ifs_count;
    stmt_block else_block;
} if_else;

typedef struct switch_case
{
    expr** expressions;
    size_t expressions_count;
    stmt_block* code_block;
} switch_case;

typedef struct switch_stmt
{
    expr* checked_expression;
    switch_case* cases;
    size_t cases_num;
} switch_stmt;

typedef struct return_stmt
{
    expr* expression;
} return_stmt;

struct stmt
{
    // brakuje:
    // break, continue
    // while, for, do while
    // assign
    stmt_kind kind;
    union
    {
        return_stmt return_statement;
        stmt* statement_list;
        if_else if_else_statement;
        switch_stmt switch_statement;
        
        expr* expression;
    };

};

typedef enum decl_kind
{
    DECL_STRUCT,
    DECL_VARIABLE,
    DECL_FUNCTION
} decl_kind;

typedef struct struct_decl
{
    int x; // tu powinna być lista typów
} struct_decl;

typedef struct variable_decl
{
    char* identifier;
    char* type; 
    expr* expression;
} variable_decl;

typedef struct function_decl
{
    char* name;
    // types list
    // return type
    stmt_block statements;

} function_decl;

typedef struct decl
{
    decl_kind kind;
    union
    {
        function_decl function_declaration;
        variable_decl variable_declaration;
        struct_decl struct_declaration;
    };
} decl;


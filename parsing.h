﻿#pragma once

#include <stdint.h>

typedef struct expr expr;
typedef struct stmt stmt;
typedef struct decl decl;
typedef struct type type;

typedef enum type_kind
{
    TYPE_NONE,

    TYPE_INCOMPLETE,
    TYPE_COMPLETING,

    // built-in types
    TYPE_INT,
    TYPE_FLOAT,
    TYPE_CHAR,

    TYPE_NAME,
    TYPE_ARRAY,
    TYPE_POINTER,
    TYPE_FUNCTION
} type_kind;

typedef struct type_array
{
    type* base_type;
    expr* size_expr;
} type_array;

typedef struct type_pointer
{
    type* base_type;
} type_pointer;

typedef struct type_function
{
    type* returned_type;
    type** parameter_types;
    size_t parameter_count;
} type_function;

typedef struct symbol symbol;

struct type
{
    type_kind kind;
    symbol* symbol;
    size_t size;
    size_t align;
    union
    {
        const char* name;
        type_array array;
        type_pointer pointer;
        type_function function;
    };
};

typedef enum expr_kind
{
    EXPR_NONE = 0,
    EXPR_INT,
    EXPR_NAME,
    EXPR_UNARY,
    EXPR_BINARY,
    EXPR_TERNARY,
    EXPR_CALL,
    EXPR_FIELD,
    EXPR_INDEX,
    EXPR_SIZEOF,
    EXPR_COMPOUND_LITERAL
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

typedef struct call_expr
{
    expr * function_expr;
    expr** args;
    size_t args_num;
} call_expr;

typedef struct index_expr
{
    expr* array_expr;
    expr* index_expr;
} index_expr;

typedef struct field_expr
{
    expr* expr;
    const char* field_name;
} field_expr;

typedef struct compound_literal_field
{
    expr* expr;
    const char* field_name;
} compound_literal_field;

typedef struct compound_literal_expr
{
    compound_literal_field** fields;
    size_t fields_count;
} compound_literal_expr;

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
        call_expr call_expr_value;
        index_expr index_expr_value;
        field_expr field_expr_value;
        compound_literal_expr compound_literal_expr_value;
    };
};

typedef struct stmt_block
{
    stmt** statements;
    size_t statements_count;
} stmt_block;

typedef enum stmt_kind
{
    STMT_NONE,
    STMT_RETURN,
    STMT_BREAK,
    STMT_CONTINUE,
    STMT_DECL,
    STMT_PRINT,
    STMT_LIST,
    STMT_IF_ELSE,
    STMT_WHILE,
    STMT_DO_WHILE,
    STMT_FOR,
    STMT_ASSIGN,
    STMT_SWITCH,
    STMT_EXPR,
    STMT_BLOCK,    
} stmt_kind;

typedef struct if_else_stmt if_else_stmt;
struct if_else_stmt
{
    expr* cond_expr;
    stmt_block then_block;
    stmt* else_stmt;
};

typedef struct return_stmt
{
    expr* expression;
} return_stmt;

typedef struct decl_stmt
{
    decl* decl;
} decl_stmt;

typedef struct while_stmt
{
    expr* cond_expr;
    stmt_block statements;    
} while_stmt;

typedef struct for_stmt
{
    decl* init_decl;
    expr* cond_expr;
    stmt* incr_stmt;
    stmt_block statements;
} for_stmt;

typedef struct assign_stmt
{
    expr* assigned_var_expr;
    token_kind operation;
    expr* value_expr;
} assign_stmt;

typedef struct switch_case
{
    expr** cond_exprs;
    size_t cond_exprs_num;
    stmt_block statements;
    bool is_default;
    bool fallthrough;
} switch_case;

typedef struct switch_stmt
{
    expr* var_expr;
    switch_case** cases;
    size_t cases_num;    
} switch_stmt;

struct stmt
{
    stmt_kind kind;
    union
    {
        return_stmt return_statement;
        stmt* statement_list;
        if_else_stmt if_else_statement;
        decl_stmt decl_statement;
        for_stmt for_statement;
        assign_stmt assign_statement;
        expr* expression;
        stmt_block statements_block;
        while_stmt while_statement;
        while_stmt do_while_statement;
        switch_stmt switch_statement;
    };
};

typedef enum decl_kind
{
    DECL_STRUCT,
    DECL_UNION,
    DECL_VARIABLE,
    DECL_FUNCTION,
    DECL_TYPEDEF,
    DECL_ENUM
} decl_kind;

typedef struct aggregate_field
{
    char* identifier;
    type* type;
} aggregate_field;

typedef struct aggregate_decl
{
    aggregate_field* fields;
    size_t fields_count;
} aggregate_decl;

typedef struct variable_decl
{
    type* type; 
    expr* expression;
} variable_decl;

typedef struct function_param
{
    char* identifier;
    type* type;
} function_param;

typedef struct function_param_list
{
    function_param* params;
    int param_count;
} function_param_list;

typedef struct function_decl
{
    function_param_list parameters;
    type* return_type;
    stmt_block statements;
} function_decl;

typedef struct enum_value
{
    char* identifier;
    bool value_set;
    int64_t value;
} enum_value;

typedef struct enum_decl
{
    enum_value* values;
    size_t values_count;
} enum_decl;

typedef struct typedef_decl
{
    type* type;
    const char* name;
} typedef_decl;

struct decl
{
    decl_kind kind;
    char* identifier;
    union
    {
        function_decl function_declaration;
        variable_decl variable_declaration;
        aggregate_decl aggregate_declaration;
        enum_decl enum_declaration;
        typedef_decl typedef_declaration;
    };
};
typedef struct expr expr;
typedef struct stmt stmt;
typedef struct decl decl;
typedef struct type type;
typedef struct typespec typespec;
typedef struct symbol symbol;

typedef enum typespec_kind
{
    TYPESPEC_NONE,
    TYPESPEC_NAME,
    TYPESPEC_ARRAY,
    TYPESPEC_LIST,
    TYPESPEC_POINTER,
    TYPESPEC_FUNCTION
} typespec_kind;

typedef struct typespec_array
{
    typespec *base_type;
    expr *size_expr;
} typespec_array;

typedef struct typespec_list
{
    typespec *base_type;
} typespec_list;

typedef struct typespec_pointer
{
    typespec *base_type;
} typespec_pointer;

typedef struct typespec_function
{
    typespec *ret_type;
    typespec **param_types;
    size_t param_count;
} typespec_function;

struct typespec
{
    typespec_kind kind;
    source_pos pos;
    union
    {
        const char *name;
        typespec_array array;
        typespec_list list;
        typespec_pointer pointer;
        typespec_function function;
    };
};

typedef enum expr_kind
{
    EXPR_NONE = 0,
    
    EXPR_INT,
    EXPR_FLOAT,
    EXPR_CHAR,
    EXPR_STRING,
    EXPR_NULL,
    EXPR_BOOL,

    EXPR_NAME,
    EXPR_UNARY,
    EXPR_BINARY,
    EXPR_TERNARY,
    EXPR_CALL,
    EXPR_FIELD,
    EXPR_INDEX,
    EXPR_NEW,
    EXPR_AUTO,
    EXPR_SIZE_OF,
    EXPR_SIZE_OF_TYPE,
    EXPR_CAST,
    EXPR_COMPOUND_LITERAL,

    EXPR_STUB
} expr_kind;

typedef struct unary_expr unary_expr;
typedef struct binary_expr binary_expr;
typedef struct ternary_expr ternary_expr;

struct unary_expr
{
    token_kind operator;
    expr *operand;
};

struct binary_expr
{
    token_kind operator;
    expr *left;
    expr *right;
};

struct ternary_expr
{
    expr *condition;
    expr *if_true;
    expr *if_false;
};

typedef struct call_expr
{
    expr *function_expr;
    expr **args;
    size_t args_num;
    expr *method_receiver;
    symbol *resolved_function; // określa, które z przeciążeń wezwać
} call_expr;

typedef struct index_expr
{
    expr *array_expr;
    expr *index_expr;
} index_expr;

typedef struct field_expr
{
    expr *expr;
    const char *field_name;
} field_expr;

typedef struct compound_literal_field
{
    expr *expr;
    int64_t field_index;
    const char *field_name;
} compound_literal_field;

typedef struct compound_literal_expr
{
    typespec *type;
    compound_literal_field **fields;
    size_t fields_count;
} compound_literal_expr;

typedef struct new_expr
{
    typespec *type;
    type *resolved_type;
} new_expr;

typedef struct auto_expr
{
    typespec *type;
    type *resolved_type;
} auto_expr;

typedef struct size_of_type_expr
{
    typespec *type;
    type *resolved_type;
} size_of_type_expr;

typedef struct size_of_expr
{
    expr *expr;
} size_of_expr;

typedef struct cast_expr
{
    typespec *type;
    type *resolved_type;
    expr *expr;
} cast_expr;

typedef enum stub_expr_kind
{
    STUB_EXPR_NONE = 0,
    STUB_EXPR_LIST_FREE,
    STUB_EXPR_LIST_REMOVE_AT,
    STUB_EXPR_LIST_LENGTH,
    STUB_EXPR_LIST_CAPACITY,
    STUB_EXPR_LIST_ADD,
    STUB_EXPR_LIST_NEW,
    STUB_EXPR_LIST_AUTO,
    STUB_EXPR_LIST_INDEX,
    STUB_EXPR_CAST,
    STUB_EXPR_POINTER_ARITHMETIC_BINARY,
    STUB_EXPR_POINTER_ARITHMETIC_INC,
} stub_expr_kind;

typedef enum cast_kind
{
    CAST_NONE = 0,
    CAST_NO_CAST_NEEDED,
    CAST_TYPES_INCOMPATIBLE,
    CAST_LEFT,
    CAST_RIGHT,
    CAST_BOTH
} cast_kind;

typedef struct cast_info
{
    cast_kind kind;
    type *type;
} cast_info;

typedef struct stub_expr
{
    stub_expr_kind kind;
    expr *original_expr;
    union
    {
        bool left_is_pointer; // gdy STUB_EXPR_POINTER_ARITHMETIC
        bool is_inc; // gdy STUB_EXPR_POINTER_ARITHMETIC_INC
    };
} stub_expr;

struct expr
{
    expr_kind kind;
    type *resolved_type; // dodawany po parsowaniu
    source_pos pos;
    union
    {
        uint64_t integer_value;
        float float_value;
        bool bool_value;
        const char *name;
        const char *string_value;
        unary_expr unary;
        binary_expr binary;
        ternary_expr ternary;
        call_expr call;
        index_expr index;
        field_expr field;
        compound_literal_expr compound;
        new_expr new;
        auto_expr auto_new;
        size_of_expr size_of;
        size_of_type_expr size_of_type;
        cast_expr cast;
        stub_expr stub;
    };
};

typedef enum stmt_kind
{
    STMT_NONE,
    STMT_RETURN,
    STMT_BREAK,
    STMT_CONTINUE,
    STMT_DECL,
    STMT_IF_ELSE,
    STMT_WHILE,
    STMT_DO_WHILE,
    STMT_FOR,
    STMT_ASSIGN,
    STMT_SWITCH,
    STMT_EXPR,
    STMT_BLOCK,  
    STMT_DELETE,
    STMT_INC
} stmt_kind;

typedef struct stmt_block
{
    stmt **stmts;
    size_t stmts_count;
} stmt_block;

typedef struct if_else_stmt
{
    expr *cond_expr;
    stmt_block then_block;
    stmt *else_stmt;
} if_else_stmt;

typedef struct return_stmt
{
    expr *ret_expr;
} return_stmt;

typedef struct decl_stmt
{
    decl *decl;
} decl_stmt;

typedef struct while_stmt
{
    expr *cond_expr;
    stmt_block stmts;    
} while_stmt;

typedef struct for_stmt
{
    stmt *init_stmt;
    expr *cond_expr;
    stmt *next_stmt;
    stmt_block stmts;
} for_stmt;

typedef struct assign_stmt
{
    expr *assigned_var_expr;
    token_kind operation;
    expr *value_expr;
} assign_stmt;

typedef struct switch_case switch_case;
struct switch_case
{
    expr **cond_exprs;
    int64_t *cond_exprs_vals;
    size_t cond_exprs_num;
    stmt_block stmts;
    char is_default;
    char fallthrough;
};

typedef struct switch_stmt
{
    expr *var_expr;
    switch_case **cases;
    size_t cases_num;  
} switch_stmt;

typedef struct delete_stmt
{
    expr *expr;
} delete_stmt;

typedef struct inc_stmt
{
    expr *operand;
    token_kind operator;
} inc_stmt;

struct stmt
{
    stmt_kind kind;
    source_pos pos;
    union
    {
        return_stmt return_stmt;
        if_else_stmt if_else;
        decl_stmt decl_stmt;
        for_stmt for_stmt;
        assign_stmt assign;
        expr *expr;
        stmt_block block;
        while_stmt while_stmt;
        while_stmt do_while_stmt;
        switch_stmt switch_stmt;
        delete_stmt delete;
        inc_stmt inc;
    };
};

typedef enum decl_kind
{
    DECL_STRUCT,
    DECL_UNION,
    DECL_VARIABLE,
    DECL_CONST,
    DECL_FUNCTION,
    DECL_ENUM
} decl_kind;

typedef struct aggregate_field
{
    const char *name;
    typespec *type;
    source_pos pos;
} aggregate_field;

typedef struct aggregate_decl
{
    aggregate_field *fields;
    size_t fields_count;
} aggregate_decl;

typedef struct variable_decl
{
    typespec *type; 
    expr *expr;
} variable_decl;

typedef struct const_decl
{
    expr *expr;
} const_decl;

typedef struct function_param
{
    const char *name;
    typespec *type;
    source_pos pos;
} function_param;

typedef struct function_param_list
{
    function_param *params;
    int param_count;
} function_param_list;

typedef struct function_decl
{
    function_param_list params;
    typespec *return_type;
    function_param *method_receiver;
    stmt_block stmts;
    bool is_extern;
} function_decl;

typedef struct enum_value enum_value;
struct enum_value
{
    const char *name;
    bool value_set;
    int64_t value;
    source_pos pos;
    enum_value *depending_on;
};

typedef struct enum_decl
{
    enum_value **values;
    size_t values_count;
} enum_decl;

struct decl
{
    const char *name;
    decl_kind kind;
    source_pos pos;
    type *resolved_type;
    union
    {
        function_decl function;
        variable_decl variable;
        aggregate_decl aggregate;
        enum_decl enum_decl;
        const_decl const_decl;
    };
};

expr *push_int_expr(source_pos pos, int64_t value)
{
    expr *result = push_struct(arena, expr);
    result->kind = EXPR_INT;
    result->integer_value = value;
    result->pos = pos;
    return result;
}

expr *push_float_expr(source_pos pos, double value)
{
    expr *result = push_struct(arena, expr);
    result->kind = EXPR_FLOAT;
    result->float_value = value;
    result->pos = pos;
    return result;
}

expr *push_name_expr(source_pos pos, const char *name)
{
    expr *result = push_struct(arena, expr);
    result->kind = EXPR_NAME;
    result->name = name;
    result->pos = pos;
    return result;
}

expr *push_new_expr(source_pos pos, typespec *t)
{
    expr *result = push_struct(arena, expr);
    result->kind = EXPR_NEW;
    result->new.type = t;
    result->pos = pos;
    return result;
}

expr *push_auto_expr(source_pos pos, typespec *t)
{
    expr *result = push_struct(arena, expr);
    result->kind = EXPR_AUTO;
    result->auto_new.type = t;
    result->pos = pos;
    return result;
}

expr *push_size_of_expr(source_pos pos, expr *e)
{
    expr *result = push_struct(arena, expr);
    result->kind = EXPR_SIZE_OF;
    result->size_of.expr = e;
    result->pos = pos;
    return result;
}

expr *push_size_of_type_expr(source_pos pos, typespec *t)
{
    expr *result = push_struct(arena, expr);
    result->kind = EXPR_SIZE_OF_TYPE;
    result->size_of_type.type = t;
    result->pos = pos;
    return result;
}

expr *push_cast_expr(source_pos pos, typespec *t, expr *e)
{
    expr *result = push_struct(arena, expr);
    result->kind = EXPR_CAST;
    result->cast.type = t;
    result->cast.expr = e;
    result->pos = pos;
    return result;
}

expr *push_string_expr(source_pos pos, const char *str_value)
{
    expr *result = push_struct(arena, expr);
    result->kind = EXPR_STRING;
    result->string_value = str_value;
    result->pos = pos;
    return result;
}

expr *push_char_expr(source_pos pos, const char *char_value)
{
    expr *result = push_struct(arena, expr);
    result->kind = EXPR_CHAR;
    result->string_value = char_value;
    result->pos = pos;
    return result;
}

expr *push_null_expr(source_pos pos)
{
    expr *result = push_struct(arena, expr);
    result->kind = EXPR_NULL;
    result->pos = pos;
    return result;
}

expr *push_bool_expr(source_pos pos, bool value)
{
    expr *result = push_struct(arena, expr);
    result->kind = EXPR_BOOL;
    result->bool_value = value;
    result->pos = pos;
    return result;
}

expr *push_unary_expr(source_pos pos, token_kind operator, expr *operand)
{
    expr *result = push_struct(arena, expr);
    result->kind = EXPR_UNARY;
    result->unary.operator = operator;
    result->unary.operand = operand;
    result->pos = pos;
    return result;
}

expr *push_binary_expr(source_pos pos, token_kind operator, expr *left, expr *right)
{
    expr *result = push_struct(arena, expr);
    result->kind = EXPR_BINARY;
    result->binary.operator = operator;
    result->binary.left = left;
    result->binary.right = right;
    result->pos = pos;
    return result;
}

typespec *push_typespec_name(source_pos pos, const char *name)
{
    typespec *result = push_struct(arena, typespec);
    result->kind = TYPESPEC_NAME;
    result->name = name;
    result->pos = pos;
    return result;
}
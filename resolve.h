#pragma once

typedef struct symbol symbol;
typedef struct type type;

typedef enum type_kind
{
    TYPE_NONE,
    TYPE_INCOMPLETE,
    TYPE_COMPLETING,
    TYPE_COMPLETED,
    TYPE_VOID,
    
    // integer types
    TYPE_INT,
    TYPE_LONG,
    TYPE_UINT,
    TYPE_ULONG,
    FIRST_INTEGER_TYPE = TYPE_INT,
    LAST_FIRST_INTEGER_TYPE = TYPE_ULONG,

    TYPE_FLOAT,
    
    TYPE_CHAR,
    TYPE_BOOL,
    TYPE_NULL,
    TYPE_STRUCT,
    TYPE_UNION,
    TYPE_ARRAY,
    TYPE_LIST,
    TYPE_POINTER,
    TYPE_FUNCTION,
    TYPE_ENUM
} type_kind;

typedef struct type_array
{
    type *base_type;
    size_t size;
} type_array;

typedef struct type_list
{
    type *base_type;
} type_list;

typedef struct type_pointer
{
    type *base_type;
} type_pointer;

typedef struct type_function
{
    type *receiver_type;
    type *return_type;
    type **param_types;
    size_t param_count;
    bool is_extern;
    bool has_variadic_arg;
} type_function;

typedef struct type_aggregate_field
{
    const char *name;
    type *type;
    size_t offset;
} type_aggregate_field;

typedef struct type_aggregate
{
    type_aggregate_field **fields;
    size_t fields_count;
} type_aggregate;

typedef struct type_enum
{
    hashmap values;
} type_enum;

struct type
{
    type_kind kind;
    symbol *symbol; // używane tylko przy typach, które muszą być completed
    size_t size;
    //size_t align;
    union
    {
        type_array array;
        type_list list;
        type_pointer pointer;
        type_function function;
        type_aggregate aggregate;
        type_enum enumeration;
    };
};

bool is_integer_type(type *t)
{
    bool result = (t != null && (t->kind >= FIRST_INTEGER_TYPE && t->kind <= LAST_FIRST_INTEGER_TYPE));
    return result;
}

bool is_numeric_type(type *t)
{
    bool result = (t != null && (is_integer_type(t) || t->kind == TYPE_FLOAT));
    return result;
}

typedef enum symbol_kind
{
    SYMBOL_NONE,
    SYMBOL_VARIABLE,
    SYMBOL_CONST,
    SYMBOL_FUNCTION,
    SYMBOL_TYPE,
} symbol_kind;

typedef enum symbol_state
{
    SYMBOL_UNRESOLVED,
    SYMBOL_RESOLVING,
    SYMBOL_RESOLVED,
} symbol_state;

typedef struct symbol symbol;
struct symbol
{
    const char *name;
    const char *mangled_name; // w przypadku funkcji
    symbol_kind kind;
    symbol_state state;
    decl *decl;
    type *type;
    union
    {
        int64_t val;
        symbol *next_overload;
    };
};

typedef struct resolved_expr
{
    type *type;
    bool is_lvalue;
    bool is_const;
    int64_t val;
} resolved_expr;
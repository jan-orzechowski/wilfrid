typedef struct symbol symbol;
typedef struct type type;

typedef enum type_kind
{
    TYPE_NONE,
    TYPE_INCOMPLETE,
    TYPE_COMPLETING,
    TYPE_COMPLETED,
    TYPE_VOID,
    
    TYPE_CHAR,
    TYPE_INT,
    TYPE_LONG,
    TYPE_UINT,
    TYPE_ULONG,
    FIRST_INTEGER_TYPE = TYPE_CHAR,
    LAST_INTEGER_TYPE = TYPE_ULONG,

    TYPE_FLOAT,
    FIRST_NUMERIC_TYPE = FIRST_INTEGER_TYPE,
    LAST_NUMERIC_TYPE = TYPE_FLOAT,
   
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
    size_t align;
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
    assert(t);
    bool result = (t->kind >= FIRST_INTEGER_TYPE && t->kind <= LAST_INTEGER_TYPE);
    return result;
}

bool is_numeric_type(type *t)
{
    assert(t);
    bool result = (t->kind >= FIRST_NUMERIC_TYPE && t->kind <= LAST_NUMERIC_TYPE);
    return result;
}

bool is_unsigned_type(type *t)
{
    assert(t);
    bool result = (t->kind == TYPE_CHAR || t->kind == TYPE_UINT || t->kind == TYPE_ULONG);
    return result;
}

bool is_signed_type(type *t)
{
    assert(t);
    bool result = (t->kind == TYPE_INT || t->kind == TYPE_LONG);
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

const size_t POINTER_SIZE = 8;
const size_t POINTER_ALIGN = 8;

type *type_void =    &(type) { .kind = TYPE_VOID,    .size = 1, .align = 0 };
type *type_null =    &(type) { .kind = TYPE_NULL,    .size = 8, .align = 0 };
type *type_char =    &(type) { .kind = TYPE_CHAR,    .size = 1, .align = 1 };
type *type_int =     &(type) { .kind = TYPE_INT,     .size = 4, .align = 4 };
type *type_uint =    &(type) { .kind = TYPE_UINT,    .size = 4, .align = 4 };
type *type_long =    &(type) { .kind = TYPE_LONG,    .size = 8, .align = 8 };
type *type_ulong =   &(type) { .kind = TYPE_ULONG,   .size = 8, .align = 8 };
type *type_float =   &(type) { .kind = TYPE_FLOAT,   .size = 4, .align = 4 };
type *type_bool =    &(type) { .kind = TYPE_BOOL,    .size = 4, .align = 4 };
type *type_invalid = &(type) { .kind = TYPE_NONE,    .size = 0, .align = 0 };

type *get_new_type(type_kind kind)
{
    type *t = push_struct(arena, type);
    t->kind = kind;
    return t;
}

bool compare_types(type *a, type *b)
{
    assert(a);
    assert(b);

    if (a == b)
    {
        return true;
    }

    if ((a == type_long && b->kind == TYPE_ENUM)
        || (b == type_long && a->kind == TYPE_ENUM))
    {
        return true;
    }

    if ((a == type_null && b->kind == TYPE_POINTER)
        || (b == type_null && a->kind == TYPE_POINTER))
    {
        return true;
    }

    if ((a == type_ulong && b->kind == TYPE_POINTER)
        || (b == type_ulong && a->kind == TYPE_POINTER))
    {
        return true;
    }

    if (a->kind != b->kind)
    {
        return false;
    }

    // teraz musimy sprawdzić tylko jeden kind
    if (a->kind == TYPE_LIST)
    {
        bool result = compare_types(a->list.base_type, b->list.base_type);
        return result;
    }

    if (a->kind == TYPE_ARRAY)
    {
        bool result = (a->array.size == b->array.size
            && compare_types(a->array.base_type, b->array.base_type));
        return result;
    }

    if (a->kind == TYPE_POINTER)
    {
        bool result = compare_types(a->pointer.base_type, b->pointer.base_type);
        return result;
    }

    return false;
}

size_t get_type_size(type *type)
{
    assert(type);
    assert(type->kind > TYPE_COMPLETING);
    assert(type->size != 0);
    return type->size;
}

size_t get_type_align(type *type)
{
    assert(type->kind > TYPE_COMPLETING);
    assert(is_power_of_2(type->align));
    return type->align;
}

chained_hashmap cached_pointer_types;

type *get_pointer_type(type *base_type)
{
    if (base_type == null)
    {
        return type_invalid;
    }
    
    type *ptr_type = map_chain_get(&cached_pointer_types, base_type);
    if (ptr_type)
    {
        return ptr_type;
    }

    type *type = get_new_type(TYPE_POINTER);
    type->size = POINTER_SIZE;
    type->align = POINTER_ALIGN;
    type->pointer.base_type = base_type;
    map_chain_put(&cached_pointer_types, base_type, type);
    return type;
}

type *get_function_type(type **param_types, size_t param_types_count, type *return_type)
{
    type *type = get_new_type(TYPE_FUNCTION);
    type->size = POINTER_SIZE;
    type->align = POINTER_ALIGN;
    type->function.param_types = param_types;
    type->function.param_count = param_types_count;
    type->function.return_type = return_type;
    return type;
}

symbol *get_new_symbol(symbol_kind kind, const char *name, decl *decl)
{
    symbol *sym = push_struct(arena, symbol);
    sym->kind = kind;
    sym->name = name;
    sym->decl = decl;
    return sym;
}

resolved_expr *get_resolved_rvalue_expr(type *t)
{
    assert(t);
    resolved_expr *result = push_struct(arena, resolved_expr);
    result->type = t;
    return result;
}

resolved_expr *get_resolved_lvalue_expr(type *t)
{
    assert(t);
    assert(t->kind != SYMBOL_CONST);
    resolved_expr *result = push_struct(arena, resolved_expr);
    result->type = t;
    result->is_lvalue = true;
    return result;
}

resolved_expr *get_resolved_const_expr(int64_t val)
{
    resolved_expr *result = push_struct(arena, resolved_expr);
    result->type = type_long;
    result->is_const = true;
    result->val = val;
    return result;
}

size_t get_field_offset(type *aggregate, char *field_name)
{
    assert_is_interned(field_name);
    assert(aggregate->kind == TYPE_STRUCT || aggregate->kind == TYPE_UNION);

    for (size_t i = 0; i < aggregate->aggregate.fields_count; i++)
    {
        type_aggregate_field *f = aggregate->aggregate.fields[i];
        if (f->name == field_name)
        {
            return f->offset;
        }
    }

    fatal("Unknown field after resolve!");
    return 0;
}

type *get_field_type(type *aggregate, char *field_name)
{
    assert_is_interned(field_name);
    assert(aggregate->kind == TYPE_STRUCT || aggregate->kind == TYPE_UNION);

    for (size_t i = 0; i < aggregate->aggregate.fields_count; i++)
    {
        type_aggregate_field *f = aggregate->aggregate.fields[i];
        if (f->name == field_name)
        {
            return f->type;
        }
    }

    fatal("Unknown field after resolve!");
    return null;
}

type *get_field_type_by_index(type *aggregate, size_t field_index)
{
    assert(aggregate->kind == TYPE_STRUCT || aggregate->kind == TYPE_UNION);
    assert(field_index < aggregate->aggregate.fields_count);
    return aggregate->aggregate.fields[field_index]->type;
}

size_t get_field_offset_by_index(type *aggr_type, size_t field_index)
{
    assert(aggr_type);
    assert(aggr_type->kind == TYPE_STRUCT || aggr_type->kind == TYPE_UNION);

    assert(field_index < aggr_type->aggregate.fields_count);
    type_aggregate_field *f = aggr_type->aggregate.fields[field_index];
    return f->offset;
}

size_t get_array_index_offset(type *arr_type, size_t element_index)
{
    assert(arr_type);
    assert(arr_type->kind == TYPE_ARRAY || arr_type->kind == TYPE_POINTER);

    if (arr_type->kind == TYPE_ARRAY)
    {
        assert(element_index < arr_type->array.size);
        size_t result = get_type_size(arr_type->array.base_type) * element_index;
        return result;
    }
    else
    {
        assert(arr_type->pointer.base_type->kind != TYPE_ARRAY);
        size_t result = get_type_size(arr_type->pointer.base_type) * element_index;
        return result;
    }
}

void plug_expr(expr *original_expr, expr *new_expr)
{
    expr temp = *original_expr;
    (*original_expr) = *new_expr;
    *new_expr = temp;
}

void plug_stub_expr(expr *original_expr, stub_expr_kind kind, type *resolved_type)
{
    expr *new_expr = push_struct(arena, expr);
    new_expr->kind = EXPR_STUB;
    new_expr->pos = original_expr->pos;
    new_expr->stub.kind = kind;
    new_expr->resolved_type = resolved_type;

    plug_expr(original_expr, new_expr);

    // tutaj nazwy są odwrócone; efekt jest taki, że stub ma teraz odniesienie do starego wyrażenia
    original_expr->stub.original_expr = new_expr;

    assert(original_expr->kind == EXPR_STUB);
}
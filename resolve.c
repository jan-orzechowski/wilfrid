#include "lexing.h"
#include "parsing.h"
#include "utils.h"

typedef struct symbol symbol;
typedef struct type type;

typedef enum type_kind
{
    TYPE_NONE,
    TYPE_INCOMPLETE,
    TYPE_COMPLETING,
    TYPE_COMPLETED,
    TYPE_INT,
    TYPE_CHAR,
    TYPE_FLOAT,
    TYPE_STRUCT,
    TYPE_UNION,
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

typedef struct type_aggregate_field
{
    char* name;
    type* type;
} type_aggregate_field;

typedef struct type_aggregate
{
    type_aggregate_field** fields;
    size_t fields_count;
} type_aggregate;

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
        type_aggregate aggregate;
    };
};

typedef enum symbol_kind
{
    SYMBOL_NONE,
    SYMBOL_VARIABLE,
    SYMBOL_CONST,
    SYMBOL_FUNC,
    SYMBOL_TYPE,
    SYMBOL_ENUM_CONST,
} symbol_kind;

typedef enum symbol_state
{
    SYMBOL_UNRESOLVED,
    SYMBOL_RESOLVING,
    SYMBOL_RESOLVED,
} symbol_state;

typedef struct symbol
{
    const char* name;
    symbol_kind kind;
    symbol_state state;
    decl* decl;
    type* type;
    int64_t val;
} symbol;


size_t get_type_size(type* type)
{
    assert(type->kind > TYPE_COMPLETING);
    assert(type->size != 0);
    return type->size;
}

size_t get_type_align(type* type)
{
    assert(type->kind > TYPE_COMPLETING);
    assert(is_power_of_2(type->align));
    return type->align;
}

type* type_float = &(type) { .name = "float", .kind = TYPE_FLOAT, .size = 4, .align = 4 };
type* type_int =   &(type) { .name = "int",   .kind = TYPE_INT, .size = 4, .align = 4 };
type* type_char =  &(type) { .name = "char",  .kind = TYPE_CHAR, .size = 1, .align = 1 };

const size_t POINTER_SIZE = 8;
const size_t POINTER_ALIGN = 8;

symbol** symbols;
symbol** ordered_symbols;

void resolve_symbol(symbol* s);
type* resolve_typespec(typespec* t);

symbol* get_symbol(char* name)
{
    symbol* result = 0;
    // do zastąpienia hash table
    for (symbol** it = symbols; it < buf_end(symbols); it++)
    {
        symbol* sym = *it;
        if (sym->name == name)
        {
            result = sym;
            break;
        }
    }
    return result;
}

type* get_new_type(type_kind kind)
{
    type* t = xcalloc(sizeof(type));
    t->kind = kind;
    return t;
}

void get_complete_struct_type(type* type, type_aggregate_field* fields, size_t fields_count)
{
    assert(type->kind == TYPE_COMPLETING);
    type->kind = TYPE_STRUCT;
    type->size = 0;
    type->align = 0;

    for (type_aggregate_field* it = fields; it != fields + fields_count; it++)
    {
        type->size = get_type_size(it->type) + align_up(type->size, get_type_align(it->type));
        type->align = max(type->align, get_type_align(it->type));
    }

    type->aggregate.fields = xmempcy(fields, fields_count * sizeof(*fields));
    type->aggregate.fields_count = fields_count;
}

void get_complete_union_type(type* type, type_aggregate_field* fields, size_t fields_count)
{
    assert(type->kind == TYPE_COMPLETING);
    type->kind = TYPE_UNION;
    type->size = 0;
    type->align = 0;

    for (type_aggregate_field* it = fields; it != fields + fields_count; it++)
    {
        assert(it->type->kind > TYPE_COMPLETING);
        type->size = max(type->size, get_type_size(it->type));
        type->align = max(type->align, get_type_align(it->type));
    }
    
    type->aggregate.fields = xmempcy(fields, fields_count * sizeof(*fields));
    type->aggregate.fields_count = fields_count;
}

type* get_incomplete_type(symbol* sym)
{
    type* type = get_new_type(TYPE_INCOMPLETE);
    type->symbol = sym;
    return type;
}

type* get_pointer_type(type* base_type)
{
    type* type = get_new_type(TYPE_POINTER);
    type->size = POINTER_SIZE;
    type->align = POINTER_ALIGN;
    type->pointer.base_type = base_type;
    return type;
}

symbol* get_new_symbol(symbol_kind kind, char* name, decl* decl)
{
    symbol* sym = xcalloc(sizeof(symbol));
    sym->kind = kind;
    sym->name = name;
    sym->decl = decl;
    return sym;
}

symbol* get_symbol_from_decl(decl* d)
{
    symbol_kind kind = SYMBOL_NONE;
    switch (d->kind)
    {
        case DECL_STRUCT:
        case DECL_UNION:
        case DECL_TYPEDEF:
        case DECL_ENUM:
        {
            kind = SYMBOL_TYPE;
        }
        break;
        case DECL_VARIABLE:
        {
            kind = SYMBOL_VARIABLE;
        }
        break;
        default:
        {
            assert("invalid default case");
        }
        break;
    }
    symbol* sym = get_new_symbol(kind, d->identifier, d);
    if (d->kind == DECL_STRUCT || d->kind == DECL_UNION)
        // te rodzaje deklaracji są rozstrzygnięte od razu, gdy je napotykamy
        // natomiast sam typ jest jeszcze incomplete
    {
        sym->state = SYMBOL_RESOLVED;
        sym->type = get_incomplete_type(sym);
    }
    return sym;
}

symbol* push_installed_symbol(char* name, type* type)
{
    symbol* sym = get_new_symbol(SYMBOL_TYPE, name, NULL);
    sym->state = SYMBOL_RESOLVED;
    sym->type = type;
    buf_push(symbols, sym);
    return sym;
}

void push_symbol_from_decl(decl* d)
{
    symbol* s = get_symbol_from_decl(d);
    buf_push(symbols, s);
}

void complete_type(type* t)
{
    if (t->kind == TYPE_COMPLETING)
    {
        fatal("detected type completion cycle");
        return;
    }
    else if (t->kind != TYPE_INCOMPLETE)
    {
        return;
    }
    
    t->kind = TYPE_COMPLETING;
    decl* d = t->symbol->decl;

    // pozostałe typy są kompletne od razu
    assert(d->kind == DECL_STRUCT || d->kind == DECL_UNION);
    
    // idziemy po kolei po polach
    type_aggregate_field* fields = NULL;
    for (size_t i = 0; i < d->aggregate_declaration.fields_count; i++)
    {
        aggregate_field field = d->aggregate_declaration.fields[i];
        type* field_type = resolve_typespec(field.type);
        complete_type(field_type); // wszystkie muszą być completed, ponieważ musimy znać ich rozmiar

        buf_push(fields, ((type_aggregate_field){ field.identifier, field_type }));
    }

    if (buf_len(fields) == 0)
    {
        fatal("Struct/union has no fields");
    }

    if (d->kind == DECL_STRUCT)
    {
        get_complete_struct_type(t, fields, buf_len(fields));
    }
    else
    {
        assert(d->kind == DECL_UNION);
        get_complete_union_type(t, fields, buf_len(fields));
    }

    buf_push(ordered_symbols, t->symbol);
}

symbol* resolve_name(char* name)
{    
    symbol* s = get_symbol(name);
    if (!s)
    {
        fatal("non-existent name");
        return 0;
    }
    resolve_symbol(s);
    return s;
}

type* resolve_typespec(typespec* t)
{
    type* result = 0;
    if (t)
    {
        switch (t->kind)
        {
            case TYPESPEC_NAME:
            {
                symbol* sym = resolve_name(t->name);
                if (sym->kind != SYMBOL_TYPE)
                {
                    fatal("%s must denote a type", t->name);
                    result = 0;
                }
                else
                {
                    result = sym->type;
                }
            }
            break;
            case TYPESPEC_ARRAY:
            {
                // musimy jeszcze wziąć rozmiar
                result = resolve_typespec(t->array.base_type);
            }
            break;
            case TYPESPEC_POINTER:
            {
                type* base_type = resolve_typespec(t->pointer.base_type);
                result = get_pointer_type(base_type);
            }
            break;
            case TYPESPEC_FUNCTION:
            {
                //result = resolve_typespec(t->function.returned_type);
            }
            break;
        }
    }
    return result;
}

type* resolve_expression(expr* e)
{
    type* result = 0;
    switch (e->kind)
    {
        case EXPR_NAME:
        {
            result = resolve_name(e->identifier);
        }
        break;
        case EXPR_INT:
        {
            debug_breakpoint;
        }
        break;
        default:
        {
            fatal("expr kind not yet supported");
        }
        break;
    }
    return result;
}

type* resolve_variable(decl* d)
type* resolve_variable_decl(decl* d)
{
    type* result = 0;
    
    // musi być albo typ, albo wyrażenie
    // mogą być oba, ale wtedy muszą się zgadzać

    if (d->variable_declaration.type)
    {
        result = resolve_typespec(d->variable_declaration.type);
    }

    if (d->variable_declaration.expression)
    {
        result = resolve_expression(d->variable_declaration.expression);
    }

    return result;
}

type* resolve_type_decl(decl* d)
{
    assert(d->kind == DECL_TYPEDEF); // jedyny rodzaj, jaki tu trzeba obsłużyć
    // unions i structs są resolved od razu
    type* result = resolve_typespec(d->typedef_declaration.type);
    return result;
}

void resolve_symbol(symbol* s)
{
    if (s->state == SYMBOL_RESOLVED)
    {
        return;
    }
    else if (s->state == SYMBOL_RESOLVING)
    {
        fatal("cyclic dependency detected");
        return;
    }

    assert(s->state == SYMBOL_UNRESOLVED);
    s->state = SYMBOL_RESOLVING;

    assert(s->decl);
    switch (s->kind)
    {
        case SYMBOL_VARIABLE:
        {
            s->type = resolve_variable_decl(s->decl);
        }
        break;
        case SYMBOL_TYPE:
        {            
            s->type = resolve_type_decl(s->decl);
        }
        break;
    }

    s->state = SYMBOL_RESOLVED;
    buf_push(ordered_symbols, s);
}

void complete_symbol(symbol* sym)
{
    resolve_symbol(sym);
    if (sym->kind == SYMBOL_TYPE)
    {
        complete_type(sym->type);
    }
}

void resolve_test(void)
{
    arena = allocate_memory_arena(megabytes(50));
    
    size_t installed_count = 3;
    push_installed_symbol(str_intern("char"), type_char);
    push_installed_symbol(str_intern("int"), type_int);
    push_installed_symbol(str_intern("float"), type_int);

    char* test_strs[] = {
        "let x: int = y",
        "let y: int = 1",
        "struct Z { s: S, t: T* }",
        "struct S { i: int, c: char }",
        "struct T { i: int }"
    };

    for (size_t i = 0; i < sizeof(test_strs)/sizeof(test_strs[0]); i++)
    {
        char* str = test_strs[i];
        decl* d = parse_decl(str);        
        push_symbol_from_decl(d);
    }

    for (symbol** it = symbols + installed_count; 
        it != buf_end(symbols); 
        it++)
    {
        symbol* sym = *it;        
        complete_symbol(sym);
    }
   
    for (symbol** it = ordered_symbols; it != buf_end(ordered_symbols); it++)
    {
        symbol* sym = *it;
        if (sym->decl)
        {
            print_declaration(sym->decl);
        }
        else
        {
            printf("%s", sym->name);
        }
        printf("\n");
    }

    debug_breakpoint;

    free_memory_arena(arena);
}





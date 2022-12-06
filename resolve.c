#include "lexing.h"
#include "parsing.h"

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

symbol** symbols;
symbol** ordered_symbols;

void resolve_symbol(symbol* s);

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

type* get_incomplete_type(symbol* sym)
{
    type* type = get_new_type(TYPE_INCOMPLETE);
    type->symbol = sym;
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
        // natomiast samy typ jest jeszcze incomplete
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

void push_symbol(decl* d)
{
    symbol* s = get_symbol_from_decl(d);
    buf_push(symbols, s);
}

void complete_type(type* t)
{

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

type* resolve_type(type* t)
{
    if (t)
    {
        switch (t->kind)
        {
            case TYPE_INT:
            {

            }
            break;
            case TYPE_FLOAT:
            {

            }
            break;
            case TYPE_CHAR:
            {

            }
            break;
            case TYPE_NAME:
            {
                resolve_name(t->name);
            }
            break;
            case TYPE_ARRAY:
            {

            }
            break;
            case TYPE_POINTER:
            {

            }
            break;
            case TYPE_FUNCTION:
            {

            }
            break;
        }
    }
    return t;
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
{
    type* result = 0;
    
    if (d->variable_declaration.type)
    {
        result = resolve_type(d->variable_declaration.type);
    }

    if (d->variable_declaration.expression)
    {
        result = resolve_expression(d->variable_declaration.expression);
    }

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
            s->type = resolve_variable(s->decl);
        }
        break;
        case SYMBOL_TYPE:
        {
            s->type = resolve_type(s->decl);
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

type* type_float = &(type) { .name = "float", .kind = TYPE_FLOAT };
type* type_int =   &(type) { .name = "int",   .kind = TYPE_INT };
type* type_char =  &(type) { .name = "char",  .kind = TYPE_CHAR };

void resolve_test(void)
{
    arena = allocate_memory_arena(megabytes(50));
    
    size_t installed_count = 3;
    push_installed_symbol(str_intern("char"), type_char);
    push_installed_symbol(str_intern("int"), type_int);
    push_installed_symbol(str_intern("float"), type_int);

    char* test_strs[] = {
        "let x: int = y",
        "let y: int = 1"
    };

    for (size_t i = 0; i < sizeof(test_strs)/sizeof(test_strs[0]); i++)
    {
        char* str = test_strs[i];
        decl* d = parse_decl(str);        
        push_symbol(d);
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





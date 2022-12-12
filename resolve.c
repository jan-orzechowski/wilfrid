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
    TYPE_VOID,
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
    size_t size;
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
    symbol* symbol; // używane tylko przy typach, które muszą być completed
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
    SYMBOL_FUNCTION,
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

typedef struct resolved_expr
{
    type* type;
    bool is_lvalue;
    bool is_const;
    int64_t val;
} resolved_expr;

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

type* type_void =  &(type) { .name = "void",  .kind = TYPE_VOID, .size = 0, .align = 0 };
type* type_char =  &(type) { .name = "char",  .kind = TYPE_CHAR, .size = 1, .align = 1 };
type* type_int =   &(type) { .name = "int",   .kind = TYPE_INT, .size = 4, .align = 4 };
type* type_float = &(type) { .name = "float", .kind = TYPE_FLOAT, .size = 4, .align = 4 };

const size_t POINTER_SIZE = 8;
const size_t POINTER_ALIGN = 8;

symbol** symbols;
symbol** ordered_symbols;

void complete_type(type* t);
void resolve_symbol(symbol* s);
type* resolve_typespec(typespec* t);
resolved_expr* resolve_expression(expr* e);

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

void get_complete_struct_type(type* type, type_aggregate_field** fields, size_t fields_count)
{
    assert(type->kind == TYPE_COMPLETING);
    type->kind = TYPE_STRUCT;
    type->size = 0;
    type->align = 0;

    for (type_aggregate_field** it = fields; it != fields + fields_count; it++)
    {
        type_aggregate_field* field = *it;
        type->size = get_type_size(field->type) + align_up(type->size, get_type_align(field->type));
        type->align = max(type->align, get_type_align(field->type));
    }

    type->aggregate.fields = xmempcy(fields, fields_count * sizeof(*fields));
    type->aggregate.fields_count = fields_count;
}

void get_complete_union_type(type* type, type_aggregate_field** fields, size_t fields_count)
{
    assert(type->kind == TYPE_COMPLETING);
    type->kind = TYPE_UNION;
    type->size = 0;
    type->align = 0;

    for (type_aggregate_field** it = fields; it != fields + fields_count; it++)
    {
        type_aggregate_field* field = *it;
        assert(field->type->kind > TYPE_COMPLETING);
        type->size = max(type->size, get_type_size(field->type));
        type->align = max(type->align, get_type_align(field->type));
    }
    
    type->aggregate.fields = xmempcy(fields, fields_count * sizeof(*fields));
    type->aggregate.fields_count = fields_count;
}

type* get_array_type(type* element, size_t size)
{
    complete_type(element);
    type* t = get_new_type(TYPE_ARRAY);
    t->size = size * get_type_size(element);
    t->align = get_type_align(element);
    t->array.base_type = element;
    t->array.size = size;
    return t; 
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

type* get_function_type(type** parameter_types, size_t parameter_types_count, type* return_type)
{
    type* type = get_new_type(TYPE_FUNCTION);
    type->size = POINTER_SIZE;
    type->align = POINTER_ALIGN;
    type->function.parameter_types = parameter_types;
    type->function.parameter_count = parameter_types_count;
    type->function.returned_type = return_type;
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

resolved_expr* get_resolved_rvalue_expr(type* t)
{
    assert(t);
    resolved_expr* result = xcalloc(sizeof(resolved_expr));
    result->type = t;
    return result;
}

resolved_expr* get_resolved_lvalue_expr(type* t)
{
    assert(t);
    assert(t->kind != SYMBOL_CONST);
    resolved_expr* result = xcalloc(sizeof(resolved_expr));
    result->type = t;
    result->is_lvalue = true;
    return result;
}

resolved_expr* get_resolved_const_expr(int64_t val)
{
    resolved_expr* result = xcalloc(sizeof(resolved_expr));
    result->type = type_int;
    result->is_const = true;
    result->val = val;
    return result;
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
        case DECL_CONST:
        {
            kind = SYMBOL_CONST;
        }
        break;
        case DECL_FUNCTION:
        {
            kind = SYMBOL_FUNCTION;
        } 
        break;
        default:
        {
            fatal("invalid default case");
        }
        break;
    }
    assert(kind != SYMBOL_NONE);

    symbol* sym = get_new_symbol(kind, d->name, d);
    if (d->kind == DECL_STRUCT || d->kind == DECL_UNION)
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
    type_aggregate_field** fields = NULL;
    for (size_t i = 0; i < d->aggregate_declaration.fields_count; i++)
    {
        aggregate_field field = d->aggregate_declaration.fields[i];
        type* field_type = resolve_typespec(field.type);
        complete_type(field_type); // wszystkie muszą być completed, ponieważ musimy znać ich rozmiar

        type_aggregate_field* type_field = xcalloc(sizeof(type_aggregate_field));
        type_field->name = field.name;
        type_field->type = field_type;
        buf_push(fields, type_field);
    }

    if (buf_len(fields) == 0)
    {
        fatal("struct/union has no fields");
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

                type* element = resolve_typespec(t->array.base_type);
                resolved_expr* size_expr = resolve_expression(t->array.size_expr);
                // teraz trzeba czegoś w rodzaju evalutate expression...

                result = get_array_type(element, size_expr->val);
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

int64_t eval_int_unary_op(token_kind op, int64_t val)
{
    switch (op)
    {
        case TOKEN_ADD: return +val;
        case TOKEN_SUB: return -val;
        case TOKEN_NOT: return !val;
        case TOKEN_BITWISE_NOT: return ~val;
        default: fatal("operation not implemented"); return 0;
    }
}

int64_t eval_int_binary_op(token_kind op, int64_t left, int64_t right)
{
    switch (op)
    {
        case TOKEN_ADD: return left + right;
        case TOKEN_SUB: return left - right;
        case TOKEN_MUL: return left * right;
        case TOKEN_DIV: return (right != 0) ? left / right : 0;
        case TOKEN_MOD: return (right != 0) ? left % right : 0;
        case TOKEN_BITWISE_AND: return left & right;
        case TOKEN_BITWISE_OR: return left | right;
        case TOKEN_LEFT_SHIFT: return left << right;
        case TOKEN_RIGHT_SHIFT: return left >> right;
        case TOKEN_XOR: return left ^ right;
        case TOKEN_EQ: return left == right;
        case TOKEN_NEQ: return left != right;
        case TOKEN_LT: return left < right;
        case TOKEN_LEQ: return left <= right;
        case TOKEN_GT: return left > right;
        case TOKEN_GEQ: return left >= right;
        case TOKEN_AND: return left && right;
        case TOKEN_OR: return left || right;
        default: fatal("operation not implemented"); return 0;
    }
}

resolved_expr* pointer_decay(resolved_expr* e)
{        
    if (e->type->kind == TYPE_ARRAY)
    {
        e = get_resolved_rvalue_expr(get_pointer_type(e->type->array.base_type));
    }
    return e;
}

resolved_expr* resolve_expr_unary(expr* expr)
{
    resolved_expr* result = 0;

    assert(expr->kind == EXPR_UNARY);
    resolved_expr* operand = resolve_expression(expr->unary_expr_value.operand);
    type* type = operand->type;
    switch (expr->unary_expr_value.operator)
    {
        case TOKEN_MUL:
        {
            operand = pointer_decay(operand);
            if (type->kind != TYPE_POINTER)
            {
                fatal("Cannot dereference non-pointer type");
            }
            result = get_resolved_lvalue_expr(type->pointer.base_type);
        }
        break;
        case TOKEN_BITWISE_AND:
        {
            if (false == operand->is_lvalue)
            {
                fatal("Cannot take address of non-lvalue");
            }
            result = get_resolved_rvalue_expr(get_pointer_type(type));
        }
        break;       
        default:
        {
            if (type->kind != TYPE_INT)
            {
                fatal("Can only use unary %s with ints", get_token_kind_name(expr->unary_expr_value.operator));
            }
            if (operand->is_const)
            {
                int64_t value = eval_int_unary_op(expr->unary_expr_value.operator, operand->val);
                result = get_resolved_const_expr(value);
            }
            else
            {
                result = get_resolved_rvalue_expr(type);
            }
        }
    }

    return result;
}

resolved_expr* resolve_expr_binary(expr* expr)
{
    resolved_expr* result = 0;

    assert(expr->kind == EXPR_BINARY);
    resolved_expr* left = resolve_expression(expr->binary_expr_value.left_operand);
    resolved_expr* right = resolve_expression(expr->binary_expr_value.right_operand);
    
    if (left->type != type_int)
    {
        fatal("left operand of + must be int");
    }
    if (right->type != left->type)
    {
        fatal("left and right operand of + must have same type");
    }

    if (left->is_const && right->is_const)
    {   
        int64_t const_value = eval_int_binary_op(expr->binary_expr_value.operator, left->val, right->val);
        result = get_resolved_const_expr(const_value);
    }
    else
    {
        result = get_resolved_rvalue_expr(left->type);
    }

    return result;
}

resolved_expr* resolve_expression(expr* e)
{
    resolved_expr* result = 0;
    switch (e->kind)
    {
        case EXPR_NAME:
        {
            symbol* sym = resolve_name(e->name);          
            if (sym->kind == SYMBOL_VARIABLE)
            {
                result = get_resolved_lvalue_expr(sym->type);
            }
            else if (sym->kind == SYMBOL_CONST)
            {
                result = get_resolved_rvalue_expr(sym->type);
            }
            else if (sym->kind == SYMBOL_FUNCTION)
            {                  
               result = get_resolved_lvalue_expr(sym->type);
               
               debug_breakpoint;
            }
            else
            {
                fatal("must be a variable name!");
            } 
        }
        break;
        case EXPR_INT:
        {
            result = get_resolved_rvalue_expr(type_int);
            result->is_const = true;
            result->val = e->number_value;
        }
        break;
        case EXPR_CALL:
        {
            resolved_expr* fn_expr = resolve_expression(e->call_expr_value.function_expr);
            size_t arg_param_count = fn_expr->type->function.parameter_count;
            if (arg_param_count == e->call_expr_value.args_num)
            {
                for (size_t i = 0; i < arg_param_count; i++)
                {
                    expr* arg_expr = e->call_expr_value.args[i];
                    resolved_expr* resolved_arg_expr = resolve_expression(arg_expr);

                    type* param_type = fn_expr->type->function.parameter_types[i];
                    
                    if (param_type != resolved_arg_expr->type)
                    {
                        debug_breakpoint;
                        fatal("argument has invalid type");
                    }
                }
            }
            else
            {
                fatal("invalid number of arguments");
            }

            debug_breakpoint;
        }
        break;
        case EXPR_BINARY:
        {
            result = resolve_expr_binary(e);
        }
        break;
        case EXPR_UNARY:
        {
            result = resolve_expr_unary(e);
        }
        break;
        case EXPR_TERNARY:
        {
            fatal("ternary expressions not allowed as constants");
        }
        break;
        case EXPR_SIZEOF:
        {
            // może być albo typ, albo całe wyrażenie
            resolved_expr* expr = resolve_expression(e->sizeof_expr_value.expr);
            type* expr_type = expr->type;
            complete_type(expr_type);
            int64_t size = get_type_size(expr_type);
            result = get_resolved_const_expr(size);
            debug_breakpoint;
        }
        break;
        case EXPR_FIELD:
        {           
            resolved_expr* aggregate_expr = resolve_expression(e->field_expr_value.expr);
            char* field_name = str_intern(e->field_expr_value.field_name);
            type* t = aggregate_expr->type;
            complete_type(t);

            type* found = 0;
            for (size_t i = 0; i < t->aggregate.fields_count; i++)
            {
                type_aggregate_field* f = t->aggregate.fields[i];
                if (field_name == f->name)
                {
                    found = f->type;
                    break;
                }                
            }
            if (!found)
            {                
                fatal("no field of name: %s", field_name);
            }

            result = get_resolved_lvalue_expr(found);

            debug_breakpoint;
        }
        break;
        case EXPR_INDEX:
        {
            debug_breakpoint;

            resolved_expr* operand_expr = resolve_expression(e->index_expr_value.array_expr);
            if (pointer_decay(operand_expr)->type->kind != TYPE_POINTER)
            {
                fatal("can only index arrays or pointers");
            }
            
            resolved_expr* index_expr = resolve_expression(e->index_expr_value.index_expr);


            if (index_expr->type->kind != TYPE_INT)
            {
                fatal("index must be an integer");
            }

            result = get_resolved_lvalue_expr(operand_expr->type->pointer.base_type);
        }
        break;
        case EXPR_COMPOUND_LITERAL:
        {
            for (size_t i = 0; i < e->compound_literal_expr_value.fields_count; i++)
            {
                compound_literal_field* field = e->compound_literal_expr_value.fields[i];
                resolved_expr* field_expr = resolve_expression(field->expr);

                // co dalej z tym robić?
            }
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

resolved_expr resolve_expected_expr(expr* e, type* expected_type)
{
    resolved_expr result = {0};

    return result;
}

type* resolve_const_decl(decl* d)
{
    type* result = 0;
    assert(d->const_declaration.expression);
    resolved_expr* expr = resolve_expression(d->const_declaration.expression);
    if (expr)
    {
        result = expr->type;
    }
    return result;
}

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
        resolved_expr* expr = resolve_expression(d->variable_declaration.expression);
        if (expr)
        {
            result = expr->type;
        }
    }

    return result;
}

type* resolve_type_decl(decl* d)
{
    assert(d->kind == DECL_TYPEDEF); 
    // jedyny rodzaj, jaki tu trzeba obsłużyć
    // unions i structs są resolved od razu
    type* result = resolve_typespec(d->typedef_declaration.type);
    return result;
}

type* resolve_function_decl(decl* d)
{
    assert(d->kind == DECL_FUNCTION);

    type* resolved_return_type = type_void;
    if (d->function_declaration.return_type)
    {
        resolved_return_type = resolve_typespec(d->function_declaration.return_type);
    }
    
    type** resolved_args = 0;
    function_param_list* args = &d->function_declaration.parameters;
    for (size_t i = 0; i < args->param_count; i++)
    {
        function_param* p = &args->params[i];
        // nazwy argumentów na razie nas nie obchodzą
        type* t = resolve_typespec(p->type);
        buf_push(resolved_args, t);
    }

    type* result = get_function_type(resolved_args, buf_len(resolved_args), resolved_return_type);
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
        case SYMBOL_CONST:
        {
            s->type = resolve_const_decl(s->decl);
        }
        break;
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
        case SYMBOL_FUNCTION:
        {
            s->type = resolve_function_decl(s->decl);
        }
        break;
        default:
        {
            fatal("unimplemented symbol type");
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

typedef struct test
{
    int i;
    int j;
} test;

test t;

const int size = sizeof(t.i);

void resolve_test(void)
{
    arena = allocate_memory_arena(megabytes(50));
    
    size_t installed_count = 3;
    push_installed_symbol(str_intern("void"), type_void);
    push_installed_symbol(str_intern("char"), type_char);
    push_installed_symbol(str_intern("int"), type_int);
    push_installed_symbol(str_intern("float"), type_float);

    char* test_strs[] = {
        "let f := g[1 + 3]",
        "let g: int[10]",
        "let e = *b",
        "let d = b[0]",
        "let c: int[4]",
        "let b = &a[0]",
        "let x: int = y",
        "let y: int = 1",
        "let i = n+m",
        "let z: Z",
        "const m = sizeof(z.t)",
        "const n = sizeof(&a[0])",
        "struct Z { s: S, t: T* }",
        "struct T { i: int }",
        "let a: int[3] = {1, 2, 3}",
        "struct S { t: int, c: char }",
        "let x = fun(1, 2)",
        "fn fun(i: int, j: int) { return i + j }"        
        /*
        */
    };

    printf("original:\n\n");
    for (size_t i = 0; i < sizeof(test_strs)/sizeof(test_strs[0]); i++)
    {
        char* str = test_strs[i];
        decl* d = parse_decl(str);
        print_declaration(d);
        printf("\n");

        push_symbol_from_decl(d);
    }

    for (symbol** it = symbols + installed_count; 
        it != buf_end(symbols); 
        it++)
    {
        symbol* sym = *it;        
        complete_symbol(sym);
    }
   
    printf("\nordered:\n\n");
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





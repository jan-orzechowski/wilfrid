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
    type* ret_type;
    type** param_types;
    size_t param_count;
} type_function;

typedef struct type_aggregate_field
{
    const char* name;
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

symbol** global_symbols;
symbol** ordered_global_symbols;

enum
{
    MAX_LOCAL_SYMBOLS = 1024
};

symbol local_symbols[MAX_LOCAL_SYMBOLS];
symbol* last_local_symbol = local_symbols;

void complete_type(type* t);
void resolve_symbol(symbol* s);
type* resolve_typespec(typespec* t);
resolved_expr* resolve_expr(expr* e);
void resolve_stmt(stmt* st, type* opt_ret_type);

symbol* get_symbol(const char* name)
{
    // do zastąpienia hash table
    for (symbol* it = last_local_symbol; it != local_symbols; it--)
    {
        symbol* sym = it - 1;
        if (sym->name == name)
        {
            return sym;
        }
    }

    for (symbol** it = global_symbols; it != buf_end(global_symbols); it++)
    {
        symbol* sym = *it;
        if (sym->name == name)
        {
            return sym;
        }
    }

    return 0;
}

type* get_new_type(type_kind kind)
{
    type* t = xcalloc(sizeof(type));
    t->kind = kind;
    return t;
}

symbol* enter_local_scope(void)
{
    symbol* marker = last_local_symbol;
    return marker;
}

void leave_local_scope(symbol* marker)
{
    assert(marker >= local_symbols && marker <= last_local_symbol);
    last_local_symbol = marker;
    // dalszych nie czyścimy - nie będziemy ich nigdy odczytywać
}

void push_local_symbol(const char* name, type* type)
{
    int64_t symbols_count = last_local_symbol - local_symbols;
    assert(symbols_count + 1 < MAX_LOCAL_SYMBOLS);
    *last_local_symbol++ = (symbol){
      .name = name,
      .kind = SYMBOL_VARIABLE,
      .state = SYMBOL_RESOLVED,
      .type = type,
    };
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

type* get_function_type(type** param_types, size_t param_types_count, type* return_type)
{
    type* type = get_new_type(TYPE_FUNCTION);
    type->size = POINTER_SIZE;
    type->align = POINTER_ALIGN;
    type->function.param_types = param_types;
    type->function.param_count = param_types_count;
    type->function.ret_type = return_type;
    return type;
}

symbol* get_new_symbol(symbol_kind kind, const char* name, decl* decl)
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

symbol* push_installed_symbol(const char* name, type* type)
{
    symbol* sym = get_new_symbol(SYMBOL_TYPE, name, NULL);
    sym->state = SYMBOL_RESOLVED;
    sym->type = type;
    buf_push(global_symbols, sym);
    return sym;
}

void push_symbol_from_decl(decl* d)
{
    symbol* s = get_symbol_from_decl(d);
    buf_push(global_symbols, s);
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
    for (size_t i = 0; i < d->aggregate.fields_count; i++)
    {
        aggregate_field field = d->aggregate.fields[i];
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

    buf_push(ordered_global_symbols, t->symbol);
}

symbol* resolve_name(const char* name)
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
                resolved_expr* size_expr = resolve_expr(t->array.size_expr);
                // teraz trzeba czegoś w rodzaju evalutate expr...

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
                //result = resolve_typespec(t->function.ret_type);
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
    resolved_expr* operand = resolve_expr(expr->unary.operand);
    type* type = operand->type;
    switch (expr->unary.operator)
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
                fatal("Can only use unary %s with ints", get_token_kind_name(expr->unary.operator));
            }
            if (operand->is_const)
            {
                int64_t value = eval_int_unary_op(expr->unary.operator, operand->val);
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
    resolved_expr* left = resolve_expr(expr->binary.left);
    resolved_expr* right = resolve_expr(expr->binary.right);
    
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
        int64_t const_value = eval_int_binary_op(expr->binary.operator, left->val, right->val);
        result = get_resolved_const_expr(const_value);
    }
    else
    {
        result = get_resolved_rvalue_expr(left->type);
    }

    return result;
}

resolved_expr* resolve_expr(expr* e)
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
            resolved_expr* fn_expr = resolve_expr(e->call.function_expr);
            size_t arg_param_count = fn_expr->type->function.param_count;
            if (arg_param_count == e->call.args_num)
            {
                for (size_t i = 0; i < arg_param_count; i++)
                {
                    expr* arg_expr = e->call.args[i];
                    resolved_expr* resolved_arg_expr = resolve_expr(arg_expr);
                    type* param_type = fn_expr->type->function.param_types[i];                    
                    if (param_type != resolved_arg_expr->type)
                    {
                        fatal("argument has invalid type");
                    }
                }
            }
            else
            {
                fatal("invalid number of arguments");
            }
            result = get_resolved_rvalue_expr(fn_expr->type->function.ret_type);
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
            fatal("ternary exprs not allowed as constants");
        }
        break;
        case EXPR_SIZEOF:
        {
            resolved_expr* expr = resolve_expr(e->size_of.expr);
            type* expr_type = expr->type;
            complete_type(expr_type);
            int64_t size = get_type_size(expr_type);
            result = get_resolved_const_expr(size);
        }
        break;
        case EXPR_FIELD:
        {           
            resolved_expr* aggregate_expr = resolve_expr(e->field.expr);
            const char* field_name = str_intern(e->field.field_name);
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
        }
        break;
        case EXPR_INDEX:
        {
            resolved_expr* operand_expr = resolve_expr(e->index.array_expr);
            if (pointer_decay(operand_expr)->type->kind != TYPE_POINTER)
            {
                fatal("can only index arrays or pointers");
            }
            
            resolved_expr* index_expr = resolve_expr(e->index.index_expr);


            if (index_expr->type->kind != TYPE_INT)
            {
                fatal("index must be an integer");
            }

            result = get_resolved_lvalue_expr(operand_expr->type->pointer.base_type);
        }
        break;
        case EXPR_COMPOUND_LITERAL:
        {
            for (size_t i = 0; i < e->compound_literal.fields_count; i++)
            {
                compound_literal_field* field = e->compound_literal.fields[i];
                resolved_expr* field_expr = resolve_expr(field->expr);

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

resolved_expr* resolve_expected_expr(expr* e, type* expected_type)
{
    resolved_expr* result = resolve_expr(e);
    if (result->type != expected_type)
    {
        fatal("type different than expected");
    }
    return result;
}

type* resolve_const_decl(decl* d)
{
    type* result = 0;
    assert(d->const_decl.expr);
    resolved_expr* expr = resolve_expr(d->const_decl.expr);
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

    if (d->variable.type)
    {
        result = resolve_typespec(d->variable.type);
    }

    if (d->variable.expr)
    {
        resolved_expr* expr = resolve_expr(d->variable.expr);
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
    type* result = resolve_typespec(d->typedef_decl.type);
    return result;
}

type* resolve_function_decl(decl* d)
{
    assert(d->kind == DECL_FUNCTION);

    type* resolved_return_type = type_void;
    if (d->function.return_type)
    {
        resolved_return_type = resolve_typespec(d->function.return_type);
    }
    
    type** resolved_args = 0;
    function_param_list* args = &d->function.params;
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
    buf_push(ordered_global_symbols, s);
}

void resolve_stmt_block(stmt_block st_block)
{
    symbol* marker = enter_local_scope();

    for (size_t i = 0; i < st_block.stmts_count; i++)
    {
        stmt* st = st_block.stmts[i];
        resolve_stmt(st, NULL);
    }

    leave_local_scope(marker);
}

void resolve_stmt(stmt* st, type* opt_ret_type)
{
    switch (st->kind)
    {
        case STMT_NONE:
        {
            fatal("statement must have a valid kind");
        }
        break;
        case STMT_RETURN:
        {
            if (st->expr)
            {
                resolved_expr* result = resolve_expected_expr(st->expr, opt_ret_type);
                if (result->type != opt_ret_type)
                {
                    fatal("return type mismatch");
                }
            }
            else
            {
                if (opt_ret_type != type_void)
                {
                    fatal("empty return expr for function with non-void return type");
                }
            }
            break;
        }
        break;
        case STMT_IF_ELSE:
        {
            resolve_expr(st->if_else.cond_expr);
            resolve_stmt_block(st->if_else.then_block);
            resolve_stmt(st->if_else.else_stmt, NULL);
        }
        break;
        case STMT_WHILE:
        {
            resolve_expr(st->while_stmt.cond_expr);
            resolve_stmt_block(st->while_stmt.stmts);
        }
        break;
        case STMT_DO_WHILE:
        {
            resolve_expr(st->do_while_stmt.cond_expr);
            resolve_stmt_block(st->do_while_stmt.stmts);
        }
        break;
        case STMT_FOR:
        {
            resolve_type_decl(st->for_stmt.init_decl);
            resolve_expr(st->for_stmt.cond_expr);
            resolve_stmt(st->for_stmt.incr_stmt, NULL);

            resolve_stmt_block(st->for_stmt.stmts);
        }
        break;
        case STMT_DECL:
        {            
            assert(st->decl.decl->kind == DECL_VARIABLE);
            type* t = resolve_variable_decl(st->decl.decl);
            push_local_symbol(st->decl.decl->name, t);
        }
        break;
        case STMT_ASSIGN:
        {
            resolved_expr* left = resolve_expr(st->assign.assigned_var_expr);
            if (st->assign.value_expr)
            {                
                resolved_expr* right = resolve_expected_expr(st->assign.value_expr, left->type);
                if (left->type != right->type)
                {
                    fatal("types do not match in assignment statement");
                }
            }
            if (false == left->is_lvalue)
            {
                fatal("cannot assign to non-lvalue");
            }
            if (st->assign.operation != TOKEN_ASSIGN && left->type != type_int)
            {
                fatal("for now can only use assignment operators with type int");
            }
        }
        break; 
        case STMT_EXPR:
        {
            resolve_expr(st->expr);
        }
        break;
        case STMT_BLOCK:
        {
            resolve_stmt_block(st->block);
        }
        break;
        case STMT_BREAK:
        case STMT_CONTINUE:
        break;
        case STMT_SWITCH:
        default:
        {
            fatal("stmt kind not implemented");
        }
        break;
    }
}

void resolve_function_body(symbol* s)
{
    assert(s->state == SYMBOL_RESOLVED);
    type* ret_type = s->type->function.ret_type;

    symbol* marker = enter_local_scope();

    for (size_t i = 0; i < s->decl->function.params.param_count; i++)
    {
        function_param* p = &s->decl->function.params.params[i];
        push_local_symbol(p->name, resolve_typespec(p->type));
    }
    
    size_t count = s->decl->function.stmts.stmts_count;
    for (size_t i = 0; i < count; i++)
    {
        stmt* st = s->decl->function.stmts.stmts[i];
        resolve_stmt(st, ret_type);        
    }

    leave_local_scope(marker);
}

void complete_symbol(symbol* sym)
{
    resolve_symbol(sym);
    if (sym->kind == SYMBOL_TYPE)
    {
        complete_type(sym->type);
    }
    else if (sym->kind == SYMBOL_FUNCTION)
    {
        resolve_function_body(sym);
    }
}

void resolve_test(void)
{
    arena = allocate_memory_arena(megabytes(50));
    
    size_t installed_count = 3;
    push_installed_symbol(str_intern("void"), type_void);
    push_installed_symbol(str_intern("char"), type_char);
    push_installed_symbol(str_intern("int"), type_int);
    push_installed_symbol(str_intern("float"), type_float);

    char* test_strs[] = {
#if 1
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
#endif
        "let i: int = 1",
        "fn fun2(vec: v2): int {\
            let local = fun(i, 2)\
                { let scoped = 1 }\
            vec.x = local\
            return vec.x } ",
        "fn fun(i: int, j: int): int { j++ i++ return i + j }",
        "struct v2 { x: int, y: int }",
    };

    printf("original:\n\n");
    for (size_t i = 0; i < sizeof(test_strs)/sizeof(test_strs[0]); i++)
    {
        char* str = test_strs[i];
        decl* d = parse_decl(str);
        print_decl(d);
        printf("\n");

        push_symbol_from_decl(d);
    }

    for (symbol** it = global_symbols + installed_count; 
        it != buf_end(global_symbols); 
        it++)
    {
        symbol* sym = *it;        
        complete_symbol(sym);
    }
   
    printf("\nordered:\n\n");
    for (symbol** it = ordered_global_symbols; it != buf_end(ordered_global_symbols); it++)
    {
        symbol* sym = *it;
        if (sym->decl)
        {
            print_decl(sym->decl);
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





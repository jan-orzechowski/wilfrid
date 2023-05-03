#include "utils.h"
#include "parsing.h"
#include "resolve.h"

size_t get_type_size(type *type)
{
    assert(type);
    assert(type->kind > TYPE_COMPLETING);
    assert(type->size != 0);
    return type->size;
}

//size_t get_type_align(type *type)
//{
//    assert(type->kind > TYPE_COMPLETING);
//    assert(is_power_of_2(type->align));
//    return type->align;
//}

const size_t POINTER_SIZE = 8;
const size_t POINTER_ALIGN = 8;

type *type_void =    &(type) { .kind = TYPE_VOID,    .size = 1, /*.align = 0 */ };
type *type_null =    &(type) { .kind = TYPE_NULL,    .size = 1, /*.align = 0 */ };
type *type_char =    &(type) { .kind = TYPE_CHAR,    .size = 1, /*.align = 1 */ };
type *type_int =     &(type) { .kind = TYPE_INT,     .size = 4, /*.align = 4 */ };
type *type_uint =    &(type) { .kind = TYPE_UINT,    .size = 4, /*.align = 4 */ };
type *type_long =    &(type) { .kind = TYPE_LONG,    .size = 8, /*.align = 8 */ };
type *type_ulong =   &(type) { .kind = TYPE_ULONG,   .size = 8, /*.align = 8 */ };
type *type_float =   &(type) { .kind = TYPE_FLOAT,   .size = 4, /*.align = 4 */ };
type *type_bool =    &(type) { .kind = TYPE_BOOL,    .size = 4, /*.align = 4 */ };
type *type_invalid = &(type) { .kind = TYPE_NONE,    .size = 0, /*.align = 0 */ };

hashmap global_symbols;
symbol **global_symbols_list;
symbol **ordered_global_symbols;

bool installed_types_initialized = false;

enum
{
    MAX_LOCAL_SYMBOLS = 1024
};

symbol local_symbols[MAX_LOCAL_SYMBOLS];
symbol *last_local_symbol = local_symbols;

bool panic_mode;

void complete_type(type *t);
void resolve_symbol(symbol *s);
type *resolve_typespec(typespec *t);
resolved_expr *resolve_expr(expr *e);
resolved_expr *resolve_expected_expr(expr *e, type *expected_type, bool ignore_expected_type_mismatch);
void resolve_stmt(stmt *st, type *opt_ret_type);

const char *get_function_mangled_name(decl *dec);

resolved_expr *resolved_expr_invalid;

void error_in_resolving(const char *error_text, source_pos pos)
{
    error(error_text, pos, 0);
}

const char *pretty_print_type_name(type *ty, bool plural);

#define on_invalid_type_return(t) if (!(t) || (t->kind == TYPE_NONE)) { return resolved_expr_invalid; }
#define on_invalid_expr_return(e) if (!(e) || !((e)->type) || ((e)->type->kind == TYPE_NONE)) { return resolved_expr_invalid; }

#define check_resolved_expr(e) ((e) && (e)->type && (e)->type->kind != TYPE_NONE)

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
    assert(arr_type->kind == TYPE_ARRAY);

    assert(element_index < arr_type->array.size);
    size_t result = get_type_size(arr_type->array.base_type) * element_index;
    return result;
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
        debug_breakpoint;
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

bool can_perform_cast(type *from_type, type *to_type, bool allow_integer_conversion)
{
    if (allow_integer_conversion)
    {
        if (is_integer_type(from_type) && is_integer_type(to_type))
        {
            return true;
        }
    }

    if (compare_types(from_type, to_type))
    {
        return true;
    }

    if (from_type == type_null && to_type->kind == TYPE_POINTER)
    {
        return true;
    }

    if ((from_type == type_ulong && to_type->kind == TYPE_POINTER)
        || (to_type == type_ulong && from_type->kind == TYPE_POINTER))
    {
        return true;
    }

    if ((from_type == type_long && to_type->kind == TYPE_ENUM)
        || (to_type == type_long && from_type->kind == TYPE_ENUM))
    {
        return true;
    }

    return false;
}

bool are_symbols_the_same_function(symbol *a, symbol *b)
{
    assert(a);
    assert(b);

    /*
        do rozważenia: zakładając interning stringów, może po prostu porównanie mangled names byłoby szybsze?
    */

    assert(a->kind == SYMBOL_FUNCTION);
    assert(b->kind == SYMBOL_FUNCTION);
    if (a == b)
    {
        return true;
    }

    if (a->decl->function.return_type == null
        && b->decl->function.return_type != null)
    {
        return false;
    }

    if (a->decl->function.return_type != null
        && b->decl->function.return_type == null)
    {
        return false;
    }

    if (a->decl->function.return_type != null
        && a->decl->function.return_type != null)
    {
        // zakładam interning stringów
        if (a->decl->function.return_type->name
            != b->decl->function.return_type->name)
        {
            return false;
        }
    }

    if (a->decl->function.method_receiver == null
        && b->decl->function.method_receiver != null)
    {
        return false;
    }

    if (a->decl->function.method_receiver != null
        && b->decl->function.method_receiver == null)
    {
        return false;
    }

    if (a->decl->function.method_receiver != null
        && a->decl->function.method_receiver != null)
    {
        // zakładam interning stringów
        assert(a->decl->function.method_receiver->type);
        assert(b->decl->function.method_receiver->type);
        if (a->decl->function.method_receiver->type->name
            != b->decl->function.method_receiver->type->name)
        {
            return false;
        }
    }

    if (a->decl->function.params.param_count 
        != b->decl->function.params.param_count)
    {
        return false;
    }

    for (size_t param_index = 0;
        param_index < a->decl->function.params.param_count;
        param_index++)
    {
        if (a->decl->function.params.params[param_index].type->name
            != b->decl->function.params.params[param_index].type->name)
        {
            return false;
        }
    }

    return true;
}

symbol *get_symbol(const char *name)
{
    for (symbol *it = last_local_symbol; it != local_symbols; it--)
    {
        symbol *sym = it - 1;
        if (sym->name == name)
        {
            return sym;
        }
    }

    void *global_symbol = map_get(&global_symbols, name);
    if (global_symbol)
    {
        return global_symbol;
    }

    return null;
}

bool check_if_symbol_name_unused(const char *name, source_pos pos)
{
    symbol *sym = get_symbol(name);
    if (sym)
    {
        error_in_resolving(xprintf("Identifier '%s' is already declared.", sym->name), pos);
        return false;
    }
    return true;
}

type *get_new_type(type_kind kind)
{
    type *t = xcalloc(sizeof(type));
    t->kind = kind;
    return t;
}

symbol *enter_local_scope(void)
{
    symbol *marker = last_local_symbol;
    return marker;
}

void leave_local_scope(symbol *marker)
{
    assert(marker >= local_symbols && marker <= last_local_symbol);
    last_local_symbol = marker;
    // dalszych nie czyścimy - nie będziemy ich nigdy odczytywać
}

void push_local_symbol(const char *name, type *type)
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

void complete_enum_type(type *type, hashmap values_map)
{
    assert(type->kind == TYPE_COMPLETING);
    type->kind = TYPE_ENUM;
    type->size = 8;
    type->enumeration.values = values_map;
}

void complete_aggregate_type(type *type, type_aggregate_field **fields, size_t fields_count, bool is_union)
{
    assert(type->kind == TYPE_COMPLETING);
    type->kind = is_union ? TYPE_UNION : TYPE_STRUCT;
    type->size = 0;
    //type->align = 0;
    
    for (type_aggregate_field **it = fields; it != fields + fields_count; it++)
    {
        type_aggregate_field *field = *it;
        if (field->type == null || field->type->kind == TYPE_NONE)
        {
            assert(type->symbol);
            error_in_resolving(
                xprintf("Could not resolve field '%s' in the '%s' struct definition", field->name, type->symbol->name),
                type->symbol->decl->pos);
            // bez tego będziemy mieli błędy później - typ będzie completed, ale pola nie będą uzupełnione
            type->kind = TYPE_NONE; 
            return;
        }
        
        if (is_union)
        {
            field->offset = 0;
            if (type->size < get_type_size(field->type))
            {
                type->size = get_type_size(field->type);
            }
        }
        else
        {
            field->offset = type->size;
            type->size += get_type_size(field->type);
        }
        
        //type->size = get_type_size(field->type) + align_up(type->size, get_type_align(field->type));
        //type->align = max(type->align, get_type_align(field->type));
    }

    type->aggregate.fields = xmempcy(fields, fields_count * sizeof(*fields));
    type->aggregate.fields_count = fields_count;
}

type *get_array_type(type *element, size_t size)
{
    complete_type(element);
    type *t = get_new_type(TYPE_ARRAY);
    t->size = size * get_type_size(element);
    //t->align = get_type_align(element);
    t->array.base_type = element;
    t->array.size = size;
    return t; 
}

type *get_list_type(type *element)
{
    complete_type(element);
    type *t = get_new_type(TYPE_LIST);
    t->size = POINTER_SIZE;
    //t->align = get_type_align(element);
    t->list.base_type = element;
    return t;
}

type *get_incomplete_type(symbol *sym)
{
    type *type = get_new_type(TYPE_INCOMPLETE);
    type->symbol = sym;
    return type;
}

type **cached_pointer_types;

type *get_pointer_type(type *base_type)
{
    if (base_type == null)
    {
        return type_invalid;
    }

    size_t ptr_count = buf_len(cached_pointer_types);
    if (ptr_count > 0)
    {
        for (size_t i = 0; i < ptr_count; i++)
        {
            type *ptr_type = cached_pointer_types[i];
            assert(ptr_type->kind == TYPE_POINTER);

            if (compare_types(ptr_type->pointer.base_type, base_type))
            {
                return ptr_type;
            }
        }
    }

    type *type = get_new_type(TYPE_POINTER);
    type->size = POINTER_SIZE;
    //type->align = POINTER_ALIGN;
    type->pointer.base_type = base_type;
    buf_push(cached_pointer_types, type);
    return type;
}

type *get_function_type(type **param_types, size_t param_types_count, type *return_type)
{
    type *type = get_new_type(TYPE_FUNCTION);
    type->size = POINTER_SIZE;
    //type->align = POINTER_ALIGN;
    type->function.param_types = param_types;
    type->function.param_count = param_types_count;
    type->function.return_type = return_type;
    return type;
}

symbol *get_new_symbol(symbol_kind kind, const char *name, decl *decl)
{
    symbol *sym = xcalloc(sizeof(symbol));
    sym->kind = kind;
    sym->name = name;
    sym->decl = decl;
    return sym;
}

resolved_expr *get_resolved_rvalue_expr(type *t)
{
    assert(t);
    resolved_expr *result = xcalloc(sizeof(resolved_expr));
    result->type = t;
    return result;
}

resolved_expr *get_resolved_lvalue_expr(type *t)
{
    assert(t);
    assert(t->kind != SYMBOL_CONST);
    resolved_expr *result = xcalloc(sizeof(resolved_expr));
    result->type = t;
    result->is_lvalue = true;
    return result;
}

resolved_expr *get_resolved_const_expr(int64_t val)
{
    resolved_expr *result = xcalloc(sizeof(resolved_expr));
    result->type = type_long;
    result->is_const = true;
    result->val = val;
    return result;
}

symbol *get_symbol_from_decl(decl *d)
{
    symbol_kind kind = SYMBOL_NONE;
    switch (d->kind)
    {
        case DECL_STRUCT:
        case DECL_UNION:
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
        invalid_default_case;
    }
    assert(kind != SYMBOL_NONE);

    symbol *sym = get_new_symbol(kind, d->name, d);
    if (kind == SYMBOL_TYPE)
    {
        sym->state = SYMBOL_RESOLVED;
        sym->type = get_incomplete_type(sym);
    }

    return sym;
}

symbol *push_installed_symbol(const char *name, type *type)
{
    const char *interned = str_intern(name);
    symbol *sym = get_new_symbol(SYMBOL_TYPE, interned, null);
    sym->state = SYMBOL_RESOLVED;
    sym->type = type;
    map_put(&global_symbols, interned, sym);
    buf_push(global_symbols_list, sym);
    return sym;
}

void push_symbol_from_decl(decl *d)
{
    symbol *sym = get_symbol_from_decl(d);

    if (sym->name == NULL)
    {
        return;
    }

    if (sym->kind == SYMBOL_FUNCTION)
    {
        sym->mangled_name = get_function_mangled_name(sym->decl);
        symbol *overload = map_get(&global_symbols, sym->name);
        if (overload)
        {
            if (overload->kind != SYMBOL_FUNCTION)
            {
                // być może to można zmienić
                // czy na pewno struct/union jest tu jedyną opcją? a const?
                error_in_resolving("Structs and functions cannot share names", d->pos);
                return;
            }

            symbol *prev = null;
            do
            {
                if (are_symbols_the_same_function(sym, overload))
                {
                    error_in_resolving("Duplicate overloaded functions", d->pos);
                    return;
                }

                prev = overload;
                overload = overload->next_overload;
            }
            while (overload);

            prev->next_overload = sym;
            return;
        }
    }
    else
    {
        // tylko funkcje mogą być przeciążone
        if (false == check_if_symbol_name_unused(sym->name, d->pos))
        {
            return;
        }
    }

    map_put(&global_symbols, sym->name, sym);
    buf_push(global_symbols_list, sym);
}

void complete_type(type *t)
{
    assert(t);

    if (t->kind == TYPE_COMPLETING)
    {
        error_in_resolving("Detected a cyclic dependency in type declarations", t->symbol->decl->pos);
        return;
    }
    else if (t->kind != TYPE_INCOMPLETE)
    {
        return;
    }
    
    t->kind = TYPE_COMPLETING;
    decl *d = t->symbol->decl;

    // pozostałe typy są kompletne od razu
    assert(d->kind == DECL_STRUCT || d->kind == DECL_UNION || d->kind == DECL_ENUM);

    if (d->kind == DECL_ENUM)
    {
        hashmap values = {0};
        map_grow(&values, 4);

        int64_t largest_value = 0;
        for (size_t i = 0; i < d->enum_decl.values_count; i++)
        {
            enum_value *val = &d->enum_decl.values[i];
            if (val->value_set && val->value > largest_value)
            {
                largest_value = val->value_set;
            }
        }

        for (size_t i = 0; i < d->enum_decl.values_count; i++)
        {
            enum_value *val = &d->enum_decl.values[i];
            if (false == val->value_set)
            {
                if (largest_value >= INT64_MAX - 1)
                {
                    error_in_resolving(xprintf(
                        "Max long integer value reached in the %s enumeration. Please set enumeration values to smaller integers.",
                        d->name), d->pos);
                }
                else
                {
                    val->value = largest_value++;
                }
            }
        }
        
        for (size_t i = 0; i < d->enum_decl.values_count; i++)
        {
            enum_value *val = &d->enum_decl.values[i];            
            assert_is_interned(val->name);
            void *val_ptr = push_size(arena, sizeof(int64_t));
            *(int64_t *)val_ptr = val->value;
            map_put(&values, val->name, val_ptr);
        }

        complete_enum_type(t, values);
    }
    else
    {
        type_aggregate_field **fields = null;
        for (size_t i = 0; i < d->aggregate.fields_count; i++)
        {
            aggregate_field field = d->aggregate.fields[i];
            type *field_type = resolve_typespec(field.type);
            complete_type(field_type); // wszystkie muszą być completed, ponieważ musimy znać ich rozmiar

            type_aggregate_field *type_field = xcalloc(sizeof(type_aggregate_field));
            type_field->name = field.name;
            type_field->type = field_type;
            buf_push(fields, type_field);
        }

        if (buf_len(fields) == 0)
        {
            if (d->kind == DECL_STRUCT)
            {
                error_in_resolving("Structs must have at least one field declared", d->pos);
            }
            else
            {
                error_in_resolving("Unions must have at least one field declared", d->pos);
            }
            return;
        }

        if (d->kind == DECL_STRUCT)
        {
            complete_aggregate_type(t, fields, buf_len(fields), false);
        }
        else
        {
            assert(d->kind == DECL_UNION);
            complete_aggregate_type(t, fields, buf_len(fields), true);
        }
    }

    buf_push(ordered_global_symbols, t->symbol);
}

symbol *resolve_name(const char *name, source_pos name_pos)
{    
    symbol *s = get_symbol(name);
    if (s == null)
    {
        error_in_resolving(xprintf("Unknown identifier: '%s'", name), name_pos);
        return null;
    }
    resolve_symbol(s);
    return s;
}

type *resolve_typespec(typespec *t)
{
    type *result = null;
    if (t)
    {
        switch (t->kind)
        {
            case TYPESPEC_NAME:
            {
                symbol *sym = resolve_name(t->name, t->pos);
                if (sym == null)
                {
                    // błąd jest już rzucony w resolve_name
                    return type_invalid;
                }
                else if (sym->kind != SYMBOL_TYPE)
                {
                    error_in_resolving(xprintf("%s must denote a type", t->name), t->pos);
                    return type_invalid;
                }
                else
                {
                    result = sym->type;
                }
            }
            break;
            case TYPESPEC_ARRAY:
            {                                
                type *element = resolve_typespec(t->array.base_type);
                if (element->kind == TYPE_NONE)
                {
                    error_in_resolving("Could not resolve type in the array declaration", t->pos);
                    return type_invalid;
                };

                resolved_expr *size_expr = resolve_expr(t->array.size_expr);
                if (size_expr == null)
                {
                    error_in_resolving("Could not resolve size expression in the array declaration", t->pos);
                    return type_invalid;
                }

                result = get_array_type(element, size_expr->val);
            }
            break;
            case TYPESPEC_LIST:
            {
                type *element = resolve_typespec(t->list.base_type);
                if (element->kind == TYPE_LIST)
                {
                    error_in_resolving("Dynamic lists of dynamic lists are not allowed", t->pos);
                    return type_invalid;
                }

                result = get_list_type(element);
            }
            break;
            case TYPESPEC_POINTER:
            {
                type *base_type = resolve_typespec(t->pointer.base_type);
                result = get_pointer_type(base_type);
            }
            break;
            case TYPESPEC_FUNCTION:
            {
                //result = resolve_typespec(t->function.ret_type);
                fatal("unimplemented");
            }
            break;
            invalid_default_case;
        }
    }
    return result;
}

resolved_expr *array_to_pointer_decay(resolved_expr *e)
{        
    if (e->type->kind == TYPE_ARRAY)
    {
        e = get_resolved_lvalue_expr(get_pointer_type(e->type->array.base_type));
    }
    return e;
}

resolved_expr *resolve_expr_unary(expr *expr)
{
    resolved_expr *result = null;

    assert(expr->kind == EXPR_UNARY);
    resolved_expr *operand = resolve_expr(expr->unary.operand);
    if (operand == null || operand->type == null || operand->type->kind == TYPE_NONE)
    {
        return resolved_expr_invalid;
    }

    type *type = operand->type;
    switch (expr->unary.operator)
    {
        case TOKEN_MUL:
        {
            operand = array_to_pointer_decay(operand);
            if (type->kind != TYPE_POINTER)
            {
                error_in_resolving("Cannot dereference non-pointer type", expr->pos);
                return resolved_expr_invalid;
            }
            result = get_resolved_lvalue_expr(type->pointer.base_type);
        }
        break;
        case TOKEN_BITWISE_AND:
        {
            if (false == operand->is_lvalue)
            {
                error_in_resolving("Cannot take address of non-lvalue", expr->pos);
                return resolved_expr_invalid;
            }
            result = get_resolved_rvalue_expr(get_pointer_type(type));
        }
        break;       
        case TOKEN_BITWISE_NOT:
        case TOKEN_NOT:
        {
            if (false == (operand->type == type_uint || operand->type == type_ulong))
            {
                error_in_resolving(
                    xprintf("Bitwise operators allowed only for unsigned integers types, got %s type instead.", operand->type), expr->pos);
                return resolved_expr_invalid;
            }
        }
        break;
        case TOKEN_SUB:
        {
            if (false == is_numeric_type(type))
            {
                error_in_resolving(xprintf("Can use unary %s only with numeric types", get_token_kind_name(expr->unary.operator)), expr->pos);
                return resolved_expr_invalid;
            }

            switch (type->kind)
            {
                case TYPE_UINT:
                    return get_resolved_rvalue_expr(type_int);
                case TYPE_ULONG:                
                    return get_resolved_rvalue_expr(type_long);
                default:
                    return get_resolved_rvalue_expr(type);
            }
        }
        break;
        case TOKEN_INC:
        case TOKEN_DEC:
        {            
            if (type->kind != TYPE_POINTER && false == is_integer_type(type))
            {
                error_in_resolving("Increment and decrement expressions only allowed for pointer and integer types.", expr->pos);
                return resolved_expr_invalid;
            }
            else
            {
                result = get_resolved_rvalue_expr(type);
            }
        }
        break;
        default:
        {
            if (false == is_numeric_type(type))
            {
                error_in_resolving(xprintf("Can use unary %s only with numeric types", get_token_kind_name(expr->unary.operator)), expr->pos);
                return resolved_expr_invalid;
            }

            if (operand->is_const)
            {
                int64_t value = eval_long_unary_op(expr->unary.operator, operand->val);
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

resolved_expr *resolve_expr_binary(expr *expr)
{
    resolved_expr *result = null;

    assert(expr->kind == EXPR_BINARY);
    resolved_expr *left = resolve_expr(expr->binary.left);
    resolved_expr *right = resolve_expr(expr->binary.right);
    
    if (left == null || right == null)
    {
        return resolved_expr_invalid;
    }

    if (false == can_perform_cast(right->type, left->type, false))
    {
        error_in_resolving(
            xprintf(
                "Types mismatch in a %s expression: %s and %s. A manual cast is required.",
                get_token_kind_name(expr->binary.operator),
                pretty_print_type_name(right->type, false),
                pretty_print_type_name(left->type, false)),
            expr->pos);
        return resolved_expr_invalid;
    }

    switch (expr->binary.operator)
    {
        case TOKEN_MOD:
        {
            if (false == is_integer_type(left->type))
            {
                error_in_resolving(
                    xprintf("Modulus operator is allowed only for integer types, got %s type.", 
                        pretty_print_type_name(left->type, false)), expr->pos);
                return resolved_expr_invalid;
            }
        }
        break;
        case TOKEN_BITWISE_AND:
        case TOKEN_BITWISE_OR:
        case TOKEN_LEFT_SHIFT: 
        case TOKEN_RIGHT_SHIFT: 
        case TOKEN_XOR:
        {
            if (false == (left->type == type_uint || left->type == type_ulong))
            {
                error_in_resolving(
                    xprintf("Bitwise operators allowed only for unsigned integers types, got %s type instead.", 
                        pretty_print_type_name(left->type, false)), expr->pos);
                return resolved_expr_invalid;
            }
        }
        break;
    };
        
    if (is_comparison_operation(expr->binary.operator))
    {
        result = get_resolved_rvalue_expr(type_bool);
    }
    else if (left->is_const && right->is_const)
    {
        int64_t const_value = eval_long_binary_op(expr->binary.operator, left->val, right->val);
        result = get_resolved_const_expr(const_value);
    }
    else
    {
        result = get_resolved_rvalue_expr(left->type);
    }

    return result;
}

bool check_compound_expr_field(resolved_expr *init_expr, type *expected_type, size_t init_expr_position, source_pos pos)
{    
    if (false == check_resolved_expr(init_expr))
    {
        error_in_resolving(
            xprintf("Could not resolve type in compound literal at position %d",
                (init_expr_position + 1)),
            pos);
        return false;
    }

    if (false == can_perform_cast(init_expr->type, expected_type, true))
    {
        error_in_resolving(
            xprintf(
                "Compound literal field type mismatch. Expected type %s at position %d, got %s",
                pretty_print_type_name(expected_type, false),
                (init_expr_position + 1),
                pretty_print_type_name(init_expr->type, false)),
            pos);
        return false;
    }

    return true;
}

resolved_expr *resolve_compound_expr(expr *e, type *expected_type, bool ignore_expected_type_mismatch)
{
    assert(e->kind == EXPR_COMPOUND_LITERAL);
    
    if (expected_type && expected_type->kind == TYPE_NONE)
    {
        return resolved_expr_invalid;
    }

    if (e->compound.type == null && expected_type == null)
    {
        error_in_resolving(
"Implicitly typed compound literal is in a context without an expected type. \
Please provide a type either as a cast or in the variable declaration", e->pos);
        return resolved_expr_invalid;
    }

    type *t = null;
    if (e->compound.type)
    {        
        t = resolve_typespec(e->compound.type);
        if (expected_type && t != expected_type)
        {
            if (false == ignore_expected_type_mismatch)
            {
                error_in_resolving("Compound literal has a different type than expected", e->pos);
                return resolved_expr_invalid;
            }
            else
            {
                debug_breakpoint;
            }
        }
    }
    else
    {
        t = expected_type;
    }
    
    complete_type(t);
    resolved_expr *result = get_resolved_rvalue_expr(t);

    if (false == (t->kind == TYPE_STRUCT || t->kind == TYPE_UNION || t->kind == TYPE_ARRAY))
    {
        error_in_resolving(xprintf(
            "Compound literals can only be used with struct and array types, got %s instead",
            pretty_print_type_name(t, false)), e->pos);
        return resolved_expr_invalid;
    }

    if (t->kind == TYPE_STRUCT || t->kind == TYPE_UNION)
    {
        if (e->compound.fields_count > t->aggregate.fields_count)
        {
            error_in_resolving(xprintf(
                "Provided more fields in a compound literal (%d) than is in the initialized struct/union type (%d)",
                e->compound.fields_count, t->aggregate.fields_count), e->pos);
            return result;
        }

        bool name_provided = false;

        for (size_t i = 0; i < e->compound.fields_count; i++)
        {
            compound_literal_field *field = e->compound.fields[i];
            type *aggr_field_type = null;
            assert(field->field_index == -1);
            if (field->field_name)
            {
                aggr_field_type = get_field_type(t, field->field_name);
                name_provided = true;
            }
            else
            {                
                aggr_field_type = get_field_type_by_index(t, i);
                if (name_provided)
                {
                    error_in_resolving(
                        "In a struct/union compound literal, either all of the fields must be named, or none", e->pos);
                    return result;
                }
            }

            resolved_expr *init_expr = resolve_expected_expr(field->expr, aggr_field_type, false);
            if (false == check_compound_expr_field(init_expr, aggr_field_type, i, field->expr->pos))
            {
                return result;
            }
        }
    }
    else if (t->kind == TYPE_ARRAY)
    {
        if (e->compound.fields_count > t->array.size)
        {
            error_in_resolving(xprintf(
                "Provided more values in a compound literal (%d) than is in the initialized array type (%d)",
                e->compound.fields_count, t->array.size), e->pos);
            return result;
        }

        type *expected_type = t->array.base_type;
        bool index_provided = false;

        for (size_t i = 0; i < e->compound.fields_count; i++)
        {
            compound_literal_field *field = e->compound.fields[i];
            assert(field->field_name == null);
            if (field->field_index >= 0)
            {
                index_provided = true;
                if (field->field_index > t->array.size)
                {
                    error_in_resolving(xprintf(
                        "Index specified in the array compound literal (%d) is out of the array bounds (%d)",
                        field->field_index, t->array.size), e->pos);
                    return result;
                }
            }
            else
            {
                assert(field->field_index == -1);
                if (index_provided)
                {
                    error_in_resolving(
                        "In an array compound literal, either each element must have specified index, or none", e->pos);
                    return result;
                }
            }
            
            resolved_expr *init_expr = resolve_expected_expr(field->expr, expected_type, false);
            if (false == check_compound_expr_field(init_expr, expected_type, i, field->expr->pos))
            {
                return result;
            }            
        }
    }
        
    return result;
}

void plug_stub_expr(expr *original_expr, stub_expr_kind kind, type *resolved_type)
{
    expr *new_expr = push_struct(arena, expr);
    new_expr->kind = EXPR_STUB;
    new_expr->pos = original_expr->pos;
    new_expr->stub.kind = kind;
    new_expr->resolved_type = resolved_type;
    
    expr temp = *original_expr;
    (*original_expr) = *new_expr;
    *new_expr = temp;

    // tutaj nazwy są odwrócone; efekt jest taki, że stub ma teraz odniesienie do starego wyrażenia
    original_expr->stub.original_expr = new_expr;
}

resolved_expr *resolve_special_case_methods(expr *e)
{    
    resolved_expr *result = null;
    if (e->kind == EXPR_CALL && e->call.method_receiver)
    {
        resolve_expr(e->call.method_receiver);
        if (null == e->call.method_receiver->resolved_type)
        {
            return null;
        }
        
        if (e->call.method_receiver->resolved_type->kind == TYPE_LIST)
        {
            stub_expr_kind stub_kind = STUB_EXPR_NONE;
            type *resolved_type = null;

            if (e->call.function_expr->name == str_intern("capacity"))
            {
                if (e->call.args_num != 0)
                {
                    error_in_resolving("Capacity method accepts no arguments", e->pos);
                    return null;
                }
                
                stub_kind = STUB_EXPR_LIST_CAPACITY;
                resolved_type = type_int;
            }            
            else if (e->call.function_expr->name == str_intern("length"))
            {
                if (e->call.args_num != 0)
                {
                    error_in_resolving("Length method accepts no arguments", e->pos);
                    return null;
                }

                stub_kind = STUB_EXPR_LIST_LENGTH;
                resolved_type = type_int;
            }
            else if (e->call.function_expr->name == str_intern("add"))
            {
                if (e->call.args_num != 1)
                {
                    error_in_resolving("Add method accepts only one argument", e->pos);
                    return null;
                }

                type *list_type = e->call.method_receiver->resolved_type;
                type *list_element_type = list_type->list.base_type;
                type *new_element_type = resolve_expr(e->call.args[0])->type;

                if (false == can_perform_cast(new_element_type, list_element_type, true))
                {
                    error_in_resolving(
                        xprintf("Cannot add %d element to a list of %d",
                            pretty_print_type_name(new_element_type, false),
                            pretty_print_type_name(list_element_type, true)),
                        e->pos);
                    return null;
                }
                
                stub_kind = STUB_EXPR_LIST_ADD;
                resolved_type = type_void;
            }

            if (stub_kind)
            {
                plug_stub_expr(e, stub_kind, resolved_type);
            }

            result = get_resolved_rvalue_expr(resolved_type);
        }
    }

    return result;
}

resolved_expr *resolve_special_case_constructors(expr *e)
{
    if (e->call.function_expr->kind != EXPR_NEW 
        && e->call.function_expr->kind != EXPR_AUTO)
    {
        return null;
    }
    
    resolved_expr *fn_expr = resolve_expr(e->call.function_expr);
    on_invalid_expr_return(fn_expr);

    type *return_type = fn_expr->type;
    if (e->call.args_num == 0)
    {
        plug_stub_expr(e, STUB_EXPR_CONSTRUCTOR, return_type);

        resolved_expr *result = get_resolved_rvalue_expr(return_type);
        return result;
    }
    else
    {
        symbol *matching = null;
        symbol *candidate = get_symbol(str_intern("constructor"));
        while (candidate)
        {
            if (candidate->state == SYMBOL_UNRESOLVED)
            {
                resolve_symbol(candidate);
            }

            assert(candidate->type->kind == TYPE_FUNCTION);
            if (candidate->type->function.has_variadic_arg)
            {
                goto constructor_candidate_check_next;
            }

            if (false == compare_types(candidate->type->function.return_type, return_type))
            {
                goto constructor_candidate_check_next;
            }

            if (candidate->type->function.param_count == e->call.args_num)
            {
                for (size_t i = 0; i < candidate->type->function.param_count; i++)
                {
                    expr *arg_expr = e->call.args[i];
                    type *expected_param_type = candidate->type->function.param_types[i];
                    resolved_expr *resolved_arg_expr = resolve_expected_expr(arg_expr, expected_param_type, true);
                    if (false == compare_types(resolved_arg_expr->type, expected_param_type))
                    {
                        goto constructor_candidate_check_next;
                    }
                }

                matching = candidate;
                break;
            }

        constructor_candidate_check_next:

            candidate = candidate->next_overload;
        }

        if (matching == null)
        {
            error_in_resolving("No overload is matching types of constructor arguments", e->pos);
            return null;
        }

        e->call.resolved_function = matching;

        resolved_expr *result = get_resolved_rvalue_expr(return_type);
        return result;
    }
}

resolved_expr *resolve_call_expr(expr *e)
{
    assert(e->kind == EXPR_CALL);
    resolved_expr *result = resolved_expr_invalid;

    result = resolve_special_case_constructors(e);
    if (result)
    {
        return result;
    }

    result = resolve_special_case_methods(e);
    if (result)
    {
        return result;
    }

    symbol *matching = null;
    resolved_expr *fn_expr = resolve_expr(e->call.function_expr);
    
    if (fn_expr == null || fn_expr->type->kind != TYPE_FUNCTION)
    {
        if (e->call.function_expr->kind == EXPR_NAME && e->call.function_expr->name)
        {
            error_in_resolving(xprintf("Could not resolve function call to '%s'", e->call.function_expr->name), e->pos);
        }
        else
        {
            error_in_resolving("Could not resolve function call", e->pos);
        }
        return resolved_expr_invalid;
    }

    symbol *candidate = fn_expr->type->symbol;
    while (candidate)
    {
        assert(candidate->type->kind == TYPE_FUNCTION);

        if (fn_expr->type->function.receiver_type)
        {
            if (fn_expr->type->function.receiver_type
                != candidate->type->function.receiver_type)
            {
                goto candidate_check_next;
            }
        }

        size_t arg_count = candidate->type->function.param_count;
        if (arg_count == e->call.args_num
            || candidate->type->function.has_variadic_arg)
        {
            for (size_t i = 0; i < arg_count; i++)
            {
                expr *arg_expr = e->call.args[i];
                type *expected_param_type = candidate->type->function.param_types[i];
                resolved_expr *resolved_arg_expr = resolve_expected_expr(arg_expr, expected_param_type, true);

                if (resolved_arg_expr->type == null || resolved_arg_expr->type->kind == TYPE_NONE)
                {
                    error_in_resolving("Not all arguments to a function call could be resolved", e->pos);
                    return resolved_expr_invalid;
                }

                if (false == compare_types(resolved_arg_expr->type, expected_param_type))
                {
                    goto candidate_check_next;
                }
            }

            // w przypadku variadic potrzebujemy resolve pozostałych argumentów
            if (e->call.args_num > arg_count)
            {
                for (size_t i = arg_count; i < e->call.args_num; i++)
                {
                    expr *arg_expr = e->call.args[i];
                    resolved_expr *resolved_arg_expr = resolve_expected_expr(arg_expr, null, false);
                }
            }

            // jeśli tu jesteśmy, to wszystkie argumenty zgadzają się
            // variadic candidate jest użyty tylko wtedy, gdy żaden inny się nie zgadza
            if (candidate->type->function.has_variadic_arg)
            {
                if (matching == null)
                {
                    matching = candidate;
                }
            }
            else
            {
                matching = candidate;
            }
        }

    candidate_check_next:

        candidate = candidate->next_overload;
    }

    if (matching == null)
    {
        error_in_resolving("No overload is matching types of arguments in the function call", e->pos);
        return resolved_expr_invalid;
    }

    e->call.resolved_function = matching;

    result = get_resolved_rvalue_expr(matching->type->function.return_type);
    return result;
}

resolved_expr *resolve_expected_expr(expr *e, type *expected_type, bool ignore_expected_type_mismatch)
{
    if (e == null)
    {
        return resolved_expr_invalid;
    }

    stub_expr_kind stub_kind = STUB_EXPR_NONE;

    resolved_expr *result = resolved_expr_invalid;
    switch (e->kind)
    {
        case EXPR_NAME:
        {
            symbol *sym = resolve_name(e->name, e->pos);
            if (sym == null)
            {
                // błąd jest już rzucony w resolve_name
                return resolved_expr_invalid;
            }
            else if (sym->type == null || sym->type->kind == TYPE_NONE)
            {
                error_in_resolving(
                    xprintf("Expression '%s' has unknown type", sym->name), e->pos);
                return resolved_expr_invalid;
            }
            else if (sym->kind == SYMBOL_VARIABLE)
            {
                result = get_resolved_lvalue_expr(sym->type);
            }
            else if (sym->kind == SYMBOL_CONST)
            {
                result = get_resolved_rvalue_expr(sym->type);
                result->is_const = true;
                result->val = sym->val;
            }
            else if (sym->kind == SYMBOL_FUNCTION)
            {                  
                result = get_resolved_lvalue_expr(sym->type);             
            }
            else if (sym->kind == SYMBOL_TYPE)
            {
                // czy powinno tak być?
                result = get_resolved_lvalue_expr(sym->type);
            }
            else
            {
                error_in_resolving(xprintf("Not expected symbol kind: %d", sym->kind), e->pos);
                return resolved_expr_invalid;
            } 
        }
        break;
        case EXPR_INT:
        {
            result = get_resolved_rvalue_expr(type_int);
            result->is_const = true;
            result->val = e->integer_value;
        }
        break;
        case EXPR_FLOAT:
        {
            result = get_resolved_rvalue_expr(type_float);
            result->is_const = true;
            result->val = e->integer_value;
        }
        break;
        case EXPR_STRING:
        {
            type *t = get_pointer_type(type_char);
            result = get_resolved_rvalue_expr(t);
            result->is_const = true;
        }
        break;
        case EXPR_BOOL:
        {
            type *t = type_bool;
            result = get_resolved_rvalue_expr(t);
            result->is_const = true;
        }
        break;
        case EXPR_NULL:
        {
            type *t = type_null;
            result = get_resolved_rvalue_expr(t);
            result->is_const = true;
        }
        break;
        case EXPR_CALL:
        {            
            result = resolve_call_expr(e);
        }
        break;
        case EXPR_CAST:
        {
            type *t = resolve_typespec(e->cast.type);
            resolved_expr *expr = resolve_expr(e->cast.expr);

            e->cast.resolved_type = t;            

            result = get_resolved_lvalue_expr(t);
        }
        break;
        case EXPR_NEW:
        {
            type *t = resolve_typespec(e->new.type);
            on_invalid_type_return(t);

            e->new.resolved_type = t;
            if (t->kind != TYPE_LIST)
            {
                t = get_pointer_type(t);
            }
            else
            {
                stub_kind = STUB_EXPR_LIST_NEW;
            }

            result = get_resolved_lvalue_expr(t);
        }
        break;
        case EXPR_AUTO:
        {
            type *t = resolve_typespec(e->auto_new.type);            
            on_invalid_type_return(t);

            e->auto_new.resolved_type = t;
            if (t->kind != TYPE_LIST)
            {
                t = get_pointer_type(t);
            }
            else
            {
                stub_kind = STUB_EXPR_LIST_AUTO;
            }
            
            result = get_resolved_rvalue_expr(t);
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
            resolved_expr *cond = resolve_expr(e->ternary.condition);
            resolved_expr *left = resolve_expr(e->ternary.if_false);
            resolved_expr *right = resolve_expr(e->ternary.if_true);

            if (false == compare_types(left->type, right->type))
            {
                error_in_resolving(xprintf(
                    "Both alternatives in a ternary expressions should have the same type. Now the types are %s and %s",
                    pretty_print_type_name(left->type, false), pretty_print_type_name(right->type, false)), e->pos);
            }
         
            result = get_resolved_rvalue_expr(left->type);
        }
        break;
        case EXPR_SIZE_OF_TYPE:
        {
            type *t = resolve_typespec(e->size_of_type.type);
            
            on_invalid_type_return(t);

            complete_type(t);
            
            e->size_of_type.resolved_type = t;
            int64_t size = get_type_size(t);
            result = get_resolved_const_expr(size);
        }
        break;
        case EXPR_SIZE_OF:
        {
            resolved_expr *expr = resolve_expr(e->size_of.expr);

            on_invalid_expr_return(expr);

            on_invalid_type_return(expr->type)

            int64_t size = get_type_size(expr->type);
            result = get_resolved_const_expr(size);
        }
        break;
        case EXPR_FIELD:
        {           
            resolved_expr *resolved_expr = resolve_expr(e->field.expr);

            on_invalid_expr_return(resolved_expr);            
            
            type *t = resolved_expr->type;
            const char *field_name = e->field.field_name;

            if (t->kind == TYPE_ENUM)
            {
                complete_type(t);

                void *val_ptr = map_get(&t->enumeration.values, field_name);
                if (val_ptr == null)
                {
                    error_in_resolving(xprintf("Enum %s has no value '%s'", t->symbol->name, field_name), e->pos);
                    return resolved_expr_invalid;
                }
                else
                {
                    int64_t val = *(int64_t *)val_ptr;
                    result = get_resolved_const_expr(val);
                }
            }
            else
            {                
                // zawsze uzyskujemy dostęp za pomocą x.y, nawet gdy x jest wskaźnikiem do wskaźnika itd.
                while (t->kind == TYPE_POINTER)
                {
                    t = t->pointer.base_type;
                }
               
                complete_type(t);

                if (false == (t->kind == TYPE_STRUCT || t->kind == TYPE_UNION))
                {
                    error_in_resolving(xprintf(
                        "Only structs, unions and enums can have values accessed by a dot; tried to access %s",
                        pretty_print_type_name(t, false)), e->pos);
                    return resolved_expr_invalid;
                }

                type *found = 0;
                for (size_t i = 0; i < t->aggregate.fields_count; i++)
                {
                    type_aggregate_field *f = t->aggregate.fields[i];
                    if (field_name == f->name)
                    {
                        found = f->type;
                        break;
                    }
                }

                if (found)
                {
                    result = get_resolved_lvalue_expr(found);
                }
                else
                {
                    error_in_resolving(xprintf("No field of name: %s", field_name), e->pos);
                    return resolved_expr_invalid;
                }
            }
        }
        break;
        case EXPR_INDEX:
        {
            resolved_expr *operand_expr = resolve_expr(e->index.array_expr);

            if (operand_expr->type->kind != TYPE_LIST 
                && operand_expr->type->kind != TYPE_ARRAY
                && operand_expr->type->kind != TYPE_POINTER)
            {
                error_in_resolving(xprintf(
                    "Only arrays, lists, and pointers can be accessed by index; tried to access %s",
                    pretty_print_type_name(operand_expr->type, false)), e->pos);
                return resolved_expr_invalid;
            }
            
            resolved_expr *index_expr = resolve_expr(e->index.index_expr);

            if (false == is_integer_type(index_expr->type))
            {
                error_in_resolving("Index to an array/pointer must be an integer", e->pos);
            }
            
            on_invalid_expr_return(operand_expr);
                        
            // przypadek specjalny
            type *t = operand_expr->type;
            if (t->kind == TYPE_POINTER)
            {
                if (t->pointer.base_type->kind == TYPE_ARRAY
                    || t->pointer.base_type->kind == TYPE_LIST)
                {
                    t = t->pointer.base_type;
                }
            }

            if (t->kind == TYPE_LIST)
            {
                result = get_resolved_lvalue_expr(t->list.base_type);
                stub_kind = STUB_EXPR_LIST_INDEX;
            }
            else if (t->kind == TYPE_ARRAY)
            {
                // tutaj ew. array to pointer decay
                result = get_resolved_lvalue_expr(t->array.base_type);
            } 
            else
            {
                result = get_resolved_lvalue_expr(t->pointer.base_type);
            }            
        }
        break;
        case EXPR_COMPOUND_LITERAL:
        {
            result = resolve_compound_expr(e, expected_type, ignore_expected_type_mismatch);
        }
        break;
        case EXPR_STUB:
        {
            // nie powinniśmy się tutaj znaleźć
            fatal("stubs should not be resolved");
        }
        break;
        default:
        {
            fatal("expr kind not yet supported");
        }
        break;
    }

    // przyda nam się podczas generowania kodu
    if (result && result->type)
    {
        assert(e->resolved_type == null || e->resolved_type == result->type);
        e->resolved_type = result->type;
    }

    if (stub_kind != STUB_EXPR_NONE)
    {
        plug_stub_expr(e, stub_kind, e->resolved_type);
    }

    return result;
}

resolved_expr *resolve_expr(expr *e)
{
    resolved_expr *result = resolve_expected_expr(e, null, true);
    return result;
}

type *resolve_variable_decl(decl *d)
{
    type *result = type_invalid;
    
    type *declared_type = null;
    if (d->variable.type)
    {
        declared_type = resolve_typespec(d->variable.type);
        result = declared_type;
    }

    if (d->variable.expr)
    {
        resolved_expr *expr = resolve_expected_expr(d->variable.expr, declared_type, false);
        if (expr == null || expr->type->kind == TYPE_NONE)
        {
            error_in_resolving("Cannot resolve type in the variable declaration", d->pos);
            return null;
        }
        
        // musimy sprawdzić, czy się zgadzają
        if (declared_type && declared_type->kind != TYPE_NONE)
        {
            if (false == can_perform_cast(expr->type, declared_type, true))
            {
                error_in_resolving(
                    xprintf(
                        "Literal and specified type do not match in the variable declaration. Literal is %s, the specified type is %s",
                        pretty_print_type_name(expr->type, false),
                        pretty_print_type_name(declared_type, false)),
                    d->pos);
            }
        }
       
        // jedyny wyjątek dotyczy null
        if (expr->type->kind == TYPE_NULL)
        {
            if (declared_type == null)
            {
                error_in_resolving("Must specify a type in the variable declaration with null value.", d->pos);
                return null;
            }
            else
            {
                result = declared_type;
            }
        }
        else
        {
            result = expr->type;
        }
    }

    return result;
}

type *resolve_function_decl(decl *d)
{
    assert(d->kind == DECL_FUNCTION);

    type *resolved_return_type = type_void;
    if (d->function.return_type)
    {
        resolved_return_type = resolve_typespec(d->function.return_type);
    }

    if (d->function.method_receiver)
    {
        resolve_typespec(d->function.method_receiver->type);
    }

    bool variadic_declared = false;

    type **resolved_args = null;
    function_param_list *args = &d->function.params;
    for (size_t i = 0; i < args->param_count; i++)
    {
        function_param *p = &args->params[i];
        if (p->name != variadic_keyword)
        {
            type *t = resolve_typespec(p->type);
            buf_push(resolved_args, t);
        }
        else
        {           
            if (d->function.is_extern == false)
            {
                error_in_resolving("Variadic allowed only in extern functions", d->pos);
                return type_invalid;
            }
            else
            {                
                if (variadic_declared)
                {
                    error_in_resolving("There can only be one variadic argument", d->pos);
                    return type_invalid;
                }
                else if (i < args->param_count - 1)
                {
                    error_in_resolving("Variadic must be the last argument", d->pos);
                    return type_invalid;
                }
                else
                {
                    variadic_declared = true;
                }
            }
        }
    }

    type *result = get_function_type(resolved_args, buf_len(resolved_args), resolved_return_type);

    result->function.has_variadic_arg = variadic_declared;
    result->function.is_extern = d->function.is_extern;

    return result;
}

void resolve_symbol(symbol *s)
{
    if (s->state == SYMBOL_RESOLVED)
    {
        return;
    }
    else if (s->state == SYMBOL_RESOLVING)
    {
        error_in_resolving(
            xprintf("Cyclic dependency detected in resolving %s symbol", s->name), s->decl->pos);
        panic_mode = true;
        return;
    }

    assert(s->state == SYMBOL_UNRESOLVED);
    s->state = SYMBOL_RESOLVING;

    assert(s->decl);
    switch (s->kind)
    {
        case SYMBOL_CONST:
        {            
            resolved_expr *expr = resolve_expr(s->decl->const_decl.expr);
            if (expr)
            {
                s->type = expr->type;
                s->val = expr->val;
            }
        }
        break;
        case SYMBOL_VARIABLE:
        {
            s->type = resolve_variable_decl(s->decl);
            debug_breakpoint;
        }
        break;
        case SYMBOL_TYPE: // to obejmuje też enums...
        {       
            // nie musimy nic robić - unions i structs są resolved od razu
        }
        break;
        case SYMBOL_FUNCTION:
        {
            s->type = resolve_function_decl(s->decl);
            s->type->symbol = s;
        }
        break;
        default:
        {
            fatal("unimplemented symbol type");
        }
        break;
    }

    if (s->type)
    {
        s->state = SYMBOL_RESOLVED;        
        s->decl->resolved_type = s->type;
        buf_push(ordered_global_symbols, s);        
    }
    else
    {
        error_in_resolving(xprintf("Could not resolve identifier: %s", s->name), s->decl->pos);
        return;
    }
}

void resolve_stmt_block(stmt_block st_block, type *opt_ret_type)
{
    symbol *marker = enter_local_scope();

    for (size_t i = 0; i < st_block.stmts_count; i++)
    {
        stmt *st = st_block.stmts[i];
        resolve_stmt(st, opt_ret_type);
    }

    leave_local_scope(marker);
}

void resolve_stmt(stmt *st, type *opt_ret_type)
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
                resolved_expr *result = resolve_expected_expr(st->expr, opt_ret_type, true);
                if (result == null || result->type == null || result->type->kind == TYPE_NONE)
                {
                    error_in_resolving("Could not resolve return statement", st->pos);
                    return;
                }
                if (result && false == can_perform_cast(result->type, opt_ret_type, false))
                {                    
                    error_in_resolving(
                        xprintf("Return type mismatch. Got %s, expected %s",
                            pretty_print_type_name(result->type, false),
                            pretty_print_type_name(opt_ret_type, false)),
                        st->pos);
                    return;
                }
            }
            else
            {
                if (opt_ret_type != type_void)
                {
                    error_in_resolving("Empty return expression for function with non-void return type", st->pos);
                    return;
                }
            }
            break;
        }
        break;
        case STMT_IF_ELSE:
        {
            resolve_expr(st->if_else.cond_expr);
            resolve_stmt_block(st->if_else.then_block, opt_ret_type);
            if (st->if_else.else_stmt)
            {
                resolve_stmt(st->if_else.else_stmt, opt_ret_type);
            }
        }
        break;
        case STMT_WHILE:
        {
            if (null == st->while_stmt.cond_expr)
            {
                error_in_resolving("While clause must have an expression", st->pos);
            }
            else
            {
                resolve_expr(st->while_stmt.cond_expr);
            }
            resolve_stmt_block(st->while_stmt.stmts, opt_ret_type);
        }
        break;
        case STMT_DO_WHILE:
        {
            resolve_expr(st->do_while_stmt.cond_expr);
            resolve_stmt_block(st->do_while_stmt.stmts, opt_ret_type);
        }
        break;
        case STMT_FOR:
        {
            symbol *marker = enter_local_scope();

            if (st->for_stmt.init_stmt)
            {
                resolve_stmt(st->for_stmt.init_stmt, null);
            }

            if (st->for_stmt.cond_expr)
            {
                resolve_expr(st->for_stmt.cond_expr);
            }

            if (st->for_stmt.next_stmt)
            {
                resolve_stmt(st->for_stmt.next_stmt, null);
            }

            resolve_stmt_block(st->for_stmt.stmts, opt_ret_type);

            leave_local_scope(marker);
        }
        break;
        case STMT_DECL:
        {
            if (st->decl_stmt.decl->kind == DECL_VARIABLE)
            {
                assert(st->decl_stmt.decl->kind == DECL_VARIABLE);
                type *t = resolve_variable_decl(st->decl_stmt.decl);
                
                if (t == null)
                {
                    // jeśli resolve się nie udało, błąd został rzucony już wcześniej
                    return;
                }

                if (check_if_symbol_name_unused(st->decl_stmt.decl->name, st->pos))
                {
                    push_local_symbol(st->decl_stmt.decl->name, t);
                }
                
                st->decl_stmt.decl->resolved_type = t;
            }
            else
            {
                error_in_resolving("Const declarations not allowed in a function scope", st->pos);
                return;
            }
        }
        break;
        case STMT_ASSIGN:
        {
            resolved_expr *left = resolve_expr(st->assign.assigned_var_expr);
            if (left == null || left->type == TYPE_NONE)
            {
                error_in_resolving("Cannot assign to an expression of an unknown type", st->assign.assigned_var_expr->pos);
                return;
            }

            if (st->assign.value_expr)
            {                
                resolved_expr *right = resolve_expected_expr(st->assign.value_expr, left->type, false);
                if (right == null || right->type == TYPE_NONE)
                {
                    error_in_resolving("Cannot assign expression of an unknown type", st->assign.value_expr->pos);
                    return;
                }

                if (false == compare_types(left->type, right->type))
                {
                    if (left->type->kind == TYPE_LIST)
                    {
                        if (right->type->kind != left->type->list.base_type->kind)
                        {
                            error_in_resolving("list of different type", st->assign.value_expr->pos);
                            return;
                        }
                    }
                    else if (false == can_perform_cast(right->type, left->type, true))
                    {
                        error_in_resolving(
                            xprintf("Types do not match in assignment. Trying to assign %s to %s",
                                pretty_print_type_name(right->type, false), 
                                pretty_print_type_name(left->type, false)),
                            st->assign.assigned_var_expr->pos);
                        return;
                    }
                }
            }
            
            if (false == left->is_lvalue)
            {
                error_in_resolving("Cannot assign to non-lvalue", st->pos);
                return;
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
            resolve_stmt_block(st->block, opt_ret_type);
        }
        break;
        case STMT_SWITCH:
        {
            resolved_expr *cond_expr = resolve_expr(st->switch_stmt.var_expr);

            if (false == can_perform_cast(cond_expr->type, type_long, true))
            {
                error_in_resolving(xprintf(
                    "Condition expression in a switch statement should be an integer or an enumeration. Specified expression was %s",
                    pretty_print_type_name(cond_expr->type, false)), st->pos);
            }

            for (size_t i = 0; i < st->switch_stmt.cases_num; i++)
            {
                switch_case *cas = st->switch_stmt.cases[i];
                int64_t *values = null;
                for (size_t k = 0; k < cas->cond_exprs_num; k++)
                {
                    resolved_expr* case_expr = resolve_expr(cas->cond_exprs[k]);
                    if (false == case_expr->is_const)
                    {
                        error_in_resolving("Case expression in a switch statement should be a constant value", st->pos);
                    }

                    if (false == can_perform_cast(case_expr->type, type_long, true))
                    {
                        error_in_resolving(xprintf(
                            "Case expression in a switch statement should be an integer or an enumeration. Specified expression was %s",
                            pretty_print_type_name(case_expr->type, false)), st->pos);
                    }

                    buf_push(values, case_expr->val);
                }
                assert(buf_len(values) == cas->cond_exprs_num);
                cas->cond_exprs_vals = copy_buf_to_arena(arena, values);
                buf_free(values);

                resolve_stmt_block(cas->stmts, opt_ret_type);
            }
        }
        break;
        case STMT_DELETE:
        {
            // TODO: powinniśmy sprawdzić czy zmienna nie jest const albo zaalokowana globalnie
            resolved_expr *expr = resolve_expr(st->delete.expr);
            if (expr->type == null || expr->type->kind == TYPE_NONE)
            {
                error_in_resolving("Could not resolve delete statement", st->pos);
                return;
            }
        }
        break;

        case STMT_BREAK:
        case STMT_CONTINUE:
        break;
        
        default:
        {
            fatal("stmt kind not implemented");
        }
        break;
    }
}

void complete_function_body(symbol *s)
{
    assert(s->state == SYMBOL_RESOLVED);
    type *return_type = s->type->function.return_type;

    symbol *marker = enter_local_scope();

    if (s->decl->function.method_receiver)
    {
        function_param *rec = s->decl->function.method_receiver;
        if (check_if_symbol_name_unused(rec->name, rec->pos))
        {
            push_local_symbol(rec->name, resolve_typespec(rec->type));
        }
    }

    for (size_t i = 0; i < s->decl->function.params.param_count; i++)
    {
        function_param *p = &s->decl->function.params.params[i];        
        if (check_if_symbol_name_unused(p->name, p->pos))
        {
            push_local_symbol(p->name, resolve_typespec(p->type));
        }
    }
    
    size_t count = s->decl->function.stmts.stmts_count;
    for (size_t i = 0; i < count; i++)
    {
        stmt *st = s->decl->function.stmts.stmts[i];
        resolve_stmt(st, return_type);
    }

    leave_local_scope(marker);
}

void complete_symbol(symbol *sym)
{
    resolve_symbol(sym);
    if (panic_mode)
    {
        return;
    }

    if (sym->kind == SYMBOL_TYPE)
    {
        complete_type(sym->type);
    }
    
    else if (sym->kind == SYMBOL_FUNCTION)
    {
        // można to też wziąć z type
        if (false == sym->decl->function.is_extern)
        {
            complete_function_body(sym);
        }
    }

    if (sym->kind == SYMBOL_FUNCTION)
    {
        if (sym->next_overload)
        {
            complete_symbol(sym->next_overload);
        }
    }
}

symbol *get_entry_point(symbol **symbols)
{
    symbol *main_function = null;
    const char *main_str = str_intern("main");

    for (symbol **it = symbols;
        it != buf_end(symbols);
        it++)
    {
        symbol *sym = *it;
        if (sym->name == main_str)
        {
            if (main_function)
            {
                error_in_resolving("Only one function 'main' allowed", sym->decl->pos);
                return null;
            }
            else
            {
                main_function = sym;
            }
        }
    }

    if (main_function == null)
    {
        error_in_resolving("Entry point function 'main' not defined", (source_pos) { 0 });
        return null;
    }

    if (false == (main_function->mangled_name == str_intern("___main___0l___0s___0v")
        || main_function->mangled_name == str_intern("___main___0v")))
    {
        error_in_resolving(
            "Main function has an incorrect declaration. Allowed declarations are 'fn main()' and 'fn main(args: string[])'", 
            main_function->decl->pos);
        return null;
    }

    return main_function;
}

void init_installed_types()
{
    if (false == installed_types_initialized)
    {
        push_installed_symbol("void", type_void);
        push_installed_symbol("char", type_char);
        push_installed_symbol("int", type_int);
        push_installed_symbol("uint", type_uint);
        push_installed_symbol("long", type_long);
        push_installed_symbol("ulong", type_ulong);
        push_installed_symbol("float", type_float);
        push_installed_symbol("bool", type_bool);

        resolved_expr_invalid = get_resolved_lvalue_expr(type_invalid);
    }
    installed_types_initialized = true;
}

symbol **resolve(decl **declarations, bool check_entry_point)
{
    assert(arena != null);
    init_installed_types();

    for (size_t i = 0; i < buf_len(declarations); i++)
    {
        decl *d = declarations[i];
        push_symbol_from_decl(d);
    }

    for (symbol **it = global_symbols_list;
        it != buf_end(global_symbols_list);
        it++)
    {
        symbol *sym = *it;
        complete_symbol(sym);
        if (panic_mode)
        {
            break;
        }
    }

    if (check_entry_point && false == panic_mode)
    {
        get_entry_point(global_symbols_list);
    }
    
    return ordered_global_symbols;
}
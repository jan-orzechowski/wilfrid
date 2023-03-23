﻿#include "parsing.h"

expr *parse_expr(void);
typespec *parse_typespec(void);
typespec *push_typespec_name(source_pos pos, const char *name);

memory_arena *arena;

int *code;

void parsing_error(const char *error_text)
{
    error(error_text, tok.pos, tok.end - tok.start);
    ignore_tokens_until_newline();
}

bool is_name_reserved(const char *name)
{
    size_t len = strlen(name);
    if (len >= 3)
    {
        size_t i = 0;
        while (i < len)
        {
            if (name[i] == '_'
                && (i + 1 < len && name[i + 1] == '_')
                && (i + 2 < len && name[i + 2] == '_'))
            {
                return true;
            }
            i++;
        }
    }
    return false;
}

bool is_token_kind(token_kind kind)
{
    bool result = (tok.kind == kind);
    return result;
}

bool match_token_kind(token_kind kind)
{
    bool result = (tok.kind == kind);
    if (result)
    {
        next_lexed_token();
    }
    return result;
}

bool expect_token_kind(token_kind kind)
{
    bool result = match_token_kind(kind);
    if (false == result)
    {
        parsing_error(xprintf("Expected %s token, got %s", 
            get_token_kind_name(kind), get_token_kind_name(tok.kind)));
    }
    return result;
}

bool is_assign_operation(token_kind kind)
{
    bool result = (kind >= TOKEN_FIRST_ASSIGN_OPERATOR && kind <= TOKEN_LAST_ASSIGN_OPERATOR);
    return result;
}

bool is_multiplicative_operation(token_kind kind)
{
    bool result = (kind >= TOKEN_FIRST_MUL_OPERATOR && kind <= TOKEN_LAST_MUL_OPERATOR);
    return result;
}

bool is_additive_operation(token_kind kind)
{
    bool result = (kind >= TOKEN_FIRST_ADD_OPERATOR && kind <= TOKEN_LAST_ADD_OPERATOR);
    return result;
}

bool is_comparison_operation(token_kind kind)
{
    bool result = (kind >= TOKEN_FIRST_CMP_OPERATOR && kind <= TOKEN_LAST_CMP_OPERATOR);
    return result;
}

expr *push_int_expr(source_pos pos, int value)
{
    expr *result = push_struct(arena, expr);
    result->kind = EXPR_INT;
    result->number_value = value;
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

expr *push_sizeof_expr(source_pos pos, expr *e)
{
    expr *result = push_struct(arena, expr);
    result->kind = EXPR_SIZEOF;
    result->size_of.expr = e;
    result->pos = pos;
    return result;
}

expr *push_cast_expr(source_pos pos, const char *type_name, expr *e)
{
    expr *result = push_struct(arena, expr);
    result->kind = EXPR_CAST;
    result->cast.type = push_typespec_name(pos, type_name);
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

const char *parse_identifier(void)
{
    const char *result = null;
    if (is_token_kind(TOKEN_NAME))
    {
        result = tok.name;
        if (is_name_reserved(result))
        {
            parsing_error("Identifiers with triple underscore ('___') are reserved");
        }
        next_lexed_token();
    }
    else
    {
        parsing_error(xprintf("Name expected, got %s", get_token_kind_name(tok.kind)));
    }
    return result;
}

typespec *parse_basic_typespec(void)
{
    typespec *t = null;

    if (is_token_kind(TOKEN_NAME))
    {
        t = push_typespec_name(tok.pos, tok.name);
        next_lexed_token();
    }
    else if (is_token_kind(TOKEN_KEYWORD) && tok.name == fn_keyword)
    {
        t = push_struct(arena, typespec);
        t->kind = TYPESPEC_FUNCTION;
        t->pos = tok.pos;
        next_lexed_token();

        expect_token_kind(TOKEN_LEFT_PAREN);
        {
            typespec **params = null;
            typespec *param = null;
            do
            {
                param = parse_typespec();
                if (param)
                {
                    buf_push(params, param);
                    if (is_token_kind(TOKEN_COMMA))
                    {
                        next_lexed_token();
                    }
                    else
                    {
                        break;
                    }
                }
            }
            while (param);

            t->function.param_count = buf_len(params);
            if (t->function.param_count > 0)
            {
                t->function.param_types = copy_buf_to_arena(arena, params);
                buf_free(params);
            }
        }

        expect_token_kind(TOKEN_RIGHT_PAREN);

        if (match_token_kind(TOKEN_COLON))
        {
            t->function.ret_type = parse_typespec();
        }        
    }
    else if (match_token_kind(TOKEN_LEFT_PAREN))
    {
        t = parse_typespec();
        expect_token_kind(TOKEN_RIGHT_PAREN);
    }
    return t;
}

typespec *parse_typespec(void)
{
    typespec *t = parse_basic_typespec();
    while(is_token_kind(TOKEN_MUL) || is_token_kind(TOKEN_LEFT_BRACKET))
    {
        if (is_token_kind(TOKEN_MUL))
        {
            typespec *base_t = t;
            t = push_struct(arena, typespec);
            t->kind = TYPESPEC_POINTER;
            t->pointer.base_type = base_t;
            t->pos = tok.pos;
            next_lexed_token();
        }
        else
        {           
            typespec *base_t = t;
            t = push_struct(arena, typespec);
            t->pos = tok.pos;
            next_lexed_token();

            if (match_token_kind(TOKEN_RIGHT_BRACKET))
            {
                t->kind = TYPESPEC_LIST;
                t->list.base_type = base_t;
            }
            else
            {
                t->kind = TYPESPEC_ARRAY;
                t->array.size_expr = parse_expr();
                t->array.base_type = base_t;
                expect_token_kind(TOKEN_RIGHT_BRACKET);
            }      
        }
    }
    return t;
}

expr *parse_compound_literal(void)
{
    expr *e = null;
    if (match_token_kind(TOKEN_LEFT_BRACE))
    {
        e = push_struct(arena, expr);
        e->kind = EXPR_COMPOUND_LITERAL;
        e->pos = tok.pos;

        compound_literal_field **fields = null;
        compound_literal_field *field = null;

        do
        {
            field = null;

            expr *val = parse_expr();
            if (val)
            {
                field = push_struct(arena, compound_literal_field);
                if (val->kind == EXPR_NAME)
                {                
                    if (match_token_kind(TOKEN_ASSIGN))
                    {
                        field->field_name = val->name;
                        field->expr = parse_expr();
                    }
                    else
                    {
                        field->expr = val;
                    }
                }
                else
                {
                    field->expr = val;
                }

                // przecinek jest opcjonalny
                match_token_kind(TOKEN_COMMA);

                buf_push(fields, field);
            }
        }
        while (field);

        if (buf_len(fields) > 0)
        {
            e->compound.fields = copy_buf_to_arena(arena, fields);
            e->compound.fields_count = buf_len(fields);
            buf_free(fields);
        }

        expect_token_kind(TOKEN_RIGHT_BRACE);
    }
    return e;
}

expr *parse_base_expr(void)
{
    expr *result = null;
    if (is_token_kind(TOKEN_INT))
    {
        result = push_int_expr(tok.pos, tok.val);
        next_lexed_token();
    }
    else if (is_token_kind(TOKEN_NAME))
    {
        result = push_name_expr(tok.pos, tok.name);
        next_lexed_token();
    }
    else if (is_token_kind(TOKEN_STRING))
    {
        result = push_string_expr(tok.pos, tok.string_val);
        next_lexed_token();
    }
    else if (is_token_kind(TOKEN_KEYWORD))
    {
        const char *keyword = tok.name;
        source_pos pos = tok.pos;
        if (keyword == new_keyword)
        {
            next_lexed_token();
            typespec *t = parse_typespec();
            result = push_new_expr(pos, t);
        }
        else if (keyword == auto_keyword)
        {
            next_lexed_token();
            typespec *t = parse_typespec();
            result = push_auto_expr(pos, t);
        }      
        else if (keyword == sizeof_keyword)
        {
            next_lexed_token();
            expect_token_kind(TOKEN_LEFT_PAREN);
            expr *e = parse_expr();            
            result = push_sizeof_expr(pos, e);
            expect_token_kind(TOKEN_RIGHT_PAREN);
        }
        else if (keyword == null_keyword)
        {
            result = push_null_expr(pos);
            next_lexed_token();
        }
        else if (keyword == true_keyword)
        {
            result = push_bool_expr(pos, true);
            next_lexed_token();
        }
        else if (keyword == false_keyword)
        {
            result = push_bool_expr(pos, false);
            next_lexed_token();
        }
    }
    else if (is_token_kind(TOKEN_LEFT_BRACE))
    {
        result = parse_compound_literal();
    }
    else if (match_token_kind(TOKEN_LEFT_PAREN))
    {
        source_pos pos = tok.pos;
        result = parse_expr();
        const char *type_name = result->name;
        expect_token_kind(TOKEN_RIGHT_PAREN);
        
        if (result->kind != EXPR_NAME)
        {
            // to znaczy, że to nie jest cast, tylko expression
            debug_breakpoint;
        }
        else
        {
            if (match_token_kind(TOKEN_LEFT_PAREN))
            {
                if (result->kind != EXPR_NAME)
                {
                    parsing_error("Only names are supported in casts");
                    return null;
                }

                expr *e = parse_expr();
                result = push_cast_expr(pos, type_name, e);

                expect_token_kind(TOKEN_RIGHT_PAREN);
            }
            else if (is_token_kind(TOKEN_LEFT_BRACE))
            {
                if (result->kind != EXPR_NAME)
                {
                    parsing_error("Only names are supported as explicit compound literal types");
                    return null;
                }

                result = parse_compound_literal();

                typespec *t = push_typespec_name(tok.pos, type_name);
                result->compound.type = t;
            }
            else
            {
                if (result->kind != EXPR_NAME)
                {
                    parsing_error("Only names are supported in casts");
                    return null;
                }

                expr *e = parse_base_expr();
                if (e == null)
                {
                    parsing_error("There is no expression to cast");
                    return null;
                }

                result = push_cast_expr(pos, type_name, e);

                debug_breakpoint;
            }
        }
    }
    return result;
}

void parse_function_call_arguments(expr *e)
{
    assert(e->kind == EXPR_CALL);
    while (false == is_token_kind(TOKEN_RIGHT_PAREN))
    {
        // tutaj może się nie udawać sparsować argumentu w nieskończoność

        expr *arg = parse_expr();
        if (arg)
        {
            buf_push(e->call.args, arg);
        }
        else
        {
            parsing_error("Expected expression in a function call");
            break;
        }

        if (false == is_token_kind(TOKEN_RIGHT_PAREN))
        {
            expect_token_kind(TOKEN_COMMA);
        }
    }
    e->call.args_num = buf_len(e->call.args);
}

// przydałaby się lepsza nazwa na to
expr *parse_complex_expr(void)
{
    source_pos pos = tok.pos;
    expr *result = parse_base_expr();
    while (is_token_kind(TOKEN_LEFT_PAREN) 
        || is_token_kind(TOKEN_LEFT_BRACKET) 
        || is_token_kind(TOKEN_DOT))
    {
        expr *left_side = result;
        if (is_token_kind(TOKEN_LEFT_PAREN))
        {
            next_lexed_token();

            result = push_struct(arena, expr);
            result->kind = EXPR_CALL;
            result->call.function_expr = left_side;
            result->pos = pos;

            parse_function_call_arguments(result);

            expect_token_kind(TOKEN_RIGHT_PAREN);

            // szczególny przypadek - 'new x()' i 'new x' traktujemy tak samo
            if (result->call.args_num == 0 
                && (result->call.function_expr->kind == EXPR_NEW
                    || result->call.function_expr->kind == EXPR_AUTO))
            {
                result = result->call.function_expr;
            }
        }
        else if (is_token_kind(TOKEN_LEFT_BRACKET))
        {
            next_lexed_token();

            result = push_struct(arena, expr);
            result->kind = EXPR_INDEX;
            result->index.array_expr = left_side;
            result->pos = tok.pos; // inaczej niż przy . i (
            
            expr *index_expr = parse_expr();
            result->index.index_expr = index_expr;

            expect_token_kind(TOKEN_RIGHT_BRACKET);
        } 
        else if (is_token_kind(TOKEN_DOT))
        {
            next_lexed_token();           
            const char *identifier = tok.name;
            
            next_lexed_token();                       
            if (match_token_kind(TOKEN_LEFT_PAREN))
            {
                // method call
                result = push_struct(arena, expr);
                result->kind = EXPR_CALL;
                result->call.function_expr = push_name_expr(pos, identifier);
                result->call.method_receiver = left_side;
                result->pos = pos;

                parse_function_call_arguments(result);

                expect_token_kind(TOKEN_RIGHT_PAREN);              
            }
            else
            {
                // field access
                result = push_struct(arena, expr);
                result->kind = EXPR_FIELD;
                result->field.expr = left_side;
                result->field.field_name = identifier;
                result->pos = tok.pos;
            }            
        }
    }
    return result;
}

expr *parse_unary_expr(void)
{
    expr *e = parse_complex_expr();
    source_pos pos = tok.pos;
    if (e == null)
    {
        if (match_token_kind(TOKEN_ADD))
        {
            e = push_unary_expr(pos, TOKEN_ADD, parse_base_expr());
        }
        else if (match_token_kind(TOKEN_SUB))
        {
            e = push_unary_expr(pos, TOKEN_SUB, parse_base_expr());
        }
        else if (match_token_kind(TOKEN_NOT))
        {
            e = push_unary_expr(pos, TOKEN_NOT, parse_base_expr());
        }
        else if (match_token_kind(TOKEN_BITWISE_NOT))
        {
            e = push_unary_expr(pos, TOKEN_BITWISE_NOT, parse_base_expr());
        }
        else if (match_token_kind(TOKEN_MUL)) // pointer dereference
        {
            e = push_unary_expr(pos, TOKEN_MUL, parse_base_expr());
        }
        else if (match_token_kind(TOKEN_BITWISE_AND)) // address of
        {
            // tutaj musimy mieć "greedy parsing" w prawo
            // np. &a[10] to adres 10. elementu, a nie 10 element w tabeli adresów
            e = push_unary_expr(pos, TOKEN_BITWISE_AND, parse_unary_expr());
        }
    }
    return e;
}

expr *parse_multiplicative_expr(void)
{
    expr *e = parse_unary_expr();
    while (is_multiplicative_operation(tok.kind))
    {
        source_pos pos = tok.pos;
        expr *left_expr = e;
        token_kind op = tok.kind;
        next_lexed_token();
        expr *right_expr = parse_unary_expr();

        e = push_binary_expr(pos, op, left_expr, right_expr);
    }
    return e;
}

expr *parse_additive_expr(void)
{
    expr *e = parse_multiplicative_expr();
    while (is_additive_operation(tok.kind))
    {
        source_pos pos = tok.pos;
        expr *left_expr = e;
        token_kind op = tok.kind;
        next_lexed_token();
        expr *right_expr = parse_multiplicative_expr();

        e = push_binary_expr(pos, op, left_expr, right_expr);
    }
    return e;
}

expr *parse_comparison_expr(void)
{
    expr *e = parse_additive_expr();
    while (is_comparison_operation(tok.kind))
    {
        source_pos pos = tok.pos;
        expr *left_expr = e;
        token_kind op = tok.kind;
        next_lexed_token();
        expr *right_expr = parse_additive_expr();

        e = push_binary_expr(pos, op, left_expr, right_expr);
    }
    return e;
}

expr *parse_and_expr(void)
{
    expr *e = parse_comparison_expr();
    while (is_token_kind(TOKEN_AND))
    {
        source_pos pos = tok.pos;
        expr *left_expr = e;
        token_kind op = tok.kind;
        next_lexed_token();
        expr *right_expr = parse_comparison_expr();

        e = push_binary_expr(pos, op, left_expr, right_expr);
    }
    return e;
}

expr *parse_or_expr(void)
{
    expr *e = parse_and_expr();
    while (is_token_kind(TOKEN_OR))
    {
        source_pos pos = tok.pos;
        expr *left_expr = e;
        token_kind op = tok.kind;
        next_lexed_token();
        expr *right_expr = parse_and_expr();

        e = push_binary_expr(pos, op, left_expr, right_expr);
    }
    return e;
}

expr *parse_ternary_expr(void)
{
    source_pos pos = tok.pos;
    expr *e = parse_or_expr();
    if (e != null)
    {
        if (is_token_kind(TOKEN_QUESTION))
        {
            next_lexed_token();
            expr *cond = e;

            expr *if_true_expr = parse_expr();
            expect_token_kind(TOKEN_COLON);
            expr *if_false_expr = parse_expr();

            e = push_struct(arena, expr);
            e->kind = EXPR_TERNARY;
            e->ternary.condition = cond;
            e->ternary.if_false = if_false_expr;
            e->ternary.if_true = if_true_expr;
            e->pos = pos;
        }
    }
    return e;
}

expr *parse_expr(void)
{
    expr *result = parse_ternary_expr(); 
    return result;
}

decl *parse_declaration(void);
stmt_block parse_statement_block(void);

stmt *parse_simple_statement(void)
{ 
    stmt *s = null; 
    expr *left_expr = parse_expr();

    if (is_assign_operation(tok.kind))
    {
        token_kind op = tok.kind;
        next_lexed_token();
        
        s = push_struct(arena, stmt);
        expr *e = parse_expr();
        s->kind = STMT_ASSIGN;
        s->assign.operation = op;
        s->assign.value_expr = e;
        s->assign.assigned_var_expr = left_expr;
    }
    else if (is_token_kind(TOKEN_INC) || is_token_kind(TOKEN_DEC))
    {
        token_kind op = tok.kind;
        next_lexed_token();

        s = push_struct(arena, stmt);
        s->kind = STMT_ASSIGN;
        s->assign.operation = op;
        s->assign.value_expr = 0;
        s->assign.assigned_var_expr = left_expr;
    }
    else 
    {
        s = push_struct(arena, stmt);
        s->kind = STMT_EXPR;
        s->expr = left_expr;
    }
   
    return s;
}

stmt *parse_statement(void);

void parse_switch_cases(switch_stmt *switch_stmt)
{
    bool default_case_defined = false;

    switch_case **cases = null;
    switch_case *c = null;

    do
    {
        c = null;
        bool case_is_default = false;
        expr **case_exprs = null;
        expr *e = null;
        
        do
        {
            e = null;

            if (is_token_kind(TOKEN_KEYWORD))
            {
                const char *keyword = tok.name;
                if (keyword == case_keyword)
                {               
                    next_lexed_token();
                    e = parse_expr();
                    expect_token_kind(TOKEN_COLON);
                    buf_push(case_exprs, e);
                }
                else if (keyword == default_keyword
                    && false == default_case_defined)
                {
                    next_lexed_token();
                    case_is_default = true;
                    default_case_defined = true;
                    expect_token_kind(TOKEN_COLON);
                }
                else
                {
                    parsing_error("Switch statement error: 'case' or 'default' keywords expected");
                }
            }
        }
        while (e);

        if (buf_len(case_exprs) > 0)
        {
            c = push_struct(arena, switch_case);
            c->cond_exprs = copy_buf_to_arena(arena, case_exprs);
            c->cond_exprs_num = buf_len(case_exprs);
            c->is_default = case_is_default;
        }

        buf_free(case_exprs);
                
        if (c || case_is_default)
        {
            if (c == 0) // przypadek, kiedy mieliśmy samo default
            {
                c = push_struct(arena, switch_case);
                c->is_default = true;
            }
            
            c->stmts = parse_statement_block();

            if (is_token_kind(TOKEN_KEYWORD)
                && tok.name == break_keyword)
            {
                c->fallthrough = false;
                next_lexed_token();
            }
            else
            {
                c->fallthrough = true;
            }

            buf_push(cases, c);
        }
    }
    while (c);

    if (buf_len(cases) > 0)
    {
        switch_stmt->cases_num = buf_len(cases);
        switch_stmt->cases = copy_buf_to_arena(arena, cases);     
    }

    buf_free(cases);
}

#define expect_token_kind_or_return(kind) if (false == expect_token_kind(kind)) { return null; }

stmt *parse_if_statement(void)
{
    stmt *s = null;
    if (tok.name == if_keyword)
    {
        s = push_struct(arena, stmt);
        s->kind = STMT_IF_ELSE;
        s->pos = tok.pos;
        next_lexed_token();

        expect_token_kind_or_return(TOKEN_LEFT_PAREN);
        s->if_else.cond_expr = parse_expr();
        if (s->if_else.cond_expr == null)
        {
            parsing_error("Expected expression in the if statement condition");
            return null;
        }
        expect_token_kind_or_return(TOKEN_RIGHT_PAREN);

        s->if_else.then_block = parse_statement_block();

        if (is_token_kind(TOKEN_KEYWORD) 
            && tok.name == else_keyword)
        {
            next_lexed_token();            
            s->if_else.else_stmt = parse_statement();           
        }
    }
    return s;
}

stmt *parse_statement(void)
{
    stmt *s = null;    
    source_pos pos = tok.pos;
    if (is_token_kind(TOKEN_KEYWORD))
    {
        const char *keyword = tok.name;
        if (keyword == return_keyword)
        {
            s = push_struct(arena, stmt);
            s->kind = STMT_RETURN;
            s->pos = pos;
            next_lexed_token();
            s->return_stmt.ret_expr = parse_expr();
        }
        else if (keyword == break_keyword)
        {
            s = push_struct(arena, stmt);
            s->kind = STMT_BREAK;
            s->pos = pos;
            next_lexed_token();
        }
        else if (keyword == continue_keyword)
        {
            s = push_struct(arena, stmt);
            s->kind = STMT_CONTINUE;
            s->pos = pos;
            next_lexed_token();
        }
        else if (keyword == let_keyword
            || keyword == const_keyword)
        {
            decl *d = parse_declaration();
            s = push_struct(arena, stmt);
            s->decl_stmt.decl = d;
            s->kind = STMT_DECL;
            s->pos = pos;
        }
        else if (keyword == for_keyword)
        {
            s = push_struct(arena, stmt);
            s->kind = STMT_FOR;
            s->pos = pos;

            next_lexed_token();

            expect_token_kind_or_return(TOKEN_LEFT_PAREN);

            s->for_stmt.init_stmt = parse_statement();

            expect_token_kind_or_return(TOKEN_COMMA);

            s->for_stmt.cond_expr = parse_expr();

            expect_token_kind_or_return(TOKEN_COMMA);

            s->for_stmt.next_stmt = parse_statement();

            expect_token_kind_or_return(TOKEN_RIGHT_PAREN);

            s->for_stmt.stmts = parse_statement_block();
        }
        else if (keyword == if_keyword)
        {
            s = parse_if_statement();
        }
        else if (keyword == do_keyword)
        {
            s = push_struct(arena, stmt);
            s->kind = STMT_DO_WHILE;
            s->pos = pos;

            next_lexed_token();

            s->do_while_stmt.stmts = parse_statement_block();
            
            if (is_token_kind(TOKEN_KEYWORD) 
                && tok.name == while_keyword)
            {
                next_lexed_token();
                expect_token_kind_or_return(TOKEN_LEFT_PAREN);
                s->do_while_stmt.cond_expr = parse_expr();
                expect_token_kind_or_return(TOKEN_RIGHT_PAREN);
            }
            else
            {
                parsing_error("Missing while clause in do while statement\n");
            }
        }
        else if (keyword == while_keyword)
        {
            s = push_struct(arena, stmt);
            s->kind = STMT_WHILE;
            s->pos = pos;

            next_lexed_token();

            expect_token_kind_or_return(TOKEN_LEFT_PAREN);
            s->while_stmt.cond_expr = parse_expr();
            expect_token_kind_or_return(TOKEN_RIGHT_PAREN);

            s->while_stmt.stmts = parse_statement_block();
        }
        else if (keyword == switch_keyword)
        {
            s = push_struct(arena, stmt);
            s->kind = STMT_SWITCH;
            s->pos = pos;

            next_lexed_token();

            expect_token_kind_or_return(TOKEN_LEFT_PAREN);
            s->switch_stmt.var_expr = parse_expr();
            expect_token_kind_or_return(TOKEN_RIGHT_PAREN);

            expect_token_kind_or_return(TOKEN_LEFT_BRACE);
            parse_switch_cases(&s->switch_stmt);
            expect_token_kind_or_return(TOKEN_RIGHT_BRACE);
        }
        else if (keyword == delete_keyword)
        {
            s = push_struct(arena, stmt);
            s->kind = STMT_DELETE;
            s->pos = pos;
            next_lexed_token();
            s->delete.expr = parse_expr();
        }
        else if (keyword == fn_keyword)
        {
            parsing_error("Function declaration is not allowed in another function scope");
        }
        else if (keyword == enum_keyword)
        {
            parsing_error("Enum declaration is not allowed in a function scope");
            ignore_tokens_until_next_block();
        }
        else if (keyword == false_keyword || keyword == true_keyword)
        {
            parsing_error("False/true can be used only as an expression");
        }
        else
        {
            parsing_error(xprintf("Keyword %s not allowed in statement", keyword));
        }
    }
    else if (is_token_kind(TOKEN_NAME))
    {        
        decl *d = parse_declaration();
        if (d)
        {
            s = push_struct(arena, stmt);
            s->kind = STMT_DECL;
            s->pos = pos;
            s->decl_stmt.decl = d;

            debug_breakpoint;
        }
        else
        {
            s = parse_simple_statement();
            s->pos = pos;
        }
    }
    else if (is_token_kind(TOKEN_MUL)) // pointer dereference
    {
        s = parse_simple_statement();
        s->pos = pos;
    }
    else if (is_token_kind(TOKEN_LEFT_BRACE))
    {
        stmt_block block = parse_statement_block();
        if (block.stmts_count > 0)
        {
            s = push_struct(arena, stmt);
            s->kind = STMT_BLOCK;
            s->block = block;
            s->pos = pos;
        }
    }
    else
    {
        parsing_error(xprintf("Unexpected token type: %s", get_token_kind_name(tok.kind)));
    }
    return s;
}

stmt_block parse_statement_block(void)
{
    expect_token_kind(TOKEN_LEFT_BRACE);

    if (is_token_kind(TOKEN_RIGHT_BRACE))
    {
        // specjalny przypadek - pusty blok
        next_lexed_token();
        return (stmt_block){0};
    }

    stmt_block result = {0};
    stmt **statements = null;
    
    for (size_t attempts = 0; attempts < 3; attempts++)
    {
        stmt *st = parse_statement();
        if (st)
        {
            attempts = 0;
            buf_push(statements, st);
        }

        if (is_token_kind(TOKEN_RIGHT_BRACE)
            || is_token_kind(TOKEN_EOF))
        {
            break;
        }
    }

    size_t count = buf_len(statements);
    if (count > 0)
    {
        result.stmts = copy_buf_to_arena(arena, statements);
        result.stmts_count = count;
    }

    expect_token_kind(TOKEN_RIGHT_BRACE);

    return result;
}

function_param parse_function_param(void)
{
    function_param p = {0};
    if (is_token_kind(TOKEN_KEYWORD))
    {
        if (tok.name == variadic_keyword)
        {
            p.name = variadic_keyword;
            next_lexed_token();
        }
    }
    else if (is_token_kind(TOKEN_NAME))
    {
        p.pos = tok.pos;
        p.name = parse_identifier();

        expect_token_kind(TOKEN_COLON);

        if (is_token_kind(TOKEN_NAME))
        {
            p.type = parse_typespec();
        }
        else
        {
            parsing_error("Expected typespec");
        }
    }
    return p;
}

function_param_list parse_function_param_list()
{
    expect_token_kind(TOKEN_LEFT_PAREN);

    function_param *params = null;
    while (is_token_kind(TOKEN_NAME) || is_token_kind(TOKEN_KEYWORD))
    {
        function_param p = parse_function_param();
        if (p.name == null)
        {
            break;
        }

        buf_push(params, p);

        if (is_token_kind(TOKEN_COMMA))
        {           
            next_lexed_token();
        }
        else
        {
            break;
        }
    }

    // sprawdzenie, czy się nie powtarzają
    for (size_t i = 0; i < buf_len(params); i++)
    {
        for (size_t j = 0; j < buf_len(params); j++)
        {
            if (i != j)
            {
                if (params[i].name == params[j].name)
                {
                    parsing_error("Two or more parameters with the same name in the function declaration");
                }
            }
        }
    }

    function_param_list result = { 0 };
    result.param_count = (int)buf_len(params);
    if (result.param_count > 0)
    {
        result.params = copy_buf_to_arena(arena, params);
        buf_free(params);
    }

    expect_token_kind(TOKEN_RIGHT_PAREN);

    return result;
}

aggregate_field parse_aggregate_field(void)
{
    aggregate_field result = {0};
    if (is_token_kind(TOKEN_NAME))
    {        
        result.pos = tok.pos;
        result.name = parse_identifier();

        expect_token_kind(TOKEN_COLON);

        if (is_token_kind(TOKEN_NAME))
        {
            result.type = parse_typespec();
        }

        if (is_token_kind(TOKEN_COMMA))
        {
            next_lexed_token();
        }
    }
    return result;
}

void parse_aggregate_fields(aggregate_decl *decl)
{
    aggregate_field *fields = null;

    aggregate_field new_field = parse_aggregate_field();
    while (new_field.type)
    {
        buf_push(fields, new_field);
        new_field = parse_aggregate_field();
    }

    // sprawdzenie, czy się nie powtarzają
    for (size_t i = 0; i < buf_len(fields); i++)
    {
        for (size_t j = 0; j < buf_len(fields); j++)
        {
            if (i != j)
            {
                if (fields[i].name == fields[j].name)
                {
                    parsing_error("Two or more fields with the same name in the struct declaration");
                }
            }
        }
    }

    decl->fields_count = (int)buf_len(fields);
    if (decl->fields_count > 0)
    {
        decl->fields = copy_buf_to_arena(arena, fields);
        buf_free(fields);
    }
}

enum_value parse_enum_value(void)
{
    enum_value result = { 0 };
    if (is_token_kind(TOKEN_NAME))
    {
        result.pos = tok.pos;
        result.name = parse_identifier();

        if (is_token_kind(TOKEN_ASSIGN))
        {
            next_lexed_token();
            if (is_token_kind(TOKEN_INT))
            {
                result.value_set = true;
                result.value = tok.val;
                next_lexed_token();
            }
        }

        if (is_token_kind(TOKEN_COMMA))
        {
            next_lexed_token();
        }
    }
    return result;
}

void parse_enum(enum_decl *decl)
{
    enum_value *values = null;

    enum_value new_value = parse_enum_value();
    while (new_value.name)
    {
        buf_push(values, new_value);
        new_value = parse_enum_value();
    }

    decl->values_count = (size_t)buf_len(values);
    if (decl->values_count > 0)
    {
        decl->values = copy_buf_to_arena(arena, values);
        buf_free(values);
    }
}

decl *parse_declaration(void)
{
    decl *declaration = null;
    source_pos pos = tok.pos;
    if (is_token_kind(TOKEN_KEYWORD))
    {
        const char *decl_keyword = tok.name;
        if (decl_keyword == let_keyword)
        {
            declaration = push_struct(arena, decl);
            declaration->kind = DECL_VARIABLE;
            declaration->pos = pos;

            next_lexed_token();
            declaration->name = parse_identifier();
            
            if (match_token_kind(TOKEN_COLON_ASSIGN))
            {
                expr *expr = parse_expr();
                declaration->variable.expr = expr;
            }
            else
            {
                if (match_token_kind(TOKEN_COLON))
                {
                    declaration->variable.type = parse_typespec();
                    if (declaration->variable.type == 0)
                    {
                        parsing_error("Expected typespec after colon");
                    }
                }

                if (match_token_kind(TOKEN_ASSIGN))
                {
                    expr *expr = parse_expr();
                    declaration->variable.expr = expr;
                }
            }
        }
        else if (decl_keyword == const_keyword)
        {
            declaration = push_struct(arena, decl);
            declaration->kind = DECL_CONST;
            declaration->pos = pos;

            next_lexed_token();
            declaration->name = parse_identifier();

            if (expect_token_kind(TOKEN_ASSIGN))
            {
                expr *expr = parse_expr();
                declaration->const_decl.expr = expr;
            }
        }       
        else if (decl_keyword == struct_keyword
                || decl_keyword == union_keyword)
        {
            declaration = push_struct(arena, decl);
            declaration->kind = decl_keyword == struct_keyword ? DECL_STRUCT : DECL_UNION;
            declaration->pos = pos;

            next_lexed_token();
            declaration->name = parse_identifier();

            expect_token_kind(TOKEN_LEFT_BRACE);

            parse_aggregate_fields(&declaration->aggregate);

            expect_token_kind(TOKEN_RIGHT_BRACE);
        }
        else if (decl_keyword == fn_keyword)
        {
            declaration = push_struct(arena, decl);
            declaration->kind = DECL_FUNCTION;
            declaration->pos = pos;

            next_lexed_token();

            // reveiver method
            if (match_token_kind(TOKEN_LEFT_PAREN))
            {
                // fn (s : int) method_name () { }
                declaration->function.method_receiver = push_struct(arena, function_param);
                declaration->function.method_receiver->pos = tok.pos;
                declaration->function.method_receiver->name = parse_identifier();
                
                expect_token_kind(TOKEN_COLON);

                declaration->function.method_receiver->type = parse_typespec();

                if (match_token_kind(TOKEN_COMMA))
                {
                    parsing_error("Only one argument is allowed as a receiver of a method");
                }

                expect_token_kind(TOKEN_RIGHT_PAREN);
            }

            declaration->name = parse_identifier();
         
            declaration->function.params = parse_function_param_list();

            if (is_token_kind(TOKEN_COLON))
            {
                next_lexed_token();
                if (is_token_kind(TOKEN_NAME))
                {
                    declaration->function.return_type = parse_typespec();
                }
            }

            declaration->function.stmts = parse_statement_block();
        }
        else if (decl_keyword == extern_keyword)
        {
            next_lexed_token();
            if (false == is_token_kind(TOKEN_KEYWORD)
                || false == (tok.name == fn_keyword))
            {
                parsing_error("Only functions can be marked as extern");
            }

            declaration = push_struct(arena, decl);
            declaration->kind = DECL_FUNCTION;
            declaration->pos = pos;
            declaration->function.is_extern = true;

            next_lexed_token();

            declaration->name = parse_identifier();

            declaration->function.params = parse_function_param_list();

            if (is_token_kind(TOKEN_COLON))
            {
                next_lexed_token();
                if (is_token_kind(TOKEN_NAME))
                {
                    declaration->function.return_type = parse_typespec();
                }
            }

            if (is_token_kind(TOKEN_LEFT_BRACE))
            {
                parsing_error("Function body not allowed for extern functions");
            }
        }
        else if (decl_keyword == enum_keyword)
        {
            declaration = push_struct(arena, decl);
            declaration->kind = DECL_ENUM;
            declaration->pos = pos;

            next_lexed_token();
            declaration->name = parse_identifier();

            expect_token_kind(TOKEN_LEFT_BRACE);

            parse_enum(&declaration->enum_decl);

            expect_token_kind(TOKEN_RIGHT_BRACE);
        }
        else
        {
            parsing_error(xprintf("Unknown keyword: %s", decl_keyword));
        }
    }
    return declaration;
}

decl **lex_and_parse(char *filename, char *source)
{
    lex(filename, source);
    
    free_memory_arena(arena);
    arena = allocate_memory_arena(megabytes(50));

    decl **decl_array = null;
    decl *dec = null;
    for (size_t attempts = 0; attempts < 5; attempts++)
    {
        if (is_token_kind(TOKEN_NEWLINE))
        {
            next_lexed_token();
        }

        dec = parse_declaration();
        if (dec)
        {
            attempts = 0;
            buf_push(decl_array, dec);
        }
        
        if (attempts == 3)
        {
            ignore_tokens_until_next_block();
        }

        if (is_token_kind(TOKEN_EOF))
        {
            break;
        }

        // szczególny przypadek - string bez zamykającego cudzysłowu
        if (is_token_kind(TOKEN_STRING) 
            && lexed_token_index - 1 == buf_len(all_tokens))
        {
            parsing_error("String without matching ending quotation marks");
            break;
        }
    }

    if (buf_len(decl_array) == 0)
    {
        parsing_error("Could not parse any declaration");
    }

    return decl_array;
}
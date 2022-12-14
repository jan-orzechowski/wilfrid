﻿#include "vm.h"
#include "parsing.h"
#include "lexing.h"

memory_arena* arena;

int* code;

bool is_token_kind(token_kind kind)
{
    bool result = (token.kind == kind);
    return result;
}

bool match_token_kind(token_kind kind)
{
    bool result = (token.kind == kind);
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
        fatal("expected token of different kind: %s", get_token_kind_name(kind));
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

expr* push_int_expr(int value)
{
    expr* result = push_struct(arena, expr);
    result->kind = EXPR_INT;
    result->number_value = value;
    next_lexed_token();
    return result;
}

expr* push_name_expr(const char* name)
{
    expr* result = push_struct(arena, expr);
    result->kind = EXPR_NAME;
    result->name = str_intern(name);
    next_lexed_token();
    return result;
}

expr* push_sizeof_expr(expr* e)
{
    expr* result = push_struct(arena, expr);
    result->kind = EXPR_SIZEOF;
    result->size_of.expr = e;
    return result;
}

expr* push_unary_expr(token_kind operator, expr* operand)
{
    expr* result = push_struct(arena, expr);
    result->kind = EXPR_UNARY;
    result->unary.operator = operator;
    result->unary.operand = operand;
    return result;
}

expr* push_binary_expr(token_kind operator, expr* left, expr* right)
{
    expr* result = push_struct(arena, expr);
    result->kind = EXPR_BINARY;
    result->binary.operator = operator;
    result->binary.left = left;
    result->binary.right = right;
    return result;
}

expr* parse_expr(void);
typespec* parse_typespec(void);

typespec* parse_basic_typespec(void)
{
    typespec* t = 0;

    if (is_token_kind(TOKEN_NAME))
    {
        t = push_struct(arena, typespec);
        t->kind = TYPESPEC_NAME;
        t->name = token.name;
        next_lexed_token();
    }
    else if (is_token_kind(TOKEN_KEYWORD) && token.name == fn_keyword)
    {
        t = push_struct(arena, typespec);
        t->kind = TYPESPEC_FUNCTION;
        next_lexed_token();

        expect_token_kind(TOKEN_LEFT_PAREN);
        {
            typespec** params = 0;
            typespec* param = 0;
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

            debug_breakpoint;
        }        
    }
    else if (match_token_kind(TOKEN_LEFT_PAREN))
    {
        t = parse_typespec();
        expect_token_kind(TOKEN_RIGHT_PAREN);
    }
    return t;
}

typespec* parse_typespec(void)
{
    typespec* t = parse_basic_typespec();
    while(is_token_kind(TOKEN_MUL) || is_token_kind(TOKEN_LEFT_BRACKET))
    {
        if (is_token_kind(TOKEN_MUL))
        {
            typespec* base_t = t;
            t = push_struct(arena, typespec);
            t->kind = TYPESPEC_POINTER;
            t->pointer.base_type = base_t;
            next_lexed_token();
        }
        else
        {
            typespec* base_t = t;
            t = push_struct(arena, typespec);
            t->kind = TYPESPEC_ARRAY;
            next_lexed_token();
            t->array.size_expr = parse_expr();
            t->array.base_type = base_t;
            expect_token_kind(TOKEN_RIGHT_BRACKET);
        }
    }
    return t;
}

expr* parse_compound_literal(void)
{
    expr* e = 0;
    if (match_token_kind(TOKEN_LEFT_BRACE))
    {
        e = push_struct(arena, expr);
        e->kind = EXPR_COMPOUND_LITERAL;

        compound_literal_field** fields = 0;
        compound_literal_field* field = 0;

        do
        {
            field = 0;

            expr* val = parse_expr();
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

expr* parse_base_expr(void)
{
    expr* result = 0;
    if (is_token_kind(TOKEN_INT))
    {
        result = push_int_expr(token.val);
    }
    else if (is_token_kind(TOKEN_NAME))
    {
        result = push_name_expr(token.name);
    }
    else if (is_token_kind(TOKEN_KEYWORD))
    {
        const char* keyword = str_intern(token.name);
        if (keyword == sizeof_keyword)
        {
            next_lexed_token();
            expect_token_kind(TOKEN_LEFT_PAREN);

            expr* e = parse_expr();
            result = push_sizeof_expr(e);

            expect_token_kind(TOKEN_RIGHT_PAREN);
        }
    }
    else if (is_token_kind(TOKEN_LEFT_BRACE))
    {
        result = parse_compound_literal();        
    }
    else if (match_token_kind(TOKEN_LEFT_PAREN))
    {
        result = parse_expr();
        expect_token_kind(TOKEN_RIGHT_PAREN);
        
        if (is_token_kind(TOKEN_LEFT_BRACE))
        {
            expr* type_expr = result;
            result = parse_compound_literal();
            result->compound.type = type_expr->name;
        }
    }
    else
    {
        // błąd
    }
    
    return result;
}

// przydałaby się lepsza nazwa na to
expr* parse_complex_expr(void)
{    
    expr* result = parse_base_expr();
    while (is_token_kind(TOKEN_LEFT_PAREN) 
        || is_token_kind(TOKEN_LEFT_BRACKET) 
        || is_token_kind(TOKEN_DOT))
    {
        expr* left_side = result;
        if (is_token_kind(TOKEN_LEFT_PAREN))
        {
            next_lexed_token();

            result = push_struct(arena, expr);
            result->kind = EXPR_CALL;
            result->call.function_expr = left_side;

            while (false == is_token_kind(TOKEN_RIGHT_PAREN))
            {
                expr* arg = parse_expr();
                buf_push(result->call.args, arg);

                if (false == is_token_kind(TOKEN_RIGHT_PAREN))
                {
                    expect_token_kind(TOKEN_COMMA);
                }
            }
            result->call.args_num = buf_len(result->call.args);

            expect_token_kind(TOKEN_RIGHT_PAREN);
        }
        else if (is_token_kind(TOKEN_LEFT_BRACKET))
        {
            next_lexed_token();

            result = push_struct(arena, expr);
            result->kind = EXPR_INDEX;
            result->index.array_expr = left_side;
            
            expr* index_expr = parse_expr();
            result->index.index_expr = index_expr;

            expect_token_kind(TOKEN_RIGHT_BRACKET);
        } 
        else if (is_token_kind(TOKEN_DOT))
        {
            next_lexed_token();
            
            result = push_struct(arena, expr);
            result->kind = EXPR_FIELD;
            result->field.expr = left_side;
            result->field.field_name = token.name;
            next_lexed_token();
        }
    }
    return result;
}

expr* parse_unary_expr(void)
{
    expr* e = parse_complex_expr();
    if (e == NULL)
    {
        if (match_token_kind(TOKEN_ADD))
        {
            e = push_unary_expr(TOKEN_ADD, parse_base_expr());
        }
        else if (match_token_kind(TOKEN_SUB))
        {
            e = push_unary_expr(TOKEN_SUB, parse_base_expr());
        }
        else if (match_token_kind(TOKEN_NOT))
        {
            e = push_unary_expr(TOKEN_NOT, parse_base_expr());
        }
        else if (match_token_kind(TOKEN_BITWISE_NOT))
        {
            e = push_unary_expr(TOKEN_BITWISE_NOT, parse_base_expr());
        }
        else if (match_token_kind(TOKEN_MUL)) // pointer dereference
        {
            e = push_unary_expr(TOKEN_MUL, parse_base_expr());
        }
        else if (match_token_kind(TOKEN_BITWISE_AND)) // address of
        {
            // tutaj musimy mieć "greedy parsing" w prawo
            // np. &a[10] to adres 10. elementu, a nie 10 element w tabeli adresów
            e = push_unary_expr(TOKEN_BITWISE_AND, parse_expr());
        }
    }
    return e;
}

expr* parse_multiplicative_expr(void)
{
    expr* e = parse_unary_expr();
    while (is_multiplicative_operation(token.kind))
    {
        expr* left_expr = e;
        token_kind op = token.kind;
        next_lexed_token();
        expr* right_expr = parse_unary_expr();

        e = push_binary_expr(op, left_expr, right_expr);
    }
    return e;
}

expr* parse_additive_expr(void)
{
    expr* e = parse_multiplicative_expr();
    while (is_additive_operation(token.kind))
    {
        expr* left_expr = e;
        token_kind op = token.kind;
        next_lexed_token();
        expr* right_expr = parse_multiplicative_expr();

        e = push_binary_expr(op, left_expr, right_expr);
    }
    return e;
}

expr* parse_comparison_expr(void)
{
    expr* e = parse_additive_expr();
    while (is_comparison_operation(token.kind))
    {
        expr* left_expr = e;
        token_kind op = token.kind;
        next_lexed_token();
        expr* right_expr = parse_additive_expr();

        e = push_binary_expr(op, left_expr, right_expr);
    }
    return e;
}

expr* parse_and_expr(void)
{
    expr* e = parse_comparison_expr();
    while (is_token_kind(TOKEN_AND))
    {
        expr* left_expr = e;
        token_kind op = token.kind;
        next_lexed_token();
        expr* right_expr = parse_comparison_expr();

        e = push_binary_expr(op, left_expr, right_expr);
    }
    return e;
}

expr* parse_or_expr(void)
{
    expr* e = parse_and_expr();
    while (is_token_kind(TOKEN_OR))
    {
        expr* left_expr = e;
        token_kind op = token.kind;
        next_lexed_token();
        expr* right_expr = parse_and_expr();

        e = push_binary_expr(op, left_expr, right_expr);
    }
    return e;
}

expr* parse_ternary_expr(void)
{
    expr* e = parse_or_expr();

    if (e != 0)
    {
        if (is_token_kind(TOKEN_QUESTION))
        {
            next_lexed_token();
            expr* cond = e;

            expr* if_true_expr = parse_expr();
            expect_token_kind(TOKEN_COLON);
            expr* if_false_expr = parse_expr();

            e = push_struct(arena, expr);
            e->kind = EXPR_TERNARY;
            e->ternary.condition = cond;
            e->ternary.if_false = if_false_expr;
            e->ternary.if_true = if_true_expr;
        }
    }

    return e;
}

expr* parse_expr(void)
{
    expr* result = parse_ternary_expr(); 
    return result;
}

decl* parse_declaration(void);
decl* parse_declaration_optional(void);
stmt_block parse_statement_block(void);

stmt* parse_simple_statement(void)
{ 
    stmt* s = 0; 
    expr* left_expr = parse_expr();

    if (is_assign_operation(token.kind))
    {
        token_kind op = token.kind;
        next_lexed_token();
        
        s = push_struct(arena, stmt);
        expr* e = parse_expr();
        s->kind = STMT_ASSIGN;
        s->assign.operation = op;
        s->assign.value_expr = e;
        s->assign.assigned_var_expr = left_expr;
    }
    else if (is_token_kind(TOKEN_INC) || is_token_kind(TOKEN_DEC))
    {
        token_kind op = token.kind;
        next_lexed_token();

        s = push_struct(arena, stmt);
        s->kind = STMT_ASSIGN;
        s->assign.operation = op;
        s->assign.value_expr = 0;
        s->assign.assigned_var_expr = left_expr;
    }
    else 
    {
        next_lexed_token();

        s = push_struct(arena, stmt);
        expr* e = parse_expr();
        s->kind = STMT_EXPR;
        s->expr = e;
    }
   
    return s;
}

stmt* parse_statement(void);

void parse_switch_cases(switch_stmt* switch_stmt)
{
    bool default_case_defined = false;

    switch_case** cases = 0;
    switch_case* c = 0;

    do
    {
        c = 0;
        bool case_is_default = false;
        expr** case_exprs = 0;
        expr* e = 0;
        
        do
        {
            e = 0;

            if (is_token_kind(TOKEN_KEYWORD))
            {
                const char* keyword = str_intern(token.name);
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
                    fatal("switch statement error: case or default keyword expected");
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
        
        if (c)
        {
            expect_token_kind(TOKEN_LEFT_BRACE);
            c->stmts = parse_statement_block();
            expect_token_kind(TOKEN_RIGHT_BRACE);

            if (is_token_kind(TOKEN_KEYWORD)
                && str_intern(token.name) == break_keyword)
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

stmt* parse_if_statement(void)
{
    stmt* s = 0;
    if (str_intern(token.name) == if_keyword)
    {
        s = push_struct(arena, stmt);
        s->kind = STMT_IF_ELSE;
        next_lexed_token();

        expect_token_kind(TOKEN_LEFT_PAREN);
        s->if_else.cond_expr = parse_expr();
        expect_token_kind(TOKEN_RIGHT_PAREN);

        expect_token_kind(TOKEN_LEFT_BRACE);
        s->if_else.then_block = parse_statement_block();
        expect_token_kind(TOKEN_RIGHT_BRACE);

        if (is_token_kind(TOKEN_KEYWORD) 
            && str_intern(token.name) == else_keyword)
        {
            next_lexed_token();            
            s->if_else.else_stmt = parse_statement();           
        }
    }
    return s;
}

stmt* parse_statement(void)
{
    stmt* s = 0;    
    if (is_token_kind(TOKEN_KEYWORD))
    {
        const char* keyword = str_intern(token.name);
        if (keyword == return_keyword)
        {
            s = push_struct(arena, stmt);
            s->kind = STMT_RETURN;
            next_lexed_token();
            s->return_stmt.ret_expr = parse_expr();
        }
        else if (keyword == break_keyword)
        {
            s = push_struct(arena, stmt);
            s->kind = STMT_BREAK;
            next_lexed_token();
        }
        else if (keyword == continue_keyword)
        {
            s = push_struct(arena, stmt);
            s->kind = STMT_CONTINUE;
            next_lexed_token();
        }
        else if (keyword == let_keyword
            || keyword == const_keyword)
        {
            decl* d = parse_declaration();
            s = push_struct(arena, stmt);
            s->decl.decl = d;
            s->kind = STMT_DECL;
        }
        else if (keyword == for_keyword)
        {
            s = push_struct(arena, stmt);
            s->kind = STMT_FOR;
            next_lexed_token();

            expect_token_kind(TOKEN_LEFT_PAREN);

            s->for_stmt.init_decl = parse_declaration();

            expect_token_kind(TOKEN_SEMICOLON);

            s->for_stmt.cond_expr = parse_expr();

            expect_token_kind(TOKEN_SEMICOLON);

            s->for_stmt.incr_stmt = parse_statement();

            expect_token_kind(TOKEN_RIGHT_PAREN);

            expect_token_kind(TOKEN_LEFT_BRACE);
            s->for_stmt.stmts = parse_statement_block();
            expect_token_kind(TOKEN_RIGHT_BRACE);
        }
        else if (keyword == if_keyword)
        {
            s = parse_if_statement();
        }
        else if (keyword == do_keyword)
        {
            s = push_struct(arena, stmt);
            s->kind = STMT_DO_WHILE;
            next_lexed_token();

            expect_token_kind(TOKEN_LEFT_BRACE);
            s->do_while_stmt.stmts = parse_statement_block();
            expect_token_kind(TOKEN_RIGHT_BRACE);
            
            if (is_token_kind(TOKEN_KEYWORD) 
                && str_intern(token.name) == while_keyword)
            {
                next_lexed_token();
                expect_token_kind(TOKEN_LEFT_PAREN);
                s->do_while_stmt.cond_expr = parse_expr();
                expect_token_kind(TOKEN_RIGHT_PAREN);
            }
            else
            {
                fatal("missing while clause in do while\n");
            }
        }
        else if (keyword == while_keyword)
        {
            s = push_struct(arena, stmt);
            s->kind = STMT_WHILE;
            next_lexed_token();

            expect_token_kind(TOKEN_LEFT_PAREN);
            s->while_stmt.cond_expr = parse_expr();
            expect_token_kind(TOKEN_RIGHT_PAREN);

            expect_token_kind(TOKEN_LEFT_BRACE);
            s->while_stmt.stmts = parse_statement_block();
            expect_token_kind(TOKEN_RIGHT_BRACE);
        }
        else if (keyword == switch_keyword)
        {
            s = push_struct(arena, stmt);
            s->kind = STMT_SWITCH;
            next_lexed_token();

            expect_token_kind(TOKEN_LEFT_PAREN);
            s->switch_stmt.var_expr = parse_expr();
            expect_token_kind(TOKEN_RIGHT_PAREN);

            expect_token_kind(TOKEN_LEFT_BRACE);
            parse_switch_cases(&s->switch_stmt);
            expect_token_kind(TOKEN_RIGHT_BRACE);
        }
    }
    else if (is_token_kind(TOKEN_NAME))
    {        
        decl* d = parse_declaration_optional();
        if (d)
        {
            s = push_struct(arena, stmt);
            s->kind = STMT_DECL;
            s->decl.decl = d;

            debug_breakpoint;
        }
        else
        {
            s = parse_simple_statement();
        }
    }
    else if (is_token_kind(TOKEN_LEFT_BRACE))
    {
        next_lexed_token();

        stmt_block block = parse_statement_block();
        if (block.stmts_count > 0)
        {
            s = push_struct(arena, stmt);
            s->kind = STMT_BLOCK;
            s->block = block;
        }

        expect_token_kind(TOKEN_RIGHT_BRACE);
    }

    return s;
}

stmt_block parse_statement_block(void)
{
    stmt_block result = {0};
    stmt** buf = 0;

    stmt* s = parse_statement();
    while (s)
    {
        buf_push(buf, s);
        s = parse_statement();
    }

    int s_count = (int)buf_len(buf);
    if (s_count > 0)
    {
        result.stmts = copy_buf_to_arena(arena, buf);
        result.stmts_count = s_count;
    }
    return result;
}

function_param parse_function_param(void)
{
    function_param p = {0};
    if (is_token_kind(TOKEN_NAME))
    {
        p.name = token.name;

        next_lexed_token();
        expect_token_kind(TOKEN_COLON);

        if (is_token_kind(TOKEN_NAME))
        {
            p.type = parse_typespec();
        }
    }
    return p;
}

function_param_list parse_function_param_list(void)
{
    function_param* params = NULL;

    while (is_token_kind(TOKEN_NAME))
    {
        function_param p = parse_function_param();
        if (p.name != NULL)
        {
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
        else
        {
            break;
        }
    }

    function_param_list result = { 0 };
    result.param_count = (int)buf_len(params);
    if (result.param_count > 0)
    {
        result.params = copy_buf_to_arena(arena, params);
        buf_free(params);
    }

    return result;
}

aggregate_field parse_aggregate_field(void)
{
    aggregate_field result = {0};
    if (is_token_kind(TOKEN_NAME))
    {        
        result.name = token.name;

        next_lexed_token();
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

void parse_aggregate_fields(aggregate_decl* decl)
{
    aggregate_field* fields = 0;

    aggregate_field new_field = parse_aggregate_field();
    while (new_field.type)
    {
        buf_push(fields, new_field);
        new_field = parse_aggregate_field();
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
        result.name = token.name;
        next_lexed_token();

        if (is_token_kind(TOKEN_ASSIGN))
        {
            next_lexed_token();
            if (is_token_kind(TOKEN_INT))
            {
                result.value_set = true;
                result.value = token.val;
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

void parse_enum(enum_decl* decl)
{
    enum_value* values = 0;

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

decl* parse_declaration_optional(void)
{
    decl* declaration = NULL;
    if (is_token_kind(TOKEN_KEYWORD))
    {
        const char* decl_keyword = str_intern(token.name);
        if (decl_keyword == let_keyword)
        {
            declaration = push_struct(arena, decl);
            declaration->kind = DECL_VARIABLE;

            next_lexed_token();
            if (is_token_kind(TOKEN_NAME))
            {
                declaration->name = token.name;
                next_lexed_token();
            }
            
            if (match_token_kind(TOKEN_COLON_ASSIGN))
            {
                expr* expr = parse_expr();
                declaration->variable.expr = expr;
            }
            else
            {
                if (match_token_kind(TOKEN_COLON))
                {
                    declaration->variable.type = parse_typespec();
                }

                if (match_token_kind(TOKEN_ASSIGN))
                {
                    expr* expr = parse_expr();
                    declaration->variable.expr = expr;
                }
            }
        }
        else if (decl_keyword == const_keyword)
        {
            declaration = push_struct(arena, decl);
            declaration->kind = DECL_CONST;

            next_lexed_token();
            if (is_token_kind(TOKEN_NAME))
            {
                declaration->name = token.name;
                next_lexed_token();
            }

            if (expect_token_kind(TOKEN_ASSIGN))
            {
                expr* expr = parse_expr();
                declaration->const_decl.expr = expr;
            }
        }       
        else if (decl_keyword == struct_keyword
                || decl_keyword == union_keyword)
        {
            declaration = push_struct(arena, decl);
            declaration->kind = decl_keyword == struct_keyword ? DECL_STRUCT : DECL_UNION;

            next_lexed_token();
            if (is_token_kind(TOKEN_NAME))
            {
                declaration->name = token.name;
                next_lexed_token();
            }

            expect_token_kind(TOKEN_LEFT_BRACE);

            parse_aggregate_fields(&declaration->aggregate);

            expect_token_kind(TOKEN_RIGHT_BRACE);
        }
        else if (decl_keyword == fn_keyword)
        {
            declaration = push_struct(arena, decl);
            declaration->kind = DECL_FUNCTION;

            next_lexed_token();
            if (is_token_kind(TOKEN_NAME))
            {
                declaration->name = token.name;
                next_lexed_token();
            }

            expect_token_kind(TOKEN_LEFT_PAREN);

            declaration->function.params
                = parse_function_param_list();

            expect_token_kind(TOKEN_RIGHT_PAREN);

            if (is_token_kind(TOKEN_COLON))
            {
                next_lexed_token();
                if (is_token_kind(TOKEN_NAME))
                {
                    declaration->function.return_type = parse_typespec();
                }
            }

            expect_token_kind(TOKEN_LEFT_BRACE);
            
            declaration->function.stmts = parse_statement_block();

            expect_token_kind(TOKEN_RIGHT_BRACE);
        }
        else if (decl_keyword == enum_keyword)
        {
            declaration = push_struct(arena, decl);
            declaration->kind = DECL_ENUM;

            next_lexed_token();
            if (is_token_kind(TOKEN_NAME))
            {
                declaration->name = token.name;
                next_lexed_token();
            }

            expect_token_kind(TOKEN_LEFT_BRACE);

            parse_enum(&declaration->enum_decl);

            expect_token_kind(TOKEN_RIGHT_BRACE);
        }
        else if (decl_keyword == typedef_keyword)
        {
            next_lexed_token();
            declaration = push_struct(arena, decl);
            declaration->kind = DECL_TYPEDEF;

            expr* name_expr = parse_expr();
            assert(name_expr->kind == EXPR_NAME);
            expect_token_kind(TOKEN_ASSIGN);
            typespec* type = parse_typespec();

            declaration->typedef_decl.name = name_expr->name;
            declaration->typedef_decl.type = type;
        }

    }
    return declaration;
}

decl* parse_declaration(void)
{
    decl* d = parse_declaration_optional();
    if (d == 0)
    {
        fatal("expected declaration\n");
    }
    return d;
}

decl* parse_decl(char* str)
{
    init_stream(str);
    while (token.kind)
    {
        next_token();
    }

    get_first_lexed_token();

    decl* result = parse_declaration();
    return result;
}

void parse_text_and_print_s_exprs(char* test)
{
    arena = allocate_memory_arena(megabytes(50));
    
    // musimy najpierw zamienić na ciąg tokenów
    init_stream(test);
    while (token.kind)
    {
        next_token();
    }

    get_first_lexed_token();

    decl* result = parse_declaration();
    print_decl(result);
    printf("\n\n");
  
    free_memory_arena(arena);
}

void parse_test(void)
{
    char* test_str = 0;

    char* test_strs[] = {
#if 1
        "const x = a + -b << c + -d + e >> f",
        "let x := a ^ *b + c * -d + e | -f & g",
        "let x := a >= b || -c * d < e && -f",
        "let x := (&a - &b) + (*c % d)",
        "let x : bool = (a == -b)",
        "let x: int[1 + 2] = {1, 2, 3}",
        "const x = sizeof(t.subt.x)",
        "const n = sizeof(&a[0].b.c[10])",
        "fn f (a: int, b: float, c : int ) : float { return a + b }",
        "fn f () {\
            x += 1\
            y -= 2\
            z %= 3\
            return x + y - z }",
        "struct x { a: int, b: float, c: y }",
        "union some_union { a: int, b: float }",
        "enum some_enum { A = 1, B, C, D = 4 }",
        "fn some_function() : int { let x = 100\
            for (let i = 0; i < x; i++) { x = x + 1 } return x }",
        "let x = 2 + f(1, 2) + g(3) + 4",
        "fn f() { x *= 2 y |= !x z &= 8 w /= z u &&= ~y v ||= z a ^= b } ",
        "fn f(ind: int) { x.arr[ind] = y[ind] }",
        "fn f(): int { let y = fun_1()\
             let z: int = fun_2(y[0], y[y[0] + 1])\
             return y * z }"
        "fn f() { if (*x == 1) { return y } else { return z } }",
        "fn f() {if (function(x)) { return y } else if (y == 2) { return z }}",
        "fn f() { if (x) { x = y } else if (y) {} else if (z) { z = x } else { y = z } }",
        "fn f() { if (sizeof(x) == 4) { return x } }"
        "fn f() { while (x > y) { x = x + 1 } }",
        "fn f() { do { x[index] = x - 1 } while (x < y) }"
        "fn f() {\
            switch (x){\
            case 1: {let x: int = 1 return x } break\
            default: { return 2 } break } } ",
        "fn f() {\
            switch (x){\
            case 1: case 2: { y++ } case 3: case 4: case 5: { y-- }\
            case 6: { return 2 } case 7: {} } }",
        "fn f(x: int*, y: int[25], z: char*[25], w: int**[20] ) : int** { let x : int[256] = 0 } ",
        "struct x { a: int[20], b: float**, c: int*[20]* } ",
        "let x = y == z ? *y : 20",
        "fn fc(x: stru*) : stu { stru.substru = {1, 2} let y = func(12, a, {2, 3, x, stru.substru}) return y }",
        "fn f() { x.y = {z = 2, w = 3, p = 44, q = z, 22} }",
        "typedef vectors = vector[1+2]",
        "typedef t = (fn(int*[2], int):int)[16]",
#endif
        "let x = (v3){1, 2, 3}",
        "let y = f(1, {1, 2}, (v2){1,2})",
        "fn f(x: int, y: int) : vec2 { return (v2){x + 1, y - 1} }",
    };

    int arr_length = sizeof(test_strs) / sizeof(test_strs[0]);
    for (int i = 0; i < arr_length; i++)
    {
        char* str = test_strs[i];
        parse_text_and_print_s_exprs(str);
    }

    debug_breakpoint;
}
#include "vm.h"
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
        fatal("expected token of different kind: %d", kind);
    }
    return result;
}

void print_expression(expr* e)
{
    if (e == NULL)
    {
        return;
    }

    switch (e->kind)
    {
        case EXPR_INT:
        {
            printf(" %d", e->number_value);
        } break;
        case EXPR_NAME:
        {
            printf(" %.1s", e->identifier);
        } break;
        case EXPR_UNARY:
        {
            printf("(");
            print_token_kind(e->binary_expr_value.operator);
            print_expression(e->unary_expr_value.operand);
            printf(")");
        } break;
        case EXPR_BINARY:
        {
            printf("(");
            print_token_kind(e->binary_expr_value.operator);
            print_expression(e->binary_expr_value.left_operand);
            print_expression(e->binary_expr_value.right_operand);
            printf(")");
        } break;
        case EXPR_TERNARY: {} break;
    }
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
    result->identifier = name;
    next_lexed_token();
    return result;
}

expr* push_unary_expr(token_kind operator, expr* operand)
{
    expr* result = push_struct(arena, expr);
    result->kind = EXPR_UNARY;
    result->unary_expr_value.operator = operator;
    result->unary_expr_value.operand = operand;
    return result;
}

expr* push_binary_expr(token_kind operator, expr* left_operand, expr* right_operand)
{
    expr* result = push_struct(arena, expr);
    result->kind = EXPR_BINARY;
    result->binary_expr_value.operator = operator;
    result->binary_expr_value.left_operand = left_operand;
    result->binary_expr_value.right_operand = right_operand;
    return result;
}

expr* parse_expression(void);

expr* parse_base_expression(void)
{
    expr* result = NULL;
    if (is_token_kind(TOKEN_INT))
    {
        result = push_int_expr(token.val);
    }
    else if (is_token_kind(TOKEN_NAME))
    {
        result = push_name_expr(token.name);
    }
    else
    {
        if (match_token_kind('('))
        {
            result = parse_expression();
            expect_token_kind(')');
        }
    }
    return result;
}

expr* parse_unary_expression(void)
{
    expr* e = parse_base_expression();
    if (e == NULL)
    {
        if (match_token_kind('+'))
        {
            e = push_unary_expr('+', parse_base_expression());
        }
        else if (match_token_kind('-'))
        {
            e = push_unary_expr('-', parse_base_expression());
        }
    }
    return e;
}


expr* parse_multiplicative_expression(void)
{
    expr* e = parse_unary_expression();
    while (is_token_kind('*') || is_token_kind('/'))
    {
        expr* left_expr = e;
        token_kind op = token.kind;
        next_lexed_token();
        expr* right_expr = parse_unary_expression();

        e = push_binary_expr(op, left_expr, right_expr);
    }
    return e;
}

expr* parse_additive_expression(void)
{
    expr* e = parse_multiplicative_expression();
    while (is_token_kind('+') || is_token_kind('-'))
    {
        expr* left_expr = e;
        token_kind op = token.kind;
        next_lexed_token();
        expr* right_expr = parse_multiplicative_expression();

        e = push_binary_expr(op, left_expr, right_expr);
    }
    return e;
}

expr* parse_comparison_expression(void)
{
    expr* e = parse_additive_expression();
    while (is_token_kind(TOKEN_EQ) || is_token_kind(TOKEN_NEQ)
        || is_token_kind(TOKEN_GT) || is_token_kind(TOKEN_GEQ) 
        || is_token_kind(TOKEN_LT) || is_token_kind(TOKEN_LEQ))
    {
        expr* left_expr = e;
        token_kind op = token.kind;
        next_lexed_token();
        expr* right_expr = parse_additive_expression();

        e = push_binary_expr(op, left_expr, right_expr);
    }
    return e;
}

expr* parse_and_expression(void)
{
    expr* e = parse_comparison_expression();
    while (is_token_kind(TOKEN_AND))
    {
        expr* left_expr = e;
        token_kind op = token.kind;
        next_lexed_token();
        expr* right_expr = parse_comparison_expression();

        e = push_binary_expr(op, left_expr, right_expr);
    }
    return e;
}

expr* parse_or_expr(void)
{
    expr* e = parse_and_expression();
    while (is_token_kind(TOKEN_OR))
    {
        expr* left_expr = e;
        token_kind op = token.kind;
        next_lexed_token();
        expr* right_expr = parse_and_expression();

        e = push_binary_expr(op, left_expr, right_expr);
    }
    return e;
}

expr* parse_ternary_expression(void)
{
    expr* e = parse_or_expr();
    return e;
}

expr* parse_expression(void)
{
    expr* result = parse_ternary_expression(); 
    return result;
}

decl* parse_declaration(void);
decl* parse_declaration_optional(void);
stmt_block parse_statement_block(void);

// i.e. assign, function invocation or incerement
stmt* parse_simple_statement(void)
{
    // x = x + 1
    // x++
    // x()
 
    stmt* s = 0; 
    char* var = str_intern(token.name);

    next_lexed_token();

    if (is_token_kind(TOKEN_ASSIGN))
    {
        next_lexed_token();
        
        s = push_struct(arena, stmt);
        expr* e = parse_expression();
        s->kind = STMT_ASSIGN;
        s->assign_statement.expr = e;
        s->assign_statement.assigned_var = var;
    }
    else
    {
        debug_breakpoint;
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
            s->return_statement.expression = parse_expression();            
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
        else if (keyword == let_keyword)
        {
            decl* d = parse_declaration();
            s = push_struct(arena, stmt);
            s->decl_statement.decl = d;
            s->kind = STMT_DECL;
        }
        else if (keyword == for_keyword)
        {
            s = push_struct(arena, stmt);
            s->kind = STMT_FOR;
            next_lexed_token();

            expect_token_kind(TOKEN_LEFT_PAREN);

            s->for_statement.init_decl = parse_declaration();

            expect_token_kind(TOKEN_SEMICOLON);

            s->for_statement.cond_expr = parse_expression();

            expect_token_kind(TOKEN_SEMICOLON);

            s->for_statement.incr_expr = parse_expression();

            expect_token_kind(TOKEN_RIGHT_PAREN);
            expect_token_kind(TOKEN_LEFT_BRACE);

            s->for_statement.statements = parse_statement_block();

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
            s->decl_statement.decl = d;

            debug_breakpoint;
        }
        else
        {
            s = parse_simple_statement();
        }
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
        result.statements = copy_buf_to_arena(arena, buf);
        result.statements_count = s_count;
    }
    return result;
}

function_param parse_function_parameter(void)
{
    function_param p = {0};
    if (is_token_kind(TOKEN_NAME))
    {
        p.identifier = token.name;

        next_lexed_token();
        expect_token_kind(TOKEN_COLON);

        if (is_token_kind(TOKEN_NAME))
        {
            p.type = token.name;
            next_lexed_token();
        }
    }
    return p;
}

function_param_list parse_function_parameter_list(void)
{
    function_param* params = NULL;

    while (is_token_kind(TOKEN_NAME))
    {
        function_param p = parse_function_parameter();
        if (p.identifier != NULL)
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
        result.identifier = token.name;

        next_lexed_token();
        expect_token_kind(TOKEN_COLON);

        if (is_token_kind(TOKEN_NAME))
        {
            result.type = token.name;
            next_lexed_token();
        }

        if (is_token_kind(','))
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
        result.identifier = token.name;
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

        if (is_token_kind(','))
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
    while (new_value.identifier)
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
        char* decl_keyword = str_intern(token.name);
        if (decl_keyword == let_keyword)
        {
            declaration = push_struct(arena, decl);
            declaration->kind = DECL_VARIABLE;

            next_lexed_token();
            if (is_token_kind(TOKEN_NAME))
            {
                declaration->identifier = token.name;
                next_lexed_token();
            }

            if (match_token_kind(':'))
            {
                if (is_token_kind(TOKEN_NAME))
                {
                    declaration->variable_declaration.type = token.name;
                    next_lexed_token();
                }
            }

            if (match_token_kind('='))
            {
                expr* expression = parse_expression();
                declaration->variable_declaration.expression = expression;
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
                declaration->identifier = token.name;
                next_lexed_token();
            }

            expect_token_kind('{');

            parse_aggregate_fields(&declaration->aggregate_declaration);

            expect_token_kind('}');
        }
        else if (decl_keyword == fn_keyword)
        {
            declaration = push_struct(arena, decl);
            declaration->kind = DECL_FUNCTION;

            next_lexed_token();
            if (is_token_kind(TOKEN_NAME))
            {
                declaration->identifier = token.name;
                next_lexed_token();
            }

            expect_token_kind('(');

            declaration->function_declaration.parameters
                = parse_function_parameter_list();

            expect_token_kind(')');

            if (is_token_kind(':'))
            {
                next_lexed_token();
                if (is_token_kind(TOKEN_NAME))
                {
                    declaration->function_declaration.return_type = token.name;
                    next_lexed_token();
                }
            }

            expect_token_kind('{');
            
            declaration->function_declaration.statements = parse_statement_block();

            expect_token_kind('}');
        }
        else if (decl_keyword == enum_keyword)
        {
            declaration = push_struct(arena, decl);
            declaration->kind = DECL_ENUM;

            next_lexed_token();
            if (is_token_kind(TOKEN_NAME))
            {
                declaration->identifier = token.name;
                next_lexed_token();
            }

            expect_token_kind('{');

            parse_enum(&declaration->enum_declaration);

            expect_token_kind('}');
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

void print_declaration(decl* declaration);
void print_statement_block(stmt_block block);

void print_statement(stmt* statement)
{
    if (statement == NULL)
    {
        return;
    }

    switch (statement->kind)
    {
        case STMT_RETURN: {
            printf("return: (");
            print_expression(statement->return_statement.expression);
            printf(")");
        }; break;
        case STMT_BREAK: { printf("break "); }; break;
        case STMT_CONTINUE: { printf("continue "); }; break;
        case STMT_PRINT: {}; break;
        case STMT_LIST: {}; break;
        case STMT_IF_ELSE: {}; break;
        case STMT_WHILE: {}; break;
        case STMT_ASSIGN: {
            printf("= (");
            printf(statement->assign_statement.assigned_var);
            print_expression(statement->assign_statement.expr);
            printf(")");
        }; break;
        case STMT_FOR: {
            printf("(for: (");
            print_declaration(statement->for_statement.init_decl);
            print_expression(statement->for_statement.cond_expr);
            print_expression(statement->for_statement.incr_expr);
            printf(")");
            print_statement_block(statement->for_statement.statements);
            printf(")");        
        }; break;
        case STMT_SWITCH: {}; break;
        case STMT_DECL:
        {
            print_declaration(statement->decl_statement.decl);
        }
        break;
        case STMT_EXPRESSION: {
            print_expression(statement->return_statement.expression);
        }; break;
    }
}

void print_statement_block(stmt_block block)
{
    for (int index = 0; index < block.statements_count; index++)
    {
        print_statement(block.statements[index]);
    }
}

void print_function_param(function_param param)
{
    printf("(param name: %s", param.identifier);
    printf(" type: %s)", param.type);
}

void print_declaration(decl* declaration)
{
    if (declaration == NULL)
    {
        return;
    }

    switch (declaration->kind)
    {
        case DECL_FUNCTION: {
            printf("(fn decl");
            if (declaration->identifier)
            {
                printf(" name: %s", declaration->identifier);
            }

            if (declaration->function_declaration.parameters.param_count > 0)
            {
                printf(" params: ");
                for (int index = 0; index < declaration->function_declaration.parameters.param_count; index++)
                {
                    print_function_param(declaration->function_declaration.parameters.params[index]);
                }
            }

            if (declaration->function_declaration.return_type)
            {
                printf(" return type: %s", declaration->function_declaration.return_type);
            }
            printf(" body: (");
            print_statement_block(declaration->function_declaration.statements);
            printf("))");
        }; 
        break;
        case DECL_VARIABLE: {
            printf("(var decl");

            if (declaration->identifier)
            {
                printf(" name: %s", declaration->identifier);
            }

            if (declaration->variable_declaration.type)
            {
                printf(" type: %s", declaration->variable_declaration.type);
            }

            if (declaration->variable_declaration.expression)
            {
                printf(" exp: ");
                print_expression(declaration->variable_declaration.expression);
            }

            printf(")");
        }; 
        break;
        case DECL_STRUCT: 
        case DECL_UNION: 
        {
            if (declaration->kind == DECL_UNION)
            {
                printf("(union decl");
            }
            else
            {
                printf("(struct decl");
            }

            if (declaration->identifier)
            {
                printf(" name: %s", declaration->identifier);
            }
            
            for (size_t index = 0; 
                index < declaration->aggregate_declaration.fields_count;
                index++)
            {
                printf("(name: %s", declaration->aggregate_declaration.fields[index].identifier);
                printf(" type: %s)", declaration->aggregate_declaration.fields[index].type);
            }
        
            printf(")");
        }; 
        break;
        case DECL_ENUM:
        {
            printf("(enum decl");

            for (size_t index = 0;
                index < declaration->enum_declaration.values_count;
                index++)
            {
                enum_value* value = &declaration->enum_declaration.values[index];
                printf("(flag: %s", value->identifier);
                if (value->value_set)
                {
                    printf(" value: %d", value->value);
                }
                printf(")");
            }

            printf(")");
        };
        break;
    }
}

void parse_text_and_print_s_expressions(char* test)
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
    print_declaration(result);
    printf("\n\n");
  
    debug_breakpoint;

    free_memory_arena(arena);
}

void test_parsing(void)
{
    char* test_str = 0;

    char* test_strs[] = {
        /*
        */
        "let x = a + -b + c + -d + e + f",    
        "let x = a * b + -c * d + e * -f",    
        "let x = a >= b || -c * d < e && -f", 
        "let x = (a - b) + (c / d)",
        "let x : float = (a == -b)",
        "fn f (a: int, b: float, c : int ) : float { return a + b }",
        "fn f () {\
            let x = 1\
            let y = 2\
            return x + y }",
        "struct x { a: int, b: float, c: y }",
        "union some_union { a: int, b: float }",    
        "enum some_enum { A = 1, B, C, D = 4 }",
        "fn some_function() : int { let x = 100\
            for (let i = 0; i < x; i++) { x = x + 1 } return x }",
    };

    int arr_length = sizeof(test_strs) / sizeof(test_strs[0]);
    for (int i = 0; i < arr_length; i++)
    {
        char* str = test_strs[i];
        parse_text_and_print_s_expressions(str);

        debug_breakpoint;
    }

    debug_breakpoint;

    /*
        what left:
        * switch
        * while, while do
        * ifs, else ifs, sizeof
        * decrement, increment, various assigns, e. g. +=
        * function invocation
        * nested structs and unions
    */

    /* 
        test_str = "struct nested_struct { struct x { a: int }, struct y { b: uint } }";
        test_str = "union nested_union { struct x { a: int }, struct y { b: uint } }";
    */    
}
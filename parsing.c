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

expr* parse_expression();

expr* parse_base_expression()
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

expr* parse_unary_expression()
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


expr* parse_multiplicative_expression()
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

// a + b + c + d + e + f
// parsujemy jako
// (((((a + b) + c) + d) + e) + f)

expr* parse_additive_expression()
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

expr* parse_comparison_expression()
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

expr* parse_and_expression()
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

expr* parse_or_expr()
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

expr* parse_ternary_expression()
{
    expr* e = parse_or_expr();
    return e;
}

expr* parse_expression()
{
    expr* result = parse_ternary_expression(); 
    return result;
}

//stmt* parse_statement()
//{
//    // return
//    // break
//    // continue
//    // print
//    // 
//
//    if ()
//    {
//
//    }
//    else
//    {
//        parse_expression();
//        if () //inc dec assign
//        {
//
//        }
//    }
//}

decl* parse_declaration()
{
    decl* declaration = NULL;
    if (is_token_kind(TOKEN_KEYWORD))
    {
        if (str_intern(token.name) == str_intern("let"))
        {
            declaration = push_struct(arena, decl);
            declaration->kind = DECL_VARIABLE;

            next_lexed_token();
            if (is_token_kind(TOKEN_NAME))
            {
                declaration->variable_declaration.identifier = token.name;
                next_lexed_token();
            }
            else
            {
                // błąd
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
        else if (str_intern(token.name) == str_intern("struct"))
        {
            declaration = push_struct(arena, decl);
            declaration->kind = DECL_STRUCT;

            next_lexed_token();
        }
        else if (str_intern(token.name) == str_intern("fn"))
        {
            declaration = push_struct(arena, decl);
            declaration->kind = DECL_FUNCTION;

            next_lexed_token();
        }
    }
    return declaration;
}


void print_declaration(decl* declaration)
{
    switch (declaration->kind)
    {
        case DECL_FUNCTION: {}; break;
        case DECL_VARIABLE: {
            printf("declaration");
            if (declaration->variable_declaration.identifier)
            {
                printf(" name: %s", declaration->variable_declaration.identifier);
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
        
        }; break;
        case DECL_STRUCT: {}; break;
    }
}

void parse_text_and_print_s_expressions(char* test, bool parse_as_declaration)
{
    arena = allocate_memory_arena(megabytes(50));

    // musimy najpierw zamienić na ciąg tokenów
    init_stream(test);
    while (token.kind)
    {
        next_token();
    }

    get_first_lexed_token();

    if (parse_as_declaration)
    {
        decl* result = parse_declaration();
        print_declaration(result);
        printf("\n\n");
    }
    else
    {
        decl* result = parse_expression();
        print_expression(result);
        printf("\n\n");
    }
  
    debug_breakpoint;

    free_memory_arena(arena);
}

void test_parsing()
{
    char* test_str = "a + -b + c + -d + e + f";
    parse_text_and_print_s_expressions(test_str, false);
       
    test_str = "a * b + -c * d + e * -f";
    parse_text_and_print_s_expressions(test_str, false);

    test_str = "a >= b || -c * d < e && -f";
    parse_text_and_print_s_expressions(test_str, false);

    test_str = "let variable = (a - b) + (c / d)";
    parse_text_and_print_s_expressions(test_str, true);

    debug_breakpoint;

 /* 

    test_str = "y + (x - y)";
    parse_text_and_print_s_expressions(test_str);

    test_str = "if (x == y) { x = y - 1 } ";
    parse_text_and_print_s_expressions(test_str);

    test_str = 
"for(int i = 0; i < length; i++)\
{\
    print i\
}";
    parse_text_and_print_s_expressions(test_str);*/

}
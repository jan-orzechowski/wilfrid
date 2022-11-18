#include "vm.h"
#include "parsing.h"
#include "lexing.h"

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

void parse_expr(void);

void parse_expr3(void)
{
    if (is_token_kind(TOKEN_INT))
    {
        int value = token.val;
        next_lexed_token();
        buf_push(code, PUSH);
        buf_push(code, value);
    }
    else
    {
        if (is_token_kind(TOKEN_LEFT_PAREN))
        {
            next_lexed_token();
            parse_expr();
            if (expect_token_kind(TOKEN_RIGHT_PAREN))
            {
                // skończyliśmy
            }
        }
    }
}

void parse_expr2(void)
{
    if (is_token_kind(TOKEN_ADD) || is_token_kind(TOKEN_SUB))
    {
        token_kind operation = token.kind;
        next_lexed_token();
        parse_expr2();
        if (operation == TOKEN_SUB)
        {
            buf_push(code, PUSH);
            buf_push(code, -1);
            buf_push(code, MUL);
        }
    }
    else
    {
        parse_expr3();
    }
}

void parse_expr1(void)
{
    parse_expr2();
    while (is_token_kind(TOKEN_MUL) || is_token_kind(TOKEN_DIV))
    {
        token_kind operation = token.kind;
        next_lexed_token();
        parse_expr2();
        if (operation == TOKEN_MUL)
        {
            buf_push(code, MUL);
        }
        else
        {
            buf_push(code, DIV);
        }
    }
}

void parse_expr0(void)
{
    parse_expr1();
    while (is_token_kind(TOKEN_ADD) || is_token_kind(TOKEN_SUB))
    {
        token_kind operation = token.kind;
        next_lexed_token();
        parse_expr1();
        if (operation == TOKEN_ADD)
        {
            buf_push(code, ADD);
        }
        else
        {
            buf_push(code, SUB);
        }
    }
}

void parse_expression(void)
{
    parse_expr0();
}
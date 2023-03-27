﻿#pragma once

typedef enum token_kind
{
    TOKEN_EOF,
    TOKEN_NEWLINE, // używany do synchronizacji w wypadku błędu parsowania
    TOKEN_COMMA,
    TOKEN_DOT,
    TOKEN_COLON,
    TOKEN_QUESTION,
    TOKEN_LEFT_PAREN,
    TOKEN_RIGHT_PAREN,
    TOKEN_LEFT_BRACKET, // [
    TOKEN_RIGHT_BRACKET, // ]
    TOKEN_LEFT_BRACE, // {
    TOKEN_RIGHT_BRACE, // }
    TOKEN_NOT, // !
    TOKEN_BITWISE_NOT, // ~

    // variables
    TOKEN_KEYWORD,
    TOKEN_INT,
    TOKEN_FLOAT,
    TOKEN_STRING,
    TOKEN_CHAR,
    TOKEN_NAME,

    // multiplicative precedence
    TOKEN_FIRST_MUL_OPERATOR,
    TOKEN_MUL = TOKEN_FIRST_MUL_OPERATOR,
    TOKEN_DIV,
    TOKEN_MOD,
    TOKEN_BITWISE_AND, // &
    TOKEN_LEFT_SHIFT,
    TOKEN_RIGHT_SHIFT,
    TOKEN_LAST_MUL_OPERATOR = TOKEN_RIGHT_SHIFT,

    // additive precedence
    TOKEN_FIRST_ADD_OPERATOR,
    TOKEN_ADD = TOKEN_FIRST_ADD_OPERATOR,
    TOKEN_SUB,
    TOKEN_XOR, // ^
    TOKEN_BITWISE_OR, // |
    TOKEN_LAST_ADD_OPERATOR = TOKEN_BITWISE_OR,

    // comparative precedence
    TOKEN_FIRST_CMP_OPERATOR,
    TOKEN_EQ = TOKEN_FIRST_CMP_OPERATOR, // ==
    TOKEN_NEQ, // !=
    TOKEN_GT, // >
    TOKEN_LT, // <
    TOKEN_GEQ, // >=
    TOKEN_LEQ, // <=
    TOKEN_LAST_CMP_OPERATOR = TOKEN_LEQ,

    TOKEN_AND, // 'and' or &&
    TOKEN_OR, // 'or' or ||

    // assignment operators
    TOKEN_FIRST_ASSIGN_OPERATOR,
    TOKEN_ASSIGN = TOKEN_FIRST_ASSIGN_OPERATOR,
    TOKEN_ADD_ASSIGN,
    TOKEN_SUB_ASSIGN,
    TOKEN_OR_ASSIGN,
    TOKEN_AND_ASSIGN,
    TOKEN_BITWISE_OR_ASSIGN,
    TOKEN_BITWISE_AND_ASSIGN,
    TOKEN_XOR_ASSIGN,
    TOKEN_LEFT_SHIFT_ASSIGN,
    TOKEN_RIGHT_SHIFT_ASSIGN,
    TOKEN_MUL_ASSIGN,
    TOKEN_DIV_ASSIGN,
    TOKEN_MOD_ASSIGN,
    TOKEN_LAST_ASSIGN_OPERATOR = TOKEN_MOD_ASSIGN,

    // inne
    TOKEN_COLON_ASSIGN, // :=
    TOKEN_INC, // ++
    TOKEN_DEC, // --

} token_kind;

const char *token_kind_names[] = {

    [TOKEN_EOF] = "EOF",
    [TOKEN_NEWLINE] = "NEWLINE",
    [TOKEN_COLON] = ":",
    [TOKEN_LEFT_PAREN] = "(",
    [TOKEN_RIGHT_PAREN] = ")",
    [TOKEN_LEFT_BRACE] = "{",
    [TOKEN_RIGHT_BRACE] = "}",
    [TOKEN_LEFT_BRACKET] = "[",
    [TOKEN_RIGHT_BRACKET] = "]",
    [TOKEN_COMMA] = ",",
    [TOKEN_DOT] = ".",
    [TOKEN_QUESTION] = "?",
    [TOKEN_NOT] = "!",
    [TOKEN_BITWISE_NOT] = "~",

    [TOKEN_KEYWORD] = "keyword",
    [TOKEN_INT] = "int",
    [TOKEN_FLOAT] = "float",
    [TOKEN_STRING] = "string",
    [TOKEN_NAME] = "name",
    [TOKEN_CHAR] = "char",

    [TOKEN_MUL] = "*",
    [TOKEN_DIV] = "/",
    [TOKEN_MOD] = "%",
    [TOKEN_BITWISE_AND] = "&",
    [TOKEN_LEFT_SHIFT] = "<<",
    [TOKEN_RIGHT_SHIFT] = ">>",

    [TOKEN_ADD] = "+",
    [TOKEN_SUB] = "-",
    [TOKEN_BITWISE_OR] = "|",
    [TOKEN_XOR] = "^",

    [TOKEN_EQ] = "==",
    [TOKEN_NEQ] = "!=",
    [TOKEN_LT] = "<",
    [TOKEN_GT] = ">",
    [TOKEN_LEQ] = "<=",
    [TOKEN_GEQ] = ">=",

    [TOKEN_AND] = "&&",
    [TOKEN_OR] = "||",

    [TOKEN_ASSIGN] = "=",
    [TOKEN_ADD_ASSIGN] = "+=",
    [TOKEN_SUB_ASSIGN] = "-=",
    [TOKEN_OR_ASSIGN] = "||=",
    [TOKEN_AND_ASSIGN] = "&&=",
    [TOKEN_BITWISE_OR_ASSIGN] = "|=",
    [TOKEN_BITWISE_AND_ASSIGN] = "&=",
    [TOKEN_XOR_ASSIGN] = "^=",
    [TOKEN_MUL_ASSIGN] = "*=",
    [TOKEN_DIV_ASSIGN] = "/=",
    [TOKEN_MOD_ASSIGN] = "%=",
    [TOKEN_LEFT_SHIFT_ASSIGN] = "<<=",
    [TOKEN_RIGHT_SHIFT_ASSIGN] = ">>=",

    [TOKEN_COLON_ASSIGN] = ":=",
    [TOKEN_INC] = "++",
    [TOKEN_DEC] = "--",
};

typedef struct token
{
    token_kind kind;
    source_pos pos;
    const char *start;
    const char *end;
    union
    {
        int val;
        const char *name;
        const char *string_val;
    };
} token;

const char *get_token_kind_name(token_kind kind)
{
    if (kind < sizeof(token_kind_names) / sizeof(*token_kind_names))
    {
        return token_kind_names[kind];
    }
    else
    {
        return 0;
    }
}
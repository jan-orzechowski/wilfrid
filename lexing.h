#pragma once

typedef enum token_kind
{
    TOKEN_EOF = 0,
    TOKEN_STAR = 42,
    TOKEN_PLUS = 43,
    TOKEN_MINUS = 45,
    // pierwsze 128 wartości jest dla one-character tokens
    TOKEN_LAST_CHAR = 127,
    TOKEN_INT,
    TOKEN_NAME,
    TOKEN_GT, // >
    TOKEN_LT, // <
    TOKEN_GEQ, // >=
    TOKEN_LEQ, // <=
    TOKEN_EQ, // ==
    TOKEN_NEQ, // !=
    TOKEN_AND, // 'and' or &&
    TOKEN_OR, // 'or' or ||
    // ...
}
token_kind;

typedef struct tok
{
    token_kind kind;
    const char* start;
    const char* end;
    union
    {
        int val;
        const char* name;
    };
}
tok;

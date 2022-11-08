#pragma once

typedef enum token_kind
{
    TOKEN_EOF = 0,
    // pierwsze 128 wartości jest dla characters
    TOKEN_LAST_CHAR = 127,
    TOKEN_INT,
    TOKEN_NAME,
    TOKEN_LEFT_PAREN,
    TOKEN_RIGHT_PAREN,
    TOKEN_ADD,
    TOKEN_SUB,
    TOKEN_MUL,
    TOKEN_DIV
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

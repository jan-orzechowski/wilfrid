#pragma once

typedef enum token_kind
{
    TOKEN_EOF = 0,
    TOKEN_STAR = 42,
    TOKEN_PLUS = 43,
    TOKEN_COMMA = 44,
    TOKEN_MINUS = 45,
    TOKEN_COLON = 58,
    TOKEN_ASSIGN = 61,
    TOKEN_LEFT_PAREN = 40,
    TOKEN_RIGHT_PAREN = 41,
    TOKEN_BACKSLASH = 47,
    // pierwsze 128 wartości jest dla one-character tokens
    TOKEN_LAST_CHAR = 127,
    TOKEN_INT,
    TOKEN_NAME,
    TOKEN_KEYWORD,
    TOKEN_SHORT_ASSIGNMENT, // :=
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

typedef enum keywords
{
    KEYWORD_RETURN,
    KEYWORD_BREAK,
    KEYWORD_CONTINUE,
    KEYWORD_PRINT,
    KEYWORD_IF,
    KEYWORD_ELSE,
    KEYWORD_FOR,
    KEYWORD_WHILE,
    KEYWORD_DO,
    KEYWORD_SIZEOF,
    KEYWORD_SWITCH,
    KEYWORD_CASE,
    KEYWORD_DEFAULT,
    KEYWORD_ENUM,
    KEYWORD_STRUCT,
    KEYWORD_UNION,
    KEYWORD_LET,
    KEYWORD_FN,
} keywords;
#pragma once

// istotne jest to, że wartości enuma nigdy się nie zmniejszają
// np. jesti TOKEN_A wynosi 1000, to zdefiniowany niżej TOKEN_B będzie wynosił 1001
typedef enum token_kind
{
    TOKEN_EOF,
    TOKEN_COMMA,
    TOKEN_DOT,
    TOKEN_COLON,
    TOKEN_SEMICOLON,
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

const char* token_kind_names[] = {
 
    [TOKEN_EOF] = "EOF",
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
    [TOKEN_SEMICOLON] = ";",
    [TOKEN_NOT] = "!",
    [TOKEN_BITWISE_NOT] = "~",

    [TOKEN_KEYWORD] = "keyword",
    [TOKEN_INT] = "int",
    [TOKEN_FLOAT] = "float",
    [TOKEN_STRING] = "string",
    [TOKEN_NAME] = "name",

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

typedef struct tok
{
    token_kind kind;
    const char* start;
    const char* end;
    union
    {
        int val;
        const char* name;
        const char* string_val;
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
    KEYWORD_TYPEDEF,
    KEYWORD_LET,
    KEYWORD_FN,
} keywords;
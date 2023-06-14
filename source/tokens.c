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

    // other
    TOKEN_COLON_ASSIGN, // :=
    TOKEN_INC, // ++
    TOKEN_DEC, // --
    TOKEN_ADDRESS_OF, // @
    TOKEN_DEREFERENCE, // #
    TOKEN_POINTER = TOKEN_XOR, // ^
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
    [TOKEN_ADDRESS_OF] = "@",
    [TOKEN_DEREFERENCE] = "#",

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
        uint64_t uint_val;
        int64_t int_val;
        double float_val;
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

uint8_t char_to_digit[256] =
{
    ['0'] = 0,
    ['1'] = 1,
    ['2'] = 2,
    ['3'] = 3,
    ['4'] = 4,
    ['5'] = 5,
    ['6'] = 6,
    ['7'] = 7,
    ['8'] = 8,
    ['9'] = 9,
    ['a'] = 10, ['A'] = 10,
    ['b'] = 11, ['B'] = 11,
    ['c'] = 12, ['C'] = 12,
    ['d'] = 13, ['D'] = 13,
    ['e'] = 14, ['E'] = 14,
    ['f'] = 15, ['F'] = 15,
};

token_kind get_assignment_operation_token(token_kind assignment)
{
    assert(assignment >= TOKEN_FIRST_ASSIGN_OPERATOR);
    assert(assignment <= TOKEN_LAST_ASSIGN_OPERATOR);
    switch (assignment)
    {
        case TOKEN_ASSIGN: return TOKEN_ASSIGN;
        case TOKEN_ADD_ASSIGN: return TOKEN_ADD;
        case TOKEN_SUB_ASSIGN: return TOKEN_SUB;
        case TOKEN_OR_ASSIGN: return TOKEN_OR;
        case TOKEN_AND_ASSIGN: return TOKEN_AND;
        case TOKEN_BITWISE_OR_ASSIGN: return TOKEN_BITWISE_OR;
        case TOKEN_BITWISE_AND_ASSIGN: return TOKEN_BITWISE_AND;
        case TOKEN_XOR_ASSIGN: return TOKEN_XOR;
        case TOKEN_LEFT_SHIFT_ASSIGN: return TOKEN_LEFT_SHIFT;
        case TOKEN_RIGHT_SHIFT_ASSIGN:  return TOKEN_RIGHT_SHIFT;
        case TOKEN_MUL_ASSIGN:return TOKEN_MUL;
        case TOKEN_DIV_ASSIGN: return TOKEN_DIV;
        case TOKEN_MOD_ASSIGN:return TOKEN_MOD;
        invalid_default_case;
    }
    return TOKEN_EOF;
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